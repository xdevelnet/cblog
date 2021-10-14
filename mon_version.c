#include <signal.h>
#include <iso646.h>
#include "mongoose.h"
#include "app.c"

static int s_signo;
static void signal_handler(int signo) {
	s_signo = signo;
}

static void write_fun(const void *addr, unsigned long amount, void *context) {
	mg_http_write_chunk(context, addr, amount);
}

static void set_http_status_and_hdr_fun_stub(unsigned short status, const char **headers, void *context) {}

static void set_http_status_and_hdr_fun(unsigned short status, const char **headers, void *context) {
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

static const char *locate_header_fun(const char *hdr, size_t *len, void *context) {
	struct mg_str *m = mg_http_get_header(context, hdr);
	if (m == NULL) return NULL;
	*len = m->len;
	return m->ptr;
}

static void cb(struct mg_connection *c, int ev, void *ev_data, void *fn_data) {
	if (ev != MG_EV_HTTP_MSG) return;
	app_write = write_fun_stub;
	set_http_status_and_hdr = set_http_status_and_hdr_fun;

	struct mg_http_message *hm = (struct mg_http_message *) ev_data;
	appargs a = {.context1 = c,
				 .context2 = hm,
				 .request = hm->uri.ptr,
				 .request_len = hm->uri.len,
				 .appcontext = fn_data,
				 .method = http_determine_method(hm->method.ptr, hm->method.len)
	};
	app_request(a);
	mg_http_write_chunk(c, "", 0);
}

int main() {
	signal(SIGINT, signal_handler);
	signal(SIGTERM, signal_handler);
	locate_header = locate_header_fun;

	struct mg_mgr mgr;
	mg_mgr_init(&mgr);
	mg_log_set("1");
	void *appcontext;
	app_prepare(&appcontext);
	if ((mg_http_listen(&mgr, "http://0.0.0.0:8000", cb, appcontext)) == NULL) {
		LOG(LL_ERROR, ("Cannot listen!"));
		return EXIT_FAILURE;
	}

	while (s_signo == 0) mg_mgr_poll(&mgr, 1000);
	mg_mgr_free(&mgr);
	app_finish(appcontext);
	return EXIT_SUCCESS;
}
