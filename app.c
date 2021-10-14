#if !defined strizeof
#define strizeof(a) (sizeof(a)-1)
#endif

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
void (*set_http_status_and_hdr) (unsigned short, const char **, void *);
void (*app_write) (const void *, unsigned long, void *);

#define REQUEST a.request
#define REQUEST_LEN a.request_len
#define METHOD a.method
#define CONTEXT a.appcontext
#define LOCATE_HEADER(arg1, arg2) locate_header(arg1, arg2, a.context2)
#define SET_HTTP_STATUS_AND_HDR(arg1, arg2) set_http_status_and_hdr(arg1, arg2, a.context1)
#define APP_WRITE(arg1, arg2) app_write(arg1, arg2, a.context1)

const char *table[] = {"Content-Type: text/html;charset=utf-8", "Server: OLOLOLO TROLOLO", NULL};

void app_prepare(void **ptr) {
//	*ptr = malloc(1000);
}

void app_finish(void *ptr) {
//	free(ptr);
}

void app_request(appargs a) {
	SET_HTTP_STATUS_AND_HDR(200, table);
	APP_WRITE("Your request is: ", 17);
	APP_WRITE(REQUEST, REQUEST_LEN);
	APP_WRITE("\n", 1);
	APP_WRITE("your user agent: ", 17);
	size_t s;
	const char *ua = LOCATE_HEADER("User-Agent", &s);
	if (ua != NULL) APP_WRITE(ua, s);
}
