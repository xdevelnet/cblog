#ifndef GUARD_UTIL_C
#define GUARD_UTIL_C

#include <string.h>
#include <stdbool.h>
#include <iso646.h>
#include <stdint.h>
#include <stddef.h>

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

// https://www.rfc-editor.org/errata_search.php?rfc=3696&eid=1690
#define EMAIL_MAXLEN 254

bool emb_isalpha(char c) {
	if ((c >= 'a' and c <= 'z') or (c >= 'A' and c <= 'Z')) return true;
	return false;
}

bool emb_isnumeric(char c) {
	if (c >= '0' and c <= '9') return true;
	return false;
}

bool emb_is_hexadecimal(char c) {
	if (emb_isnumeric(c)) return true;
	if ((c >= 'a' and c <= 'f') or (c >= 'A' and c <= 'F')) return true;
	return false;
}

bool is_special_character(char c) {
	if (c >= ' ' and c <= '/') return true; // spacebar!"#$%&'()*+,-./
	if (c >= '.' and c <= '@') return true; // :;<=>?@
	if (c >= '[' and c <= '`') return true; // [\]^_`
	if (c >= '{' and c <= '~') return true; // {|}~
	return false;
}

bool is_special_in_prefix(char c) {
	switch (c) {
	case '.':
	case '-':
	case '_':
	case '+':
	case '%':
		return true;
	default :
		break;
	}

	return false;
}

bool validate_prefix(const char *email) {
	bool special_char = false;
	if (is_special_in_prefix(*email)) return false;

	while(*email != '@') {
		if (emb_isalpha(*email) == true or emb_isnumeric(*email)) {
			special_char = false;
			email++;
			continue;
		}

		if (is_special_in_prefix(*email) == true) {
			if (special_char == true) return false;
			special_char = true;
			email++;
			continue;
		}

		return false;
	}

	if (special_char == true) return false;
	return true;
}

bool validate_domain(const char *domain) {
	domain++;
	size_t dots = 0;

	bool dot = false;
	bool dash = false;
	bool regular = false;

	while(*domain != '\0') {
		if (emb_isalpha(*domain) == true or emb_isnumeric(*domain)) {
			dot = false;
			dash = false;
			regular = true;
			domain++;
			continue;
		}

		if (*domain == '.') {
			if (regular == false or dot == true) return false;

			dot = true;
			dash = false;
			regular = false;
			domain++;
			dots++;
			continue;
		}

		if (*domain == '-') {
			if (regular == false and dash == false) return false;

			dot = false;
			dash = true;
			regular = false;
			domain++;
			continue;
		}

		return false;
	}

	if (dash == true or dot == true or dots == 0) return false;
	return true;
}

bool validate_email_wo_regex(char *email) { // while this validator is not perfect, it usually covers 99% of weird cases
	if (email == NULL or email[0] == '\0') return false;
	size_t len = strlen(email);
	if (len > EMAIL_MAXLEN) return false;
	if (len < strizeof("a@b.c")) return false;
	char *at_sign = strchr(email, '@');
	if (at_sign == NULL or at_sign == email) return false;

	if (validate_prefix(email) == false or validate_domain(at_sign) == false) return false;
	return true;
}

//bool validate_email(char *email) {
//	regex_t regex;
//	const char *r4 = "[A-Za-z0-9._%+-]+@[a-zA-Z0-9.-]+\\.[a-zA-Z]{2,3}(/[^[:space:]]*)?$"; // please suggest better regexp, i'm not pro with regexps.
//	int ret = regcomp(&regex, r4, REG_EXTENDED); // REG_EXTENDED
//	if (ret) {return false;}
//	if (regexec(&regex, email, 0, NULL, 0) != 0) {return false;}
//	return true;
//}

char strpartcmp(char *str, char *part) {
	while(1) {
		if (*part == 0) return STREQ;
		if (*(str++) != *(part++)) return STRNEQ;
	}
}

const unsigned char *utf8_check(const void *a){
	const unsigned char *s = a;
	while(*s) {
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

char *http_query_finder(const char *looking_for, const char *source, size_t source_len, size_t *result_len) {
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

union rands {
	int in;
	char out;
};

void unsafe_rand(void *ptr, size_t width) {
	char *fill = ptr;
	union rands r;
	for (size_t i = 0; i < width; i++) {
		r.in = rand();
		fill[i] = r.out;
	}
}

void urldecode2(char *dst, const char *src)
{
	char a, b;
	while(*src) {
		if ((*src == '%') &&
			((a = src[1]) && (b = src[2])) &&
			(emb_is_hexadecimal(a) && emb_is_hexadecimal(b))) {
			if (a >= 'a')
				a -= 'a'-'A';
			if (a >= 'A')
				a -= ('A' - 10);
			else
				a -= '0';
			if (b >= 'a')
				b -= 'a'-'A';
			if (b >= 'A')
				b -= ('A' - 10);
			else
				b -= '0';
			*dst++ = 16*a+b;
			src+=3;
		} else if (*src == '+') {
			*dst++ = ' ';
			src++;
		} else {
			*dst++ = *src++;
		}
	}
	*dst++ = '\0';
}

#ifdef __USE_GNU
#define qsort_pass qsort_r
#else
#define qsort_pass emb_qsort_r

#ifndef a_ctz_32
#define a_ctz_32 a_ctz_32
static inline int a_ctz_32(uint32_t x)
{
#ifdef a_clz_32
	return 31-a_clz_32(x&-x);
#else
	static const char debruijn32[32] = {
		0, 1, 23, 2, 29, 24, 19, 3, 30, 27, 25, 11, 20, 8, 4, 13,
		31, 22, 28, 18, 26, 10, 7, 12, 21, 17, 9, 6, 16, 5, 15, 14
	};
	return debruijn32[(x&-x)*0x076be629 >> 27];
#endif
}
#endif

#ifndef a_ctz_64
#define a_ctz_64 a_ctz_64
static inline int a_ctz_64(uint64_t x)
{
	static const char debruijn64[64] = {
		0, 1, 2, 53, 3, 7, 54, 27, 4, 38, 41, 8, 34, 55, 48, 28,
		62, 5, 39, 46, 44, 42, 22, 9, 24, 35, 59, 56, 49, 18, 29, 11,
		63, 52, 6, 26, 37, 40, 33, 47, 61, 45, 43, 21, 23, 58, 17, 10,
		51, 25, 36, 32, 60, 20, 57, 16, 50, 31, 19, 15, 30, 14, 13, 12
	};
	if (sizeof(long) < 8) {
		uint32_t y = x;
		if (!y) {
			y = x>>32;
			return 32 + a_ctz_32(y);
		}
		return a_ctz_32(y);
	}
	return debruijn64[(x&-x)*0x022fdd63cc95386dull >> 58];
}
#endif

static inline int a_ctz_l(unsigned long x)
{
	return (sizeof(long) < 8) ? a_ctz_32(x) : a_ctz_64(x);
}

#define ntz(x) a_ctz_l((x))

typedef int (*cmpfun)(const void *, const void *, void *);

static inline int pntz(size_t p[2]) {
	int r = ntz(p[0] - 1);
	if (r != 0 || (r = 8 * sizeof(size_t) + ntz(p[1])) != 8 * sizeof(size_t)) return r;
	return 0;
}

static void cycle(size_t width, unsigned char* ar[], int n) {
	unsigned char tmp[256];
	size_t l;
	int i;

	if (n < 2) {
		return;
	}

	ar[n] = tmp;
	while(width) {
		l = sizeof(tmp) < width ? sizeof(tmp) : width;
		memcpy(ar[n], ar[0], l);
		for (i = 0; i < n; i++) {
			memcpy(ar[i], ar[i + 1], l);
			ar[i] += l;
		}
		width -= l;
	}
}

/* shl() and shr() need n > 0 */
static inline void shl(size_t p[2], int n) {
	if (n >= (int) (8 * sizeof(size_t))) {
		n -= (int) (8 * sizeof(size_t));
		p[1] = p[0];
		p[0] = 0;
	}
	p[1] <<= n;
	p[1] |= p[0] >> (sizeof(size_t) * 8 - n);
	p[0] <<= n;
}

static inline void shr(size_t p[2], int n) {
	if (n >= (int) ( 8 * sizeof(size_t))) {
		n -= (int) (8 * sizeof(size_t));
		p[0] = p[1];
		p[1] = 0;
	}
	p[0] >>= n;
	p[0] |= p[1] << (sizeof(size_t) * 8 - n);
	p[1] >>= n;
}

static void sift(unsigned char *head, size_t width, cmpfun cmp, void *arg, int pshift, size_t lp[]) {
	unsigned char *rt, *lf;
	unsigned char *ar[14 * sizeof(size_t) + 1];
	int i = 1;

	ar[0] = head;
	while(pshift > 1) {
		rt = head - width;
		lf = head - width - lp[pshift - 2];

		if (cmp(ar[0], lf, arg) >= 0 && cmp(ar[0], rt, arg) >= 0) {
			break;
		}
		if (cmp(lf, rt, arg) >= 0) {
			ar[i++] = lf;
			head = lf;
			pshift -= 1;
		} else {
			ar[i++] = rt;
			head = rt;
			pshift -= 2;
		}
	}
	cycle(width, ar, i);
}

static void trinkle(unsigned char *head, size_t width, cmpfun cmp, void *arg, size_t pp[2], int pshift, int trusty, size_t lp[]) {
	unsigned char *stepson, *rt, *lf;
	size_t p[2];
	unsigned char *ar[14 * sizeof(size_t) + 1];
	int i = 1;
	int trail;

	p[0] = pp[0];
	p[1] = pp[1];

	ar[0] = head;
	while(p[0] != 1 || p[1] != 0) {
		stepson = head - lp[pshift];
		if (cmp(stepson, ar[0], arg) <= 0) {
			break;
		}
		if (!trusty && pshift > 1) {
			rt = head - width;
			lf = head - width - lp[pshift - 2];
			if (cmp(rt, stepson, arg) >= 0 || cmp(lf, stepson, arg) >= 0) {
				break;
			}
		}

		ar[i++] = stepson;
		head = stepson;
		trail = pntz(p);
		shr(p, trail);
		pshift += trail;
		trusty = 0;
	}
	if (!trusty) {
		cycle(width, ar, i);
		sift(head, width, cmp, arg, pshift, lp);
	}
}

void emb_qsort_r(void *base, size_t nel, size_t width, cmpfun cmp, void *arg) {
	size_t lp[ 12 * sizeof(size_t)];
	size_t i, size = width * nel;
	unsigned char *head, *high;
	size_t p[2] = {1, 0};
	int pshift = 1;
	int trail;

	if (!size) return;

	head = base;
	high = head + size - width;

	/* Precompute Leonardo numbers, scaled by element width */
	for (lp[0]=lp[1]=width, i=2; (lp[i]=lp[i-2]+lp[i-1]+width) < size; i++);

	while(head < high) {
		if ((p[0] & 3) == 3) {
			sift(head, width, cmp, arg, pshift, lp);
			shr(p, 2);
			pshift += 2;
		} else {
			if ((ptrdiff_t) lp[pshift - 1] >= high - head) {
				trinkle(head, width, cmp, arg, p, pshift, 0, lp);
			} else {
				sift(head, width, cmp, arg, pshift, lp);
			}

			if (pshift == 1) {
				shl(p, 1);
				pshift = 0;
			} else {
				shl(p, pshift - 1);
				pshift = 1;
			}
		}

		p[0] |= 1;
		head += width;
	}

	trinkle(head, width, cmp, arg, p, pshift, 0, lp);

	while(pshift != 1 || p[0] != 1 || p[1] != 0) {
		if (pshift <= 1) {
			trail = pntz(p);
			shr(p, trail);
			pshift += trail;
		} else {
			shl(p, 2);
			pshift -= 2;
			p[0] ^= 7;
			shr(p, 1);
			trinkle(head - lp[pshift] - width, width, cmp, arg, p, pshift + 1, 1, lp);
			shl(p, 1);
			p[0] |= 1;
			trinkle(head - width, width, cmp, arg, p, pshift, 1, lp);
		}
		head -= width;
	}
}
#endif

#endif // GUARD_UTIL_C
