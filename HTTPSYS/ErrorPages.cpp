#include "stdafx.h"
#include "http_private.h"

const char* http_server_error = 
  "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0 Transitional//EN\">\n"
  "<html>\n"
  "<head>\n"
  "<title>Webserver error</title>\n"
  "</head>\n"
  "<body bgcolor=\"#00FFFF\" text=\"#FF0000\">\n"
  "<p><font size=\"5\" face=\"Arial\"><strong>Server error: %d</strong></font></p>\n"
  "<p><font size=\"5\" face=\"Arial\"><strong>%s</strong></font></p>\n"
  "</body>\n"
  "</html>\n";

const char* http_client_error =
  "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0 Transitional//EN\">\n"
  "<html>\n"
  "<head>\n"
  "<title>Client error</title>\n"
  "</head>\n"
  "<body bgcolor=\"#00FFFF\" text=\"#FF0000\">\n"
  "<p><font size=\"5\" face=\"Arial\"><strong>Client error: %d</strong></font></p>\n"
  "<p><font size=\"5\" face=\"Arial\"><strong>%s</strong></font></p>\n"
  "</body>\n"
  "</html>\n";
