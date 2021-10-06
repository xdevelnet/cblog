#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <threads.h>
#include <sys/stat.h>
#include <dirent.h>
#include <iso646.h>
#include <errno.h>
#include <stdio.h>
#include <sys/mman.h>

struct fileno_context {
	const void *addr;
};

mtx_t scanmutex;
bool mutex_initialized = false;

static bool initialize_fileno_context(const void *addr, void *context, const char **error) {
	if (mutex_initialized == false) {
		mutex_initialized = true;
		mtx_init(&scanmutex, 0);
	}

	struct fileno_context *ret = context;
	memset(ret, 0, sizeof(struct fileno_context));
	ret->addr = addr;
	return true;
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
} // todo: asc desc

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

	if (d->d_type != DT_LNK) return away;
	if (is_str_unsignedint(d->d_name) == false) return away;
	struct stat s;
	fstatat(glob_dirfd, d->d_name, &s, 0);
	if (s.st_mtim.tv_sec < glob_from.t) return away;
	if (s.st_mtim.tv_sec > glob_to.t) return away;
	return keep;
}

// SELECT * from records WHERE modified_time > from and modified_time < to LIMIT amount OFFSET offset
static bool list_records_fileno(unsigned *amount,       // Pointer that could be used for limiting amount of results in list. After executing places amount of results.
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

	mtx_lock(&scanmutex);
	glob_from = from;
	glob_to = to;
	glob_dirfd = dfd;
	int ret = scandir(f->addr, &e, filter, comparator);
	mtx_unlock(&scanmutex);

	close(dfd);
	unsigned limit = *amount; // 8 lines below could be refactored... Or now. IDK.
	*amount = 0;
	if (ret < 0) return false;
	if (ret == 0 or (unsigned) ret <= offset) return true;

	for (unsigned i = offset; i < (unsigned) ret and *amount < limit; i++) {
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
	}

	char *map = mmap(NULL, s.st_size, PROT_READ, MAP_SHARED, fd, 0);
	close(fd);

	if (map == MAP_FAILED) {
		if (error) *error = strerror(errno);
		return false;
	}

	char *fly = r->stack;
	r->recordname = memcpy(fly, name, strlen(name) + sizeof(char));
	fly += strlen(name) + sizeof(char);
	r->recordcontent = memcpy(fly, map, s.st_size);
	fly += s.st_size;
	*fly = '\0';
	munmap(map, s.st_size);

	return true;
}
