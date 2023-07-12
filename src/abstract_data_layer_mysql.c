bool initialize_mysql_context(struct data_layer *d, const char **error) {
	return NULL;
}

bool list_records_mysql(unsigned *amount, unsigned long *result_list, unsigned offset, struct list_filter filter, void *context, const char **error) {
	*error = data_layer_error_havent_implemented;
	return false;
}

bool get_record_mysql(struct blog_record *r, unsigned choosen_record, void *context, const char **error) {
	*error = data_layer_error_havent_implemented;
	return false;
}

bool insert_record_mysql(struct blog_record *r, void *context, const char **error) {
	*error = data_layer_error_havent_implemented;
	return false;
}

bool key_val_mysql(char key[KEY_VAL_MAXKEYLEN], void *value, ssize_t *size, void *context, const char **error) {
	*error = data_layer_error_havent_implemented;
	return false;
}

bool user_mysql(struct usr *usr, struct user_action action, void *context, const char **error) {
	*error = data_layer_error_havent_implemented;
	return false;
}
