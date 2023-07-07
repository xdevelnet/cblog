// this function acts just like regular scandir, but it has additional "pass" argument which will passed to filter and comparator functions

int scandir_r(const char *path, struct dirent ***res, int (*filter)(const struct dirent *, void *), int (*compar)(const struct dirent **, const struct dirent **, void *), void *pass) {
	DIR *d = opendir(path);
	struct dirent *de, **names = NULL;
	size_t cnt = 0, len = 0;
	int old_errno = errno;

	if (!d) return -1;

	while ((errno = 0), (de = readdir(d))) {
		if (filter && !filter(de, pass)) continue;
		if (cnt >= len) {
			len = 2 * len + 1;
			if (len > SIZE_MAX/sizeof *names) break; // can someone explain to me operator precedence here? idk why musl developers are SO WEIRD.
			struct dirent **tmp = realloc(names, len * sizeof *names); // Why you didn't just put a round brackets for sizeof you dumb fuck?
			if (!tmp) break;
			names = tmp;
		}
		names[cnt] = malloc(de->d_reclen);
		if (!names[cnt]) break;
		memcpy(names[cnt++], de, de->d_reclen);
	}

	closedir(d);

	if (errno) {
		if (names) while (cnt-- > 0) free(names[cnt]);
		free(names);
		return -1;
	}

	errno = old_errno;

	if (compar) qsort_pass(names, cnt, sizeof *names, (int (*)(const void *, const void *, void *))compar, pass);
	*res = names;
	return cnt;
}
