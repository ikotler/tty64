/*
 * httpd_ext.h
 */

typedef struct httpd_page httpd_page;
typedef struct httpd_page_menu httpd_page_menu;

enum httpd_page_types {
	HTTPD_WWW,
	HTTPD_CGI_REDIRECT,
	HTTPD_CGI,
	HTTPD_RAW
};

struct httpd_page {
	char *page;
	int (*proc)(int, char *, char *);
	int type;
	int has_customize_menu;
};
