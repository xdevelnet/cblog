#ifndef CBLOG_DEFAULT_RODATA_H
#define CBLOG_DEFAULT_RODATA_H

#define HOW_MANY_RECORDS_U_WANT_TO_SEE_ON_TITLEPAGE 5

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

#endif //CBLOG_DEFAULT_RODATA_H
