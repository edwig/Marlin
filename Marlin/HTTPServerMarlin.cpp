/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: HTTPServerMarlin.cpp
//
// Marlin Server: Internet server/client
// 
// Copyright (c) 2017 ir. W.E. Huisman
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
#include "HTTPServerMarlin.h"
#include "HTTPSiteMarlin.h"
#include "AutoCritical.h"
#include "WebServiceServer.h"
#include "HTTPURLGroup.h"
#include "HTTPRequest.h"
#include "GetLastErrorAsString.h"
#include "ConvertWideString.h"
#include "WebSocket.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// Logging macro's
#define DETAILLOG1(text)          if(m_detail && m_log) { DetailLog (__FUNCTION__,LogType::LOG_INFO,text); }
#define DETAILLOGS(text,extra)    if(m_detail && m_log) { DetailLogS(__FUNCTION__,LogType::LOG_INFO,text,extra); }
#define DETAILLOGV(text,...)      if(m_detail && m_log) { DetailLogV(__FUNCTION__,LogType::LOG_INFO,text,__VA_ARGS__); }
#define WARNINGLOG(text,...)      if(m_detail && m_log) { DetailLogV(__FUNCTION__,LogType::LOG_WARN,text,__VA_ARGS__); }
#define ERRORLOG(code,text)       ErrorLog (__FUNCTION__,code,text)
#define HTTPERROR(code,text)      HTTPError(__FUNCTION__,code,text)

HTTPServerMarlin::HTTPServerMarlin(CString p_name)
                 :HTTPServer(p_name)
{
}

HTTPServerMarlin::~HTTPServerMarlin()
{
  // Cleanup the server objects
  Cleanup();
}

CString
HTTPServerMarlin::GetVersion()
{
  return CString(MARLIN_SERVER_VERSION " on Microsoft HTTP-Server API/2.0");
}

// Initialise a HTTP server and server-session
bool
HTTPServerMarlin::Initialise()
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

  // STEP 4: CREATE SERVER SESSION
  retCode = HttpCreateServerSession(HTTPAPI_VERSION_2,&m_session,0);
  if(retCode != NO_ERROR)
  {
    ERRORLOG(retCode,"CreateServerSession");
    return false;
  }
  DETAILLOGV("Serversession created: %I64X",m_session);

  // STEP 5: Create a request queue with NO name
  // Although we CAN create a name, it would mean a global object (and need to be unique)
  // So it's harder to create more than one server per machine
  retCode = HttpCreateRequestQueue(HTTPAPI_VERSION_2,NULL,NULL,0,&m_requestQueue);
  if(retCode != NO_ERROR)
  {
    ERRORLOG(retCode,"CreateRequestQueue");
    return false;
  }
  DETAILLOGV("Request queue created: %p",m_requestQueue);

  // STEP 6: SET UP FOR ASYNC I/O
  // Register the request queue for async I/O
  retCode = m_pool.AssociateIOHandle(m_requestQueue,(ULONG_PTR)HandleAsynchroneousIO);
  if(retCode != NO_ERROR)
  {
    ERRORLOG(retCode,"Associate request queue with the I/O completion port of the threadpool.");
    return false;
  }
  DETAILLOGV("Request queue registrated by the threadpool");

  // STEP 7: SET THE LENGTH OF THE BACKLOG QUEUE FOR INCOMING TRAFFIC
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

  // STEP 8: SET VERBOSITY OF THE 503 SERVER ERROR
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

  // STEP 9: Set the hard limits
  InitHardLimits();

  // STEP 10: Init the response headers to send
  InitHeaders();

  // We are airborne!
  return (m_initialized = true);
}

// Cleanup the server
void
HTTPServerMarlin::Cleanup()
{
  ULONG retCode;
  USES_CONVERSION;
  AutoCritSec lock1(&m_sitesLock);
  AutoCritSec lock2(&m_eventLock);

  // Remove all remaining sockets
  for(auto& it : m_sockets)
  {
    delete it.second;
  }
  m_sockets.clear();

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
    retCode = HttpCloseServerSession(m_session);
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
HTTPServerMarlin::InitLogging()
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

// Initialise general server header settings
void
HTTPServerMarlin::InitHeaders()
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
HTTPServerMarlin::InitHardLimits()
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

// Initialise the threadpool limits
void
HTTPServerMarlin::InitThreadpoolLimits(int& p_minThreads,int& p_maxThreads,int& p_stackSize)
{
  p_minThreads = m_webConfig.GetParameterInteger("Server","MinThreads",p_minThreads);
  p_maxThreads = m_webConfig.GetParameterInteger("Server","MaxThreads",p_maxThreads);
  p_stackSize  = m_webConfig.GetParameterInteger("Server","StackSize", p_stackSize);
}

// Initialise the servers webroot
void
HTTPServerMarlin::InitWebroot(CString p_webroot)
{
  m_webroot = m_webConfig.GetParameterString("Server","WebRoot",p_webroot);
}

// Create a site to bind the traffic to
HTTPSite*
HTTPServerMarlin::CreateSite(PrefixType    p_type
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
        message.Format("Tried to register a sub-site, without a main-site: %s",p_baseURL.GetString());
        ERRORLOG(ERROR_NOT_FOUND,message);
        return nullptr;
      }
    }
    // Create and register a URL
    // Remember URL Prefix strings, and create the site
    HTTPSiteMarlin* registeredSite = new HTTPSiteMarlin(this,p_port,p_baseURL,prefix,mainSite,p_callback);
    if(RegisterSite(registeredSite,prefix))
    {
      // Site created and registered
      return registeredSite;
    }
    delete registeredSite;
  }
  // No luck
  return nullptr;
}

// Delete a channel (from prefix OR base URL forms)
bool
HTTPServerMarlin::DeleteSite(int p_port,CString p_baseURL,bool p_force /*=false*/)
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

// Find and make an URL group
HTTPURLGroup*
HTTPServerMarlin::FindUrlGroup(CString p_authName
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
       group->GetAuthenticationDomain()    == p_domain)
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
    int number = (int)m_urlGroups.size();
    DETAILLOGV("Created URL group [%d] for authentication: %s",number,p_authName.GetString());
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
HTTPServerMarlin::RemoveURLGroup(HTTPURLGroup* p_group)
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

// New threads in the threadpool will start on listening to our HTTP queue
// and respond with to a HTTPRequest
/*static*/ void
StartHTTPRequest(void* p_argument)
{
  HTTPServerMarlin* server = reinterpret_cast<HTTPServerMarlin*>(p_argument);
  if(server)
  {
    HTTPRequest* request = new HTTPRequest(server);
    server->RegisterHTTPRequest(request);
    request->StartRequest();
  }
}

// Clean up our registered overlapping I/O object
/*static*/ void
CancelHTTPRequest(void* p_argument)
{
  OutstandingIO* outstanding = reinterpret_cast<OutstandingIO*>(p_argument);
  if(outstanding)
  {
    HTTPRequest* request = outstanding->m_request;
    if(request)
    {
      HTTPServer* server = request->GetHTTPServer();
      if(server)
      {
        server->UnRegisterHTTPRequest(request);
      }
      delete request;
    }
  }
}

// Running the server on asynchronous I/O in the threadpool
void
HTTPServerMarlin::Run()
{
  // Do all initialization
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

  // Registering the init for the threadpool
  // This get new threads started on a HTTPRequest
  if(m_pool.SetThreadInitFunction(StartHTTPRequest,CancelHTTPRequest,(void*)this))
  {
    DETAILLOG1("HTTPServer registered for the threadpool init loop");
  }
  else
  {
    ERRORLOG(ERROR_INVALID_PARAMETER,"Threadpool cannot service our request queue!");
  }

  // STARTED OUR MAIN LOOP
  DETAILLOG1("HTTPServer entering main loop");
  m_running = true;

  // Starting the pool will trigger the starting of the reading threads
  // Because we registered our init function to start on a HTTPRequest
  m_pool.Run();
}

// Running the main loop of the webserver
// void
// HTTPServerMarlin::RunHTTPServer()
// {
//   ULONG              result    = 0;
//   DWORD              bytesRead = 0;
//   bool               doReceive = true;
//   HTTP_REQUEST_ID    requestId = NULL;
//   PHTTP_REQUEST      request   = nullptr;
//   PCHAR              requestBuffer = nullptr;
//   ULONG              requestBufferLength = 0;
//   HTTPMessage*       message = NULL;
//   USES_CONVERSION;
// 
//   // Use counter
//   m_counter.Start();
// 
//   Initialise();
// 
//   // See if we are in a state to receive requests
//   if(GetLastError() || !m_initialized)
//   {
//     ERRORLOG(ERROR_INVALID_PARAMETER,"RunHTTPServer called too early");
//     return;
//   }
//   DETAILLOG1("HTTPServer initialised and ready to go!");
// 
//   // Check if all sites were properly started
//   // Catches programmers who forget to call HTTPSite::StartSite()
//   CheckSitesStarted();
// 
//   // Allocate a 32 KB buffer. This size should work for most 
//   // requests. The buffer size can be increased if required. Space
//   // is also required for an HTTP_REQUEST structure.
//   requestBufferLength = sizeof(HTTP_REQUEST) + INIT_HTTP_BUFFERSIZE;
//   requestBuffer       = (PCHAR) new uchar[requestBufferLength];
// 
//   if(requestBuffer == NULL)
//   {
//     ERRORLOG(ERROR_NOT_ENOUGH_MEMORY,"Out of memory");
//     return;
//   }
//   DETAILLOGV("Request buffer allocated [%d] bytes",requestBufferLength);
// 
//   // Buffer where we receive our requests
//   request = (PHTTP_REQUEST)requestBuffer;
// 
//   // Wait for a new request. This is indicated by a NULL request ID.
//   HTTP_SET_NULL_ID(&requestId);
// 
//   // START OUR MAIN LOOP
//   m_running = true;
//   DETAILLOG1("HTTPServer entering main loop");
// 
//   while(m_running)
//   {
//     // New receive in the buffer
//     RtlZeroMemory(request,requestBufferLength);
// 
//     // Use counter
//     m_counter.Stop();
//     // Sit tight for the next request
//     result = HttpReceiveHttpRequest(m_requestQueue,     // Request Queue
//                                     requestId,          // Request ID
//                                     0,                  // Flags
//                                     request,            // HTTP request buffer
//                                     requestBufferLength,// request buffer length
//                                     &bytesRead,         // bytes received
//                                     NULL);              // LPOVERLAPPED
//     // Use counter
//     m_counter.Start();
// 
//     // Grab the senders content
//     CString   acceptTypes    = request->Headers.KnownHeaders[HttpHeaderAccept         ].pRawValue;
//     CString   contentType    = request->Headers.KnownHeaders[HttpHeaderContentType    ].pRawValue;
//     CString   acceptEncoding = request->Headers.KnownHeaders[HttpHeaderAcceptEncoding ].pRawValue;
//     CString   cookie         = request->Headers.KnownHeaders[HttpHeaderCookie         ].pRawValue;
//     CString   authorize      = request->Headers.KnownHeaders[HttpHeaderAuthorization  ].pRawValue;
//     CString   modified       = request->Headers.KnownHeaders[HttpHeaderIfModifiedSince].pRawValue;
//     CString   referrer       = request->Headers.KnownHeaders[HttpHeaderReferer        ].pRawValue;
//     CString   rawUrl         = CW2A(request->CookedUrl.pFullUrl);
//     PSOCKADDR sender         = request->Address.pRemoteAddress;
//     PSOCKADDR receiver       = request->Address.pLocalAddress;
//     int       remDesktop     = FindRemoteDesktop(request->Headers.UnknownHeaderCount
//                                                 ,request->Headers.pUnknownHeaders);
// 
//     // Log earliest as possible
//     DETAILLOGV("Received HTTP call from [%s] with length: %I64u"
//               ,SocketToServer((PSOCKADDR_IN6)sender).GetString()
//               ,request->BytesReceived);
// 
//     // Test if server already stopped, and we are here because of the stopping
//     if(m_running == false)
//     {
//       DETAILLOG1("HTTPServer stopped in mainloop.");
//       break;
//     }
// 
//     if(result == NO_ERROR)
//     {
//       HANDLE accessToken = NULL;
//       // Log incoming request
//       DETAILLOGS("Got a request for: ",rawUrl);
// 
//       // FInding the site
//       bool eventStream = false;
//       LPFN_CALLBACK callback = nullptr;
//       HTTPSite* site = reinterpret_cast<HTTPSite*>(request->UrlContext);
//       if(site)
//       {
//         callback    = site->GetCallback();
//         eventStream = site->GetIsEventStream();
//       }
// 
//       // See if we must substitute for a sub-site
//       if(m_hasSubsites)
//       {
//         CString absPath = CW2A(request->CookedUrl.pAbsPath);
//         site = FindHTTPSite(site,absPath);
//       }
// 
//       if(request->RequestInfoCount > 0)
//       {
//         for(unsigned ind = 0; ind < request->RequestInfoCount; ++ind)
//         {
//           if(request->pRequestInfo[ind].InfoType == HttpRequestInfoTypeAuth) 
//           {
//             // Default is failure
//             doReceive = false;
// 
//             PHTTP_REQUEST_AUTH_INFO auth = (PHTTP_REQUEST_AUTH_INFO)request->pRequestInfo[ind].pInfo;
//             if(auth->AuthStatus == HttpAuthStatusNotAuthenticated)
//             {
//               // Not (yet) authenticated. Back to the client for authentication
//               DETAILLOGS("Not yet authenticated for: ",rawUrl);
//               HTTPMessage msg(HTTPCommand::http_response,HTTP_STATUS_DENIED);
//               msg.SetRequestHandle(request->RequestId);
//               result = RespondWithClientError(site,&msg,HTTP_STATUS_DENIED,"Not authenticated",site->GetAuthenticationScheme());
//             }
//             else if(auth->AuthStatus == HttpAuthStatusFailure)
//             {
//               // Second round. Still not authenticated. Drop the connection, better next time
//               DETAILLOGS("Authentication failed for: ",rawUrl);
//               DETAILLOGV("Authentication failed because of: %s",AuthenticationStatus(auth->SecStatus).GetString());
//               HTTPMessage msg(HTTPCommand::http_response,HTTP_STATUS_DENIED);
//               msg.SetRequestHandle(request->RequestId);
//               result = RespondWithClientError(site,&msg,HTTP_STATUS_DENIED,"Not authenticated",site->GetAuthenticationScheme());
//               // Go to next request
//               HTTP_SET_NULL_ID(&requestId);
//             }
//             else if(auth->AuthStatus == HttpAuthStatusSuccess)
//             {
//               // Authentication accepted: all is well
//               DETAILLOGS("Authentication done for: ",rawUrl);
//               accessToken = auth->AccessToken;
//               doReceive   = true;
//             }
//             else
//             {
//               CString authError;
//               authError.Format("Authentication mechanism failure. Unknown status: %d",auth->AuthStatus);
//               ERRORLOG(ERROR_NOT_AUTHENTICATED,authError);
//             }
//           }
//           else if(request->pRequestInfo[ind].InfoType == HttpRequestInfoTypeSslProtocol)
//           {
//             // Only exists on Windows 10 / Server 2016
//             if(m_detail)
//             {
//               PHTTP_SSL_PROTOCOL_INFO sslInfo = (PHTTP_SSL_PROTOCOL_INFO)request->pRequestInfo[ind].pInfo;
//               LogSSLConnection(sslInfo);
//             }
//           }
//         }
//       }
//       if(doReceive)
//       {
//         // Remember the context: easy in API 2.0
//         if(callback == nullptr && site == nullptr)
//         {
//           HTTPMessage msg(HTTPCommand::http_response,HTTP_STATUS_NOT_FOUND);
//           msg.SetRequestHandle(request->RequestId);
//           SendResponse(site
//                       ,&msg
//                       ,HTTP_STATUS_NOT_FOUND
//                       ,"URL not found"
//                       ,(PSTR)rawUrl.GetString()
//                       ,site->GetAuthenticationScheme());
//           // Ready with this request
//           HTTP_SET_NULL_ID(&requestId);
//           continue;
//         }
// 
//         // Translate the command. Now reduced to just this switch
//         HTTPCommand type = HTTPCommand::http_no_command;
//         switch(request->Verb)
//         {
//           case HttpVerbOPTIONS:   type = HTTPCommand::http_options;    break;
//           case HttpVerbGET:       type = HTTPCommand::http_get;        break;
//           case HttpVerbHEAD:      type = HTTPCommand::http_head;       break;
//           case HttpVerbPOST:      type = HTTPCommand::http_post;       break;
//           case HttpVerbPUT:       type = HTTPCommand::http_put;        break;
//           case HttpVerbDELETE:    type = HTTPCommand::http_delete;     break;
//           case HttpVerbTRACE:     type = HTTPCommand::http_trace;      break;
//           case HttpVerbCONNECT:   type = HTTPCommand::http_connect;    break;
//           case HttpVerbMOVE:      type = HTTPCommand::http_move;       break;
//           case HttpVerbCOPY:      type = HTTPCommand::http_copy;       break;
//           case HttpVerbPROPFIND:  type = HTTPCommand::http_proppfind;  break;
//           case HttpVerbPROPPATCH: type = HTTPCommand::http_proppatch;  break;
//           case HttpVerbMKCOL:     type = HTTPCommand::http_mkcol;      break;
//           case HttpVerbLOCK:      type = HTTPCommand::http_lock;       break;
//           case HttpVerbUNLOCK:    type = HTTPCommand::http_unlock;     break;
//           case HttpVerbSEARCH:    type = HTTPCommand::http_search;     break;
//           default:                // Try to get a less known verb as 'last resort'
//                                   type = GetUnknownVerb(request->pUnknownVerb);
//                                   if(type == HTTPCommand::http_no_command)
//                                   {
//                                     // Non implemented like HttpVerbTRACK or other non-known verbs
//                                     HTTPMessage msg(HTTPCommand::http_response,HTTP_STATUS_NOT_SUPPORTED);
//                                     msg.SetRequestHandle(request->RequestId);
//                                     RespondWithServerError(site,&msg,HTTP_STATUS_NOT_SUPPORTED,"Not implemented","");
//                                     // Ready with this request
//                                     HTTP_SET_NULL_ID(&requestId);
//                                     continue;
//                                   }
//                                   break;
//         }
// 
//         // Receiving the initiation of an event stream for the server
//         acceptTypes.Trim();
//         if((type == HTTPCommand::http_get) && (eventStream || acceptTypes.Left(17).CompareNoCase("text/event-stream") == 0))
//         {
//           CString absolutePath = CW2A(request->CookedUrl.pAbsPath);
//           EventStream* stream = SubscribeEventStream(site,site->GetSite(), absolutePath,request->RequestId,accessToken);
//           if(stream)
//           {
//             stream->m_baseURL = rawUrl;
//             m_pool->SubmitWork(callback,(void*)stream);
//             HTTP_SET_NULL_ID(&requestId);
//             continue;
//           }
//         }
// 
//         // For all types of requests: Create the HTTPMessage
//         message = new HTTPMessage(type,site);
//         message->SetURL(rawUrl);
//         message->SetReferrer(referrer);
//         message->SetAuthorization(authorize);
//         message->SetRequestHandle(request->RequestId);
//         message->SetConnectionID(request->ConnectionId);
//         message->SetContentType(contentType);
//         message->SetAccessToken(accessToken);
//         message->SetRemoteDesktop(remDesktop);
//         message->SetSender  ((PSOCKADDR_IN6)sender);
//         message->SetReceiver((PSOCKADDR_IN6)receiver);
//         message->SetCookiePairs(cookie);
//         message->SetAcceptEncoding(acceptEncoding);
//         if(site->GetAllHeaders())
//         {
//           // If requested so, copy all headers to the message
//           message->SetAllHeaders(&request->Headers);
//         }
//         else
//         {
//           // As a minimum, always add the unknown headers
//           // in case of a 'POST', as the SOAPAction header is here too!
//           message->SetUnknownHeaders(&request->Headers);
//         }
// 
//         // Handle modified-since 
//         // Rest of the request is then not needed any more
//         if(type == HTTPCommand::http_get && !modified.IsEmpty())
//         {
//           message->SetHTTPTime(modified);
//           if(DoIsModifiedSince(message))
//           {
//             // Answer already sent, go on to the next request
//             HTTP_SET_NULL_ID(&requestId);
//             continue;
//           }
//         }
// 
//         // Find X-HTTP-Method VERB Tunneling
//         if(type == HTTPCommand::http_post && site->GetVerbTunneling())
//         {
//           if(message->FindVerbTunneling())
//           {
//             DETAILLOGV("Request VERB changed to: %s",message->GetVerb().GetString());
//           }
//         }
// 
//         // Remember the fact that we should read the rest of the message
//         message->SetReadBuffer(request->Flags & HTTP_REQUEST_FLAG_MORE_ENTITY_BODY_EXISTS);
// 
//         // Hit the thread pool with this message
//         m_pool->SubmitWork(callback,(void*)message);
// 
//         // Ready with this request
//         HTTP_SET_NULL_ID(&requestId);
//       }
//     }
//     else if(result == ERROR_MORE_DATA)
//     {
//       // The input buffer was too small to hold the request headers.
//       // Increase the buffer size and call the API again. 
//       //
//       // When calling the API again, handle the request
//       // that failed by passing a RequestID.
//       // This RequestID is read from the old buffer.
//       requestId = request->RequestId;
// 
//       // Free the old buffer and allocate a new buffer.
//       requestBufferLength = bytesRead;
//       delete [] requestBuffer;
//       requestBuffer = (PCHAR) new uchar[requestBufferLength];
//       if(requestBuffer == NULL)
//       {
//         ERRORLOG(ERROR_NOT_ENOUGH_MEMORY,"Out of memory");
//         break;
//       }
//       request = (PHTTP_REQUEST)requestBuffer;
// 
//       DETAILLOGV("Request buffer length expanded to [%d] bytes",requestBufferLength);
//     }
//     else if(ERROR_CONNECTION_INVALID == result && !HTTP_IS_NULL_ID(&requestId))
//     {
//       // The TCP connection was corrupted by the peer when
//       // attempting to handle a request with more buffer. 
//       // Continue to the next request.
//       HTTP_SET_NULL_ID(&requestId);
//     }
//     else if(result == ERROR_OPERATION_ABORTED)
//     {
//       if(m_running)
//       {
//         // Not called by StopServer
//         ERRORLOG(result,"ReceiveHttpRequest aborted");
//       }
//     }
//     else // ERROR_NO_ACCESS / ERROR_HANDLE_EOF / ERROR_INVALID_PARAMETER
//     {
//       // Any other error
//       ERRORLOG(result,"ReceiveHttpRequest");
//     }
// 
//     // Do error handler at the end of the mainloop
//     if(GetLastError())
//     {
//       m_log->AnalysisLog(__FUNCTION__, LogType::LOG_ERROR,true,"HTTP Mainloop: %d",GetLastError());
//       SetError(NO_ERROR);
//     }
//   }
//   // Free the request buffer
//   if(requestBuffer)
//   {
//     delete [] requestBuffer;
//     DETAILLOG1("Dropped request buffer");
//   }
// 
//   // Use the counter
//   m_counter.Stop();
// 
//   // Last action in the server thread
//   m_serverThread = 0;
// }

//////////////////////////////////////////////////////////////////////////
//
// STOPPING THE SERVER
//
//////////////////////////////////////////////////////////////////////////

void
HTTPServerMarlin::StopServer()
{
  AutoCritSec lock(&m_eventLock);
  DETAILLOG1("Received a StopServer request");

  // See if we are running at all
  if(m_running == false)
  {
    return;
  }

  // Try to remove all WebSockets
  while(!m_sockets.empty())
  {
    WebSocket* socket = m_sockets.begin()->second;
    socket->CloseSocket();
    UnRegisterWebSocket(socket);
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

  // Cancel ALL (overlapped = null) outstanding I/O actions for the request queue
  ULONG result = CancelIoEx(m_requestQueue,NULL);
  if(result == NO_ERROR || result == ERROR_INVALID_FUNCTION)
  {
    // canceled, or no I/O to be canceled
    DETAILLOG1("HTTP Request queue cancelled all outstanding async I/O");
  }
  else
  {
    ERRORLOG(result,"Cannot cancel outstanding async I/O for the request queue");
  }

  // Shutdown the request queue canceling everything, and freeing the threads
  result = HttpShutdownRequestQueue(m_requestQueue);
  if(result == NO_ERROR)
  {
    DETAILLOG1("HTTP Request queue shutdown issued");
  }
  else
  {
    ERRORLOG(result,"Cannot shutdown the HTTP request queue");
  }
  m_requestQueue = NULL;

  // Cleanup the sites and groups
  Cleanup();
}

// DWORD
// HTTPServerMarlin::SendResponse(HTTPSite*    p_site
//                               ,HTTPMessage* p_message
//                               ,USHORT       p_statusCode
//                               ,PSTR         p_reason
//                               ,PSTR         p_entityString
//                               ,CString      p_authScheme
//                               ,PSTR         p_cookie      /*=NULL*/
//                               ,PSTR         p_contentType /*=NULL*/)
// {
//   HTTP_REQUEST_ID requestID = p_message->GetRequestHandle();
//   HTTP_RESPONSE   response;
//   HTTP_DATA_CHUNK dataChunk;
//   DWORD           result;
//   DWORD           bytesSent;
//   CString         challenge;
// 
//   // Initialize the HTTP response structure.
//   InitializeHttpResponse(&response,p_statusCode,p_reason);
// 
//   // Add a known header.
//   if(p_contentType && strlen(p_contentType))
//   {
//     AddKnownHeader(response,HttpHeaderContentType,p_contentType);
//   }
//   else
//   {
//     AddKnownHeader(response,HttpHeaderContentType,"text/html");
//   }
//   // Adding authentication schema challenge
//   if(p_statusCode == HTTP_STATUS_DENIED)
//   {
//     // Add authentication scheme
//     challenge = BuildAuthenticationChallenge(p_authScheme,p_site->GetAuthenticationRealm());
//     AddKnownHeader(response,HttpHeaderWwwAuthenticate,challenge);
//   }
//   else if (p_statusCode >= HTTP_STATUS_AMBIGUOUS)
//   {
//     // Log responding with error status code
//     HTTPError(__FUNCTION__,p_statusCode,"Returning from: " + p_site->GetSite());
//   }
// 
//   // Adding the body
//   if(p_entityString && strlen(p_entityString) > 0)
//   {
//     // Add an entity chunk.
//     dataChunk.DataChunkType           = HttpDataChunkFromMemory;
//     dataChunk.FromMemory.pBuffer      = p_entityString;
//     dataChunk.FromMemory.BufferLength = (ULONG) strlen(p_entityString);
//     response.EntityChunkCount         = 1;
//     response.pEntityChunks            = &dataChunk;
//   }
// 
//   // Adding a cookie
//   if(p_cookie && strlen(p_cookie) > 0)
//   {
//     AddKnownHeader(response,HttpHeaderSetCookie,p_cookie);
//   }
// 
//   // Prepare our cache-policy
//   HTTP_CACHE_POLICY policy;
//   policy.Policy        = m_policy;
//   policy.SecondsToLive = m_secondsToLive;
// 
//   // Because the entity body is sent in one call, it is not
//   // required to specify the Content-Length.
//   result = HttpSendHttpResponse(m_requestQueue,    // ReqQueueHandle
//                                 requestID,         // Request ID
//                                 0,                 // Flags
//                                 &response,         // HTTP response
//                                 &policy,           // Policy
//                                 &bytesSent,        // bytes sent  (OPTIONAL)
//                                 NULL,              // pReserved2  (must be NULL)
//                                 0,                 // Reserved3   (must be 0)
//                                 NULL,              // LPOVERLAPPED(OPTIONAL)
//                                 NULL               // pReserved4  (must be NULL)
//                                 ); 
//   if(result != NO_ERROR)
//   {
//     ERRORLOG(result,"SendHttpResponse");
//   }
//   else
//   {
//     DETAILLOGV("HTTP Send %d %s %s",p_statusCode,p_reason,p_entityString);
//   }
//   return result;
// }

// Used for canceling a WebSocket for an event stream
void 
HTTPServerMarlin::CancelRequestStream(HTTP_REQUEST_ID p_response)
{
  // Cancel the outstanding request from the request queue
  ULONG result = HttpCancelHttpRequest(m_requestQueue,p_response,NULL);
  if(result == NO_ERROR)
  {
    DETAILLOG1("Event stream connection closed");
  }
  else
  {
    ERRORLOG(result,"Event stream incorrectly canceled");
  }
}

// Receive incoming HTTP request
// bool
// HTTPServerMarlin::ReceiveIncomingRequest(HTTPMessage* p_message)
// {
//   bool   retval    = true;
//   ULONG  bytesRead = 0;
//   ULONG  entityBufferLength = INIT_HTTP_BUFFERSIZE;
//   PUCHAR entityBuffer       = nullptr;
// 
//   // Create a buffer + 1 extra byte for the closing 0
//   entityBuffer = new uchar[entityBufferLength + 1];
//   if(entityBuffer == NULL)
//   {
//     ERRORLOG(ERROR_NOT_ENOUGH_MEMORY,"Out of memory");
//     return false;
//   }
// 
//   // Reading loop
//   bool reading = true;
//   do 
//   {
//     bytesRead = 0; 
//     DWORD result = HttpReceiveRequestEntityBody(m_requestQueue
//                                                ,p_message->GetRequestHandle()
//                                                ,HTTP_RECEIVE_REQUEST_ENTITY_BODY_FLAG_FILL_BUFFER
//                                                ,entityBuffer
//                                                ,entityBufferLength
//                                                ,&bytesRead
//                                                ,NULL);
//     switch(result)
//     {
//       case NO_ERROR:          // Regular incoming body part
//                               entityBuffer[bytesRead] = 0;
//                               p_message->AddBody(entityBuffer,bytesRead);
//                               DETAILLOGV("ReceiveRequestEntityBody [%d] bytes",bytesRead);
//                               break;
//       case ERROR_HANDLE_EOF:  // Very last incoming body part
//                               if(bytesRead)
//                               {
//                                 entityBuffer[bytesRead] = 0;
//                                 p_message->AddBody(entityBuffer,bytesRead);
//                                 DETAILLOGV("ReceiveRequestEntityBody [%d] bytes",bytesRead);
//                               }
//                               reading = false;
//                               break;
//       default:                ERRORLOG(result,"ReceiveRequestEntityBody");
//                               reading = false;
//                               retval  = false;
//                               break;
//                               
//     }
//   } 
//   while(reading);
// 
//   // Clean up buffer after reading
//   delete [] entityBuffer;
//   entityBuffer = nullptr;
// 
//   // In case of a POST, try to convert character set before submitting to site
//   if(p_message->GetCommand() == HTTPCommand::http_post)
//   {
//     if(p_message->GetContentType().Find("multipart") <= 0)
//     {
//       HandleTextContent(p_message);
//     }
//   }
//   DETAILLOGV("Received %s message from: %s Size: %lu"
//             ,headers[(unsigned)p_message->GetCommand()]
//             ,(LPCTSTR)SocketToServer(p_message->GetSender())
//             ,p_message->GetBodyLength());
// 
//   // This message is read!
//   p_message->SetReadBuffer(false);
// 
//   // Receiving succeeded?
//   return retval;
// }

// Create a new WebSocket in the subclass of our server
WebSocket*
HTTPServerMarlin::CreateWebSocket(CString p_uri)
{
  return new WebSocketServer(p_uri);
}

// Receive the WebSocket stream and pass on the the WebSocket
void
HTTPServerMarlin::ReceiveWebSocket(WebSocket* /*p_socket*/,HTTP_REQUEST_ID /*p_request*/)
{
  // FOR WEBSOCKETS TO WORK ON THE STAND-ALONE MARLIN
  // IT NEEDS TO BE REWRIITEN TO DO ASYNC I/O THROUGHOUT THE SERVER!
  return;



//   // Increment thread count for the server while we are in the read loop
//   AutoIncrementPoolMax inc(m_pool);
// 
//   BYTE* buffer  = nullptr;
//   DWORD total   = 0;
//   ULONG size    = p_socket->GetFragmentSize();
//   bool  reading = true;
//   WebSocketServer* websocket = reinterpret_cast<WebSocketServer*>(p_socket);
// 
//   if(buffer == nullptr)
//   {
//     buffer = (BYTE*)malloc(size + WS_OVERHEAD);
//     total = 0;
//   }


  // testcode to receive right from the raw socket
//   PSOCKADDR_IN6 address = websocket->GetReceiverAddress();
//   SOCKET sock = socket(address->sin6_family,SOCK_STREAM,IPPROTO_TCP);
//   if(sock)
//   {
//     BOOL reuse = TRUE;
//     if(setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,(char*)&reuse,sizeof(BOOL)))
//     {
//       int error = WSAGetLastError();
//       ERRORLOG(error,"Receive from websocket");
//     }
// 
//     // Getting the socketaddress length
//     int length = 0;
//     if(address->sin6_family == AF_INET)
//     {
//       length = sizeof(SOCKADDR_IN);
//     }
//     else  // if(address->sin6_family == AF_INET6) 
//     {
//       length = sizeof(SOCKADDR_IN6);
//     }
// 
//     // Bind to the socket
//     if(bind(sock,(sockaddr*)address,length) == 0)
//     {
//       int bytes = recv(sock,(char*) buffer,size,0);
// 
//       if(bytes < 0)
//       {
//         int error = WSAGetLastError();
//         ERRORLOG(error,"Receive from websocket");
//       }
//     }
//     else
//     {
//       int error = WSAGetLastError();
//       ERRORLOG(error,"Receive from websocket");
//     }
//   }

//   do
//   {
//     DWORD bytesRead = 0;
//     DWORD result = HttpReceiveRequestEntityBody(m_requestQueue
//                                                ,p_request
//                                                ,0
//                                                ,&buffer[total]
//                                                ,size
//                                                ,&bytesRead
//                                                ,NULL);
//     switch(result)
//     {
//       case NO_ERROR:          // Regular incoming body part
//                               DETAILLOGV("WebSocket receiving [%d] bytes",bytesRead);
//                               break;
//       case ERROR_HANDLE_EOF:  // Very last incoming body part
//                               if(bytesRead)
//                               {
//                                 DETAILLOGV("WebSocket receiving [%d] bytes",bytesRead);
//                               }
//                               reading = false;
//                               break;
//       default:                ERRORLOG(result,"Receiving for WebSocket");
//                               bytesRead = 0;
//                               reading = false;
//                               break;
//     }
//     if(bytesRead)
//     {
//       total += bytesRead;
// 
//       RawFrame* frame = new RawFrame();
//       frame->m_data = buffer;
//       if(websocket->DecodeFrameBuffer(frame,total))
//       {
//         // Shrink the buffer and store it for the socket
//         frame->m_data = (BYTE*)realloc(buffer,total);
//         buffer = nullptr;
// 
//         if(websocket->StoreFrameBuffer(frame) == false)
//         {
//           // closing channel on a close operator
//           reading = false;
//         }
//       }
//       else
//       {
//         // Incomplete buffer, expand buffer to receive some more
//         buffer = (uchar*)realloc(buffer,total + size + WS_OVERHEAD);
//         // Remove unused frame buffer
//         frame->m_data = nullptr;
//         delete frame;
//       }
//     }
//   }
//   while(reading);
// 
//   // Do not forget to free the buffer memory
//   if(buffer)
//   {
//     free(buffer);
//   }
}

bool       
HTTPServerMarlin::ReceiveIncomingRequest(HTTPMessage* /*p_message*/)
{
  ASSERT(false);
  ERRORLOG(ERROR_INVALID_PARAMETER,"ReceiveIncomingRequest in HTTPServerMarlin: Should never come to here!!");
  return false;
}


// Sending response for an incoming message
void       
HTTPServerMarlin::SendResponse(HTTPMessage* p_message)
{
  HTTPRequest* request = reinterpret_cast<HTTPRequest*>(p_message->GetRequestHandle());
  if(request)
  {
    // Reset the request handle here. The response will continue async from here
    // so the next handlers cannot find a request handle to answer to
    p_message->SetRequestHandle(NULL);

    // Go send the response ASYNC
    request->StartResponse(p_message);
  }
}
bool
HTTPServerMarlin::SendResponseWebSocket(PHTTP_RESPONSE  /*p_response*/
                                       ,HTTP_REQUEST_ID /*p_requestID*/
                                       ,UKHeaders&      /*p_headers*/)
{
  // FOR WEBSOCKETS TO WORK ON THE STAND-ALONE MARLIN
  // IT NEEDS TO BE REWRIITEN TO DO ASYNC I/O THROUGHOUT THE SERVER!

  return false;
  
//   // Build protocol headers
//   CString response = "HTTP/1.1 101 Switching protocols\r\n";
//   for(auto& header : p_headers)
//   {
//     CString line;
//     line.Format("%s: %s\r\n",header.first.GetString(),header.second.GetString());
//     response += line;
//   }
//   response += "\r\n";
// 
//   // Put the response in a data chunk
//   HTTP_DATA_CHUNK dataChunk;
//   dataChunk.DataChunkType      = HttpDataChunkFromMemory;
//   dataChunk.FromMemory.pBuffer = (void*) response.GetString();
//   dataChunk.FromMemory.BufferLength = (ULONG)response.GetLength();
//   p_response->EntityChunkCount = 1;
//   p_response->pEntityChunks    = &dataChunk;
// 
// 
//   // Preparing our cache-policy
//   HTTP_CACHE_POLICY policy;
//   policy.Policy        = m_policy;
//   policy.SecondsToLive = m_secondsToLive;
// 
//   ULONG flags = HTTP_SEND_RESPONSE_FLAG_OPAQUE;
//              // HTTP_SEND_RESPONSE_FLAG_MORE_DATA;
//              // HTTP_SEND_RESPONSE_FLAG_BUFFER_DATA;
//   DWORD bytesSent = 0;
// 
//   OVERLAPPED ov;
//   RtlZeroMemory(&ov,sizeof(OVERLAPPED));
// 
//   DWORD result = HttpSendHttpResponse(m_requestQueue,      // ReqQueueHandle
//                                       p_requestID,         // Request ID
//                                       flags,               // Flags
//                                       p_response,          // HTTP response
//                                       &policy,             // Cache policy
//                                       &bytesSent,          // bytes sent  (OPTIONAL)
//                                       NULL,                // pReserved2  (must be NULL)
//                                       0,                   // Reserved3   (must be 0)
//                                       &ov,                 // LPOVERLAPPED(OPTIONAL)
//                                       NULL                 // pReserved4  (must be NULL)
//                                       );
//   if(result != NO_ERROR)
//   {
//     ERRORLOG(result,"Send WebSockets handshake");
//   }
//   else
//   {
//     DETAILLOGV("Send WebSocket handshake: [%d] bytes sent",bytesSent);
//   }
//   return (result == NO_ERROR);
}



//////////////////////////////////////////////////////////////////////////
//
// EVENT STREAMS
//
//////////////////////////////////////////////////////////////////////////

// Init the stream response
bool
HTTPServerMarlin::InitEventStream(EventStream& /*p_stream*/)
{
//   // First comment to push to the stream (not an event!)
//   CString init = m_eventBOM ? ConstructBOM() : "";
//   init += ":init event-stream\n";
// 
//   // Initialize the HTTP response structure.
//   InitializeHttpResponse(&p_stream.m_response,HTTP_STATUS_OK,"OK");
// 
//   // Add a known header.
//   AddKnownHeader(p_stream.m_response,HttpHeaderContentType,"text/event-stream");
// 
//   // Add an entity chunk.
//   HTTP_DATA_CHUNK dataChunk;
//   dataChunk.DataChunkType           = HttpDataChunkFromMemory;
//   dataChunk.FromMemory.pBuffer      = (PVOID)init.GetString();
//   dataChunk.FromMemory.BufferLength = (ULONG)init.GetLength();
//   // Add chunk to response
//   p_stream.m_response.EntityChunkCount = 1;
//   p_stream.m_response.pEntityChunks    = &dataChunk;
// 
//   // Preparing the cache-policy
//   HTTP_CACHE_POLICY policy;
//   policy.Policy = HttpCachePolicyNocache;
//   policy.SecondsToLive = 0;
// 
//   // Because the entity body is sent in one call, it is not
//   // required to specify the Content-Length.
//   DWORD bytesSent = 0;
//   ULONG flags  = HTTP_SEND_RESPONSE_FLAG_MORE_DATA;
//   DWORD result = HttpSendHttpResponse(m_requestQueue,         // ReqQueueHandle
//                                       p_stream.m_requestID,   // Request ID
//                                       flags,                  // Flags
//                                       &p_stream.m_response,   // HTTP response
//                                       &policy,                // Policy
//                                       &bytesSent,             // bytes sent  (OPTIONAL)
//                                       NULL,                   // pReserved2  (must be NULL)
//                                       0,                      // Reserved3   (must be 0)
//                                       NULL,                   // LPOVERLAPPED(OPTIONAL)
//                                       NULL                    // pReserved4  (must be NULL)
//                                       );
//   if(result != NO_ERROR)
//   {
//     ERRORLOG(result,"SendHttpResponse for init of event-stream");
//   }
//   else
//   {
//     DETAILLOGS("HTTP Send 200 OK ",init);
//   }
  return NO_ERROR;
}

// Sending a chunk to an event stream
bool
HTTPServerMarlin::SendResponseEventBuffer(HTTP_REQUEST_ID  p_requestID
                                         ,const char*      p_buffer
                                         ,size_t           p_length
                                         ,bool             p_continue /*=true*/)
{
  DWORD  result    = 0;
  DWORD  bytesSent = 0;
  HTTP_DATA_CHUNK dataChunk;

  // Only if a buffer present
  dataChunk.DataChunkType           = HttpDataChunkFromMemory;
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
    // Final closing of the connection
    if(p_continue == false)
    {
      DETAILLOG1("Event stream connection closed");
    }
  }
  return (result == NO_ERROR);
}

// Send to a WebSocket
bool
HTTPServerMarlin::SendSocket(RawFrame& p_frame,HTTP_REQUEST_ID p_request)
{
  DWORD  result    = 0;
  DWORD  bytesSent = 0;
  HTTP_DATA_CHUNK dataChunk;

  // Only if a buffer present
  dataChunk.DataChunkType           = HttpDataChunkFromMemory;
  dataChunk.FromMemory.pBuffer      = (void*)p_frame.m_data;
  dataChunk.FromMemory.BufferLength = (ULONG)(p_frame.m_headerLength + p_frame.m_payloadLength);

  // Keep the socket open!
  ULONG flags = HTTP_SEND_RESPONSE_FLAG_MORE_DATA;

  // Go send it
  result = HttpSendResponseEntityBody(m_requestQueue,      // ReqQueueHandle
                                      p_request,           // Request ID
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
    ERRORLOG(result,"HttpSendResponseEntityBody failed for WebSocket");
  }
  else
  {
    DETAILLOGV("HttpSendResponseEntityBody [%d] bytes sent",bytesSent);
  }
  return (result == NO_ERROR);
}

// Cancel and close a WebSocket
bool
HTTPServerMarlin::FlushSocket(HTTP_REQUEST_ID /*p_request*/)
{
  return true;
}
