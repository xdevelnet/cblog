#ifndef GUARD_APP_C
#define GUARD_APP_C

#include "util.c"

#define DATA_LAYER_FILENO
#define DATA_LAYER_MYSQL
#include "abstract_data_layer.c"
#include "libessb.c"

#include "default_rodata.h"

typedef void (*rand_fill)(void *, size_t);
typedef void (*current_time)(struct unix_epoch_with_ms *);

#define CRED_HASHING_SALT_SIZE 16
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
	char salt[CRED_HASHING_SALT_SIZE];
	int32_t minimum_passwd_len;
	bool passwd_specialchars;

	rand_fill r;
	current_time t;
};

struct appcontext {
	essb templates;
	struct layer_context layer;
	struct appconfig *config;
	char freebuffer[];
};

typedef enum {POST, GET, PUT, PATCH, DELETE, UNKNOWN} http_methods;
typedef struct reqargs {
	const char *request;
	size_t request_len;
	const char *query;
	size_t query_len;
	http_methods method;
	void *appcontext; // struct appcontext
	void *servercontext1;
	void *servercontext2;
} reqargs;

const char *(*locate_header)(const char *, size_t *, void *);
void (*set_http_status_and_hdr) (unsigned short, const char * const *, void *);
void (*app_write) (const void *, unsigned long, void *);
void (*app_read) (void *, unsigned long *, void *);

#define REQUEST a.request
#define REQUEST_LEN a.request_len
#define QUERY a.query
#define QUERY_LEN a.query_len
#define METHOD a.method
#define CONTEXT a.appcontext
#define LOCATE_HEADER(arg1, arg2) locate_header(arg1, arg2, a.servercontext2)
#define SET_HTTP_STATUS_AND_HDR(arg1, arg2) set_http_status_and_hdr(arg1, arg2, a.servercontext1)
#define APP_WRITE(arg1, arg2) app_write(arg1, arg2, a.servercontext1)
#define APP_WRITECS(a) APP_WRITE(a, strizeof(a))
#define APP_READ(arg1, argv2) app_read(arg1, argv2, a.servercontext2)

const char default_header_content_type[] = "Content-Type: text/html;charset=utf-8";
const char default_header_server_type[] = "Server: cblog app operator";
const char default_header_location_slash[] = "Location: /";
const char default_header_location_user[] = "Location: /user";
const char default_header_location_page[] = "Location: /page";
const char default_header_nocache_1[] = "Cache-Control: no-cache, no-store, must-revalidate";
const char default_header_nocache_2[] = "Pragma: no-cache";
const char default_header_nocache_3[] = "Expires: 0";

#define LI_AND_A_TAGS_PREF "<li><a href=/tags?tag="
#define LI_A_SUFF "</a></li>"
#define LI_AND_A_PAGE_FULL_STR "<li><a href=/page>Add" LI_A_SUFF
#define LI_AND_A_USER "<li><a href=/user>"
#define LI_FULL_LOGIN_STR LI_AND_A_USER "Login" LI_A_SUFF
#define LI_AND_A_LOGOUT_FULL_STR "<li><a href=/logout>Logout" LI_A_SUFF

const char * const default_headers_table[] = {default_header_content_type, default_header_server_type, NULL};

#define CONTEXTAPPBUFFERSIZE 524288

// expected pages

enum pages {
	UNKNOWN_PAGE_PART   = 0,
	TITLE_PAGE_PART     = 1,
	SITENAME_PAGE_PART  = 2,
	FOOTER_PAGE_PART    = 3,
	CONTENT_PAGE_PART   = 4,
	REPEATONE_PAGE_PART = 5,
	REPEATTWO_PAGE_PART = 6,
	TAGS_PAGE_PART      = 7,
	USER_PAGE_PART      = 8,
	PAGES_MAX
};

http_methods http_determine_method(const char *ptr, size_t len) {
	switch (len) {
	case strizeof("GET"): // also case strizeof("PUT"):
		if (memcmp(ptr, "GET", strizeof("GET")) == STREQ) return GET;
		if (memcmp(ptr, "PUT", strizeof("PUT")) == STREQ) return PUT;
		return UNKNOWN;
	case strizeof("POST"):
		if (memcmp(ptr, "POST", strizeof("POST")) == STREQ) return POST;
		return UNKNOWN;
	case strizeof("DELETE"):
		if (memcmp(ptr, "DELETE", strizeof("DELETE")) == STREQ) return DELETE;
		return UNKNOWN;
	case strizeof("PATCH"):
		if (memcmp(ptr, "PATCH", strizeof("PATCH")) == STREQ) return PATCH;
		// fallthrough
	default:
		return UNKNOWN;
	}
}

void headers_table_append(const char **headers_table, const char *header) {
	while(*headers_table) {
		headers_table++;
	}

	*headers_table = header;
}

#define USER_PAGE_NAME "user"
#define TAGS_PAGE_NAME "tags"
#define TITLE_PAGE_NAME "title"
#define FOOTER_PAGE_NAME "footer"
#define CONTENT_PAGE_NAME "content"
#define SITE_NAME "sitename"
#define REPEATONE_PAGE_NAME "repeat_1"
#define REPEATTWO_PAGE_NAME "repeat_2"

static inline int32_t template_tag_to_number(const char *name, int32_t length) {
	switch (-length) {
	case strizeof(TAGS_PAGE_NAME): // also USER_PAGE_NAME
		if (memcmp(name, TAGS_PAGE_NAME, -length) == STREQ) return -TAGS_PAGE_PART;
		if (memcmp(name, USER_PAGE_NAME, -length) == STREQ) return -USER_PAGE_PART;
		break;
	case strizeof(TITLE_PAGE_NAME):
		if (memcmp(name, TITLE_PAGE_NAME, -length) == STREQ) return -TITLE_PAGE_PART;
		break;
	case strizeof(FOOTER_PAGE_NAME):
		if (memcmp(name, FOOTER_PAGE_NAME, -length) == STREQ) return -FOOTER_PAGE_PART;
		break;
	case strizeof(CONTENT_PAGE_NAME):
		if (memcmp(name, CONTENT_PAGE_NAME, -length) == STREQ) return -CONTENT_PAGE_PART;
		break;
	case strizeof(REPEATONE_PAGE_NAME):
		if (memcmp(name, SITE_NAME, -length) == 0) return -SITENAME_PAGE_PART;
		if (memcmp(name, REPEATONE_PAGE_NAME, -length) == STREQ) return -REPEATONE_PAGE_PART;
		if (memcmp(name, REPEATTWO_PAGE_NAME, -length) == STREQ) return -REPEATTWO_PAGE_PART;
		// fallthrough
	default:
		break;
	}

	return UNKNOWN_PAGE_PART;
}

bool app_prepare(void **ptr, struct appconfig *config) {
	srand((unsigned int) time(NULL));
	struct appcontext *con = *ptr;
	essb *e = &con->templates;
	struct layer_context *l = &con->layer;
	memset(e, '\0', sizeof(essb));
	parse_essb(e, config->template_type, config->temlate_name, NULL);
	if (e->errreasonstr != NULL) {
		snprintf(*ptr, CONTEXTAPPBUFFERSIZE, "Error during parsing essb: %s", e->errreasonstr);
		return false;
	}

	const char *error;
	struct data_layer d = {.e = config->datalayer_type, .addr = config->datalayer_addr, .context = l, .randfun = config->r};
	if (initialize_engine(&d, &error) == false) {
		snprintf(*ptr, CONTEXTAPPBUFFERSIZE, "Error during initializing_engine: %s", error);
		return false;
	}

	for (unsigned i = 0; i < e->records_amount; i++) {
		if (e->record_size[i] >= 0) continue;
		e->record_size[i] = template_tag_to_number(&e->records[e->record_seek[i]], e->record_size[i]);
	}

	con->config = config;

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

	char buffer[strizeof(CBL_STRINGIZE(UINT32_MAX))];
	long digitslen = string + len - locate;
	memcpy(buffer, locate, digitslen);
	buffer[digitslen] = '\0';

	return strtoul(buffer, NULL, 10);
}

size_t find_cookie_existence(reqargs a, const char *cookie_key, char cookie_value_buffer[KEY_VAL_MAXKEYLEN]) {
	size_t cookiehdrlen = 0;
	const char *cookie_header = LOCATE_HEADER("Cookie", &cookiehdrlen);
	if (cookie_header == NULL) return 0;

	size_t cookie_val_len;
	char *cookie_val = http_query_finder(cookie_key, cookie_header, cookiehdrlen, &cookie_val_len, true);
	if (cookie_val == NULL) return 0;
	if (cookie_val_len >= KEY_VAL_MAXKEYLEN) cookie_val_len = KEY_VAL_MAXKEYLEN - 1;
	memcpy(cookie_value_buffer, cookie_val, cookie_val_len);
	cookie_value_buffer[cookie_val_len] = '\0';
	return cookie_val_len;
}

bool is_user_valid(struct usr *u) {
	if (u->display_name[0] == '\0' or u->email[0] == '\0' or u->create_time.t == 0 or u->id == 0 or u->status != ACTIVE) return false;
	return true;
}

bool is_user_legit(struct layer_context *l, struct usr *u) {
	if (is_user_valid(u) == false) return false;

	struct usr u2 = {.id = u->id};
	struct user_action action = {.filter = BY_ID, .operation = SELECT};
	bool ret = user(&u2, action, l, NULL);
	if (ret == false) return false;
	if (memcmp(u->display_name, u2.display_name, sizeof(u2.display_name)) != STREQ or memcmp(u->email, u2.email, sizeof(u2.email)) != STREQ) return false;
	return true;
}

#define BUF_USERDISPLAY_CALC (strizeof(LI_AND_A_PAGE_FULL_STR)+sizeof(u->display_name)+strizeof(LI_AND_A_LOGOUT_FULL_STR)+strizeof(LI_AND_A_USER)+strizeof(LI_A_SUFF)+sizeof(char))

static void internal_server_error(reqargs a, const char *error) {
	struct appcontext *con = CONTEXT;
	essb *e = &con->templates;
	struct layer_context *l = &con->layer;
	struct appconfig *config = con->config;

	SET_HTTP_STATUS_AND_HDR(500, default_headers_table);

	const char *out[PAGES_MAX] = {
		[SITENAME_PAGE_PART] = config->appname,
		[TITLE_PAGE_PART]    = "500 INTERNAL SERVER ERROR",
		[CONTENT_PAGE_PART]  = error,
	};
	size_t outsizes[PAGES_MAX] = {
		[SITENAME_PAGE_PART] = config->appnamelen,
		[TITLE_PAGE_PART]    = strizeof("500 INTERNAL SERVER ERROR"),
		[CONTENT_PAGE_PART]  = strlen(error),
	};

	struct usr u[1];
	char buffer[CBL_MAX(KEY_VAL_MAXKEYLEN, BUF_USERDISPLAY_CALC)];
	ssize_t size = - ((ssize_t) sizeof(struct usr));
	if (find_cookie_existence(a, "id", buffer) != 0 and key_val(buffer, u, &size, l, NULL) == true and is_user_valid(u) == true) {
		size_t strsize = (size_t) sprintf(buffer, LI_AND_A_PAGE_FULL_STR LI_AND_A_USER "%s" LI_A_SUFF LI_AND_A_LOGOUT_FULL_STR, u->display_name);
		out[USER_PAGE_PART] = buffer;
		outsizes[USER_PAGE_PART] = strsize;
	}

	for (unsigned i = 0; i < e->records_amount; i++) {
		if (e->record_size[i] < 0) APP_WRITE(out[-e->record_size[i]], outsizes[-e->record_size[i]]);
		else APP_WRITE(&e->records[e->record_seek[i]], e->record_size[i]);
	}
}

static void notfound(reqargs a) {
	struct appcontext *con = CONTEXT;
	essb *e = &con->templates;
	struct layer_context *l = &con->layer;
	struct appconfig *config = con->config;

	SET_HTTP_STATUS_AND_HDR(404, default_headers_table);

	const char *out[PAGES_MAX] = {
		[SITENAME_PAGE_PART] = config->appname,
		[TITLE_PAGE_PART]    = "404 NOT FOUND",
		[CONTENT_PAGE_PART]  = "Sorry but we can't find content that you are looking for :(",
	};
	size_t outsizes[PAGES_MAX] = {
		[SITENAME_PAGE_PART] = config->appnamelen,
		[TITLE_PAGE_PART]    = strizeof("404 NOT FOUND"),
		[CONTENT_PAGE_PART]  = strizeof("Sorry but we can't find content that you are looking for :("),
	};

	struct usr u[1];
	char buffer[CBL_MAX(KEY_VAL_MAXKEYLEN, BUF_USERDISPLAY_CALC)];
	ssize_t size = - ((ssize_t) sizeof(struct usr));
	if (find_cookie_existence(a, "id", buffer) != 0 and key_val(buffer, u, &size, l, NULL) == true and is_user_valid(u) == true) {
		size_t strsize = (size_t) sprintf(buffer, LI_AND_A_PAGE_FULL_STR LI_AND_A_USER "%s" LI_A_SUFF LI_AND_A_LOGOUT_FULL_STR, u->display_name);
		out[USER_PAGE_PART] = buffer;
		outsizes[USER_PAGE_PART] = strsize;
	}

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
	bool href;
	bool end_at_vline;
};

static inline void selector_show_tag_processing(reqargs a, struct blog_record *b, struct select *s, struct usr *u) {
	struct appcontext *con = CONTEXT;
	essb *e = &con->templates;
	struct layer_context *l = &con->layer;
	struct appconfig *config = con->config;

	int32_t tag = -e->record_size[s->iter];

	switch (tag) {
	case SITENAME_PAGE_PART:
		APP_WRITE(config->appname, config->appnamelen);
		break;
	case TITLE_PAGE_PART:
		if (s->href) {
			APP_WRITE("<a href=\"", strizeof("<a href=\""));
			APP_WRITE(b->title, b->titlelen);
			APP_WRITE("-", strizeof("-"));
			char buffer[CBL_UINT32_STR_MAX];
			sprintf(buffer, "%lu", b->chosen_record);
			APP_WRITE(buffer, strlen(buffer));
			APP_WRITE("\">", strizeof("\">"));
		}
		APP_WRITE(b->title, b->titlelen);
		if (s->href) {
			APP_WRITE("</a>", strizeof("</a>"));
			s->href = false;
		}
		break;
	case USER_PAGE_PART:
		if (u == NULL) {
			APP_WRITE(LI_FULL_LOGIN_STR, strizeof(LI_FULL_LOGIN_STR));
			break;
		}
		APP_WRITECS(LI_AND_A_PAGE_FULL_STR);
		APP_WRITE(LI_AND_A_USER, strizeof(LI_AND_A_USER));
		APP_WRITE(u->display_name, strlen(u->display_name));
		APP_WRITE(LI_A_SUFF, strizeof(LI_A_SUFF));
		APP_WRITE(LI_AND_A_LOGOUT_FULL_STR, strizeof(LI_AND_A_LOGOUT_FULL_STR));
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
		} else {
			s->href = true;
			const char *target = b->datasource;
#define VLINE_HTMLTAG "<hr>"
			char *found = util_memmem(target, b->datasourcelen, VLINE_HTMLTAG, strizeof(VLINE_HTMLTAG));
			if (found) {
				if (s->end_at_vline == true) {
					b->datasourcelen -= b->datasourcelen - (found - target);
				} else {
					memset(found, ' ', strizeof(VLINE_HTMLTAG));
				}
			}
		}

		rewind_back(e, REPEATONE_PAGE_PART, &(s->iter));
		break;
	default:
		return;
	}
}

static void selector(reqargs a, unsigned limit, unsigned offset, struct list_filter filter, struct blog_record *b, bool end_at_vline) {
	// above
	// select records by criteria on a single page

	struct appcontext *con = CONTEXT;
	essb *e = &con->templates;
	struct layer_context *l = &con->layer;

	struct select s = {.limit = limit, .end_at_vline = end_at_vline};
	s.found = alloca(sizeof(unsigned long) * limit);
	memset(s.found, 0, sizeof(unsigned long) * limit); // TODO: for some reason valgrind yells and unitialized values when tag filter is used, this memset should be absent
	if (list_records(&limit, s.found, offset, filter, l, NULL) == false) return notfound(a);
	SET_HTTP_STATUS_AND_HDR(200, default_headers_table);

	char key[KEY_VAL_MAXKEYLEN];
	struct usr u_anon[1];
	struct usr *u = NULL;
	ssize_t usize = - ((ssize_t) sizeof(struct usr));
	if (find_cookie_existence(a, "id", key) != 0 and key_val(key, u_anon, &usize, l, NULL) == true and is_user_valid(u_anon) == true) u = u_anon;

	for (; s.iter < e->records_amount; s.iter++) {
		if (e->record_size[s.iter] < 0) {
			selector_show_tag_processing(a, b, &s, u);
		} else {
			char *target = &e->records[e->record_seek[s.iter]];
			size_t size = e->record_size[s.iter];

			APP_WRITE(target, size);
		}
	}
}

static void title(reqargs a) {
	// show tittle page with pre-record values
	struct appcontext *con = CONTEXT;
//	essb *e = &con->templates;
//	struct layer_context *l = &con->layer;
	struct appconfig *config = con->config;

	struct blog_record b = {
		.title = config->title_page_name,
		.titlelen = config->title_page_name_len,
		.datasource = config->title_page_content,
		.datasourcelen = config->title_page_content_len,
	};
	struct list_filter filter = {.from.t = 0l, .to.t = 2147483647l, .sort = DESC}; // (unix_epoch) 0l, (unix_epoch) 2147483647l
	const unsigned offset = 0;
	selector(a, HOW_MANY_RECORDS_U_WANT_TO_SEE_ON_TITLEPAGE, offset, filter, &b, true);
}

static void show_with_tags(reqargs a) {
	// show records with specific tag

	size_t size = 0;
	char *find = http_query_finder("tag", QUERY, QUERY_LEN, &size, false);
	if (find == NULL or size == 0) return notfound(a);
	char tag[size + sizeof(char)];
	memcpy(tag, find, size);
	tag[size] = '\0';
	char *tags[2] = {tag, NULL};

	struct blog_record b = {
		.title = find,
		.titlelen = size,
		.datasource = default_show_tags_content,
		.datasourcelen = default_show_tag_content_len,
	};
	// some day, later, this page will be just a "search" page by different filters, including "tag"
	struct list_filter filter = {.from.t = 0l, .to.t = 2147483647l, .tags = tags};
	selector(a, 4, 0, filter, &b, true);
}

static inline void record_show_tag_processing(reqargs a, int32_t tag, struct blog_record b, struct usr *u) {
	struct appcontext *con = CONTEXT;
//	essb *e = &con->templates;
//	struct layer_context *l = &con->layer;
	struct appconfig *config = con->config;

	tag = -tag;

	switch (tag) {
	case SITENAME_PAGE_PART:
		APP_WRITE(config->appname, config->appnamelen);
		break;
	case TITLE_PAGE_PART:
		APP_WRITE(b.title, b.titlelen);
		break;
	case CONTENT_PAGE_PART:
	{
		const char *target = b.datasource;
		size_t size = b.datasourcelen;
		char *found = util_memmem(target, size, VLINE_HTMLTAG, strizeof(VLINE_HTMLTAG));
		if (found) {
			memset(found, ' ', strizeof(VLINE_HTMLTAG));
		}
		APP_WRITE(b.datasource, b.datasourcelen);
	}
		break;
	case TAGS_PAGE_PART:
	{
		char **tags = b.tags;
		if (tags == NULL) break;
		while(*tags) {
			size_t taglen = strlen(*tags);
			APP_WRITE(LI_AND_A_TAGS_PREF, strizeof(LI_AND_A_TAGS_PREF));
			APP_WRITE(*tags, taglen);
			APP_WRITE(">", strizeof(">"));
			APP_WRITE(*tags, taglen);
			APP_WRITE(LI_A_SUFF, strizeof(LI_A_SUFF));
			tags++;
		}
	}
		break;
	case USER_PAGE_PART:
	{
		if (u == NULL) {
			APP_WRITE(LI_FULL_LOGIN_STR, strizeof(LI_FULL_LOGIN_STR));
			break;
		}
		APP_WRITECS(LI_AND_A_PAGE_FULL_STR);
		APP_WRITE(LI_AND_A_USER, strizeof(LI_AND_A_USER));
		APP_WRITE(u->display_name, strlen(u->display_name));
		APP_WRITE(LI_A_SUFF, strizeof(LI_A_SUFF));
		APP_WRITE(LI_AND_A_LOGOUT_FULL_STR, strizeof(LI_AND_A_LOGOUT_FULL_STR));
	}
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

	char key[KEY_VAL_MAXKEYLEN];
	struct usr u_anon[1];
	struct usr *u = NULL;
	ssize_t size = - ((ssize_t) sizeof(struct usr));
	if (find_cookie_existence(a, "id", key) != 0 and key_val(key, u_anon, &size, l, NULL) == true and is_user_valid(u_anon) == true) u = u_anon;

	for (unsigned i = 0; i < e->records_amount; i++) {
		if (e->record_size[i] < 0) record_show_tag_processing(a, e->record_size[i], b, u);
		else APP_WRITE(&e->records[e->record_seek[i]], e->record_size[i]);
	}
}

static bool minimum_passwd_requirements(char *password, size_t passwd_minlen, bool passwd_specialchar) {
	if (utf8_check(password, strlen(password)) != NULL) return false;
	size_t passwd_len = 0;
	bool passwd_specialchar_presence = false;

	while(*password != '\0') {
		unsigned char width = utf8_byte_width(password);
		if (width == sizeof(char)) {
			if (is_special_character(*password)) passwd_specialchar_presence = true;
		}
		password += width;
		passwd_len++;
	}

	if (passwd_len < passwd_minlen) return false;
	if (passwd_specialchar == true and passwd_specialchar_presence == false) return false;

	return true;
}

bool newuser(reqargs a, char *display_name, char *email, char *password) {
	struct appcontext *con = CONTEXT;
//	essb *e = &con->templates;
	struct layer_context *l = &con->layer;
	struct appconfig *config = con->config;

	struct usr u = {.status = UNCONFIRMED};

	size_t name_len = strlen(display_name);
	size_t password_len = strlen(password);

	if (name_len >= sizeof(u.display_name)) return false;
	if (validate_email_wo_regex(email) == false) return false;
	if (minimum_passwd_requirements(password, config->minimum_passwd_len, config->passwd_specialchars) == false) return false;

	memcpy(u.display_name, display_name, name_len); u.display_name[name_len] = '\0';


	SHA256_CTX ctx;
	sha256_init(&ctx);
	sha256_update(&ctx, (BYTE *) password, password_len);
	sha256_final(&ctx, (BYTE *) u.credentials);

	struct user_action action = {.operation = ADD};
	user_fileno(&u, action, l, NULL);

	return true;
}

static inline void user_panel_processing(reqargs a, int32_t tag, struct usr *u) {
	struct appcontext *con = CONTEXT;
//	essb *e = &con->templates;
//	struct layer_context *l = &con->layer;
	struct appconfig *config = con->config;

	tag = -tag;

	switch (tag) {
	case SITENAME_PAGE_PART:
		APP_WRITE(config->appname, config->appnamelen);
		break;
	case TITLE_PAGE_PART:
		APP_WRITECS("User panel");
		break;
	case CONTENT_PAGE_PART:
		APP_WRITECS("OH MY! HEELOU, ");
		APP_WRITE(u->display_name, strlen(u->display_name));
		break;
	case USER_PAGE_PART:
		APP_WRITECS(LI_AND_A_PAGE_FULL_STR);
		APP_WRITE(LI_AND_A_USER, strizeof(LI_AND_A_USER));
		APP_WRITE(u->display_name, strlen(u->display_name));
		APP_WRITE(LI_A_SUFF, strizeof(LI_A_SUFF));
		APP_WRITE(LI_AND_A_LOGOUT_FULL_STR, strizeof(LI_AND_A_LOGOUT_FULL_STR));
		break;
	default:
		return;
	}
}

void user_panel(reqargs a, struct usr *u) {
	struct appcontext *con = CONTEXT;
	essb *e = &con->templates;
//	struct layer_context *l = &con->layer;

	const char *headers_table[] = {default_header_nocache_1, default_header_nocache_2, default_header_nocache_3,
                                   default_header_content_type, default_header_server_type, NULL};
	SET_HTTP_STATUS_AND_HDR(200, headers_table);

	for (unsigned i = 0; i < e->records_amount; i++) {
		if (e->record_size[i] < 0) user_panel_processing(a, e->record_size[i], u);
		else APP_WRITE(&e->records[e->record_seek[i]], e->record_size[i]);
	}
}

bool check_user_password(const void *password, size_t password_len, char password_hashed[SHA256_BLOCK_SIZE]) {
	SHA256_CTX ctx;
	sha256_init(&ctx);
	sha256_update(&ctx, password, password_len);
	BYTE buf[SHA256_BLOCK_SIZE];
	sha256_final(&ctx, buf);
	if (memcmp(buf, password_hashed, SHA256_BLOCK_SIZE) == STREQ) return true;

	return false;
}

#define SETCOOKIEID "Set-Cookie: id="
#define SESSION_KEY "session_"
#define SMCOL_EXPIRES "; expires=Thu, 01 Jan 1970 00:00:00 GMT"

void user_login(reqargs a) {
	struct appcontext *con = CONTEXT;
	essb *e = &con->templates;
	struct layer_context *l = &con->layer;
	struct appconfig *config = con->config;

	const char *error;

	const char *out[PAGES_MAX] = {
		[SITENAME_PAGE_PART] = config->appname,
		[TITLE_PAGE_PART]    = "Login",
		[CONTENT_PAGE_PART]  = default_form_html,
	};

	size_t outsizes[PAGES_MAX] = {
		[SITENAME_PAGE_PART] = config->appnamelen,
		[TITLE_PAGE_PART]    = strizeof("Login"),
		[CONTENT_PAGE_PART]  = default_form_html_len,
	};

	const char *headers_table[] = {default_header_nocache_1, default_header_nocache_2, default_header_nocache_3,
                                   default_header_content_type, default_header_server_type, NULL, NULL, NULL};

	char cookie[sizeof(SETCOOKIEID SMCOL_EXPIRES) + KEY_VAL_MAXKEYLEN];

	bool user_logged_in = false;
	struct usr logged_in_user;
	do{
		char key[KEY_VAL_MAXKEYLEN];
		if (find_cookie_existence(a, "id", key) == 0) break;

		ssize_t size = - ((ssize_t) sizeof(logged_in_user));
		bool ret = key_val(key, &logged_in_user, &size, l, &error);
		if (ret == false or is_user_legit(l, &logged_in_user) == false) {
			sprintf(cookie, "Set-Cookie: id=%s%s", key, SMCOL_EXPIRES);
			headers_table_append(headers_table, cookie);
			headers_table_append(headers_table, default_header_location_slash);
			SET_HTTP_STATUS_AND_HDR(302, headers_table);
			APP_WRITECS("Redirecting: /");
			return;
		}

		user_logged_in = true;
	} while(0);

	if (METHOD == GET) {
		if (user_logged_in) return user_panel(a, &logged_in_user);

		SET_HTTP_STATUS_AND_HDR(200, headers_table);

		for (unsigned i = 0; i < e->records_amount; i++) {
			if (e->record_size[i] < 0) APP_WRITE(out[-e->record_size[i]], outsizes[-e->record_size[i]]);
			else APP_WRITE(&e->records[e->record_seek[i]], e->record_size[i]);
		}

		return;
	}

	if (user_logged_in) {
		headers_table_append(headers_table, default_header_location_slash);
		SET_HTTP_STATUS_AND_HDR(302, headers_table);
		APP_WRITECS("Redirecting: /");

		return;
	}

	size_t freespace = CONTEXTAPPBUFFERSIZE - (con->freebuffer - (char *) con);
	size_t size = freespace;
	char *post_data = con->freebuffer;
	char *nextbuffer = NULL;
	APP_READ(post_data, &size);
	if (size > 0) do {
		post_data[size] = '\0';
		size = urldecode2(post_data, post_data) - post_data;
		nextbuffer = post_data + size;
		struct usr *u = (void *) nextbuffer;
		freespace -= size;
		if (freespace < sizeof(struct usr)) return internal_server_error(a, data_layer_error_not_enough_stack_space);
		size_t namelen;
		char *name = http_query_finder("name", post_data, size, &namelen, false);
		if (name == NULL or namelen >= sizeof(u->display_name)) {
			out[TITLE_PAGE_PART] = data_layer_error_invalid_argument;
			outsizes[TITLE_PAGE_PART] = strizeof(data_layer_error_invalid_argument);
			break;
		}
		size_t passwordlen;
		char *password = http_query_finder("password", post_data, size, &passwordlen, false);
		if (password == NULL) {
			out[TITLE_PAGE_PART] = data_layer_error_invalid_argument;
			outsizes[TITLE_PAGE_PART] = strizeof(data_layer_error_invalid_argument);
			break;
		}
		name[namelen] = '\0';
		password[passwordlen] = '\0';
		memcpy(u->display_name, name, namelen);
		u->display_name[namelen] = '\0';
		struct user_action action = {.operation = SELECT, .filter = BY_NAME};
		char key[KEY_VAL_MAXKEYLEN] = "\0"SESSION_KEY;
		ssize_t keyval_size = sizeof(struct usr);
		if (user(u, action, l, NULL) == false or check_user_password(password, passwordlen, u->credentials) == false or key_val(key, u, &keyval_size, l, NULL) == false) {
			out[TITLE_PAGE_PART] = data_layer_error_invalid_argument;
			outsizes[TITLE_PAGE_PART] = strizeof(data_layer_error_invalid_argument);
			break;
		}

		sprintf(cookie, "Set-Cookie: id=%s", key);
		headers_table_append(headers_table, cookie);
		out[TITLE_PAGE_PART] = default_welcome_after_login_title;
		outsizes[TITLE_PAGE_PART] = strizeof(default_welcome_after_login_title);
		out[CONTENT_PAGE_PART] = default_welcome_after_login;
		outsizes[CONTENT_PAGE_PART] = strizeof(default_welcome_after_login);
	} while(0);

	SET_HTTP_STATUS_AND_HDR(200, headers_table);

	for (unsigned i = 0; i < e->records_amount; i++) {
		if (e->record_size[i] < 0) APP_WRITE(out[-e->record_size[i]], outsizes[-e->record_size[i]]);
		else APP_WRITE(&e->records[e->record_seek[i]], e->record_size[i]);
	}
}

void user_logout(reqargs a) {
	struct appcontext *con = CONTEXT;
//	essb *e = &con->templates;
	struct layer_context *l = &con->layer;
//	struct appconfig *config = con->config;

	const char *logout_headers_table[] = {default_header_nocache_1, default_header_nocache_2, default_header_nocache_3,
                                          default_header_server_type, default_header_location_slash, NULL, NULL};

	char cookie[sizeof(SETCOOKIEID SMCOL_EXPIRES) + KEY_VAL_MAXKEYLEN];
	char key[KEY_VAL_MAXKEYLEN];
	size_t keylen = find_cookie_existence(a, "id", key);
	if (keylen != 0) {
		key_val(key, NULL, NULL, l, NULL);
		sprintf(cookie, "Set-Cookie: id=%s%s", key, SMCOL_EXPIRES);
		headers_table_append(logout_headers_table, cookie);
	}

	SET_HTTP_STATUS_AND_HDR(302, logout_headers_table);
	APP_WRITECS("Redirecting: /");
}

static inline void editor_processing(reqargs a, int32_t tag, struct usr *u, char *error, size_t errorlen) {
	struct appcontext *con = CONTEXT;
//	essb *e = &con->templates;
//	struct layer_context *l = &con->layer;
	struct appconfig *config = con->config;

	tag = -tag;

	switch (tag) {
	case SITENAME_PAGE_PART:
		APP_WRITE(config->appname, config->appnamelen);
		break;
	case TITLE_PAGE_PART:
		APP_WRITECS("Add/edit page/record");
		break;
	case CONTENT_PAGE_PART:
		if (error != NULL and errorlen > 0) {
			APP_WRITECS("Error: ");
			APP_WRITE(error, errorlen);
			APP_WRITECS("<br><br>");
		}
		APP_WRITE(default_add_edit_form_html, strizeof(default_add_edit_form_html));
		break;
	case USER_PAGE_PART:
		APP_WRITECS(LI_AND_A_PAGE_FULL_STR);
		APP_WRITE(LI_AND_A_USER, strizeof(LI_AND_A_USER));
		APP_WRITE(u->display_name, strlen(u->display_name));
		APP_WRITE(LI_A_SUFF, strizeof(LI_A_SUFF));
		APP_WRITE(LI_AND_A_LOGOUT_FULL_STR, strizeof(LI_AND_A_LOGOUT_FULL_STR));
		break;
	default:
		return;
	}
}

void page(reqargs a) {
	struct appcontext *con = CONTEXT;
	essb *e = &con->templates;
	struct layer_context *l = &con->layer;
//	struct appconfig *config = con->config;

	const char *headers_table[] = {default_header_nocache_1, default_header_nocache_2, default_header_nocache_3,
                                   default_header_content_type, default_header_server_type, NULL, NULL, NULL};

	char cookie[sizeof(SETCOOKIEID SMCOL_EXPIRES) + KEY_VAL_MAXKEYLEN];

	bool user_logged_in = false;
	struct usr logged_in_user;
	do{
		char key[KEY_VAL_MAXKEYLEN];
		if (find_cookie_existence(a, "id", key) == 0) break;

		ssize_t size = - ((ssize_t) sizeof(logged_in_user));
		bool ret = key_val(key, &logged_in_user, &size, l, NULL);
		if (ret == false or is_user_legit(l, &logged_in_user) == false) {
			sprintf(cookie, "Set-Cookie: id=%s%s", key, SMCOL_EXPIRES);
			headers_table_append(headers_table, cookie);
			headers_table_append(headers_table, default_header_location_user);
			SET_HTTP_STATUS_AND_HDR(302, headers_table);
			APP_WRITECS("Redirecting: /user");
			return;
		}

		user_logged_in = true;
	} while(0);

	if (user_logged_in == false) {
		headers_table_append(headers_table, default_header_location_user);
		SET_HTTP_STATUS_AND_HDR(302, headers_table);
		APP_WRITECS("Redirecting: /user");
		return;
	}

	size_t freespace = CONTEXTAPPBUFFERSIZE - (con->freebuffer - (char *) con);
	char *input_data = con->freebuffer;

	if (METHOD == GET) {
		SET_HTTP_STATUS_AND_HDR(200, headers_table);
		size_t size = 0;
		char *find = http_query_finder("error", QUERY, QUERY_LEN, &size, false);
		memcpy(input_data, find, size);
		input_data[size] = '\0';
		size = urldecode2(input_data, input_data) - input_data;

		for (unsigned i = 0; i < e->records_amount; i++) {
			if (e->record_size[i] < 0) editor_processing(a, e->record_size[i], &logged_in_user, input_data, size);
			else APP_WRITE(&e->records[e->record_seek[i]], e->record_size[i]);
		}

		return;
	}

	size_t size = freespace;
	APP_READ(input_data, &size);

	if (size == 0) {
		headers_table_append(headers_table, default_header_location_page);
		SET_HTTP_STATUS_AND_HDR(302, headers_table);
		APP_WRITECS("Redirecting: /page");
		return;
	}

	input_data[size] = '\0';
	size = urldecode2(input_data, input_data) - input_data;
	input_data[size] = '\0';

	size_t titlelen;
	char *title = http_query_finder("title", input_data, size, &titlelen, false);

	size_t datalen;
	char *data = http_query_finder("data", input_data, size, &datalen, false);

	if (title == NULL or data == NULL) {
		headers_table_append(headers_table, default_header_location_page);
		SET_HTTP_STATUS_AND_HDR(302, headers_table);
		APP_WRITECS("Redirecting: /page");
		return;
	}

	struct blog_record b = {.title = title, .titlelen = titlelen, .data = data, .datalen = datalen, .display = DISPLAY_DATASOURCE};
	const char *error;
	bool result = insert_record(&b, l, &error);
	char strhdr[200];
	if (result == false) {
		printf("error: %s\n", error);
		snprintf(strhdr, sizeof(strhdr), "Location: /page?error=%s", error);

		headers_table_append(headers_table, strhdr);
		SET_HTTP_STATUS_AND_HDR(302, headers_table);
		APP_WRITECS("Redirecting: /page");
		return;
	}

	snprintf(strhdr, sizeof(strhdr), "Location: /newpage-%lu", b.chosen_record);
	headers_table_append(headers_table, strhdr);
	SET_HTTP_STATUS_AND_HDR(302, headers_table);
	APP_WRITECS("Redirecting: /newpage...");
}

//#define IFREQ(page, fun) do{if(REQUEST_LEN==strizeof(page) and memcmp(REQUEST, page, strizeof(page)) == STREQ) return fun(a);}while(0)

void app_request(reqargs a) {
	if ((METHOD != GET and METHOD != POST) or REQUEST_LEN == 0 or REQUEST[0] != '/') return notfound(a);
	if (REQUEST_LEN == 1 and REQUEST[0] == '/') return title(a);
	if (REQUEST_LEN == strizeof("/tags") and memcmp(REQUEST, "/tags", strizeof("/tags")) == STREQ) return show_with_tags(a);
	if (REQUEST_LEN == strizeof("/user") and memcmp(REQUEST, "/user", strizeof("/user")) == STREQ) return user_login(a);
	if (REQUEST_LEN == strizeof("/page") and memcmp(REQUEST, "/page", strizeof("/page")) == STREQ) return page(a);
	if (REQUEST_LEN == strizeof("/logout") and memcmp(REQUEST, "/logout", strizeof("/logout")) == STREQ) return user_logout(a);
	return show_record(a, get_u32_from_end_of_string(REQUEST, REQUEST_LEN));
}

#endif // GUARD_APP_C
