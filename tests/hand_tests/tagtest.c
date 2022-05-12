#include <stdio.h>
#include <iso646.h>
#include <string.h>

struct blog_record {
	void *stack;
	size_t stack_space;
	char **tags;
};

static char *skip_spaces(char *ptr) {
	while(*ptr == ' ') ptr++;
	return ptr;
}

size_t taglen(const char *str) {
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

unsigned char_occurences(const char *str, char lf) {
	const char *temp = str;
	unsigned count = 0;
	while( (temp = strchr(temp, lf)) != NULL) {
		count++;
		temp++;
	}

	return count;
}

void tag_processing(char *comma_separated_tags, struct blog_record *r) {
	size_t tags_amount = char_occurences(comma_separated_tags, ',') + 1;
	char *put = (char *) r->stack + sizeof(void *) * (tags_amount + 1);
	r->tags = r->stack;
	r->tags[tags_amount] = NULL;
	for (size_t i = 0; i < tags_amount; i++) {
		comma_separated_tags = skip_spaces(comma_separated_tags);
		size_t l = taglen(comma_separated_tags);
		r->tags[i] = memcpy(put, comma_separated_tags, l);
		r->tags[i][l] = '\0';
		put += l + 1;
		comma_separated_tags += l;
		comma_separated_tags = skip_spaces(comma_separated_tags);
		//if (*comma_separated_tags == '\n') break;
		comma_separated_tags++;
	}
}

void test(char *tags) {
	char stack[16500];
	struct blog_record r = {.stack = stack, .stack_space = sizeof(stack)};
	tag_processing(tags, &r);
	for(unsigned i = 0; r.tags[i] != NULL; i++) {
		printf("%u: %p\n", i, r.tags[i]);
		puts(r.tags[i]);
		putchar('\n');
	}
}

int main() {
	test("aaaaa, bbbb, ccccc\n");
	test("wwwwwww\n");
	test("   eee  , oooo , bbbb\n");
	test("rrrr,tttt,yyyy\n");
	test("uuuu ,iiii ,oooo\n");
	return 0;
}
