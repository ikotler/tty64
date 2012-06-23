/*
 * httpd.h
 */

void httpd_start_html(int);
void httpd_end_html(int);

int wprintf(int, char *, ...);
void httpd_msg_page(int, char *, int, char *, char *, ...);
char *httpd_url_decode(char *);

void httpd_header(int, int, char *);
void httpd_handler(int, char *, char *);

void httpd_www_noproxies_msg(int);

