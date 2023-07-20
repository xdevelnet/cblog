#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sys/stat.h>
#include <dirent.h>
#include <iso646.h>
#include <errno.h>
#include <stdio.h>
#include <sys/mman.h>
#include <inttypes.h>
#include "fileno_util.c"

#define DEFAULT_FILE_MODE (S_IREAD | S_IWRITE | S_IRGRP | S_IROTH)

const char fileno_data_dir[] = "data";
const char fileno_datasource_dir[] = "html";
const char fileno_tag_dir[] = "tags";
const char fileno_keyval_dir[] = "sessions";
const char fileno_users_dir[] = "users";
const char fileno_rbac_dir[] = "rbac";
const char fileno_last_record_file[] = "last_record";

/* This engine is operating with the following directory structure
 *
 *  /dir/
 *           data/
 *                first_file
 *                second_file
 *                third_file
 *                fourth_file
 *                fifth_file
 *           datasource/
 *                first_file.html
 *                second_file.html
 *                third_file.html
 *                fourth_file.html
 *                fifth_file.html
 *           tags/
 *                travel/
 *                       1
 *                photos/
 *                       2
 *                       3
 *           sessions/
 *                    session_dasiodsaiodnas
 *                    session_cxzkclzxncckzz
 *           users/
 *                 1
 *                 2
 *                 3
 *                 last_user
 *           1
 *           2
 *           3
 *           4
 *           5
 *           last_record
 *
 * In this case, 1,2,3,4,5 are metadata files. They could point to any of
 * files inside data/ or html/ files.
 * data/ could be any dir with any name, but "data/" and "html/" are
 * not configurable on runtime.
 *
 * last_record is a file which should contain unsigned integer (at least at
 * file's start). It should describe the name of new symlink for a new file
 * after each insert. In this case, last_record should contain "6" (without
 * double quotes)
 *
 * tags/ is a directory with multiple directories. Each directory is tag name.
 * Each symlink inside is a member of each tag.
 *
 * sessions/ is a directory with key-value storage. Each key is filename, value
 * is a file content.
 */

#if !defined strizeof
#define strizeof(a) (sizeof(a)-1)
#endif

struct fileno_context {
	const void *addr;
	datalayer_rand_fun randfun;
	int dfd;
	int datafd;
	int datasourcefd;
	int tagsfd;
	int keyvalfd;
	int users;
	int rbac;
};

#define OUCH_ERROR(errmsg, action) do {if (error) *error = errmsg; action;} while(0)


static bool initialize_fileno_context(struct data_layer *d, const char **error) { // d->addr, d->context
	char *path = realpath(d->addr, NULL);
	if (path == NULL) OUCH_ERROR("Request address for engine has not found", return false);
	size_t realpathlen = strlen(path);
	free(path);
	size_t maxlen = CBL_MAX(strizeof(fileno_data_dir), CBL_MAX(strizeof(fileno_tag_dir), strizeof(fileno_datasource_dir)));

	if (realpathlen + sizeof('/') + maxlen > PATH_MAX) {
		OUCH_ERROR("You managed to put storage files so deep, that resulting path length may reach 4096 bytes! OMG!", return false);
	}

	struct fileno_context *ret = d->context;
	ret->randfun = d->randfun;
	ret->addr = d->addr;
	ret->dfd = open(d->addr, O_DIRECTORY | O_RDONLY);
	ret->datafd = -1;
	ret->datasourcefd = -1;
	ret->keyvalfd = -1;
	ret->users = -1;
	ret->rbac = -1;

	if (ret->dfd < 0) goto fail;
	if (mkdirat(ret->dfd, fileno_data_dir, 0700) != 0 and errno != EEXIST) goto fail;
	if (mkdirat(ret->dfd, fileno_datasource_dir, 0700) != 0 and errno != EEXIST) goto fail;
	if (mkdirat(ret->dfd, fileno_tag_dir, 0700) != 0 and errno != EEXIST) goto fail;
	if (mkdirat(ret->dfd, fileno_keyval_dir, 0700) != 0 and errno != EEXIST) goto fail;
	if (mkdirat(ret->dfd, fileno_users_dir, 0700) != 0 and errno != EEXIST) goto fail;
	if (mkdirat(ret->dfd, fileno_rbac_dir, 0700) != 0 and errno != EEXIST) goto fail;
	ret->datafd = openat(ret->dfd, fileno_data_dir, O_DIRECTORY | O_RDONLY);
	ret->datasourcefd = openat(ret->dfd, fileno_datasource_dir, O_DIRECTORY | O_RDONLY);
	ret->tagsfd = openat(ret->dfd, fileno_tag_dir, O_DIRECTORY | O_RDONLY);
	ret->keyvalfd = openat(ret->dfd, fileno_keyval_dir, O_DIRECTORY | O_RDONLY);
	ret->users = openat(ret->dfd, fileno_users_dir, O_DIRECTORY | O_RDONLY);
	ret->rbac = openat(ret->dfd, fileno_rbac_dir, O_DIRECTORY | O_RDONLY);
	if (ret->datafd < 0 or ret->datasourcefd < 0 or ret->tagsfd < 0 or ret->keyvalfd < 0 or ret->users < 0 or ret->rbac < 0) goto fail;
	return true;

fail:
	close(ret->dfd);
	close(ret->datafd);
	close(ret->datasourcefd);
	close(ret->keyvalfd);
	close(ret->users);
	close(ret->rbac);
	OUCH_ERROR(strerror(errno), return false);
}

void deinitialize_engine_fileno(void *context) {
	struct fileno_context *ret = context;
	close(ret->dfd);
	close(ret->datafd);
	close(ret->datasourcefd);
	close(ret->keyvalfd);
	close(ret->users);
	close(ret->rbac);
}

struct fileno_scandir_pass {
	unix_epoch from;
	unix_epoch to;
	int dirfd;
};

static int comparator(const struct dirent **d1, const struct dirent **d2, void *pass) {
	struct fileno_scandir_pass *fpass = pass;
	struct stat s[2];
	fstatat(fpass->dirfd, (*d1)->d_name, s, 0);
	fstatat(fpass->dirfd, (*d2)->d_name, s + 1, 0);

	if (s[0].st_mtim.tv_sec == s[1].st_mtim.tv_sec and s[0].st_mtim.tv_nsec == s[1].st_mtim.tv_nsec) return 0;
	if (abiggerb_timespec(s[0].st_mtim, s[1].st_mtim)) return 1;
	return -1;
}

static void scanfree(struct dirent **e, int amount) {
	for (int i = 0; i < amount; i++) {
		free(e[i]);
	}
	free(e);
}

// This filter is NOT REENTRANT! Use scandir() with mutextes to make it thread-safe
static int filter_fun(const struct dirent *d, void *pass) {
	static const int keep = 1;
	static const int away = 0;

	if (is_str_unsignedint(d->d_name) == false) return away;
	struct fileno_scandir_pass *fpass = pass;
	struct stat s;
	fstatat(fpass->dirfd, d->d_name, &s, 0);
	if (s.st_mtim.tv_sec < fpass->from.t) return away;
	if (s.st_mtim.tv_sec > fpass->to.t) return away;
	return keep;
}

// SELECT record_id from records WHERE modified_time > from and modified_time < to LIMIT amount OFFSET offset sort by modified_time;
bool list_records_fileno(unsigned *amount,// Pointer that could be used for limiting amount of results in list. After executing places amount of results.
						unsigned long *result_list, // Array that will be filled with results
						unsigned offset,            // Skip some amount rows/records/results
						struct list_filter filter,  // filter selection
						void *context,
						const char **error) {
	struct fileno_context *f = context;

	int scanfd = f->dfd;
	const char *scanaddr = f->addr;

	char path[PATH_MAX];
	do {
		if (filter.tags == NULL or filter.tags[0] == NULL) break;
		int dfd = openat(f->tagsfd, filter.tags[0], O_DIRECTORY | O_RDONLY); // that's the main limitation of fileno engine - we only support filtering by one tag
		if (dfd < 0) {
			*amount = 0;
			break;
		}
		snprintf(path, PATH_MAX, "%s/tags/%s", scanaddr, filter.tags[0]);
		scanfd = dfd;
		scanaddr = path;
	} while(0);

	struct dirent **dirent;
	struct fileno_scandir_pass fpass = {.from = filter.from, .to = filter.to, .dirfd = scanfd};
	int ret = scandir_r(scanaddr, &dirent, filter_fun, comparator, &fpass);

	unsigned limit = *amount; // 8 lines below could be refactored... Or now. IDK.
	*amount = 0;

	if (ret < 0) OUCH_ERROR(strerror(errno), return false);
	if (ret == 0 or (unsigned) ret <= offset) {scanfree(dirent, ret); return true;}

	for (unsigned i = offset; i < (unsigned) ret and *amount < limit; i++) {
		result_list[*amount] = strtoul(dirent[i]->d_name, NULL, 10);
		++*amount;
	}

	scanfree(dirent, ret);
	return true;
}

static inline void *map_whole_file(int fd, size_t *size, int flag, const char **error) {
	// above
	// Map whole file from fd

	struct stat st;
	int rval = fstat(fd, &st);
	if (rval < 0) OUCH_ERROR(strerror(errno), return NULL);
	if (st.st_size <= 0) OUCH_ERROR(data_layer_error_metadata_corrupted, return NULL);
	*size = (size_t) st.st_size;

	void *retval = mmap(NULL, *size, PROT_READ | ((flag == MAP_PRIVATE) ? PROT_WRITE : 0), flag, fd, 0);
	if (retval == MAP_FAILED) OUCH_ERROR(strerror(errno), return NULL);
	return retval;
}

void *map_whole_file_shared(int fd, size_t *size, const char **error) {
	return map_whole_file(fd, size, MAP_SHARED, error);
}

void *map_whole_file_private(int fd, size_t *size, const char **error) {
	return map_whole_file(fd, size, MAP_PRIVATE, error);
}

static size_t taglen(const char *str) {
	/*
	 *    string: tag1   ,   tag2   ,    tag3
	 *            ^  ^^ ^^
	 * each step: 1  45 32
	 */

	const char *ptr = str; //1
	while(*ptr != ',' and *ptr != '\n') ptr++; // 2
	ptr--; // 3
	while(*ptr == ' ') ptr--; // 4
	ptr++; // 5

	return ptr - str;
}

static char *tag_processing(char *comma_separated_tags, struct blog_record *r) {
	size_t tags_amount = char_occurences(comma_separated_tags, ',') + 1;
	size_t stack_ptrs_space = sizeof(void *) * (tags_amount + 1);
	char *put = (char *) r->stack + stack_ptrs_space;
	r->tags = r->stack;
	r->stack += stack_ptrs_space;
	r->stack_space -= stack_ptrs_space;
	r->tags[tags_amount] = NULL;
	for (size_t i = 0; i < tags_amount; i++) {
		comma_separated_tags = skip_spaces(comma_separated_tags);
		size_t l = taglen(comma_separated_tags);
		r->tags[i] = memcpy(put, comma_separated_tags, l);
		r->tags[i][l] = '\0';
		r->stack += l + 1;
		r->stack_space -= l + 1;
		put += l + 1;
		comma_separated_tags += l;
		comma_separated_tags = skip_spaces(comma_separated_tags);
		comma_separated_tags++;
	}

	return put;
}

#define METADATA_VER "METADATA1"
#define METADATA_FMT_WO_TAGS METADATA_VER "\ndisplay: %s\nunix access: %03"PRIu32"\nuser id: %"PRIu32"\ngroup id: %"PRIu32"\ntitle: %.*s" \
                                          "\ndata: %s\ndatasource: %s\ncreation_unixepoch: %lu\nmodificated_unixepoch: %lu\n"


struct metadata_strings {
	void *meta;
	size_t metalen;

	char *display;
	char *unix_access;
	char *user_id;
	char *group_id;
	char *title;
	char *data;
	char *datasource;
	char *tags;
	char *creation_unixepoch;
	char *modificated_unixepoch;
};

bool parse_metadata(int fd, struct metadata_strings *meta_strings, const char **error) {
	/* metadata format:
	 *
	 * METADATA1
	 * display: source | data | both | none
	 * unix access: object rights
	 * user id: id of owner/creator of object
	 * group id: id of group that file is belong to, might be 0 if not required
	 * title: The actual title that will be displayed
	 * data: filename_in_data_directory
	 * datasource: filename_in_datasource_directory
	 * tags: first tag, second tag
	 * creation_unixepoch: 1652107591
	 *
	 * strict requirements for metadata:
	 * 1. \n (newline) at the end of file
	 * 2. "display: " and other keys must be just like they are presented, so
	 *    absent space, uppercase, and other derivatives are strictly prohibited
	 * 3. All fields are required, even if their value could be empty, just put it like this:
	 *    tags:
	 *    creation_unixepoch: 1652107592
	 */

	struct stat s;
	if (fstat(fd, &s) < 0) OUCH_ERROR(strerror(errno), return false);
	if (s.st_size < (ssize_t) strizeof(METADATA_FMT_WO_TAGS)) OUCH_ERROR(data_layer_error_metadata_corrupted, return false);

	const char meta_header[] = METADATA_VER;
	const char meta_display[] = "\ndisplay: ";
	const char meta_unix_access[] = "\nunix access: ";
	const char meta_user_id[] = "\nuser id: ";
	const char meta_group_id[] = "\ngroup id: ";
	const char meta_title[] = "\ntitle: ";
	const char meta_data[] = "\ndata: ";
	const char meta_datasource[] = "\ndatasource: ";
	const char meta_tags[] = "\ntags: ";
	const char meta_creation_unixepoch[] = "\ncreation_unixepoch: ";
	const char meta_modificated_unixepoch[] = "\nmodificated_unixepoch: ";

	meta_strings->meta = map_whole_file_shared(fd, &(meta_strings->metalen), error);
	if (meta_strings->meta == NULL) return false;

	if (((char *) meta_strings->meta)[meta_strings->metalen - 1] != '\n' or memcmp(meta_strings->meta, meta_header, strizeof(meta_header)) != STREQ) {
		OUCH_ERROR(data_layer_error_metadata_corrupted, return false);
	}

	meta_strings->display = util_memmem(meta_strings->meta, meta_strings->metalen, meta_display, strizeof(meta_display));
	meta_strings->unix_access = util_memmem(meta_strings->meta, meta_strings->metalen, meta_unix_access, strizeof(meta_unix_access));
	meta_strings->user_id = util_memmem(meta_strings->meta, meta_strings->metalen, meta_user_id, strizeof(meta_user_id));
	meta_strings->group_id = util_memmem(meta_strings->meta, meta_strings->metalen, meta_group_id, strizeof(meta_group_id));
	meta_strings->title = util_memmem(meta_strings->meta, meta_strings->metalen, meta_title, strizeof(meta_title));
	meta_strings->data = util_memmem(meta_strings->meta, meta_strings->metalen, meta_data, strizeof(meta_data));
	meta_strings->datasource = util_memmem(meta_strings->meta, meta_strings->metalen, meta_datasource, strizeof(meta_datasource));
	meta_strings->tags = util_memmem(meta_strings->meta, meta_strings->metalen, meta_tags, strizeof(meta_tags));
	meta_strings->creation_unixepoch = util_memmem(meta_strings->meta, meta_strings->metalen, meta_creation_unixepoch, strizeof(meta_creation_unixepoch));
	meta_strings->modificated_unixepoch = util_memmem(meta_strings->meta, meta_strings->metalen, meta_modificated_unixepoch, strizeof(meta_modificated_unixepoch));

	if (meta_strings->display == NULL or
		meta_strings->unix_access == NULL or
		meta_strings->user_id == NULL or
		meta_strings->group_id == NULL or
		meta_strings->title == NULL or
		meta_strings->data == NULL or
		meta_strings->datasource == NULL or
		meta_strings->tags == NULL or
		meta_strings->creation_unixepoch == NULL or
		meta_strings->modificated_unixepoch == NULL
		) OUCH_ERROR(data_layer_error_metadata_corrupted, return false);


	meta_strings->display += strizeof(meta_display);
	meta_strings->unix_access += strizeof(meta_unix_access);
	meta_strings->user_id += strizeof(meta_user_id);
	meta_strings->group_id += strizeof(meta_group_id);
	meta_strings->title += strizeof(meta_title);
	meta_strings->data += strizeof(meta_data);
	meta_strings->datasource += strizeof(meta_datasource);
	meta_strings->tags += strizeof(meta_tags);
	meta_strings->creation_unixepoch += strizeof(meta_creation_unixepoch);
	meta_strings->modificated_unixepoch += strizeof(meta_modificated_unixepoch);

	if (meta_strings->display[0] == '\n' or
		meta_strings->unix_access[0] == '\n' or
		meta_strings->user_id[0] == '\n' or
		meta_strings->group_id[0] == '\n' or
		meta_strings->title[0] == '\n' or
		(meta_strings->data[0] == '\n' and meta_strings->datasource[0] == '\n') or
		emb_isdigit(meta_strings->creation_unixepoch[0]) == false or
		emb_isdigit(meta_strings->modificated_unixepoch[0]) == false
		) OUCH_ERROR(data_layer_error_metadata_corrupted, return false);

	return true;
}

bool retrieve_metadata(int fd, struct blog_record *r, const char **error) {
	struct metadata_strings m = {.meta = NULL};
	if (parse_metadata(fd, &m, error) == false) OUCH_ERROR(data_layer_error_metadata_corrupted, munmap(m.meta, m.metalen); return false);

	char *newline_fly = strchr(m.display, '\n');
	r->display = parse_meta_display(m.display, newline_fly - m.display);
	if (r->display == DISPLAY_INVALID) OUCH_ERROR(data_layer_error_metadata_corrupted, goto ohno);

	r->rights.mode = (acl_mode) oct_to_dec(strtoul(m.unix_access, NULL, 10));
	r->rights.user = (uint32_t) strtoul(m.user_id, NULL, 10);
	r->rights.group = (uint32_t) strtoul(m.group_id, NULL, 10);

	newline_fly = strchr(m.title, '\n');
	if (newline_fly == m.title) OUCH_ERROR(data_layer_error_metadata_corrupted, goto ohno);
	r->titlelen = newline_fly - m.title;

	if (r->stack_space < r->titlelen) OUCH_ERROR(data_layer_error_not_enough_stack_space, goto ohno);
	memcpy(r->stack, m.title, r->titlelen);
	r->title = r->stack;
	r->stack += r->titlelen;
	r->stack_space -= r->titlelen;

	if (m.data[0] != '\n') {
		newline_fly = strchr(m.data, '\n');
		r->datalen = newline_fly - m.data;
		if (r->stack_space < r->datalen) OUCH_ERROR(data_layer_error_not_enough_stack_space, goto ohno);
		memcpy(r->stack, m.data, r->datalen);
		r->data = r->stack;
		r->stack += r->datalen;
		r->stack_space -= r->datalen;
	}

	if (m.datasource[0] != '\n') {
		newline_fly = strchr(m.datasource, '\n');
		r->datasourcelen = newline_fly - m.datasource;
		if (r->stack_space < r->datasourcelen) OUCH_ERROR(data_layer_error_not_enough_stack_space, goto ohno);
		memcpy(r->stack, m.datasource, r->datasourcelen);
		r->datasource = r->stack;
		r->stack += r->datasourcelen;
		r->stack_space -= r->datalen;
	}

	newline_fly = strchr(m.tags, '\n');
	if (r->stack_space < (size_t) (newline_fly - m.tags)) OUCH_ERROR(data_layer_error_not_enough_stack_space, goto ohno);
	if (skip_spaces(m.tags) != newline_fly) tag_processing(m.tags, r);

	r->creation_date.t = (time_t) strtoll(m.creation_unixepoch, NULL, 10);
	r->modification_date.t = (time_t) strtoll(m.modificated_unixepoch, NULL, 10);

	munmap(m.meta, m.metalen);
	return true;

	ohno:
	munmap(m.meta, m.metalen);
	return false;
}

static bool get_record_fileno(struct blog_record *r, unsigned choosen_record, void *context, const char **error) {
	struct fileno_context *f = context;

	char name[NAME_MAX];
	sprintf(name, "%u", choosen_record);

	int meta = openat(f->dfd, name, O_RDONLY);
	if (meta < 0) OUCH_ERROR(strerror(errno), return false);

	bool parse_result = retrieve_metadata(meta, r, error);
	close(meta);
	if (parse_result == false) OUCH_ERROR(data_layer_error_metadata_corrupted, return false);

	int fd[2] = {-1, -1};

	if (r->display == DISPLAY_BOTH or r->display == DISPLAY_DATA) {
		memcpy(name, r->data, r->datalen);
		name[r->datalen] = '\0';
		fd[0] = openat(f->datafd, name, O_RDONLY);
		if (fd[0] >= 0) {
			ssize_t got = read(fd[0], r->stack, r->stack_space);
			close(fd[0]);
			if (got > 0) {
				r->stack_space -= got;
				r->data = r->stack;
				r->stack += (size_t) got;
				r->datalen = (unsigned) got;
			} else {
				r->datalen = 0;
			}
		}
	}

	if (r->display == DISPLAY_BOTH or r->display == DISPLAY_DATASOURCE) {
		memcpy(name, r->datasource, r->datasourcelen);
		name[r->datasourcelen] = '\0';
		fd[1] = openat(f->datasourcefd, name, O_RDONLY);
		if (fd[1] >= 0) {
			ssize_t got = read(fd[1], r->stack, r->stack_space);
			close(fd[1]);
			if (got > 0) {
				r->stack_space -= got;
				r->datasource = r->stack;
				r->stack += (size_t) got;
				r->datasourcelen = (unsigned) got;
			} else {
				r->datasourcelen = 0;
			}
		}
	}

	if (fd[0] < 0 and fd[1] < 0) return false;

	r->chosen_record = choosen_record;

	return true;
}

#define RANDBYTES_WIDTH 11

static void randfilename(struct fileno_context *f, char *ptr, size_t len) {
	const char pool[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz123456789";
	char randbytes[RANDBYTES_WIDTH];

	f->randfun(randbytes, sizeof(randbytes));

	for (unsigned i = 0; i < sizeof(randbytes); i++) {
		randbytes[i] = pool[randbytes[i] % (sizeof(pool) - 1)];
	}

	if (len + sizeof(randbytes) > NAME_MAX) len -= len + sizeof(randbytes) - NAME_MAX;
	memcpy(ptr + len, randbytes, sizeof(randbytes));
	ptr[len + sizeof(randbytes)] = '\0';
}

static void normalize_filename(char *name) {
	char *push = name;
#define PUSH_NEWN(bytes, width) {memcpy(push, bytes, (width)); push += (width);}

	while(1) {
		if (*name == '\0') {*push = '\0'; break;}
		unsigned char width = utf8_byte_width(name);
		if (width != sizeof(char) or should_i_skip(*name) != true) PUSH_NEWN(name, width);
		name += width;
	}
}

static void markdown_output_process(const char *data, unsigned size, void *context) {
	int *fd = context;
	write(*fd, data, size);
}

static void add_to_tag(const char *tag, struct fileno_context *f, struct blog_record *r) {
	if (r->stack_space < NAME_MAX) return;
	mkdirat(f->tagsfd, tag, 0700);
	int dir = openat(f->tagsfd, tag, O_DIRECTORY | O_RDONLY);
	// the main reason why I'm not checking return values is not because I don't care
	// but because any subsequent call will easily fail
	sprintf(r->stack, "%lu", r->chosen_record);
	linkat(f->dfd, r->stack, dir, r->stack, 0);
	close(dir);
}

void tag_writer(int fd, char **tags, struct fileno_context *f, struct blog_record *r) {
	write(fd, "tags: ", strizeof("tags: "));
	bool commaspace = false;
	while(*tags) {
		if (commaspace == true) {write(fd, ", ", 2);} else {commaspace = true;}
		write(fd, *tags, strlen(*tags));
		add_to_tag(*tags, f, r);
		tags++;
	}
	write(fd, "\n", 1);
}

static bool flush_files(int meta, struct fileno_context *f, struct blog_record *r, const char **error) {
	char name[NAME_MAX + 1];
	memcpy(name, r->title, r->titlelen);
	name[r->titlelen] = '\0';
	normalize_filename(name);
	size_t len = strlen(name);

	if (r->stack_space <  len + NAME_MAX) OUCH_ERROR(data_layer_error_not_enough_stack_space, return false);

	int fd, sfd;

	char name2[NAME_MAX + 1];
	strcpy(name2, name);

	while(1) {
		randfilename(f, name, len);
		fd = openat(f->datafd, name, O_RDWR | O_CREAT | O_EXCL, DEFAULT_FILE_MODE);
		if (fd >= 0) break;
		if (errno != EEXIST) OUCH_ERROR(strerror(errno), return false);
	}

	while(1) {
		randfilename(f, name2, len);
		sfd = openat(f->datasourcefd, name2, O_RDWR | O_CREAT | O_EXCL, DEFAULT_FILE_MODE);
		if (sfd >= 0) break;
		if (errno != EEXIST) {
			close(fd);
			unlinkat(f->datafd, name, 0);
			OUCH_ERROR(strerror(errno), return false);
		}
	}

	const char *display = display_enum_to_str(r->display);
	dprintf(meta, METADATA_FMT_WO_TAGS,
			display,
			dec_to_oct(r->rights.mode ? r->rights.mode : ACL_DEFAULT_NEW_OBJECT_MODE),
			r->rights.user,
			r->rights.group,
			r->titlelen, r->title,
			name,
			name2,
			r->creation_date.t ? r->creation_date.t : time(NULL),
			r->modification_date.t ? r->modification_date.t : time(NULL));
	tag_writer(meta, r->tags, f, r);
	close(meta);

	if ((r->display == DISPLAY_BOTH or r->display == DISPLAY_DATASOURCE) and r->datasourcelen == 0) {
		// cblog is transforming markdown to html when only markdown is provided, but displaying html is possible (html only or html+md)

		write(fd, r->data, r->datalen);
		md_html(r->data, r->datalen, markdown_output_process, &sfd, 0, 0);
		close(fd);
		close(sfd);

		return true;
	}

	if (r->datalen > 0) write(fd, r->data, r->datalen);
	close(fd);

	if (r->datasourcelen > 0) write(sfd, r->datasource, r->datasourcelen);
	close(sfd);

	return true;
}

static bool last_prepare(int fd, char str[CBL_UINT32_STR_MAX], unsigned long *val, const char **error) {
	ssize_t got;

	if (fd < 0 or (got = read(fd, str, CBL_UINT32_STR_MAX)) < 0) OUCH_ERROR(strerror(errno), return false);
	if (got > 0) {
		if (emb_isdigit(str[0]) == false) OUCH_ERROR(data_layer_error_metadata_corrupted, return false);
		str[got] = '\0';
		*val = strtoul(str, NULL, 10);
		return true;
	}

	if (write(fd, "1", 1) < 0) OUCH_ERROR(strerror(errno), return false);
	strcpy(str, "1");
	*val = 1;

	return true;
}

bool insert_record_fileno(struct blog_record *r, void *context, const char **error) {
	struct fileno_context *f = context;

	if (r->titlelen > NAME_MAX or r->titlelen == 0) OUCH_ERROR(data_layer_error_invalid_argument, return false);
	if (display_enum_to_str(r->display) == NULL) OUCH_ERROR(data_layer_error_invalid_argument, return false);
	if (utf8_check(r->title) != NULL) OUCH_ERROR(data_layer_error_invalid_argument_utf8, return false);
	if (r->datalen == 0 and r->datasourcelen == 0) OUCH_ERROR(data_layer_error_invalid_argument, return false);

	int last_record_storage_fd = openat(f->dfd, fileno_last_record_file, O_RDWR | O_CREAT, DEFAULT_FILE_MODE);
	char last_record_str[CBL_UINT32_STR_MAX];
	if (last_prepare(last_record_storage_fd, last_record_str, &(r->chosen_record), error) == false) {
		close(last_record_storage_fd);
		return false;
	}

	int metadata = openat(f->dfd, last_record_str, O_RDWR | O_CREAT | O_EXCL, DEFAULT_FILE_MODE);
	if (metadata < 0) {
		if (errno == EEXIST) OUCH_ERROR(data_layer_error_data_already_exist, close(last_record_storage_fd); return false);
		OUCH_ERROR(strerror(errno), close(last_record_storage_fd); return false);
	}

	if (flush_files(metadata, f, r, error) == false) {
		close(last_record_storage_fd);
		return false;
	}

	lseek(last_record_storage_fd, 0, SEEK_SET);
	ftruncate(last_record_storage_fd, 0);
	int got = sprintf(last_record_str, "%lu", r->chosen_record + 1);
	write(last_record_storage_fd, last_record_str, (size_t) got);
	close(last_record_storage_fd);

	return true;
}

void special_markdown_case(int fd, struct blog_record *old, struct blog_record *r, struct fileno_context *f) {

}

#define METADATA_FMT_WITH_TAGS_LIMITED METADATA_VER "\ndisplay: %s\nunix access: %03"PRIu32"\nuser id: %"PRIu32"\ngroup id: %"PRIu32"\ntitle: %.*s" \
                                          "\ndata: %.*s\ndatasource: %.*s\ncreation_unixepoch: %lu\nmodificated_unixepoch: %lu\ntags: %.*s\n"

bool alter_record_fileno(struct blog_record *r, void *context, const char **error) {
	// this is the most tricky function I ever had in this scope
	// We're expecting at least one change: from title, from datasource or from data.
	// In any case, we should open last-made metadata, map it to memory, parse, then we could write to new temporary
	// metadata file. After all writing is complete, temporary becomes the new metadata, the old one should be deleted

	struct fileno_context *f = context;

	if (r->chosen_record == 0) OUCH_ERROR(data_layer_error_invalid_argument, return false);
	if (r->datalen == 0 and r->datasourcelen == 0 and r->titlelen == 0) OUCH_ERROR(data_layer_error_invalid_argument, return false);

	// It's hard to write to a same file where we're reading from, don't want to have any collision during that
	char first_filename[NAME_MAX];
	char second_filename[NAME_MAX];
	sprintf(first_filename, "%lu", r->chosen_record);

	int meta = openat(f->dfd, first_filename, O_RDONLY);
	if (meta < 0) {
		if (errno == ENOENT) OUCH_ERROR(data_layer_error_item_not_found, return false);
		return false;
	}

	struct blog_record old;

	struct metadata_strings m = {.meta = NULL};
	if (parse_metadata(meta, &m, error) == false) {
		close(meta);
		OUCH_ERROR(data_layer_error_metadata_corrupted, munmap(m.meta, m.metalen); return false);
	}
	close(meta);

	if (r->titlelen > 0) {
		old.title = r->title;
		old.titlelen = r->titlelen;
	} else {
		old.title = m.title;
		old.titlelen = strchr(m.title, '\n') - m.title;
	}
	old.display = r->display ? r->display : parse_meta_display(m.display, strchr(m.display, '\n') - m.display);
	if (old.display == DISPLAY_INVALID) OUCH_ERROR(data_layer_error_metadata_corrupted, munmap(m.meta, m.metalen); return false);
	old.rights.mode = (acl_mode) strtoul(m.unix_access, NULL, 10); // we don't need octal/decimal conversion here
	old.rights.user = (uint32_t) strtoul(m.user_id, NULL, 10);
	old.rights.group = (uint32_t) strtoul(m.group_id, NULL, 10);
	old.creation_date.t = (time_t) strtoll(m.creation_unixepoch, NULL, 10);
	old.data = m.data;
	old.datalen = strchr(m.data, '\n') - m.data;
	old.datasource = m.datasource;
	old.datasourcelen = strchr(m.datasource, '\n') - m.datasource;
	char *tags = "";
	size_t tagslen = 0;
	if (skip_spaces(m.tags) != strchr(m.tags, '\n')) {
		tags = skip_spaces(m.tags);
		tagslen = strchr(tags, '\n') - tags;
	}

	sprintf(second_filename, "new_%lu\n", r->chosen_record);
	meta = openat(f->dfd, second_filename, O_RDWR | O_CREAT | O_EXCL, DEFAULT_FILE_MODE);
	if (meta < 0) OUCH_ERROR(data_layer_error_metadata_corrupted, munmap(m.meta, m.metalen); return false);

	unlinkat(f->dfd, first_filename, 0);
	renameat(f->dfd, second_filename, f->dfd, first_filename);

	const char *display = display_enum_to_str(old.display);
	dprintf(meta, METADATA_FMT_WITH_TAGS_LIMITED,
			display,
			old.rights.mode,
			old.rights.user,
			old.rights.group,
			(int) old.titlelen, old.title,
			(int) old.datalen, old.data,
			(int) old.datasourcelen, old.datasource,
			old.creation_date.t,
			time(NULL),
			(int) tagslen, tags);
	close(meta);

	if (r->datalen > 0) {
		size_t len = strchr(m.data, '\n') - m.data;
		memcpy(first_filename, m.data, len);
		first_filename[len] = '\0';
		int fd = openat(f->datafd, first_filename, O_RDWR);
		if (fd >= 0) {
			ftruncate(fd, 0);
			write(fd, r->data, r->datalen);
			close(fd);
		}

		if ((old.display == DISPLAY_BOTH or old.display == DISPLAY_DATASOURCE) and r->datasourcelen == 0) {
			// special case for markdown processing
			len = strchr(m.datasource, '\n') - m.datasource;
			memcpy(first_filename, m.datasource, len);
			first_filename[len] = '\0';
			fd = openat(f->datasourcefd, first_filename, O_RDWR);
			if (fd >= 0) {
				ftruncate(fd, 0);
				md_html(r->data, r->datalen, markdown_output_process, &fd, 0, 0);
				close(fd);
			}
		}
	}

	if (r->datasourcelen > 0) {
		size_t len = strchr(m.datasource, '\n') - m.datasource;
		memcpy(first_filename, m.datasource, len);
		first_filename[len] = '\0';
		int fd = openat(f->datasourcefd, first_filename, O_RDWR);
		if (fd >= 0) {
			ftruncate(fd, 0);
			write(fd, r->datasource, r->datasourcelen);
			close(fd);
		}
	}

	munmap(m.meta, m.metalen);

	return true;
}

static bool key_val_fileno_remove(char key[KEY_VAL_MAXKEYLEN], void *value, ssize_t *size, void *context, const char **error) {
	UNUSED(value);
	UNUSED(size);

	struct fileno_context *f = context;

	if (unlinkat(f->keyvalfd, key, 0) == 0) return true;
	*error = strerror(errno);
	return false;
}

static bool key_val_fileno_check(char key[KEY_VAL_MAXKEYLEN], void *value, ssize_t *size, void *context, const char **error) {
	UNUSED(value);
	UNUSED(size);

	struct fileno_context *f = context;

	if (key == NULL) return false;
	int olderrno = errno;
	errno = 0;
	if (faccessat(f->keyvalfd, key, R_OK | W_OK, 0) == 0) {
		return true;
	}
	if (errno != 0) *error = strerror(errno);
	errno = olderrno;
	return false;
}

static bool key_val_fileno_insert(char key[KEY_VAL_MAXKEYLEN], void *value, ssize_t *size, void *context, const char **error) {
	struct fileno_context *f = context;

	if (key[0] == '\0') {
		size_t prefixlen = strlen(key + 1);
		memmove(key, key + 1, prefixlen);
		key[prefixlen] = '\0';
		do {
			randfilename(f, key, prefixlen);
		} while (faccessat(f->keyvalfd, key, R_OK | W_OK, 0) == 0);
	}
	int fd = openat(f->keyvalfd, key, O_CREAT | O_RDWR | O_EXCL, DEFAULT_FILE_MODE);
	if (fd < 0) {
		if (errno == EEXIST) OUCH_ERROR(data_layer_error_data_already_exist, return false);
		OUCH_ERROR(strerror(errno), return false);
	}
	ssize_t got = write(fd, value, (size_t) *size);
	close(fd);
	if (got < *size) {
		unlinkat(f->keyvalfd, key, 0);
		OUCH_ERROR(data_layer_error_unable_to_process_kval, return false);
	}

	return true;
}

bool key_val_fileno_read(char key[KEY_VAL_MAXKEYLEN], void *value, ssize_t *size, void *context, const char **error) {
	struct fileno_context *f = context;

	if (key == NULL) return false;
	int fd = openat(f->keyvalfd, key,  O_RDWR , DEFAULT_FILE_MODE);
	if (fd < 0) {
		if (errno == ENOENT) OUCH_ERROR(data_layer_error_item_not_found, return false);
		OUCH_ERROR(strerror(errno), return false);
	}
	ssize_t got = read(fd, value, (size_t) -*size);
	close(fd);
	if (got < 0) OUCH_ERROR(strerror(errno), return false);
	*size = got;

	return true;
}

// TODO: This storage should be rewritten in order to use as much ram memory and possible
//       and write to disk as rare as possible. Workaround is using ramdisk, duh.
bool key_val_fileno(char key[KEY_VAL_MAXKEYLEN], void *value, ssize_t *size, void *context, const char **error) {
	if (size == NULL) return key_val_fileno_remove(key, value, size, context, error);

	if (*size == 0) return key_val_fileno_check(key, value, size, context, error);

	if (*size > 0) return key_val_fileno_insert(key, value, size, context, error);

	return key_val_fileno_read(key, value, size, context, error);
}

bool user_fileno(struct usr *usr, struct user_action action, void *context, const char **error) {
	struct fileno_context *f = context;

	char filename[NAME_MAX + sizeof(char)];
	char *target = NULL;
	if (action.operation == ADD) {
		if (usr->id == 0 or
			strchr(usr->display_name, '/') != NULL or
			usr->display_name[0] == '\0' or
			usr->email[0] == '\0') {
			OUCH_ERROR(data_layer_error_invalid_argument, return false);
		}
		int fd = openat(f->users, fileno_last_record_file, O_RDWR | O_CREAT, DEFAULT_FILE_MODE);
		unsigned long new_user_id;
		if (last_prepare(fd, filename, &new_user_id, error) == false) {
			OUCH_ERROR("Unable to obtain last id file from user storage", close(fd); return false);
		}

		if (faccessat(f->users, usr->email, R_OK | W_OK, 0) == 0 or faccessat(f->users, usr->display_name, R_OK | W_OK, 0) == 0) {
			OUCH_ERROR(data_layer_error_user_already_exist, close(fd); return false);
		}

		int userfd = openat(f->users, filename, O_RDWR | O_CREAT | O_EXCL, DEFAULT_FILE_MODE);
		if (userfd < 0) {
			if (errno == EEXIST) OUCH_ERROR(data_layer_error_user_already_exist, close(fd); return false);
			OUCH_ERROR(strerror(errno), close(fd); return false);
		}
		usr->id = (uint32_t) new_user_id;
		ssize_t got = write(userfd, usr, sizeof(struct usr));
		close(userfd);
		if (got < 0) OUCH_ERROR(strerror(errno), close(fd); unlinkat(f->users, filename, 0); return false);
		linkat(f->users, filename, f->users, usr->email, 0);
		linkat(f->users, filename, f->users, usr->display_name, 0);

		lseek(fd, 0, SEEK_SET);
		ftruncate(fd, 0);
		got = sprintf(filename, "%lu", new_user_id + 1);
		write(fd, filename, (size_t) got);
		close(fd);

		return true;
	} else {
		if (action.filter == BY_ID) {
			snprintf(filename, sizeof(filename), "%"PRIu32, usr->id);
			target = filename;
		}
		else if (action.filter == BY_NAME) target = usr->display_name;
		else if (action.filter == BY_EMAIL) target = usr->email;
		else return false;
	}

	switch (action.operation) {
	case CHECK:
		if (faccessat(f->users, target, R_OK | W_OK, 0) == 0) return true;
		return false;
	case SELECT:
	{
		int fd = openat(f->users, target, O_RDONLY);
		if (fd < 0) {
			if (errno == ENOENT) OUCH_ERROR(data_layer_error_item_not_found, return false);
			OUCH_ERROR(strerror(errno), return false);
		}
		ssize_t got = read(fd, usr, sizeof(struct usr));
		close(fd);
		if (got < 0) OUCH_ERROR(strerror(errno), return false);
		if (got < (ssize_t) sizeof(struct usr) or usr->id == 0) OUCH_ERROR(data_layer_error_metadata_corrupted, return false);
		return true;
	}
	case ALTER:
	{ // YOU MUST PERFORM SELECT BEFORE CALLING ALTER
		int fd = openat(f->users, target, O_RDWR);
		if (fd < 0) {
			if (errno == ENOENT) OUCH_ERROR(data_layer_error_item_not_found, return false);
			OUCH_ERROR(strerror(errno), return false);
		}
		ssize_t got = write(fd, usr, sizeof(struct usr));
		close(fd);
		if (got < 0) OUCH_ERROR(strerror(errno), return false);
		if (got < (ssize_t) sizeof(struct usr)) {
			OUCH_ERROR(data_layer_error_metadata_corrupted, return false);
		}

		return true;
	}
	case REMOVE:
	{
		if (unlinkat(f->users, target, 0) < 0) OUCH_ERROR(strerror(errno), return false); // TODO: WRONG!
		return true;
	}
	default:
		break;
	}

	return false;
}
