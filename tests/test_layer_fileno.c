#define _BSD_SOURCE
#define _DEFAULT_SOURCE
#define _XOPEN_SOURCE 600

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <ftw.h>

#define DATA_LAYER_FILENO
#include "../src/util.c"
#include "../src/abstract_data_layer.c"

static int unlink_cb(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf) {
	int rv = remove(fpath);

	if (rv) perror(fpath);

	return rv;
}

static inline int rmrf(const char *path) {
	return nftw(path, unlink_cb, 64, FTW_DEPTH | FTW_PHYS);
}

int randfd;
void rfill(void *ptr, size_t size) {
	ssize_t got = read(randfd, ptr, size);
	if (got < 0) unsafe_rand(ptr, size);
}

#define TESTSETPATH "fileno_testset"

int main() {
	rmrf(TESTSETPATH);
	mkdir(TESTSETPATH, 0777);

	randfd = open("/dev/urandom", O_RDONLY);
	if (randfd < 0) {
		perror("Can open urandom");
		return EXIT_FAILURE;
	}

	struct layer_context con;
	struct data_layer d = {.e = ENGINE_FILENO, TESTSETPATH, .context = &con, .randfun = rfill};
	const char *error;
	if (initialize_engine(&d, &error) == false) {
		printf("Failed to initialize engine: %s\n", error);
		return EXIT_FAILURE;
	}

	char buffer[35250];
	char *tags[] = {"abc", "def", "ghi", "jkl", NULL};

	const char test_data[] = "# Hello!\n"
							 "## abc\n"
							 "dasdasdas\n"
							 "\n"
							 "DSADASDASD\n"
							 "\n"
							 "__OH NO? OH YEEES!__";

	const char test_data2[] = "# h1 Heading 8-)\n"
							  "## h2 Heading\n"
							  "### h3 Heading\n"
							  "#### h4 Heading\n"
							  "##### h5 Heading\n"
							  "###### h6 Heading\n"
							  "\n"
							  "\n"
							  "## Horizontal Rules\n"
							  "\n"
							  "___\n"
							  "\n"
							  "---\n"
							  "\n"
							  "***\n"
							  "\n"
							  "\n"
							  "## Typographic replacements\n"
							  "\n"
							  "Enable typographer option to see result.\n"
							  "\n"
							  "(c) (C) (r) (R) (tm) (TM) (p) (P) +-\n"
							  "\n"
							  "test.. test... test..... test?..... test!....\n"
							  "\n"
							  "!!!!!! ???? ,,  -- ---\n"
							  "\n"
							  "\"Smartypants, double quotes\" and 'single quotes'\n"
							  "\n"
							  "\n"
							  "## Emphasis\n"
							  "\n"
							  "**This is bold text**\n"
							  "\n"
							  "__This is bold text__\n"
							  "\n"
							  "*This is italic text*\n"
							  "\n"
							  "_This is italic text_\n"
							  "\n"
							  "~~Strikethrough~~";

	struct blog_record b = {
		.stack = buffer,
		.stack_space = sizeof(buffer),
		.title = "ITS MY GOOD TITLE!",
		.titlelen = sizeof("ITS MY GOOD TITLE!") - 1,
		.data = test_data,
		.datalen = strizeof(test_data),
		.display = DISPLAY_BOTH,
		.tags = tags,
	};
	if (insert_record(&b, &con, &error) == false) {
		printf("Failed insert record: Error: %s\n", error);
		return EXIT_FAILURE;
	}

	memset(&b, '\0', sizeof(b));
	b.stack = buffer;
	b.stack_space = sizeof(buffer);
	b.title = "Мій новий заголовок? Ось він: 記事のタイトル";
	b.titlelen = strlen(b.title);
	b.data = test_data2;
	b.datalen = strizeof(test_data2);
	b.display = DISPLAY_DATASOURCE;
	b.tags = tags; // yes, I am attempting to use the same tags in order to test how they will be filled

	if (insert_record(&b, &con, &error) == false) {
		printf("Failed insert record #2: Error: %s\n", error);
		return EXIT_FAILURE;
	}

#define RLIM 3
	unsigned amount = RLIM;
	unsigned long list[RLIM];

	struct list_filter filter = {.from.t = 0l, .to.t = 2147483647l};
	if (list_records(&amount, list, 0, filter, &con, &error) == false) {
		printf("Failed list records! Error: %s\n", error);
		return EXIT_FAILURE;
	}

	printf("amount: %u\n", amount);

	for (unsigned i = 0; i < amount; i++) {
		printf("Found record: %lu\n", list[i]);
		struct blog_record b1 = {
			.stack = buffer,
			.stack_space = sizeof(buffer),
		};
		bool result = get_record(&b1, list[i], &con, &error);
		if (result == false) printf("Failed to get record: %lu Reason: %s\n", list[i], error);

		printf("title: %.*s\ndata: %.*s\ndatasource: %.*s\ndisplay: %i\n",
			   b1.titlelen, b1.title,
			   b1.datalen, b1.data,
			   b1.datasourcelen, b1.datasource,
			   b1.display);
		char **tags1 = b1.tags;
		while(*tags1 != NULL) {
			printf("tag: %s\n", *tags1);
			tags1++;
		}
	}

	deinitialize_engine(ENGINE_FILENO, &con);

	return EXIT_SUCCESS;
}
