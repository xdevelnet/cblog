#ifndef GUARD_UTIL_C
#define GUARD_UTIL_C

#include <string.h>
#include <stdbool.h>

#if !defined strizeof
#define strizeof(a) (sizeof(a)-1)
#endif

#define CBL_STRINGIZE(a) #a

#define CBL_MAX(a,b) (((a)>(b))?(a):(b))

#if !defined UNUSED
#define UNUSED(x) (void)(x)
#endif

#if !defined STREQ
#define STREQ (0)
#endif

#if !defined STRNEQ
#define STRNEQ (1)
#endif

// the maximum amount of bytes that's required to express integers as string
#define CBL_UINT8_STR_MAX   strizeof("255")
#define CBL_UINT16_STR_MAX  strizeof("65535")
#define CBL_UINT32_STR_MAX  strizeof("4294967295")
#define CBL_UINT64_STR_MAX  strizeof("18446744073709551615")
#define CBL_INT8_STR_MAX  strizeof("-128")
#define CBL_INT16_STR_MAX strizeof("-32767") // -1
#define CBL_INT32_STR_MAX strizeof("-2147483647")
#define CBL_INT64_STR_MAX strizeof("-9223372036854775807")

char strpartcmp(char *str, char *part) {
	while (1) {
		if (*part == 0) return STREQ;
		if (*(str++) != *(part++)) return STRNEQ;
	}
}

const unsigned char *utf8_check(const void *a){
	const unsigned char *s = a;
	while (*s) {
		if (*s < 0x80)
			/* 0xxxxxxx */
			s++;
		else if ((s[0] & 0xe0) == 0xc0) {
			/* 110XXXXx 10xxxxxx */
			if ((s[1] & 0xc0) != 0x80 ||
				(s[0] & 0xfe) == 0xc0)                        /* overlong? */
				return s;
			else
				s += 2;
		} else if ((s[0] & 0xf0) == 0xe0) {
			/* 1110XXXX 10Xxxxxx 10xxxxxx */
			if ((s[1] & 0xc0) != 0x80 ||
				(s[2] & 0xc0) != 0x80 ||
				(s[0] == 0xe0 && (s[1] & 0xe0) == 0x80) ||    /* overlong? */
				(s[0] == 0xed && (s[1] & 0xe0) == 0xa0) ||    /* surrogate? */
				(s[0] == 0xef && s[1] == 0xbf &&
				 (s[2] & 0xfe) == 0xbe))                      /* U+FFFE or U+FFFF? */
				return s;
			else
				s += 3;
		} else if ((s[0] & 0xf8) == 0xf0) {
			/* 11110XXX 10XXxxxx 10xxxxxx 10xxxxxx */
			if ((s[1] & 0xc0) != 0x80 ||
				(s[2] & 0xc0) != 0x80 ||
				(s[3] & 0xc0) != 0x80 ||
				(s[0] == 0xf0 && (s[1] & 0xf0) == 0x80) ||    /* overlong? */
				(s[0] == 0xf4 && s[1] > 0x8f) || s[0] > 0xf4) /* > U+10FFFF? */
				return s;
			else
				s += 4;
		} else
			return s;
	}

	return NULL;
}

unsigned char utf8_byte_width(const void *ptr) {

	// |----------|----------|----------|----------|
	// |     1    |     2    |     3    |     4    | |----------|-----|
	// |----------+----------+----------+----------| | 11000000 | 192 |
	// | 0xxxxxxx |          |          |          | |----------+-----|
	// |----------+----------+----------+----------| | 11100000 | 224 |
	// | 110xxxxx | 10xxxxxx |          |          | |----------+-----|
	// |----------+----------|----------+----------| | 11110000 | 240 |
	// | 1110xxxx | 10xxxxxx | 10xxxxxx |          | |----------+-----|
	// |----------+----------+----------+----------| | 11111100 | 252 |
	// | 11110xxx | 10xxxxxx | 10xxxxxx | 10xxxxxx | |----------|-----|
	// |----------|----------|----------|----------|

	const unsigned char *utf = ptr;

	if (*utf < 192) return 1;
	if (*utf < 224) return 2;
	if (*utf < 240) return 3;
	if (*utf < 252) return 4;
	return 1;
}

unsigned char_occurences(const char *str, char lf) {
	const char *temp = str;
	unsigned count = 0;
	while( (temp = strchr(temp, lf)) != NULL) {
		count++;
		temp++;
	}

	return count;
}

static char *skip_spaces(char *ptr) {
	while(*ptr == ' ') ptr++;
	return ptr;
}

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

static bool abiggerb_timespec(struct timespec a, struct timespec b) {
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

bool should_i_skip(char character) {
	if (character < ' ') return true;
	switch (character)
	{
	case '/':
	case '%':
	case '|':
	case '<':
	case '>':
	case '\\':
	case '?':
	case '*':
	case ':':
	case '"':
		return true;
	default:
		return false;
	}
}

static bool is_str_unsignedint(const char *p) {
	while(*p != '\0') {
		if (emb_isdigit(*p) == false) return false;
		p++;
	}

	return true;
}

#endif // GUARD_UTIL_C
