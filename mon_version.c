#define _DEFAULT_SOURCE
#define _BSD_SOURCE

#include <signal.h>
#include <iso646.h>
#include "mongoose.h"
#include "app.c"

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
	set_http_status_and_hdr = set_http_status_and_hdr_fun;
	reqargs a = {.servercontext1 = c,
				 .servercontext2 = hm,
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

	// here you can add cli parsing or even config file reading
	config.appname = default_appname;
	config.appnamelen = default_appnamelen;
	config.template_type = default_template_type;
	config.temlate_name = default_template_name;
	config.datalayer_type = default_datalayer_type;
	config.datalayer_addr = default_datalayer_addr;
	config.title_page_name = default_title_page_name;
	config.title_page_name_len = default_title_page_len;
	config.title_page_content = default_title_content;
	config.title_page_content_len = default_title_content_len;

	struct mg_mgr mgr;
	mg_mgr_init(&mgr);
	mg_log_set("1");
	char contextbuffer[CONTEXTAPPBUFFERSIZE];
	void *appcontext = contextbuffer;
	if (app_prepare(&appcontext) == false) {
		MG_ERROR(("Unable to initialize app"));
		return EXIT_FAILURE;
	}
	if ((mg_http_listen(&mgr, "http://0.0.0.0:8000", cb, appcontext)) == NULL) {
		MG_ERROR(("Cannot listen!"));
		return EXIT_FAILURE;
	}

	while (s_signo == 0) mg_mgr_poll(&mgr, 1000);
	mg_mgr_free(&mgr);
	app_finish(appcontext);
	return EXIT_SUCCESS;
}
