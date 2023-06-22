bool initialize_mysql_context(const void *addr, void *context, const char **error) {
	return NULL;
}

bool list_records_mysql(unsigned *amount, unsigned long *result_list, unsigned offset, unix_epoch from, unix_epoch to, void *context, const char **error) {
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

bool key_val_mysql(const char *key, void *value, ssize_t *size, void *context, const char **error) {
	*error = data_layer_error_havent_implemented;
	return false;
}
