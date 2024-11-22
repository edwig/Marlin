/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: HTTPClientTracing.cpp
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
#include "HTTPClient.h"
#include "LogAnalysis.h"
#include "HTTPClientTracing.h"
#include "HTTPCertificate.h"
#include <wincrypt.h>

#ifdef _AFX
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#endif

#undef  TRACE
#define TRACE(text,...) m_log->AnalysisLog(_T(__FUNCTION__),LogType::LOG_TRACE,true,(text),__VA_ARGS__)

HTTPClientTracing::HTTPClientTracing(HTTPClient* p_client)
                  :m_client(p_client)
{
  m_log = p_client->GetLogging();
}

HTTPClientTracing::~HTTPClientTracing()
{
}

// DO ALL THE TRACING
void 
HTTPClientTracing::Trace(LPCTSTR p_when,HINTERNET p_session,HINTERNET p_request)
{
  // See if we can log our tracing work to the log file
  if(m_log == nullptr || m_log->GetLogLevel() < HLL_TRACE)
  {
    return;
  }
  // General variables
  BOOL  theBool = FALSE;
  DWORD theWord = 0L;
  void* thePointer = nullptr;

  TRACE(_T("NOW TRACING THE REQUEST: %s"),p_when);

  // OPTION 1
  if(QueryWord(p_request,WINHTTP_OPTION_AUTOLOGON_POLICY,&theWord,_T("WINHTTP_OPTION_AUTOLOGON_POLICY")))
  {
    // 0,1,2 = WINHTTP_AUTOLOGON_SECURITY_LEVEL_HIGH
    TRACE(_T("WinHTTP auto login policy set to: %d"),theWord);
  }
  // OPTION 2: Callback function set
  if(QueryVoid(p_request,WINHTTP_OPTION_CALLBACK,&thePointer,_T("WINHTTP_OPTION_CALLBACK")))
  {
    TRACE(_T("WinHTTP callback function set to address: %lX"),thePointer);
  }
  // OPTION 3: connection retries
  if(QueryWord(p_request,WINHTTP_OPTION_CONNECT_RETRIES,&theWord,_T("WINHTTP_OPTION_CONNECT_RETRIES")))
  {
    TRACE(_T("WinHTTP request retries: %d"),theWord);
  }
  // OPTION 4: Tracing on or off
  if(QueryBool(nullptr,WINHTTP_OPTION_ENABLETRACING,&theBool,_T("WINHTTP_OPTION_ENABLETRACING")))
  {
    TRACE(_T("WinHTTP tracing set to: %s"),theBool ? _T("on") : _T("off"));
  }
  // OPTION 5: Extended sockets error
  if(QueryWord(nullptr,WINHTTP_OPTION_EXTENDED_ERROR,&theWord,_T("WINHTTP_OPTION_EXTENDED_ERROR")))
  {
    TRACE(_T("WinHTTP Extended sockets error: %d"),theWord);
  }
  // OPTION 6: getting the version
  HTTP_VERSION_INFO version;
  memset(&version,0,sizeof(HTTP_VERSION_INFO));
  if(QueryObject(p_session,WINHTTP_OPTION_HTTP_VERSION,&version,sizeof(HTTP_VERSION_INFO),_T("WINHTTP_OPTION_HTTP_VERSION")))
  {
    TRACE(_T("WinHTTP Version is: %d:%d"),version.dwMajorVersion,version.dwMinorVersion);
  }
  // OPTION 7: Proxy response found
  if(QueryBool(p_request,WINHTTP_OPTION_IS_PROXY_CONNECT_RESPONSE,&theBool,_T("WINHTTP_OPTION_IS_PROXY_CONNECT_RESPONSE")))
  {
    TRACE(_T("WinHTTP proxy is response can be retrieved: %s"),theBool ? _T("yes") : _T("no"));
  }
  // OPTION 8: Maximum number of connections/server
  if(QueryWord(p_session,WINHTTP_OPTION_MAX_CONNS_PER_SERVER,&theWord,_T("WINHTTP_OPTION_MAX_CONNS_PER_SERVER")))
  {
    TRACE(_T("WinHTTP allows connections per server: %d"),theWord);
  }
  // OPTION 9: Maximum number of redirects
  if(QueryWord(p_request,WINHTTP_OPTION_MAX_HTTP_AUTOMATIC_REDIRECTS,&theWord,_T("WINHTTP_OPTION_MAX_HTTP_AUTOMATIC_REDIRECTS")))
  {
    TRACE(_T("WinHTTP allows a maximum number of redirects of: %d"),theWord);
  }
  // OPTION 10: Maximum number of HTTP_STATUS_CONTINUE
  if(QueryWord(p_request,WINHTTP_OPTION_MAX_HTTP_STATUS_CONTINUE,&theWord,_T("WINHTTP_OPTION_MAX_HTTP_STATUS_CONTINUE")))
  {
    TRACE(_T("WinHTTP allows a maximum number of STATUS_CONTINUE responses: %d"),theWord);
  }
  // OPTION 11: Drain size
  if(QueryWord(p_request,WINHTTP_OPTION_MAX_RESPONSE_DRAIN_SIZE,&theWord,_T("WINHTTP_OPTION_MAX_RESPONSE_DRAIN_SIZE")))
  {
    TRACE(_T("WinHTTP will drain the connection after [%d] bytes"),theWord);
  }
  // OPTION 12: Max header size
  if(QueryWord(p_request,WINHTTP_OPTION_MAX_RESPONSE_HEADER_SIZE,&theWord,_T("WINHTTP_OPTION_MAX_RESPONSE_HEADER_SIZE")))
  {
    TRACE(_T("WinHTTP allows for a request header size up to a total of [%d] bytes"),theWord);
  }
  // OPTION 13: Response header size
  if(QueryWord(p_request,WINHTTP_OPTION_MAX_RESPONSE_HEADER_SIZE,&theWord,_T("WINHTTP_OPTION_MAX_RESPONSE_HEADER_SIZE")))
  {
    TRACE(_T("WinHTTP allows for a response header size up to a total of [%d] bytes"),theWord);
  }
  // OPTION 14: Co-branding text
  XString cobrandingText = QueryString(p_request,WINHTTP_OPTION_PASSPORT_COBRANDING_TEXT,_T("WINHTTP_OPTION_PASSPORT_COBRANDING_TEXT"));
  TRACE(_T("Microsoft Passport cobranding text: %s"),cobrandingText.GetString());
  // OPTION 15: Co-branding URL
  XString cobrandingUrl  = QueryString(p_request,WINHTTP_OPTION_PASSPORT_COBRANDING_URL,_T("WINHTTP_OPTION_PASSPORT_COBRANDING_URL"));
  TRACE(_T("Microsoft Passport cobranding URL: %s"),cobrandingUrl.GetString());
  // OPTION 16: Co-branding returning URL
  XString cobrandingRetUrl = QueryString(p_request,WINHTTP_OPTION_PASSPORT_RETURN_URL,_T("WINHTTP_OPTION_PASSPORT_RETURN_URL"));
  TRACE(_T("Microsoft Passport returning  URL: %s"),cobrandingRetUrl.GetString());
  // OPTION 17: User name
  XString user = QueryString(p_request,WINHTTP_OPTION_USERNAME,_T("WINHTTP_OPTION_USERNAME"));
  TRACE(_T("WinHTTP uses the user name: %s"),user.GetString());
  // OTPION 18: Password
  XString password = QueryString(p_request,WINHTTP_OPTION_PASSWORD,_T("WINHTTP_OPTION_PASSWORD"));
  TRACE(_T("Users password: %s"),password.GetString());
  // OPTION 19: The agent name
  XString agent = QueryString(p_session,WINHTTP_OPTION_USER_AGENT,_T("WINHTTP_OPTION_USER_AGENT"));
  TRACE(_T("WinHTTP uses the agent name: %s"),agent.GetString());
  // OPTION 20: Configured URL
  XString url = QueryString(p_request,WINHTTP_OPTION_URL,_T("WINHTTP_OPTION_URL"));
  TRACE(_T("WinHTTP uses the configured URL: %s"),url.GetString());
  // OPTION 21: Read buffer size
  if(QueryWord(p_request,WINHTTP_OPTION_READ_BUFFER_SIZE,&theWord,_T("WINHTTP_OPTION_READ_BUFFER_SIZE")))
  {
    TRACE(_T("WInHTTP uses a reading buffer size of: %d"),theWord);  
  }
  // OPTION 22 : Writing buffer size
  if(QueryWord(p_request,WINHTTP_OPTION_WRITE_BUFFER_SIZE,&theWord,_T("WINHTTP_OPTION_WRITE_BUFFER_SIZE")))
  {
    TRACE(_T("WinHTTP uses a writing buffer size of: %d"),theWord);
  }
  // OPTION 23: Proxy used
  WINHTTP_PROXY_INFO proxy;
  memset(&proxy,0,sizeof(WINHTTP_PROXY_INFO));
  if(QueryObject(p_session,WINHTTP_OPTION_PROXY,&proxy,sizeof(WINHTTP_PROXY_INFO),_T("WINHTTP_OPTION_PROXY")))
  {
    XString type;
    switch(proxy.dwAccessType)
    {
      case WINHTTP_ACCESS_TYPE_DEFAULT_PROXY:  type = _T("Default proxy");  break;
      case WINHTTP_ACCESS_TYPE_NO_PROXY:       type = _T("No proxy");       break;
      case WINHTTP_ACCESS_TYPE_NAMED_PROXY:    type = _T("Named proxy");    break;
      case WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY:type = _T("Automatic proxy");break;
    }
    TRACE(_T("WinHTTP Uses proxy type: %s"),  type.GetString());
    TRACE(_T("WinHTTP uses proxy list: %s"),  proxy.lpszProxy);
    TRACE(_T("WinHTTP uses proxy bypass list: %s"),proxy.lpszProxyBypass);
  }
  // OPTION 24: Proxy user
  XString proxyUser = QueryString(p_request,WINHTTP_OPTION_PROXY_USERNAME,_T("WINHTTP_OPTION_PROXY_USERNAME"));
  TRACE(_T("WinHTTP uses proxy user: %s"),proxyUser.GetString());
  // OPTION 25: Proxy password
  XString proxyPassword = QueryString(p_request,WINHTTP_OPTION_PROXY_PASSWORD,_T("WINHTTP_OPTION_PROXY_PASSWORD"));
  TRACE(_T("WinHTTP uses proxy password: %s"),proxyPassword.GetString());
  // OPTION 26: Proxies principal server name
  XString proxyServer = QueryString(p_request,WINHTTP_OPTION_PROXY_SPN_USED,_T("WINHTTP_OPTION_PROXY_SPN_USED"));
  TRACE(_T("WinHTTP proxy's principal server name: %s"),proxyServer.GetString());
  // OPTION 27: Resolve timeout
  if(QueryWord(p_request,WINHTTP_OPTION_RESOLVE_TIMEOUT,&theWord,_T("WINHTTP_OPTION_RESOLVE_TIMEOUT")))
  {
    TRACE(_T("WinHTTP resolve timeout milliseconds: %d"),theWord);
  }
  // OPTION 28: Connection timeout
  if(QueryWord(p_request,WINHTTP_OPTION_CONNECT_TIMEOUT,&theWord,_T("WINHTTP_OPTION_CONNECT_TIMEOUT")))
  {
    TRACE(_T("WinHTTP connection timeout milliseconds: %d"),theWord);
  }
  // OPTION 29: Send timeout
  if(QueryWord(p_request,WINHTTP_OPTION_SEND_TIMEOUT,&theWord,_T("WINHTTP_OPTION_SEND_TIMEOUT")))
  {
    TRACE(_T("WinHTTP send timeout milliseconds: %d"),theWord);
  }
  // OPTION 30: Response timeout
  if(QueryWord(p_request,WINHTTP_OPTION_RECEIVE_RESPONSE_TIMEOUT,&theWord,_T("WINHTTP_OPTION_RECEIVE_RESPONSE_TIMEOUT")))
  {
    TRACE(_T("WinHTTP response timeout milliseconds: %d"),theWord);
  }
  // OPTION 31: Receive timeout
  if(QueryWord(p_request,WINHTTP_OPTION_RECEIVE_TIMEOUT,&theWord,_T("WINHTTP_OPTION_RECEIVE_TIMEOUT")))
  {
    TRACE(_T("WinHTTP receive timeout milliseconds: %d"),theWord);
  }
  // OPTION 32: Redirect policy
  if(QueryWord(p_request,WINHTTP_OPTION_REDIRECT_POLICY,&theWord,_T("WINHTTP_OPTION_REDIRECT_POLICY")))
  {
    XString policy;
    switch(theWord)
    {
      case WINHTTP_OPTION_REDIRECT_POLICY_ALWAYS: policy = _T("Redirect always"); break;
      case WINHTTP_OPTION_REDIRECT_POLICY_NEVER:  policy = _T("Never redirect");  break;
      case WINHTTP_OPTION_REDIRECT_POLICY_DISALLOW_HTTPS_TO_HTTP: policy = _T("Disallow https to http redirection"); break;
    }
    TRACE(_T("WinHTTP uses a redirection policy: %s"),policy.GetString());
  }
  // OPTION 33: Secure options
  if(QueryWord(p_request,WINHTTP_OPTION_SECURITY_FLAGS,&theWord,_T("WINHTTP_OPTION_SECURITY_FLAGS")))
  {
    if(theWord & SECURITY_FLAG_IGNORE_CERT_CN_INVALID)   TRACE(_T("WinHTTP ignores invalid common names in certificates"));
    if(theWord & SECURITY_FLAG_IGNORE_CERT_DATE_INVALID) TRACE(_T("WinHTTP ignores invalid dates in certificates"));
    if(theWord & SECURITY_FLAG_IGNORE_UNKNOWN_CA)        TRACE(_T("WinHTTP ignores invalid certifcate authorities"));
    if(theWord & SECURITY_FLAG_IGNORE_CERT_WRONG_USAGE)  TRACE(_T("WinHTTP ignores wrong usages of certificates"));
    if(theWord & SECURITY_FLAG_SECURE)                   TRACE(_T("WinHTTP in secure transfer modus"));
    if(theWord & SECURITY_FLAG_STRENGTH_MEDIUM)          TRACE(_T("WinHTTP uses 56 bits encryption"));
    if(theWord & SECURITY_FLAG_STRENGTH_STRONG)          TRACE(_T("WinHTTP uses 128 bits encryption"));
    if(theWord & SECURITY_FLAG_STRENGTH_WEAK)            TRACE(_T("WinHTTP uses 40 bits encryption"));
  }
  // OPTION 34: Key strength
  if(QueryWord(p_request,WINHTTP_OPTION_SECURITY_KEY_BITNESS,&theWord,_T("WINHTTP_OPTION_SECURITY_KEY_BITNESS")))
  {
    TRACE(_T("WinHTTP uses encryption key of length [%d] bits"),theWord);
  }
  // OPTION 35: Server Principal name used
  XString serverSPN = QueryString(p_request,WINHTTP_OPTION_SERVER_SPN_USED,_T("WINHTTP_OPTION_SERVER_SPN_USED"));
  TRACE(_T("WinHTTP used server principal name: %s"),serverSPN.GetString());

  // OPTION 36: Secure Server Certificate
  PCERT_CONTEXT context = NULL;
  if(QueryObject(p_request,WINHTTP_OPTION_SERVER_CERT_CONTEXT,&context,sizeof(PCERT_CONTEXT),_T("WINHTTP_OPTION_SERVER_CERT_CONTEXT")))
  {
    HTTPCertificate certificate(context->pbCertEncoded,context->cbCertEncoded);
    XString subject = certificate.GetSubject();
    XString issuer  = certificate.GetIssuer();
    TRACE(_T("WinHTTP uses SSL certificate issuer/subject: %s/%s"),issuer.GetString(),subject.GetString());

    CertFreeCertificateContext(context);
  }
}

//////////////////////////////////////////////////////////////////////////
//
// PRIVATE QUERY METHODS
//
//////////////////////////////////////////////////////////////////////////

bool 
HTTPClientTracing::QueryBool(HINTERNET p_handle,DWORD p_option,BOOL* p_bool,const TCHAR* p_optionName)
{
  DWORD bufLength = sizeof(BOOL);
  *p_bool = FALSE;
  BOOL  result = WinHttpQueryOption(p_handle,p_option,p_bool,&bufLength);
  if(result == FALSE)
  {
    m_log->AnalysisLog(_T("Trace"),LogType::LOG_TRACE,true,_T("Cannot get the option name: %s"),p_optionName);
  }
  return (result == TRUE);
}

bool 
HTTPClientTracing::QueryWord(HINTERNET p_handle,DWORD p_option,DWORD* p_word,const TCHAR* p_optionName)
{
  DWORD bufLength = sizeof(DWORD);
  *p_word = 0L;
  BOOL  result = WinHttpQueryOption(p_handle,p_option,p_word,&bufLength);
  if(result == FALSE)
  {
    m_log->AnalysisLog(_T("Trace"),LogType::LOG_TRACE,true,_T("Cannot get the option name: %s"),p_optionName);
  }
  return (result == TRUE);
}

bool 
HTTPClientTracing::QueryVoid(HINTERNET p_handle,DWORD p_option,void** p_pointer,const TCHAR* p_optionName)
{
  DWORD bufLength = sizeof(DWORD_PTR);
  *p_pointer = nullptr;
  BOOL  result = WinHttpQueryOption(p_handle,p_option,p_pointer,&bufLength);
  if(result == FALSE)
  {
    m_log->AnalysisLog(_T("Trace"),LogType::LOG_TRACE,true,_T("Cannot get the option name: %s"),p_optionName);
  }
  return (result == TRUE);
}

bool
HTTPClientTracing::QueryObject(HINTERNET p_handle,DWORD p_option,void* p_pointer,DWORD p_size,const TCHAR* p_optionName)
{
  DWORD bufLength = p_size;
  BOOL  result = WinHttpQueryOption(p_handle,p_option,p_pointer,&bufLength);
  if(result == FALSE)
  {
    m_log->AnalysisLog(_T("Trace"),LogType::LOG_TRACE,true,_T("Cannot get the option name: %s"),p_optionName);
  }
  if(bufLength != p_size)
  {
    m_log->AnalysisLog(_T("Trace"),LogType::LOG_TRACE,true,_T("Panic: internal object sizes in MS-Windows changed!"));
  }
  return (result == TRUE);
}

XString
HTTPClientTracing::QueryString(HINTERNET p_handle,DWORD p_option,const TCHAR* p_optionName)
{
  DWORD bufLength = 0L;
  XString theString;
  bool result = WinHttpQueryOption(p_handle,p_option,&theString,&bufLength);
  if(result == false && GetLastError() == ERROR_INSUFFICIENT_BUFFER)
  {
    wchar_t* name = new wchar_t[(size_t)bufLength + 1];
    result = WinHttpQueryOption(p_handle,p_option,name,&bufLength);
    if(result == TRUE)
    {
      theString = WStringToString(name);
    }
    delete[] name;
  }
  if(result == FALSE)
  {
    m_log->AnalysisLog(_T("Trace"),LogType::LOG_TRACE,true,_T("Cannot get the option name: %s"),p_optionName);
  }
  return theString;
}
