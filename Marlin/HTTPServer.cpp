/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: HTTPServer.cpp
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
//////////////////////////////////////////////////////////////////////////
//
// MarlinServer using WinHTTP API 2.0
//
//////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "HTTPServer.h"
#include "HTTPMessage.h"
#include "SOAPMessage.h"
#include "HTTPURLGroup.h"
#include "HTTPCertificate.h"
#include "HTTPRequest.h"
#include "HTTPError.h"
#include "WebServiceServer.h"
#include "WebSocket.h"
#include "ThreadPool.h"
#include "ConvertWideString.h"
#include "GetLastErrorAsString.h"
#include "LogAnalysis.h"
#include "AutoCritical.h"
#include "PrintToken.h"
#include "Cookie.h"
#include "Crypto.h"
#include <WinFile.h>
#include <ServiceReporting.h>
#include <algorithm>
#include <io.h>
#include <sys/timeb.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//////////////////////////////////////////////////////////////////////////
//
// ERROR PAGES
//
//////////////////////////////////////////////////////////////////////////

constexpr const TCHAR* global_server_error = 
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

constexpr const TCHAR* global_client_error =
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

// Static globals for the server as a whole
// Can be set through the Marlin.config reading of the HTTPServer
// unsigned long g_streaming_limit = STREAMING_LIMIT;
// unsigned long g_compress_limit  = COMPRESS_LIMIT;

// Logging macro's
#define DETAILLOG1(text)          if(MUSTLOG(HLL_LOGGING) && m_log) { DetailLog (_T(__FUNCTION__),LogType::LOG_INFO,text); }
#define DETAILLOGS(text,extra)    if(MUSTLOG(HLL_LOGGING) && m_log) { DetailLogS(_T(__FUNCTION__),LogType::LOG_INFO,text,extra); }
#define DETAILLOGV(text,...)      if(MUSTLOG(HLL_LOGGING) && m_log) { DetailLogV(_T(__FUNCTION__),LogType::LOG_INFO,text,__VA_ARGS__); }
#define WARNINGLOG(text,...)      if(MUSTLOG(HLL_LOGGING) && m_log) { DetailLogV(_T(__FUNCTION__),LogType::LOG_WARN,text,__VA_ARGS__); }
#define ERRORLOG(code,text)       ErrorLog (_T(__FUNCTION__),code,text)
#define HTTPERROR(code,text)      HTTPError(_T(__FUNCTION__),code,text)

// Media types are here
MediaTypes* g_media = nullptr;

//////////////////////////////////////////////////////////////////////////
//
// THE SERVER
//
//////////////////////////////////////////////////////////////////////////

HTTPServer::HTTPServer(XString p_name)
           :m_name(p_name)
           ,m_marlinConfig(nullptr)
           ,m_clientErrorPage(global_client_error)
           ,m_serverErrorPage(global_server_error)
           ,m_hostName(GetHostName(HOSTNAME_SHORT))
{
  // Preparing for error reporting
  PrepareProcessForSEH();

  // Clear request queue ID
  HTTP_SET_NULL_ID(&m_requestQueue);

  InitializeCriticalSection(&m_eventLock);
  InitializeCriticalSection(&m_sitesLock);
  InitializeCriticalSection(&m_socketLock);

  // Initially the counter is stopped
  m_counter.Stop();
}

HTTPServer::~HTTPServer()
{
  // Clean out all request memory
  for(auto& request : m_requests)
  {
    delete request;
  }
  m_requests.clear();

  // Clean out the Marlin.config
  if(m_marlinConfig)
  {
    delete m_marlinConfig;
    m_marlinConfig = nullptr;
  }

  // Clean out the media types
  if(g_media)
  {
    delete g_media;
    g_media = nullptr;
  }

  // Free CS to the OS
  DeleteCriticalSection(&m_eventLock);
  DeleteCriticalSection(&m_sitesLock);
  DeleteCriticalSection(&m_socketLock);

  // Resetting the signal handlers
  ResetProcessAfterSEH();
}

void
HTTPServer::InitLogging()
{
  // Check for a logging object
  if(m_log && !m_logOwner && m_log->GetIsOpen())
  {
    // Already opened somewhere else. Leave it alone!
    return;
  }

  // Create a new one for ourselves
    m_log = LogAnalysis::CreateLogfile(m_name);
    m_logOwner = true;

  XString file    = m_log->GetLogFileName();
  int  cache      = m_log->GetCacheSize();
  int  logging    = m_log->GetLogLevel();
  bool timing     = m_log->GetDoTiming();
  bool events     = m_log->GetDoEvents();
  bool doLogging  = m_log->GetDoLogging();
  int  keepfiles  = m_log->GetKeepfiles();

  // Get parameters from Marlin.config
  file      = m_marlinConfig->GetParameterString (_T("Logging"),_T("Logfile"),  file);
  doLogging = m_marlinConfig->GetParameterBoolean(_T("Logging"),_T("DoLogging"),doLogging);
  logging   = m_marlinConfig->GetParameterInteger(_T("Logging"),_T("LogLevel"), logging);
  timing    = m_marlinConfig->GetParameterBoolean(_T("Logging"),_T("DoTiming"), timing);
  events    = m_marlinConfig->GetParameterBoolean(_T("Logging"),_T("DoEvents"), events);
  cache     = m_marlinConfig->GetParameterInteger(_T("Logging"),_T("Cache"),    cache);
  keepfiles = m_marlinConfig->GetParameterInteger(_T("Logging"),_T("Keep"),     keepfiles);

  // Use if overridden in Marlin.config
  if(!file.IsEmpty())
  {
    m_log->SetLogFilename(file);
  }
  m_log->SetCache(cache);
  m_log->SetLogLevel(m_logLevel = logging);
  m_log->SetDoTiming(timing);
  m_log->SetDoEvents(events);
  m_log->SetKeepfiles(keepfiles);
}

// Initialise the ThreadPool
void
HTTPServer::InitThreadPool()
{
  int minThreads = m_marlinConfig->GetParameterInteger(_T("Server"),_T("MinThreads"),NUM_THREADS_MINIMUM);
  int maxThreads = m_marlinConfig->GetParameterInteger(_T("Server"),_T("MaxThreads"),NUM_THREADS_MAXIMUM);
  int stackSize  = m_marlinConfig->GetParameterInteger(_T("Server"),_T("StackSize"), THREAD_STACKSIZE);

  m_pool.TrySetMinimum(minThreads);
  m_pool.TrySetMaximum(maxThreads);
  m_pool.SetStackSize(stackSize);
}

// Initialise the hard server limits in bytes
void
HTTPServer::InitHardLimits()
{
  g_streaming_limit = m_marlinConfig->GetParameterInteger(_T("Server"),_T("StreamingLimit"),g_streaming_limit);
  g_compress_limit  = m_marlinConfig->GetParameterInteger(_T("Server"),_T("CompressLimit"),g_compress_limit);

  // Cannot be bigger than 2 GB, otherwise use indirect file access!
  if(g_streaming_limit > (0x7FFFFFFF))
  {
    g_streaming_limit = 0x7FFFFFFF;
  }
  // Should not be smaller than 1MB
  if(g_streaming_limit < (1024 * 1024))
  {
    g_streaming_limit = (1024 * 1024);
  }
  // Should not be bigger than 25 4K pages
  if(g_compress_limit > (25 * 4 * 1024))
  {
    g_compress_limit = (25 * 4 * 1024);
  }
  if(g_compress_limit < (4 * 1024))
  {
    g_compress_limit = (4 * 1024);
  }

  DETAILLOGV(_T("Server hard-limit file-size streaming limit: %d"),g_streaming_limit);
  DETAILLOGV(_T("Server hard-limit compression threshold: %d"),    g_compress_limit);
}

// Initialise the even stream parameters
void
HTTPServer::InitEventstreamKeepalive()
{
  m_eventKeepAlive = m_marlinConfig->GetParameterInteger(_T("Server"),_T("EventKeepAlive"),DEFAULT_EVENT_KEEPALIVE);
  m_eventRetryTime = m_marlinConfig->GetParameterInteger(_T("Server"),_T("EventRetryTime"),DEFAULT_EVENT_RETRYTIME);

  if(m_eventKeepAlive < EVENT_KEEPALIVE_MIN) m_eventKeepAlive = EVENT_KEEPALIVE_MIN;
  if(m_eventKeepAlive > EVENT_KEEPALIVE_MAX) m_eventKeepAlive = EVENT_KEEPALIVE_MAX;
  if(m_eventRetryTime < EVENT_RETRYTIME_MIN) m_eventRetryTime = EVENT_RETRYTIME_MIN;
  if(m_eventRetryTime > EVENT_RETRYTIME_MAX) m_eventRetryTime = EVENT_RETRYTIME_MAX;

  DETAILLOGV(_T("Server SSE keepalive interval: %d ms"), m_eventKeepAlive);
  DETAILLOGV(_T("Server SSE client retry time : %d ms"), m_eventRetryTime);
}

void
HTTPServer::SetQueueLength(ULONG p_length)
{
  // Queue length check, at minimum this
  if(p_length < INIT_HTTP_BACKLOGQUEUE)
  {
    p_length = INIT_HTTP_BACKLOGQUEUE;
  }
  if(p_length > MAXX_HTTP_BACKLOGQUEUE)
  {
    p_length = MAXX_HTTP_BACKLOGQUEUE;
  }
  m_queueLength = p_length;
}

void
HTTPServer::SetLogLevel(int p_logLevel)
{
  // Check boundaries
  if (p_logLevel < HLL_NOLOG)   p_logLevel = HLL_NOLOG;
  if (p_logLevel > HLL_HIGHEST) p_logLevel = HLL_HIGHEST;

  // Keep the loglevel
  m_logLevel = p_logLevel;
  if(m_log)
  {
    m_log->SetLogLevel(p_logLevel);
  }
}

void
HTTPServer::SetDetailedLogging(bool p_detail)
{
  TRACE(_T("WARNING: Use SetLogLevel()\n"));
  m_logLevel = p_detail ? HLL_LOGGING : HLL_NOLOG;
}

bool
HTTPServer::GetDetailedLogging()
{
  TRACE(_T("WARNING: Use GetLogLevel()\n"));
  return (m_logLevel>=HLL_LOGGING);
}

void
HTTPServer::DetailLog(const TCHAR* p_function,LogType p_type,const TCHAR* p_text)
{
  if(m_log && MUSTLOG(HLL_LOGGING))
  {
    m_log->AnalysisLog(p_function,p_type,false,p_text);
  }
}

void
HTTPServer::DetailLogS(const TCHAR* p_function,LogType p_type,const TCHAR* p_text,const TCHAR* p_extra)
{
  if(m_log && MUSTLOG(HLL_LOGGING))
  {
    XString text(p_text);
    text += p_extra;

    m_log->AnalysisLog(p_function,p_type,false,text);
  }
}

void
HTTPServer::DetailLogV(const TCHAR* p_function,LogType p_type,const TCHAR* p_text,...)
{
  if(m_log && MUSTLOG(HLL_LOGGING))
  {
    va_list varargs;
    va_start(varargs,p_text);
    XString text;
    text.FormatV(p_text,varargs);
    va_end(varargs);

    m_log->AnalysisLog(p_function,p_type,false,text);
  }
}

// Error logging to the log file
void
HTTPServer::ErrorLog(const TCHAR* p_function,DWORD p_code,XString p_text)
{
  bool result = false;

  if(m_log)
  {
    p_text.AppendFormat(_T(" Error [%08lX] %s"),p_code,GetLastErrorAsString(p_code).GetString());
    result = m_log->AnalysisLog(p_function, LogType::LOG_ERROR,false,p_text);
  }

  // nothing logged
  if(!result)
  {
    // What can we do? As a last result: print to the MS-Windows event log
    SvcReportErrorEvent(0,true,_T(__FUNCTION__),_T("Marlin ") MARLIN_SERVER_VERSION _T(" Error [%08lX] %s\n"),p_code,p_text.GetString());
  }
}

void
HTTPServer::HTTPError(const TCHAR* p_function,int p_status,XString p_text)
{
  bool result = false;

  if(m_log)
  {
    p_text.AppendFormat(_T(" HTTP Status [%d] %s"),p_status,GetHTTPStatusText(p_status));
    result = m_log->AnalysisLog(p_function, LogType::LOG_ERROR,false,p_text);
  }

  // nothing logged
  if(!result)
  {
    // What can we do? As a last result: print to the MS-Windows event log
    SvcReportErrorEvent(0,true,_T(__FUNCTION__),_T("Marlin ") MARLIN_SERVER_VERSION _T(" Status [%d] %s\n"),p_status,p_text.GetString());
  }
}

// General checks before starting
bool
HTTPServer::GeneralChecks()
{
  // Check the error report object
  if(m_errorReport == nullptr)
  {
    ERRORLOG(ERROR_INVALID_PARAMETER,_T("Connect an ErrorReport object for handling safe-exceptions"));
    return false;
  }
  DETAILLOG1(_T("ErrorReporting checks out OK"));

  // Make sure we have the webroot directory
  WinFile ensDir(m_webroot);
  ensDir.CreateDirectory();

  // Check the webroot directory for read/write access
  int retCode = _taccess(m_webroot,06);
  if(retCode != 0)
  {
    ERRORLOG(retCode,_T("No access to the webroot directory: ") + m_webroot);
    return false;
  }
  DETAILLOGS(_T("Access to webroot directory ok: "),m_webroot);

  return true;
}

void
HTTPServer::SetLogging(LogAnalysis* p_log)
{
  if(m_log && m_logOwner)
  {
    LogAnalysis::DeleteLogfile(m_log);
    m_logOwner = false;
  }
  m_log = p_log;
  InitLogging();
}

void
HTTPServer::SetWebroot(XString p_webroot)
{
  // Check WebRoot against your config
  m_webroot = m_marlinConfig->GetParameterString(_T("Server"),_T("WebRoot"),p_webroot);

  // Directly set WebRoot
  WinFile ensure(m_webroot);
  if(!ensure.CreateDirectory())
  {
    ERRORLOG(ensure.GetLastError(),_T("Cannot reach server root directory: ") + p_webroot);
  }
  m_webroot = p_webroot;
}

XString
HTTPServer::MakeSiteRegistrationName(int p_port,XString p_url)
{
  XString registration;
  p_url.MakeLower();

  // Now chop off all parameters and anchors
  int posquest = p_url.Find('?');
  int posquote = p_url.Find('\'');
  int posanchr = p_url.Find('#');

  if((posquest > 0 && posquote < 0) ||
     (posquest > 0 && posquote > 0 && posquest < posquote))
  {
    p_url = p_url.Left(posquest);
  }
  else if((posanchr > 0 && posquote < 0) ||
          (posanchr > 0 && posquote > 0 && posanchr < posquote))
  {
    p_url = p_url.Left(posanchr);
  }
  p_url.TrimRight('/');
  registration.Format(_T("%d:%s"),p_port,p_url.GetString());

  return registration;
}

// Register a URL to listen on
bool
HTTPServer::RegisterSite(const HTTPSite* p_site,const XString& p_urlPrefix)
{
  AutoCritSec lock(&m_sitesLock);

  // Check that we have the minimal parameters
  if(p_site == nullptr || p_urlPrefix.IsEmpty())
  {
    ERRORLOG(ERROR_INVALID_PARAMETER,_T("RegisterSite: no site to register"));
    return false;
  }

  // Use counter
  m_counter.Start();

  // See to it that we are initialized
  if(!Initialise())
  {
    // If initialize did not work out OK
    m_counter.Stop();
    return false;
  }

  // Remember our context
  int     port = p_site->GetPort();
  XString base = p_site->GetSite();
  XString site(MakeSiteRegistrationName(port,base));
  SiteMap::iterator it = m_allsites.find(site);
  if(it != m_allsites.end())
  {
    if(p_urlPrefix.CompareNoCase(it->second->GetPrefixURL()) == 0)
    {
      // Duplicate site found: Not necessarily an error
      WARNINGLOG(_T("Site was already registered: %s"),base.GetString());
      return false;
    }
  }

  // Remember the site 
  m_allsites[site] = const_cast<HTTPSite*>(p_site);

  // Use counter
  m_counter.Stop();

  return true;
}

// Cache policy is registered
void
HTTPServer::SetCachePolicy(HTTP_CACHE_POLICY_TYPE p_type,ULONG p_seconds)
{
  if(p_type >= HttpCachePolicyNocache &&
     p_type <= HttpCachePolicyMaximum )
  {
    m_policy = p_type;
    m_secondsToLive = 0;
    if(m_policy == HttpCachePolicyTimeToLive)
    {
      m_secondsToLive = p_seconds;
    }
  }
  else
  {
    m_policy = HttpCachePolicyNocache;
    m_secondsToLive = 0;
  }
}

// Find less known verb
// Returns the extra verbs known by HTTPMessage
// Needed because they are NOT in <winhttp.h>!!
HTTPCommand
HTTPServer::GetUnknownVerb(LPCSTR p_verb)
{
  if(_stricmp(p_verb,"MERGE") == 0) return HTTPCommand::http_merge;
  if(_stricmp(p_verb,"PATCH") == 0) return HTTPCommand::http_patch;
  
  return HTTPCommand::http_no_command;
}

// Finding a Site Context
HTTPSite*
HTTPServer::FindHTTPSite(int p_port,PCWSTR p_url)
{
  XString site = WStringToString(p_url);

  return FindHTTPSite(p_port,site);
}

// Find a site or a sub-site of the site
HTTPSite*
HTTPServer::FindHTTPSite(HTTPSite* p_site,const XString& p_url)
{
  HTTPSite* site = FindHTTPSite(p_site->GetPort(),p_url);
  if(site)
  {
    // Check if the found site is indeed it's main site
    if(site->GetMainSite() == p_site)
    {
      return site;
    }
  }
  return p_site;
}

// Finding the HTTP site from the mappings of all sites
// by the 'longest match' method, optimized for pathnames
HTTPSite*
HTTPServer::FindHTTPSite(int p_port,const XString& p_url)
{
  AutoCritSec lock(&m_sitesLock);

  // Prepare the URL
  XString search(MakeSiteRegistrationName(p_port,p_url));

  // Search for longest match between incoming URL 
  // and registered sites from "RegisterSite"
  int pos = search.GetLength();
  while(pos > 0)
  {
    XString finding = search.Left(pos);
    SiteMap::iterator it = m_allsites.find(finding);
    if(it != m_allsites.end())
    {
      // Longest match is found
      return it->second;
    }
    // Find next length to match
    while(--pos > 0)
    {
      if(search.GetAt(pos) == _T('/') ||
         search.GetAt(pos) == '\\')
      {
        break;
      }
    }
  }
  return nullptr;
}

// Find routing information within the site
void
HTTPServer::CalculateRouting(const HTTPSite* p_site,HTTPMessage* p_message)
{
  XString url = p_message->GetCrackedURL().AbsoluteResource();
  XString known(p_site->GetSite());

  XString route = url.Mid(known.GetLength());
  route = route.Trim('/');
  route = route.Trim('\\');

  while(route.GetLength())
  {
    int pos = route.Find('/');
    if (pos < 0) pos = route.Find('\\');

    if(pos > 0)
    {
      p_message->AddRoute(route.Left(pos));
      route = route.Mid(pos + 1);
    }
    else
    {
      // Last route part to add
      p_message->AddRoute(route);
      route.Empty();
    }
  }
}

// Find our extra header for RemoteDesktop (Citrix!) support
int
HTTPServer::FindRemoteDesktop(USHORT p_count,PHTTP_UNKNOWN_HEADER p_headers)
{
  int remDesktop = 0;
  
  // Iterate through all unknown headers
  // Take care: it could be Unicode!
  for(int ind = 0;ind < p_count; ++ind)
  {
    XString name = LPCSTRToString(p_headers[ind].pName);
    if(name.GetLength() != p_headers[ind].NameLength)
    {
      // Different length: suppose it's Unicode
      wstring nameDesk = reinterpret_cast<const wchar_t*>(p_headers[ind].pName);
      if(nameDesk.compare(L"RemoteDesktop") == 0)
      {
        remDesktop = _wtoi(reinterpret_cast<const wchar_t*>(p_headers[ind].pRawValue));
        break;
      }
    }
    else
    {
      if(name.Compare(_T("RemoteDesktop")) == 0)
      {
        remDesktop = atoi(p_headers[ind].pRawValue);
        break;
      }
    }
  }
  return remDesktop;
}

// Check authentication of a HTTP request
bool
HTTPServer::CheckAuthentication(PHTTP_REQUEST  p_request
                               ,HTTP_OPAQUE_ID p_id
                               ,HTTPSite*      p_site
                               ,XString&       p_rawUrl
                               ,XString        p_authorize
                               ,HANDLE&        p_token)
{
  bool doReceive = true;

  if(p_request->RequestInfoCount>0)
  {
    for(unsigned ind = 0; ind<p_request->RequestInfoCount; ++ind)
    {
      if(p_request->pRequestInfo[ind].InfoType == HttpRequestInfoTypeAuth)
      {
        // Default is failure
        doReceive = false;

        PHTTP_REQUEST_AUTH_INFO auth = (PHTTP_REQUEST_AUTH_INFO) p_request->pRequestInfo[ind].pInfo;
        if(auth->AuthStatus == HttpAuthStatusNotAuthenticated)
        {
          // Not (yet) authenticated. Back to the client for authentication
          DETAILLOGS(_T("Not yet authenticated for: "),p_rawUrl);
          HTTPMessage msg(HTTPCommand::http_response,HTTP_STATUS_DENIED);
          msg.SetHTTPSite(p_site);
          msg.SetRequestHandle(p_id);
          RespondWithClientError(&msg,HTTP_STATUS_DENIED,_T("Not authenticated"));
        }
        else if(auth->AuthStatus == HttpAuthStatusFailure)
        {
          if(p_authorize.IsEmpty())
          {
            // Second round. Still not authenticated. Drop the connection, better next time
            DETAILLOGS(_T("Authentication failed for: "),p_rawUrl);
            DETAILLOGS(_T("Authentication failed because of: "),AuthenticationStatus(auth->SecStatus));
            HTTPMessage msg(HTTPCommand::http_response,HTTP_STATUS_DENIED);
            msg.SetHTTPSite(p_site);
            msg.SetRequestHandle(p_id);
            RespondWithClientError(&msg,HTTP_STATUS_DENIED,_T("Not authenticated"));
          }
          else
          {
            // Site must do the authorization
            DETAILLOGS(_T("Authentication by HTTPSite: "),p_authorize);
            doReceive = true;
          }
        }
        else if(auth->AuthStatus == HttpAuthStatusSuccess)
        {
          // Authentication accepted: all is well
          DETAILLOGS(_T("Authentication done for: "),p_rawUrl);
          p_token = auth->AccessToken;
          doReceive = true;
        }
        else
        {
          XString authError;
          authError.Format(_T("Authentication mechanism failure. Unknown status: %d"),auth->AuthStatus);
          ERRORLOG(ERROR_NOT_AUTHENTICATED,authError);
          HTTPMessage msg(HTTPCommand::http_response,HTTP_STATUS_FORBIDDEN);
          msg.SetHTTPSite(p_site);
          msg.SetRequestHandle(p_id);
          RespondWithClientError(&msg,HTTP_STATUS_FORBIDDEN,_T("Forbidden"));
        }
      }
      else if (p_request->pRequestInfo[ind].InfoType == HttpRequestInfoTypeSslProtocol)
      {
        // Only exists on Windows 10 / Server 2016
        if (GetLogLevel() >= HLL_TRACE)
        {
          PHTTP_SSL_PROTOCOL_INFO sslInfo = reinterpret_cast<PHTTP_SSL_PROTOCOL_INFO>(p_request->pRequestInfo[ind].pInfo);
          LogSSLConnection(sslInfo);
        }
      }
    }
  }
  return doReceive;
}

// Build the www-auhtenticate challenge
XString
HTTPServer::BuildAuthenticationChallenge(XString p_authScheme,XString p_realm)
{
  XString challenge;

  if(p_authScheme.CompareNoCase(_T("Basic")) == 0)
  {
    // Basic authentication
    // the real realm comes from the URL properties!
    challenge = _T("Basic realm=\"BasicRealm\"");
  }
  else if(p_authScheme.CompareNoCase(_T("NTLM")) == 0)
  {
    // Standard MS-Windows authentication
    challenge = _T("NTLM");
  }
  else if(p_authScheme.CompareNoCase(_T("Negotiate")) == 0)
  {
    // Will fall back to NTLM on MS-Windows systems
    challenge = _T("Negotiate");
  }
  else if(p_authScheme.CompareNoCase(_T("Digest")) == 0)
  {
    // Will use Digest authentication
    // the real realm and domain comes from the URL properties!
    challenge.Format(_T("Digest realm=\"%s\""),p_realm.GetString());
  }
  else if(p_authScheme.CompareNoCase(_T("Kerberos")) == 0)
  {
    challenge = _T("Kerberos");
  }
  return challenge;
}

// Sending response for an incoming message
void
HTTPServer::SendResponse(SOAPMessage* p_message)
{
  // This message already sent, or not able to send: no request is known
  if(p_message->GetHasBeenAnswered())
  {
    ERRORLOG(ERROR_INVALID_PARAMETER,_T("SendResponse: nothing to send"));
    return;
  }

  // Check if we must restore the encryption
  if(p_message->GetFaultCode().IsEmpty())
  {
    HTTPSite* site = p_message->GetHTTPSite();
    if (site && site->GetEncryptionLevel() != XMLEncryption::XENC_Plain)
    {
      // Restore settings in a secure-scenario
      // But only if there is no SOAP Fault in there, 
      // As these should always be readable
      p_message->SetSecurityLevel   (site->GetEncryptionLevel());
      p_message->SetSecurityPassword(site->GetEncryptionPassword());
    }
    DETAILLOG1(_T("Send SOAP response"));
  }

  // Convert to a HTTP response
  HTTPMessage* answer = new HTTPMessage(HTTPCommand::http_response,p_message);

  // Check if Content-type was correctly set
  if(answer->GetContentType().IsEmpty())
  {
    switch(p_message->GetSoapVersion())
    {
      case SoapVersion::SOAP_10: // Fall Through
      case SoapVersion::SOAP_11: answer->SetContentType(_T("text/xml"));             break;
      case SoapVersion::SOAP_12: answer->SetContentType(_T("application/soap+xml")); break;
    }
  }

  // Check if we have a SOAPAction header for SOAP 1.1 situations
  if(p_message->GetSoapVersion() == SoapVersion::SOAP_11 &&
    !p_message->GetSoapAction().IsEmpty() &&
     answer->GetHeader(_T("SOAPAction")).IsEmpty())
  {
    XString action = _T("\"") + p_message->GetNamespace();
    if(action.Right(1) != _T("/")) action += _T("/");
    action += p_message->GetSoapAction() + _T("\"");

    answer->AddHeader(_T("SOAPAction"), action);
  }

  // Set status in case of an error
  if(p_message->GetErrorState())
  {
    answer->SetStatus(HTTP_STATUS_BAD_REQUEST);
    DETAILLOG1(_T("Send SOAP FAULT response"));
  }

  // Send the HTTP Message as response
  SendResponse(answer);
  answer->DropReference();

  // Do **NOT** send an answer twice
  p_message->SetHasBeenAnswered();
}

// Sending response for an incoming message
void
HTTPServer::SendResponse(JSONMessage* p_message)
{
  // This message already sent, or not able to send: no request is known
  if(p_message->GetHasBeenAnswered())
  {
    ERRORLOG(ERROR_INVALID_PARAMETER,_T("SendResponse: nothing to send"));
    return;
  }

  // Check if we must restore the encryption
  if(p_message->GetErrorState())
  {
    DETAILLOGS(_T("Send JSON FAULT response:\n"),p_message->GetJsonMessage());
    // Convert to a HTTP response
    HTTPMessage* answer = new HTTPMessage(HTTPCommand::http_response,p_message);
    answer->SetStatus(HTTP_STATUS_BAD_REQUEST);
    answer->GetFileBuffer()->Reset();
    SendResponse(answer);
    answer->DropReference();
  }
  else
  {
    DETAILLOG1(_T("Send JSON response"));
    // Convert to a HTTP response
    HTTPMessage* answer = new HTTPMessage(HTTPCommand::http_response,p_message);
    if(answer->GetContentType().Find(_T("json")) < 0)
    {
      answer->SetContentType(_T("application/json"));
    }
    // Send the HTTP Message as response
    SendResponse(answer);
    answer->DropReference();
  }
  // Do **NOT** send an answer twice
  p_message->SetHasBeenAnswered();
}

// Response in the server error range (500-505)
void
HTTPServer::RespondWithServerError(HTTPMessage* p_message
                                  ,int          p_error
                                  ,XString      p_reason)
{
  HTTPERROR(p_error,_T("Respond with server error"));
  XString page;
  page.Format(m_serverErrorPage,p_error,p_reason.GetString());

  p_message->Reset();
  p_message->GetFileBuffer()->Reset();
  p_message->GetFileBuffer()->SetBuffer(reinterpret_cast<uchar*>(const_cast<TCHAR*>(page.GetString())),page.GetLength());
  p_message->SetStatus(p_error);
  p_message->SetContentType(_T("text/html"));

  SendResponse(p_message);
}

void
HTTPServer::RespondWithServerError(HTTPMessage*    p_message
                                  ,int             p_error)
{
  RespondWithServerError(p_message,p_error,_T("General server error"));
}

// Response in the client error range (400-417)
void
HTTPServer::RespondWithClientError(HTTPMessage* p_message
                                  ,int          p_error
                                  ,XString      p_reason
                                  ,XString      p_authScheme
                                  ,XString      p_realm)
{
  HTTPERROR(p_error,_T("Respond with client error"));
  XString page;
  page.Format(m_clientErrorPage,p_error,p_reason.GetString());

  p_message->Reset();
  p_message->GetFileBuffer()->Reset();
  p_message->GetFileBuffer()->SetBuffer(reinterpret_cast<uchar*>(const_cast<TCHAR*>(page.GetString())),page.GetLength());
  p_message->SetStatus(p_error);
  p_message->SetContentType(_T("text/html"));

  XString challenge = BuildAuthenticationChallenge(p_authScheme,p_realm);
  if(!challenge.IsEmpty())
  {
    p_message->AddHeader(_T("AuthenticationScheme"),challenge);
  }

  SendResponse(p_message);
}

void
HTTPServer::RespondWith2FASuccess(HTTPMessage* p_message,XString p_body)
{
  p_message->Reset();
  p_message->GetFileBuffer()->Reset();
  p_message->GetFileBuffer()->SetBuffer(reinterpret_cast<uchar*>(const_cast<TCHAR*>(p_body.GetString())),p_body.GetLength());
  p_message->SetContentType(_T("application/json"));
  p_message->SetStatus(HTTP_STATUS_OK);
  SendResponse(p_message);
}

// Authentication failed for this reason
XString 
HTTPServer::AuthenticationStatus(SECURITY_STATUS p_secStatus)
{
  XString answer;

  switch(p_secStatus)
  {
    case SEC_E_INCOMPLETE_MESSAGE:      answer = _T("Incomplete authentication message answer or channel-broken");
                                        break;
    case SEC_E_INSUFFICIENT_MEMORY:     answer = _T("Security sub-system out of memory");
                                        break;
    case SEC_E_INTERNAL_ERROR:          answer = _T("Security sub-system internal error");
                                        break;
    case SEC_E_INVALID_HANDLE:          answer = _T("Security sub-system invalid handle");
                                        break;
    case SEC_E_INVALID_TOKEN:           answer = _T("Security token is not (no longer) valid");
                                        break;
    case SEC_E_LOGON_DENIED:            answer = _T("Logon denied");
                                        break;
    case SEC_E_NO_AUTHENTICATING_AUTHORITY: 
                                        answer = _T("Authentication authority (domain/realm) not contacted, incorrect or unavailable");
                                        break;
    case SEC_E_NO_CREDENTIALS:          answer = _T("No credentials presented");
                                        break;
    case SEC_E_OK:                      answer = _T("Security OK, token generated");
                                        break;
    case SEC_E_SECURITY_QOS_FAILED:     answer = _T("Quality of Security failed due to Digest authentication");
                                        break;
    case SEC_E_UNSUPPORTED_FUNCTION:    answer = _T("Security attribute / unsupported function error");
                                        break;
    case SEC_I_COMPLETE_AND_CONTINUE:   answer = _T("Complete and continue, pass token to client");
                                        break;
    case SEC_I_COMPLETE_NEEDED:         answer = _T("Send completion message to client");
                                        break;
    case SEC_I_CONTINUE_NEEDED:         answer = _T("Send token to client and wait for return");
                                        break;
    case STATUS_LOGON_FAILURE:          answer = _T("Logon tried but failed");
                                        break;
    default:                            answer = _T("Authentication failed for unknown reason");
                                        break;
  }
  return answer;
}

// Log SSL Info of the connection
void
HTTPServer::LogSSLConnection(PHTTP_SSL_PROTOCOL_INFO p_sslInfo)
{
  Crypto  crypt;

  // Getting the SSL Protocol / Cipher / Hash / KeyExchange descriptions
  XString protocol = crypt.GetSSLProtocol(p_sslInfo->Protocol);
  XString cipher   = crypt.GetCipherType (p_sslInfo->CipherType);
  XString hash     = crypt.GetHashMethod (p_sslInfo->HashType);
  XString exchange = crypt.GetKeyExchange(p_sslInfo->KeyExchangeType);

  hash.MakeUpper();

  // Now log the cryptographic elements of the connection
  DETAILLOGV(_T("Secure SSL Connection. Protocol [%s] Cipher [%s:%d] Hash [%s:%d] Key Exchange [%s:%d]")
            ,protocol.GetString()
            ,cipher.GetString(),  p_sslInfo->CipherStrength
            ,hash.GetString(),    p_sslInfo->HashStrength
            ,exchange.GetString(),p_sslInfo->KeyExchangeStrength);
}

// Handle text-based content-type messages
void
HTTPServer::HandleTextContent(HTTPMessage* p_message)
{
#ifdef UNICODE
  UNREFERENCED_PARAMETER(p_message);
#else
  uchar* body   = nullptr;
  size_t length = 0;
  
  // Get a copy of the body
  p_message->GetRawBody(&body,length);

  int uni = IS_TEXT_UNICODE_UNICODE_MASK;   // Intel/AMD processors + BOM
  if(IsTextUnicode(body,(int)length,&uni))
  {
    XString output;
    bool doBOM = false;

    // Find specific code page and try to convert
    XString charset = FindCharsetInContentType(p_message->GetContentType());
    DETAILLOGS(_T("Try convert charset in pre-handle fase: "),charset);
    bool convert = TryConvertWideString(body,static_cast<int>(length),_T(""),output,doBOM);

    // Output to message->body
    if(convert)
    {
      // Copy back the converted string (does a copy!!)
      p_message->GetFileBuffer()->SetBuffer(reinterpret_cast<uchar*>(const_cast<TCHAR*>(output.GetString())),output.GetLength());
      p_message->SetSendBOM(doBOM);
    }
    else
    {
      ERRORLOG(ERROR_NO_UNICODE_TRANSLATION,_T("Incoming text body: no translation"));
    }
  }
  // Free raw buffer
  delete [] body;
#endif
}

void
HTTPServer::CheckSitesStarted()
{
  AutoCritSec lock(&m_sitesLock);

  // Cycle through all the sites
  for(auto& site : m_allsites)
  {
    if(site.second->GetIsStarted() == false)
    {
      XString message;
      message.Format(_T("Forgotten to call 'StartSite' for: %s"),site.second->GetPrefixURL().GetString());
      ERRORLOG(ERROR_CALL_NOT_IMPLEMENTED,message);
    }
  }
}

//////////////////////////////////////////////////////////////////////////
//
// HEADER METHODS
//
//////////////////////////////////////////////////////////////////////////

// RFC 2616: paragraph 14.25: "if-modified-since"
bool
HTTPServer::DoIsModifiedSince(HTTPMessage* p_msg)
{
  WIN32_FILE_ATTRIBUTE_DATA data;
  PSYSTEMTIME sinceTime = p_msg->GetSystemTime();
  HTTPSite* site   = p_msg->GetHTTPSite();
  XString fileName;
  
  // Getting the filename
  if(site)
  {
    fileName = site->GetWebroot() + p_msg->GetAbsoluteResource();
  }
  
  // Normalize pathname to MS-Windows
  fileName.Replace(_T('/'),'\\');

  // Check for a name
  if(fileName.IsEmpty())
  {
    return false;
  }

  // See if the file is there (existence)
  if(_taccess(fileName,00) == 0)
  {
    // Get extended file attributes to get to the last-written-to filetime
    if(GetFileAttributesEx(fileName,GetFileExInfoStandard,&data))
    {
      // Convert requested system time to file time for comparison
      FILETIME fTime;
      SystemTimeToFileTime(sinceTime,&fTime);

      // Compare filetimes always in two steps
      if(fTime.dwHighDateTime < data.ftLastWriteTime.dwHighDateTime)
      {
        return false;
      }
      if(fTime.dwHighDateTime == data.ftLastWriteTime.dwHighDateTime)
      {
        if(fTime.dwLowDateTime < data.ftLastWriteTime.dwLowDateTime)
        {
          return false;
        }
      }
      // Not modified = 304
      DETAILLOG1(_T("Sending response: Not modified"));
      p_msg->Reset();
      p_msg->GetFileBuffer()->Reset();
      p_msg->SetStatus(HTTP_STATUS_NOT_MODIFIED);
      SendResponse(p_msg);
      return true;
    }
  }
  else
  {
    // DO NOT SEND A 404 NOT FOUND ERROR
    // IT COULD BE THAT THE SERVER CANNOT READ THE FILE, BUT THE IMPERSONATED CLIENT CAN!!!
    // SendResponse(p_msg->GetRequestHandle(),HTTP_STATUS_NOT_FOUND,"Resource not found","");
  }
  return false;
}

//////////////////////////////////////////////////////////////////////////
//
// SERVER PUSH EVENTS
//
//////////////////////////////////////////////////////////////////////////

// Register server push event stream for this site
EventStream*
HTTPServer::SubscribeEventStream(PSOCKADDR_IN6    p_sender
                                ,int              p_desktop
                                ,HTTPSite*        p_site
                                ,XString          p_url
                                ,const XString&   p_path
                                ,HTTP_OPAQUE_ID   p_requestId
                                ,HANDLE           p_token)
{
  AutoCritSec lock(&m_eventLock);

  EventMap::iterator lower = m_eventStreams.lower_bound(p_url);
  EventMap::iterator upper = m_eventStreams.upper_bound(p_url);

  while(lower != m_eventStreams.end())
  {
    if(lower->second->m_site      == p_site      &&
       lower->second->m_requestID == p_requestId &&
       lower->second->m_alive     == true)
    {
      // Stream by accident still active -> re-use it!
      return lower->second;
    }
    // Next iterator
    if(lower == upper)
    {
      break;
    }
    ++lower;
  }

  // ADD NEW STREAM
  EventStream* stream = new EventStream();
  memcpy(&stream->m_sender,p_sender,sizeof(SOCKADDR_IN6));
  stream->m_desktop   = p_desktop;
  stream->m_port      = p_site->GetPort();
  stream->m_requestID = p_requestId;
  stream->m_baseURL   = p_url;
  stream->m_absPath   = p_path;
  stream->m_site      = p_site;
  stream->m_alive     = InitEventStream(*stream);
  if(p_token)
  {
    GetTokenOwner(p_token,stream->m_user);
  }
  // If the stream is still alive, keep it!
  if(stream->m_alive)
  {
    m_eventStreams.insert(std::make_pair(p_url,stream));
    TryStartEventHeartbeat();
  }
  else
  {
    ERRORLOG(ERROR_ACCESS_DENIED,_T("Could not initialize event stream. Dropping stream."));
    delete stream;
    stream = nullptr;
  }
  return stream;
}

// Send to a server push event stream / deleting p_event
bool
HTTPServer::SendEvent(int p_port,XString p_site,ServerEvent* p_event,XString p_user /*=""*/)
{
  AutoCritSec lock(&m_eventLock);
  bool result = false;

  p_site.MakeLower();
  p_site.TrimRight('/');

//   // Get the stream-string of the event
//   BYTE* buffer = nullptr;
//   int   length = 0;
//   EventToStringBuffer(p_event,&buffer,length);

  // Find the context of the URL (if any)
  HTTPSite* context = FindHTTPSite(p_port,p_site);
  if(context && context->GetIsEventStream())
  {
    EventMap::iterator lower = m_eventStreams.lower_bound(p_site);
    EventMap::iterator upper = m_eventStreams.upper_bound(p_site);

    while(lower != m_eventStreams.end())
    {
      // Find the stream
      EventStream* stream = lower->second;

      // Find next already before we remove this one on closing the stream
      if(lower != upper)
      {
        ++lower;
      }

      // See if we must push the event to this stream
      if(p_user.IsEmpty() || p_user.CompareNoCase(stream->m_user) == 0)
      {
        // Copy the event
        ServerEvent* event = new ServerEvent(p_event);

        // Sending the event, while deleting it
        // Accumulating the results, true if at least one is sent :-)
        result |= SendEvent(stream,event);
      }
      // Next iterator
      if(lower == upper)
      {
        break;
      }
    }
  }
  // Ready with the event
  delete p_event;
  // Accumulated results
  return result;
}

// Send to a server push event stream on EventStream basis
bool
HTTPServer::SendEvent(EventStream* p_stream
                     ,ServerEvent* p_event
                     ,bool         p_continue /*=true*/)
{
  // Check for input parameters
  if(p_stream == nullptr || p_event == nullptr)
  {
    ERRORLOG(ERROR_FILE_NOT_FOUND,_T("SendEvent missing a stream or an event"));
    return false;
  }

  // But we must now make sure the event stream still does exist
  // and has not been closed by the event monitor
  if(!HasEventStream(p_stream))
  {
    return false;
  }

  // Set next stream id
  ++p_stream->m_lastID;

  // See to it that we have an id
  // The given ID from the event is prevailing over the automatic stream id
  if(p_event->m_id == 0)
  {
    p_event->m_id = p_stream->m_lastID;
  }
  else
  {
    p_stream->m_lastID = p_event->m_id;
  }

  // Tell what we are about to do
  if (MUSTLOG(HLL_LOGGING) && m_log && p_stream->m_alive)
  {
    XString text;
    text.Format(_T("Sent event id: %d to client(s) on URL: "), p_event->m_id);
    text += p_stream->m_baseURL;
    DETAILLOG1(text);
  }

  // Produce the event string in UTF-8 format
  BYTE* buffer = nullptr;
  int   length = 0;
  EventToStringBuffer(p_event,&buffer,length);

  // Send the event to the client. This can take an I/O wait time
  bool alive = SendResponseEventBuffer(p_stream->m_requestID,&p_stream->m_lock,&buffer,length,p_continue);

  // Lock server as short as possible to register the return status
  // But we must now make sure the event stream still does exist
  if(HasEventStream(p_stream))
  {
    p_stream->m_alive = alive;
  }

  // Remember the time we sent the event pulse
  _time64(&(p_stream->m_lastPulse));

  // Remember the highest last ID
  if(p_event->m_id > p_stream->m_lastID)
  {
    p_stream->m_lastID = p_event->m_id;
  }

  // Increment the chunk counter
  ++p_stream->m_chunks;

  // Ready with the event
  delete[] buffer;
  delete p_event;

  // Stream not alive, or stopping
  if(!p_stream->m_alive || !p_continue)
  {
    AbortEventStream(p_stream);
    return false;
  }

  // Delivered our event, but out of more data chunks for next requests
  if(p_stream->m_chunks > MAX_DATACHUNKS)
  {
    // Abort stream, so client will reopen a new stream
    AbortEventStream(p_stream);
    return false;
  }

  // Delivered the event at least to one stream
  return p_stream->m_alive;
}

// Form event to a stream string buffer 
// Guaranteed to be in UTF-8 format
// Caller must delete the buffer
void
HTTPServer::EventToStringBuffer(ServerEvent* p_event,BYTE** p_buffer,int& p_length)
{
  XString stream;

  // Append client retry time to the first event
  if(p_event->m_id == 1)
  {
    stream.Format(_T("retry: %u\n"),p_event->m_id);
  }
  // Event name if not standard 'message'
  if(!p_event->m_event.IsEmpty() && p_event->m_event.CompareNoCase(_T("message")))
  {
    stream.AppendFormat(_T("event:%s\n"),p_event->m_event.GetString());
  }
  // Event ID if not zero
  if(p_event->m_id > 0)
  {
    stream.AppendFormat(_T("id:%u\n"),p_event->m_id);
  }
  if(!p_event->m_data.IsEmpty())
  {
    XString buffer = p_event->m_data;
    // Optimize our carriage returns
    if(buffer.Find('\r') >= 0)
    {
      buffer.Replace(_T("\r\n"),_T("\n"));
      buffer.Replace(_T("\r"),_T("\n"));
    }
    while(!buffer.IsEmpty())
    {
      int pos = buffer.Find('\n');
      if(pos < 0)
      {
        stream += _T("data:") + buffer + _T("\n");
        buffer.Empty();
      }
      else
      {
        ++pos;
        stream += _T("data:") + buffer.Left(pos);
        buffer = buffer.Mid(pos);
      }
    }
    // Catch the last line without a newline
    if(stream.Right(1) != _T("\n"))
    {
      stream += _T("\n");
    }
  }
  stream += _T("\n");

  // SSE Stream is always in UTF-8 format.
#ifdef UNICODE
  TryCreateNarrowString(stream,_T("utf-8"),false,p_buffer,p_length);
#else
  XString utf8stream = EncodeStringForTheWire(stream,_T("utf-8"));
  p_length  = utf8stream.GetLength();
  *p_buffer = new BYTE[p_length + 1];
  memcpy_s(*p_buffer,p_length + 1,utf8stream.GetString(),p_length);
  (*p_buffer)[p_length] = 0;
#endif
}

// Running our event monitor heartbeat thread
/*static*/ void
RunEventMonitor(void* p_server)
{
  HTTPServer* server = reinterpret_cast<HTTPServer*>(p_server);
  server->EventMonitor();
}

// Try to start the even heartbeat monitor
void
HTTPServer::TryStartEventHeartbeat()
{
  // If already started, do not start again
  if(m_eventMonitor)
  {
    return;
  }
  m_eventEvent   = CreateEvent(NULL,FALSE,FALSE,NULL);
  m_eventMonitor = reinterpret_cast<HANDLE>(_beginthread(RunEventMonitor,0L,reinterpret_cast<void*>(this)));
}

void
HTTPServer::EventMonitor()
{
  DWORD streams  = 0;
  bool  startNew = false;
  _set_se_translator(SeTranslator);

  DETAILLOG1(_T("Event heartbeat monitor started"));
  do
  {
    DWORD waited = WaitForSingleObjectEx(m_eventEvent,m_eventKeepAlive,true);
    switch(waited)
    {
      case WAIT_TIMEOUT:        streams = CheckEventStreams();
                                break;
      case WAIT_OBJECT_0:       // Explicit check event (stopping!)
                                streams = 0;
                                break;
      case WAIT_IO_COMPLETION:  // Log file has been written
                                break;
      case WAIT_FAILED:         // Failed for some reason
      case WAIT_ABANDONED:      // Interrupted for some reason
      default:                  // Start a new monitor / new event
                                streams  = 0;
                                startNew = true;
                                break;
    }                   
  }
  while(streams);

  // Reset the monitor
  CloseHandle(m_eventEvent);
  m_eventEvent   = nullptr;
  m_eventMonitor = nullptr;
  DETAILLOG1(_T("Event heartbeat monitor stopped"));

  if(startNew)
  {
    // Retry starting a new thread/event
    TryStartEventHeartbeat();
  }
}

// Check all event streams for the heartbeat monitor
// No events are sent on this server, for the duration of this call
// Dead streams will be cleaned
UINT
HTTPServer::CheckEventStreams()
{
  // Calculate the current moment in milliseconds
  __timeb64 now;
  _ftime64_s(&now);
  __time64_t pulse = (now.time * CLOCKS_PER_SEC) + now.millitm;

  AutoCritSec lock(&m_eventLock);
  UINT number = 0;
  // Create keep alive buffer (always UTF-8 format compatible)
  char* keepAlive = ":keepalive\r\n\r\n";
  int size = (int) strlen(keepAlive);

  DETAILLOG1(_T("Starting event heartbeat"));

  // Pulse event stream with a comment
  EventMap::iterator it = m_eventStreams.begin();
  while(it != m_eventStreams.end())
  {
    try
    {
      EventStream* stream = it->second;
      // If we did not send anything for the last eventKeepAlive seconds,
      // Keep a margin of half a second for the wakeup of the server
      // we send a ":keepalive" comment to the clients
      if((pulse - stream->m_lastPulse) > (m_eventKeepAlive - 500))
      {
        stream->m_alive = SendResponseEventBuffer(stream->m_requestID,&stream->m_lock,(BYTE**)&keepAlive,size);
        stream->m_lastPulse = pulse;
        ++stream->m_chunks;
        ++number;
      }
      ++it;
    }
    catch(StdException& ex)
    {
      ERRORLOG(ERROR_NOT_FOUND,_T("Cannot handle event stream: ") + ex.GetErrorMessage());
      try
      {
        delete it->second;
      }
      catch(StdException&)
      {
        ERRORLOG(ERROR_NOT_FOUND,_T("Event stream already gone!"));
      }
      it = m_eventStreams.erase(it);
    }
  }

  // What we just did
  DETAILLOGV(_T("Sent heartbeat to %d push-event clients."),number);

  // Clean up dead event streams
  it = m_eventStreams.begin();
  while(it != m_eventStreams.end())
  {
    try
    {
      EventStream* stream = it->second;
          if(stream->m_chunks > MAX_DATACHUNKS)
      {
        DETAILLOGS(_T("Push-event stream out of data chunks: "),stream->m_baseURL);

        // Send a close-stream event
        ServerEvent* event = new ServerEvent(_T("close"));
        SendEvent(stream,event);
        // Remove request from the request queue, closing the connection
        CancelRequestStream(stream->m_requestID);
        // Erase stream, it's out of chunks now
        delete it->second;
        it = m_eventStreams.erase(it);
      }
      else if(stream->m_alive == false)
      {
        DETAILLOGS(_T("Abandoned push-event client from: "),stream->m_baseURL);

        // Remove request from the request queue, closing the connection
        CancelRequestStream(stream->m_requestID);
        // Erase dead stream, and goto next
        delete it->second;
        it = m_eventStreams.erase(it);
      }
      else
      {
        ++it; // Next stream
      }
    }
    catch(StdException& ex)
    {
      ERRORLOG(ERROR_NOT_FOUND,_T("Cannot clean up event stream: ") + ex.GetErrorMessage());
      try
      {
        delete it->second;
      }
      catch(StdException&)
      {
        ERRORLOG(ERROR_NOT_FOUND,_T("Event stream already gone!"));
      }
      it = m_eventStreams.erase(it);
    }
  }

  // Monitor still needed?
  return (unsigned) m_eventStreams.size();
}

// Return the fact that we have an event stream
bool
HTTPServer::HasEventStream(const EventStream* p_stream)
{
  AutoCritSec lock(&m_eventLock);

  for(const auto& stream : m_eventStreams)
  {
    if(p_stream == stream.second)
    {
      return true;
    }
  }
  return false;
}

// Return the number of push-event-streams for this URL
int
HTTPServer::HasEventStreams(int p_port,XString p_url,XString p_user /*=""*/)
{
  AutoCritSec lock(&m_eventLock);
  unsigned count = 0;

  HTTPSite* context = FindHTTPSite(p_port,p_url);

  if(context && context->GetIsEventStream())
  {
    EventMap::iterator lower = m_eventStreams.lower_bound(p_url);
    EventMap::iterator upper = m_eventStreams.upper_bound(p_url);

    while(lower != m_eventStreams.end())
    {
      if(p_user.IsEmpty() || p_user.CompareNoCase(lower->second->m_user) == 0)
      {
        ++count;
      }
      if(lower == upper) break;
      ++lower;
    }
  }
  return count;
}

// Close an event stream for one stream only
bool
HTTPServer::CloseEventStream(const EventStream* p_stream)
{
  AutoCritSec lock(&m_eventLock);

  for(const auto& stream : m_eventStreams)
  {
    if(stream.second == p_stream)
    {
      // Show in the log
      DETAILLOGV(_T("Closing event stream (user: %s) for URL: %s"),p_stream->m_user.GetString(),p_stream->m_baseURL.GetString());
      if(stream.second->m_alive)
      {
        // Send a close-stream event
        // And close the stream by sending a close flag
        ServerEvent* event = new ServerEvent(_T("close"));
        SendEvent(stream.second,event,false);
      }
      else
      {
        AbortEventStream(stream.second);
      }
      return true;
    }
  }
  return false;
}

// Close event streams for an URL and probably a user
void
HTTPServer::CloseEventStreams(int p_port,XString p_url,XString p_user /*=""*/)
{
  AutoCritSec lock(&m_eventLock);

  HTTPSite* context = FindHTTPSite(p_port,p_url);
  if(context && context->GetIsEventStream())
  {
    EventMap::iterator lower = m_eventStreams.lower_bound(p_url);
    EventMap::iterator upper = m_eventStreams.upper_bound(p_url);

    while(lower != m_eventStreams.end())
    {
      EventStream* stream = lower->second;
      if(lower != upper)
      {
        ++lower;
      }
      if(p_user.IsEmpty() || p_user.CompareNoCase(stream->m_user) == 0)
      {
        CloseEventStream(stream);
      }
      if(lower == upper)
      {
        break;
      }
    }
  }
}

// Close and abort an event stream for whatever reason
// Call only on an abandoned stream.
// No "OnError" or "OnClose" can be sent now
bool
HTTPServer::AbortEventStream(EventStream* p_stream)
{
  // No lock on the m_siteslock, as we are dealing with an event
  AutoCritSec lock(&m_eventLock);

  EventMap::iterator it = m_eventStreams.begin();
  while(it != m_eventStreams.end())
  {
    if(it->second == p_stream)
    {
      if(p_stream->m_alive)
      {
        // Abandon the stream in the correct server
        CancelRequestStream(p_stream->m_requestID);
      }
      // Done with the stream
      delete p_stream;
      // Erase from the cache and return next position
      m_eventStreams.erase(it);
      return true;
    }
    ++it;
  }
  return false;
}

void
HTTPServer::RemoveEventStream(CString p_url)
{
  // Event stream NOT accepted
  AutoCritSec lock(&m_eventLock);
  EventMap::iterator it = m_eventStreams.find(p_url);
  if (it != m_eventStreams.end())
  {
    m_eventStreams.erase(it);
  }
}

void
HTTPServer::RemoveEventStream(const EventStream* p_stream)
{
  AutoCritSec lock(&m_eventLock);

  EventMap::iterator it = m_eventStreams.begin();
  while(it != m_eventStreams.end())
  {
    if(it->second == p_stream)
    {
      m_eventStreams.erase(it);
      return;
    }
    ++it;
  }
}

//////////////////////////////////////////////////////////////////////////
//
// WebServices
//
//////////////////////////////////////////////////////////////////////////

// Register a WebServiceServer
bool
HTTPServer::RegisterService(WebServiceServer* p_service)
{
  AutoCritSec lock(&m_sitesLock);

  XString name = p_service->GetName();
  name.MakeLower();
  ServiceMap::iterator it = m_allServices.find(name);
  if(it == m_allServices.end())
  {
    m_allServices[name] = p_service;
    return true;
  }
  return false;
}

// Remove registration of a service
bool
HTTPServer::UnRegisterService(XString p_serviceName)
{
  AutoCritSec lock(&m_sitesLock);

  p_serviceName.MakeLower();
  ServiceMap::iterator it = m_allServices.find(p_serviceName);
  if(it != m_allServices.end())
  {
    delete it->second;
    m_allServices.erase(it);
    return true;
  }
  return false;
}

// Finding a previous registered service endpoint
WebServiceServer*
HTTPServer::FindService(XString p_serviceName)
{
  AutoCritSec lock(&m_sitesLock);

  p_serviceName.MakeLower();
  ServiceMap::iterator it = m_allServices.find(p_serviceName);
  if(it != m_allServices.end())
  {
    return it->second;
  }
  return nullptr;
}

//////////////////////////////////////////////////////////////////////////
//
// WEBSOCKET
//
//////////////////////////////////////////////////////////////////////////

// Register a WebSocket
bool
HTTPServer::RegisterSocket(WebSocket* p_socket)
{
  XString key(p_socket->GetIdentityKey());
  DETAILLOGV(_T("Register websocket [%s] at the server"),key.GetString());
  key.MakeLower();

  AutoCritSec lock(&m_socketLock);
  SocketMap::iterator it = m_sockets.find(key);
  if(it != m_sockets.end())
  {
    // Drop the double socket. Removes socket from the mapping!
    it->second->CloseSocket();
  }
  m_sockets.insert(std::make_pair(key,p_socket));
  return true;
}

// Remove registration of a WebSocket
bool
HTTPServer::UnRegisterWebSocket(WebSocket* p_socket)
{
  XString key;
  AutoCritSec lock(&m_socketLock);
  try
  {
    key = p_socket->GetIdentityKey();
    DETAILLOGV(_T("Unregistering websocket [%s] from the server"),key.GetString());
    key.MakeLower();

    SocketMap::iterator it = m_sockets.find(key);
    if(it != m_sockets.end())
    {
      delete it->second;
      m_sockets.erase(it);
      return true;
    }
  }
  catch(StdException& ex)
  {
    ERRORLOG(ERROR_INVALID_ACCESS,_T("WebSocket memory NOT FOUND! : " + ex.GetErrorMessage()));
  }
  // We don't have it
  ERRORLOG(ERROR_FILE_NOT_FOUND,_T("Websocket to unregister NOT FOUND! : ") + key);
  return false;
}

// Finding a previous registered websocket
WebSocket*
HTTPServer::FindWebSocket(XString p_key)
{
  p_key.MakeLower();

  AutoCritSec lock(&m_socketLock);
  SocketMap::iterator it = m_sockets.find(p_key);
  if(it != m_sockets.end())
  {
    return it->second;
  }
  return nullptr;
}

//////////////////////////////////////////////////////////////////////////
//
// OUTSTANDING HTTPRequests
//
//////////////////////////////////////////////////////////////////////////

// Outstanding asynchronous I/O requests
void
HTTPServer::RegisterHTTPRequest(HTTPRequest* p_request)
{
  AutoCritSec lock(&m_sitesLock);

  m_requests.push_back(p_request);
}

void
HTTPServer::UnRegisterHTTPRequest(const HTTPRequest* p_request)
{
  AutoCritSec lock(&m_sitesLock);

  RequestMap::iterator it;
  for(it = m_requests.begin();it != m_requests.end();++it)
  {
    if((*it) == p_request)
    {
      m_requests.erase(it);
      return;
    }
  }
}

//////////////////////////////////////////////////////////////////////////
//
// REQUEST: BODY LOGGING & TRACING
//
//////////////////////////////////////////////////////////////////////////

void
HTTPServer::TraceKnownRequestHeader(unsigned p_number,LPCTSTR p_value)
{
  // See if known header is 'given'
  if (!p_value || *p_value == 0)
  {
    return;
  }

  // Header fields are defined in HTTPMessage.cpp!!
  XString line = header_fields[p_number];
  line += _T(": ");
  line += p_value;
  m_log->BareStringLog(line);
}

void
HTTPServer::TraceRequest(PHTTP_REQUEST p_request)
{
  // Print HTTPSite context first before anything else of the call
  m_log->AnalysisLog(_T(__FUNCTION__),LogType::LOG_TRACE,true,_T("Incoming call for site context: %lX"),p_request->UrlContext);

  XString httpVersion;
  httpVersion.Format(_T("HTTP/%d.%d"),p_request->Version.MajorVersion,p_request->Version.MinorVersion);

  // The request verb.
  XString verb;
  if(p_request->Verb == HttpVerbUnknown && p_request->UnknownVerbLength)
  {
#ifdef UNICODE
    bool foundBom(false);
    TryConvertNarrowString((BYTE*)p_request->pUnknownVerb,(int)strlen(p_request->pUnknownVerb),_T(""),verb,foundBom);
#else
    verb = p_request->pUnknownVerb;
#endif
  }
  else
  {
    switch (p_request->Verb)
    {
      case HttpVerbOPTIONS:   verb = _T("OPTIONS");   break;
      case HttpVerbGET:       verb = _T("GET");       break;
      case HttpVerbHEAD:      verb = _T("HEAD");      break;
      case HttpVerbPOST:      verb = _T("POST");      break;
      case HttpVerbPUT:       verb = _T("PUT");       break;
      case HttpVerbDELETE:    verb = _T("DELETE");    break;
      case HttpVerbTRACE:     verb = _T("TRACE");     break;
      case HttpVerbCONNECT:   verb = _T("CONNECT");   break;
      case HttpVerbTRACK:     verb = _T("TRACK");     break;
      case HttpVerbMOVE:      verb = _T("MOVE");      break;
      case HttpVerbCOPY:      verb = _T("COPY");      break;
      case HttpVerbPROPFIND:  verb = _T("PROPFIND");  break;
      case HttpVerbPROPPATCH: verb = _T("PROPPATCH"); break;
      case HttpVerbMKCOL:     verb = _T("MKCOL");     break;
      case HttpVerbLOCK:      verb = _T("LOCK");      break;
      case HttpVerbUNLOCK:    verb = _T("UNLOCK");    break;
      case HttpVerbSEARCH:    verb = _T("SEARCH");    break;
      default:                verb = _T("UNKNOWN");   break;
    }
  }

  // THE PRINCIPAL HTTP PROTOCOL CALL LINE
  XString httpLine;

#ifdef UNICODE
  XString rawUrl;
  bool foundBom(false);
  TryConvertNarrowString((BYTE*)p_request->pRawUrl,(int)strlen(p_request->pRawUrl),_T(""),rawUrl,foundBom);
#else
  XString rawUrl = p_request->pRawUrl;
#endif
  httpLine.Format(_T("%s %s %s"),verb.GetString(),rawUrl.GetString(),httpVersion.GetString());
  m_log->BareStringLog(httpLine);

  // Print all 'known' HTTP headers
  for (unsigned ind = 0; ind < HttpHeaderMaximum; ++ind)
  {
    TraceKnownRequestHeader(ind,LPCSTRToString(p_request->Headers.KnownHeaders[ind].pRawValue));
  }

  // Print all 'unknown' headers
  for (unsigned ind = 0;ind < p_request->Headers.UnknownHeaderCount; ++ind)
  {
    XString uheader;
    uheader  = p_request->Headers.pUnknownHeaders[ind].pName;
    uheader += _T(": ");
    uheader += p_request->Headers.pUnknownHeaders[ind].pRawValue;
    m_log->BareStringLog(uheader);
  }

  // Print empty line between header and body (!!)
  m_log->BareStringLog(XString(_T("")));

  // The following members are NOT used here

  //   ULONGLONG              BytesReceived;
  //   USHORT                 EntityChunkCount;
  //   PHTTP_DATA_CHUNK       pEntityChunks;
  //   HTTP_RAW_CONNECTION_ID RawConnectionId;
  //   PHTTP_SSL_INFO         pSslInfo;
}

void
HTTPServer::LogTraceRequest(PHTTP_REQUEST p_request,FileBuffer* p_buffer,bool p_utf16 /*=false*/)
{
  // Only if we have an attached logfile
  if(!m_log)
  {
    return;
  }

  // Dump request + headers
  if(MUSTLOG(HLL_TRACEDUMP))
  {
    if(p_request)
    {
      TraceRequest(p_request);
    }
  }
  // Dump the body
  if(MUSTLOG(HLL_LOGBODY))
  {
    if(p_buffer && p_buffer->GetLength())
    {
      LogTraceRequestBody(p_buffer,p_utf16);
    }
  }
}

void
HTTPServer::LogTraceRequestBody(FileBuffer* p_buffer,bool p_utf16 /*=false*/)
{
  // Only if we have work to do
  if(!p_buffer || !m_log)
  {
    return;
  }

  if(MUSTLOG(HLL_LOGBODY) && p_buffer->GetLength())
  {
    // Get copy of the body and dump in the logfile
    uchar* buffer = nullptr;
    size_t length = 0;
    p_buffer->GetBufferCopy(buffer,length);

    if(p_utf16)
    {
#ifdef UNICODE
      XString string = (LPCTSTR)buffer;
      AutoCSTR body(string);
      m_log->BareBufferLog((void*)body.cstr(),body.size());
#else
      XString string;
      bool foundBom(false);
      TryConvertWideString(buffer,(int)length,_T(""),string,foundBom);
      m_log->BareBufferLog((void*) string.GetString(),string.GetLength() * sizeof(TCHAR));
#endif
    }
    else
    {
      m_log->BareBufferLog(buffer,static_cast<unsigned>(length));
    }
    if(MUSTLOG(HLL_TRACEDUMP))
    {
      m_log->AnalysisHex(_T(__FUNCTION__),_T("Incoming"),buffer,static_cast<unsigned>(length));
    }

    // Delete buffer copy
    delete[] buffer;
  }
}

//////////////////////////////////////////////////////////////////////////
//
// RESPONSE: BODY LOGGING & TRACING
//
//////////////////////////////////////////////////////////////////////////

void
HTTPServer::TraceKnownResponseHeader(unsigned p_number,LPCTSTR p_value)
{
  // See if known header is 'given'
  if(!p_value || *p_value == 0)
  {
    return;
  }

  // Header fields are defined in HTTPMessage.cpp!!
  XString line = p_number < HttpHeaderAcceptRanges ? header_fields[p_number] : header_response[p_number - HttpHeaderAcceptRanges];
  line += _T(": ");
  line += p_value;
  m_log->BareStringLog(line);
}

void
HTTPServer::TraceResponse(PHTTP_RESPONSE p_response)
{
  // See if we have a response, or just did a ResponseEntityBody sent
  if(p_response == nullptr)
  {
    return;
  }

  // Print the principal first protocol line
  XString line;
  XString reason = LPCSTRToString(p_response->pReason);
  line.Format(_T("HTTP/%d.%d %d %s")
             ,p_response->Version.MajorVersion
             ,p_response->Version.MinorVersion
             ,p_response->StatusCode
             ,reason.GetString());
  m_log->AnalysisLog(_T(__FUNCTION__),LogType::LOG_TRACE,false,_T("Full HTTP protocol"));
  m_log->BareStringLog(line);

  // Print all 'known' HTTP headers
  for (unsigned ind = 0; ind < HttpHeaderResponseMaximum; ++ind)
  {
    TraceKnownResponseHeader(ind,LPCSTRToString(p_response->Headers.KnownHeaders[ind].pRawValue));
  }

  // Print all 'unknown' headers
  if(p_response->Headers.pUnknownHeaders)
  {
    for(unsigned ind = 0;ind < p_response->Headers.UnknownHeaderCount; ++ind)
    {
      XString uheader;
      uheader = p_response->Headers.pUnknownHeaders[ind].pName;
      uheader += _T(": ");
      uheader += p_response->Headers.pUnknownHeaders[ind].pRawValue;
      m_log->BareStringLog(uheader);
    }
  }
  m_log->BareStringLog(XString(_T("")));
}

void
HTTPServer::LogTraceResponse(PHTTP_RESPONSE p_response,FileBuffer* p_buffer,bool p_utf16)
{
  // Only if we have a logfile
  if(!m_log)
  {
    return;
  }

  // Trace the protocol and headers
  if(MUSTLOG(HLL_TRACEDUMP))
  {
    TraceResponse(p_response);
  }

  // Log&Trace the body
  if(MUSTLOG(HLL_LOGBODY))
  {
    if(p_buffer && p_buffer->GetLength())
    {
      if(p_buffer->GetFileName().IsEmpty())
      {
        uchar* buffer = nullptr;
        size_t length = 0;
        p_buffer->GetBufferCopy(buffer,length);

        if(p_utf16)
        {
#ifdef UNICODE
          XString string = (LPCTSTR) buffer;
          AutoCSTR str(string);
          m_log->BareBufferLog((void*)str.cstr(),str.size());
#else
          XString string;
          bool foundBom(false);
          TryConvertWideString(buffer,(int) length,_T(""),string,foundBom);
          m_log->BareBufferLog((void*) string.GetString(),string.GetLength());
#endif
        }
        else
        {
          m_log->BareBufferLog(buffer,static_cast<unsigned>(length));
        }

        if(MUSTLOG(HLL_TRACEDUMP))
        {
          m_log->AnalysisHex(_T(__FUNCTION__),_T("Outgoing"),buffer,static_cast<unsigned>(length));
        }
        // Delete buffer copy
        delete[] buffer;
      }
      else
      {
        m_log->AnalysisLog(_T(__FUNCTION__),LogType::LOG_INFO,true,_T("Uploading FILE: %s"),p_buffer->GetFileName().GetString());
      }
    }
  }
}

void
HTTPServer::LogTraceResponse(PHTTP_RESPONSE p_response,unsigned char* p_buffer,unsigned p_length,bool p_utf16)
{
  // Only if we have a logfile
  if(!m_log)
  {
    return;
  }

  // Trace the protocol and headers
  if(MUSTLOG(HLL_TRACEDUMP))
  {
    TraceResponse(p_response);
  }

  // Log&Trace the body
  if(MUSTLOG(HLL_LOGBODY) && p_buffer)
  {
    if(p_utf16)
    {
      XString string;
      bool foundBom(false);
#ifdef UNICODE
      TryConvertNarrowString(p_buffer,(int) p_length,_T(""),string,foundBom);
#else
      TryConvertWideString(p_buffer,(int) p_length,_T(""),string,foundBom);
#endif
      m_log->BareBufferLog((void*) string.GetString(),string.GetLength());
    }
    else
    {
      m_log->BareBufferLog(p_buffer,static_cast<unsigned>(p_length));
    }
    if(MUSTLOG(HLL_TRACEDUMP))
    {
      m_log->AnalysisHex(_T(__FUNCTION__),_T("Outgoing"),p_buffer,p_length);
    }
  }
}

//////////////////////////////////////////////////////////////////////////
//
// DDOS Attacks
//
//////////////////////////////////////////////////////////////////////////

// Applications may call this registration after several failed 
// login attempts of several failed SSE stream registration events
void
HTTPServer::RegisterDDOSAttack(PSOCKADDR_IN6 p_sender,XString p_path)
{
  AutoCritSec lock(&m_sitesLock);

  if(CheckUnderDDOSAttack(p_sender, p_path) == false)
  {
    DDOS attack;
    memcpy(&attack.m_sender,p_sender,sizeof(SOCKADDR_IN6));
    attack.m_abspath   = p_path;
    attack.m_beginTime = clock();

    m_attacks.push_back(attack);

    // REGISTER THE ATTACK
    XString sender = SocketToServer(p_sender);
    ERRORLOG(ERROR_TOO_MANY_SESS,_T("DDOS ATTACK REGISTERED FOR: ") + sender + _T(" : ") + p_path);

    // If we have a logfile where administrators may look
    // register the attack there for all to see!
    if(m_log)
    {
      XString filename = m_log->GetLogFileName();
      int pos = filename.ReverseFind('\\');
      if(pos)
      {
        filename = filename.Left(pos) + _T("ALARM_DDOS_ATTACK.txt");
        WinFile file(filename);

        if(file.Open(winfile_write,FAttributes::attrib_none,Encoding::UTF8))
        {
          file.Format(_T("%s: %s\n"),sender.GetString(),p_path.GetString());
          file.Close();
        }
      }
    }
  }
}

bool
HTTPServer::CheckUnderDDOSAttack(PSOCKADDR_IN6 p_sender,XString p_path)
{
  AutoCritSec lock(&m_sitesLock);

  // If no attacks registered: we are *NOT* under attack
  if(m_attacks.empty())
  {
    return false;
  }
  // See if we find the attack vector.
  for(DDOSMap::iterator it = m_attacks.begin();it != m_attacks.end();++it)
  {
    if((memcmp(&it->m_sender,p_sender,sizeof(SOCKADDR_IN6)) == 0) &&
       (it->m_abspath.CompareNoCase(p_path) == 0))
    {
      if(it->m_beginTime < (clock() - TIMEOUT_BRUTEFORCE))
      {
        // DDOS Attack is now officially over from this sender
        m_attacks.erase(it);
        return false;
      }
      else
      {
        // Extra attack: Bump the clock and wait an extra interval
        it->m_beginTime = clock();
        return true;
      }
    }
  }
  // Nothing found: we are *NOT* part of any attack
  return false;
}

