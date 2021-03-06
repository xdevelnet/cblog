#ifndef GUARD_APP_C
#define GUARD_APP_C

#include "util.c"

#define DATA_LAYER_FILENO
#define DATA_LAYER_MYSQL
#include "abstract_data_layer.c"
#include "libessb.c"

#include "default_rodata.h"

struct appconfig {
	void *context;
	size_t contextsize;
	const char *appname;
	size_t appnamelen;
	enum datalayer_engines datalayer_type;
	const void *datalayer_addr;
	source_type template_type;
	const char *temlate_name;
	const char *title_page_name;
	size_t title_page_name_len;
	const char *title_page_content;
	size_t title_page_content_len;
} config;

struct appcontext {
	essb templates;
	struct layer_context layer;
	char freebuffer[];
};

typedef enum {POST, GET, PUT, PATCH, DELETE, UNKNOWN} http_methods;
typedef struct reqargs {
	const char *request;
	size_t request_len;
	http_methods method;
	void *appcontext; // struct appcontext
	void *servercontext1;
	void *servercontext2;
} reqargs;

const char *(*locate_header)(const char *, size_t *, void *);
void (*set_http_status_and_hdr) (unsigned short, const char * const *, void *);
void (*app_write) (const void *, unsigned long, void *);

#define REQUEST a.request
#define REQUEST_LEN a.request_len
#define METHOD a.method
#define CONTEXT a.appcontext
#define LOCATE_HEADER(arg1, arg2) locate_header(arg1, arg2, a.servercontext2)
#define SET_HTTP_STATUS_AND_HDR(arg1, arg2) set_http_status_and_hdr(arg1, arg2, a.servercontext1)
#define APP_WRITE(arg1, arg2) app_write(arg1, arg2, a.servercontext1)

#define APP_WRITECS(a) APP_WRITE(a, strizeof(a))

const char *headers_table[] = {"Content-Type: text/html;charset=utf-8", "Server: OLOLOLO TROLOLO", NULL};

#define CONTEXTAPPBUFFERSIZE 524288

// expected pages

enum pages {
	TITLE_PAGE_PART     = 1,
	SITENAME_PAGE_PART  = 2,
	FOOTER_PAGE_PART    = 3,
	CONTENT_PAGE_PART   = 4,
	REPEATONE_PAGE_PART = 5,
	REPEATTWO_PAGE_PART = 6,
	PAGES_MAX
};

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

#define TITILE_PAGE_NAME "title"
#define SITE_NAME "sitename"
#define FOOTER_PAGE_NAME "footer"
#define CONTENT_PAGE_NAME "content"
#define REPEATONE_PAGE_NAME "repeat_1"
#define REPEATTWO_PAGE_NAME "repeat_2"

static inline int32_t template_tag_to_number(const char *name, int32_t length) {
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
	case strizeof(REPEATONE_PAGE_NAME):
		if (memcmp(name, SITE_NAME, -length) == 0) return -SITENAME_PAGE_PART;
		if (memcmp(name, REPEATONE_PAGE_NAME, -length) == 0) return -REPEATONE_PAGE_PART;
		if (memcmp(name, REPEATTWO_PAGE_NAME, -length) == 0) return -REPEATTWO_PAGE_PART;
		return -length;
	default:
		return -length;
	}
}

bool app_prepare(void **ptr) {
	struct appcontext *con = *ptr;
	essb *e = &con->templates;
	struct layer_context *l = &con->layer;
	memset(e, '\0', sizeof(essb));
	parse_essb(e, config.template_type, config.temlate_name, NULL);
	if (e->errreasonstr != NULL) {
		snprintf(*ptr, CONTEXTAPPBUFFERSIZE, "Error during parsing essb: %s", e->errreasonstr);
		return false;
	}

	const char *error;
	if (initialize_engine(config.datalayer_type, config.datalayer_addr, l, &error) == false) {
		snprintf(*ptr, CONTEXTAPPBUFFERSIZE, "Error during initializing_engine: %s", error);
		return false;
	}

	for (unsigned i = 0; i < e->records_amount; i++) {
		if (e->record_size[i] >= 0) continue;
		e->record_size[i] = template_tag_to_number(&e->records[e->record_seek[i]], e->record_size[i]);
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

static void notfound(reqargs a) {
	SET_HTTP_STATUS_AND_HDR(404, headers_table);

	const char *out[PAGES_MAX] = {
		[SITENAME_PAGE_PART] = config.appname,
		[TITLE_PAGE_PART]    = "404 NOT FOUND",
		[CONTENT_PAGE_PART]  = "Sorry but we can't find content that you are looking for :(",
	};
	size_t outsizes[PAGES_MAX] = {
		[SITENAME_PAGE_PART] = config.appnamelen,
		[TITLE_PAGE_PART]    = strizeof("404 NOT FOUND"),
		[CONTENT_PAGE_PART]  = strizeof("Sorry but we can't find content that you are looking for :("),
	};

	struct appcontext *con = CONTEXT;
	essb *e = &con->templates;
	for (unsigned i = 0; i < e->records_amount; i++) {
		if (e->record_size[i] < 0) APP_WRITE(out[-e->record_size[i]], outsizes[-e->record_size[i]]);
		else APP_WRITE(&e->records[e->record_seek[i]], e->record_size[i]);
	}
}

static void rewind_back(essb *e, long looking_for, unsigned *position) {
	while(e->record_size[*position] != -looking_for) --*position;
	--*position;
}

struct select {
	unsigned iter;
	unsigned limit;
	unsigned long *found;
	unsigned position;
};

static inline void selector_show_tag_processing(reqargs a, struct blog_record *b, struct select *s) {
	struct appcontext *con = CONTEXT;
	essb *e = &con->templates;
	struct layer_context *l = &con->layer;

	int32_t tag = -e->record_size[s->iter];

	switch (tag) {
	case SITENAME_PAGE_PART:
		APP_WRITE(config.appname, config.appnamelen);
		break;
	case TITLE_PAGE_PART:
		APP_WRITE(b->title, b->titlelen);
		break;
	case CONTENT_PAGE_PART:
		APP_WRITE(b->datasource, b->datasourcelen);
		break;
	case REPEATTWO_PAGE_PART:
		if (s->position >= s->limit) break;

		memset(b, '\0', sizeof(struct blog_record));
		b->stack = con->freebuffer;
		b->stack_space = CONTEXTAPPBUFFERSIZE - (con->freebuffer - (char *) con);
		bool get_record_result = get_record(b, s->found[s->position], l, NULL);
		s->position++;
		if (get_record_result == false) {
			s->iter--;
			break;
		}

		rewind_back(e, REPEATONE_PAGE_PART, &(s->iter));
		break;
	default:
		return;
	}
}

static void selector(reqargs a, unsigned limit, unsigned offset, unix_epoch from, unix_epoch to, struct blog_record *b) {
	// above
	// select records by criteria on a single page

	struct appcontext *con = CONTEXT;
	essb *e = &con->templates;
	struct layer_context *l = &con->layer;

	struct select s = {.limit = limit,};
	s.found = alloca(sizeof(unsigned long) * limit);
	if (list_records(&limit, s.found, offset, from, to, l, NULL) == false) return notfound(a);

	for (; s.iter < e->records_amount; s.iter++) {
		if (e->record_size[s.iter] < 0) selector_show_tag_processing(a, b, &s);
		else APP_WRITE(&e->records[e->record_seek[s.iter]], e->record_size[s.iter]);
	}
}

static void title(reqargs a) {
	// show tittle page with pre-record values

	struct blog_record b = {
		.title = config.title_page_name,
		.titlelen = config.title_page_name_len,
		.datasource = config.title_page_content,
		.datasourcelen = config.title_page_content_len,
	};
	selector(a, 4, 0, (unix_epoch) 0l, (unix_epoch) 2147483647l, &b);
}

static inline void record_show_tag_processing(reqargs a, int32_t tag, struct blog_record b) {
	tag = -tag;

	switch (tag) {
		case SITENAME_PAGE_PART:
			APP_WRITE(config.appname, config.appnamelen);
			break;
		case TITLE_PAGE_PART:
			APP_WRITE(b.title, b.titlelen);
			break;
		case CONTENT_PAGE_PART:
			APP_WRITE(b.datasource, b.datasourcelen);
			break;
		default:
			return;
	}
}

static void show_record(reqargs a, uint32_t record) {
	if (record == UINT32_MAX) return notfound(a);

	struct appcontext *con = CONTEXT;
	essb *e = &con->templates;
	struct layer_context *l = &con->layer;

	struct blog_record b = {
		.stack = con->freebuffer,
		.stack_space = CONTEXTAPPBUFFERSIZE - (con->freebuffer - (char *) con)
	};
	if (get_record(&b, record, l, NULL) == false) {
		return notfound(a);
	}

	for (unsigned i = 0; i < e->records_amount; i++) {
		if (e->record_size[i] < 0) record_show_tag_processing(a, e->record_size[i], b);
		else APP_WRITE(&e->records[e->record_seek[i]], e->record_size[i]);
	}
}

void app_request(reqargs a) {
	if (METHOD != GET or REQUEST_LEN == 0 or REQUEST[0] != '/') return notfound(a);
	if (REQUEST_LEN == 1 and REQUEST[0] == '/') return title(a);
	return show_record(a, get_u32_from_end_of_string(REQUEST, REQUEST_LEN));
}

#endif // GUARD_APP_C
