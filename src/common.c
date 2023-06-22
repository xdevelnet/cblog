const char config_parser_error[] = "Invalid config file";
const char config_parser_fcreated[] = "File with default config has been created";

void set_config_defaults(struct appconfig *conf) {
	conf->appname = default_appname;
	conf->appnamelen = default_appnamelen;
	conf->template_type = default_template_type;
	conf->temlate_name = default_template_name;
	conf->datalayer_type = default_datalayer_type;
	conf->datalayer_addr = default_datalayer_addr;
	conf->title_page_name = default_title_page_name;
	conf->title_page_name_len = default_title_page_len;
	conf->title_page_content = default_title_content;
	conf->title_page_content_len = default_title_content_len;
}

#define CONFIG_HEADER "CBLOG1:"
#define CONFIG_APPNAME "appname: "
#define CONFIG_TEMPLATE_ADDR "template_addr: "
#define CONFIG_DATALAYER_TYPE "datalayer_type: "
#define CONFIG_DATALAYER_ADDR "datalayer_addr: "
#define CONFIG_TITLE_PAGE_NAME "title_page_name: "
#define CONFIG_TITLE_PAGE_CONTENT "title_page_content: "
bool if_empty_flush_default_config(int fd) {
	char a;
	ssize_t got = read(fd, &a, sizeof(char));
	if (got > 0) {
		lseek(fd, SEEK_CUR, 0);
		return false;
	}
	dprintf(fd, CONFIG_HEADER "\n"
				CONFIG_APPNAME"%s\n"
				CONFIG_TEMPLATE_ADDR"%s\n"
				CONFIG_DATALAYER_TYPE"%s\n"
				CONFIG_DATALAYER_ADDR"%s\n"
				CONFIG_TITLE_PAGE_NAME"%s\n"
				CONFIG_TITLE_PAGE_CONTENT"%s\n",
				default_appname,
				default_template_name,
layer_engine_to_str(default_datalayer_type),
				default_datalayer_addr,
				default_title_page_name,
				default_title_content);

	return true;
}

#define CONFIG_TEST(TEST, FIELD, LEN) do {if (strpartcmp(str, TEST) == STREQ) {str += strlen(TEST);if (*str != '\0' and *str != '\n') {conf->FIELD = str;conf->LEN = strlen(str);} return true;}} while(0)
#define CONFIG_TEST_WOLEN(TEST, FIELD) do {if (strpartcmp(str, TEST) == STREQ) {str += strlen(TEST);if (*str != '\0' and *str != '\n') {conf->FIELD = str;} return true;}} while(0)
#define CONFIG_TEST_OBJ(TEST, OBJ) do {if (strpartcmp(str, TEST) == STREQ) {str += strlen(TEST);if (*str != '\0' and *str != '\n') {OBJ = str;} return true;}} while(0)

bool config_record(struct appconfig *conf, char *str) {
	CONFIG_TEST(CONFIG_APPNAME, appname, appnamelen);
	CONFIG_TEST_WOLEN(CONFIG_TEMPLATE_ADDR, temlate_name);
	const char *datalayer_type = NULL; // todo: parseit
	CONFIG_TEST_OBJ(CONFIG_DATALAYER_TYPE, datalayer_type);
	if (str_to_layer_engine(datalayer_type) != ENGINE_NULL) conf->datalayer_type = str_to_layer_engine(datalayer_type);
	CONFIG_TEST_WOLEN(CONFIG_DATALAYER_ADDR, datalayer_addr);
	CONFIG_TEST(CONFIG_TITLE_PAGE_NAME, title_page_name, title_page_name_len);
	CONFIG_TEST(CONFIG_TITLE_PAGE_CONTENT, title_page_content, title_page_content_len);

	return false;
}

bool parse_config(struct appconfig *conf, const char *file, const char **error) {
	int fd = open(file, O_RDWR | O_CREAT, DEFAULT_FILE_MODE);
	if (fd < 0) OUCH_ERROR(strerror(errno), return false);
	if (if_empty_flush_default_config(fd) == true) OUCH_ERROR(config_parser_fcreated, close(fd); return false);
	size_t size;
	char *map = map_whole_file_private(fd, &size, NULL);
	close(fd);
	if (map == NULL) OUCH_ERROR(strerror(errno), return false);

	if (map[size - 1] != '\n' or strpartcmp(map, CONFIG_HEADER) == STRNEQ) goto fail;
	map[size - 1] = '\0';

	char *fly = map + strizeof(CONFIG_HEADER);
	if (*fly == '\0') goto fail;

	char *current = map;
	bool cont = true;
	while(cont) {
		if (*current == '\n') {current++; continue;}
		if (*current == '\0') break;
		char *key = current;
		current = strchr(current, '\n');
		if (current == NULL) {
			cont = false;
		} else {
			*current = '\0';
			current++;
		}
		config_record(conf, key);
	}

	conf->context = map;
	conf->contextsize = size;

	return true;

	fail:
	if (error) *error = config_parser_error;
	munmap(map, size);
	return false;
}

void parse_config_erase(struct appconfig *conf) {
	if (conf->context == NULL) return;
	munmap(conf->context, conf->contextsize);
}
