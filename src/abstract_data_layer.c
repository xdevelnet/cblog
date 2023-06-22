#include <stdint.h>
#include <time.h>
#include <stdbool.h>

//#include "../../md4c/src/md4c.c"
//#include "../../md4c/src/md4c-html.c"
//#include "../../md4c/src/entity.c"

typedef union {
	time_t t;
	char spare[8];
} unix_epoch;

enum record_display {DISPLAY_INVALID, DISPLAY_SOURCE, DISPLAY_DATA, DISPLAY_BOTH};

struct blog_record {
	void *stack;
	size_t stack_space; // Pass here the amount of stack you have BEFORE executing get_record() or other functions.
	                    // This variable will have the following values AFTER executing such functions.
	                    // 1. If not enough space on stack, place here the amount of bytes that is needed.
	                    // 2. If it's enough space, place here the used amount of bytes.
	unsigned titlelen;
	const char *title;
	enum record_display display;
	unsigned datasourcelen;
	const char *datasource; // the common source of data for displaying. Usually html
	unsigned datalen;
	const char *data; // the second source of data. Sometimes could be displayed, but usually is used as predecessor for datasource
	unsigned long choosen_record; // decimal that represents record in database
	unix_epoch creation_date;
	unix_epoch modification_date;   // seconds since unix epoch.
	char **tags; // array with tags. Empty string means end of this array.
};

struct layer_context {
	uint32_t a;
	uint32_t b;
	uint32_t c;
	uint32_t d;
	uint32_t e;
	uint32_t f;
	uint32_t g;
	uint32_t h;
	uint32_t i;
}; // 40 byte context is probably enough for any engine needs

const char data_layer_error_init[] = "Data layer haven't been initialized";
const char data_layer_error_havent_implemented[] = "Selected data layer engine still not implemented";
const char data_layer_error_wrong_engine[] = "Wrong engine selected";
const char data_layer_error_not_enough_stack_space[] = "Not enough stack space in choosen region";
const char data_layer_error_metadata_corrupted[] = "Metadata corrupted. Storage engine can't be used.";
const char data_layer_error_invalid_argument[] = "Invalid argument.";
const char data_layer_error_invalid_argument_utf8[] = "One of the arguments contains invalid utf-8 string";
const char data_layer_error_record_already_exist[] = "You are attempting to insert a record that's already exist";
const char data_layer_error_unable_to_process_kval[] = "Operation with choosen key-value pair wasn't successful";

const char meta_display_source [] = "source";
const char meta_display_data   [] = "data";
const char meta_display_both   [] = "both";

const char *display_enum_to_str(enum record_display d) {
	if (d == DISPLAY_SOURCE) return meta_display_source;
	if (d == DISPLAY_DATA) return meta_display_data;
	if (d == DISPLAY_BOTH) return meta_display_both;
	return NULL;
}

enum record_display parse_meta_display(char *ptr, size_t len) {
	if (len == 0) return DISPLAY_INVALID;

	if (memcmp(ptr, meta_display_source, len) == 0) return DISPLAY_SOURCE;
	if (memcmp(ptr, meta_display_data,   len) == 0) return DISPLAY_DATA;
	if (memcmp(ptr, meta_display_both,   len) == 0) return DISPLAY_BOTH;
	return DISPLAY_INVALID;
}

bool list_records_dummy(unsigned *amount, unsigned long *result_list, unsigned offset, unix_epoch from, unix_epoch to, void *context, const char **error) {
	*error = data_layer_error_init;
	return false;
}

bool get_record_dummy(struct blog_record *r, unsigned choosen_record, void *context, const char **error) {
	UNUSED(r);
	UNUSED(choosen_record);
	UNUSED(context);

	*error = data_layer_error_init;
	return false;
}

bool insert_record_dummy(struct blog_record *r, void *context, const char **error) {
	UNUSED(r);
	UNUSED(context);

	*error = data_layer_error_init;
	return false;
}

bool key_val_dummy(const char *key, void *value, ssize_t *size, void *context, const char **error) {
	UNUSED(key);
	UNUSED(value);
	UNUSED(size);
	UNUSED(context);

	*error = data_layer_error_init;
	return false;
}

bool (*list_records)(unsigned *, unsigned long *, unsigned, unix_epoch, unix_epoch, void *, const char **) = list_records_dummy;
// attempt to fill a limited-size array integers which corresponds record's id
bool (*get_record)(struct blog_record *, unsigned , void *, const char **) = get_record_dummy;
// retrieve blog_record itself into empty structure. Non-empty structures are prohibited because of stack usage
bool (*insert_record)(struct blog_record *, void *, const char **) = insert_record_dummy;
// insert a blog_record
bool (*key_val)(const char *, void *, ssize_t *, void *, const char **) = key_val_dummy;
// simple key-value storage. if size equals zero - we're checking if pair exists.
// If size is greater then zero, we're attempting to insert a new record
// If size is less then zero, we're attempting to retrieve a record

#ifdef DATA_LAYER_MYSQL
#include "abstract_data_layer_mysql.c"
#endif

#ifdef DATA_LAYER_FILENO
#include "abstract_data_layer_fileno.c"
#endif

enum datalayer_engines {
	ENGINE_NULL,
#ifdef DATA_LAYER_MYSQL
	ENGINE_MYSQL,
#endif
#ifdef DATA_LAYER_FILENO
	ENGINE_FILENO,
#endif
};

const char *layer_engine_to_str(enum datalayer_engines e) {
#ifdef DATA_LAYER_MYSQL
	if (e == ENGINE_MYSQL) return "ENGINE_MYSQL";
#endif
#ifdef DATA_LAYER_FILENO
	if (e == ENGINE_FILENO) return "ENGINE_FILENO";
#endif
	return NULL;
}

enum datalayer_engines str_to_layer_engine(const char *str) {
	if (str == NULL) return ENGINE_NULL;
#ifdef DATA_LAYER_MYSQL
	if (strcmp(str, "ENGINE_MYSQL") == 0) return ENGINE_MYSQL;
#endif
#ifdef DATA_LAYER_FILENO
	if (strcmp(str, "ENGINE_FILENO") == 0) return ENGINE_FILENO;
#endif
	return ENGINE_NULL;
}

bool initialize_engine(enum datalayer_engines e, const void *addr, void *context, const char **error) {
	switch (e) {
#ifdef DATA_LAYER_MYSQL
	case ENGINE_MYSQL:
		list_records = list_records_mysql;
		get_record = get_record_mysql;
		insert_record = insert_record_mysql;
		key_val = key_val_mysql;
		return initialize_mysql_context(addr, context, error);
#endif
#ifdef DATA_LAYER_FILENO
	case ENGINE_FILENO:
		list_records = list_records_fileno;
		get_record = get_record_fileno;
		insert_record = insert_record_fileno;
		key_val = key_val_fileno;
		return initialize_fileno_context(addr, context, error);
#endif
	default:
		*error = data_layer_error_wrong_engine;
		return NULL;
	}
}

void deinitialize_engine(enum datalayer_engines e, void *context) {
	switch (e) {
#ifdef DATA_LAYER_MYSQL
	case ENGINE_MYSQL:
		break;
#endif
#ifdef DATA_LAYER_FILENO
	case ENGINE_FILENO:
		deinitialize_engine_fileno(context);
		break;
#endif
	default:
		break;
	}
}

#if !defined(DATA_LAYER_MYSQL) && !defined(DATA_LAYER_FILENO)
#error Please define at least one data layer engine!
#endif
