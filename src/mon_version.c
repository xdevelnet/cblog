#define _DEFAULT_SOURCE
#define _BSD_SOURCE

#include <signal.h>
#include <iso646.h>
#include "mongoose.h"
#include "app.c"
#include "common.c"

static int s_signo = 0;
static void signal_handler(int signo) {
	s_signo = signo;
}

static void write_fun(const void *addr, unsigned long amount, void *context) {
	if (amount == 0) return;
	mg_http_write_chunk(context, addr, amount);
}

static void set_http_status_and_hdr_fun_stub(unsigned short status, const char * const *headers, void *context) {}

static void set_http_status_and_hdr_fun(unsigned short status, const char * const *headers, void *context) {
	if (status > 999 or status < 100) status = 503;
	mg_printf(context, "HTTP/1.1 %u\r\nTransfer-Encoding: chunked\r\n", status);
	if (headers != NULL) {
		for (unsigned i = 0; headers[i] != NULL; i++) {
			mg_send(context, headers[i], strlen(headers[i]));
			mg_send(context, "\r\n", 2);
		}
	}
	mg_send(context, "\r\n", 2);

	app_write = write_fun;
	set_http_status_and_hdr = set_http_status_and_hdr_fun_stub;
}

static void write_fun_stub(const void *addr, unsigned long amount, void *context) {
	set_http_status_and_hdr_fun(200, NULL, context);
	write_fun(addr, amount, context);
}

static void read_fun(void *addr, unsigned long *amount, void *context) {
	struct mg_http_message *hm = context;
	if (*amount > INT_MAX) *amount = INT_MAX;
	*amount = (unsigned long) hm->body.len;
	if (*amount > 0) memcpy(addr, hm->body.ptr, *amount);
}

static const char *locate_header_fun(const char *hdr, size_t *len, void *context) {
	struct mg_str *m = mg_http_get_header(context, hdr);
	if (m == NULL) return NULL;
	*len = m->len;
	return m->ptr;
}

static void cb(struct mg_connection *c, int ev, void *ev_data, void *fn_data) {
	if (ev != MG_EV_HTTP_MSG) return;
	struct mg_http_message *hm = (struct mg_http_message *) ev_data;
//	if (mg_http_match_uri(hm, "/static/**")) {
// that ugly shit below is the reason of mg_http_serve_dir() retardness. It's unable to serve dir as itself
// so, you can only access /static/static with such URI. Facepalm.jpg.
	static const unsigned staticstrsize = strizeof("/static");
	if (hm->uri.len > staticstrsize and memcmp(hm->uri.ptr, "/static/", staticstrsize) == 0) {
		memmove((char *)hm->uri.ptr, hm->uri.ptr +  staticstrsize, hm->uri.len - staticstrsize);
		hm->uri.len -= staticstrsize;
		struct mg_http_serve_opts opts = {.root_dir = "static"}; // please help me to restrict access to anything except /static/
		mg_http_serve_dir(c, ev_data, &opts);
		return;
	}

	app_write = write_fun_stub;
	app_read = read_fun;
	set_http_status_and_hdr = set_http_status_and_hdr_fun;
	reqargs a = {.servercontext1 = c,
				 .servercontext2 = hm,
				 .request = hm->uri.ptr,
				 .request_len = hm->uri.len,
				 .query = hm->query.ptr,
				 .query_len = hm->query.len,
				 .appcontext = fn_data,
				 .method = http_determine_method(hm->method.ptr, hm->method.len)
	};

//	size_t i, max = sizeof(hm->headers) / sizeof(hm->headers[0]);
//	printf("Request headers:\n");
//	// Iterate over request headers, and print them one by one
//	for (i = 0; i < max && hm->headers[i].name.len > 0; i++) {
//		struct mg_str *kik = &hm->headers[i].name, *v = &hm->headers[i].value;
//		printf("%.*s -> %.*s\n", (int) kik->len, kik->ptr, (int) v->len, v->ptr);
//	}

	app_request(a);
	mg_http_write_chunk(c, "", 0);
}

int randfd;
void rfill(void *ptr, size_t size) {
	ssize_t got = read(randfd, ptr, size);
	if (got < 0) unsafe_rand(ptr, size);
}

int main(int argc, char **argv) {
	signal(SIGINT, signal_handler);
	signal(SIGTERM, signal_handler);
	locate_header = locate_header_fun;
	randfd = open("/dev/urandom", O_RDONLY);
	if (randfd < 0) return EXIT_FAILURE;
	struct appconfig config = {.r = rfill};
	set_config_defaults(&config);
	if (argc > 1) {
		const char *error;
		bool ret = parse_config(&config, argv[1], &error);
		if (ret == false) {
			printf("%s\n", error);
			return ret;
		}
	}

	struct mg_mgr mgr;
	mg_mgr_init(&mgr);
	mg_log_set(MG_LL_INFO);
	char contextbuffer[CONTEXTAPPBUFFERSIZE];
	void *appcontext = contextbuffer;
	if (app_prepare(&appcontext, &config) == false) {
		parse_config_erase(&config);
		MG_ERROR(("Unable to initialize app, reason: %s\n", contextbuffer));
		return EXIT_FAILURE;
	}
	if ((mg_http_listen(&mgr, "http://0.0.0.0:8000", cb, appcontext)) == NULL) {
		parse_config_erase(&config);
		MG_ERROR(("Cannot listen!"));
		return EXIT_FAILURE;
	}

	while (s_signo == 0) mg_mgr_poll(&mgr, 1000);
	mg_mgr_free(&mgr);
	app_finish(appcontext);
	parse_config_erase(&config);
	return EXIT_SUCCESS;
}
