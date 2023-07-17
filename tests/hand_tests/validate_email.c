#include <stdio.h>
#include <stdbool.h>
#include <iso646.h>
#include <string.h>

#if !defined strizeof
#define strizeof(a) (sizeof(a)-1)
#endif

#define EMAIL_MAXLEN 254

bool emb_isalpha(char c) {
	if ((c >= 'a' and c <= 'z') or (c >= 'A' and c <= 'Z')) return true;
	return false;
}

bool emb_isnumeric(char c) {
	if (c >= '0' and c <= '9') return true;
	return false;
}

bool is_special_in_prefix(char c) {
	switch (c) {
	case '.':
	case '-':
	case '_':
	case '+':
	case '%':
		return true;
	default :
		break;
	}

	return false;
}

bool validate_prefix(const char *email) {
	bool special_char = false;
	if (is_special_in_prefix(*email)) return false;

	while (*email != '@') {
		if (emb_isalpha(*email) == true or emb_isnumeric(*email)) {
			special_char = false;
			email++;
			continue;
		}

		if (is_special_in_prefix(*email) == true) {
			if (special_char == true) return false;
			special_char = true;
			email++;
			continue;
		}

		return false;
	}

	if (special_char == true) return false;
	return true;
}

bool validate_domain(const char *domain) {
	domain++;
	size_t dots = 0;

	bool dot = false;
	bool dash = false;
	bool regular = false;

	while (*domain != '\0') {
		if (emb_isalpha(*domain) == true or emb_isnumeric(*domain)) {
			dot = false;
			dash = false;
			regular = true;
			domain++;
			continue;
		}

		if (*domain == '.') {
			if (regular == false or dot == true) return false;

			dot = true;
			dash = false;
			regular = false;
			domain++;
			dots++;
			continue;
		}

		if (*domain == '-') {
			if (regular == false and dash == false) return false;

			dot = false;
			dash = true;
			regular = false;
			domain++;
			continue;
		}

		return false;
	}

	if (dash == true or dot == true or dots == 0) return false;
	return true;
}

bool validate_email_wo_regex(char *email) {
	if (email == NULL or email[0] == '\0') return false;
	size_t len = strlen(email);
	if (len > EMAIL_MAXLEN) return false;
	if (len < strizeof("a@b.c")) return false;
	char *at_sign = strchr(email, '@');
	if (at_sign == NULL or at_sign == email) return false;

	if (validate_prefix(email) == false or validate_domain(at_sign) == false) return false;
	return true;
}

bool validate_email(char *email) {
	return validate_email_wo_regex(email);
}

void vp(char *s) {
	char *result = "\033[0;31mfailure\033[0m";
	if (validate_email(s) == true) result = "\033[0;32msuccess\033[0m";
	printf("%s: %s\n", result, s);
}

int main() {
	vp("@");
	vp("a@b.c");
	vp("a@b.c.d");
	vp("aaa@bbbb.cccc");
	vp("aaa@bbbb.ccc");
	vp("a.aa@bbbb.ccc");
	vp("a..aa@bbbb.ccc");
	vp("a.a.a@bbbb.ccc");
	vp("a.aa.@bbbb.ccc");
	vp(".aaa@bbbb.ccc");
	vp("@bbbb.ccc");
	vp("abcdef@xn--80alnp8bxf.pl.ua");
	vp("qqqqqqqqqqqqqq@eeeeeeee.co.asdas.com");
	vp("president@president.gov.ua");
	vp("hello@domain..com");
	vp("hello@-domain.com");
	vp("hello@do-main.com");
	vp("hello@domain-.com");
	vp("hello@domain.-com");
	vp("hello@dom--n.com");
	vp("hello@dom--ncom");
	return 0;
}
