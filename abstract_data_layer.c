#include <stdint.h>
#include <time.h>
#include <stdbool.h>

typedef union {
	time_t t;
	char spare[8];
} ttime_t;

struct blog_record {
	void *stack;
	size_t stack_space; // Pass here the amount of stack you have BEFORE executing get_record().
	                    // This variable will have the following values AFTER executing get_record():
	                    // 1. If not enough space on stack, place here the amount of bytes that is needed.
	                    // 2. If it's enough space, place here the used amount of bytes.
	unsigned recordnamelen;
	const char *recordname;
	unsigned recordcontentlen;
	const char *recordcontent;
	ttime_t unixepoch;   // seconds since unix epoch.
};

struct layer_context {
	uint32_t a;
	uint32_t b;
	uint32_t c;
	uint32_t d;
	uint32_t e;
	uint32_t f;
}; // 24 byte context is probably enough for any engine needs

const char data_layer_error_init[] = "Data layer haven't been initialized";
const char data_layer_error_havent_implemented[] = "Selected data layer engine still not implemented";
const char data_layer_error_wrong_engine[] = "Wrong engine selected";
const char data_layer_error_not_enough_stack_space[] = "Not enough stack space in choosen region";

bool list_records_dummy(unsigned *amount, unsigned long *result_list, unsigned offset, ttime_t from, ttime_t to, void *context, const char **error) {
	*error = data_layer_error_init;
	return false;
}

bool get_record_dummy(struct blog_record *r, unsigned choosen_record, void *context, const char **error) {
	*error = data_layer_error_init;
	return false;
}

bool (*list_records)(unsigned *, unsigned long *, unsigned, ttime_t, ttime_t, void *, const char **) = list_records_dummy;
bool (*get_record)(struct blog_record *, unsigned , void *, const char **) = get_record_dummy;

#ifdef DATA_LAYER_MYSQL
#include "abstract_data_layer_mysql.c"
#endif

#ifdef DATA_LAYER_FILENO
#include "abstract_data_layer_fileno.c"
#endif

enum datalayer_engines {
#ifdef DATA_LAYER_MYSQL
	ENGINE_MYSQL,
#endif
#ifdef DATA_LAYER_FILENO
	ENGINE_FILENO,
#endif
};

bool initialize_engine(enum datalayer_engines e, void *addr, void *context, const char **error) {
	switch (e) {
#ifdef DATA_LAYER_MYSQL
	case ENGINE_MYSQL:
		list_records = list_records_mysql;
		get_record = get_record_mysql;
		return initialize_mysql_context(addr, context, error);
#endif
#ifdef DATA_LAYER_FILENO
	case ENGINE_FILENO:
		list_records = list_records_fileno;
		get_record = get_record_fileno;
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
