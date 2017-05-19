/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: HTTPServer.h
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

//////////////////////////////////////////////////////////////////////////
//
// HTTPServer
//
//////////////////////////////////////////////////////////////////////////

#pragma once
#include "Analysis.h"
#include "HTTPMessage.h"
#include "WebConfig.h"
#include "CreateURLPrefix.h"
#include "HPFCounter.h"
#include "ServerEvent.h"
#include "ThreadPool.h"
#include "MediaType.h"
#include "ErrorReport.h"
#include "EventStream.h"
#include "Version.h"
#include <http.h>
#include <winhttp.h>
#include <string>
#include <vector>
#include <deque>
// For SSPI functions
#ifndef SECURITY_WIN32
#define SECURITY_WIN32
#endif
#include <security.h>

// Using wstring for Unicode conversions
using std::wstring;

// Initial request buffer length should be above MTU of 8KB
// Otherwise the network traffic will decrease in efficiency
// Should be the same as the initial size for the HTTPClient!
constexpr auto INIT_HTTP_BUFFERSIZE   = (32 * 1024);
// Initial HTTP backlog queue length
constexpr auto INIT_HTTP_BACKLOGQUEUE = 64;
constexpr auto MAXX_HTTP_BACKLOGQUEUE = 640;

// Static globals for the server as a whole
// Can be set through the web.config reading of the HTTPServer
extern unsigned long g_streaming_limit; // = STREAMING_LIMIT;
extern unsigned long g_compress_limit;  // = COMPRESS_LIMIT;

// From <nstatus.h>
#ifndef STATUS_LOGON_FAILURE
#define STATUS_LOGON_FAILURE  ((LONG)0xC000006DL)
#endif

// Windows 10 extension on HTTP_REQUEST_INFO_TYPE
typedef enum _HTTP_REQUEST_INFO_TYPE_W10
{
//HttpRequestInfoTypeAuth,           // Already in HTTP_REQUEST_INFO_TYPE
//HttpRequestInfoTypeChannelBind,    // Already in HTTP_REQUEST_INFO_TYPE
  HttpRequestInfoTypeSslProtocol     = 2,
  HttpRequestInfoTypeSslTokenBinding = 3
} 
HTTP_REQUEST_INFO_TYPE_W10,*PHTTP_REQUEST_INFO_TYPE_W10;

// From the Windows 10 SDK
typedef struct _HTTP_SSL_PROTOCOL_INFO
{
  ULONG Protocol;
  ULONG CipherType;
  ULONG CipherStrength;
  ULONG HashType;
  ULONG HashStrength;
  ULONG KeyExchangeType;
  ULONG KeyExchangeStrength;
} 
HTTP_SSL_PROTOCOL_INFO,*PHTTP_SSL_PROTOCOL_INFO;

enum class SendHeader
{
  HTTP_SH_MICROSOFT = 1         // Send standard Microsoft server header
 ,HTTP_SH_MARLIN                // Send our standard server header
 ,HTTP_SH_APPLICATION           // Application's server name
 ,HTTP_SH_WEBCONFIG             // Send server from the web.config
 ,HTTP_SH_HIDESERVER            // Hide the server type - do not send header
};

// Forward declarations
class LogAnalysis;
class HTTPSite;
class HTTPURLGroup;
class HTTPRequest;
class JSONMessage;
class WebServiceServer;
class WebSocket;
class RawFrame;

// Type declarations for mappings
using SiteMap     = std::map<CString,HTTPSite*>;
using EventMap    = std::multimap<CString,EventStream*>;
using ServiceMap  = std::map<CString,WebServiceServer*>;
using URLGroupMap = std::vector<HTTPURLGroup*>;
using UKHeaders   = std::multimap<CString,CString>;
using SocketMap   = std::map<CString,WebSocket*>;;
using RequestMap  = std::deque<HTTPRequest*>;

// Global error variable in Thread-Local-Storage
// Per-thread basis error status
extern __declspec(thread) ULONG tls_lastError;

// All the media types
extern MediaTypes g_media;

//////////////////////////////////////////////////////////////////////////
//
// HTTP Server
//
//////////////////////////////////////////////////////////////////////////

// The HTTPServer itself follows here
//
class HTTPServer
{
public:
  HTTPServer(CString p_name);
  virtual ~HTTPServer();

  // Running the server 
  virtual void       Run() = 0;
  // Stop the server
  virtual void       StopServer() = 0;
  // Initialise a HTTP server and server-session
  virtual bool       Initialise() = 0;
  // Return a version string
  virtual CString    GetVersion() = 0;
  // Create a site to bind the traffic to
  virtual HTTPSite*  CreateSite(PrefixType    p_type
                                  ,bool          p_secure
                                  ,int           p_port
                                  ,CString       p_baseURL
                                  ,bool          p_subsite  = false
                                  ,LPFN_CALLBACK p_callback = nullptr) = 0;
  // Delete a site from the remembered set of sites
  virtual bool       DeleteSite(int p_port,CString p_baseURL,bool p_force = false) = 0;
  // Receive (the rest of the) incoming HTTP request
  virtual bool       ReceiveIncomingRequest(HTTPMessage* p_message) = 0;
  // Create a new WebSocket in the subclass of our server
  virtual WebSocket* CreateWebSocket(CString p_uri) = 0;
  // Receive the WebSocket stream and pass on the the WebSocket
  virtual void       ReceiveWebSocket(WebSocket* p_socket,HTTP_REQUEST_ID p_request) = 0;
  // Send to a WebSocket
  virtual bool       SendSocket(RawFrame& p_frame,HTTP_REQUEST_ID p_request) = 0;
  // Flushing a WebSocket intermediate
  virtual bool       FlushSocket (HTTP_REQUEST_ID p_request) = 0;
  // Used for canceling a WebSocket for an event stream
  virtual void       CancelRequestStream(HTTP_REQUEST_ID p_response) = 0;
  // Sending a response on a message
  virtual void       SendResponse(HTTPMessage* p_message) = 0;

  // SETTERS
 
  // MANDATORY: Set the webroot of the server
  void       SetWebroot(CString p_webroot);
  // MANDATORY: Set the ErrorReport object
  void       SetErrorReport(ErrorReport* p_report);
  // OPTIONAL: Set the client error page (range 400-417)
  void       SetClientErrorPage(CString& p_errorPage);
  // OPTIONAL: Set the server error page (range 500-505)
  void       SetServerErrorPage(CString& p_errorPage);
  // OPTIONAL: Set a logging object (lest we create our own!)
  void       SetLogging(LogAnalysis* p_log);
  // OPTIONAL: Set another name
  void       SetName(CString p_name);
  // OPTIONAL: Set cache policy
  void       SetCachePolicy(HTTP_CACHE_POLICY_TYPE p_type,ULONG p_seconds);
  // OPTIONAL: Set the event-streams keep-alive heartbeat
  void       SetEventKeepAlive(ULONG p_milliseconds);
  // OPTIONAL: Set the reconnect time for the clients
  void       SetEventRetryConnection(ULONG p_milliseconds);
  // OPTIONAL: Set HTTP Queue length
  void       SetQueueLength(ULONG p_length);
  // OPTIONAL: Set (detailed) logging of the server components
  void       SetDetailedLogging(bool p_detail);
  // OPTIONAL: Sent a BOM in the event stream
  void       SetByteOrderMark(bool p_mark);

  // GETTERS

  // Getting the server's name (according to application)
  CString     GetName();
  // Getting the server's name (according to the config file)
  CString     GetConfiguredName();
  // Getting the WebRoot
  CString     GetWebroot();
  // Get host name of the server's machine
  CString     GetHostname();
  // Last error encountered
  ULONG       GetLastError();
  // Is the server still running
  bool        GetIsRunning();
  // Get High Performance counter
  HPFCounter* GetCounter();
  // Get map with URL info
  SiteMap*    GetSiteMap();
  // Get the threadpool
  ThreadPool* GetThreadPool();
  // Get the event-stream keep alive time
  ULONG       GetEventKeepAlive();
  // Get the client retry connection time
  ULONG       GetEventRetryConnection();
  // Reference to the WebConfig
  WebConfig&  GetWebConfig();
  // Getting the logfile
  LogAnalysis* GetLogfile();
  // Server session ID for the groups
  HTTP_SERVER_SESSION_ID GetServerSessionID();
  // Getting the request queue (for the group)
  HANDLE      GetRequestQueue();
  // Getting the error report object
  ErrorReport* GetErrorReport();
  // Get the fact that we do detailed logging
  bool        GetDetailedLogging();
  // Has sub-sites registered
  bool        GetHasSubsites();
  // Getting the cache policy
  HTTP_CACHE_POLICY_TYPE GetCachePolicy();
  // Getting the cache lifetime
  ULONG       GetCacheSecondsToLive();
  // How we send our server name header
  SendHeader  GetSendServerHeader();

  // FUNCTIONS

  // Detailed and error logging to the log file
  void       DetailLog (const char* p_function,LogType p_type,const char* p_text);
  void       DetailLogS(const char* p_function,LogType p_type,const char* p_text,const char* p_extra);
  void       DetailLogV(const char* p_function,LogType p_type,const char* p_text,...);
  void       ErrorLog  (const char* p_function,DWORD p_code,CString p_text);
  void       HTTPError (const char* p_function,int p_status,CString p_text);
  // Log SSL Info of the connection
  void       LogSSLConnection(PHTTP_SSL_PROTOCOL_INFO p_sslInfo);

  // Find HTTPSite for an URL
  HTTPSite*  FindHTTPSite(int p_port,PCWSTR   p_url);
  HTTPSite*  FindHTTPSite(int p_port,CString& p_url);
  HTTPSite*  FindHTTPSite(HTTPSite* p_default,CString& p_url);

  // Outstanding asynchronous I/O requests
  void         RegisterHTTPRequest(HTTPRequest* p_request);
  void       UnRegisterHTTPRequest(HTTPRequest* p_request);

  // Sending response for an incoming message
  void       SendResponse(SOAPMessage* p_message);
  void       SendResponse(JSONMessage* p_message);
  // Return the number of push-event-streams for this URL, and probably for a user
  int        HasEventStreams(int p_port,CString p_url,CString p_user = "");
  // Return the fact that we have an event stream
  bool       HasEventStream(EventStream* p_stream);
  // Send to a server push event stream / deleting p_event
  bool       SendEvent(int p_port,CString p_site,ServerEvent* p_event,CString p_user = "");
  // Send to a server push event stream on EventStream basis
  bool       SendEvent(EventStream* p_stream,ServerEvent* p_event,bool p_continue = true);
  // Close an event stream for one stream only
  bool       CloseEventStream(EventStream* p_stream);
  // Close and abort an event stream for whatever reason
  bool       AbortEventStream(EventStream* p_stream);
  // Close event streams for an URL and probably a user
  void       CloseEventStreams(int p_port,CString p_url,CString p_user = "");
  // Monitor all server push event streams
  void       EventMonitor();
  // Register a WebServiceServer
  bool       RegisterService(WebServiceServer* p_service);
  // Remove registration of a service
  bool       UnRegisterService (CString p_serviceName);
  // Register a WebSocket
  bool       RegisterSocket(WebSocket* p_socket);
  // Remove registration of a WebSocket
  bool       UnRegisterWebSocket(WebSocket* p_socket);
  // Find our extra header for RemoteDesktop (Citrix!) support
  int        FindRemoteDesktop(USHORT p_count,PHTTP_UNKNOWN_HEADER p_headers);
  // Authentication failed for this reason
  CString    AuthenticationStatus(SECURITY_STATUS p_secStatus);
  // Finding a previous registered service endpoint
  WebServiceServer* FindService(CString p_serviceName);
  // Finding a previous registered WebSocket
  WebSocket*        FindWebSocket(CString p_uri);
  // Finding the locking object for the sites.
  CRITICAL_SECTION* AcquireSitesLockObject();
  // Handle text-based content-type messages
  void       HandleTextContent(HTTPMessage* p_message);
  // Build the www-auhtenticate challenge
  CString    BuildAuthenticationChallenge(CString p_authScheme,CString p_realm);
  // Find less known verb
  HTTPCommand GetUnknownVerb(PCSTR p_verb);
  // Response in the server error range (500-505)
  void       RespondWithServerError(HTTPMessage*    p_message
                                   ,int             p_error
                                   ,CString         p_reason
                                   ,CString         p_authScheme
                                   ,CString         p_cookie = "");
  // Response in the client error range (400-417)
  void       RespondWithClientError(HTTPMessage*    p_message
                                   ,int             p_error
                                   ,CString         p_reason
                                   ,CString         p_authScheme
                                   ,CString         p_cookie = "");
  // REQUEST HEADER METHODS

  // RFC 2616: paragraph 14.25: "if-modified-since"
  bool          DoIsModifiedSince(HTTPMessage* p_msg);
  // Register server push event stream for this site
  EventStream*  SubscribeEventStream(HTTPSite* p_site,CString p_url,CString& p_pad,HTTP_REQUEST_ID p_requestID,HANDLE p_token);

protected:
  // Cleanup the server
  virtual void  Cleanup() = 0;
  // Init the stream response
  virtual bool  InitEventStream(EventStream& p_stream) = 0;
  // Initialise the logging and error mechanism
  virtual void  InitLogging() = 0;
  // Initialise general server header settings
  virtual void  InitHeaders() = 0;
  // Initialise the hard server limits in bytes
  virtual void  InitHardLimits() = 0;
  // Initialise the servers WebRoot
  virtual void  InitWebroot(CString p_webroot) = 0;
  // Initialise the threadpool limits
  virtual void  InitThreadpoolLimits(int& p_minThreads,int& p_maxThreads,int& p_stackSize) = 0;
  // Register a URL to listen on
  bool          RegisterSite(HTTPSite* p_site,CString p_urlPrefix);
  // General checks before starting
  bool      GeneralChecks();
  // Checks if all sites are started
  void      CheckSitesStarted();
  // Make a "port:url" registration name
  CString   MakeSiteRegistrationName(int p_port,CString p_url);
    // Form event to a stream string
  CString   EventToString(ServerEvent* p_event);
  // Try to start the even heartbeat monitor
  void      TryStartEventHartbeat();
  // Check all event streams for the heartbeat monitor
  UINT      CheckEventStreams();
  // Set the error status
  void      SetError(int p_error);
  // For the handling of the event streams
  virtual bool SendResponseEventBuffer(HTTP_REQUEST_ID p_response,const char* p_buffer,size_t p_totalLength,bool p_continue = true) = 0;

  // Protected data
  CString                 m_name;                   // How the outside world refers to me
  CString                 m_hostName;               // How i am registered with the DNS
  bool                    m_initialized { false };  // Server initialized or not
  bool                    m_running     { false };  // In our main loop
  CString                 m_webroot;                // Webroot of the server
  // HTTP-API V 2.0 session 
  HTTP_SERVER_SESSION_ID  m_session     { NULL  };  // Server session within Windows OS
  HANDLE                  m_requestQueue{ NULL  };  // Request queue within an URL group
  ULONG                   m_queueLength { INIT_HTTP_BACKLOGQUEUE };     // HTTP backlog queue length
  // Bookkeeping
  CString                 m_serverErrorPage;        // Current server error
  CString                 m_clientErrorPage;        // Current client error
  HTTP_CACHE_POLICY_TYPE  m_policy   { HttpCachePolicyNocache };        // Cache policy
  ULONG                   m_secondsToLive  { 0 };   // Seconds to live in the cache
  ThreadPool              m_pool;                   // Our threadpool for the server
  WebConfig               m_webConfig;              // Webconfig from current directory
  bool                    m_detail   { false   };   // Do detailed logging
  LogAnalysis*            m_log      { nullptr };   // Logging object
  bool                    m_logOwner { false   };   // Server owns the log
  HPFCounter              m_counter;                // High performance counter
  SendHeader              m_sendHeader { SendHeader::HTTP_SH_HIDESERVER }; // Server header to send
  CString                 m_configServerName;       // Server header name from web.config
  ErrorReport*            m_errorReport{ nullptr};  // Error report handling
  // All sites of the server
  SiteMap                 m_allsites;               // All URL's and context pointers
  ServiceMap              m_allServices;            // All Services
  CRITICAL_SECTION        m_sitesLock;              // Creating/starting/stopping sites
  bool                    m_hasSubsites{ false };   // Server serves at least 1 sub-site
  // All requests
  RequestMap              m_requests;               // All outstanding HTTP requests
  // Server push events
  EventMap                m_eventStreams;           // Server push event streams
  HANDLE                  m_eventMonitor{ NULL };   // Monitoring the event streams
  HANDLE                  m_eventEvent  { NULL };   // Monitor wakes up on this event
  ULONG                   m_eventKeepAlive{ DEFAULT_EVENT_KEEPALIVE };  // MS between keep-alive pulses
  ULONG                   m_eventRetryTime{ DEFAULT_EVENT_RETRYTIME };  // Clients must retry after this time
  bool                    m_eventBOM   { false };   // Prepend all event with a Byte-order-mark
  CRITICAL_SECTION        m_eventLock;              // Pulsing events or accessing streams
  // WebSocket
  SocketMap               m_sockets;                // Registered WebSockets
};

inline CString
HTTPServer::GetName()
{
  return m_name;
}

inline CString
HTTPServer::GetConfiguredName()
{
  return m_configServerName;
}

inline CString
HTTPServer::GetWebroot()
{
  return m_webroot;
}

inline CString
HTTPServer::GetHostname()
{
  return m_hostName;
}

inline void
HTTPServer::SetClientErrorPage(CString& p_errorPage)
{
  m_clientErrorPage = p_errorPage;
}

inline void
HTTPServer::SetServerErrorPage(CString& p_errorPage)
{
  m_serverErrorPage = p_errorPage;
}

inline void
HTTPServer::SetName(CString p_name)
{
  m_name = p_name;
}

inline bool
HTTPServer::GetIsRunning()
{
  return m_running;
}

inline HPFCounter* 
HTTPServer::GetCounter()
{
  return &m_counter;
}

inline SiteMap*
HTTPServer::GetSiteMap()
{
  return &m_allsites;
}

inline ThreadPool* 
HTTPServer::GetThreadPool()
{
  return &m_pool;
}

inline void
HTTPServer::SetEventKeepAlive(ULONG p_milliseconds)
{
  m_eventKeepAlive = p_milliseconds;
}

inline ULONG
HTTPServer::GetEventKeepAlive()
{
  return m_eventKeepAlive;
}

inline void
HTTPServer::SetEventRetryConnection(ULONG p_milliseconds)
{
  m_eventRetryTime = p_milliseconds;
}

inline ULONG
HTTPServer::GetEventRetryConnection()
{
  return m_eventRetryTime;
}

inline void
HTTPServer::SetByteOrderMark(bool p_mark)
{
  m_eventBOM = p_mark;
}

inline WebConfig&
HTTPServer::GetWebConfig()
{
  return m_webConfig;
}

inline LogAnalysis* 
HTTPServer::GetLogfile()
{
  return m_log;
}

inline HTTP_SERVER_SESSION_ID 
HTTPServer::GetServerSessionID()
{
  return m_session;
}

inline HANDLE
HTTPServer::GetRequestQueue()
{
  return m_requestQueue;
}

inline void
HTTPServer::SetDetailedLogging(bool p_detail)
{
  m_detail = p_detail;
}

inline bool
HTTPServer::GetDetailedLogging()
{
  return m_detail;
}

inline CRITICAL_SECTION* 
HTTPServer::AcquireSitesLockObject()
{
  return &m_sitesLock;
}

inline void
HTTPServer::SetErrorReport(ErrorReport* p_report)
{
  m_errorReport = p_report;
}

inline ErrorReport*
HTTPServer::GetErrorReport()
{
  return m_errorReport;
}

inline bool
HTTPServer::GetHasSubsites()
{
  return m_hasSubsites;
}

inline HTTP_CACHE_POLICY_TYPE 
HTTPServer::GetCachePolicy()
{
  return m_policy;
}

inline ULONG
HTTPServer::GetCacheSecondsToLive()
{
  return m_secondsToLive;
}

inline SendHeader  
HTTPServer::GetSendServerHeader()
{
  return m_sendHeader;
}
