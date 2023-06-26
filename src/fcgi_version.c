// You may use the config below for this fastcgi application
//
// server {
//     listen 8080;
//     location / {
//         include /etc/nginx/fastcgi_params;
//         fastcgi_pass unix:/tmp/cblog.sock;
//     }
//     location /static/ {
//         autoindex off;
//         disable_symlinks on;
//         root CHANGEME/cblog/;
//     }
// }

#define _BSD_SOURCE
#define _DEFAULT_SOURCE

#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#include <sys/stat.h>
#include <fcgiapp.h>
#include <limits.h>
#include <fcgios.h>

#include "app.c"
#include "common.c"

const char * const sockpath = "/tmp/cblog.sock";

static int s_signo = 0;
static void signal_handler(int signo) {
	FCGX_ShutdownPending();
	s_signo = signo;
}

static void write_fun(const void *addr, unsigned long amount, void *context) {
	if (amount == 0) return;
	FCGX_Request *request = context;
	if (amount > INT_MAX) amount = INT_MAX;
	FCGX_PutStr(addr, (int) amount, request->out);
}

static void set_http_status_and_hdr_fun_stub(unsigned short status, const char * const *headers, void *context) {}

static void set_http_status_and_hdr_fun(unsigned short status, const char * const *headers, void *context) {
	FCGX_Request *request = context;
	if (status > 999 or status < 100) status = 503;
	FCGX_FPrintF(request->out, "Status: %u\r\n", status);

	if (headers != NULL) {
		for (unsigned i = 0; headers[i] != NULL; i++) {
			FCGX_FPrintF(request->out, "%s\r\n", headers[i]);
		}
	}
	FCGX_PutStr("\r\n", 2, request->out);

	app_write = write_fun;
	set_http_status_and_hdr = set_http_status_and_hdr_fun_stub;
}

static void write_fun_stub(const void *addr, unsigned long amount, void *context) {
	set_http_status_and_hdr_fun(200, NULL, context);
	write_fun(addr, amount, context);
}

#ifndef NO_NGINX_KLUDGE
const char * const nginx_headers_table[] = {
	"Host", "HTTP_HOST",
	"Content-Type", "CONTENT_TYPE",
	"Content-Length", "CONTENT_LENGTH",
	"User-Agent", "HTTP_USER_AGENT",
	"Accept", "HTTP_ACCEPT",
	"Accept-Language", "HTTP_ACCEPT_LANGUAGE",
	"Accept-Encoding", "HTTP_ACCEPT_ENCODING",
	"Connection", "HTTP_CONNECTION",
	"Upgrade-Insecure-Requests", "HTTP_UPGRADE_INSECURE_REQUESTS",
	NULL, NULL
};
// Unfortunately, NGINX can't pass all http headers, only predefined.
// Also, even if you are going to use them, defaults aren't looks the same
// as headers. For example, you may look for a "User-Agent", but under nginx that
// would be HTTP_USER_AGENT. Currently I would like to know what would be the
// best solution to unify it's usage... Well, currently I can see only few options here:
// 1. Edit nginx configuration and change these variables names to the names that matches
//    header. For example, in /etc/nginx/fastcgi_params you can change CONTENT_TYPE
//    to Content-Type
// 2. Make a patch in nginx to allow pass headers as is, LOL
// 3. Make a kludge in this software, like, correspondence headers_table, that silently changes
//    string from User-Agent to HTTP_USER_AGENT just because of nginx
// 4. Ask a application developer to support both (no, I definitely don't like it)
static const char *locate_header_fun(const char *hdr, size_t *len, void *context) {
	const char *looking_for = NULL;
	for (unsigned i = 0; nginx_headers_table[i] != NULL; i+=2) {
		if (strcmp(nginx_headers_table[i], hdr) == 0) {
			looking_for = nginx_headers_table[i + 1];
			break;
		}
	}
	if (looking_for == NULL) return NULL;
	hdr = looking_for;
#else
static const char *locate_header_fun(const char *hdr, size_t *len, void *context) {
#endif // NO_NGINX_KLUDGE
	FCGX_Request *r = context;
	char *result = FCGX_GetParam(hdr, r->envp);
	if (result == NULL) return NULL;
	*len = strlen(result);
	return result;
}

int fd;
void *worker(void *arg) {
	FCGX_Request request;
	FCGX_InitRequest(&request, fd, 0);
	char workerbuffer[CONTEXTAPPBUFFERSIZE];
	memcpy(workerbuffer, arg, sizeof(struct appcontext));

	while (1) {
		if (FCGX_Accept_r(&request) == -1) break;
		app_write = write_fun_stub;
		set_http_status_and_hdr = set_http_status_and_hdr_fun;
//		char **ptr = request.envp;
//		while(*ptr != NULL) {
//			puts(*ptr);
//			ptr++;
//		}

		const char *req = FCGX_GetParam("DOCUMENT_URI", request.envp);
		const char *method = FCGX_GetParam("REQUEST_METHOD", request.envp);
		const char *query = FCGX_GetParam("QUERY_STRING", request.envp);

		reqargs a = {.servercontext1 = &request,
					 .servercontext2 = &request,
					 .request = req,
					 .request_len = strlen(req),
					 .query = query,
					 .query_len = query != NULL ? strlen(query) : 0,
					 .appcontext = workerbuffer,
					 .method = http_determine_method(method, strlen(method))
		};
		app_request(a);
		FCGX_Finish_r(&request);
		if (s_signo) break;
	}
	return NULL;
}

int main(int argc, char **argv) {
	fd = FCGX_OpenSocket(sockpath, 128);
	FCGX_Init();
	if (fd < 0) return EXIT_FAILURE;
	chmod(sockpath, 0777);
	signal(SIGINT, signal_handler);  // threads WILL NOT exit unless they finish request,
	signal(SIGTERM, signal_handler); // even if it's still received. Thats a todo.
	locate_header = locate_header_fun;

	set_config_defaults(&config);
	if (argc > 1) {
		const char *error;
		bool ret = parse_config(&config, argv[1], &error);
		if (ret == false) {
			printf("%s\n", error);
			return ret;
		}
	}

	char contextbuffer[sizeof(struct appcontext)];
	void *appcontext = contextbuffer;
	if (app_prepare(&appcontext) == false) {
		printf("Unable to initialize app");
		parse_config_erase(&config);
		return EXIT_FAILURE;
	}

	const unsigned n_threads = 4;
	pthread_t threads[n_threads];
	for (unsigned i = 0; i < n_threads; i++ ) {
		pthread_create(&threads[i], NULL, worker, appcontext);
	}

	for (unsigned i = 0; i < n_threads; i++ ) {
		pthread_join(threads[i], NULL);
	}
	app_finish(appcontext);
	unlink(sockpath);
	parse_config_erase(&config);
	OS_LibShutdown(); // IMO function should be exported.

	return EXIT_SUCCESS;
}
