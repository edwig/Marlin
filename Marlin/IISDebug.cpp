/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: IISDebug.cpp
//
// Marlin Server: Internet server/client
// 
// Copyright (c) 2014-2024 ir. W.E. Huisman
// All rights reserved
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
#include "stdafx.h"
#include "ServiceReporting.h"
#include "ConvertWideString.h"

// To prevent bug report from the Windows 8.1 SDK
#pragma warning (disable:4091)
#include <httpserv.h>
#pragma warning (error:4091)

#ifdef _AFX
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#endif

static XString
DebugVariable(IHttpContext* p_context,LPCTSTR p_description,const char* p_name)
{
  XString message;
  DWORD size    = 1024;
  PCSTR pointer = reinterpret_cast<PCSTR>(p_context->AllocateRequestMemory(size));
  HRESULT hr    = p_context->GetServerVariable(p_name,&pointer,&size);

  XString name  = LPCSTRToString(p_name);
  XString value = LPCSTRToString(pointer);
  if(SUCCEEDED(hr))
  {
    message.Format(_T("%s (%s): %s\n"),p_description,name.GetString(),value.GetString());
  }
  else
  {
    message.Format(_T("Server variable not found: %s\n"),name.GetString());
  }
  return message;
}

// Total list with all server variables
// https://msdn.microsoft.com/en-us/library/ms524602(v=vs.90).aspx
// Including those not listed here as
// HTTP_<header>
// HEADER_<header>
// UNICODE_<variable>
//
void
IISDebugAllVariables(IHttpContext* p_context)
{
  XString line(_T("=============================================================="));
  
  XString message(_T("Listing of all IIS Server variables"));
  message += line;
  message += DebugVariable(p_context,_T("All headers in HTTP form"),        "ALL_HTTP");
  message += DebugVariable(p_context,_T("All headers in RAW form"),         "ALL_RAW");
  message += DebugVariable(p_context,_T("Application pool ID"),             "APP_POOL_ID");
  message += DebugVariable(p_context,_T("Application metabase path"),       "ALL_MD_PATH");
  message += DebugVariable(p_context,_T("Application physical path"),       "APPL_PHYSICAL_PATH");
  message += DebugVariable(p_context,_T("Basic authentication password"),   "AUTH_PASSWORD");
  message += DebugVariable(p_context,_T("Authentication method used"),      "AUTH_TYPE");
  message += DebugVariable(p_context,_T("Authenticated as user"),           "AUTH_USER");
  message += DebugVariable(p_context,_T("Unambiguous URL name"),            "CACHE_URL");
  message += DebugVariable(p_context,_T("Unique ID for client certificate"),"CERT_COOKIE");
  message += DebugVariable(p_context,_T("Client cert. present flags"),      "CERT_FLAGS");
  message += DebugVariable(p_context,_T("Issuer of the client certificate"),"CERT_ISSUER");
  message += DebugVariable(p_context,_T("SSL certificate keysize"),         "CERT_KEYSIZE");
  message += DebugVariable(p_context,_T("Bits in SSL private key"),         "CERT_SECRETKEYSIZE");
  message += DebugVariable(p_context,_T("Client cert. serial number"),      "CERT_SERIALNUMBER");
  message += DebugVariable(p_context,_T("Issuer of the server certificate"),"CERT_SERVER_ISSUER");
  message += DebugVariable(p_context,_T("Subject of the server cert."),     "CERT_SERVER_SUBJECT");
  message += DebugVariable(p_context,_T("Client certificate subject"),      "CERT_SUBJECT");
  message += DebugVariable(p_context,_T("Content length of HTTP payload"),  "CONTENT_LENGTH");
  message += DebugVariable(p_context,_T("Content type of HTTP payload"),    "CONTENT_TYPE");
  message += DebugVariable(p_context,_T("CGI Specification of the server"), "GATEWAY_INTERFACE");
  message += DebugVariable(p_context,_T("Accepts encoding (, separators)"), "HTTP_ACCEPT");
  message += DebugVariable(p_context,_T("Accepts encoding (no separators)"),"HTTP_ACCEPT_ENCODING");
  message += DebugVariable(p_context,_T("Accepted displaying language"),    "HTTP_ACCEPT_LANGUAGE");
  message += DebugVariable(p_context,_T("Type of connection (keep-alive)"), "HTTP_CONNECTION");
  message += DebugVariable(p_context,_T("Cookie strings"),                  "HTTP_COOKIE");
  message += DebugVariable(p_context,_T("Server host"),                     "HTTP_HOST");
  message += DebugVariable(p_context,_T("HTTP Method (get/post etc)"),      "HTTP_METHOD");
  message += DebugVariable(p_context,_T("HTTP Referer"),                    "HTTP_REFERER");
  message += DebugVariable(p_context,_T("ENCODED full url"),                "HTTP_URL");
  message += DebugVariable(p_context,_T("User agent (if any)"),             "HTTP_USER_AGENT");
  message += DebugVariable(p_context,_T("Version of the HTTP protocol"),    "HTTP_VERSION");
  message += DebugVariable(p_context,_T("HTTPS (ON or OFF)"),               "HTTPS");
  message += DebugVariable(p_context,_T("SSL bits in the keysize"),         "HTTPS_KEYSIZE");
  message += DebugVariable(p_context,_T("Bits in SSL Private key"),         "HTTPS_SECRETKEYSIZE");
  message += DebugVariable(p_context,_T("Issuer of server certificate"),    "HTTPS_SERVER_ISSUER");
  message += DebugVariable(p_context,_T("Subject of SSL Certificate"),      "HTTPS_SERVER_SUBJECT");
  message += DebugVariable(p_context,_T("IIS Server instance ID"),          "INSTANCE_ID");
  message += DebugVariable(p_context,_T("IIS Instance meta path"),          "INSTANCE_META_PATH");
  message += DebugVariable(p_context,_T("Local address of the server"),     "LOCAL_ADDR");
  message += DebugVariable(p_context,_T("Impersonated logged on user"),     "LOGON_USER");
  message += DebugVariable(p_context,_T("Absolute path (no params/anchor)"),"PATH_INFO");
  message += DebugVariable(p_context,_T("Path translated to machine"),      "PATH_TRANSLATED");
  message += DebugVariable(p_context,_T("Query part of the URL"),           "QUERY_STRING");
  message += DebugVariable(p_context,_T("Remote address making requests"),  "REMOTE_ADDR");
  message += DebugVariable(p_context,_T("Remote host name"),                "REMOTE_HOST");
  message += DebugVariable(p_context,_T("Remote port number"),              "REMOTE_PORT");
  message += DebugVariable(p_context,_T("Remote user logging in"),          "REMOTE_USER");
  message += DebugVariable(p_context,_T("HTTP Request method (get/post)"),  "REQUEST_METHOD");
  message += DebugVariable(p_context,_T("Script name (absolute path)"),     "SCRIPT_NAME");
  message += DebugVariable(p_context,_T("Script translated to physical"),   "SCRIPT_TRANSLATED");
  message += DebugVariable(p_context,_T("Host name (DNS alias)"),           "SERVER_NAME");
  message += DebugVariable(p_context,_T("Host server port number"),         "SERVER_PORT");
  message += DebugVariable(p_context,_T("Secure HTTPS (0 or 1)"),           "SERVER_PORT_SECURE");
  message += DebugVariable(p_context,_T("Canonical HTTP version"),          "SERVER_PROTOCOL");
  message += DebugVariable(p_context,_T("Server Software/Version"),         "SERVER_SOFTWARE");
  message += DebugVariable(p_context,_T("Server Side Includes Disabled"),   "SSI_EXEC_DISABLED");
  message += DebugVariable(p_context,_T("Raw unencoded URL"),               "UNENCODED_URL");
  message += DebugVariable(p_context,_T("Remote user (unmapped)"),          "UNMAPPED_REMOTE_USER");
  message += DebugVariable(p_context,_T("Absolute path in the URL"),        "URL");
  message += DebugVariable(p_context,_T("Path info (IIS 5.0)"),             "URL_PATH_INFO");
  message += line;

  SvcReportInfoEvent(false,message.GetString());
}
