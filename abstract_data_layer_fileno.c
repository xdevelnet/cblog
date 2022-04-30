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

#if !defined strizeof
#define strizeof(a) (sizeof(a)-1)
#endif

struct fileno_context {
	const void *addr;
};

pthread_mutex_t scanmutex;
bool mutex_initialized = false;

static bool initialize_fileno_context(const void *addr, void *context, const char **error) {
	if (mutex_initialized == false) {
		mutex_initialized = true;
		pthread_mutex_init(&scanmutex, 0);
	}

	struct fileno_context *ret = context;
	memset(ret, 0, sizeof(struct fileno_context));
	ret->addr = addr;
	return true;
}

void deinitialize_engine_fileno(void *context) {
	pthread_mutex_destroy(&scanmutex);
}

static bool abiggerb(struct timespec a, struct timespec b) {
	if (a.tv_sec == b.tv_sec) {
		if (a.tv_nsec > b.tv_nsec) return true;
		return false;
	}

	if (a.tv_sec > b.tv_sec) return true;
	return false;
}

static bool emb_isdigit(char c) {
	if (c >= '0' && c <= '9') return true;
	return false;
}

static bool is_str_unsignedint(const char *p) {
	while(*p != '\0') {
		if (emb_isdigit(*p) == false) return false;
		p++;
	}

	return true;
}

ttime_t glob_from; // these variables should be protected with scanmutex
ttime_t glob_to;
int glob_dirfd;

static int comparator(const struct dirent **d1, const struct dirent **d2) {
	struct stat s[2];
	fstatat(glob_dirfd, (*d1)->d_name, s, 0);
	fstatat(glob_dirfd, (*d2)->d_name, s + 1, 0);

	if (s[0].st_mtim.tv_sec == s[1].st_mtim.tv_sec and s[0].st_mtim.tv_nsec == s[1].st_mtim.tv_nsec) return 0;
	if (abiggerb(s[0].st_mtim, s[1].st_mtim)) return 1;
	return -1;
}

static void scanfree(struct dirent **e, int amount) {
	for (int i = 0; i < amount; i++) {
		free(e[i]);
	}
	free(e);
}

// This filter is NOT REENTRANT! Use scandir() with mutextes to make it thread-safe
static int filter(const struct dirent *d) {
	static const int keep = 1;
	static const int away = 0;

	if (d->d_type != DT_LNK) return away; // I wanted to detect hardlinks because they are much more flexible for end user. Sigh.
	if (is_str_unsignedint(d->d_name) == false) return away;
	struct stat s;
	fstatat(glob_dirfd, d->d_name, &s, 0);
	if (s.st_mtim.tv_sec < glob_from.t) return away;
	if (s.st_mtim.tv_sec > glob_to.t) return away;
	return keep;
}

// SELECT record_id from records WHERE modified_time > from and modified_time < to LIMIT amount OFFSET offset sort by modified_time;
bool list_records_fileno(unsigned *amount,// Pointer that could be used for limiting amount of results in list. After executing places amount of results.
						unsigned long *result_list, // Array that will be filled with results
						unsigned offset,            // Skip some amount rows/records/results
						ttime_t from,               // Select only records that were created from specified unixtime
						ttime_t to,                 // Same as above, but until specified unixtime
						void *context,
						const char **error) {
	struct fileno_context *f = context;
	struct dirent **e;
	int dfd = open(f->addr, O_DIRECTORY | O_RDONLY);
	if (dfd < 0) {
		if (error) *error = strerror(errno);
		return false;
	}

	pthread_mutex_lock(&scanmutex);
	glob_from = from;
	glob_to = to;
	glob_dirfd = dfd;
	int ret = scandir(f->addr, &e, filter, comparator);
	pthread_mutex_unlock(&scanmutex);

	close(dfd);
	unsigned limit = *amount; // 8 lines below could be refactored... Or now. IDK.
	*amount = 0;
	if (ret < 0) return false;
	if (ret == 0 or (unsigned) ret <= offset) return true;

	for (unsigned i = offset; i < (unsigned) ret and *amount < limit; i++) {
//		if (alike_utf8(e[i]->d_name, looking_for) == false) continue;
		result_list[*amount] = strtoul(e[i]->d_name, NULL, 10);
		++*amount;
	}

	scanfree(e, ret);
	return true;
}

static bool get_record_fileno(struct blog_record *r, unsigned choosen_record, void *context, const char **error) {
	struct fileno_context *f = context;
	int dfd = open(f->addr, O_DIRECTORY | O_RDONLY);
	if (dfd < 0) {
		if (error) *error = strerror(errno);
		return false;
	}

	char name[NAME_MAX];
	sprintf(name, "%u", choosen_record);
	char path[PATH_MAX];
	ssize_t rl = readlinkat(dfd, name, path, sizeof(path));
	if (rl < 0) {
		close(dfd);
		if (error) *error = strerror(errno);
		return false;
	}

	path[rl] = '\0';
	strcpy(name, strrchr(path, '/') + sizeof(char));

	int fd = openat(dfd, path, O_RDONLY);
	close(dfd);
	if (fd < 0) {
		if (error) *error = strerror(errno);
		return false;
	}

	struct stat s;
	if (fstat(fd, &s) < 0) {
		close(fd);
		if (error) *error = strerror(errno);
		return false;
	}

	size_t calc = strlen(name) + sizeof(char) + s.st_size + sizeof(char);

	if (calc > r->stack_space) {
		close(fd);
		if (error) *error = data_layer_error_not_enough_stack_space;
		r->stack_space = calc;
		return false;
	}

	char *map = mmap(NULL, s.st_size, PROT_READ, MAP_SHARED, fd, 0);
	close(fd);

	if (map == MAP_FAILED) {
		if (error) *error = strerror(errno);
		return false;
	}

	char *fly = r->stack;
	r->recordname = memcpy(fly, name, strlen(name) + sizeof(char));
	r->recordnamelen = strlen(name);
	fly += strlen(name) + sizeof(char);
	r->recordcontent = memcpy(fly, map, s.st_size);
	r->recordcontentlen = s.st_size;
	fly += s.st_size;
	*fly = '\0';
	munmap(map, s.st_size);
	r->choosen_record = choosen_record;
	r->unixepoch.t = s.st_mtim.tv_sec;

	return true;
}

bool insert_record_fileno(struct blog_record *r, void *context, const char **error) {
	if (r->recordnamelen > NAME_MAX) {
		if (error) *error = data_layer_error_invalid_argument;
		return false;
	}

	struct fileno_context *f = context;
	int dfd = open(f->addr, O_DIRECTORY | O_RDONLY);
	if (dfd < 0) {
		if (error) *error = strerror(errno);
		return false;
	}

	int last = openat(dfd, "last_record", O_RDONLY);
	if (last < 0) {
		if (errno != ENOENT) {
			close(dfd);
			if (error) *error = strerror(errno);
			return false;
		}

		last = openat(dfd, "last_record", O_RDONLY);
		if (last < 0 or write(last, "1", strizeof("1")) < 0) {
			close(last);
			close(dfd);
			if (error) *error = strerror(errno);
			return false;
		}
	}

	char last_number[strizeof("-2147483648")];
	ssize_t ret = read(last, last_number, strizeof(last_number));
	if (ret < 0) {
		close(last);
		close(dfd);
		if (error) *error = strerror(errno);
		return false;
	} else if (ret == 0) {
		ret = write(last, "1", strizeof("1"));
		close(last);
		strcpy(last_number, "1");
		if (ret < 0) {
			close(dfd);
			if (error) *error = strerror(errno);
			return false;
		}
	}

	if (emb_isdigit(last_number[0]) == false) {
		close(last);
		close(dfd);
		if (error) *error = data_layer_error_metadata_corrupted;
		return false;
	}

	r->choosen_record = strtoul(last_number, NULL, 10);

	mkdirat(dfd, "data", 0700); // make dir if it's not exist. Fail if exist and that's fine.
	int datadfd = openat(dfd, f->addr, O_DIRECTORY | O_RDONLY);
	if (datadfd < 0) {
		close(last);
		close(dfd);
		if (error) *error = strerror(errno);
		return false;
	}

	char recordname[r->recordnamelen + sizeof('\0')];
	memcpy(recordname, r->recordname, r->recordnamelen);
	recordname[r->recordnamelen] = '\0';

	int fd = open(recordname, O_RDWR | O_CREAT | O_EXCL, 0755);
	if (fd < 0) {
		close(last);
		close(dfd);
		close(datadfd);
		if (error) *error = strerror(errno);
		return false;
	}

	linkat(dfd, last_number, datadfd, recordname, 0);
	close(dfd);
	close(datadfd);
	write(fd, r->recordcontent, r->recordcontentlen);
	lseek(last, 0, SEEK_SET);
	ret = sprintf(last_number, "%u", r->choosen_record + 1);
	write(last, last_number, ret);
	close(last);
	close(fd);
	return true;
}
