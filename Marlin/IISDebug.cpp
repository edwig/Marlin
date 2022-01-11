/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: IISDebug.cpp
//
// Marlin Server: Internet server/client
// 
// Copyright (c) 2014-2022 ir. W.E. Huisman
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

// To prevent bug report from the Windows 8.1 SDK
#pragma warning (disable:4091)
#include <httpserv.h>
#pragma warning (error:4091)

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static CString
DebugVariable(IHttpContext* p_context,const char* p_description,const char* p_name)
{
  CString message;
  DWORD size    = 1024;
  PCSTR pointer = (PCSTR)p_context->AllocateRequestMemory(size);
  HRESULT hr = p_context->GetServerVariable(p_name,&pointer,&size);
  if(SUCCEEDED(hr))
  {
    message.Format("%s (%s): %s\n",p_description,p_name,pointer);
  }
  else
  {
    message.Format("Server variable not found: %s\n",p_name);
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
  CString line("==============================================================");
  
  CString message("Listing of all IIS Server variables");
  message += line;
  message += DebugVariable(p_context,"All headers in HTTP form",        "ALL_HTTP");
  message += DebugVariable(p_context,"All headers in RAW form",         "ALL_RAW");
  message += DebugVariable(p_context,"Application pool ID",             "APP_POOL_ID");
  message += DebugVariable(p_context,"Application metabase path",       "ALL_MD_PATH");
  message += DebugVariable(p_context,"Application physical path",       "APPL_PHYSICAL_PATH");
  message += DebugVariable(p_context,"Basic authentication password",   "AUTH_PASSWORD");
  message += DebugVariable(p_context,"Authentication method used",      "AUTH_TYPE");
  message += DebugVariable(p_context,"Authenticated as user",           "AUTH_USER");
  message += DebugVariable(p_context,"Unambiguous URL name",            "CACHE_URL");
  message += DebugVariable(p_context,"Unique ID for client certificate","CERT_COOKIE");
  message += DebugVariable(p_context,"Client cert. present flags",      "CERT_FLAGS");
  message += DebugVariable(p_context,"Issuer of the client certificate","CERT_ISSUER");
  message += DebugVariable(p_context,"SSL certificate keysize",         "CERT_KEYSIZE");
  message += DebugVariable(p_context,"Bits in SSL private key",         "CERT_SECRETKEYSIZE");
  message += DebugVariable(p_context,"Client cert. serial number",      "CERT_SERIALNUMBER");
  message += DebugVariable(p_context,"Issuer of the server certificate","CERT_SERVER_ISSUER");
  message += DebugVariable(p_context,"Subject of the server cert.",     "CERT_SERVER_SUBJECT");
  message += DebugVariable(p_context,"Client certificate subject",      "CERT_SUBJECT");
  message += DebugVariable(p_context,"Content length of HTTP payload",  "CONTENT_LENGTH");
  message += DebugVariable(p_context,"Content type of HTTP payload",    "CONTENT_TYPE");
  message += DebugVariable(p_context,"CGI Specification of the server", "GATEWAY_INTERFACE");
  message += DebugVariable(p_context,"Accepts encoding (, separators)", "HTTP_ACCEPT");
  message += DebugVariable(p_context,"Accepts encoding (no separators)","HTTP_ACCEPT_ENCODING");
  message += DebugVariable(p_context,"Accepted displaying language",    "HTTP_ACCEPT_LANGUAGE");
  message += DebugVariable(p_context,"Type of connection (keep-alive)", "HTTP_CONNECTION");
  message += DebugVariable(p_context,"Cookie strings",                  "HTTP_COOKIE");
  message += DebugVariable(p_context,"Server host",                     "HTTP_HOST");
  message += DebugVariable(p_context,"HTTP Method (get/post etc)",      "HTTP_METHOD");
  message += DebugVariable(p_context,"HTTP Referer",                    "HTTP_REFERER");
  message += DebugVariable(p_context,"ENCODED full url",                "HTTP_URL");
  message += DebugVariable(p_context,"User agent (if any)",             "HTTP_USER_AGENT");
  message += DebugVariable(p_context,"Version of the HTTP protocol",    "HTTP_VERSION");
  message += DebugVariable(p_context,"HTTPS (ON or OFF)",               "HTTPS");
  message += DebugVariable(p_context,"SSL bits in the keysize",         "HTTPS_KEYSIZE");
  message += DebugVariable(p_context,"Bits in SSL Private key",         "HTTPS_SECRETKEYSIZE");
  message += DebugVariable(p_context,"Issuer of server certificate",    "HTTPS_SERVER_ISSUER");
  message += DebugVariable(p_context,"Subject of SSL Certificate",      "HTTPS_SERVER_SUBJECT");
  message += DebugVariable(p_context,"IIS Server instance ID",          "INSTANCE_ID");
  message += DebugVariable(p_context,"IIS Instance meta path",          "INSTANCE_META_PATH");
  message += DebugVariable(p_context,"Local address of the server",     "LOCAL_ADDR");
  message += DebugVariable(p_context,"Impersonated logged on user",     "LOGON_USER");
  message += DebugVariable(p_context,"Absolute path (no params/anchor)","PATH_INFO");
  message += DebugVariable(p_context,"Path translated to machine",      "PATH_TRANSLATED");
  message += DebugVariable(p_context,"Query part of the URL",           "QUERY_STRING");
  message += DebugVariable(p_context,"Remote address making requests",  "REMOTE_ADDR");
  message += DebugVariable(p_context,"Remote host name",                "REMOTE_HOST");
  message += DebugVariable(p_context,"Remote port number",              "REMOTE_PORT");
  message += DebugVariable(p_context,"Remote user logging in",          "REMOTE_USER");
  message += DebugVariable(p_context,"HTTP Request method (get/post)",  "REQUEST_METHOD");
  message += DebugVariable(p_context,"Script name (absolute path)",     "SCRIPT_NAME");
  message += DebugVariable(p_context,"Script translated to physical",   "SCRIPT_TRANSLATED");
  message += DebugVariable(p_context,"Host name (DNS alias)",           "SERVER_NAME");
  message += DebugVariable(p_context,"Host server port number",         "SERVER_PORT");
  message += DebugVariable(p_context,"Secure HTTPS (0 or 1)",           "SERVER_PORT_SECURE");
  message += DebugVariable(p_context,"Canonical HTTP version",          "SERVER_PROTOCOL");
  message += DebugVariable(p_context,"Server Software/Version",         "SERVER_SOFTWARE");
  message += DebugVariable(p_context,"Server Side Includes Disabled",   "SSI_EXEC_DISABLED");
  message += DebugVariable(p_context,"Raw unencoded URL",               "UNENCODED_URL");
  message += DebugVariable(p_context,"Remote user (unmapped)",          "UNMAPPED_REMOTE_USER");
  message += DebugVariable(p_context,"Absolute path in the URL",        "URL");
  message += DebugVariable(p_context,"Path info (IIS 5.0)",             "URL_PATH_INFO");
  message += line;

  SvcReportInfoEvent(false,message.GetString());
}
