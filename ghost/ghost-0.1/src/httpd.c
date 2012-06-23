/*
 * httpd.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>

#include <ctype.h>

#include "hash.h"
#include "httpd_ext.h"
#include "www.h"
#include "www_ext.h"

extern hashtable *cfg;

int httpd_css_playback(int, char *, char *);

/*
 * http pages/cgis database
 */

httpd_page pages[] = {
	{ "/"             	  , httpd_www_main            , HTTPD_WWW          } ,
	{ "/log.html"	  	  , httpd_www_log             , HTTPD_WWW          } ,
	{ "/proxies.html"         , httpd_www_proxies         , HTTPD_WWW          } ,
	{ "/config.html"   	  , httpd_www_config   	      , HTTPD_WWW          } ,
	{ "/add-proxy.html"       , httpd_www_add_proxy       , HTTPD_WWW          } ,
	{ "/about.html"           , httpd_www_about           , HTTPD_WWW	   } ,
	{ "/proxy-modifier.cgi"   , httpd_cgi_mod_proxy       , HTTPD_CGI	   } ,
	{ "/proxy-checkup.cgi"    , httpd_cgi_chk_proxy       , HTTPD_CGI          } ,
	{ "/config-newval.cgi"    , httpd_cgi_conf_newval     , HTTPD_CGI          } ,
	{ "/proxies-dump.cgi"     , httpd_cgi_dump_proxies    , HTTPD_CGI_REDIRECT } ,
	{ "/proxies-vall.cgi"	  , httpd_cgi_vall_proxies    , HTTPD_CGI_REDIRECT } ,
	{ "/proxies-rebuild.cgi"  , httpd_cgi_proxies_rebuild , HTTPD_CGI_REDIRECT } ,
	{ "/delete-proxy.cgi"     , httpd_cgi_del_proxy       , HTTPD_CGI_REDIRECT } ,
	{ "/config-dump.cgi"      , httpd_cgi_dump_config     , HTTPD_CGI_REDIRECT } ,
	{ "/config-compile.cgi"   , httpd_cgi_conf_compile    , HTTPD_CGI_REDIRECT } ,
	{ "/proxy-processor.cgi"  , httpd_cgi_proc_proxy      , HTTPD_CGI_REDIRECT } ,
	{ "/config-processor.cgi" , httpd_cgi_proc_conf       , HTTPD_CGI_REDIRECT } ,
	{ "/config-flipflop.cgi"  , httpd_cgi_conf_flipflop   , HTTPD_CGI_REDIRECT } ,
	{ "/cleanlog.cgi"         , httpd_cgi_log_clean       , HTTPD_CGI_REDIRECT } ,
 	{ "/style.css"            , httpd_css_playback	      , HTTPD_RAW          } ,
	{ NULL		  	  , NULL	              , -1                 }
};

/*
 * wprintf, write hybrid data into fd
 * * fd, given fd
 * * fmt, format string 
 */

int wprintf(int fd, char *fmt, ...) {
        va_list ap;
        char buf[1024];

        va_start(ap, fmt);
        vsnprintf(buf, sizeof(buf), fmt, ap);
        va_end(ap);

	return write(fd, buf, strlen(buf));
}

/*
 * httpd_www_msg_page, simple msg page w/ redirection option
 * * fd, given fd
 * * title, given page title
 * * rsecs, given secs til redirection
 * * rurl, given redirection url
 * * msg, given msg
 */

void httpd_msg_page(int fd, char *title, int rsecs, char *rurl, char *msg, ...) {
	char buf[1024];
        va_list ap;

        va_start(ap, msg);
        vsnprintf(buf, sizeof(buf), msg, ap);
        va_end(ap);

	write(fd, WWW_HTML_START, strlen(WWW_HTML_START));

	if (rsecs ) {
		wprintf(fd, "<META HTTP-EQUIV=\"Refresh\" CONTENT=\"%d; URL=%s\">\n", rsecs, rurl);
	}

	wprintf(fd, "<TITLE>%s</TITLE>\r\n", title);

	wprintf(fd, "<STYLE TYPE=\"text/css\">\r\n");
	write(fd, WWW_CSS_FILE, strlen(WWW_CSS_FILE));
	wprintf(fd, "</STYLE>\r\n");

	wprintf(fd, "</HEAD>\r\n<BODY>\r\n");

	wprintf(fd, "<TABLE CLASS=\"fullheight\" WIDTH=\"100%%\" BORDER=\"0\">\r\n");

	wprintf(fd, "<TR>\r\n");
	wprintf(fd, "<TD ALIGN=\"CENTER\" VALIGN=\"MIDDLE\">\r\n");
	wprintf(fd, "<H1>%s</H1>\r\n", title);
	wprintf(fd, "<B>%s</B>\r\n", buf);
	wprintf(fd, "</TD>\r\n");
	wprintf(fd, "</TR>\r\n");

	wprintf(fd, "</TABLE>\r\n");

	write(fd, WWW_HTML_END, strlen(WWW_HTML_END));

	return ;
}

/*
 * httpd_www_noproxies_msg, ghost out of proxies msg
 * * fd, given fd
 */

void httpd_www_noproxies_msg(int fd) {
	char *retval;

	retval = hashtable_lookup(cfg, "http-analyzer");

	if (!retval || !strcmp(retval, "off")) {

		hashtable_insert(cfg, "http-analyzer", "on");
		hashtable_insert(cfg, "http-admin-mode", "on");
		hashtable_insert(cfg, "http-admin-url", "http://ghost/");

	} else {

		retval = hashtable_lookup(cfg, "http-admin-mode");
	
		if (!retval || !strcmp(retval, "off")) {
			hashtable_insert(cfg, "http-admin-mode", "on");
			hashtable_insert(cfg, "http-admin-url", "http://ghost/");
		}
	}

	httpd_msg_page(fd, "Out of Proxies!", -1, NULL,
		"There are currectly no proxies or no working proxies in the database.<BR>\n \
	        Please click <A HREF=\"%sproxies.html\"><U>here</U></A> to access the database<BR> \n",
			hashtable_lookup(cfg, "http-admin-url"));

	return ;
}

/*
 * httpd_start_html, start HTML page
 * * fd, given fd
 */

void httpd_start_html(int fd) {
	write(fd, WWW_HTML_START, strlen(WWW_HTML_START));

        wprintf(fd, "<STYLE TYPE=\"text/css\">\r\n");
        write(fd, WWW_CSS_FILE, strlen(WWW_CSS_FILE));
        wprintf(fd, "</STYLE>\r\n");
	
	write(fd, WWW_HTML_PAGE, strlen(WWW_HTML_PAGE));
	write(fd, WWW_HTML_LOGO, strlen(WWW_HTML_LOGO));
	return ;
}

/*
 * httpd_end_html, end HTML page
 * * fd, given fd
 */

void httpd_end_html(int fd) {
	write(fd, WWW_HTML_END, strlen(WWW_HTML_END));
	return ;
}

/*
 * httpd_header, http server header
 * * fd, given fd
 * * code, http response code
 * * redirect, redirect url
 */

void httpd_header(int fd, int code, char *redirect) {

	char hdr[1024];

	memset((void *)hdr, 0x0, 1024);

	switch (code) {
		
		case 302:
		
			strcat(hdr, "HTTP/1.1 302 Found\r\n");
			break;

		case 200: 

			strcat(hdr, "HTTP/1.1 200 OK\r\n");
			break;

		case 404:

			strcat(hdr, "HTTP/1.1 404 Not Found\r\n");
			break;
	}

	strcat(hdr, "Date: Sun, 01 Jan 1970 00:00:00 GMT\r\n");
	strcat(hdr, "Server: Ghost\r\n");

	if (redirect) {
		strcat(hdr, "Location: ");
		strcat(hdr, redirect);
		strcat(hdr, "\r\n");
	}

	strcat(hdr, "Content-Type: text/html; charset=iso-8859-1\r\n");
	strcat(hdr, "Connection: close\r\n");
	strcat(hdr, "Proxy-Connection: close\r\n");
	strcat(hdr, "\r\n");

	write (fd, hdr, strlen(hdr));

	return ;
}

/*
 * httpd_handler, local httpd request handler
 * * fd, given fd
 * * url, given url
 * * buf, given http chunk
 */

void httpd_handler(int fd, char *url, char *buf) {
	int idx, url_length;
	char *cgi_params;

	cgi_params = strchr(url, '?');
	
	url_length = strlen(url);

	if (cgi_params) {
		url_length -= strlen(cgi_params);
	}

	for (idx = 0; pages[idx].page != NULL; idx++) {

		if (!strncmp(url, pages[idx].page, url_length)) {

			switch (pages[idx].type) {

				case HTTPD_CGI:
				case HTTPD_WWW:
					
					httpd_header(fd, 200, NULL);
					httpd_start_html(fd);
					pages[idx].proc(fd, buf, cgi_params);
					httpd_end_html(fd);
					break;

				case HTTPD_RAW:

					httpd_header(fd, 200, NULL);
					pages[idx].proc(fd, buf, cgi_params);
					break;

				case HTTPD_CGI_REDIRECT:

					pages[idx].proc(fd, buf, cgi_params);
					break;
			}

			return ;
		}
	}

	httpd_www_404(fd, url, buf);

	return ;
}

/*
 * httpd_url_decode, decode %'s from URL
 * * url, given url
 */

char *httpd_url_decode(char *url) {
        int j, i, val;
        char buf[2];

	if (!url) {
		return url;
	}

        for (j = 0; j < strlen(url); j++) {

                if (url[j] == '%') {

                        if (j+2 > strlen(url)) {
                                break;
                        }

                        buf[0] = url[j+1];
                        buf[1] = url[j+2];

                        if (isxdigit(buf[0]) && isxdigit(buf[1])) {

                                sscanf(buf, "%x", &val);
                                url[j] = val;

                                for (i = j+3; i < strlen(url) + 1; i++) {
                                        url[i-2] = url[i];
                                }
                        }
                }
        }

        return url;
}

/*
 * httpd_css_playback, play .CSS back to client
 * * fd, given socket
 * * raw, given raw http chunk
 * * cgi, given cgi options
 */

int httpd_css_playback(int fd, char *raw, char *cgi) {
	return write(fd, WWW_CSS_FILE, strlen(WWW_CSS_FILE));
}
