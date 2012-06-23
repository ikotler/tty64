/*
 * www_ext.h
 */

const char WWW_HTML_START[] = 
" \
        <!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0 Transitional//EN\"> \n \
        <HTML> \n \
        <HEAD> \n \
		<META HTTP-EQUIV=\"CONTENT-TYPE\" CONTENT=\"text/html; charset=ISO-8859-1\"> \n \
";

const char WWW_HTML_PAGE[] =
" \
		<TITLE>GHOST</TITLE> \n \
	</HEAD> \n \
	<BODY> \n \
";

const char WWW_HTML_LOGO[] = 
" \
	<TABLE BORDER=\"0\" ALIGN=\"CENTER\" VALIGN=\"MIDDLE\"> \n \
	        <TR> \n \
	                <TD> \n \
	                        <H1>Ghost</H1> \n \
	                </TD> \n \
	        </TR> \n \
	</TABLE> \n \
";

const char WWW_HTML_END[] =
" \
	</BODY> \n \
	</HTML> \n \
";

const char WWW_CSS_FILE[] = 
" \
	body { font-family: monospace; background: #000000; color: #FFFFFF; } \n \
	a:link { color:#FFFFFF; text-decoration: none; } \n \
	a:visited { color:#FFFFFF; text-decoration:none; } \n \
	a:hover { color:#404040; text-decoration:none; } \n \
	td { padding: 2px; } \n \
	table { empty-cells: hide } \n \
	.fullheight { height: 100% } \n \
	input { font-family: monospace; border:0px; background: #000000; color: #FFFFFF; } \
";

