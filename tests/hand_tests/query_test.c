#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#define strizeof(a) (sizeof(a)-1)

void *util_memmem(const void *l, size_t l_len, const void *s, size_t s_len) {
	// TODO: replace with freebsd memmem https://github.com/freebsd/freebsd-src/blob/main/lib/libc/string/memmem.c
	register char *cur, *last;
	const char *cl = (const char *)l;
	const char *cs = (const char *)s;

	if (l_len == 0 || s_len == 0) return NULL;
	if (l_len < s_len) return NULL;
	if (s_len == 1) return memchr(l, (int) *cs, l_len);

	last = (char *)cl + l_len - s_len;

	for (cur = (char *)cl; cur <= last; cur++) {
		if (cur[0] == cs[0] && memcmp(cur, cs, s_len) == 0) return cur;
	}

	return NULL;
}

char *post_query_finder(const char *looking_for, const char *source, size_t source_len, size_t *result_len) {
	// Ok, let's be honest here.
	// this is not even fine implementation of such function
	// it's just suits enough for this project and I don't want to
	// spend a lot of time on it now. If someone wants to improve it
	// then I'll accept their pull request.
	// Current goal is to make it maintainable, or we could say "simple"

	size_t looking_len = strlen(looking_for);
	if (looking_len > source_len + strizeof("=a")) return NULL;
	char lf[looking_len + 1];
	memcpy(lf, looking_for, looking_len);
	lf[looking_len] = '=';

	char *find = util_memmem(source, source_len, lf, sizeof(lf));
	if (find == NULL) {
		return NULL;
	}

	char *seek = find + sizeof(lf);
	char *amp = strchr(seek, '&');
	if (amp == NULL) {
		*result_len = source + source_len - seek;
	} else {
		*result_len = amp - seek;
	}

	return find + sizeof(lf);
}

int main() {
	char *result;
	size_t size = 0;

	const char a[] = "abc=12&def=qweqwe";
	result = post_query_finder("abc", a, strizeof(a), &size);
	printf("len: %zu str: %.*s\n", size, (int) size, result);

	const char b[] = "abc=12&def=qweqwe";
	result = post_query_finder("def", b, strizeof(a), &size);
	printf("len: %zu str: %.*s\n", size, (int) size, result);

	const char c[] = "abc=12&def=";
	result = post_query_finder("def", c, strizeof(c), &size);
	printf("len: %zu str: %.*s\n", size, (int) size, result);
	return 0;
}
