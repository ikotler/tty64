/*
 * www.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <time.h>
#include <errno.h>
#include <ctype.h>

#define __USE_GNU
#include <string.h>

#include "ghostd.h"
#include "hash.h"
#include "log.h"
#include "proxy.h"
#include "proxy_ext.h"
#include "conf.h"
#include "conf_ext.h"
#include "httpd.h"
#include "www_links.h"

extern hashtable *cfg;
extern cfg_validate_entry cfg_truthtable[];

hashtable *alt_cfg;

/*
 * cgi_parse_token, parse string/token from cgi options
 * * buf, given cgi options/url
 * * opt, given cgi option
 */

char *cgi_parse_token(char *buf, char *opt) {
	char *p, *val;
	int j;

	if (!opt || !buf) {
		return NULL;
	}

	p = strstr(buf, opt);
	p += strlen(opt) + 1;

	if (!p) {
		return NULL;
	}

	for (j = 0; j < strlen(buf); j++) {

		if (p[j] == '&') {

			if (!j) {
				return NULL;
			}

			break;
		}
	}

	val = strndup(p, j);

	if (!val) {
		return NULL;
	}

	return val;
}

/*
 * cgi_parse_int, parse int from cgi options
 * * buf, given cgi options/url
 * * opt, given cgi option
 */

int cgi_parse_int(char *buf, char *opt) {
	char *p, *val;
	int j;

	if (!opt || !buf) {
		return -1;
	}

	p = strstr(buf, opt);
	p += strlen(opt) + 1;

	if (!p) {
		return -1;
	}

	for (j = 0; j < strlen(buf); j++) {

		if (!isdigit(p[j])) {

			if (!j) {
				return -1;
			}

			break;
		}
	}

	val = strndup(p, j);

	if (!val) {
		return -1;
	}

	j = atoi(val);

	free(val);

	return j;
}

/*
 * httpd_menu_builder, build TD's tags for given items
 * * fd, given fd
 * * items, given menu items
 */

__inline__ int httpd_menu_builder(int fd, www_page_menu *items) {
        int j;

        if (items) {
                for (j = 0; items[j].item != NULL; j++) {
                        wprintf(fd, "<TD>[<A HREF=\"%s\">%s</A>]</TD>\n", items[j].url, items[j].item);
                }

		return 1;
        }

        return 0;
}

/*
 * httpd_start_hmenu, start HTML menu
 * * fd, given fd
 * * items, given extra/customize items
 */

void httpd_start_hmenu(int fd, www_page_menu *items) {
        write(fd, WWW_HTML_MENU_START, strlen(WWW_HTML_MENU_START));

        if (httpd_menu_builder(fd, items)) {
                wprintf(fd, "<TD> / </TD>\n");
        }

        httpd_menu_builder(fd, WWW_DEFAULT_ITEMS);

        write(fd, WWW_HTML_MENU_END, strlen(WWW_HTML_MENU_END));

        return ;
}

/*
 * httpd_end_hmenu, end HTML menu
 * * fd, given fd
 */

void httpd_end_hmenu(int fd) {
        write(fd, WWW_HTML_MENU_END, strlen(WWW_HTML_MENU_END));
        return ;
}

/*
 * httpd_www_err, generic httpd error page/dialog
 * * fd, given fd
 * * title, given page title
 * * chunk, given http chunk
 * * msg, given msg
 */

void httpd_www_err(int fd, char *title, char *chunk, char *msg, ...) {
        va_list ap;
        char buf[1024];

        va_start(ap, msg);
        vsnprintf(buf, sizeof(buf), msg, ap);
        va_end(ap);

	wprintf(fd, "<H1>%s</H1>\n", title);
	wprintf(fd, "%s\n", buf);
	wprintf(fd, "<HR>\n");
	wprintf(fd, "<BR>\n");
	wprintf(fd, "<PRE>\n");
	wprintf(fd, "%s", chunk);
	wprintf(fd, "</PRE>\n");

	return ;
}

/*
 * httpd_www_syserr_msg, system error msg
 * * fd, given fd
 * * str, who's to blame?
 * * args, given raw http chunk
 */

__inline__ void httpd_www_syserr_msg(int fd, char *str, char *args) {
	httpd_www_err(fd, "System Error", args, "%s: %s\n", str, strerror(errno));
	return ;
}

/*
 * httpd_www_syserr, system error page
 * * fd, given fd
 * * str, who's to blame?
 * * args, given chunk
 */

void httpd_www_syserr(int fd, char *str, char *args) {
	httpd_header(fd, 200, NULL);
	httpd_start_html(fd);
	httpd_www_syserr_msg(fd, str, args);
	httpd_end_html(fd);

	return ;
}

/*
 * httpd_www_404, the famous 404 page
 * * fd, given fd
 * * url, given url
 * * args, given chunk
 */

void httpd_www_404(int fd, char *url, char *args) {
	httpd_header(fd, 404, NULL);
	httpd_start_html(fd);
	httpd_www_err(fd, "Not Found", args, "The requested URL %s was not found on this server.\n", url);
	httpd_end_html(fd);

	return ;
}

/*
 * httpd_www_gerror, ghost error page
 * * fd, given fd
 * * title, given title
 * * raw, given httpd raw chunk
 * * msg, given msg
 */

void httpd_www_gerror(int fd, char *title, char *raw, char *msg, ...) {
        va_list ap;
        char buf[1024];

        va_start(ap, msg);
        vsnprintf(buf, sizeof(buf), msg, ap);
        va_end(ap);

	httpd_header(fd, 200, NULL);
	httpd_start_html(fd);
	httpd_www_err(fd, title, raw, buf);
	httpd_end_html(fd);

	return ;
}

/*
 * httpd_www_label, html label box
 * * fd, given fd
 * * text, given text
 * * width, given width % for label
 */

void httpd_www_label(int fd, char *text, int width) {

	wprintf(fd, "<BR>\n");
	wprintf(fd, "<TABLE BORDER=\"0\" CELLPADDING=\"0\" CELLSPACING=\"0\" ALIGN=\"CENTER\" WIDTH=\"%d%%\" BGCOLOR=\"#404040\">\n", width);
        wprintf(fd, "\t<TR>\n");
        wprintf(fd, "\t\t<TD ALIGN=\"CENTER\">\n");
        wprintf(fd, "\t\t\t<B>%s</B>\n", text);
        wprintf(fd, "\t\t</TD>\n");
        wprintf(fd, "\t</TR>\n");
	wprintf(fd, "</TABLE>\n");
	wprintf(fd, "<BR>\n");

	return ;
}

/*
 * cgi_parse_index, quick inline to parse index (id) passed via cgi option
 * * fd, given fd
 * * buf, cgi-url options
 */

__inline__ int cgi_parse_index(int fd, char *buf) {
	char *err;
	int idx;

	idx = cgi_parse_int(buf, "id");

	if (!buf || idx < 0) {

		err = "Not enough parameters!";
		
		if (idx < 0) {
			err = "Invalid index!";
		}

		httpd_www_label(fd, err, 40);

		return -1;
	}		

	return idx;
}

/*
 * httpd_playfile, play given file into socket
 * * fd, given fd
 * * file, given file
 */

int httpd_playfile(int fd, char *file) {
	FILE *fp;
	char chunk[1024];

	fp = fopen(file, "r");

	if (!fp) {
		lerror(file);
		return -1;
	}
	
	while (!feof(fp)) {

		if (fgets(chunk, sizeof(chunk), fp) == NULL) {
			break;
		}

		if (write(fd, chunk, strlen(chunk)) < 0) {
			fclose(fp);
			return -1;
		}

		memset(chunk, 0x0, sizeof(chunk));
	}

	fclose(fp);

	return 1;
}

/*
 * httpd_www_config, ghost config.html page
 * * fd, given fd
 * * raw, given raw http chunk 
 * * cgi, given cgi options
 */

int httpd_www_config(int fd, char *raw, char *cgi) {
	int idx;
	char *refer_cgi, *a_retval, *retval;

	if (!alt_cfg) {

		alt_cfg = hashtable_create(_MAX_CONF_OPTS);

		if (!alt_cfg) {
			httpd_www_syserr_msg(fd, "hashtable_create", raw);
			return 0;
		}
	}

	httpd_start_hmenu(fd, config_menu);
	httpd_end_hmenu(fd);

	httpd_www_label(fd, "Configuration", 80);

 	wprintf(fd, "<TABLE BORDER=\"1\" CELLPADDING=\"0\" CELLSPACING=\"0\" WIDTH=\"85%\" ALIGN=\"CENTER\" VALIGN=\"MIDDLE\">\n");
	wprintf(fd, "\t<TR>\n");
	wprintf(fd, "\t\t<TD ALIGN=\"CENTER\">option</TD>\n");
	wprintf(fd, "\t\t<TD ALIGN=\"CENTER\">value</TD>\n");
	wprintf(fd, "\t</TR>\n");

	for (idx = 0; cfg_truthtable[idx].tag != NULL; idx++) {

		if (cfg_truthtable[idx].tag_mode) {

			wprintf(fd, "\t<TR>\n");
			wprintf(fd, "\t\t<TD>%s</TD>\n", cfg_truthtable[idx].tag);

			refer_cgi = "config-newval.cgi";

			if (cfg_truthtable[idx].tag_type == IS_BOOLEAN) {
					refer_cgi = "config-flipflop.cgi";
			}	
			
			a_retval = hashtable_lookup(alt_cfg, cfg_truthtable[idx].tag);

			retval = hashtable_lookup(cfg, cfg_truthtable[idx].tag);

			if (!retval) {
				retval = "no value";
			}

			if (a_retval) {

				wprintf(fd, "\t\t<TD ALIGN=\"CENTER\"><A HREF=\"/%s?id=%d\">%s (%s)</A></TD>\n",
					refer_cgi, idx, retval, a_retval);

			} else {

				wprintf(fd, "\t\t<TD ALIGN=\"CENTER\"><A HREF=\"/%s?id=%d\">%s</A></TD>\n",
					refer_cgi, idx, retval);
			}

			wprintf(fd, "\t</TR>\n");
		}
	}

	wprintf(fd, "</TABLE>\r\n");
	
	return 1;	
}

/*
 * httpd_www_proxies, ghost proxies.html page
 * * fd, given fd
 * * raw, given raw http chunk 
 * * cgi, given cgi options
 */

int httpd_www_proxies(int fd, char *raw, char *cgi) {
	int idx, p_counter;
	proxy_entry *cur_proxy;
	proxy_db_stats *cur_db;

	cur_db = proxy_get_stats();
		
	if (!cur_db) {
		httpd_www_syserr(fd, "calloc", raw);
		return 0;
	}

	httpd_start_hmenu(fd, proxies_menu);
	httpd_end_hmenu(fd);

	if (cur_db->index < 1) {

		httpd_www_label(fd, "There are no proxies in the database!", 45);
		free(cur_db);
		return 1;
	}

	httpd_www_label(fd, "Proxies Database", 80);

 	wprintf(fd, "<TABLE BORDER=\"1\" CELLPADDING=\"0\" CELLSPACING=\"0\" WIDTH=\"85%\" ALIGN=\"CENTER\" VALIGN=\"MIDDLE\">\n");
	wprintf(fd, "\t<TR>\n");
	wprintf(fd, "\t\t<TD ALIGN=\"CENTER\">#</TD>\n");
	wprintf(fd, "\t\t<TD ALIGN=\"CENTER\">proxy address</TD>\n");
	wprintf(fd, "\t\t<TD ALIGN=\"CENTER\">proxy port</TD>\n");
	wprintf(fd, "\t\t<TD ALIGN=\"CENTER\">proxy rank</TD>\n");
	wprintf(fd, "\t\t<TD ALIGN=\"CENTER\">proxy strikes</TD>\n");
	wprintf(fd, "\t\t<TD ALIGN=\"CENTER\">proxy jobs</TD>\n");
	wprintf(fd, "\t\t<TD ALIGN=\"CENTER\" COLSPAN=\"3\">proxy options</TD>\n");
	wprintf(fd, "\t</TR>\n");

	p_counter = 1;

	for (idx = 0; idx < cur_db->index; idx++) {

		cur_proxy = proxy_querydb(idx);

		if (!cur_proxy) {
			continue;
		}

		wprintf(fd, "\t<TR>\n");
		wprintf(fd, "\t\t<TD ALIGN=\"CENTER\">%d</TD>\n", p_counter);
		wprintf(fd, "\t\t<TD>%s</TD>\n", cur_proxy->addr);
		wprintf(fd, "\t\t<TD ALIGN=\"CENTER\">%d</TD>\n", cur_proxy->port);
		wprintf(fd, "\t\t<TD ALIGN=\"CENTER\">%s</TD>\n", proxy_getrank(cur_proxy));
		wprintf(fd, "\t\t<TD ALIGN=\"CENTER\">%d</TD>\n", cur_proxy->strikes);
		wprintf(fd, "\t\t<TD ALIGN=\"CENTER\">%d</TD>\n", cur_proxy->jobs);
		wprintf(fd, "\t\t<TD ALIGN=\"CENTER\"><A HREF=\"/proxy-checkup.cgi?id=%d\">validate</A></TD>\n", idx);
		wprintf(fd, "\t\t<TD ALIGN=\"CENTER\"><A HREF=\"/proxy-modifier.cgi?id=%d\">modify</A></TD>\n", idx);
		wprintf(fd, "\t\t<TD ALIGN=\"CENTER\"><A HREF=\"/delete-proxy.cgi?id=%d\">delete</A></TD>\n", idx);
		wprintf(fd, "\t</TR>\n");

		p_counter++;

		free(cur_proxy);
	}

	wprintf(fd, "</TABLE>\r\n");
	
	free(cur_db);

	return 1;
}

/*
 * httpd_www_proxy_dialog, display proxy dialog
 * * fd, given fd
 * * action, url for action
 * * proxyidx, given proxy idx 
 * * proxyaddr, given proxy addr
 * * proxyport, given proxy port
 * * submit_cpt, submit button caption
 */

__inline__ void httpd_www_proxy_dialog(int fd, char *action, int proxyidx, char *proxyaddr, int proxyport, char *submit_cpt) {

 	wprintf(fd, "<TABLE BORDER=\"1\" CELLPADDING=\"0\" CELLSPACING=\"0\" WIDTH=\"35%\" ALIGN=\"CENTER\" VALIGN=\"MIDDLE\">\n");
	wprintf(fd, "<TR>\n");
	wprintf(fd, "<TD COLSPAN=\"2\">\n");
    	wprintf(fd, "<FORM ACTION=\"%s\" METHOD=\"GET\">\n", action);

	if (proxyaddr) {
		wprintf(fd, "proxy address: <INPUT SIZE=\"24\" TYPE=\"TEXT\" VALUE=\"%s\" NAME=\"proxyaddr\">\n", proxyaddr);
	        wprintf(fd, "<BR>\n");
		wprintf(fd, "proxy port: <INPUT SIZE=\"6\" TYPE=\"TEXT\" VALUE=\"%d\" NAME=\"proxyport\">\n", proxyport);
	} else {
		wprintf(fd, "proxy address: <INPUT SIZE=\"24\" TYPE=\"TEXT\" NAME=\"proxyaddr\">\n");
	        wprintf(fd, "<BR>\n");
		wprintf(fd, "proxy port: <INPUT SIZE=\"6\" TYPE=\"TEXT\" NAME=\"proxyport\">\n");
	}

	wprintf(fd, "<INPUT READONLY TYPE=\"HIDDEN\" NAME=\"proxyidx\" VALUE=\"%d\">\n", proxyidx);
        wprintf(fd, "<BR>\n");
	wprintf(fd, "</TD>\n");
	wprintf(fd, "</TR>\n");
	wprintf(fd, "<TR>\n");
	wprintf(fd, "<TD ALIGN=\"CENTER\"><INPUT TYPE=SUBMIT VALUE=\"%s\"></TD>\n", submit_cpt);
	wprintf(fd, "<TD ALIGN=\"CENTER\"><INPUT TYPE=RESET VALUE=\"clear\"></TD>\n");
	wprintf(fd, "</FORM>\n");
	wprintf(fd, "</TD>\n");
	wprintf(fd, "</TR>\n");
	wprintf(fd, "</TABLE>\n");
	
	return ;
}

/*
 * httpd_www_conf_dialog, display config new value dialog
 * * fd, given fd
 * * action, url for action
 * * confidx, given config entry idx
 * * curval, current entry value
 * * submit_cpt, submit button caption
 */

__inline__ void httpd_www_conf_dialog(int fd, char *action, int confidx, char *curval, char *submit_cpt) {

    	wprintf(fd, "<FORM ACTION=\"%s\" METHOD=\"GET\">\n", action);

	wprintf(fd, "<TABLE BORDER=\"0\" ALIGN=\"CENTER\" VALIGN=\"MIDDLE\">\n");
	wprintf(fd, "<TR><TD ALIGN=\"CENTER\">Current value: %s</TD></TR>\n", curval);
	wprintf(fd, "</TABLE>\n");
	wprintf(fd, "<BR>\n");

 	wprintf(fd, "<TABLE BORDER=\"1\" CELLPADDING=\"0\" CELLSPACING=\"0\" WIDTH=\"35%\" ALIGN=\"CENTER\" VALIGN=\"MIDDLE\">\n");
	wprintf(fd, "<TR>\n");
	wprintf(fd, "<TD COLSPAN=\"2\">\n");
	wprintf(fd, "New value: <INPUT SIZE=\"24\" TYPE=\"TEXT\" NAME=\"newval\">\n");
	wprintf(fd, "<INPUT READONLY TYPE=\"HIDDEN\" NAME=\"idx\" VALUE=\"%d\">\n", confidx);
	wprintf(fd, "</TD>\n");
	wprintf(fd, "</TR>\n");

	wprintf(fd, "<TR>\n");
	wprintf(fd, "<TD ALIGN=\"CENTER\"><INPUT TYPE=SUBMIT VALUE=\"%s\"></TD>\n", submit_cpt);
	wprintf(fd, "<TD ALIGN=\"CENTER\"><INPUT TYPE=RESET VALUE=\"clear\"></TD>\n");
	wprintf(fd, "</FORM>\n");
	wprintf(fd, "</TD>\n");
	wprintf(fd, "</TR>\n");

	wprintf(fd, "</TABLE>\n");
	
	return ;
}

/*
 * httpd_www_add_proxy, add proxy .cgi
 * * fd, given fd
 * * raw, given raw http chunk 
 * * cgi, given cgi options
 */

int httpd_www_add_proxy(int fd, char *raw, char *cgi) {
	httpd_start_hmenu(fd, NULL);
	httpd_end_hmenu(fd);

	httpd_www_label(fd, "Add Proxy", 40);

	httpd_www_proxy_dialog(fd, "/proxy-processor.cgi", -1, NULL, -1, "insert");

	return 1;
}

/*
 * httpd_www_log, ghost log.html page
 * * fd, given fd
 * * raw, given raw http chunk 
 * * cgi, given cgi options
 */

int httpd_www_log(int fd, char *raw, char *cgi) {
	httpd_start_hmenu(fd, log_menu);
	httpd_end_hmenu(fd);

	httpd_www_label(fd, hashtable_lookup(cfg, "daemon-logfile"), 80);

 	wprintf(fd, "<TABLE BORDER=\"0\" CELLPADDING=\"0\" CELLSPACING=\"0\" WIDTH=\"45%\" ALIGN=\"CENTER\" VALIGN=\"MIDDLE\">\n");
	wprintf(fd, "\t<TR>\n");
	wprintf(fd, "\t\t<TD>\n");
	wprintf(fd, "\t\t\t<PRE>\n");

	httpd_playfile(fd, hashtable_lookup(cfg, "daemon-logfile"));

	wprintf(fd, "\t\t\t</PRE>\n");
	wprintf(fd, "\t\t</TD>\n");
	wprintf(fd, "\t</TR>\n");
	wprintf(fd, "</TABLE>\r\n");
	httpd_www_label(fd, "EOF", 80);

	return 1;
}

/*
 * httpd_www_main, ghost index.html page
 * * fd, given fd
 * * raw, given raw http chunk 
 * * cgi, given cgi options
 */

int httpd_www_main(int fd, char *raw, char *cgi) {
	proxy_db_stats *cur_db;
	int frag_level;

	cur_db = proxy_get_stats();

	if (!cur_db) {
		httpd_www_syserr(fd, "calloc", raw);
		return 0;
	}

	httpd_start_hmenu(fd, NULL);
	httpd_end_hmenu(fd);

	raw[strlen(raw)-3] = '\0';

	httpd_www_label(fd, "Proxies Status", 17);

 	wprintf(fd, "<TABLE BORDER=\"1\" CELLPADDING=\"0\" CELLSPACING=\"0\" WIDTH=\"15%\" ALIGN=\"CENTER\" VALIGN=\"MIDDLE\">\n");

	if (cur_db->valids) {
		wprintf(fd, "\t<TR>\n");
		wprintf(fd, "\t\t<TD>Valids Proxies </TD>\n");
		wprintf(fd, "\t\t<TD ALIGN=\"CENTER\">%d</TD>\n", cur_db->valids);
		wprintf(fd, "\t</TR>\n");
		wprintf(fd, "\t<TR>\n");
	}

	if (cur_db->deads) {
		wprintf(fd, "\t\t<TD>Dead Proxies </TD>\n");
		wprintf(fd, "\t\t<TD ALIGN=\"CENTER\">%d</TD>\n", cur_db->deads);
		wprintf(fd, "\t</TR>\n");
		wprintf(fd, "\t<TR>\n");
	}

	if (cur_db->broken) {
		wprintf(fd, "\t\t<TD>Broken Proxies </TD>\n");
		wprintf(fd, "\t\t<TD ALIGN=\"CENTER\">%d</TD>\n", cur_db->broken);
		wprintf(fd, "\t</TR>\n");
		wprintf(fd, "\t<TR>\n");
	}

	wprintf(fd, "\t\t<TD>Total Proxies </TD>\n");
	wprintf(fd, "\t\t<TD ALIGN=\"CENTER\">%d</TD>\n", cur_db->valids + cur_db->deads + cur_db->unknowns + cur_db->broken);
	wprintf(fd, "\t</TR>\n");

	if (cur_db->index) {
		wprintf(fd, "\t<TR>\n");
		wprintf(fd, "\t\t<TD>Fragmentation </TD>\n");

		frag_level = 100;

		if (cur_db->holes || cur_db->deads || cur_db->broken) {
			frag_level = ((cur_db->holes + cur_db->deads + cur_db->broken)  * 100 / cur_db->index);
		}

		wprintf(fd, "\t\t<TD ALIGN=\"CENTER\">%d%%</TD>\n", frag_level);
		wprintf(fd, "\t</TR>\n");
	}

	wprintf(fd, "</TABLE>\n");

	httpd_www_label(fd, "Your Browser HTTP Headers", 45);

 	wprintf(fd, "<TABLE BORDER=\"1\" CELLPADDING=\"0\" CELLSPACING=\"0\" WIDTH=\"45%\" ALIGN=\"CENTER\" VALIGN=\"MIDDLE\">\n");
	wprintf(fd, "\t<TR>\n");
	wprintf(fd, "\t\t<TD>\n");
	wprintf(fd, "\t\t\t<PRE>%s</PRE>\n", raw);
	wprintf(fd, "\t\t</TD>\n");
	wprintf(fd, "\t</TR>\n");
	wprintf(fd, "</TABLE>\r\n");

	free(cur_db);

	return 1;
}

/*
 * httpd_www_about, ghost about.html page
 * * fd, given fd
 * * raw, given raw http chunk
 * * cgi, given cgi options
 */
 
int httpd_www_about(int fd, char *raw, char *cgi) {
	
	httpd_start_hmenu(fd, NULL);
	httpd_end_hmenu(fd);

	wprintf(fd, "<BR>\n");

	wprintf(fd, "<TABLE BORDER=\"1\" CELLPADDING=\"0\" CELLSPACING=\"0\" ALIGN=\"CENTER\" VALIGN=\"TOP\">\n");
	wprintf(fd, "<TR>\n");
	wprintf(fd, "<TD ALIGN=\"CENTER\" VALIGN=\"MIDDLE\">\n");
	wprintf(fd, "Ghost (v/%s)\n", GHOST_VERSION);
	wprintf(fd, "</TD>\n");
	wprintf(fd, "</TR>\n");
	wprintf(fd, "</TABLE>\n");	

	wprintf(fd, "<BR>\n");

	wprintf(fd, "<TABLE BORDER=\"1\" CELLPADDING=\"0\" CELLSPACING=\"0\" ALIGN=\"CENTER\" VALIGN=\"BOTTOM\">\n");
	wprintf(fd, "<TR>\n");
	wprintf(fd, "<TD ALIGN=\"CENTER\" VALIGN=\"MIDDLE\">\n");
	wprintf(fd, "<A HREF=\"http://www.ikotler.org\">http://www.ikotler.org</A>\n");
	wprintf(fd, "</TD>\n");
	wprintf(fd, "</TR>\n");
	wprintf(fd, "<TR>\n");
	wprintf(fd, "<TD ALIGN=\"CENTER\" VALIGN=\"MIDDLE\">\n");
	wprintf(fd, "<A HREF=\"mailto:ik@ikotler.org\">ik@ikotler.org</A>\n");
	wprintf(fd, "</TR>\n");
	wprintf(fd, "</TABLE>\n");		

	wprintf(fd, "<BR>\n");

	return 1;
}

/*
 * httpd_cgi_conf_compile, apply config options from alternative hashtable to runtime hashtable
 * * fd, given fd
 * * raw, given raw http chunk
 * * cgi, given cgi options
 */

int httpd_cgi_conf_compile(int fd, char *raw, char *cgi) {
	char *nval, *old_www, *new_url;
	int i;

	lprintf("[ghost/notice] compiling config options ...\n");

	if (!alt_cfg) {
		lprintf("[ghost/notice] no alternative config hashtable, aborted!\n");
		httpd_header(fd, 302, "/config.html");
		return 0;
	}

	old_www = strdup(hashtable_lookup(cfg, "http-admin-url"));

	if (!old_www) {

		lerror("calloc");
		httpd_header(fd, 302, "/config.html");
		return 0;
				
	}

	for (i = 0; cfg_truthtable[i].tag != NULL; i++) {

		if (cfg_truthtable[i].tag_mode == STATIC) {
			continue;
		}

		nval = hashtable_lookup(alt_cfg, cfg_truthtable[i].tag);

		if (nval) {

			lprintf("[ghost/notice]\t* %s -> %s\n", cfg_truthtable[i].tag, nval);
			hashtable_insert(cfg, cfg_truthtable[i].tag, nval);
			hashtable_delete(alt_cfg, cfg_truthtable[i].tag);
		}
	}

	if (!cfg_validate_all(cfg, GLITCH_MODE)) {

		httpd_www_gerror(fd, "Validation Failed!", raw, \
			"Unable to validate new configuration layout. Please refer to the <A HREF=\"/log.html\">log</A> for more details!");

		free(old_www);

		return 0;
	}

	proxy_build_limits();
	
	lprintf("[ghost/notice] new config options are now in place!\n");

	if (!strcmp(hashtable_lookup(cfg, "http-analyzer"), "off")) {

		httpd_header(fd, 200, NULL);

		httpd_msg_page(fd, "HTTP-ANALYZER", -1, NULL,
			"You have choosen to deactivate the HTTP-ANALYZER option. <BR> \n \
			Therefor the Web Interface is no longer accessable. <BR> \n \
			You can manually reactivate it by restarting the program!");

		free(old_www);
			
		return 1;
	}

	if (!strcmp(hashtable_lookup(cfg, "http-admin-mode"), "off")) {

		httpd_header(fd, 200, NULL);

		httpd_msg_page(fd, "Web Interface", -1, NULL,
			"You have choosen to deactivate this Web Interface. <BR> \n \
			You can manually reactivate it by restarting the program!");

		free(old_www);

		return 1;
	}

	new_url = hashtable_lookup(cfg, "http-admin-url");

	if (strcmp(new_url, old_www)) {

		free(old_www);

		httpd_header(fd, 302, new_url);

		return 0;
	}

	free(old_www);

	httpd_header(fd, 302, "/config.html");

	return 1;
}

/*
 * httpd_cgi_proc_conf, process POST request from previous form
 * * fd, given fd
 * * raw, given raw http chunk
 * * cgi, given cgi options
 */

int httpd_cgi_proc_conf(int fd, char *raw, char *cgi) {
	int idx;
	char *nval, *a_retval, *retval;

	nval = httpd_url_decode(cgi_parse_token(cgi, "newval"));
	idx = cgi_parse_int(cgi, "idx");

	if (!alt_cfg || !cgi || !nval || idx < 0 || cfg_truthtable[idx].tag_type == STATIC) {

		if (nval) {
			free(nval);
		}

		httpd_header(fd, 302, "/config.html");
		return 0;
	}

	if (!cfg_validate(idx, nval)) {

		httpd_www_gerror(fd, "Invalid Value", raw, "Assigned value %s for %s isn't valid nor match it's type!",
			nval, cfg_truthtable[idx].tag, cfg_truthtable[idx].tag_type);

		return 0;
	}

	retval = hashtable_lookup(cfg, cfg_truthtable[idx].tag);

	a_retval = hashtable_lookup(alt_cfg, cfg_truthtable[idx].tag);

	if (!retval) {

		if (!a_retval) {

			hashtable_insert(alt_cfg, cfg_truthtable[idx].tag, nval);

		} else {

			if (!strcmp(a_retval, nval)) {

				hashtable_delete(alt_cfg, cfg_truthtable[idx].tag);

			} else {

				hashtable_insert(alt_cfg, cfg_truthtable[idx].tag, nval);						
			}
		}

	} else {

		if (!a_retval) {

			if (strcmp(retval, nval)) {

				hashtable_insert(alt_cfg, cfg_truthtable[idx].tag, nval);

			}

		} else  {

			if (!strcmp(retval, nval)) {

				hashtable_delete(alt_cfg, cfg_truthtable[idx].tag);

			} else {

				if (strcmp(a_retval, nval)) {

					hashtable_insert(alt_cfg, cfg_truthtable[idx].tag, nval);
				}
			}
		}
	}

	free(nval);

	httpd_header(fd, 302, "/config.html");

	return 1;
}

/*
 * httpd_cgi_conf_newval, ghost new value for option cgi page
 * * fd, given fd
 * * raw, given raw http chunk 
 * * cgi, given cgi options
 */

int httpd_cgi_conf_newval(int fd, char *raw, char *cgi) {
	int idx;
	char *retval;

	httpd_start_hmenu(fd, NULL);
	httpd_end_hmenu(fd);

	idx = cgi_parse_index(fd, cgi);

	if (idx < 0){
		return 0;
	}

	if (_MAX_CONF_OPTS < idx) {
		httpd_www_label(fd, "Index out of range", 40);
		return 0;
	}

	if (cfg_truthtable[idx].tag_mode == STATIC) {

		httpd_www_label(fd, "The option you trying to change is STATIC, therefor it could only be changed through the config file", 60);

		return 0;
	}

	httpd_www_label(fd, cfg_truthtable[idx].tag, strlen(cfg_truthtable[idx].tag));

	retval = hashtable_lookup(alt_cfg, cfg_truthtable[idx].tag);

	if (!retval) {
		retval = hashtable_lookup(cfg, cfg_truthtable[idx].tag);
	}

	httpd_www_conf_dialog(fd, "/config-processor.cgi", idx, retval, "set");

	return 1;
}

/*
 * httpd_cgi_mod_proxy, modify proxy .cgi
 * * fd, given fd
 * * raw, given raw http chunk
 * * cgi, given cgi options
 */

int httpd_cgi_mod_proxy(int fd, char *raw, char *cgi) {
	int idx;
	proxy_entry *q_proxy;

	httpd_start_hmenu(fd, NULL);
	httpd_end_hmenu(fd);

	idx = cgi_parse_index(fd, cgi);

	if (idx < 0) {
		return 0;
	}

	q_proxy = proxy_querydb(idx);

	if (!q_proxy) {
		httpd_www_label(fd, "Index out of range", 40);
		return 0;
	}

	if (!q_proxy->addr) {
		httpd_www_label(fd, "Corrupted/Invalid Proxy Entry!", 40);
		return 0;
	}

	httpd_www_label(fd, "Modify Proxy", 40);

	httpd_www_proxy_dialog(fd, "/proxy-processor.cgi", idx, q_proxy->addr, q_proxy->port, "modify");

	free(q_proxy);

	return 1;
}

/*
 * httpd_cgi_conf_flipflop, change option boolean value
 * * fd, given fd
 * * raw, given raw http chunk
 * * cgi, given cgi options
 */

int httpd_cgi_conf_flipflop(int fd, char *raw, char *cgi) {
	int idx;
	char *retval, *nval, *a_retval;

	idx = cgi_parse_int(cgi, "id");

	if (!alt_cfg || !cgi || idx < 0 || _MAX_CONF_OPTS < idx) {
		httpd_header(fd, 302, "/config.html");
		return 0;
	}

	if (cfg_truthtable[idx].tag_type == IS_BOOLEAN && cfg_truthtable[idx].tag_mode == DYANMIC) {

			retval = hashtable_lookup(cfg, cfg_truthtable[idx].tag);

			if (!retval) {

				retval = "off";
			}	

			if (retval) {

				nval = "off";

				if (!strcmp(retval, "off")) {
					nval = "on";
				}

				a_retval = hashtable_lookup(alt_cfg, cfg_truthtable[idx].tag);

				if (!a_retval) {

					hashtable_insert(alt_cfg, cfg_truthtable[idx].tag, nval);

				} else  {

					if (!strcmp(a_retval, nval)) {					
						hashtable_delete(alt_cfg, cfg_truthtable[idx].tag);
					}
				}
			}
	}

	httpd_header(fd, 302, "/config.html");

	return 1;
}

/*
 * httpd_cgi_proc_proxy, process POST request from previous form
 * * fd, given fd
 * * raw, given raw http chunk
 * * cgi, given cgi options
 */

int httpd_cgi_proc_proxy(int fd, char *raw, char *cgi) {
	int proxy_idx, proxy_port, rval;
	char *proxy_addr;

	if (!cgi) {
		httpd_header(fd, 302, "/proxies.html");
		return 0;
	}
	
	proxy_idx = cgi_parse_int(cgi, "proxyidx");
	proxy_addr = cgi_parse_token(cgi, "proxyaddr");
	proxy_port = cgi_parse_int(cgi, "proxyport");

	if (!proxy_addr || proxy_port < 0) {

		free(proxy_addr);
		httpd_header(fd, 302, "/proxies.html");
		return 0;
	}

	if (proxy_idx < 0) {

		rval = proxy_add(proxy_addr, proxy_port);

	} else {

		rval = proxy_mod(proxy_idx, proxy_addr, proxy_port);
	}

	free(proxy_addr);

	httpd_header(fd, 302, "/proxies.html");

	return 1;
}

/*
 * httpd_cgi_del_proxy, delete given proxy .cgi
 * * fd, given fd
 * * raw, given raw http chunk 
 * * cgi, given cgi options
 */

int httpd_cgi_del_proxy(int fd, char *raw, char *cgi) {
	int idx = -1;

	idx = cgi_parse_int(cgi, "id");

	if (!cgi || idx < 0) {
		httpd_header(fd, 302, "/proxies.html");
		return 0;
	}	

	proxy_delete(idx);
		
	httpd_header(fd, 302, "/proxies.html");

	return 1;
}

/*
 * httpd_cgi_proxies_rebuild, rebuild proxies database
 * * fd, given fd
 * * raw, given raw http chunk 
 * * cgi, given cgi options
 */

int httpd_cgi_proxies_rebuild(int fd, char *raw, char *cgi) {
	proxy_rebuild_db();
	httpd_header(fd, 302, "/proxies.html");
	return 1;
}

/*
 * httpd_www_proxytst_dialog, dialog for proxy test w/ error handler
 * * fd, given fd
 * * retval, retval of test function
 * * tname, given test name
 * * addr, given proxy address
 * * port, given port address
 */

int httpd_www_proxytst_dialog(int fd, int retval, char *tname, char *addr, int port) {
	char *errstr, *testval;

	testval = "PASSED";
	errstr = NULL;

	if (retval < 0) {

		testval = "FAILED";
		errstr = "Connection timed out";

		if (errno != EAGAIN && errno != EWOULDBLOCK) {
			errstr = strerror(errno);
		}
	}

	wprintf(fd, "<BR>\n");
	wprintf(fd, "<TABLE BORDER=\"1\" CELLPADDING=\"0\" CELLSPACING=\"0\" ALIGN=\"CENTER\" WIDTH=\"45%\">\n");
	wprintf(fd, "\t<TR>\n");
	wprintf(fd, "\t<TD COLSPAN=\"2\" ALIGN=\"CENTER\">%s</TD>\n", tname);
	wprintf(fd, "\t</TR>\n");
	wprintf(fd, "\t<TR>\n");
	wprintf(fd, "\t<TD ALIGN=\"CENTER\">%s:%d/tcp</TD>\n", addr, port);
	wprintf(fd, "\t<TD ALIGN=\"CENTER\"><B>%s</B></TD>\n", testval);
	wprintf(fd, "\t</TR>\n");

	if (errstr) {

		wprintf(fd, "</TABLE>\n");
		wprintf(fd, "<BR>\n");
		wprintf(fd, "<BR>\n");
		wprintf(fd, "<TABLE BORDER=\"1\" CELLPADDING=\"0\" CELLSPACING=\"0\" ALIGN=\"CENTER\" WIDTH=\"45%\">\n");
		wprintf(fd, "<TR><TD ALIGN=\"CENTER\">%s</TD></TR>\n", errstr);
		wprintf(fd, "</TABLE>\n");
	}

	return retval;
}

/*
 * httpd_cgi_chk_proxy, validate given proxy
 * * fd, given fd
 * * raw, given raw http chunk 
 * * cgi, given cgi options
 */

int httpd_cgi_chk_proxy(int fd, char *raw, char *cgi) {
	int idx, retval;
	proxy_entry *cur;
	char *buf;

	httpd_start_hmenu(fd, NULL);
	httpd_end_hmenu(fd);

	idx = cgi_parse_index(fd, cgi);

	cur = proxy_querydb(idx);

	if (!cur) {

		httpd_www_err(fd, "Invalid Proxy Entry", raw, "Given index (#%d) point's to invalid entry in the proxies database!", idx);
		return 0;
	}

	retval = httpd_www_proxytst_dialog(fd, proxy_t_validate(idx), "Connectivity", cur->addr, cur->port);

	if (retval < 0) {

		free(cur);
		return 0;
	}

	wprintf(fd, "</TABLE>\n");

	close(retval);

	retval = httpd_www_proxytst_dialog(fd, proxy_t_validate_http(idx, &buf), "HTTP Validation", cur->addr, cur->port);

	if (retval < 0) {

		free(cur);
		return 0;
	}

	if (buf) {

		wprintf(fd, "\t<TR>\n");
		wprintf(fd, "\t<TD COLSPAN=\"100%\"><PRE>%s</PRE></TD>\n", buf);
		wprintf(fd, "\t</TR>\n");

		free(buf);
	}

	wprintf(fd, "</TABLE>\n");

	free(cur);

	return 1;
}

/*
 * httpd_cgi_vall_proxies, validate all proxies in the database
 * * fd, given fd
 * * raw, given raw http chunk
 * * cgi, given cgi options
 */

int httpd_cgi_vall_proxies(int fd, char *raw, char *cgi) {
	proxy_validateall();
	httpd_header(fd, 302, "/proxies.html");
	return 1;
}

/*
 * httpd_misc_filedump, file dump utility function
 * * fd, given fd
 * * raw, given raw http chunk
 * * cgi, given cgi options
 * * dumpfunc, dump function callback
 * * destfile, output file
 */

int httpd_misc_filedump(int fd, char *raw, char *cgi, int (*dumpfunc)(FILE *), char *destfile) {
	FILE *fp;
	char rand_file[256]; 

	snprintf(rand_file, sizeof(rand_file), "tmp-%ud.%ud", (int)time(NULL), rand());

	remove(rand_file);
	
	fp = fopen(rand_file, "a");

	if (!fp) {

		httpd_www_syserr(fd, rand_file, raw);
		return 0;
	}
	
	dumpfunc(fp);

	fflush(fp);
	fclose(fp);
	
	if (rename(rand_file, destfile) < 0) {

		httpd_www_syserr(fd, destfile, raw);
		remove(rand_file);
		return 0;
	}

	return 1;
}

/* 
 * httpd_cgi_dump_proxies, dump proxies into a file
 * * fd, given fd
 * * raw, given raw http chunk 
 * * cgi, given cgi options
 */

int httpd_cgi_dump_proxies(int fd, char *raw, char *cgi) {
	httpd_misc_filedump(fd, raw, cgi, proxy_dump, hashtable_lookup(cfg, "proxy-file"));
	httpd_header(fd, 302, "/proxies.html");
	return 1;
}

/* 
 * httpd_cgi_dump_config, dump config into a file
 * * fd, given fd
 * * raw, given raw http chunk 
 * * cgi, given cgi options
 */

int httpd_cgi_dump_config(int fd, char *raw, char *cgi) {
	httpd_misc_filedump(fd, raw, cgi, config_dump, hashtable_lookup(cfg, "config-file"));
	httpd_header(fd, 302, "/config.html");
	return 1;
}

/*
 * httpd_cgi_log_clean, cleanlog.cgi is bascily clean option for log.html
 * * fd, given fd
 * * raw, given raw http chunk 
 * * cgi, given cgi options
 */

int httpd_cgi_log_clean(int fd, char *raw, char *cgi) {
	int retval;

	retval = log_restart();

	if (retval < 1) {

		httpd_www_syserr(fd, hashtable_lookup(cfg, "daemon-logfile"), NULL);
	 	return retval;
	}

	httpd_header(fd, 302, "/log.html");

	return 1;
}
