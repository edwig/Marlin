#include "stdafx.h"
#include "http_private.h"

LPCTSTR http_server_error = 
  _T("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0 Transitional//EN\">\n")
  _T("<html>\n")
  _T("<head>\n")
  _T("<title>Webserver error</title>\n")
  _T("</head>\n")
  _T("<body bgcolor=\"#00FFFF\" text=\"#FF0000\">\n")
  _T("<p><font size=\"5\" face=\"Arial\"><strong>Server error: %d</strong></font></p>\n")
  _T("<p><font size=\"5\" face=\"Arial\"><strong>%s</strong></font></p>\n")
  _T("</body>\n")
  _T("</html>\n");

LPCTSTR http_client_error =
  _T("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0 Transitional//EN\">\n")
  _T("<html>\n")
  _T("<head>\n")
  _T("<title>Client error</title>\n")
  _T("</head>\n")
  _T("<body bgcolor=\"#00FFFF\" text=\"#FF0000\">\n")
  _T("<p><font size=\"5\" face=\"Arial\"><strong>Client error: %d</strong></font></p>\n")
  _T("<p><font size=\"5\" face=\"Arial\"><strong>%s</strong></font></p>\n")
  _T("</body>\n")
  _T("</html>\n");
