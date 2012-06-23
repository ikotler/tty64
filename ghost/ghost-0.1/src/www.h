/*
 * www.h
 */

void httpd_www_err(int, char *, char *, char *, ...);
void httpd_www_syserr_msg(int, char *, char *);
void httpd_www_syserr(int, char *, char *);
void httpd_www_404(int, char *, char *);
void httpd_www_label(int, char *, int);

int httpd_playfile(int, char *);

int httpd_www_config(int, char *, char *);
int httpd_www_proxies(int, char *, char *);
int httpd_www_log(int, char *, char *);
int httpd_www_main(int, char *, char *);
int httpd_www_add_proxy(int, char *, char *);
int httpd_www_about(int, char *, char *);

int httpd_cgi_vall_proxies(int, char *, char *);
int httpd_cgi_chk_proxy(int, char *, char *);
int httpd_cgi_dump_proxies(int, char *, char *);
int httpd_cgi_dump_config(int, char *, char *);
int httpd_cgi_conf_compile(int, char *, char *);
int httpd_cgi_proc_conf(int, char *, char *);
int httpd_cgi_mod_proxy(int, char *, char *);
int httpd_cgi_proc_proxy(int, char *, char *);
int httpd_cgi_del_proxy(int, char *, char *);
int httpd_cgi_proxies_rebuild(int, char *, char *);
int httpd_cgi_conf_newval(int, char *, char *);
int httpd_cgi_conf_flipflop(int, char *, char *);
int httpd_cgi_log_clean(int, char *, char *);
