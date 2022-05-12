#include <stdio.h>
#include <iso646.h>
#include <stdbool.h>
#include <string.h>

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

void normalize_filename(char *name) {
	char *push = name;
#define PUSH_NEWN(bytes, width) {memcpy(push, bytes, (width)); push += (width);}
	
	while(1) {
		if (*name == '\0') {*push = '\0'; break;}
		unsigned char width = utf8_byte_width(name);
		if (width != sizeof(char) or should_i_skip(*name) != true) PUSH_NEWN(name, width);
		name += width;
	}
}

#define TEST(str) {strcpy(name, str);normalize_filename(name);printf("%s\n", name);}

int main() {
	char name[255 + 1];


	TEST("abcdef");
	TEST("мікрокомпонент? НІТ!");
	TEST("мікро\\челік Папіча: лютого венціанскьго маніяка");
	TEST("фівіфв/віфвфі");
	TEST("%%/%%/%%.\\ПРИВІТ, АНДРІЙ!");
	TEST("Статья:/ ХЕЛЛОУ");
	TEST("プーチン・フイロ");


	return 0;
}
