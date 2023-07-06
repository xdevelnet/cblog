#ifndef CBLOG_DEFAULT_RODATA_H
#define CBLOG_DEFAULT_RODATA_H

#define HOW_MANY_RECORDS_U_WANT_TO_SEE_ON_TITLEPAGE 4
#define DEFAULT_MINIMUM_PASSWORD_LEN 7

#define DEFAULT_CRED_HASHING_SALT "change_this_salt"

const char default_appname[] = "Blog demo";
size_t default_appnamelen = strizeof(default_appname);
source_type default_template_type = SOURCE_FILE;
const char default_template_name[] = "static/minimalist/index (copy).ssb";
enum datalayer_engines default_datalayer_type = ENGINE_FILENO;
const char default_datalayer_addr[] = "demo_data";
const char default_title_page_name[] = "Welcome to my blog!";
size_t default_title_page_len = strizeof(default_title_page_name);
const char default_title_content[] = ""
	"<h2>Welcome to my personal blog!<h2><br>"
	"Here you may find some notes and records that I am write from time to time.<br>"
	"Recent posts:";
size_t default_title_content_len = strizeof(default_title_content);
const char default_show_tags_content[] = "Displaying blog by tag";
size_t default_show_tag_content_len = strizeof(default_show_tags_content);
const bool default_password_specialchars_needed = false;

const char default_form_html[] = "<form action=\"/user\" method=\"POST\"><input type=\"text\" placeholder=\"Enter Username\" name=\"name\" required><br><input type=\"password\" placeholder=\"Enter Password\" name=\"password\" required><br><button type=\"submit\">Login</button></form>";
size_t default_form_html_len = strizeof(default_form_html);

#endif //CBLOG_DEFAULT_RODATA_H
