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
#define METADATA_FMT_WO_TAGS METADATA_VER "\ndisplay: %s\ntitle: %.*s\ndata: %s\ndatasource: %s\ncreation_unixepoch: %lu\n"

bool parse_metadata(int fd, struct blog_record *r, const char **error) {
	/* metadata format:
	 *
	 * METADATA1
	 * display: source | data | both | none
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
	if (s.st_size < (ssize_t) strizeof(METADATA_FMT_WO_TAGS))  OUCH_ERROR(data_layer_error_metadata_corrupted, return false;);

	const char meta_header[] = METADATA_VER;
	const char meta_display[] = "\ndisplay: ";
	const char meta_title[] = "\ntitle: ";
	const char meta_data[] = "\ndata: ";
	const char meta_datasource[] = "\ndatasource: ";
	const char meta_tags[] = "\ntags: ";
	const char meta_creation_unixepoch[] = "\ncreation_unixepoch: ";

	size_t metalen;
	char *meta = map_whole_file_shared(fd, &(metalen), error);
	if (meta == NULL) return false;

	if (meta[metalen - 1] != '\n' or memcmp(meta, meta_header, strizeof(meta_header)) != 0) {
		OUCH_ERROR(data_layer_error_metadata_corrupted, goto ohno;);
	}

	char *display = util_memmem(meta, metalen, meta_display, strizeof(meta_display));
	char *title = util_memmem(meta, metalen, meta_title, strizeof(meta_title));
	char *data = util_memmem(meta, metalen, meta_data, strizeof(meta_data));
	char *datasource = util_memmem(meta, metalen,  meta_datasource, strizeof(meta_datasource));
	char *tags = util_memmem(meta, metalen, meta_tags, strizeof(meta_tags));
	char *creation_unixepoch = util_memmem(meta, metalen, meta_creation_unixepoch, strizeof(meta_creation_unixepoch));

	if (display == NULL or
		title == NULL or
		data == NULL or
		datasource == NULL or
		tags == NULL or
		creation_unixepoch == NULL
		) OUCH_ERROR(data_layer_error_metadata_corrupted, goto ohno;);


	display += strizeof(meta_display);
	title += strizeof(meta_title);
	data += strizeof(meta_data);
	datasource += strizeof(meta_datasource);
	tags += strizeof(meta_tags);
	creation_unixepoch += strizeof(meta_creation_unixepoch);

	if (*display == '\n' or
	*title == '\n' or
	(*data == '\n' and *datasource == '\n') or
	emb_isdigit(creation_unixepoch[0]) == false) OUCH_ERROR(data_layer_error_metadata_corrupted, goto ohno;);

	char *newline_fly = strchr(display, '\n');
	r->display = parse_meta_display(display, newline_fly - display);
	if (r->display == DISPLAY_INVALID) OUCH_ERROR(data_layer_error_metadata_corrupted, goto ohno;);

	newline_fly = strchr(title, '\n');
	if (newline_fly == title) OUCH_ERROR(data_layer_error_metadata_corrupted, goto ohno;);
	r->titlelen = newline_fly - title;

	if (r->stack_space < r->titlelen) OUCH_ERROR(data_layer_error_not_enough_stack_space, goto ohno;);
	memcpy(r->stack, title, r->titlelen);
	r->title = r->stack;
	r->stack += r->titlelen;
	r->stack_space -= r->titlelen;

	if (*data != '\n') {
		newline_fly = strchr(data, '\n');
		r->datalen = newline_fly - data;
		if (r->stack_space < r->datalen) OUCH_ERROR(data_layer_error_not_enough_stack_space, goto ohno;);
		memcpy(r->stack, data, r->datalen);
		r->data = r->stack;
		r->stack += r->datalen;
		r->stack_space -= r->datalen;
	}

	if (*datasource != '\n') {
		newline_fly = strchr(datasource, '\n');
		r->datasourcelen = newline_fly - datasource;
		if (r->stack_space < r->datasourcelen) OUCH_ERROR(data_layer_error_not_enough_stack_space, goto ohno;);
		memcpy(r->stack, datasource, r->datasourcelen);
		r->datasource = r->stack;
		r->stack += r->datasourcelen;
		r->stack_space -= r->datalen;
	}

	newline_fly = strchr(tags, '\n');
	if (r->stack_space < (size_t) (newline_fly - tags)) OUCH_ERROR(data_layer_error_not_enough_stack_space, goto ohno;);
	if (skip_spaces(tags) != newline_fly) tag_processing(tags, r);

	r->creation_date.t = (time_t) strtoll(creation_unixepoch, NULL, 10);

	r->modification_date.t = s.st_mtim.tv_sec;

	munmap(meta, metalen);
	return true;

	ohno:
	munmap(meta, metalen);
	return false;
}

static bool get_record_fileno(struct blog_record *r, unsigned choosen_record, void *context, const char **error) {
	struct fileno_context *f = context;

	char name[NAME_MAX];
	sprintf(name, "%u", choosen_record);

	int meta = openat(f->dfd, name, O_RDONLY);
	if (meta < 0) OUCH_ERROR(strerror(errno), return false);

	bool parse_result = parse_metadata(meta, r, error);
	close(meta);
	if (parse_result == false) return false;

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

	if (r->display == DISPLAY_BOTH or r->display == DISPLAY_SOURCE) {
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

	r->choosen_record = choosen_record;

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
	sprintf(r->stack, "%lu", r->choosen_record);
	linkat(f->dfd, r->stack, dir, r->stack, 0);
	close(dir);
}

static bool flush_files(int meta, struct fileno_context *f, struct blog_record *r, const char **error) {
	char name[NAME_MAX + 1];
	memcpy(name, r->title, r->titlelen);
	name[r->titlelen] = '\0';
	normalize_filename(name);
	size_t len = strlen(name);

	if (r->stack_space <  len + NAME_MAX) OUCH_ERROR(data_layer_error_not_enough_stack_space, return false);

	int fd = -1;
	int sfd = -1;

	char name2[NAME_MAX + 1];
	strcpy(name2, name);

	if (r->datalen > 0) while(1) {
		randfilename(f, name, len);
		fd = openat(f->datafd, name, O_RDWR | O_CREAT | O_EXCL, DEFAULT_FILE_MODE);
		if (fd >= 0) break;
		if (errno != EEXIST) OUCH_ERROR(strerror(errno), return false);
	}

	if (r->datasourcelen > 0 or r->display == DISPLAY_BOTH or r->display == DISPLAY_SOURCE) while(1) {
		randfilename(f, name2, len);
		sfd = openat(f->datasourcefd, name2, O_RDWR | O_CREAT | O_EXCL, DEFAULT_FILE_MODE);
		if (fd >= 0) break;
		if (errno != EEXIST) {
			close(fd);
			unlinkat(f->datafd, name, 0);
			OUCH_ERROR(strerror(errno), return false);
		}
	}

	// TODO: I want to rewrite somehow this ugly tag generation below
	const char *display = display_enum_to_str(r->display);
	dprintf(meta, METADATA_FMT_WO_TAGS,
			display,
			r->titlelen, r->title,
			name,
			name2,
			r->creation_date.t ? r->creation_date.t : time(NULL));
	write(meta, "tags: ", strizeof("tags: "));
	char **tags = r->tags;
	bool commaspace = false;
	while(*tags) {
		if (commaspace == true) {write(meta, ", ", 2);} else {commaspace = true;}
		write(meta, *tags, strlen(*tags));
		add_to_tag(*tags, f, r);
		tags++;
	}
	write(meta, "\n", 1);
	close(meta);

	if ((r->display == DISPLAY_BOTH or r->display == DISPLAY_SOURCE) and r->datasourcelen == 0) {

		// cblog is transforming markdown to html in the following case:
		// Only markdown is provided, but displaying html is possible (html only or html+md)

		write(fd, r->data, r->datalen);
		md_html(r->data, r->datalen, markdown_output_process, &sfd, 0, 0);
		close(fd);
		close(sfd);

		return true;
	}

	if (r->datalen > 0) {
		write(fd, r->data, r->datalen);
		close(fd);
	}

	if (r->datasourcelen > 0) {
		write(sfd, r->datasource, r->datasourcelen);
		close(sfd);
	}

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
	if (last_prepare(last_record_storage_fd, last_record_str, &(r->choosen_record), error) == false) {
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
	int got = sprintf(last_record_str, "%lu", r->choosen_record + 1);
	write(last_record_storage_fd, last_record_str, (size_t) got);
	close(last_record_storage_fd);

	return true;
}

static bool key_val_fileno_remove(char key[KEY_VAL_MAXKEYLEN], void *value, ssize_t *size, void *context, const char **error) {
	UNUSED(value);
	UNUSED(size);

	struct fileno_context *f = context;

	if (unlinkat(f->keyvalfd, key, 0) == 0) return true;
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
		int fd = openat(f->users, target, O_RDONLY, DEFAULT_FILE_MODE);
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
		int fd = openat(f->users, target, O_RDWR, DEFAULT_FILE_MODE);
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
