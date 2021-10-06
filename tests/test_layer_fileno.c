#define _BSD_SOURCE
#define _DEFAULT_SOURCE

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define DATA_LAYER_FILENO
#include "../abstract_data_layer.c"

int main() {
	struct context con;
	const char *error;
	if (initialize_engine(ENGINE_FILENO, "fileno_testset", &con, &error) == false) {
		printf("Failed to initialize engine!\n");
		return EXIT_FAILURE;
	}

#define RLIM 3
	unsigned amount = RLIM;
	unsigned long list[RLIM];

	if (list_records(&amount, list, 1, (ttime_t) 0l, (ttime_t) 2147483647l, &con, &error) == false) {
		printf("Failed to initialize context! Error: %s\n", error);
		return EXIT_FAILURE;
	}

	for (unsigned i = 0; i < amount; i++) {
		printf("Found record: %lu\n", list[i]);

		char buffer[512];
		struct blog_record b = {.stack = buffer, .stack_space = sizeof(buffer)};
		if (get_record(&b, list[i], &con, &error) == false) {
			printf("get_record() failed! Reason: %s", error);
		} else {
			printf("Record name: %s\nRecord data: %s\n", b.recordname, b.recordcontent);
		}
	}

	return EXIT_SUCCESS;
}
