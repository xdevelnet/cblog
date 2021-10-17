#if !defined strizeof
#define strizeof(a) (sizeof(a)-1)
#endif

#if !defined STRINGIZE
#define STRINGIZE(a) #a
#endif

#include <stdio.h>

#define DATA_LAYER_FILENO
#define DATA_LAYER_MYSQL
#include "abstract_data_layer.c"
#include "libessb.c"

typedef enum {POST, GET, PUT, PATCH, DELETE, UNKNOWN} http_methods;

http_methods http_determine_method(const char *ptr, size_t len) {
	switch (len) {
		case strizeof("GET"): // also case strizeof("PUT"):
			if (memcmp(ptr, "GET", strizeof("GET")) == 0) return GET;
			if (memcmp(ptr, "PUT", strizeof("PUT")) == 0) return PUT;
			return UNKNOWN;
		case strizeof("POST"):
			if (memcmp(ptr, "POST", strizeof("POST")) == 0) return POST;
			return UNKNOWN;
		case strizeof("DELETE"):
			if (memcmp(ptr, "DELETE", strizeof("DELETE")) == 0) return DELETE;
			return UNKNOWN;
		case strizeof("PATCH"):
			if (memcmp(ptr, "PATCH", strizeof("PATCH")) == 0) return PATCH;
			// fallthrough
		default:
			return UNKNOWN;
	}
}

typedef struct appargs {
	const char *request;
	size_t request_len;
	http_methods method;
	void *appcontext;
	void *context1;
	void *context2;
} appargs;

const char *(*locate_header)(const char *, size_t *, void *);
void (*set_http_status_and_hdr) (unsigned short, const char * const *, void *);
void (*app_write) (const void *, unsigned long, void *);

#define REQUEST a.request
#define REQUEST_LEN a.request_len
#define METHOD a.method
#define CONTEXT a.appcontext
#define LOCATE_HEADER(arg1, arg2) locate_header(arg1, arg2, a.context2)
#define SET_HTTP_STATUS_AND_HDR(arg1, arg2) set_http_status_and_hdr(arg1, arg2, a.context1)
#define APP_WRITE(arg1, arg2) app_write(arg1, arg2, a.context1)

const char *headers_table[] = {"Content-Type: text/html;charset=utf-8", "Server: OLOLOLO TROLOLO", NULL};

#define CONTEXTAPPBUFFERSIZE 500

struct appcontext {
	essb templates;
};

// expected pages
#define TITLE_PAGE_PART 1
#define TITILE_PAGE_NAME "title"
#define FOOTER_PAGE_PART 2
#define FOOTER_PAGE_NAME "footer"
#define CONTENT_PAGE_PART 3
#define CONTENT_PAGE_NAME "content"

static inline int32_t template_name_to_template_number(const char *name, int32_t length) {
	switch (-length) {
	case strizeof(TITILE_PAGE_NAME):
		if (memcmp(name, TITILE_PAGE_NAME, -length) == 0) return -TITLE_PAGE_PART;
		return -length;
	case strizeof(FOOTER_PAGE_NAME):
		if (memcmp(name, FOOTER_PAGE_NAME, -length) == 0) return -FOOTER_PAGE_PART;
		return -length;
	case strizeof(CONTENT_PAGE_NAME):
		if (memcmp(name, CONTENT_PAGE_NAME, -length) == 0) return -CONTENT_PAGE_PART;
		return -length;
	default:
		return -length;
	}
}

bool app_prepare(void **ptr) {
	struct appcontext *a = *ptr;
	essb *e = &a->templates;
	memset(e, '\0', sizeof(essb));
	parse_essb(e, SOURCE_FILE, "static/minimalist/index (copy).ssb", NULL);
	if (e->errreasonstr != NULL) {
		snprintf(*ptr, CONTEXTAPPBUFFERSIZE, "Error during parsing essb: %s", e->errreasonstr);
		return false;
	}

	for (unsigned i = 0; i < e->records_amount; i++) {
		if (e->record_size[i] >= 0) continue;
		e->record_size[i] = template_name_to_template_number(&e->records[e->record_seek[i]], e->record_size[i]);
	}
	return true;
}

void app_finish(void *ptr) {
	struct appcontext *a = ptr;
	essb *e = &a->templates;
	free(e->records);
}

static bool em_isdigit(char c) {
	if (c >= '0' && c <= '9') return true;
	return false;
}

uint32_t get_u32_from_end_of_string(const char *string, size_t len) {
	const char *locate = string + len;
	while(em_isdigit(*(locate-1))) {
		locate--;
		if (locate == string) break;
	}

	if (em_isdigit(*locate) == false) return UINT32_MAX;

	char buffer[strizeof(STRINGIZE(UINT32_MAX))];
	long digitslen = string + len - locate;
	memcpy(buffer, locate, digitslen);
	buffer[digitslen] = '\0';

	return strtoul(buffer, NULL, 10);
}

static void notfound(appargs a) {
	SET_HTTP_STATUS_AND_HDR(404, headers_table);

	const char *out[] = {
		[TITLE_PAGE_PART]   = "404 NOT FOUND",
		[FOOTER_PAGE_PART]  = "",
		[CONTENT_PAGE_PART] = "Sorry but we can't find content that you are looking for :("
	};
	size_t outsizes[] = {
		[TITLE_PAGE_PART]   = strizeof("404 NOT FOUND"),
		[FOOTER_PAGE_PART]  = strizeof(""),
		[CONTENT_PAGE_PART] = strizeof("Sorry but we can't find content that you are looking for :(")
	};

	essb *e = a.appcontext;
	for (unsigned i = 0; i < e->records_amount; i++) {
		if (e->record_size[i] < 0) APP_WRITE(out[-e->record_size[i]], outsizes[-e->record_size[i]]);
		else APP_WRITE(&e->records[e->record_seek[i]], e->record_size[i]);
	}
}

static void title(appargs a) {
	APP_WRITE("Your request is: ", 17);
	APP_WRITE(REQUEST, REQUEST_LEN);
	APP_WRITE("\n", 1);
	APP_WRITE("your user agent: ", 17);
	size_t s;
	const char *ua = LOCATE_HEADER("User-Agent", &s);
	if (ua != NULL) APP_WRITE(ua, s);
}

static void show_record(appargs a, uint32_t record) {
	if (record == UINT32_MAX) return notfound(a);
	return notfound(a);
}

void app_request(appargs a) {
	if (METHOD != GET or REQUEST_LEN == 0 or REQUEST[0] != '/') return notfound(a);
	if (REQUEST_LEN == 1 and REQUEST[0] == '/') return title(a);
	return show_record(a, get_u32_from_end_of_string(REQUEST, REQUEST_LEN));
}
