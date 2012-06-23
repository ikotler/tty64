/*
 * www_links.h
 */

#ifndef WWW_DEFAULT_ITEMS
	#define WWW_DEFAULT_ITEMS core_menu
#endif

const char WWW_HTML_MENU_START[] =
" \
        <TABLE ALIGN=\"CENTER\" VALIGN=\"MIDDLE\"> \n \
                <TR> \n \
";

const char WWW_HTML_MENU_END[] =
" \n \
                </TR> \n \
        </TABLE> \n \
";

typedef struct www_page_menu www_page_menu;

struct www_page_menu {
	char *item;
	char *url;
};

www_page_menu core_menu[] = {
	{ "main"    	 , "/" 	           } ,
	{ "proxies" 	 , "/proxies.html" } ,
	{ "config"  	 , "/config.html"  } ,
	{ "log"     	 , "/log.html"     } ,
	{ "about"   	 , "/about.html"   } ,
	{ NULL	    	 , NULL	           }
};

www_page_menu log_menu[] = {
	{ "clean"   	  , "/cleanlog.cgi" } ,
	{ NULL      	  , NULL	    }
};

www_page_menu proxies_menu[] = {
	{ "add proxy"     , "/add-proxy.html"      } ,
	{ "rebuild db"    , "/proxies-rebuild.cgi" } ,
	{ "validate all"  , "/proxies-vall.cgi"    } ,
	{ "save to file"  , "/proxies-dump.cgi"	   } ,
	{ NULL	          , NULL		   }
};

www_page_menu config_menu[] = {
	{ "apply changes" , "/config-compile.cgi" } ,
	{ "save to file"  , "/config-dump.cgi"    } ,
	{ NULL		  , NULL		  }	
};
