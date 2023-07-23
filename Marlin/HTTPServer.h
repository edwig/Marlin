/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: HTTPServer.h
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

//////////////////////////////////////////////////////////////////////////
//
// HTTPServer
//
//////////////////////////////////////////////////////////////////////////

#pragma once
#include "LogAnalysis.h"
#include "HTTPMessage.h"
#include "MarlinConfig.h"
#include "CreateURLPrefix.h"
#include "HPFCounter.h"
#include "ServerEvent.h"
#include "ThreadPool.h"
#include "MediaType.h"
#include "ErrorReport.h"
#include "EventStream.h"
#include "Version.h"
#include <wincred.h>
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
// Timeout after bruteforce attack
constexpr auto TIMEOUT_BRUTEFORCE     = (10 * CLOCKS_PER_SEC);

// Websockets are two sided sockets, not HTTP
#ifndef HANDLER_HTTPSYS_UNFRIENDLY
#define HANDLER_HTTPSYS_UNFRIENDLY 9
#endif

// Static globals for the server as a whole
// Can be set through the Marlin.config reading of the HTTPServer
extern unsigned long g_streaming_limit; // = STREAMING_LIMIT;
extern unsigned long g_compress_limit;  // = COMPRESS_LIMIT;

#ifndef _WIN32_WINNT_WIN10
// Windows 10 extension on HTTP_REQUEST_INFO_TYPE
typedef enum _HTTP_REQUEST_INFO_TYPE_W10
{
//HttpRequestInfoTypeAuth,           = 0   // Already in HTTP_REQUEST_INFO_TYPE
//HttpRequestInfoTypeChannelBind,    = 1   // Already in HTTP_REQUEST_INFO_TYPE
  HttpRequestInfoTypeSslProtocol     = 2,
  HttpRequestInfoTypeSslTokenBinding = 3
} 
HTTP_REQUEST_INFO_TYPE_W10,*PHTTP_REQUEST_INFO_TYPE_W10;
#endif

#ifndef _WIN32_WINNT_WIN10
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
#endif

// How to handle the "server" header
enum class SendHeader
{
  HTTP_SH_MICROSOFT = 1         // Send standard Microsoft server header
 ,HTTP_SH_MARLIN                // Send our standard server header
 ,HTTP_SH_APPLICATION           // Application's server name
 ,HTTP_SH_WEBCONFIG             // Send server from the Marlin.config
 ,HTTP_SH_HIDESERVER            // Hide the server type - do not send header
};

// Registration of DDOS attacks on the server
class DDOS
{
public:
  DDOS()
  {
    memset(&m_sender,0,sizeof(SOCKADDR_IN6));
  }

  SOCKADDR_IN6  m_sender;
  XString       m_abspath;
  clock_t       m_beginTime { 0 };
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
using SiteMap     = std::map<XString,HTTPSite*>;
using EventMap    = std::multimap<XString,EventStream*>;
using ServiceMap  = std::map<XString,WebServiceServer*>;
using URLGroupMap = std::vector<HTTPURLGroup*>;
using UKHeaders   = std::multimap<XString,XString>;
using SocketMap   = std::map<XString,WebSocket*>;;
using RequestMap  = std::deque<HTTPRequest*>;
using DDOSMap     = std::vector<DDOS>;

// All the media types
extern MediaTypes* g_media;

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
  HTTPServer(XString p_name);
  virtual ~HTTPServer();

  // Running the server 
  virtual void       Run() = 0;
  // Stop the server
  virtual void       StopServer() = 0;
  // Initialise a HTTP server and server-session
  virtual bool       Initialise() = 0;
  // Return a version string
  virtual XString    GetVersion() = 0;
  // Create a site to bind the traffic to
  virtual HTTPSite*  CreateSite(PrefixType    p_type
                                  ,bool          p_secure
                                  ,int           p_port
                                  ,XString       p_baseURL
                                  ,bool          p_subsite  = false
                                  ,LPFN_CALLBACK p_callback = nullptr) = 0;
  // Delete a site from the remembered set of sites
  virtual bool       DeleteSite(int p_port,XString p_baseURL,bool p_force = false) = 0;
  // Receive (the rest of the) incoming HTTP request
  virtual bool       ReceiveIncomingRequest(HTTPMessage* p_message) = 0;
  // Create a new WebSocket in the subclass of our server
  virtual WebSocket* CreateWebSocket(XString p_uri) = 0;
  // Receive the WebSocket stream and pass on the the WebSocket
  virtual void       ReceiveWebSocket(WebSocket* p_socket,HTTP_OPAQUE_ID p_request) = 0;
  // Flushing a WebSocket intermediate
  virtual bool       FlushSocket (HTTP_OPAQUE_ID p_request,XString p_prefix) = 0;
  // Used for canceling a WebSocket for an event stream
  virtual void       CancelRequestStream(HTTP_OPAQUE_ID p_response,bool p_reset = false) = 0;
  // Sending a response on a message
  virtual void       SendResponse(HTTPMessage* p_message) = 0;
  // Sending a response as a chunk
  virtual void       SendAsChunk(HTTPMessage* p_message,bool p_final = false) = 0;

  // SETTERS
 
  // MANDATORY: Set the webroot of the server
  void       SetWebroot(XString p_webroot);
  // MANDATORY: Set the ErrorReport object
  void       SetErrorReport(ErrorReport* p_report);
  // OPTIONAL: Set the client error page (range 400-417)
  void       SetClientErrorPage(const XString& p_errorPage);
  // OPTIONAL: Set the server error page (range 500-505)
  void       SetServerErrorPage(const XString& p_errorPage);
  // OPTIONAL: Set a logging object (lest we create our own!)
  void       SetLogging(LogAnalysis* p_log);
  // OPTIONAL: Set another name
  void       SetName(XString p_name);
  // OPTIONAL: Set cache policy
  void       SetCachePolicy(HTTP_CACHE_POLICY_TYPE p_type,ULONG p_seconds);
  // OPTIONAL: Set the event-streams keep-alive heartbeat
  void       SetEventKeepAlive(ULONG p_milliseconds);
  // OPTIONAL: Set the reconnect time for the clients
  void       SetEventRetryConnection(ULONG p_milliseconds);
  // OPTIONAL: Set HTTP Queue length
  void       SetQueueLength(ULONG p_length);
  // OPTIONAL: Sent a BOM in the event stream
  void       SetByteOrderMark(bool p_mark);
  // OPTIONAL: Set (detailed) logging of the server components
  // DEPRECATED: Do no longer use this interface!
  void       SetDetailedLogging(bool p_detail);
  // OPTIONAL: Set (detailed) logging of the server components
  void       SetLogLevel(int p_logLevel);

  // GETTERS

  // Getting the server's name (according to application)
  XString     GetName();
  // Getting the server's name (according to the config file)
  XString     GetConfiguredName();
  // Getting the WebRoot
  XString     GetWebroot();
  // Get host name of the server's machine
  XString     GetHostname();
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
  // Reference to the WebConfigIIS
  MarlinConfig&  GetWebConfig();
  // Getting the logfile
  LogAnalysis* GetLogfile();
  // Server session ID for the groups
  HTTP_SERVER_SESSION_ID GetServerSessionID();
  // Getting the request queue (for the group)
  HANDLE      GetRequestQueue();
  // Getting the error report object
  ErrorReport* GetErrorReport();
  // Get the fact that we do detailed logging
  int         GetLogLevel();
  // DEPRECATED old method of gettin the logging
  bool        GetDetailedLogging();
  // Has sub-sites registered
  bool        GetHasSubsites();
  // Getting the cache policy
  HTTP_CACHE_POLICY_TYPE GetCachePolicy();
  // Getting the cache lifetime
  ULONG       GetCacheSecondsToLive();
  // How we send our server name header
  SendHeader  GetSendServerHeader();
  // Event stream starts with BOM
  bool        GetEventBOM();
  // Exposes the server-sent-event lock
  CRITICAL_SECTION* GetEventLock();

  // FUNCTIONS

  // Detailed and error logging to the log file
  void       DetailLog (const char* p_function,LogType p_type,const char* p_text);
  void       DetailLogS(const char* p_function,LogType p_type,const char* p_text,const char* p_extra);
  void       DetailLogV(const char* p_function,LogType p_type,const char* p_text,...);
  void       ErrorLog  (const char* p_function,DWORD p_code,XString p_text);
  void       HTTPError (const char* p_function,int p_status,XString p_text);
  // Log SSL Info of the connection
  void       LogSSLConnection(PHTTP_SSL_PROTOCOL_INFO p_sslInfo);

  // Find HTTPSite for an URL
  HTTPSite*  FindHTTPSite(int p_port,PCWSTR p_url);
  HTTPSite*  FindHTTPSite(int p_port,const XString& p_url);
  HTTPSite*  FindHTTPSite(HTTPSite* p_default,const XString& p_url);

  // Logging and tracing: The response
  void      LogTraceResponse(PHTTP_RESPONSE p_response,FileBuffer* p_buffer);
  void      LogTraceResponse(PHTTP_RESPONSE p_response,unsigned char* p_buffer,unsigned p_length);
  // Logging and tracing: The request
  void      LogTraceRequest(PHTTP_REQUEST p_request,FileBuffer* p_buffer);
  void      LogTraceRequestBody(FileBuffer* p_buffer);

  // Outstanding asynchronous I/O requests
  void         RegisterHTTPRequest(HTTPRequest* p_request);
  void       UnRegisterHTTPRequest(const HTTPRequest* p_request);

  // Check authentication of a HTTP request
  bool       CheckAuthentication(PHTTP_REQUEST  p_request
                                ,HTTP_OPAQUE_ID p_id
                                ,HTTPSite*      p_site
                                ,XString&       p_rawUrl
                                ,XString        p_authorize
                                ,HANDLE&        p_token);
  // Sending response for an incoming message
  void       SendResponse(SOAPMessage* p_message);
  void       SendResponse(JSONMessage* p_message);
  // Return the number of push-event-streams for this URL, and probably for a user
  int        HasEventStreams(int p_port,XString p_url,XString p_user = "");
  // Return the fact that we have an event stream
  bool       HasEventStream(const EventStream* p_stream);
  // Send to a server push event stream / deleting p_event
  bool       SendEvent(int p_port,XString p_site,ServerEvent* p_event,XString p_user = "");
  // Send to a server push event stream on EventStream basis
  bool       SendEvent(EventStream* p_stream,ServerEvent* p_event,bool p_continue = true);
  // Close an event stream for one stream only
  bool       CloseEventStream(const EventStream* p_stream);
  // Close and abort an event stream for whatever reason
  bool       AbortEventStream(EventStream* p_stream);
  // Close event streams for an URL and probably a user
  void       CloseEventStreams(int p_port,XString p_url,XString p_user = "");
  // Delete event stream
  void       RemoveEventStream(CString p_url);
  void       RemoveEventStream(const EventStream* p_stream);
  // Monitor all server push event streams
  void       EventMonitor();
  // Register a WebServiceServer
  bool       RegisterService(WebServiceServer* p_service);
  // Remove registration of a service
  bool       UnRegisterService (XString p_serviceName);
  // Register a WebSocket
  bool       RegisterSocket(WebSocket* p_socket);
  // Remove registration of a WebSocket
  bool       UnRegisterWebSocket(WebSocket* p_socket);
  // Find our extra header for RemoteDesktop (Citrix!) support
  int        FindRemoteDesktop(USHORT p_count,PHTTP_UNKNOWN_HEADER p_headers);
  // Authentication failed for this reason
  XString    AuthenticationStatus(SECURITY_STATUS p_secStatus);
  // Find routing information within the site
  void       CalculateRouting(const HTTPSite* p_site,HTTPMessage* p_message);
  // Finding a previous registered service endpoint
  WebServiceServer* FindService(XString p_serviceName);
  // Finding a previous registered WebSocket
  WebSocket*        FindWebSocket(XString p_uri);
  // Finding the locking object for the sites.
  CRITICAL_SECTION* AcquireSitesLockObject();
  // Handle text-based content-type messages
  void       HandleTextContent(HTTPMessage* p_message);
  // Build the WWW-authenticate challenge
  XString    BuildAuthenticationChallenge(XString p_authScheme,XString p_realm);
  // Find less known verb
  HTTPCommand GetUnknownVerb(PCSTR p_verb);
  // Response in the server error range (500-505)
  void       RespondWithServerError(HTTPMessage*    p_message
                                   ,int             p_error
                                   ,XString         p_reason);
  void       RespondWithServerError(HTTPMessage*    p_message
                                   ,int             p_error);
  // Response in the client error range (400-417)
  void       RespondWithClientError(HTTPMessage*    p_message
                                   ,int             p_error
                                   ,XString         p_reason
                                   ,XString         p_authScheme = ""
                                   ,XString         p_realm      = "");
  // Response in case of succesful sending of 2FA code
  void       RespondWith2FASuccess (HTTPMessage*    p_message
                                   ,XString         p_body);
  // REQUEST HEADER METHODS

  // RFC 2616: paragraph 14.25: "if-modified-since"
  bool          DoIsModifiedSince(HTTPMessage* p_msg);
  // Register server push event stream for this site
  EventStream*  SubscribeEventStream(PSOCKADDR_IN6   p_sender
                                    ,int             p_desktop
                                    ,HTTPSite*       p_site
                                    ,XString         p_url
                                    ,const XString&  p_pad
                                    ,HTTP_OPAQUE_ID  p_requestID
                                    ,HANDLE          p_token);
  // DDOS Attack mechanism
  void       RegisterDDOSAttack  (PSOCKADDR_IN6 p_sender,XString p_path);
  bool       CheckUnderDDOSAttack(PSOCKADDR_IN6 p_sender,XString p_path);

protected:
  // Cleanup the server
  virtual void  Cleanup() = 0;
  // Init the stream response
  virtual bool  InitEventStream(EventStream& p_stream) = 0;
  // Initialise general server header settings
  virtual void  InitHeaders() = 0;
  // Initialise the hard server limits in bytes
  virtual void  InitHardLimits();
  // Initialise the even stream parameters
  virtual void  InitEventstreamKeepalive();
  // Initialise the logging and error mechanism
  virtual void  InitLogging();
  // Initialise the ThreadPool
  virtual void  InitThreadPool();

  // Register a URL to listen on
  bool      RegisterSite(const HTTPSite* p_site,const XString& p_urlPrefix);
  // General checks before starting
  bool      GeneralChecks();
  // Checks if all sites are started
  void      CheckSitesStarted();
  // Make a "port:url" registration name
  XString   MakeSiteRegistrationName(int p_port,XString p_url);
    // Form event to a stream string
  XString   EventToString(ServerEvent* p_event);
  // Try to start the even heartbeat monitor
  void      TryStartEventHeartbeat();
  // Check all event streams for the heartbeat monitor
  UINT      CheckEventStreams();
  // Set the error status
  void      SetError(int p_error);
  // For the handling of the event streams: implement this function
  virtual bool SendResponseEventBuffer(HTTP_OPAQUE_ID     p_response
                                      ,CRITICAL_SECTION*  p_lock
                                      ,const char*        p_buffer
                                      ,size_t             p_totalLength
                                      ,bool               p_continue = true) = 0;
  // Logging and tracing: The response
  void      TraceResponse(PHTTP_RESPONSE p_response);
  void      TraceKnownResponseHeader(unsigned p_number,const char* p_value);
  // Logging and tracing: The request
  void      TraceRequest(PHTTP_REQUEST p_request);
  void      TraceKnownRequestHeader(unsigned p_number,const char* p_value);

  // Protected data
  XString                 m_name;                   // How the outside world refers to me
  XString                 m_hostName;               // How i am registered with the DNS
  bool                    m_initialized { false };  // Server initialized or not
  bool                    m_running     { false };  // In our main loop
  XString                 m_webroot;                // Webroot of the server
  // HTTP-API V 2.0 session 
  HTTP_SERVER_SESSION_ID  m_session     { NULL  };  // Server session within Windows OS
  HANDLE                  m_requestQueue{ NULL  };  // Request queue within an URL group
  ULONG                   m_queueLength { INIT_HTTP_BACKLOGQUEUE };     // HTTP backlog queue length
  // Bookkeeping
  XString                 m_serverErrorPage;        // Current server error
  XString                 m_clientErrorPage;        // Current client error
  HTTP_CACHE_POLICY_TYPE  m_policy   { HttpCachePolicyNocache };        // Cache policy
  ULONG                   m_secondsToLive  { 0 };   // Seconds to live in the cache
  ThreadPool              m_pool;                   // Our threadpool for the server
  MarlinConfig*              m_marlinConfig;           // Web.config or Marlin.Config in our current directory
  LogAnalysis*            m_log      { nullptr };   // Logging object
  bool                    m_logOwner { false   };   // Server owns the log
  int                     m_logLevel { HLL_NOLOG }; // Detailed logging of the server
  HPFCounter              m_counter;                // High performance counter
  SendHeader              m_sendHeader { SendHeader::HTTP_SH_HIDESERVER }; // Server header to send
  XString                 m_configServerName;       // Server header name from Marlin.config
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
  CRITICAL_SECTION        m_socketLock;             // Lock to register, find, remove WebSockets
  // Registered DDOS Attacks
  DDOSMap                 m_attacks;                // Registration of DDOS attacks
};

inline XString
HTTPServer::GetName()
{
  return m_name;
}

inline XString
HTTPServer::GetConfiguredName()
{
  return m_configServerName;
}

inline XString
HTTPServer::GetWebroot()
{
  return m_webroot;
}

inline XString
HTTPServer::GetHostname()
{
  return m_hostName;
}

inline void
HTTPServer::SetClientErrorPage(const XString& p_errorPage)
{
  m_clientErrorPage = p_errorPage;
}

inline void
HTTPServer::SetServerErrorPage(const XString& p_errorPage)
{
  m_serverErrorPage = p_errorPage;
}

inline void
HTTPServer::SetName(XString p_name)
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

inline MarlinConfig&
HTTPServer::GetWebConfig()
{
  return *m_marlinConfig;
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

inline int
HTTPServer::GetLogLevel()
{
  return m_logLevel;
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

inline bool
HTTPServer::GetEventBOM()
{
  return m_eventBOM;
}

inline CRITICAL_SECTION*
HTTPServer::GetEventLock()
{
  return &m_eventLock;
}
