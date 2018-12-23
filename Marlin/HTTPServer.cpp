/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: HTTPServer.cpp
//
// Marlin Server: Internet server/client
// 
// Copyright (c) 2015-2018 ir. W.E. Huisman
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
#include "EnsureFile.h"
#include "GetLastErrorAsString.h"
#include "Analysis.h"
#include "AutoCritical.h"
#include "PrintToken.h"
#include "Cookie.h"
#include "Crypto.h"
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

constexpr char* global_server_error = 
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

constexpr char* global_client_error =
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

// Error state is retained in TLS (Thread-Local-Storage) on a per-call basis for the server
__declspec(thread) ULONG tls_lastError = 0;

// Static globals for the server as a whole
// Can be set through the web.config reading of the HTTPServer
unsigned long g_streaming_limit = STREAMING_LIMIT;
unsigned long g_compress_limit  = COMPRESS_LIMIT;

// Logging macro's
#define DETAILLOG1(text)          if(MUSTLOG(HLL_LOGGING) && m_log) { DetailLog (__FUNCTION__,LogType::LOG_INFO,text); }
#define DETAILLOGS(text,extra)    if(MUSTLOG(HLL_LOGGING) && m_log) { DetailLogS(__FUNCTION__,LogType::LOG_INFO,text,extra); }
#define DETAILLOGV(text,...)      if(MUSTLOG(HLL_LOGGING) && m_log) { DetailLogV(__FUNCTION__,LogType::LOG_INFO,text,__VA_ARGS__); }
#define WARNINGLOG(text,...)      if(MUSTLOG(HLL_LOGGING) && m_log) { DetailLogV(__FUNCTION__,LogType::LOG_WARN,text,__VA_ARGS__); }
#define ERRORLOG(code,text)       ErrorLog (__FUNCTION__,code,text)
#define HTTPERROR(code,text)      HTTPError(__FUNCTION__,code,text)

// Media types are here
MediaTypes g_media;

//////////////////////////////////////////////////////////////////////////
//
// THE SERVER
//
//////////////////////////////////////////////////////////////////////////

HTTPServer::HTTPServer(CString p_name)
           :m_name(p_name)
{
  // Set defaults
  m_clientErrorPage = global_client_error;
  m_serverErrorPage = global_server_error;
  m_hostName        = GetHostName(HOSTNAME_SHORT);

  // Preparing for error reporting
  PrepareProcessForSEH();

  // Clear request queue ID
  HTTP_SET_NULL_ID(&m_requestQueue);

  InitializeCriticalSection(&m_eventLock);
  InitializeCriticalSection(&m_sitesLock);

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

  // Clean out the web.config
  if(m_webConfig)
  {
    delete m_webConfig;
    m_webConfig = nullptr;
  }

  // Free CS to the OS
  DeleteCriticalSection(&m_eventLock);
  DeleteCriticalSection(&m_sitesLock);

  // Resetting the signal handlers
  ResetProcessAfterSEH();
}

void
HTTPServer::InitLogging()
{
  // Check for a logging object
  if(m_log == NULL)
  {
    // Create a new one
    m_log = new LogAnalysis(m_name);
    m_logOwner = true;
  }

  CString file    = m_log->GetLogFileName();
  int  cache      = m_log->GetCache();
  int  logging    = m_log->GetLogLevel();
  bool timing     = m_log->GetDoTiming();
  bool events     = m_log->GetDoEvents();
  bool doLogging  = m_log->GetDoLogging();

  // Get parameters from web.config
  file      = m_webConfig->GetParameterString ("Logging","Logfile",  file);
  doLogging = m_webConfig->GetParameterBoolean("Logging","DoLogging",doLogging);
  logging   = m_webConfig->GetParameterInteger("Logging","LogLevel", logging);
  timing    = m_webConfig->GetParameterBoolean("Logging","DoTiming", timing);
  events    = m_webConfig->GetParameterBoolean("Logging","DoEvents", events);
  cache     = m_webConfig->GetParameterInteger("Logging","Cache",    cache);

  // Use if overridden in web.config
  if(!file.IsEmpty())
  {
    m_log->SetLogFilename(file);
  }
  m_log->SetCache(cache);
  m_log->SetLogLevel(m_logLevel = logging);
  m_log->SetDoTiming(timing);
  m_log->SetDoEvents(events);
}

// Initialise the ThreadPool
void
HTTPServer::InitThreadPool()
{
  int minThreads = m_webConfig->GetParameterInteger("Server","MinThreads",NUM_THREADS_MINIMUM);
  int maxThreads = m_webConfig->GetParameterInteger("Server","MaxThreads",NUM_THREADS_MAXIMUM);
  int stackSize  = m_webConfig->GetParameterInteger("Server","StackSize", THREAD_STACKSIZE);

  m_pool.TrySetMinimum(minThreads);
  m_pool.TrySetMaximum(maxThreads);
  m_pool.SetStackSize(stackSize);
}

// Initialise the hard server limits in bytes
void
HTTPServer::InitHardLimits()
{
  g_streaming_limit = m_webConfig->GetParameterInteger("Server","StreamingLimit",g_streaming_limit);
  g_compress_limit  = m_webConfig->GetParameterInteger("Server","CompressLimit", g_compress_limit);

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

  DETAILLOGV("Server hard-limit file-size streaming limit: %d",g_streaming_limit);
  DETAILLOGV("Server hard-limit compression threshold: %d",    g_compress_limit);
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

// Setting the error in TLS, so no locking needed.
void
HTTPServer::SetError(int p_error)
{
  tls_lastError = p_error;
}

ULONG
HTTPServer::GetLastError()
{
  return tls_lastError;
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
  TRACE("WARNING: Use SetLogLevel()\n");
  m_logLevel = p_detail ? HLL_LOGGING : HLL_NOLOG;
}

bool
HTTPServer::GetDetailedLogging()
{
  TRACE("WARNING: Use GetLogLevel()\n");
  return (m_logLevel>=HLL_LOGGING);
}

void
HTTPServer::DetailLog(const char* p_function,LogType p_type,const char* p_text)
{
  if(m_log && MUSTLOG(HLL_LOGGING))
  {
    m_log->AnalysisLog(p_function,p_type,false,p_text);
  }
}

void
HTTPServer::DetailLogS(const char* p_function,LogType p_type,const char* p_text,const char* p_extra)
{
  if(m_log && MUSTLOG(HLL_LOGGING))
  {
    CString text(p_text);
    text += p_extra;

    m_log->AnalysisLog(p_function,p_type,false,text);
  }
}

void
HTTPServer::DetailLogV(const char* p_function,LogType p_type,const char* p_text,...)
{
  if(m_log && MUSTLOG(HLL_LOGGING))
  {
    va_list varargs;
    va_start(varargs,p_text);
    CString text;
    text.FormatV(p_text,varargs);
    va_end(varargs);

    m_log->AnalysisLog(p_function,p_type,false,text);
  }
}

// Error logging to the log file
void
HTTPServer::ErrorLog(const char* p_function,DWORD p_code,CString p_text)
{
  bool result = false;

  if(m_log)
  {
    p_text.AppendFormat(" Error [%d] %s",p_code,GetLastErrorAsString(p_code).GetString());
    result = m_log->AnalysisLog(p_function, LogType::LOG_ERROR,false,p_text);
  }

#ifdef _DEBUG
  // nothing logged
  if(!result)
  {
    // What can we do? As a last result: print to stdout
    printf(MARLIN_SERVER_VERSION " Error [%d] %s\n",p_code,(LPCTSTR)p_text);
  }
#endif
}

void
HTTPServer::HTTPError(const char* p_function,int p_status,CString p_text)
{
  bool result = false;

  if(m_log)
  {
    p_text.AppendFormat(" HTTP Status [%d] %s",p_status,GetHTTPStatusText(p_status));
    result = m_log->AnalysisLog(p_function, LogType::LOG_ERROR,false,p_text);
  }

#ifdef _DEBUG
  // nothing logged
  if(!result)
  {
    // What can we do? As a last result: print to stdout
    printf(MARLIN_SERVER_VERSION " Status [%d] %s\n",p_status,(LPCTSTR)p_text);
  }
#endif
}

// General checks before starting
bool
HTTPServer::GeneralChecks()
{
  // Check the error report object
  if(m_errorReport == nullptr)
  {
    ERRORLOG(ERROR_INVALID_PARAMETER,"Connect an ErrorReport object for handling safe-exceptions");
    return false;
  }
  DETAILLOG1("ErrorReporting checks out OK");

  // Make sure we have the webroot directory
  EnsureFile ensDir(m_webroot);
  ensDir.CheckCreateDirectory();

  // Check the webroot directory for read/write access
  int retCode = _access(m_webroot,06);
  if(retCode != 0)
  {
    ERRORLOG(retCode,"No access to the webroot directory: " + m_webroot);
    return false;
  }
  DETAILLOGS("Access to webroot directory ok: ",m_webroot);

  return true;
}

void
HTTPServer::SetLogging(LogAnalysis* p_log)
{
  if(m_log && m_logOwner)
  {
    delete m_log;
    m_logOwner = false;
  }
  m_log = p_log;
  InitLogging();
}

void
HTTPServer::SetWebroot(CString p_webroot)
{
  // Check WebRoot against your config
  m_webroot = m_webConfig->GetParameterString("Server","WebRoot",p_webroot);

  // Directly set WebRoot
  EnsureFile ensure(p_webroot);
  int er = ensure.CheckCreateDirectory();
  if(er)
  {
    ERRORLOG(er,"Cannot reach server root directory: " + p_webroot);
  }
  m_webroot = p_webroot;
}

CString
HTTPServer::MakeSiteRegistrationName(int p_port,CString p_url)
{
  CString registration;
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
  registration.Format("%d:%s",p_port,p_url.GetString());

  return registration;
}

// Register a URL to listen on
bool
HTTPServer::RegisterSite(HTTPSite* p_site,CString p_urlPrefix)
{
  AutoCritSec lock(&m_sitesLock);

  // Use counter
  m_counter.Start();

  if(GetLastError())
  {
    ERRORLOG(ERROR_INVALID_PARAMETER,"RegisterSite called too early");
    return false;
  }

  // See to it that we are initialized
  Initialise();
  if(GetLastError())
  {
    // If initialize did not work out OK
    m_counter.Stop();
    return false;
  }

  // Remember our context
  int port = p_site->GetPort();
  CString base = p_site->GetSite();
  CString site(MakeSiteRegistrationName(port,base));
  SiteMap::iterator it = m_allsites.find(site);
  if(it != m_allsites.end())
  {
    if(p_urlPrefix.CompareNoCase(it->second->GetPrefixURL()) == 0)
    {
      // Duplicate site found: Not necesseraly an error
      WARNINGLOG("Site was already registered: %s",base.GetString());
      return true;
    }
  }

  // Remember the site 
  if(p_site)
  {
    m_allsites[site] = p_site;
  }
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
HTTPServer::GetUnknownVerb(PCSTR p_verb)
{
  if(_stricmp(p_verb,"MERGE") == 0) return HTTPCommand::http_merge;
  if(_stricmp(p_verb,"PATCH") == 0) return HTTPCommand::http_patch;
  
  return HTTPCommand::http_no_command;
}

// Finding a Site Context
HTTPSite*
HTTPServer::FindHTTPSite(int p_port,PCWSTR p_url)
{
  USES_CONVERSION;
  CString site = CW2A(p_url);

  return FindHTTPSite(p_port,site);
}

// Find a site or a sub-site of the site
HTTPSite*
HTTPServer::FindHTTPSite(HTTPSite* p_site,CString& p_url)
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
HTTPServer::FindHTTPSite(int p_port,CString& p_url)
{
  AutoCritSec lock(&m_sitesLock);

  // Prepare the URL
  CString search(MakeSiteRegistrationName(p_port,p_url));

  // Search for longest match between incoming URL 
  // and registered sites from "RegisterSite"
  int pos = search.GetLength();
  while(pos > 0)
  {
    CString finding = search.Left(pos);
    SiteMap::iterator it = m_allsites.find(finding);
    if(it != m_allsites.end())
    {
      // Longest match is found
      return it->second;
    }
    // Find next length to match
    while(--pos > 0)
    {
      if(search.GetAt(pos) == '/' ||
         search.GetAt(pos) == '\\')
      {
        break;
      }
    }
  }
  return nullptr;
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
    CString name = p_headers[ind].pName;
    if(name.GetLength() != p_headers[ind].NameLength)
    {
      // Different length: suppose it's Unicode
      wstring nameDesk = (wchar_t*)p_headers[ind].pName;
      if(nameDesk.compare(L"RemoteDesktop") == 0)
      {
        remDesktop = _wtoi((const wchar_t*)p_headers[ind].pRawValue);
        break;
      }
    }
    else
    {
      if(name.Compare("RemoteDesktop") == 0)
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
                               ,CString&       p_rawUrl
                               ,CString        p_authorize
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
          DETAILLOGS("Not yet authenticated for: ",p_rawUrl);
          HTTPMessage msg(HTTPCommand::http_response,HTTP_STATUS_DENIED);
          msg.SetHTTPSite(p_site);
          msg.SetRequestHandle(p_id);
          RespondWithClientError(&msg,HTTP_STATUS_DENIED,"Not authenticated");
        }
        else if(auth->AuthStatus == HttpAuthStatusFailure)
        {
          if(p_authorize.IsEmpty())
          {
            // Second round. Still not authenticated. Drop the connection, better next time
            DETAILLOGS("Authentication failed for: ",p_rawUrl);
            DETAILLOGS("Authentication failed because of: ",AuthenticationStatus(auth->SecStatus));
            HTTPMessage msg(HTTPCommand::http_response,HTTP_STATUS_DENIED);
            msg.SetHTTPSite(p_site);
            msg.SetRequestHandle(p_id);
            RespondWithClientError(&msg,HTTP_STATUS_DENIED,"Not authenticated");
          }
          else
          {
            // Site must do the authorization
            DETAILLOGS("Authentication by HTTPSite: ",p_authorize);
            doReceive = true;
          }
        }
        else if(auth->AuthStatus == HttpAuthStatusSuccess)
        {
          // Authentication accepted: all is well
          DETAILLOGS("Authentication done for: ",p_rawUrl);
          p_token = auth->AccessToken;
          doReceive = true;
        }
        else
        {
          CString authError;
          authError.Format("Authentication mechanism failure. Unknown status: %d",auth->AuthStatus);
          ERRORLOG(ERROR_NOT_AUTHENTICATED,authError);
          HTTPMessage msg(HTTPCommand::http_response,HTTP_STATUS_FORBIDDEN);
          msg.SetHTTPSite(p_site);
          msg.SetRequestHandle(p_id);
          RespondWithClientError(&msg,HTTP_STATUS_FORBIDDEN,"Forbidden");
        }
      }
      else if (p_request->pRequestInfo[ind].InfoType == HttpRequestInfoTypeSslProtocol)
      {
        // Only exists on Windows 10 / Server 2016
        if (GetLogLevel() >= HLL_TRACE)
        {
          PHTTP_SSL_PROTOCOL_INFO sslInfo = (PHTTP_SSL_PROTOCOL_INFO) p_request->pRequestInfo[ind].pInfo;
          LogSSLConnection(sslInfo);
        }
      }
    }
  }
  return doReceive;
}

// Build the www-auhtenticate challenge
CString
HTTPServer::BuildAuthenticationChallenge(CString p_authScheme,CString p_realm)
{
  CString challenge;

  if(p_authScheme.CompareNoCase("Basic") == 0)
  {
    // Basic authentication
    // the real realm comes from the URL properties!
    challenge = "Basic";
  }
  else if(p_authScheme.CompareNoCase("NTLM") == 0)
  {
    // Standard MS-Windows authentication
    challenge = "NTLM";
  }
  else if(p_authScheme.CompareNoCase("Negotiate") == 0)
  {
    // Will fall back to NTLM on MS-Windows systems
    challenge = "Negotiate";
  }
  else if(p_authScheme.CompareNoCase("Digest") == 0)
  {
    // Will use Digest authentication
    // the real realm and domain comes from the URL properties!
    challenge.Format("Digest realm=\"%s\"",p_realm.GetString());
  }
  else if(p_authScheme.CompareNoCase("Kerberos") == 0)
  {
    challenge = "Kerberos";
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
    ERRORLOG(ERROR_INVALID_PARAMETER,"SendResponse: nothing to send");
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
    DETAILLOG1("Send SOAP response");
  }

  // Convert to a HTTP response
  HTTPMessage* answer = new HTTPMessage(HTTPCommand::http_response,p_message);

  // Check if Content-type was correctly set
  if(answer->GetContentType().IsEmpty())
  {
    switch(p_message->GetSoapVersion())
    {
      case SoapVersion::SOAP_10: // Fall Through
      case SoapVersion::SOAP_11: answer->SetContentType("text/xml");             break;
      case SoapVersion::SOAP_12: answer->SetContentType("application/soap+xml"); break;
    }
  }

  // Check if we have a SOAPAction header for SOAP 1.1 situations
  if(p_message->GetSoapVersion() == SoapVersion::SOAP_11 &&
    !p_message->GetSoapAction().IsEmpty() &&
     answer->GetHeader("SOAPAction").IsEmpty())
  {
    answer->AddHeader("SOAPAction", "\"" + p_message->GetNamespace() + p_message->GetSoapAction() + "\"", false);
  }

  // Set status in case of an error
  if(p_message->GetErrorState())
  {
    answer->SetStatus(HTTP_STATUS_BAD_REQUEST);
    DETAILLOG1("Send SOAP FAULT response");
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
    ERRORLOG(ERROR_INVALID_PARAMETER,"SendResponse: nothing to send");
    return;
  }

  // Check if we must restore the encryption
  if(p_message->GetErrorState())
  {
    DETAILLOGS("Send JSON FAULT response:\n",p_message->GetJsonMessage());
    // Convert to a HTTP response
    HTTPMessage* answer = new HTTPMessage(HTTPCommand::http_response,p_message);
    answer->SetStatus(HTTP_STATUS_BAD_REQUEST);
    answer->GetFileBuffer()->Reset();
    SendResponse(answer);
    answer->DropReference();
  }
  else
  {
    DETAILLOG1("Send JSON response");
    // Convert to a HTTP response
    HTTPMessage* answer = new HTTPMessage(HTTPCommand::http_response,p_message);
    if(answer->GetContentType().Find("json") < 0)
    {
      answer->SetContentType("application/text+json");
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
                                  ,CString      p_reason)
{
  HTTPERROR(p_error,"Respond with server error");
  CString page;
  page.Format(m_serverErrorPage,p_error,p_reason.GetString());

  p_message->Reset();
  p_message->GetFileBuffer()->Reset();
  p_message->GetFileBuffer()->SetBuffer((uchar*)page.GetString(),page.GetLength());
  p_message->SetStatus(p_error);

  SendResponse(p_message);
}

void
HTTPServer::RespondWithServerError(HTTPMessage*    p_message
                                  ,int             p_error)
{
  RespondWithServerError(p_message,p_error,"General server error");
}

// Response in the client error range (400-417)
void
HTTPServer::RespondWithClientError(HTTPMessage* p_message
                                  ,int          p_error
                                  ,CString      p_reason
                                  ,CString      p_authScheme
                                  ,CString      p_realm)
{
  HTTPERROR(p_error,"Respond with client error");
  CString page;
  page.Format(m_clientErrorPage,p_error,p_reason.GetString());

  p_message->Reset();
  p_message->GetFileBuffer()->Reset();
  p_message->GetFileBuffer()->SetBuffer((uchar*)page.GetString(),page.GetLength());
  p_message->SetStatus(p_error);

  CString challenge = BuildAuthenticationChallenge(p_authScheme,p_realm);
  if(!challenge.IsEmpty())
  {
    p_message->AddHeader("AuthenticationScheme",challenge);
  }

  SendResponse(p_message);
}

// Authentication failed for this reason
CString 
HTTPServer::AuthenticationStatus(SECURITY_STATUS p_secStatus)
{
  CString answer;

  switch(p_secStatus)
  {
    case SEC_E_INCOMPLETE_MESSAGE:      answer = "Incomplete authentication message answer or channel-broken";
                                        break;
    case SEC_E_INSUFFICIENT_MEMORY:     answer = "Security sub-system out of memory";
                                        break;
    case SEC_E_INTERNAL_ERROR:          answer = "Security sub-system internal error";
                                        break;
    case SEC_E_INVALID_HANDLE:          answer = "Security sub-system invalid handle";
                                        break;
    case SEC_E_INVALID_TOKEN:           answer = "Security token is not (no longer) valid";
                                        break;
    case SEC_E_LOGON_DENIED:            answer = "Logon denied";
                                        break;
    case SEC_E_NO_AUTHENTICATING_AUTHORITY: 
                                        answer = "Authentication authority (domain/realm) not contacted, incorrect or unavailable";
                                        break;
    case SEC_E_NO_CREDENTIALS:          answer = "No credentials presented";
                                        break;
    case SEC_E_OK:                      answer = "Security OK, token generated";
                                        break;
    case SEC_E_SECURITY_QOS_FAILED:     answer = "Quality of Security failed due to Digest authentication";
                                        break;
    case SEC_E_UNSUPPORTED_FUNCTION:    answer = "Security attribute / unsupported function error";
                                        break;
    case SEC_I_COMPLETE_AND_CONTINUE:   answer = "Complete and continue, pass token to client";
                                        break;
    case SEC_I_COMPLETE_NEEDED:         answer = "Send completion message to client";
                                        break;
    case SEC_I_CONTINUE_NEEDED:         answer = "Send token to client and wait for return";
                                        break;
    case STATUS_LOGON_FAILURE:          answer = "Logon tried but failed";
                                        break;
    default:                            answer = "Authentication failed for unknown reason";
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
  CString protocol = crypt.GetSSLProtocol(p_sslInfo->Protocol);
  CString cipher   = crypt.GetCipherType (p_sslInfo->CipherType);
  CString hash     = crypt.GetHashMethod (p_sslInfo->HashType);
  CString exchange = crypt.GetKeyExchange(p_sslInfo->KeyExchangeType);

  hash.MakeUpper();

  // Now log the cryptographic elements of the connection
  DETAILLOGV("Secure SSL Connection. Protocol [%s] Cipher [%s:%d] Hash [%s:%d] Key Exchange [%s:%d]"
            ,protocol.GetString()
            ,cipher.GetString(),  p_sslInfo->CipherStrength
            ,hash.GetString(),    p_sslInfo->HashStrength
            ,exchange.GetString(),p_sslInfo->KeyExchangeStrength);
}

// Handle text-based content-type messages
void
HTTPServer::HandleTextContent(HTTPMessage* p_message)
{
  uchar* body   = nullptr;
  size_t length = 0;
  bool   doBOM  = false;
  
  // Get a copy of the body
  p_message->GetRawBody(&body,length);

  int uni = IS_TEXT_UNICODE_UNICODE_MASK;   // Intel/AMD processors + BOM
  if(IsTextUnicode(body,(int)length,&uni))
  {
    CString output;

    // Find specific code page and try to convert
    CString charset = FindCharsetInContentType(p_message->GetContentType());
    DETAILLOGS("Try convert charset in pre-handle fase: ",charset);
    bool convert = TryConvertWideString(body,(int)length,"",output,doBOM);

    // Output to message->body
    if(convert)
    {
      // Copy back the converted string (does a copy!!)
      p_message->GetFileBuffer()->SetBuffer((uchar*)output.GetString(),output.GetLength());
      p_message->SetSendBOM(doBOM);
    }
    else
    {
      ERRORLOG(ERROR_NO_UNICODE_TRANSLATION,"Incoming text body: no translation");
    }
  }
  // Free raw buffer
  delete [] body;
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
      CString message;
      message.Format("Forgotten to call 'StartSite' for: %s",site.second->GetPrefixURL().GetString());
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
  CString fileName;
  
  // Getting the filename
  if(site)
  {
    fileName = site->GetWebroot() + p_msg->GetAbsoluteResource();
  }
  
  // Normalize pathname to MS-Windows
  fileName.Replace('/','\\');

  // Check for a name
  if(fileName.IsEmpty())
  {
    return false;
  }

  // See if the file is there (existence)
  if(_access(fileName,00) == 0)
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
      DETAILLOG1("Sending response: Not modified");
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
                                ,CString          p_url
                                ,CString&         p_path
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
    ERRORLOG(ERROR_ACCESS_DENIED,"Could not initialize event stream. Dropping stream.");
    delete stream;
    stream = nullptr;
  }
  return stream;
}

// Send to a server push event stream / deleting p_event
bool
HTTPServer::SendEvent(int p_port,CString p_site,ServerEvent* p_event,CString p_user /*=""*/)
{
  AutoCritSec lock(&m_eventLock);
  bool result = false;

  p_site.MakeLower();
  p_site.TrimRight('/');

  // Get the stream-string of the event
  CString sendString = EventToString(p_event);

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
    ERRORLOG(ERROR_FILE_NOT_FOUND,"SendEvent missing a stream or an event");
    return false;
  }

  // But we must now make sure the event stream still does exist
  // and has not been closed by the event monitor
  {
    AutoCritSec lock(&m_eventLock);
    if(!HasEventStream(p_stream))
    {
      return false;
    }
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
 
  // Produce the event string
  CString sendString = EventToString(p_event);

  // Send the event to the client. This can take an I/O wait time
  bool alive = SendResponseEventBuffer(p_stream->m_requestID,sendString.GetString(),sendString.GetLength(),p_continue);

  // Lock server as short as possible to register the return status
  // But we must now make sure the event stream still does exist
  {
    AutoCritSec lock(&m_eventLock);
    if(HasEventStream(p_stream))
    {
      p_stream->m_alive = alive;
    }
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

  // Tell what we just did
  if(MUSTLOG(HLL_LOGGING) && m_log && p_stream->m_alive)
  {
    CString text;
    text.Format("Sent event id: %d to client(s) on URL: ",p_event->m_id);
    text += p_stream->m_baseURL;
    if(MUSTLOG(HLL_LOGBODY))
    {
      text += "\n";
      text += p_event->m_data;
    }
    DETAILLOG1(text);

    if(MUSTLOG(HLL_TRACEDUMP))
    {
      m_log->AnalysisHex(__FUNCTION__,"SSE",(void*) p_event->m_data.GetString(),p_event->m_data.GetLength());
    }
  }

  // Ready with the event
  delete p_event;

  // Stream not alive, or stopping
  if(!p_stream->m_alive || !p_continue)
  {
    AbortEventStream(p_stream);
    return false;
  }

  // Delivered our event, but out of more datachunks for next requests
  if(p_stream->m_chunks > MAX_DATACHUNKS)
  {
    // Abort stream, so client will reopen a new stream
    AbortEventStream(p_stream);
    return false;
  }

  // Delivered the event at least to one stream
  return p_stream->m_alive;
}

// Form event to a stream string
CString
HTTPServer::EventToString(ServerEvent* p_event)
{
  CString stream;

  // Append client retry time to the first event
  if(p_event->m_id == 1)
  {
    stream.Format("retry: %u\n",p_event->m_id);
  }
  // Event naam if not standard 'message'
  if(!p_event->m_event.IsEmpty() && p_event->m_event.CompareNoCase("message"))
  {
    stream.AppendFormat("event:%s\n",p_event->m_event.GetString());
  }
  // Event ID if not zero
  if(p_event->m_id > 0)
  {
    stream.AppendFormat("id:%u\n",p_event->m_id);
  }
  if(!p_event->m_data.IsEmpty())
  {
    CString buffer = p_event->m_data;
    // Optimize our carriage returns
    if(buffer.Find('\r') >= 0)
    {
      buffer.Replace("\r\n","\n");
      buffer.Replace("\r","\n");
    }
    while(!buffer.IsEmpty())
    {
      int pos = buffer.Find('\n');
      if(pos < 0)
      {
        stream += "data:" + buffer + "\n";
        buffer.Empty();
      }
      else
      {
        ++pos;
        stream += "data:" + buffer.Left(pos);
        buffer = buffer.Mid(pos);
      }
    }
    // Catch the last line without a newline
    if(stream.Right(1) != "\n")
    {
      stream += "\n";
    }
  }
  stream += "\n";
  return stream;
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
  m_eventMonitor = (HANDLE)_beginthread(RunEventMonitor,0L,(void*)this);
}

void
HTTPServer::EventMonitor()
{
  DWORD streams  = 0;
  bool  startNew = false;

  DETAILLOG1("Event heartbeat monitor started");
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
  DETAILLOG1("Event heartbeat monitor stopped");

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
  // Create keep alive buffer
  CString keepAlive = ":keepalive\r\n\r\n";

  DETAILLOG1("Starting event heartbeat");

  // Pulse event stream with a comment
  for(auto& str : m_eventStreams)
  {
    EventStream* stream = str.second;
    // If we did not send anything for the last eventKeepAlive seconds,
    // Keep a margin of half a second for the wakeup of the server
    // we send a ":keepalive" comment to the clients
    if((pulse - stream->m_lastPulse) > (m_eventKeepAlive - 500))
    {
      stream->m_alive = SendResponseEventBuffer(stream->m_requestID,keepAlive.GetString(),keepAlive.GetLength());
      stream->m_lastPulse = pulse;
      ++stream->m_chunks;
      ++number;
    }
  }

  // What we just did
  DETAILLOGV("Sent heartbeat to %d push-event clients.",number);

  // Clean up dead event streams
  EventMap::iterator it = m_eventStreams.begin();
  while(it != m_eventStreams.end())
  {
    EventStream* stream = it->second;

    if(stream->m_chunks > MAX_DATACHUNKS)
    {
      DETAILLOGS("Push-event stream out of data chunks: ",stream->m_baseURL);

      // Send a close-stream event
      ServerEvent* event = new ServerEvent("close");
      SendEvent(stream,event);
      // Remove request from the request queue, closing the connection
      CancelRequestStream(stream->m_requestID);
      // Erase stream, it's out of chunks now
      delete it->second;
      it = m_eventStreams.erase(it);
    }
    else if(stream->m_alive == false)
    {
      DETAILLOGS("Abandoned push-event client from: ",stream->m_baseURL);

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

  // Monitor still needed?
  return (unsigned) m_eventStreams.size();
}

// Return the fact that we have an event stream
bool
HTTPServer::HasEventStream(EventStream* p_stream)
{
  AutoCritSec lock(&m_eventLock);

  for(auto& it : m_eventStreams)
  {
    if(p_stream == it.second)
    {
      return true;
    }
  }
  return false;
}

// Return the number of push-event-streams for this URL
int
HTTPServer::HasEventStreams(int p_port,CString p_url,CString p_user /*=""*/)
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
HTTPServer::CloseEventStream(EventStream* p_stream)
{
  AutoCritSec lock(&m_eventLock);

  for(auto& stream : m_eventStreams)
  {
    if(stream.second == p_stream)
    {
      // Show in the log
      DETAILLOGV("Closing event stream (user: %s) for URL: %s",p_stream->m_user.GetString(),p_stream->m_baseURL.GetString());
      if(stream.second->m_alive)
      {
        // Send a close-stream event
        // And close the stream by sending a close flag
        ServerEvent* event = new ServerEvent("close");
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
HTTPServer::CloseEventStreams(int p_port,CString p_url,CString p_user /*=""*/)
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

  CString name = p_service->GetName();
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
HTTPServer::UnRegisterService(CString p_serviceName)
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
HTTPServer::FindService(CString p_serviceName)
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
  CString uri = p_socket->GetURI();
  uri.MakeLower();

  TRACE("Register WebSocket [%s] %lX\n",uri.GetString(),p_socket);

  SocketMap::iterator it = m_sockets.find(uri);
  if(it != m_sockets.end())
  {
    // Drop the double socket. Removes socket from the mapping!
    it->second->CloseSocket();
  }
  m_sockets.insert(std::make_pair(uri,p_socket));
  return true;
}

// Remove registration of a WebSocket
bool
HTTPServer::UnRegisterWebSocket(WebSocket* p_socket)
{
  CString uri = p_socket->GetURI();
  uri.MakeLower();

  // TRACE("UN-Register WebSocket [%s] %lX\n",uri,p_socket);

  SocketMap::iterator it = m_sockets.find(uri);
  if(it != m_sockets.end())
  {
    delete it->second;
    m_sockets.erase(it);
    return true;
  }
  // We don't have it
  return false;
}

// Finding a previous registered websocket
WebSocket*
HTTPServer::FindWebSocket(CString p_uri)
{
  p_uri.MakeLower();

  TRACE("Find WebSocket: %s\n",p_uri.GetString());

  SocketMap::iterator it = m_sockets.find(p_uri);
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
HTTPServer::UnRegisterHTTPRequest(HTTPRequest* p_request)
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
HTTPServer::TraceKnownRequestHeader(unsigned p_number,const char* p_value)
{
  // See if known header is 'given'
  if (!p_value||*p_value==0)
  {
    return;
  }

  // Header fields are defined in HTTPMessage.cpp!!
  CString line = header_fields[p_number];
  line += ": ";
  line += p_value;
  m_log->BareStringLog(line.GetString(),line.GetLength());
}

void
HTTPServer::TraceRequest(PHTTP_REQUEST p_request)
{
  // Print HTTPSite context first before anything else of the call
  m_log->AnalysisLog(__FUNCTION__,LogType::LOG_TRACE,true,"Incoming call for site context: %lX",p_request->UrlContext);

  CString httpVersion;
  httpVersion.Format("HTTP/%d.%d",p_request->Version.MajorVersion,p_request->Version.MinorVersion);

  // The request verb.
  CString verb;
  if(p_request->Verb == HttpVerbUnknown && p_request->UnknownVerbLength)
  {
    verb = p_request->pUnknownVerb;
  }
  else
  {
    switch (p_request->Verb)
    {
      case HttpVerbOPTIONS:   verb = "OPTIONS";   break;
      case HttpVerbGET:       verb = "GET";       break;
      case HttpVerbHEAD:      verb = "HEAD";      break;
      case HttpVerbPOST:      verb = "POST";      break;
      case HttpVerbPUT:       verb = "PUT";       break;
      case HttpVerbDELETE:    verb = "DELETE";    break;
      case HttpVerbTRACE:     verb = "TRACE";     break;
      case HttpVerbCONNECT:   verb = "CONNECT";   break;
      case HttpVerbTRACK:     verb = "TRACK";     break;
      case HttpVerbMOVE:      verb = "MOVE";      break;
      case HttpVerbCOPY:      verb = "COPY";      break;
      case HttpVerbPROPFIND:  verb = "PROPFIND";  break;
      case HttpVerbPROPPATCH: verb = "PROPPATCH"; break;
      case HttpVerbMKCOL:     verb = "MKCOL";     break;
      case HttpVerbLOCK:      verb = "LOCK";      break;
      case HttpVerbUNLOCK:    verb = "UNLOCK";    break;
      case HttpVerbSEARCH:    verb = "SEARCH";    break;
      default:                verb = "UNKNOWN";   break;
    }
  }

  // THE PRINCIPAL HTTP PROTOCOL CALL LINE
  CString httpLine;
  httpLine.Format("%s %s %s",verb.GetString(),p_request->pRawUrl,httpVersion.GetString());
  m_log->BareStringLog(httpLine.GetString(),httpLine.GetLength());

  // Print all 'known' HTTP headers
  for (unsigned ind = 0; ind < HttpHeaderMaximum; ++ind)
  {
    TraceKnownRequestHeader(ind,p_request->Headers.KnownHeaders[ind].pRawValue);
  }

  // Print all 'unknown' headers
  for (unsigned ind = 0;ind < p_request->Headers.UnknownHeaderCount; ++ind)
  {
    CString uheader;
    uheader  = p_request->Headers.pUnknownHeaders[ind].pName;
    uheader += ": ";
    uheader += p_request->Headers.pUnknownHeaders[ind].pRawValue;
    m_log->BareStringLog(uheader.GetString(),uheader.GetLength());
  }

  // Print empty line between header and body (!!)
  m_log->BareStringLog("",0);

  // The following members are NOT used here

  //   ULONGLONG              BytesReceived;
  //   USHORT                 EntityChunkCount;
  //   PHTTP_DATA_CHUNK       pEntityChunks;
  //   HTTP_RAW_CONNECTION_ID RawConnectionId;
  //   PHTTP_SSL_INFO         pSslInfo;
}

void
HTTPServer::LogTraceRequest(PHTTP_REQUEST p_request,FileBuffer* p_buffer)
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
      LogTraceRequestBody(p_buffer);
    }
  }
}

void
HTTPServer::LogTraceRequestBody(FileBuffer* p_buffer)
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

    m_log->BareStringLog((const char*) buffer,(int) length);
    if(MUSTLOG(HLL_TRACEDUMP))
    {
      m_log->AnalysisHex(__FUNCTION__,"Incoming",buffer,(unsigned) length);
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
HTTPServer::TraceKnownResponseHeader(unsigned p_number,const char* p_value)
{
  // See if known header is 'given'
  if(!p_value || *p_value == 0)
  {
    return;
  }

  // Header fields are defined in HTTPMessage.cpp!!
  CString line = p_number < HttpHeaderAcceptRanges ? header_fields[p_number] : header_response[p_number - HttpHeaderAcceptRanges];
  line += ": ";
  line += p_value;
  m_log->BareStringLog(line.GetString(),line.GetLength());
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
  CString line;
  line.Format("HTTP/%d.%d %d %s"
             ,p_response->Version.MajorVersion
             ,p_response->Version.MinorVersion
             ,p_response->StatusCode
             ,p_response->pReason);
  m_log->AnalysisLog(__FUNCTION__,LogType::LOG_TRACE,false,"Full HTTP protocol");
  m_log->BareStringLog(line.GetString(),line.GetLength());

  // Print all 'known' HTTP headers
  for (unsigned ind = 0; ind < HttpHeaderResponseMaximum; ++ind)
  {
    TraceKnownResponseHeader(ind,p_response->Headers.KnownHeaders[ind].pRawValue);
  }

  // Print all 'unknown' headers
  if(p_response->Headers.pUnknownHeaders)
  {
    for(unsigned ind = 0;ind < p_response->Headers.UnknownHeaderCount; ++ind)
    {
      CString uheader;
      uheader = p_response->Headers.pUnknownHeaders[ind].pName;
      uheader += ": ";
      uheader += p_response->Headers.pUnknownHeaders[ind].pRawValue;
      m_log->BareStringLog(uheader.GetString(),uheader.GetLength());
    }
  }
  m_log->BareStringLog("",0);
}

void
HTTPServer::LogTraceResponse(PHTTP_RESPONSE p_response,FileBuffer* p_buffer)
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

        m_log->BareStringLog((const char*) buffer,(int) length);
        if(MUSTLOG(HLL_TRACEDUMP))
        {
          m_log->AnalysisHex(__FUNCTION__,"Outgoing",buffer,(unsigned) length);
        }
        // Delete buffer copy
        delete[] buffer;
      }
      else
      {
        m_log->AnalysisLog(__FUNCTION__,LogType::LOG_INFO,true,"Uploading FILE: %s",p_buffer->GetFileName().GetString());
      }
    }
  }
}

void
HTTPServer::LogTraceResponse(PHTTP_RESPONSE p_response,unsigned char* p_buffer,unsigned p_length)
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
    m_log->BareStringLog((const char*)p_buffer,p_length);
    if(MUSTLOG(HLL_TRACEDUMP))
    {
      m_log->AnalysisHex(__FUNCTION__,"Outgoing",p_buffer,p_length);
    }
  }
}

