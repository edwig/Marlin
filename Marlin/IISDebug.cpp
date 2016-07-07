/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: IISDebug.cpp
//
// Marlin Server: Internet server/client
// 
// Copyright (c) 2016 ir. W.E. Huisman
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
#include "Analysis.h"
#pragma warning (disable:4091)
#include <httpserv.h>

#define DETAILLOG(s1,s2,s3)    p_log->AnalysisLog(__FUNCTION__,LogType::LOG_INFO, true,"%-35s: %-25s: %s",(s1),(s2),(s3))

static void
DebugVariable(IHttpContext* p_context,LogAnalysis* p_log,char* p_description,char* p_name)
{
  DWORD size    = 1024;
  PCSTR pointer = (PCSTR)p_context->AllocateRequestMemory(size);
  HRESULT hr = p_context->GetServerVariable(p_name,&pointer,&size);
  if(SUCCEEDED(hr))
  {
    DETAILLOG(p_description,p_name,pointer);
  }
  else
  {
    DETAILLOG("Server variable not found",p_name,"");
  }
}

// Total list with all server variables
// https://msdn.microsoft.com/en-us/library/ms524602(v=vs.90).aspx
// Including those not listed here as
// HTTP_<header>
// HEADER_<header>
// UNICODE_<variable>
//
void
IISDebugAllVariables(IHttpContext* p_context,LogAnalysis* p_log)
{
  CString line("==============================================================");
  
  DETAILLOG("Listing of all IIS Server variables","","");
  DETAILLOG(line,"","");
  DebugVariable(p_context,p_log,"All headers in HTTP form",        "ALL_HTTP");
  DebugVariable(p_context,p_log,"All headers in RAW form",         "ALL_RAW");
  DebugVariable(p_context,p_log,"Application pool ID",             "APP_POOL_ID");
  DebugVariable(p_context,p_log,"Application metabase path",       "ALL_MD_PATH");
  DebugVariable(p_context,p_log,"Application physical path",       "APPL_PHYSICAL_PATH");
  DebugVariable(p_context,p_log,"Basic authentication password",   "AUTH_PASSWORD");
  DebugVariable(p_context,p_log,"Authentication method used",      "AUTH_TYPE");
  DebugVariable(p_context,p_log,"Authenticated as user",           "AUTH_USER");
  DebugVariable(p_context,p_log,"Unambiguous URL name",            "CACHE_URL");
  DebugVariable(p_context,p_log,"Unique ID for client certificate","CERT_COOKIE");
  DebugVariable(p_context,p_log,"Client cert. present flags",      "CERT_FLAGS");
  DebugVariable(p_context,p_log,"Issuer of the client certificate","CERT_ISSUER");
  DebugVariable(p_context,p_log,"SSL certificate keysize",         "CERT_KEYSIZE");
  DebugVariable(p_context,p_log,"Bits in SSL private key",         "CERT_SECRETKEYSIZE");
  DebugVariable(p_context,p_log,"Client cert. serial number",      "CERT_SERIALNUMBER");
  DebugVariable(p_context,p_log,"Issuer of the server certificate","CERT_SERVER_ISSUER");
  DebugVariable(p_context,p_log,"Subject of the server cert.",     "CERT_SERVER_SUBJECT");
  DebugVariable(p_context,p_log,"Client certificate subject",      "CERT_SUBJECT");
  DebugVariable(p_context,p_log,"Content length of HTTP payload",  "CONTENT_LENGTH");
  DebugVariable(p_context,p_log,"Content type of HTTP payload",    "CONTENT_TYPE");
  DebugVariable(p_context,p_log,"CGI Specification of the server", "GATEWAY_INTERFACE");
  DebugVariable(p_context,p_log,"Accepts encoding (, separators)", "HTTP_ACCEPT");
  DebugVariable(p_context,p_log,"Accepts encoding (no separators)","HTTP_ACCEPT_ENCODING");
  DebugVariable(p_context,p_log,"Accepted displaying language",    "HTTP_ACCEPT_LANGUAGE");
  DebugVariable(p_context,p_log,"Type of connection (keep-alive)", "HTTP_CONNECTION");
  DebugVariable(p_context,p_log,"Cookie strings",                  "HTTP_COOKIE");
  DebugVariable(p_context,p_log,"Server host",                     "HTTP_HOST");
  DebugVariable(p_context,p_log,"HTTP Method (get/post etc)",      "HTTP_METHOD");
  DebugVariable(p_context,p_log,"HTTP Referer",                    "HTTP_REFERER");
  DebugVariable(p_context,p_log,"ENCODED full url",                "HTTP_URL");
  DebugVariable(p_context,p_log,"User agent (if any)",             "HTTP_USER_AGENT");
  DebugVariable(p_context,p_log,"Version of the HTTP protocol",    "HTTP_VERSION");
  DebugVariable(p_context,p_log,"HTTPS (ON or OFF)",               "HTTPS");
  DebugVariable(p_context,p_log,"SSL bits in the keysize",         "HTTPS_KEYSIZE");
  DebugVariable(p_context,p_log,"Bits in SSL Private key",         "HTTPS_SECRETKEYSIZE");
  DebugVariable(p_context,p_log,"Issuer of server certificate",    "HTTPS_SERVER_ISSUER");
  DebugVariable(p_context,p_log,"Subject of SSL Certificate",      "HTTPS_SERVER_SUBJECT");
  DebugVariable(p_context,p_log,"IIS Server instance ID",          "INSTANCE_ID");
  DebugVariable(p_context,p_log,"IIS Instance meta path",          "INSTANCE_META_PATH");
  DebugVariable(p_context,p_log,"Local address of the server",     "LOCAL_ADDR");
  DebugVariable(p_context,p_log,"Impersonated logged on user",     "LOGON_USER");
  DebugVariable(p_context,p_log,"Absolute path (no params/anchor)","PATH_INFO");
  DebugVariable(p_context,p_log,"Path translated to machine",      "PATH_TRANSLATED");
  DebugVariable(p_context,p_log,"Query part of the URL",           "QUERY_STRING");
  DebugVariable(p_context,p_log,"Remote address making requests",  "REMOTE_ADDR");
  DebugVariable(p_context,p_log,"Remote host name",                "REMOTE_HOST");
  DebugVariable(p_context,p_log,"Remote port number",              "REMOTE_PORT");
  DebugVariable(p_context,p_log,"Remote user logging in",          "REMOTE_USER");
  DebugVariable(p_context,p_log,"HTTP Request method (get/post)",  "REQUEST_METHOD");
  DebugVariable(p_context,p_log,"Script name (absolute path)",     "SCRIPT_NAME");
  DebugVariable(p_context,p_log,"Script translated to physical",   "SCRIPT_TRANSLATED");
  DebugVariable(p_context,p_log,"Host name (DNS alias)",           "SERVER_NAME");
  DebugVariable(p_context,p_log,"Host server port number",         "SERVER_PORT");
  DebugVariable(p_context,p_log,"Secure HTTPS (0 or 1)",           "SERVER_PORT_SECURE");
  DebugVariable(p_context,p_log,"Canonical HTTP version",          "SERVER_PROTOCOL");
  DebugVariable(p_context,p_log,"Server Software/Version",         "SERVER_SOFTWARE");
  DebugVariable(p_context,p_log,"Server Side Includes Disabled",   "SSI_EXEC_DISABLED");
  DebugVariable(p_context,p_log,"Raw unencoded URL",               "UNENCODED_URL");
  DebugVariable(p_context,p_log,"Remote user (unmapped)",          "UNMAPPED_REMOTE_USER");
  DebugVariable(p_context,p_log,"Absolute path in the URL",        "URL");
  DebugVariable(p_context,p_log,"Path info (IIS 5.0)",             "URL_PATH_INFO");
  DETAILLOG(line,"","");
}
