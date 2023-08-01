#include <stdint.h>
#include <time.h>
#include <stdbool.h>

#include "../../md4c/src/md4c.c"
#include "../../md4c/src/md4c-html.c"
#include "../../md4c/src/entity.c"
#include "external/sha256.c"

typedef uint32_t acl_mode;

#define ACL_USR_READ 0400
#define ACL_USR_WRITE 0200
#define ACL_USR_LIST 0100

#define ACL_GROUP_READ (ACL_USR_READ >> 3)
#define ACL_GROUP_WRITE (ACL_USR_WRITE >> 3)
#define ACL_GROUP_LIST (ACL_USR_LIST >> 3)

#define ACL_ALL_READ (ACL_GROUP_READ >> 3)
#define ACL_ALL_WRITE (ACL_GROUP_WRITE >> 3)
#define ACL_ALL_LIST (ACL_GROUP_LIST >> 3)

#define ACL_DEFAULT_NEW_OBJECT_MODE (ACL_USR_LIST | ACL_USR_READ | ACL_USR_WRITE | ACL_GROUP_LIST | ACL_GROUP_READ | ACL_ALL_LIST | ACL_ALL_READ)

#define MAXGROUPS 112

// ACL - access control list
// GB - group-based
// GBACL - group-based access control list. It's like RBAC and ACL

struct object_access_list { // this structure should be used for making exclusive access for user to object
	uint32_t object;
	acl_mode access;
};

struct user_gbacl { // this structure represents an actual list of user's group participation and exclusive access
	uint32_t user_participate_groups_amount; // maximum is MAXGROUPS
	uint32_t group[MAXGROUPS];
	struct object_access_list acl[];
};

struct object_gbac {
	uint32_t user; // id of user who owns/makes this object
	uint32_t group; // id of group which object is belongs to
	acl_mode mode; // mode, from 000 to 777
};

typedef union {
	time_t t;
	char spare[8];
} unix_epoch;

struct unix_epoch_with_ms { // just like unix_epoch, but when we need more accuracy
	unix_epoch epoch;
	uint32_t milliseconds;
};

enum record_display {DISPLAY_INVALID = 0, DISPLAY_DATASOURCE, DISPLAY_DATA, DISPLAY_BOTH};

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
	unsigned long chosen_record; // decimal that represents record in database. 0 if we need to create it, non-0 if we need to edit it
	unix_epoch creation_date;
	unix_epoch modification_date;   // seconds since unix epoch.
	struct object_gbac rights;
	char **tags; // array with tags. Empty string means end of this array.
};

struct layer_context {
	uint32_t enough_space[14];
}; // 14*4 byte context is probably enough for any engine needs

enum sorting_seq {DESC = 0, ASC};

struct list_filter {
	unix_epoch from; // unixtime
	unix_epoch to;
	enum sorting_seq sort;
	char **tags;
};

enum user_status {UNCONFIRMED, ACTIVE, RESERVED, DEACTIVATED};

union expiration { // depending on status, different expiration should be used
	unix_epoch approve_code_expiration; // UNCONFIRMED
	unix_epoch login_ban_expiration; // ACTIVE
	unix_epoch deactivated_expiration; // DEACTIVATED
};

struct usr {
	uint32_t id;
	char display_name[64]; // enough for most of UTF-8 names
	char email[EMAIL_MAXLEN + sizeof(char)];
	char credentials[SHA256_BLOCK_SIZE];
	enum user_status status;
	unix_epoch create_time;
	char approve_code[8];
	union expiration expiration;
};

enum user_operation {CHECK, SELECT, ALTER, ADD, REMOVE};
enum user_filter {BY_ID, BY_NAME, BY_EMAIL};

struct user_action {
	enum user_operation operation;
	enum user_filter filter;
};

const char data_layer_error_init[] = "Data layer haven't been initialized";
const char data_layer_error_havent_implemented[] = "Selected data layer engine still not implemented";
const char data_layer_error_wrong_engine[] = "Wrong engine selected";
const char data_layer_error_not_enough_stack_space[] = "Not enough stack space in choosen region";
const char data_layer_error_metadata_corrupted[] = "Metadata corrupted. Storage engine can't be used.";
const char data_layer_error_invalid_argument[] = "Invalid argument.";
const char data_layer_error_invalid_argument_utf8[] = "One of the arguments contains invalid utf-8 string";
const char data_layer_error_data_already_exist[] = "You are attempting to insert a data that's already exist";
const char data_layer_error_user_already_exist[] = "You are attempting to create an user that's already exist";
const char data_layer_error_unable_to_process_kval[] = "Operation with chosen key-value pair wasn't successful";
const char data_layer_error_item_not_found[] = "Not found";

const char meta_display_source [] = "source";
const char meta_display_data   [] = "data";
const char meta_display_both   [] = "both";

const char *display_enum_to_str(enum record_display d) {
	if (d == DISPLAY_DATASOURCE) return meta_display_source;
	if (d == DISPLAY_DATA) return meta_display_data;
	if (d == DISPLAY_BOTH) return meta_display_both;
	return NULL;
}

enum record_display parse_meta_display(char *ptr, size_t len) {
	if (len == 0) return DISPLAY_INVALID;

	if (memcmp(ptr, meta_display_source, len) == 0) return DISPLAY_DATASOURCE;
	if (memcmp(ptr, meta_display_data,   len) == 0) return DISPLAY_DATA;
	if (memcmp(ptr, meta_display_both,   len) == 0) return DISPLAY_BOTH;
	return DISPLAY_INVALID;
}

bool list_records_dummy(unsigned *amount, unsigned long *result_list, unsigned offset, struct list_filter filter, void *context, const char **error) {
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

bool alter_record_dummy(struct blog_record *r, void *context, const char **error) {
	UNUSED(r);
	UNUSED(context);

	*error = data_layer_error_init;
	return false;
}

#define KEY_VAL_MAXKEYLEN 255

bool key_val_dummy(char *key, void *value, ssize_t *size, void *context, const char **error) {
	UNUSED(key);
	UNUSED(value);
	UNUSED(size);
	UNUSED(context);

	*error = data_layer_error_init;
	return false;
}

bool user_dummy(struct usr *usr, struct user_action action, void *context, const char **error) {
	UNUSED(usr);
	UNUSED(action);
	UNUSED(context);

	*error = data_layer_error_init;
	return false;
}

bool (*list_records)(unsigned *, unsigned long *, unsigned, struct list_filter, void *, const char **) = list_records_dummy;
// attempt to fill a limited-size array integers which corresponds record's id

bool (*get_record)(struct blog_record *, unsigned , void *, const char **) = get_record_dummy;
// retrieve blog_record itself into empty structure. Non-empty structures are prohibited because of stack usage

bool (*insert_record)(struct blog_record *, void *, const char **) = insert_record_dummy;
// insert a blog_record
// if "datasourcelen" field is zero, but requested by "display", markdown processing will be performed

bool (*alter_record)(struct blog_record *, void *, const char **) = alter_record_dummy;
// change the title, tags, or contents of a blog record
// please, passing the following fields is mandatory "chosen_record" and "display".
// Pass "title"+"titlelen" or "datasource"+"datasourcelen" or "data"+"datalen" depending on what do you want to change

bool (*key_val)(char[KEY_VAL_MAXKEYLEN], void *, ssize_t *, void *, const char **) = key_val_dummy;
// simple key-value storage. if size equals zero - we're checking if pair exists.
// If size is greater than zero, we're attempting to insert a new record
// If size is less than zero, we're attempting to retrieve a record
// If size points to NULL pointer, we're removing a record
// if key points to buffer that starts with \0, they key will be provided for API user, but user should
// provide PREFIX that exist right behind this \0 byte. Prefix should be null-terminated string

bool (*user)(struct usr *, struct user_action, void *, const char **) = user_dummy;

enum datalayer_engines {
	ENGINE_NULL,
#ifdef DATA_LAYER_MYSQL
	ENGINE_MYSQL,
#endif
#ifdef DATA_LAYER_FILENO
	ENGINE_FILENO,
#endif
};

typedef void (*datalayer_rand_fun)(void *, size_t);

struct data_layer {
	enum datalayer_engines e;
	const void *addr;
	void *context;
	datalayer_rand_fun randfun;
};

#ifdef DATA_LAYER_MYSQL
#include "abstract_data_layer_mysql.c"
#endif

#ifdef DATA_LAYER_FILENO
#include "abstract_data_layer_fileno.c"
#endif

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

bool initialize_engine(struct data_layer *d, const char **error) {
	switch (d->e) {
#ifdef DATA_LAYER_MYSQL
	case ENGINE_MYSQL:
		list_records = list_records_mysql;
		get_record = get_record_mysql;
		insert_record = insert_record_mysql;
		alter_record = alter_record_mysql;
		key_val = key_val_mysql;
		user = user_mysql;
		return initialize_mysql_context(d, error);
#endif
#ifdef DATA_LAYER_FILENO
	case ENGINE_FILENO:
		list_records = list_records_fileno;
		get_record = get_record_fileno;
		insert_record = insert_record_fileno;
		alter_record = alter_record_fileno;
		key_val = key_val_fileno;
		user = user_fileno;
		return initialize_fileno_context(d, error);
#endif
	default:
		*error = data_layer_error_wrong_engine;
		break;
	}

	return false;
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
