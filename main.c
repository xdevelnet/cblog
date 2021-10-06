#define _BSD_SOURCE
#define _DEFAULT_SOURCE

#include <stdlib.h>
#include <threads.h>
#include <sys/stat.h>
#include <fcgiapp.h>

#define DATA_LAYER_FILENO
#define DATA_LAYER_MYSQL
#include "abstract_data_layer.c"

const char* const sockpath = "/tmp/eclock.sock";

int worker(void *arg) {
	FCGX_Init();
	FCGX_Request request;
	int *fd = arg;
	FCGX_InitRequest(&request, *fd, 0);

	while (1) {
		FCGX_Accept_r(&request);

		FCGX_PutStr("Content-type: text/plain\r\n\r\n", 28, request.out);
		FCGX_PutStr("Hey!\n", 5, request.out);

		FCGX_Finish_r(&request);
	}
}

int main(void) {
	int fd = FCGX_OpenSocket(sockpath, 128);
	if (fd < 0) return EXIT_FAILURE;
	chmod(sockpath, 0777);

	const unsigned n_threads = 4;
	thrd_t threads[n_threads];
	for (unsigned i = 0; i < n_threads; i++ ) {
		thrd_create(&threads[i], worker, &fd);
	}

	for (unsigned i = 0; i < n_threads; i++ ) {
		thrd_join(threads[i], NULL);
	}

	return EXIT_SUCCESS;
}
