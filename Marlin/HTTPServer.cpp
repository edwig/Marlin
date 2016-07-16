/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: HTTPServer.cpp
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
#include "WebServiceServer.h"
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

// Do not warn about formatting CStrings
#pragma warning(disable:6284)

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
#define DETAILLOG1(text)          if(m_detail && m_log) { DetailLog (__FUNCTION__,LogType::LOG_INFO,text); }
#define DETAILLOGS(text,extra)    if(m_detail && m_log) { DetailLogS(__FUNCTION__,LogType::LOG_INFO,text,extra); }
#define DETAILLOGV(text,...)      if(m_detail && m_log) { DetailLogV(__FUNCTION__,LogType::LOG_INFO,text,__VA_ARGS__); }
#define WARNINGLOG(text,...)      if(m_detail && m_log) { DetailLogV(__FUNCTION__,LogType::LOG_WARN,text,__VA_ARGS__); }
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
  // Cleanup the server objects
  Cleanup();

  DeleteCriticalSection(&m_eventLock);
  DeleteCriticalSection(&m_sitesLock);

  // Resetting the signal handlers
  ResetProcessAfterSEH();
}

CString
HTTPServer::GetVersion()
{
  return CString(MARLIN_SERVER_VERSION " on Microsoft HTTP-Server API/2.0");
}

// Initialise a HTTP server and server-session
bool
HTTPServer::Initialise()
{
  AutoCritSec lock(&m_sitesLock);

  // See if there is something to do!
  if(m_initialized)
  {
    return true;
  }

  // STEP 1: LOGGING
  // Init logging, so we can complain about errors
  InitLogging();

  // STEP 2: CHECKING OF SETTINGS
  if(GeneralChecks() == false)
  {
    return false;
  }
  
  // STEP 3: INIT THE SERVER
  // Initialize HTTP library as-a-server.
  // Specifying VERSION_2 ensures we have the version with authentication and SSL/TLS!!!
  // Fails on Windows-XP and Server 2003 and lower
  ULONG retCode = HttpInitialize(HTTPAPI_VERSION_2,HTTP_INITIALIZE_SERVER,NULL);
  if(retCode != NO_ERROR)
  {
    ERRORLOG(retCode,"HTTP Initialize");
    return false;
  }
  DETAILLOG1("HTTPInitialize OK");

  // Create a session 
  retCode = HttpCreateServerSession(HTTPAPI_VERSION_2,&m_session,0);
  if(retCode != NO_ERROR)
  {
    ERRORLOG(retCode,"CreateServerSession");
    return false;
  }
  DETAILLOGV("Serversession created: %I64X",m_session);

  // Create a request queue with NO name
  // Although we CAN create a name, it would mean a global object (and need to be unique)
  // So it's harder to create more than one server per machine
  retCode = HttpCreateRequestQueue(HTTPAPI_VERSION_2,NULL,NULL,0,&m_requestQueue);
  if(retCode != NO_ERROR)
  {
    ERRORLOG(retCode,"CreateRequestQueue");
    return false;
  }
  DETAILLOGV("Request queue created: %p",m_requestQueue);

  // Overrides for the HTTP Site. Test min/max via SetQueueLength
  int queueLength = m_webConfig.GetParameterInteger("Server","QueueLength",m_queueLength);
  SetQueueLength(queueLength);
  
  // Set backlog queue: using HttpSetRequestQueueProperty
  retCode = HttpSetRequestQueueProperty(m_requestQueue
                                       ,HttpServerQueueLengthProperty
                                       ,&m_queueLength
                                       ,sizeof(ULONG)
                                       ,0
                                       ,NULL);
  if(retCode != NO_ERROR)
  {
    ERRORLOG(retCode,"HTTP backlog queue length NOT set!");
  }
  else
  {
    DETAILLOGV("HTTP backlog queue set to length: %lu",m_queueLength);
  }

  // Set the 503-Verbosity: using HttpSetRequestQueueProperty
  DWORD verbosity = Http503ResponseVerbosityFull;
  retCode = HttpSetRequestQueueProperty(m_requestQueue
                                        ,HttpServer503VerbosityProperty
                                        ,&verbosity
                                        ,sizeof(DWORD)
                                        ,0
                                        ,NULL);
  if(retCode != NO_ERROR)
  {
    ERRORLOG(retCode,"Setting 503 verbosity property");
  }
  else
  {
    DETAILLOGV("HTTP 503-Error verbosity set to: %d",verbosity);
  }

  // Set the hard limits
  InitHardLimits();

  // Init the response headers to send
  InitHeaders();

  // We are airborne!
  return (m_initialized = true);
}

// Cleanup the server
void
HTTPServer::Cleanup()
{
  ULONG retCode;
  USES_CONVERSION;
  AutoCritSec lock1(&m_sitesLock);
  AutoCritSec lock2(&m_eventLock);

  // Remove all event streams within the scope of the eventLock
  for(auto& it : m_eventStreams)
  {
    delete it.second;
  }
  m_eventStreams.clear();

  // Remove all services
  while(!m_allServices.empty())
  {
    ServiceMap::iterator it = m_allServices.begin();
    it->second->Stop();
  }

  // Remove all URL's in the sites map
  while(!m_allsites.empty())
  {
    SiteMap::iterator it = m_allsites.begin();
    it->second->StopSite(true);
  }

  // Try to stop and remove all groups
  for(auto group : m_urlGroups)
  {
    // Gracefully stop
    group->StopGroup();
    // And delete memory
    delete group;
  }
  m_urlGroups.clear();

  // Close the Request Queue handle.
  // Do this before removing the group
  if(m_requestQueue)
  {
    // using: HttpCloseRequestQueue
    retCode = HttpCloseRequestQueue(m_requestQueue);
    m_requestQueue = NULL;

    if(retCode == NO_ERROR)
    {
      DETAILLOG1("Closed the request queue");
    }
    else
    {
      ERRORLOG(retCode,"Cannot close the request queue");
    }
  }
  
  // Now close the session
  if(m_session)
  {
    // Using: HttpCloseServerSession
    retCode   = HttpCloseServerSession(m_session);
    m_session = NULL;

    if(retCode == NO_ERROR)
    {
      DETAILLOG1("Closed the HTTP server session");
    }
    else
    {
      ERRORLOG(retCode,"Cannot close the HTTP server session");
    }
  }
  // Reset error state
  SetError(NO_ERROR);

  if(m_initialized)
  {
    // Call HttpTerminate.
    retCode = HttpTerminate(HTTP_INITIALIZE_SERVER,NULL);

    if(retCode == NO_ERROR)
    {
      DETAILLOG1("HTTP Server terminated OK");
    }
    else
    {
      ERRORLOG(retCode,"HTTP Server terminated with error");
    }
    m_initialized = false;
  }

  // Closing the logging file
  if(m_log && m_logOwner)
  {
    delete m_log;
    m_log = NULL;
  }
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
HTTPServer::DetailLog(const char* p_function,LogType p_type,const char* p_text)
{
  if(m_log && m_detail)
  {
    m_log->AnalysisLog(p_function,p_type,false,p_text);
  }
}

void
HTTPServer::DetailLogS(const char* p_function,LogType p_type,const char* p_text,const char* p_extra)
{
  if(m_log && m_detail)
  {
    CString text(p_text);
    text += p_extra;

    m_log->AnalysisLog(p_function,p_type,false,text);
  }
}

void
HTTPServer::DetailLogV(const char* p_function,LogType p_type,const char* p_text,...)
{
  if(m_log && m_detail)
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
    p_text.AppendFormat(" Error [%d] %s",p_code,GetLastErrorAsString(p_code));
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
    p_text.AppendFormat(" HTTP Status [%d] %s",p_status,GetStatusText(p_status));
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
  // Initial checks
  if(m_pool == nullptr)
  {
    // Cannot run without a pool
    ERRORLOG(ERROR_INVALID_PARAMETER,"Connect a threadpool for handling url's");
    return false;
  }
  DETAILLOG1("Threadpool checks out OK");

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
HTTPServer::InitLogging()
{
  // Check for a logging object
  if(m_log == NULL)
  {
    // Create a new one
    m_log = new LogAnalysis(m_name);
    m_logOwner = true;
  }
  CString file = m_log->GetLogFileName();
  int  cache   = m_log->GetCache();
  bool logging = m_log->GetDoLogging();
  bool timing  = m_log->GetDoTiming();
  bool events  = m_log->GetDoEvents();

  // Get parameters from web.config
  file     = m_webConfig.GetParameterString ("Logging","Logfile",  file);
  logging  = m_webConfig.GetParameterBoolean("Logging","DoLogging",logging);
  timing   = m_webConfig.GetParameterBoolean("Logging","DoTiming", timing);
  events   = m_webConfig.GetParameterBoolean("Logging","DoEvents", events);
  cache    = m_webConfig.GetParameterInteger("Logging","Cache",    cache);
  m_detail = m_webConfig.GetParameterBoolean("Logging","Detail",   m_detail);

  // Use if overridden in web.config
  if(!file.IsEmpty())
  {
    m_log->SetLogFilename(file);
  }
  m_log->SetCache(cache);
  m_log->SetDoLogging(logging);
  m_log->SetDoTiming(timing);
  m_log->SetDoEvents(events);
}

void
HTTPServer::SetLogging(LogAnalysis* p_log)
{
  if(m_log && m_logOwner)
  {
    delete m_log;
  }
  m_log = p_log;
  InitLogging();
}

// Initialise general server header settings
void
HTTPServer::InitHeaders()
{
  CString name = m_webConfig.GetParameterString("Server","ServerName","");
  CString type = m_webConfig.GetParameterString("Server","TypeServerName","Hide");

  // Server name combo
  if(type.CompareNoCase("Microsoft")   == 0) m_sendHeader = SendHeader::HTTP_SH_MICROSOFT;
  if(type.CompareNoCase("Marlin")      == 0) m_sendHeader = SendHeader::HTTP_SH_MARLIN;
  if(type.CompareNoCase("Application") == 0) m_sendHeader = SendHeader::HTTP_SH_APPLICATION;
  if(type.CompareNoCase("Configured")  == 0) m_sendHeader = SendHeader::HTTP_SH_WEBCONFIG;
  if(type.CompareNoCase("Hide")        == 0) m_sendHeader = SendHeader::HTTP_SH_HIDESERVER;

  if(m_sendHeader == SendHeader::HTTP_SH_WEBCONFIG)
  {
    m_configServerName = name;
    DETAILLOGS("Server sends 'server' response header of type: ",name);
  }
  else
  {
    DETAILLOGS("Server sends 'server' response header: ",type);
  }
}

// Initialise the hard server limits in bytes
void
HTTPServer::InitHardLimits()
{
  g_streaming_limit = m_webConfig.GetParameterInteger("Server","StreamingLimit",g_streaming_limit);
  g_compress_limit  = m_webConfig.GetParameterInteger("Server","CompressLimit", g_compress_limit);

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
HTTPServer::SetThreadPool(ThreadPool* p_pool)
{
  // Look for override in webconfig
  if(p_pool)
  {
    int minThreads = m_webConfig.GetParameterInteger("Server","MinThreads",p_pool->GetMinThreads());
    int maxThreads = m_webConfig.GetParameterInteger("Server","MaxThreads",p_pool->GetMaxThreads());
    int stackSize  = m_webConfig.GetParameterInteger("Server","StackSize", p_pool->GetStackSize());

    if(minThreads && minThreads >= NUM_THREADS_MINIMUM)
    {
      p_pool->TrySetMinimum(minThreads);
    }
    if(maxThreads && maxThreads <= NUM_THREADS_MAXIMUM)
    {
      p_pool->TrySetMaximum(maxThreads);
    }
    p_pool->SetStackSize(stackSize);
  }
  m_pool = p_pool;
}

void
HTTPServer::SetWebroot(CString p_webroot)
{
  // Look for override in webconfig
  m_webroot = m_webConfig.GetParameterString("Server","WebRoot",p_webroot);
}

HTTPSite*
HTTPServer::CreateSite(PrefixType    p_type
                      ,bool          p_secure
                      ,int           p_port
                      ,CString       p_baseURL
                      ,bool          p_subsite  /* = false */
                      ,LPFN_CALLBACK p_callback /* = NULL  */)
{
  // USE OVERRIDES FROM WEBCONFIG 
  // BUT USE PROGRAM'S SETTINGS AS DEFAULT VALUES
  
  CString chanType;
  int     chanPort;
  CString chanBase;
  bool    chanSecure;

  // Changing the input to a name
  switch(p_type)
  {
    case PrefixType::URLPRE_Strong: chanType = "strong";   break;
    case PrefixType::URLPRE_Named:  chanType = "named";    break;
    case PrefixType::URLPRE_FQN:    chanType = "full";     break;
    case PrefixType::URLPRE_Address:chanType = "address";  break;
    case PrefixType::URLPRE_Weak:   chanType = "weak";     break;
  }

  // Getting the settings from the web.config, use parameters as defaults
  chanType      = m_webConfig.GetParameterString ("Server","ChannelType",chanType);
  chanSecure    = m_webConfig.GetParameterBoolean("Server","Secure",     p_secure);
  chanPort      = m_webConfig.GetParameterInteger("Server","Port",       p_port);
  chanBase      = m_webConfig.GetParameterString ("Server","BaseURL",    p_baseURL);

  // Recalculate the type
       if(chanType.CompareNoCase("strong")  == 0) p_type = PrefixType::URLPRE_Strong;
  else if(chanType.CompareNoCase("named")   == 0) p_type = PrefixType::URLPRE_Named;
  else if(chanType.CompareNoCase("full")    == 0) p_type = PrefixType::URLPRE_FQN;
  else if(chanType.CompareNoCase("address") == 0) p_type = PrefixType::URLPRE_Address;
  else if(chanType.CompareNoCase("weak")    == 0) p_type = PrefixType::URLPRE_Weak;

  // Only ports out of IANA/IETF range permitted!
  if(chanPort >= 1024 || 
     chanPort == INTERNET_DEFAULT_HTTP_PORT ||
     chanPort == INTERNET_DEFAULT_HTTPS_PORT )
  {
    p_port = chanPort;
  }
  // Only use other URL if one specified
  if(!chanBase.IsEmpty())
  {
    p_baseURL = chanBase;
  }

  // Create our URL prefix
  CString prefix = CreateURLPrefix(p_type,chanSecure,p_port,p_baseURL);
  if(!prefix.IsEmpty())
  {
    HTTPSite* mainSite = nullptr;
    if(p_subsite)
    {
      // Do sub-site lookup on receiving messages
      m_hasSubsites = true;
      // Finding the main site of this sub-site
      mainSite = FindHTTPSite(p_port,p_baseURL);
      if(mainSite == nullptr)
      {
        // No luck: No main site to register against
        CString message;
        message.Format("Tried to register a sub-site, without a main-site: %s",p_baseURL);
        ERRORLOG(ERROR_NOT_FOUND,message);
        return nullptr;
      }
    }
    // Create and register a URL
    HTTPSite* registeredSite = RegisterSite(prefix,p_port,p_baseURL,p_callback,mainSite);

    if(registeredSite != nullptr)
    {
      // Site created and registered
      return registeredSite;
    }
  }
  // No luck
  return nullptr;
}

CString
HTTPServer::MakeSiteRegistrationName(int p_port,CString p_url)
{
  CString registration;
  p_url.MakeLower();
  p_url.TrimRight('/');
  registration.Format("%d:%s",p_port,p_url);

  return registration;
}


// Delete a channel (from prefix OR base URL forms)
bool
HTTPServer::DeleteSite(int p_port,CString p_baseURL,bool p_force /*=false*/)
{
  AutoCritSec lock(&m_sitesLock);
  // Default result
  bool result = false;

  // Use counter
  m_counter.Start();

  CString search(MakeSiteRegistrationName(p_port,p_baseURL));
  SiteMap::iterator it = m_allsites.find(search);
  if(it != m_allsites.end())
  {
    // Finding our site
    HTTPSite* site = it->second;

    // See if other sites are dependent on this one
    if(p_force == false && site->GetIsSubsite() == false)
    {
      // Walk all sites, to see if subsites still dependent on this main site
      for(SiteMap::iterator fit = m_allsites.begin(); fit != m_allsites.end(); ++fit)
      {
        if(fit->second->GetMainSite() == site)
        {
          // Cannot delete this site, other sites are dependent on this one
          ERRORLOG(ERROR_ACCESS_DENIED,"Cannot remove site. Sub-sites still dependent on: " + p_baseURL);
          m_counter.Stop();
          return false;
        }
      }
    }
    // Remove a site from the URL group, so no 
    // HTTP calls will be accepted any more
    result = site->RemoveSiteFromGroup();
    // And remove from the site map
    if(result || p_force)
    {
      delete site;
      m_allsites.erase(it);
      result = true;
    }
  }
  // Use counter
  m_counter.Stop();

  return result;
}

// Register a URL to listen on
HTTPSite*
HTTPServer::RegisterSite(CString        p_urlPrefix
                        ,int            p_port
                        ,CString        p_baseURL
                        ,LPFN_CALLBACK  p_callback
                        ,HTTPSite*      p_mainSite /*=nullptr*/)
{
  AutoCritSec lock(&m_sitesLock);

  // Use counter
  m_counter.Start();

  if(GetLastError())
  {
    ERRORLOG(ERROR_INVALID_PARAMETER,"RegisterSite called too early");
    return nullptr;
  }

  // See to it that we are initialized
  Initialise();
  if(GetLastError())
  {
    // If initialize did not work out OK
    m_counter.Stop();
    return nullptr;
  }

  // Remember our context
  CString site(MakeSiteRegistrationName(p_port,p_baseURL));
  SiteMap::iterator it = m_allsites.find(site);
  if(it != m_allsites.end())
  {
    if(p_urlPrefix.CompareNoCase(it->second->GetPrefixURL()) == 0)
    {
      // Duplicate site found
      ERRORLOG(ERROR_ALIAS_EXISTS,p_baseURL);
      return nullptr;
    }
  }

  // Remember URL Prefix strings, and create the site
  HTTPSite* context = new HTTPSite(this,p_port,p_baseURL,p_urlPrefix,p_mainSite,p_callback);
  if(context)
  {
    m_allsites[site] = context;
  }
  // Use counter
  m_counter.Stop();

  return context;
}

// Find and make an URL group
HTTPURLGroup* 
HTTPServer::FindUrlGroup(CString p_authName
                        ,ULONG   p_authScheme
                        ,bool    p_cache
                        ,CString p_realm
                        ,CString p_domain)
{
  // See if we already have a group of these combination
  // And if so: reuse that URL group
  for(auto group : m_urlGroups)
  {
    if(group->GetAuthenticationScheme()    == p_authScheme &&
       group->GetAuthenticationNtlmCache() == p_cache      &&
       group->GetAuthenticationRealm()     == p_realm      &&
       group->GetAuthenticationDomain()    == p_domain     )
    {
      DETAILLOGS("URL Group recycled for authentication scheme: ",p_authName);
      return group;
    }
  }

  // No group found, create a new one
  HTTPURLGroup* group = new HTTPURLGroup(this,p_authName,p_authScheme,p_cache,p_realm,p_domain);

  // Start the group
  if(group->StartGroup())
  {
    m_urlGroups.push_back(group);
    int number = (int) m_urlGroups.size();
    DETAILLOGV("Created URL group [%d] for authentication: %s",number,p_authName);
  }
  else
  {
    ERRORLOG(ERROR_CANNOT_MAKE,"URL Group ***NOT*** created for authentication: " + p_authName);
    delete group;
    group = nullptr;
  }
  return group;
}

// Remove an URLGroup. Called by HTTPURLGroup itself
void
HTTPServer::RemoveURLGroup(HTTPURLGroup* p_group)
{
  for(URLGroupMap::iterator it = m_urlGroups.begin(); it != m_urlGroups.end(); ++it)
  {
    if(p_group == *it)
    {
      m_urlGroups.erase(it);
      return;
    }
  }
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

static unsigned int 
__stdcall StartingTheServer(void* pParam)
{
  HTTPServer* server = reinterpret_cast<HTTPServer*>(pParam);
  server->RunHTTPServer();
  return 0;
}

// Running the server in a separate thread
void
HTTPServer::Run()
{
  if(m_serverThread == NULL)
  {
    // Base thread of the server
    unsigned int threadID;
    if((m_serverThread = (HANDLE) _beginthreadex(NULL, 0,StartingTheServer,(void *)(this),0,&threadID)) == INVALID_HANDLE_VALUE)
    {
      m_serverThread = NULL;
      tls_lastError  = GetLastError();
      ERRORLOG(tls_lastError,"Cannot create a thread for the MarlinServer.");
    }
  }
}

// Running the main loop of the webserver
void
HTTPServer::RunHTTPServer()
{
  ULONG              result    = 0;
  DWORD              bytesRead = 0;
  bool               doReceive = true;
  HTTP_REQUEST_ID    requestId = NULL;
  PHTTP_REQUEST      request   = nullptr;
  PCHAR              requestBuffer = nullptr;
  ULONG              requestBufferLength = 0;
  HTTPMessage*       message = NULL;
  USES_CONVERSION;

  // Use counter
  m_counter.Start();

  Initialise();

  // See if we are in a state to receive requests
  if(GetLastError() || !m_initialized)
  {
    ERRORLOG(ERROR_INVALID_PARAMETER,"RunHTTPServer called too early");
    return;
  }
  DETAILLOG1("HTTPServer initialised and ready to go!");

  // Check if all sites were properly started
  // Catches programmers who forget to call HTTPSite::StartSite()
  CheckSitesStarted();

  // Allocate a 32 KB buffer. This size should work for most 
  // requests. The buffer size can be increased if required. Space
  // is also required for an HTTP_REQUEST structure.
  requestBufferLength = sizeof(HTTP_REQUEST) + INIT_HTTP_BUFFERSIZE;
  requestBuffer       = (PCHAR) new uchar[requestBufferLength];

  if(requestBuffer == NULL)
  {
    ERRORLOG(ERROR_NOT_ENOUGH_MEMORY,"Out of memory");
    return;
  }
  DETAILLOGV("Request buffer allocated [%d] bytes",requestBufferLength);

  // Buffer where we receive our requests
  request = (PHTTP_REQUEST)requestBuffer;

  // Wait for a new request. This is indicated by a NULL request ID.
  HTTP_SET_NULL_ID(&requestId);

  // START OUR MAIN LOOP
  m_running = true;
  DETAILLOG1("HTTPServer entering main loop");

  while(m_running)
  {
    // New receive in the buffer
    RtlZeroMemory(request,requestBufferLength);

    // Use counter
    m_counter.Stop();
    // Sit tight for the next request
    result = HttpReceiveHttpRequest(m_requestQueue,     // Request Queue
                                    requestId,          // Request ID
                                    0,                  // Flags
                                    request,            // HTTP request buffer
                                    requestBufferLength,// request buffer length
                                    &bytesRead,         // bytes received
                                    NULL);              // LPOVERLAPPED
    // Use counter
    m_counter.Start();

    // Grab the senders content
    CString   acceptTypes    = request->Headers.KnownHeaders[HttpHeaderAccept         ].pRawValue;
    CString   contentType    = request->Headers.KnownHeaders[HttpHeaderContentType    ].pRawValue;
    CString   acceptEncoding = request->Headers.KnownHeaders[HttpHeaderAcceptEncoding ].pRawValue;
    CString   cookie         = request->Headers.KnownHeaders[HttpHeaderCookie         ].pRawValue;
    CString   authorize      = request->Headers.KnownHeaders[HttpHeaderAuthorization  ].pRawValue;
    CString   modified       = request->Headers.KnownHeaders[HttpHeaderIfModifiedSince].pRawValue;
    CString   rawUrl         = CW2A(request->CookedUrl.pFullUrl);
    PSOCKADDR sender         = request->Address.pRemoteAddress;
    int       remDesktop     = FindRemoteDesktop(request->Headers.UnknownHeaderCount
                                                ,request->Headers.pUnknownHeaders);

    // Log earliest as possible
    DETAILLOGV("Received HTTP call from [%s] with length: %I64u"
              ,SocketToServer((PSOCKADDR_IN6)sender)
              ,request->BytesReceived);

    // Test if server already stopped, and we are here because of the stopping
    if(m_running == false)
    {
      ERRORLOG(ERROR_HANDLE_EOF,"HTTPServer stopped in ReceiveHttpRequest.");
      break;
    }

    if(result == NO_ERROR)
    {
      HANDLE accessToken = NULL;
      // Log incoming request
      DETAILLOGS("Got a request for: ",rawUrl);

      // FInding the site
      bool eventStream = false;
      LPFN_CALLBACK callback = nullptr;
      HTTPSite* site = reinterpret_cast<HTTPSite*>(request->UrlContext);
      if(site)
      {
        callback    = site->GetCallback();
        eventStream = site->GetIsEventStream();
      }

      // See if we must substitute for a sub-site
      if(m_hasSubsites)
      {
        CString absPath = CW2A(request->CookedUrl.pAbsPath);
        site = FindHTTPSite(site,absPath);
      }

      if(request->RequestInfoCount > 0)
      {
        for(unsigned ind = 0; ind < request->RequestInfoCount; ++ind)
        {
          if(request->pRequestInfo[ind].InfoType == HttpRequestInfoTypeAuth) 
          {
            // Default is failure
            doReceive = false;

            PHTTP_REQUEST_AUTH_INFO auth = (PHTTP_REQUEST_AUTH_INFO)request->pRequestInfo[ind].pInfo;
            if(auth->AuthStatus == HttpAuthStatusNotAuthenticated)
            {
              // Not (yet) authenticated. Back to the client for authentication
              DETAILLOGS("Not yet authenticated for: ",rawUrl);
              result = RespondWithClientError(site,request->RequestId,HTTP_STATUS_DENIED,"Not authenticated",site->GetAuthenticationScheme());
            }
            else if(auth->AuthStatus == HttpAuthStatusFailure)
            {
              // Second round. Still not authenticated. Drop the connection, better next time
              DETAILLOGS("Authentication failed for: ",rawUrl);
              DETAILLOGV("Authentication failed because of: %s",AuthenticationStatus(auth->SecStatus));
              result = RespondWithClientError(site,request->RequestId,HTTP_STATUS_DENIED,"Not authenticated",site->GetAuthenticationScheme());
              // Go to next request
              HTTP_SET_NULL_ID(&requestId);
            }
            else if(auth->AuthStatus == HttpAuthStatusSuccess)
            {
              // Authentication accepted: all is well
              DETAILLOGS("Authentication done for: ",rawUrl);
              accessToken = auth->AccessToken;
              doReceive   = true;
            }
            else
            {
              CString authError;
              authError.Format("Authentication mechanism failure. Unknown status: %d",auth->AuthStatus);
              ERRORLOG(ERROR_NOT_AUTHENTICATED,authError);
            }
          }
          else if(request->pRequestInfo[ind].InfoType == HttpRequestInfoTypeSslProtocol)
          {
            // Only exists on Windows 10 / Server 2016
            if(m_detail)
            {
              PHTTP_SSL_PROTOCOL_INFO sslInfo = (PHTTP_SSL_PROTOCOL_INFO)request->pRequestInfo[ind].pInfo;
              LogSSLConnection(sslInfo);
            }
          }
        }
      }
      if(doReceive)
      {
        // Remember the context: easy in API 2.0
        if(callback == nullptr && site == nullptr)
        {
          SendResponse(site
                      ,request->RequestId
                      ,HTTP_STATUS_NOT_FOUND
                      ,"URL not found"
                      ,(PSTR)rawUrl.GetString()
                      ,site->GetAuthenticationScheme());
          // Ready with this request
          HTTP_SET_NULL_ID(&requestId);
          continue;
        }

        // Translate the command. Now reduced to just this switch
        HTTPCommand type = HTTPCommand::http_no_command;
        switch(request->Verb)
        {
          case HttpVerbOPTIONS:   type = HTTPCommand::http_options;    break;
          case HttpVerbGET:       type = HTTPCommand::http_get;        break;
          case HttpVerbHEAD:      type = HTTPCommand::http_head;       break;
          case HttpVerbPOST:      type = HTTPCommand::http_post;       break;
          case HttpVerbPUT:       type = HTTPCommand::http_put;        break;
          case HttpVerbDELETE:    type = HTTPCommand::http_delete;     break;
          case HttpVerbTRACE:     type = HTTPCommand::http_trace;      break;
          case HttpVerbCONNECT:   type = HTTPCommand::http_connect;    break;
          case HttpVerbMOVE:      type = HTTPCommand::http_move;       break;
          case HttpVerbCOPY:      type = HTTPCommand::http_copy;       break;
          case HttpVerbPROPFIND:  type = HTTPCommand::http_proppfind;  break;
          case HttpVerbPROPPATCH: type = HTTPCommand::http_proppatch;  break;
          case HttpVerbMKCOL:     type = HTTPCommand::http_mkcol;      break;
          case HttpVerbLOCK:      type = HTTPCommand::http_lock;       break;
          case HttpVerbUNLOCK:    type = HTTPCommand::http_unlock;     break;
          case HttpVerbSEARCH:    type = HTTPCommand::http_search;     break;
          default:                // Try to get a less known verb as 'last resort'
                                  type = GetUnknownVerb(request->pUnknownVerb);
                                  if(type == HTTPCommand::http_no_command)
                                  {
                                    // Non implemented like HttpVerbTRACK or other non-known verbs
                                    RespondWithServerError(site,request->RequestId,HTTP_STATUS_NOT_SUPPORTED,"Not implemented","");
                                    // Ready with this request
                                    HTTP_SET_NULL_ID(&requestId);
                                    continue;
                                  }
                                  break;
        }

        // Receiving the initiation of an event stream for the server
        acceptTypes.Trim();
        if((type == HTTPCommand::http_get) && (eventStream || acceptTypes.Left(17).CompareNoCase("text/event-stream") == 0))
        {
          CString absolutePath = CW2A(request->CookedUrl.pAbsPath);
          EventStream* stream = SubscribeEventStream(site,site->GetSite(), absolutePath,request->RequestId,accessToken);
          if(stream)
          {
            stream->m_baseURL = rawUrl;
            m_pool->SubmitWork(callback,(void*)stream);
            HTTP_SET_NULL_ID(&requestId);
            continue;
          }
        }

        // For all types of requests: Create the HTTPMessage
        message = new HTTPMessage(type,site);
        message->SetURL(rawUrl);
        message->SetAuthorization(authorize);
        message->SetRequestHandle(request->RequestId);
        message->SetConnectionID(request->ConnectionId);
        message->SetContentType(contentType);
        message->SetAccessToken(accessToken);
        message->SetRemoteDesktop(remDesktop);
        message->SetSender((PSOCKADDR_IN6)sender);
        message->SetCookiePairs(cookie);
        message->SetAcceptEncoding(acceptEncoding);
        if(site->GetAllHeaders())
        {
          // If requested so, copy all headers to the message
          message->SetAllHeaders(&request->Headers);
        }
        else
        {
          // As a minimum, always add the unknown headers
          // in case of a 'POST', as the SOAPAction header is here too!
          message->SetUnknownHeaders(&request->Headers);
        }

        // Handle modified-since 
        // Rest of the request is then not needed any more
        if(type == HTTPCommand::http_get && !modified.IsEmpty())
        {
          message->SetHTTPTime(modified);
          if(DoIsModifiedSince(message))
          {
            // Answer already sent, go on to the next request
            HTTP_SET_NULL_ID(&requestId);
            continue;
          }
        }

        // Find X-HTTP-Method VERB Tunneling
        if(type == HTTPCommand::http_post && site->GetVerbTunneling())
        {
          if(message->FindVerbTunneling())
          {
            DETAILLOGV("Request VERB changed to: %s",message->GetVerb());
          }
        }

        // Remember the fact that we should read the rest of the message
        message->SetReadBuffer(request->Flags & HTTP_REQUEST_FLAG_MORE_ENTITY_BODY_EXISTS);

        // Hit the thread pool with this message
        m_pool->SubmitWork(callback,(void*)message);

        // Ready with this request
        HTTP_SET_NULL_ID(&requestId);
      }
    }
    else if(result == ERROR_MORE_DATA)
    {
      // The input buffer was too small to hold the request headers.
      // Increase the buffer size and call the API again. 
      //
      // When calling the API again, handle the request
      // that failed by passing a RequestID.
      // This RequestID is read from the old buffer.
      requestId = request->RequestId;

      // Free the old buffer and allocate a new buffer.
      requestBufferLength = bytesRead;
      delete [] requestBuffer;
      requestBuffer = (PCHAR) new uchar[requestBufferLength];
      if(requestBuffer == NULL)
      {
        ERRORLOG(ERROR_NOT_ENOUGH_MEMORY,"Out of memory");
        break;
      }
      request = (PHTTP_REQUEST)requestBuffer;

      DETAILLOGV("Request buffer length expanded to [%d] bytes",requestBufferLength);
    }
    else if(ERROR_CONNECTION_INVALID == result && !HTTP_IS_NULL_ID(&requestId))
    {
      // The TCP connection was corrupted by the peer when
      // attempting to handle a request with more buffer. 
      // Continue to the next request.
      HTTP_SET_NULL_ID(&requestId);
    }
    else if(result == ERROR_OPERATION_ABORTED)
    {
      if(m_running)
      {
        // Not called by StopServer
        ERRORLOG(result,"ReceiveHttpRequest aborted");
      }
    }
    else // ERROR_NO_ACCESS / ERROR_HANDLE_EOF / ERROR_INVALID_PARAMETER
    {
      // Any other error
      ERRORLOG(result,"ReceiveHttpRequest");
    }

    // Do error handler at the end of the mainloop
    if(GetLastError())
    {
      m_log->AnalysisLog(__FUNCTION__, LogType::LOG_ERROR,true,"HTTP Mainloop: %d",GetLastError());
      SetError(NO_ERROR);
    }
  }
  // Free the request buffer
  if(requestBuffer)
  {
    delete [] requestBuffer;
    DETAILLOG1("Dropped request buffer");
  }

  // Use the counter
  m_counter.Stop();

  // Last action in the server thread
  m_serverThread = 0;
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

void
HTTPServer::InitializeHttpResponse(HTTP_RESPONSE* p_response,USHORT p_status,PSTR p_reason)
{
  RtlZeroMemory(p_response, sizeof(HTTP_RESPONSE));
  p_response->StatusCode   = p_status;
  p_response->pReason      = p_reason;
  p_response->ReasonLength = (USHORT) strlen(p_reason);
}

DWORD
HTTPServer::SendResponse(HTTPSite*       p_site
                        ,HTTP_REQUEST_ID p_requestID
                        ,USHORT          p_statusCode
                        ,PSTR            p_reason
                        ,PSTR            p_entityString
                        ,CString         p_authScheme
                        ,PSTR            p_cookie      /*=NULL*/
                        ,PSTR            p_contentType /*=NULL*/)
{
  HTTP_RESPONSE   response;
  HTTP_DATA_CHUNK dataChunk;
  DWORD           result;
  DWORD           bytesSent;
  CString         challenge;

  // Initialize the HTTP response structure.
  InitializeHttpResponse(&response,p_statusCode,p_reason);

  // Add a known header.
  if(p_contentType && strlen(p_contentType))
  {
    AddKnownHeader(response,HttpHeaderContentType,p_contentType);
  }
  else
  {
    AddKnownHeader(response,HttpHeaderContentType,"text/html");
  }
  // Adding authentication schema challenge
  if(p_statusCode == HTTP_STATUS_DENIED)
  {
    // Add authentication scheme
    challenge = BuildAuthenticationChallenge(p_authScheme,p_site->GetAuthenticationRealm());
    AddKnownHeader(response,HttpHeaderWwwAuthenticate,challenge);
  }
  else if (p_statusCode)
  {
    // Log responding with error status code
    HTTPError(__FUNCTION__,p_statusCode,"Returning from: " + p_site->GetSite());
  }

  // Adding the body
  if(p_entityString && strlen(p_entityString) > 0)
  {
    // Add an entity chunk.
    dataChunk.DataChunkType           = HttpDataChunkFromMemory;
    dataChunk.FromMemory.pBuffer      = p_entityString;
    dataChunk.FromMemory.BufferLength = (ULONG) strlen(p_entityString);
    response.EntityChunkCount         = 1;
    response.pEntityChunks            = &dataChunk;
  }

  // Adding a cookie
  if(p_cookie && strlen(p_cookie) > 0)
  {
    AddKnownHeader(response,HttpHeaderSetCookie,p_cookie);
  }

  // Prepare our cache-policy
  HTTP_CACHE_POLICY policy;
  policy.Policy        = m_policy;
  policy.SecondsToLive = m_secondsToLive;

  // Because the entity body is sent in one call, it is not
  // required to specify the Content-Length.
  result = HttpSendHttpResponse(m_requestQueue,      // ReqQueueHandle
                                p_requestID,         // Request ID
                                0,                   // Flags
                                &response,           // HTTP response
                                &policy,             // Policy
                                &bytesSent,          // bytes sent  (OPTIONAL)
                                NULL,                // pReserved2  (must be NULL)
                                0,                   // Reserved3   (must be 0)
                                NULL,                // LPOVERLAPPED(OPTIONAL)
                                NULL                 // pReserved4  (must be NULL)
                                ); 
  if(result != NO_ERROR)
  {
    ERRORLOG(result,"SendHttpResponse");
  }
  else
  {
    DETAILLOGV("HTTP Send %d %s %s",p_statusCode,p_reason,p_entityString);
  }
  return result;
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
    challenge = "Basic realm=\"BasicRealm\"";
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
    challenge.Format("Digest realm=\"%s\"",p_realm);
  }
  else if(p_authScheme.CompareNoCase("Kerberos") == 0)
  {
    challenge = "Kerberos";
  }
  return challenge;
}

// Response in the server error range (500-505)
DWORD
HTTPServer::RespondWithServerError(HTTPSite*       p_site
                                  ,HTTP_REQUEST_ID p_requestID
                                  ,int             p_error
                                  ,CString         p_reason
                                  ,CString         p_authScheme
                                  ,CString         p_cookie /*=""*/)
{
  HTTPERROR(p_error,"Respond with server error");
  CString page;
  page.Format(m_serverErrorPage,p_error,p_reason);
  return SendResponse(p_site
                     ,p_requestID
                     ,(USHORT)p_error
                     ,(PSTR)GetStatusText(p_error)
                     ,(PSTR)page.GetString()
                     ,p_authScheme
                     ,(PSTR)p_cookie.GetString());
}

// Response in the client error range (400-417)
DWORD
HTTPServer::RespondWithClientError(HTTPSite*       p_site
                                  ,HTTP_REQUEST_ID p_requestID
                                  ,int             p_error
                                  ,CString         p_reason
                                  ,CString         p_authScheme
                                  ,CString         p_cookie /*=""*/)
{
  HTTPERROR(p_error,"Respond with client error");
  CString page;
  page.Format(m_clientErrorPage,p_error,p_reason);
  return SendResponse(p_site
                     ,p_requestID
                     ,(USHORT)p_error
                     ,(PSTR)GetStatusText(p_error)
                     ,(PSTR)page.GetString()
                     ,p_authScheme
                     ,(PSTR)p_cookie.GetString());
}

// Receive incoming HTTP request
bool
HTTPServer::ReceiveIncomingRequest(HTTPMessage* p_message)
{
  bool   retval    = true;
  ULONG  bytesRead = 0;
  ULONG  entityBufferLength = INIT_HTTP_BUFFERSIZE;
  PUCHAR entityBuffer       = nullptr;

  // Create a buffer + 1 extra byte for the closing 0
  entityBuffer = new uchar[entityBufferLength + 1];
  if(entityBuffer == NULL)
  {
    ERRORLOG(ERROR_NOT_ENOUGH_MEMORY,"Out of memory");
    return false;
  }

  // Reading loop
  bool reading = true;
  do 
  {
    bytesRead = 0; 
    DWORD result = HttpReceiveRequestEntityBody(m_requestQueue
                                                ,p_message->GetRequestHandle()
                                                ,HTTP_RECEIVE_REQUEST_ENTITY_BODY_FLAG_FILL_BUFFER
                                                ,entityBuffer
                                                ,entityBufferLength
                                                ,&bytesRead
                                                ,NULL);
    switch(result)
    {
      case NO_ERROR:          // Regular incoming body part
                              entityBuffer[bytesRead] = 0;
                              p_message->AddBody(entityBuffer,bytesRead);
                              DETAILLOGV("ReceiveRequestEntityBody [%d] bytes",bytesRead);
                              break;
      case ERROR_HANDLE_EOF:  // Very last incoming body part
                              if(bytesRead)
                              {
                                entityBuffer[bytesRead] = 0;
                                p_message->AddBody(entityBuffer,bytesRead);
                                DETAILLOGV("ReceiveRequestEntityBody [%d] bytes",bytesRead);
                              }
                              reading = false;
                              break;
      default:                ERRORLOG(result,"ReceiveRequestEntityBody");
                              reading = false;
                              retval  = false;
                              break;
                              
    }

  } 
  while(reading);

  // Clean up buffer after reading
  delete [] entityBuffer;
  entityBuffer = nullptr;

  // In case of a POST, try to convert charset before submitting to site
  if(p_message->GetCommand() == HTTPCommand::http_post)
  {
    if(p_message->GetContentType().Find("multipart") <= 0)
    {
      HandleTextContent(p_message);
    }
  }
  DETAILLOGV("Received %s message from: %s Size: %lu"
            ,headers[(unsigned)p_message->GetCommand()]
            ,(LPCTSTR)SocketToServer(p_message->GetSender())
            ,p_message->GetBodyLength());

  // This message is read!
  p_message->SetReadBuffer(false);

  // Receiving succeeded?
  return retval;
}

// Add a well known HTTP header to the response structure
void
HTTPServer::AddKnownHeader(HTTP_RESPONSE& p_response,HTTP_HEADER_ID p_header,const char* p_value)
{
  p_response.Headers.KnownHeaders[p_header].pRawValue      = p_value;
  p_response.Headers.KnownHeaders[p_header].RawValueLength = (USHORT) strlen(p_value);
}

PHTTP_UNKNOWN_HEADER 
HTTPServer::AddUnknownHeaders(UKHeaders& p_headers)
{
  // Something to do?
  if(p_headers.empty())
  {
    return nullptr;
  }
  // Alloc some space
  PHTTP_UNKNOWN_HEADER header = new HTTP_UNKNOWN_HEADER [p_headers.size()];

  unsigned ind = 0;
  UKHeaders::iterator it = p_headers.begin();
  while(it != p_headers.end())
  {
    CString name  = it->first;
    CString value = it->second;

    header[ind].NameLength     = (USHORT) name.GetLength();
    header[ind].RawValueLength = (USHORT) value.GetLength();
    header[ind].pName          = name.GetString();
    header[ind].pRawValue      = value.GetString();

    // next header
    ++ind;
    ++it;
  }
  return header;
}

// Sending response for an incoming message
void
HTTPServer::SendResponse(SOAPMessage* p_message)
{
  // This message already sent, or not able to send: no request is known
  if(p_message->GetRequestHandle() == NULL)
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
    DETAILLOGS("Send SOAP response:\n",p_message->GetSoapMessage());
  }

  // Convert to a HTTP response
  HTTPMessage answer(HTTPCommand::http_response,p_message);
  // Set status in case of an error
  if(p_message->GetErrorState())
  {
    answer.SetStatus(HTTP_STATUS_BAD_REQUEST);
    DETAILLOGS("Send SOAP FAULT response:\n", p_message->GetSoapMessage());
  }

  // Send the HTTP Message as response
  SendResponse(&answer);

  // Do **NOT** send an answer twice
  p_message->SetRequestHandle(NULL);
}

// Sending response for an incoming message
void
HTTPServer::SendResponse(JSONMessage* p_message)
{
  // This message already sent, or not able to send: no request is known
  if(p_message->GetRequestHandle() == NULL)
  {
    ERRORLOG(ERROR_INVALID_PARAMETER,"SendResponse: nothing to send");
    return;
  }

  // Check if we must restore the encryption
  if(p_message->GetErrorState())
  {
    DETAILLOGS("Send JSON FAULT response:\n",p_message->GetJsonMessage());
    HTTPSite* site = p_message->GetHTTPSite();
    SendResponse(site,p_message->GetRequestHandle(),HTTP_STATUS_BAD_REQUEST,"Bad JSON request","","");
  }
  else
  {
    DETAILLOGS("Send JSON response:\n",p_message->GetJsonMessage());
    // Convert to a HTTP response
    HTTPMessage answer(HTTPCommand::http_response,p_message);
    // Send the HTTP Message as response
    SendResponse(&answer);
  }
  // Do **NOT** send an answer twice
  p_message->SetRequestHandle(NULL);
}

// Sending response for an incoming message
void       
HTTPServer::SendResponse(HTTPMessage* p_message)
{
  HTTP_RESPONSE   response;
  HTTP_REQUEST_ID requestID   = p_message->GetRequestHandle();
  FileBuffer*     buffer      = p_message->GetFileBuffer();
  CString         contentType("application/octet-stream"); 

  // See if there is something to send
  if(requestID == NULL)
  {
    ERRORLOG(ERROR_INVALID_PARAMETER,"SendResponse: nothing to send");
    return;
  }

  // Respond to general HTTP status
  int status = p_message->GetStatus();
  if(HTTP_STATUS_SERVER_ERROR <= status && status <= HTTP_STATUS_LAST)
  {
    RespondWithServerError(p_message->GetHTTPSite(),requestID,status,"","");
    p_message->SetRequestHandle(NULL);
    return;
  }
  if(HTTP_STATUS_BAD_REQUEST <= status && status < HTTP_STATUS_SERVER_ERROR)
  {
    RespondWithClientError(p_message->GetHTTPSite(),requestID,status,"","");
    p_message->SetRequestHandle(NULL);
    return;
  }

  // Initialize the HTTP response structure.
  InitializeHttpResponse(&response,(USHORT)p_message->GetStatus(),(PSTR) GetStatusText(p_message->GetStatus()));

  // Add a known header. (octet-stream or the message content type)
  if(!p_message->GetContentType().IsEmpty())
  {
    contentType = p_message->GetContentType();
  }
  AddKnownHeader(response,HttpHeaderContentType,contentType);

  // Add the server header or suppress it
  switch(m_sendHeader)
  {
    case SendHeader::HTTP_SH_MICROSOFT:   // Do nothing, Microsoft will add the server header
                                          break;
    case SendHeader::HTTP_SH_MARLIN:      AddKnownHeader(response,HttpHeaderServer,MARLIN_SERVER_VERSION);
                                          break;
    case SendHeader::HTTP_SH_APPLICATION: AddKnownHeader(response,HttpHeaderServer,m_name);
                                          break;
    case SendHeader::HTTP_SH_WEBCONFIG:   AddKnownHeader(response,HttpHeaderServer,m_configServerName);
                                          break;
    case SendHeader::HTTP_SH_HIDESERVER:  // Fill header with empty string will suppress it
                                          AddKnownHeader(response,HttpHeaderServer,"");
                                          break;
  }

  // Add cookies to the unknown response headers
  // Because we can have more than one Set-Cookie: header
  // and HTTP API just supports one set-cookie.
  UKHeaders ukheaders;
  Cookies& cookies = p_message->GetCookies();
  for(auto& cookie : cookies.GetCookies())
  {
    ukheaders.insert(std::make_pair("Set-Cookie",cookie.GetSetCookieText()));
  }

  // Other unknown headers
  HTTPSite* site = p_message->GetHTTPSite();
  if(site)
  {
    site->AddSiteOptionalHeaders(ukheaders);
  }

  // Add extra headers from the message
  HeaderMap* map = p_message->GetHeaderMap();
  for(HeaderMap::iterator it = map->begin(); it != map->end(); ++it)
  {
    ukheaders.insert(std::make_pair(it->first,it->second));
  }

  // Possible zip the contents, and add content-encoding header
  if(p_message->GetHTTPSite()->GetHTTPCompression() && buffer)
  {
    // But only if the client side requested it
    if(p_message->GetAcceptEncoding().Find("gzip") >= 0)
    {
      if(buffer->ZipBuffer())
      {
        DETAILLOGV("GZIP the buffer to size: %lu",buffer->GetLength());
        ukheaders.insert(std::make_pair("Content-Encoding","gzip"));
      }
    }
  }

  // Now add all unknown headers to the response
  PHTTP_UNKNOWN_HEADER unknown = AddUnknownHeaders(ukheaders);
  response.Headers.UnknownHeaderCount = (USHORT) ukheaders.size();
  response.Headers.pUnknownHeaders    = unknown;

  // Now after the compression, add the total content length
  CString contentLength;
  size_t totalLength = buffer ? buffer->GetLength() : 0;
#ifdef _WIN64
  contentLength.Format("%I64u",totalLength);
#else
  contentLength.Format("%lu",totalLength);
#endif
  AddKnownHeader(response,HttpHeaderContentLength,contentLength);

  // Dependent on the filling of FileBuffer
  // Send 1 or more buffers or the file
  if(buffer->GetHasBufferParts())
  {
    SendResponseBufferParts(&response,requestID,buffer,totalLength);
  }
  else if(buffer->GetFileName().IsEmpty())
  {
    SendResponseBuffer(&response,requestID,buffer,totalLength);
  }
  else
  {
    SendResponseFileHandle(&response,requestID,buffer);
  }
  if(GetLastError())
  {
    // Error handler
    CString message = GetLastErrorAsString(tls_lastError);
    m_log->AnalysisLog(__FUNCTION__, LogType::LOG_ERROR,true,"HTTP Answer [%d:%s]",GetLastError(),message);
    // Reset the last error
    SetError(NO_ERROR);
  }
  // Remove unknown header information
  delete [] unknown;

  // Do **NOT** send an answer twice
  p_message->SetRequestHandle(NULL);
}

bool      
HTTPServer::SendResponseBuffer(PHTTP_RESPONSE   p_response
                              ,HTTP_REQUEST_ID  p_requestID
                              ,FileBuffer*      p_buffer
                              ,size_t           p_totalLength)
{
  uchar* entity       = NULL;
  DWORD  result       = 0;
  DWORD  bytesSent    = 0;
  size_t entityLength = 0;
  HTTP_DATA_CHUNK dataChunk;

  // Get buffer to send
  if(p_totalLength)
  {
    p_buffer->GetBuffer(entity,entityLength);
  }

  // Only if a buffer present
  if(entity && entityLength)
  {
    // Add an entity chunk.
    dataChunk.DataChunkType           = HttpDataChunkFromMemory;
    dataChunk.FromMemory.pBuffer      = entity;
    dataChunk.FromMemory.BufferLength = (ULONG)entityLength;
    p_response->EntityChunkCount      = 1;
    p_response->pEntityChunks         = &dataChunk;
  }

  // Preparing our cache-policy
  HTTP_CACHE_POLICY policy;
  policy.Policy        = m_policy;
  policy.SecondsToLive = m_secondsToLive;

  // Because the entity body is sent in one call, it is not
  // required to specify the Content-Length.
  result = HttpSendHttpResponse(m_requestQueue,      // ReqQueueHandle
                                p_requestID,         // Request ID
                                0,                   // Flags
                                p_response,          // HTTP response
                                &policy,             // Cache policy
                                &bytesSent,          // bytes sent  (OPTIONAL)
                                NULL,                // pReserved2  (must be NULL)
                                0,                   // Reserved3   (must be 0)
                                NULL,                // LPOVERLAPPED(OPTIONAL)
                                NULL                 // pReserved4  (must be NULL)
                                ); 
  if(result != NO_ERROR)
  {
    ERRORLOG(result,"SendHttpResponseBuffer");
  }
  else
  {
    DETAILLOGV("SendHttpResponseBuffer [%d] bytes sent",bytesSent);
  }
  return (result == NO_ERROR);
}

void      
HTTPServer::SendResponseBufferParts(PHTTP_RESPONSE  p_response
                                   ,HTTP_REQUEST_ID p_request
                                   ,FileBuffer*     p_buffer
                                   ,size_t          p_totalLength)
{
  int    transmitPart = 0;
  uchar* entityBuffer = NULL;
  size_t entityLength = 0;
  size_t totalSent    = 0;
  DWORD  bytesSent    = 0;
  DWORD  result       = 0;
  HTTP_DATA_CHUNK dataChunk;
  memset(&dataChunk,0,sizeof(HTTP_DATA_CHUNK));

  while(p_buffer->GetBufferPart(transmitPart,entityBuffer,entityLength))
  {
    if(entityBuffer)
    {
      // Add an entity chunk.
      dataChunk.DataChunkType           = HttpDataChunkFromMemory;
      dataChunk.FromMemory.pBuffer      = entityBuffer;
      dataChunk.FromMemory.BufferLength = (ULONG)entityLength;
      p_response->EntityChunkCount      = 1;
      p_response->pEntityChunks         = &dataChunk;
    }
    // Flag to calculate the last sending part
    ULONG flags = (totalSent + entityLength) < p_totalLength ? HTTP_SEND_RESPONSE_FLAG_MORE_DATA : 0;

    if(transmitPart == 0)
    {
      // Preparing our cache-policy 
      HTTP_CACHE_POLICY policy;
      policy.Policy        = m_policy;
      policy.SecondsToLive = m_secondsToLive;

      result = HttpSendHttpResponse(m_requestQueue,      // ReqQueueHandle
                                    p_request,           // Request ID
                                    flags,               // Flags   DISCONNECT/MORE_DATA
                                    p_response,          // HTTP response
                                    &policy,             // Cache policy
                                    &bytesSent,          // bytes sent  (OPTIONAL)
                                    NULL,                // pReserved2  (must be NULL)
                                    0,                   // Reserved3   (must be 0)
                                    NULL,                // LPOVERLAPPED(OPTIONAL)
                                    NULL                 // Log data structure
                                    ); 
    }
    else
    {
      // Next part to send
      // Flags contains the fact that this is NOT the last part.
      result = HttpSendResponseEntityBody(m_requestQueue
                                         ,p_request
                                         ,flags
                                         ,1
                                         ,&dataChunk
                                         ,&bytesSent
                                         ,NULL
                                         ,NULL
                                         ,NULL
                                         ,NULL);
    }
    if(result != NO_ERROR)
    {
      ERRORLOG(result,"HTTP SendResponsePart error");
      break;
    }
    else
    {
      DETAILLOGV("HTTP SendResponsePart [%d] bytes sent",entityLength);
    }
    // Next buffer part
    totalSent += entityLength;
    ++transmitPart;
  }
}

void      
HTTPServer::SendResponseFileHandle(PHTTP_RESPONSE   p_response
                                  ,HTTP_REQUEST_ID  p_request
                                  ,FileBuffer*      p_buffer)
{
  DWORD  result    = 0;
  DWORD  bytesSent = 0;
  HANDLE file      = NULL;
  HTTP_DATA_CHUNK dataChunk;

  // File to transmit
  if(p_buffer->OpenFile() == false)
  {
    ERRORLOG(GetLastError(),"OpenFile for SendHttpResponse");
    return;
  }
  // Get the filehandle from buffer
  file = p_buffer->GetFileHandle();

  // See if file is under 4G large
  DWORD high = 0L;
  ULONG fileSize = GetFileSize(file,&high);
  if(high)
  {
    // 413: Request entity too large
    SendResponseError(p_response,p_request,m_clientErrorPage,413,"Request entity too large");
    // Error state
    ERRORLOG(ERROR_INVALID_PARAMETER,"File to send is too big (>4G)");
    // Close the filehandle
    p_buffer->CloseFile();
    return;
  }

  // Preparing the cache-policy
  HTTP_CACHE_POLICY policy;
  policy.Policy        = m_policy;
  policy.SecondsToLive = m_secondsToLive;

  result = HttpSendHttpResponse(m_requestQueue,
                                p_request,       // Request ID
                                HTTP_SEND_RESPONSE_FLAG_MORE_DATA,
                                p_response,      // HTTP response
                                &policy,         // Cache policy
                                &bytesSent,      // bytes sent-optional
                                NULL,            // pReserved2
                                0,               // Reserved3
                                NULL,            // LPOVERLAPPED
                                NULL             // pReserved4
                                );
  if(result != NO_ERROR)
  {
    ERRORLOG(result,"SendHttpResponse");
    p_buffer->CloseFile();
    return;
  }
  else
  {
    DETAILLOGS("SendHttpResponse file: ",p_buffer->GetFileName());
    DETAILLOGV("SendHttpResponse file header. Filesize: %d",fileSize);
  }

  // Send entity body from a file handle.
  dataChunk.DataChunkType = HttpDataChunkFromFileHandle;
  dataChunk.FromFileHandle.ByteRange.StartingOffset.QuadPart = 0;
  dataChunk.FromFileHandle.ByteRange.Length.QuadPart = fileSize; // HTTP_BYTE_RANGE_TO_EOF;
  dataChunk.FromFileHandle.FileHandle = file;

  result = HttpSendResponseEntityBody(m_requestQueue,
                                      p_request,
                                      0,           // This is the last send.
                                      1,           // Entity Chunk Count.
                                      &dataChunk,
                                      NULL,
                                      NULL,
                                      0,
                                      NULL,
                                      NULL);
  if(result != NO_ERROR)
  {
    ERRORLOG(result,"SendResponseEntityBody for file");
  }
  else
  {
    DETAILLOG1("SendResponseEntityBody for file");
  }
  // Now close our filehandle
  p_buffer->CloseFile();
}

void      
HTTPServer::SendResponseError(PHTTP_RESPONSE  p_response
                             ,HTTP_REQUEST_ID p_request
                             ,CString&        p_page
                             ,int             p_error
                             ,const char*     p_reason)
{
  DWORD result = 0;
  DWORD bytesSent = 0;
  HTTP_DATA_CHUNK dataChunk;
  CString sending;

  // Format our error page
  sending.Format(p_page,p_error,p_reason);

  // Add an entity chunk.
  dataChunk.DataChunkType           = HttpDataChunkFromMemory;
  dataChunk.FromMemory.pBuffer      = (void*) sending.GetString();
  dataChunk.FromMemory.BufferLength = sending.GetLength();
  p_response->EntityChunkCount      = 1;
  p_response->pEntityChunks         = &dataChunk;

  // Preparing our cache-policy
  HTTP_CACHE_POLICY policy;
  policy.Policy        = m_policy;
  policy.SecondsToLive = m_secondsToLive;

  // Because the entity body is sent in one call, it is not
  // required to specify the Content-Length.
  result = HttpSendHttpResponse(m_requestQueue,      // ReqQueueHandle
                                p_request,           // Request ID
                                0,                   // Flags
                                p_response,          // HTTP response
                                &policy,             // Cache policy
                                &bytesSent,          // bytes sent  (OPTIONAL)
                                NULL,                // pReserved2  (must be NULL)
                                0,                   // Reserved3   (must be 0)
                                NULL,                // LPOVERLAPPED(OPTIONAL)
                                NULL                 // pReserved4  (must be NULL)
                                ); 
  if(result != NO_ERROR)
  {
    ERRORLOG(result,"SendHttpResponse");
  }
  else
  {
    DETAILLOG1("SendHttpResponse (serverpage)");
  }
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
            ,protocol
            ,cipher,  p_sslInfo->CipherStrength
            ,hash,    p_sslInfo->HashStrength
            ,exchange,p_sslInfo->KeyExchangeStrength);
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
      message.Format("Forgotten to call 'StartSite' for: %s",site.second->GetPrefixURL());
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
      SendResponse(p_msg->GetHTTPSite(),p_msg->GetRequestHandle(),HTTP_STATUS_NOT_MODIFIED,"Not modified","","");
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

// Get text from HTTP_STATUS code
const char*
HTTPServer::GetStatusText(int p_status)
{
       if(p_status == HTTP_STATUS_CONTINUE)           return "Continue";
  else if(p_status == HTTP_STATUS_SWITCH_PROTOCOLS)   return "Switch protocols";
  // 200
  else if(p_status == HTTP_STATUS_OK)                 return "OK";
  else if(p_status == HTTP_STATUS_CREATED)            return "Created";
  else if(p_status == HTTP_STATUS_ACCEPTED)           return "Accepted";
  else if(p_status == HTTP_STATUS_PARTIAL)            return "Partial completion";
  else if(p_status == HTTP_STATUS_NO_CONTENT)         return "No info";
  else if(p_status == HTTP_STATUS_RESET_CONTENT)      return "Completed, form cleared";
  else if(p_status == HTTP_STATUS_PARTIAL_CONTENT)    return "Partial GET";
  // 300
  else if(p_status == HTTP_STATUS_AMBIGUOUS)          return "Server could not decide";
  else if(p_status == HTTP_STATUS_MOVED)              return "Moved resource";
  else if(p_status == HTTP_STATUS_REDIRECT)           return "Redirect to moved resource";
  else if(p_status == HTTP_STATUS_REDIRECT_METHOD)    return "Redirect to new access method";
  else if(p_status == HTTP_STATUS_NOT_MODIFIED)       return "Not modified since time";
  else if(p_status == HTTP_STATUS_USE_PROXY)          return "Use specified proxy";
  else if(p_status == HTTP_STATUS_REDIRECT_KEEP_VERB) return "HTTP/1.1: Keep same verb";
  // 400
  else if(p_status == HTTP_STATUS_BAD_REQUEST)        return "Invalid syntax";
  else if(p_status == HTTP_STATUS_DENIED)             return "Access denied";
  else if(p_status == HTTP_STATUS_PAYMENT_REQ)        return "Payment required";
  else if(p_status == HTTP_STATUS_FORBIDDEN)          return "Request forbidden";
  else if(p_status == HTTP_STATUS_NOT_FOUND)          return "URL/Object not found";
  else if(p_status == HTTP_STATUS_BAD_METHOD)         return "Method is not allowed";
  else if(p_status == HTTP_STATUS_NONE_ACCEPTABLE)    return "No acceptable response found";
  else if(p_status == HTTP_STATUS_PROXY_AUTH_REQ)     return "Proxy authentication required";
  else if(p_status == HTTP_STATUS_REQUEST_TIMEOUT)    return "Server timed out";
  else if(p_status == HTTP_STATUS_CONFLICT)           return "Conflict";
  else if(p_status == HTTP_STATUS_GONE)               return "Resource is no longer available";
  else if(p_status == HTTP_STATUS_LENGTH_REQUIRED)    return "Length required";
  else if(p_status == HTTP_STATUS_PRECOND_FAILED)     return "Precondition failed";
  else if(p_status == HTTP_STATUS_REQUEST_TOO_LARGE)  return "Request body too large";
  else if(p_status == HTTP_STATUS_URI_TOO_LONG)       return "URI too long";
  else if(p_status == HTTP_STATUS_UNSUPPORTED_MEDIA)  return "Unsupported media type";
  else if(p_status == HTTP_STATUS_RETRY_WITH)         return "Retry after appropriate action";
  // 500
  else if(p_status == HTTP_STATUS_SERVER_ERROR)       return "Internal server error";
  else if(p_status == HTTP_STATUS_NOT_SUPPORTED)      return "Not supported";
  else if(p_status == HTTP_STATUS_BAD_GATEWAY)        return "Error from gateway";
  else if(p_status == HTTP_STATUS_SERVICE_UNAVAIL)    return "Temporarily overloaded";
  else if(p_status == HTTP_STATUS_GATEWAY_TIMEOUT)    return "Gateway timeout";
  else if(p_status == HTTP_STATUS_VERSION_NOT_SUP)    return "HTTP version not supported";
  else if(p_status == HTTP_STATUS_LEGALREASONS)       return "Unavailable for legal reasons";
  else                                                return "Unknown HTTP Status";
}

//////////////////////////////////////////////////////////////////////////
//
// SERVER PUSH EVENTS
//
//////////////////////////////////////////////////////////////////////////

// Register server push event stream for this site
EventStream*
HTTPServer::SubscribeEventStream(HTTPSite*        p_site
                                ,CString          p_url
                                ,CString&         p_path
                                ,HTTP_REQUEST_ID  p_requestId
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
  stream->m_port      = p_site->GetPort();
  stream->m_requestID = p_requestId;
  stream->m_baseURL   = p_url;
  stream->m_absPath   = p_path;
  stream->m_site      = p_site;
  stream->m_lastID    = 0;
  stream->m_lastPulse = 0;
  stream->m_alive     = InitEventStream(*stream);
  if(p_token)
  {
    GetTokenOwner(p_token,stream->m_user);
  }
  // If the stream is still alive, keep it!
  if(stream->m_alive)
  {
    m_eventStreams.insert(std::make_pair(p_url,stream));
    TryStartEventHartbeat();
  }
  else
  {
    ERRORLOG(ERROR_ACCESS_DENIED,"Could not initialize event stream. Dropping stream.");
    delete stream;
    stream = nullptr;
  }
  return stream;
}

// Init the stream response
bool
HTTPServer::InitEventStream(EventStream& p_stream)
{
  // First comment to push to the stream (not an event!)
  CString init = m_eventBOM ? ConstructBOM() : "";
  init += ":init event-stream\n";

  // Initialize the HTTP response structure.
  InitializeHttpResponse(&p_stream.m_response,HTTP_STATUS_OK,"OK");

  // Add a known header.
  AddKnownHeader(p_stream.m_response,HttpHeaderContentType,"text/event-stream");

  // Add an entity chunk.
  HTTP_DATA_CHUNK dataChunk;
  dataChunk.DataChunkType           = HttpDataChunkFromMemory;
  dataChunk.FromMemory.pBuffer      = (PVOID)init.GetString();
  dataChunk.FromMemory.BufferLength = (ULONG)init.GetLength();
  // Add chunk to response
  p_stream.m_response.EntityChunkCount = 1;
  p_stream.m_response.pEntityChunks    = &dataChunk;

  // Preparing the cache-policy
  HTTP_CACHE_POLICY policy;
  policy.Policy = HttpCachePolicyNocache;
  policy.SecondsToLive = 0;

  // Because the entity body is sent in one call, it is not
  // required to specify the Content-Length.
  DWORD bytesSent = 0;
  ULONG flags  = HTTP_SEND_RESPONSE_FLAG_MORE_DATA;
  DWORD result = HttpSendHttpResponse(m_requestQueue,         // ReqQueueHandle
                                      p_stream.m_requestID,   // Request ID
                                      flags,                  // Flags
                                      &p_stream.m_response,   // HTTP response
                                      &policy,                // Policy
                                      &bytesSent,             // bytes sent  (OPTIONAL)
                                      NULL,                   // pReserved2  (must be NULL)
                                      0,                      // Reserved3   (must be 0)
                                      NULL,                   // LPOVERLAPPED(OPTIONAL)
                                      NULL                    // pReserved4  (must be NULL)
                                      );
  if(result != NO_ERROR)
  {
    ERRORLOG(result,"SendHttpResponse for init of event-stream");
  }
  else
  {
    DETAILLOGS("HTTP Send 200 OK ",init);
  }
  return (result == NO_ERROR);
}

bool
HTTPServer::SendResponseEventBuffer(HTTP_REQUEST_ID  p_requestID
                                     ,const char*      p_buffer
                                     ,size_t           p_length
                                     ,bool             p_continue /*=true*/)
{
  DWORD  result    = 0;
  DWORD  bytesSent = 0;
  HTTP_DATA_CHUNK dataChunk;

  // Only if a buffer present
  dataChunk.DataChunkType = HttpDataChunkFromMemory;
  dataChunk.FromMemory.pBuffer      = (void*)p_buffer;
  dataChunk.FromMemory.BufferLength = (ULONG)p_length;

  ULONG flags =         HTTP_SEND_RESPONSE_FLAG_BUFFER_DATA;
  flags += p_continue ? HTTP_SEND_RESPONSE_FLAG_MORE_DATA 
                      : HTTP_SEND_RESPONSE_FLAG_DISCONNECT;
  result = HttpSendResponseEntityBody(m_requestQueue,      // ReqQueueHandle
                                      p_requestID,         // Request ID
                                      flags,               // Flags
                                      1,                   // Entity chunk count
                                      &dataChunk,          // Chunk to send
                                      &bytesSent,          // bytes sent  (OPTIONAL)
                                      NULL,                // pReserved1  (must be NULL)
                                      0,                   // Reserved2   (must be 0)
                                      NULL,                // LPOVERLAPPED(OPTIONAL)
                                      NULL                 // pLogData
                                      );
  if(result != NO_ERROR)
  {
    ERRORLOG(result,"HttpSendResponseEntityBody failed for SendEvent");
  }
  else
  {
    DETAILLOGV("HttpSendResponseEntityBody [%d] bytes sent",p_length);
  }
  return (result == NO_ERROR);
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
      EventStream* stream = lower->second;

      // See if we must push the event to this stream
      if(p_user.IsEmpty() || p_user.CompareNoCase(stream->m_user) == 0)
      {
        // Sending the event, while deleting it
        if(SendEvent(stream,p_event) == false)
        {
          lower = AbortEventStream(stream);
          if(lower == upper) break;
          continue;
        }
        result = true;
      }
      // Next iterator
      if(lower == upper)
      {
        break;
      }
      ++lower;
    }
  }
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

  // Lock server as short as possible and Push to the stream
  {
    AutoCritSec lock(&m_eventLock);
    p_stream->m_alive = SendResponseEventBuffer(p_stream->m_requestID,sendString.GetString(),sendString.GetLength(),p_continue);
  }
  // Remember the time we sent the event pulse
  _time64(&(p_stream->m_lastPulse));

  // Remember the highest last ID
  if(p_event->m_id > p_stream->m_lastID)
  {
    p_stream->m_lastID = p_event->m_id;
  }

  // Tell what we just did
  if(m_detail && m_log && p_stream->m_alive)
  {
    CString text;
    text.Format("Sent event id: %d to client(s) on URL: ",p_event->m_id);
    text += p_stream->m_baseURL;
    text += "\n";
    text += p_event->m_data;
    DETAILLOG1(text);
  }

  // Ready with the event
  delete p_event;

  if(p_stream->m_alive == false)
  {
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
    stream.AppendFormat("event:%s\n",p_event->m_event);
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
HTTPServer::TryStartEventHartbeat()
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

  DETAILLOG1("Event hartbeat monitor started");
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
  DETAILLOG1("Event hartbeat monitor stopped");

  if(startNew)
  {
    // Retry starting a new thread/event
    TryStartEventHartbeat();
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
  EventMap::iterator it = m_eventStreams.begin();
  UINT number = 0;
  // Create keep alive buffer
  CString keepAlive = ":keepalive\n";

  DETAILLOG1("Starting event hartbeat");

  // Pulse event stream with a comment
  while(it != m_eventStreams.end())
  {
    EventStream& stream = *it->second;

    // If we did not send anything for the last eventKeepAlive seconds,
    // Keep a margin of half a second for the wakeup of the server
    // we send a ":keepalive" comment to the clients
    if((pulse - stream.m_lastPulse) > (m_eventKeepAlive - 500))
    {
      stream.m_alive = SendResponseEventBuffer(stream.m_requestID,keepAlive.GetString(),keepAlive.GetLength());
      stream.m_lastPulse = pulse;
      ++number;
    }
    // Next stream
    ++it;
  }

  // What we just did
  DETAILLOGV("Sent hartbeat to %d push-event clients.",number);

  // Clean up dead event streams
  it = m_eventStreams.begin();
  while(it != m_eventStreams.end())
  {
    if(it->second->m_alive == false)
    {
      DETAILLOGS("Abandoned push-event client from: ",it->second->m_baseURL);

      // Remove request from the request queue, closing the connection
      HttpCancelHttpRequest(m_requestQueue,it->second->m_requestID,NULL);

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
EventMap::iterator
HTTPServer::CloseEventStream(EventStream* p_stream)
{
  AutoCritSec lock(&m_eventLock);

  EventMap::iterator it = m_eventStreams.begin();
  while(it != m_eventStreams.end())
  {
    if(it->second == p_stream)
    {
      // Show in the log
      DETAILLOGV("Closing event stream (user: %s) for URL: %s",p_stream->m_user,p_stream->m_baseURL);
      // Send a close-stream event
      // And close the stream by sending a close flag
      ServerEvent* event = new ServerEvent("close");
      if(SendEvent(it->second,event,false))
      {
        // delete the stream object
        delete it->second;
        // Erase from the cache
        return m_eventStreams.erase(it);
      }
      break;
    }
    ++it;
  }
  return m_eventStreams.end();
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
      bool erased = false;
      if(p_user.IsEmpty() || p_user.CompareNoCase(lower->second->m_user) == 0)
      {
        lower = CloseEventStream(lower->second);
        erased = true;
      }
      if(lower == upper) break;
      if(!erased)
      {
        ++lower;
      }
    }
  }
}

// Close and abort an event stream for whatever reason
// Call only on an abandoned stream.
// No "OnError" or "OnClose" can be sent now
EventMap::iterator
HTTPServer::AbortEventStream(EventStream* p_stream)
{
  // No lock on the m_siteslock, as we are dealing with an event
  AutoCritSec lock(&m_eventLock);

  EventMap::iterator it = m_eventStreams.begin();
  while(it != m_eventStreams.end())
  {
    if(it->second == p_stream)
    {
      // Abandon the stream
      HttpCancelHttpRequest(m_requestQueue,p_stream->m_requestID,NULL);

      // Done with the stream
      delete p_stream;
      // Erase from the cache and return next position
      return m_eventStreams.erase(it);
    }
    ++it;
  }
  return it;
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
// STOPPING THE SERVER
//
//////////////////////////////////////////////////////////////////////////

void
HTTPServer::StopServer()
{
  AutoCritSec lock(&m_eventLock);
  DETAILLOG1("Received a StopServer request");

  // See if we are running at all
  if(m_running == false)
  {
    m_serverThread = NULL;
    return;
  }

  // Try to remove all event streams
  for(auto& it : m_eventStreams)
  {
    // SEND OnClose event
    ServerEvent* event = new ServerEvent("close");
    SendEvent(it.second->m_port,it.second->m_baseURL,event);
  }
  // Try to remove all event streams
  while(!m_eventStreams.empty())
  {
    EventStream* stream = m_eventStreams.begin()->second;
    CloseEventStream(stream);
  }

  // See if we have a running server-push-event heartbeat monitor
  if(m_eventEvent)
  {
    // Explicitly pulse the event heartbeat monitor
    // this abandons the monitor in one go!
    DETAILLOG1("Abandon the server-push-events hartbeat monitor");
    SetEvent(m_eventEvent);
  }

  // mainloop will stop on next iteration
  m_running = false;

  // Shutdown the request queue canceling 
  // the outstanding call to HttpReceiveHttpRequest in the mainloop
  // Using: HttpShutdownRequestQueue
  ULONG result = HttpShutdownRequestQueue(m_requestQueue);
  if(result == NO_ERROR)
  {
    DETAILLOG1("HTTP Request queue shutdown issued");
  }
  else
  {
    ERRORLOG(result,"Cannot shutdown the HTTP request queue");
  }
  m_requestQueue = NULL;

  // Wait for a maximum of 30 seconds for the queue to stop
  // All server action should be ended after that.
  for(int ind = 0; ind < 300; ++ind)
  {
    Sleep(100);
    // Wait till the breaking of the mainloop
    if(m_serverThread == nullptr)
    {
      break;
    }
  }
  // Cleanup the sites and groups
  Cleanup();
}
