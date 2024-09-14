/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: HTTPServerSync.cpp
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
#include "HTTPServerSync.h"
#include "HTTPSiteMarlin.h"
#include "AutoCritical.h"
#include "WebServiceServer.h"
#include "HTTPURLGroup.h"
#include "HTTPError.h"
#include "HTTPTime.h"
#include "GetLastErrorAsString.h"
#include "ConvertWideString.h"
#include "WebSocketServerSync.h"
#include <ServiceReporting.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// Logging macro's
#define DETAILLOG1(text)          if(MUSTLOG(HLL_LOGGING) && m_log) { DetailLog (_T(__FUNCTION__),LogType::LOG_INFO,text); }
#define DETAILLOGS(text,extra)    if(MUSTLOG(HLL_LOGGING) && m_log) { DetailLogS(_T(__FUNCTION__),LogType::LOG_INFO,text,extra); }
#define DETAILLOGV(text,...)      if(MUSTLOG(HLL_LOGGING) && m_log) { DetailLogV(_T(__FUNCTION__),LogType::LOG_INFO,text,__VA_ARGS__); }
#define WARNINGLOG(text,...)      if(MUSTLOG(HLL_LOGGING) && m_log) { DetailLogV(_T(__FUNCTION__),LogType::LOG_WARN,text,__VA_ARGS__); }
#define ERRORLOG(code,text)       ErrorLog (_T(__FUNCTION__),code,text)
#define HTTPERROR(code,text)      HTTPError(_T(__FUNCTION__),code,text)

HTTPServerSync::HTTPServerSync(XString p_name)
               :HTTPServerMarlin(p_name)
{
}

HTTPServerSync::~HTTPServerSync()
{
  // Cleanup the server objects
  HTTPServerSync::Cleanup();
}

// Initialise a HTTP server and server-session
bool
HTTPServerSync::Initialise()
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
    ERRORLOG(retCode,_T("HTTP Initialize"));
    return false;
  }
  DETAILLOG1(_T("HTTPInitialize OK"));

  // STEP 4: CREATE SERVER SESSION
  retCode = HttpCreateServerSession(HTTPAPI_VERSION_2,&m_session,0);
  if(retCode != NO_ERROR)
  {
    ERRORLOG(retCode,_T("CreateServerSession"));
    return false;
  }
  DETAILLOGV(_T("Serversession created: %I64X"),m_session);

  // STEP 5: Create a request queue with NO name
  // Although we CAN create a name, it would mean a global object (and need to be unique)
  // So it's harder to create more than one server per machine
  retCode = HttpCreateRequestQueue(HTTPAPI_VERSION_2,NULL,NULL,0,&m_requestQueue);
  if(retCode != NO_ERROR)
  {
    ERRORLOG(retCode,_T("CreateRequestQueue"));
    return false;
  }
  DETAILLOGV(_T("Request queue created: %p"),m_requestQueue);

  // STEP 7: SET THE LENGTH OF THE BACKLOG QUEUE FOR INCOMING TRAFFIC
  // Overrides for the HTTP Site. Test min/max via SetQueueLength
  int queueLength = m_marlinConfig->GetParameterInteger(_T("Server"),_T("QueueLength"),m_queueLength);
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
    ERRORLOG(retCode,_T("HTTP backlog queue length NOT set!"));
  }
  else
  {
    DETAILLOGV(_T("HTTP backlog queue set to length: %lu"),m_queueLength);
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
    ERRORLOG(retCode,_T("Setting 503 verbosity property"));
  }
  else
  {
    DETAILLOGV(_T("HTTP 503-Error verbosity set to: %d"),verbosity);
  }

  // STEP 9: Set the hard limits
  InitHardLimits();

  // STEP 10: Set the event stream parameters
  InitEventstreamKeepalive();

  // STEP 11: Init the response headers to send
  InitHeaders();

  // STEP 12: Init the ThreadPool
  InitThreadPool();

  // We are airborne!
  return (m_initialized = true);
}

// Cleanup the server
void
HTTPServerSync::Cleanup()
{
  ULONG retCode;
  AutoCritSec lock1(&m_sitesLock);
  AutoCritSec lock2(&m_eventLock);

  // Remove all event streams within the scope of the eventLock
  for(const auto& it : m_eventStreams)
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
  for(auto& group : m_urlGroups)
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
      DETAILLOG1(_T("Closed the request queue"));
    }
    else
    {
      ERRORLOG(retCode,_T("Cannot close the request queue"));
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
      DETAILLOG1(_T("Closed the HTTP server session"));
    }
    else
    {
      ERRORLOG(retCode,_T("Cannot close the HTTP server session"));
    }
  }

  if(m_initialized)
  {
    // Call HttpTerminate.
    retCode = HttpTerminate(HTTP_INITIALIZE_SERVER,NULL);

    if(retCode == NO_ERROR)
    {
      DETAILLOG1(_T("HTTP Server terminated OK"));
    }
    else
    {
      ERRORLOG(retCode,_T("HTTP Server terminated with error"));
    }
    m_initialized = false;
  }

  // Closing the logging file
  if(m_log && m_logOwner)
  {
    LogAnalysis::DeleteLogfile(m_log);
    m_log = NULL;
  }
}

//////////////////////////////////////////////////////////////////////////
//
// THESE ARE THE DEFAULT CALLBACK HANDLERS FOR ASYNC EXECUTION
// CALLBACK ADDRESS TO BE ADDED TO THE THREADPOOL
//
//////////////////////////////////////////////////////////////////////////

void HTTPSiteCallbackMessage(void* p_argument)
{
  HTTPMessage* msg = reinterpret_cast<HTTPMessage*>(p_argument);
  if (msg)
  {
    HTTPSite* site = msg->GetHTTPSite();
    if (site)
    {
      site->HandleHTTPMessage(msg);
    }
  }
}
void HTTPSiteCallbackEvent(void* p_argument)
{
  MsgStream* dispatch = reinterpret_cast<MsgStream*>(p_argument);
  EventStream* stream = dispatch->m_stream;
  HTTPMessage* message = dispatch->m_message;
  if(stream)
  {
    HTTPSite* site = stream->m_site;
    if(site)
    {
      if(!site->HandleEventStream(message,stream))
      {
        site->GetHTTPServer()->RemoveEventStream(stream);
        delete stream;
      }
    }
  }
  delete message;
  delete dispatch;
}

static unsigned int
__stdcall StartingTheServer(void* pParam)
{
  HTTPServerSync* server = reinterpret_cast<HTTPServerSync*>(pParam);
  server->RunHTTPServer();
  return 0;
}

// Running the server in a separate thread
void
HTTPServerSync::Run()
{
  if(m_serverThread == NULL)
  {
    // Base thread of the server
    unsigned int threadID;
    if((m_serverThread = reinterpret_cast<HANDLE>(_beginthreadex(NULL,0,StartingTheServer,reinterpret_cast<void *>(this),0,&threadID))) == INVALID_HANDLE_VALUE)
    {
      m_serverThread = NULL;
      ERRORLOG(::GetLastError(),_T("Cannot create a thread for the MarlinServer."));
    }
  }
}

// Running the main loop of the WebServer
void
HTTPServerSync::RunHTTPServer()
{
  // Install SEH to regular exception translator
  _set_se_translator(SeTranslator);

  DWORD          bytesRead = 0;
  HTTP_OPAQUE_ID requestId = NULL;
  PHTTP_REQUEST  request   = nullptr;
  PCHAR          requestBuffer = nullptr;
  ULONG          requestBufferLength = 0;
  HTTPMessage*   message = NULL;

  // Use counter
  m_counter.Start();

  Initialise();

  // See if we are in a state to receive requests
  if(!m_initialized)
  {
    ERRORLOG(ERROR_INVALID_PARAMETER,_T("RunHTTPServer called too early"));
    return;
  }
  DETAILLOG1(_T("HTTPServer initialised and ready to go!"));

  // Check if all sites were properly started
  // Catches programmers who forget to call HTTPSite::StartSite()
  CheckSitesStarted();

  // Allocate a 32 KB buffer. This size should work for most 
  // requests. The buffer size can be increased if required. Space
  // is also required for an HTTP_REQUEST structure.
  requestBufferLength = sizeof(HTTP_REQUEST) + INIT_HTTP_BUFFERSIZE;
  requestBuffer       = reinterpret_cast<PCHAR>(new uchar[requestBufferLength]);

  if(requestBuffer == NULL)
  {
    ERRORLOG(ERROR_NOT_ENOUGH_MEMORY,_T("Out of memory"));
    return;
  }
  DETAILLOGV(_T("Request buffer allocated [%d] bytes"),requestBufferLength);

  // Buffer where we receive our requests
  request = (PHTTP_REQUEST)requestBuffer;

  // Wait for a new request. This is indicated by a NULL request ID.
  HTTP_SET_NULL_ID(&requestId);

  // START OUR MAIN LOOP
  m_running = true;
  DETAILLOG1(_T("HTTPServer entering main loop"));

  while(m_running)
  {
    // New receive in the buffer
    RtlZeroMemory(request,requestBufferLength);

    // Use counter
    m_counter.Stop();
    // Sit tight for the next request
    ULONG result = HttpReceiveHttpRequest(m_requestQueue,     // Request Queue
                                          requestId,          // Request ID
                                          0,                  // Flags
                                          request,            // HTTP request buffer
                                          requestBufferLength,// request buffer length
                                          &bytesRead,         // bytes received
                                          NULL);              // LPOVERLAPPED
    
    // See if server was aborted by the closing of the request queue
    if(result == ERROR_OPERATION_ABORTED)
    {
      DETAILLOG1(_T("Operation on the HTTP server aborted!"));
      m_running = false;
      break;
    }

    // Use counter
    m_counter.Start();

    // Grab the senders content
    XString   acceptTypes    = LPCSTRToString(request->Headers.KnownHeaders[HttpHeaderAccept         ].pRawValue);
    XString   contentType    = LPCSTRToString(request->Headers.KnownHeaders[HttpHeaderContentType    ].pRawValue);
    XString   acceptEncoding = LPCSTRToString(request->Headers.KnownHeaders[HttpHeaderAcceptEncoding ].pRawValue);
    XString   cookie         = LPCSTRToString(request->Headers.KnownHeaders[HttpHeaderCookie         ].pRawValue);
    XString   authorize      = LPCSTRToString(request->Headers.KnownHeaders[HttpHeaderAuthorization  ].pRawValue);
    XString   modified       = LPCSTRToString(request->Headers.KnownHeaders[HttpHeaderIfModifiedSince].pRawValue);
    XString   referrer       = LPCSTRToString(request->Headers.KnownHeaders[HttpHeaderReferer        ].pRawValue);
    XString   contentLength  = LPCSTRToString(request->Headers.KnownHeaders[HttpHeaderContentLength  ].pRawValue);
    XString   rawUrl         = WStringToString(request->CookedUrl.pFullUrl);
    PSOCKADDR sender         = request->Address.pRemoteAddress;
    int       remDesktop     = FindRemoteDesktop(request->Headers.UnknownHeaderCount
                                                ,request->Headers.pUnknownHeaders);
    // Our charset
    XString charset;
    Encoding encoding = Encoding::EN_ACP;

    // If positive request ID received
    if(request->RequestId)
    {
      // Log earliest as possible
      DETAILLOGV(_T("Received HTTP %s call from [%s] with length: %I64u for: %s")
                 ,GetHTTPVerb(request->Verb,request->pUnknownVerb).GetString()
                 ,SocketToServer((PSOCKADDR_IN6) sender).GetString()
                 ,request->BytesReceived
                 ,rawUrl.GetString());

      // Find our charset
      charset = FindCharsetInContentType(contentType);
      if(!charset.IsEmpty())
      {
        encoding = (Encoding)CharsetToCodepage(charset);
      }

      // Trace the request in full
      LogTraceRequest(request,nullptr,encoding);
    }

    // Test if server already stopped, and we are here because of the stopping
    if(m_running == false)
    {
      DETAILLOG1(_T("HTTPServer stopped in mainloop."));
      break;
    }

    if(result == NO_ERROR)
    {
      HANDLE accessToken = NULL;

      // FInding the site
      bool eventStream = false;
      LPFN_CALLBACK callback = nullptr;
      HTTPSite* site = reinterpret_cast<HTTPSite*>(request->UrlContext);
      if(site)
      {
        callback    = site->GetCallback();
        eventStream = site->GetIsEventStream();
      }
      else
      {
        SvcReportErrorEvent(0,true,_T(__FUNCTION__),_T("FATAL: Site not found: ") + rawUrl);
      }
      // See if we must substitute for a sub-site
      if(site && m_hasSubsites)
      {
        XString absPath = WStringToString(request->CookedUrl.pAbsPath);
        site = FindHTTPSite(site,absPath);
      }

      // Check our authentication
      if(site && site->GetAuthentication())
      {
        if(!CheckAuthentication(request,request->RequestId,site,rawUrl,authorize,accessToken))
        {
          // Ready with this request
          HTTP_SET_NULL_ID(&requestId);
          continue;
        }
      }

      // Remember the context: easy in API 2.0
      if(callback == nullptr && site == nullptr)
      {
        HTTPMessage msg(HTTPCommand::http_response,HTTP_STATUS_NOT_FOUND);
        msg.SetRequestHandle(request->RequestId);
        msg.SetHTTPSite(site);
        SendResponse(&msg);
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
                                  HTTPMessage msg(HTTPCommand::http_response,HTTP_STATUS_NOT_SUPPORTED);
                                  msg.SetRequestHandle(request->RequestId);
                                  msg.SetHTTPSite(site);
                                  RespondWithServerError(&msg,HTTP_STATUS_NOT_SUPPORTED,_T("Not implemented"));
                                  // Ready with this request
                                  HTTP_SET_NULL_ID(&requestId);
                                  continue;
                                }
                                break;
      }

      // Receiving the initiation of an event stream for the server
      acceptTypes.Trim();
      EventStream* stream = nullptr;
      if((type == HTTPCommand::http_get) && (eventStream || acceptTypes.Left(17).CompareNoCase(_T("text/event-stream")) == 0))
      {
        XString absolutePath = WStringToString(request->CookedUrl.pAbsPath);
        if(CheckUnderDDOSAttack((PSOCKADDR_IN6)sender,absolutePath))
        {
          continue;
        }
        stream = SubscribeEventStream((PSOCKADDR_IN6) sender
                                      ,remDesktop
                                      ,site
                                      ,site->GetSite()
                                      ,absolutePath
                                      ,request->RequestId
                                      ,accessToken);
      }

      // For all types of requests: Create the HTTPMessage
      message = new HTTPMessage(type,site);
      message->SetURL(rawUrl);
      message->SetReferrer(referrer);
      message->SetAuthorization(authorize);
      message->SetRequestHandle(request->RequestId);
      message->SetConnectionID(request->ConnectionId);
      message->SetAccessToken(accessToken);
      message->SetRemoteDesktop(remDesktop);
      message->SetSender((PSOCKADDR_IN6)sender);
      message->SetCookiePairs(cookie);
      message->SetAcceptEncoding(acceptEncoding);
      message->SetContentType(contentType);
      message->SetContentLength((size_t)_ttoll(contentLength));
      message->SetAllHeaders(&request->Headers);
      message->SetUnknownHeaders(&request->Headers);
      message->SetEncoding(encoding);

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

      // Find routing information within the site
      CalculateRouting(site,message);

      // Find X-HTTP-Method VERB Tunneling
      if(type == HTTPCommand::http_post && site->GetVerbTunneling())
      {
        if(message->FindVerbTunneling())
        {
          DETAILLOGV(_T("Request VERB changed to: %s"),message->GetVerb().GetString());
        }
      }

      if(stream)
      {
        // Remember our URL
        stream->m_baseURL = rawUrl;
        // Create callback structure
        MsgStream* dispatch = new MsgStream();
        dispatch->m_message = message;
        dispatch->m_stream  = stream;
        // Check for a correct callback
        callback = callback ? callback : HTTPSiteCallbackEvent;
        m_pool.SubmitWork(callback,reinterpret_cast<void*>(dispatch));
        HTTP_SET_NULL_ID(&requestId);
        continue;
      }


      // Remember the fact that we should read the rest of the message
      message->SetReadBuffer(request->Flags & HTTP_REQUEST_FLAG_MORE_ENTITY_BODY_EXISTS);

      // Hit the thread pool with this message
      callback = callback ? callback : HTTPSiteCallbackMessage;
      m_pool.SubmitWork(callback,reinterpret_cast<void*>(message));

      // Ready with this request
      HTTP_SET_NULL_ID(&requestId);
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
      requestBuffer = reinterpret_cast<PCHAR>(new uchar[requestBufferLength]);
      if(requestBuffer == NULL)
      {
        ERRORLOG(ERROR_NOT_ENOUGH_MEMORY,_T("Out of memory"));
        break;
      }
      request = (PHTTP_REQUEST)requestBuffer;

      DETAILLOGV(_T("Request buffer length expanded to [%d] bytes"),requestBufferLength);
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
      // Not called by StopServer
      ERRORLOG(result,_T("ReceiveHttpRequest aborted"));
    }
    else // ERROR_NO_ACCESS / ERROR_HANDLE_EOF / ERROR_INVALID_PARAMETER
    {
      // Any other error
      ERRORLOG(result,_T("ReceiveHttpRequest"));
    }

    // Do error handler at the end of the main loop
    if(::GetLastError())
    {
      m_log->AnalysisLog(_T(__FUNCTION__),LogType::LOG_ERROR,true,_T("HTTP Mainloop: %d"),::GetLastError());
    }
  }
  // Free the request buffer
  if(requestBuffer)
  {
    delete [] requestBuffer;
    DETAILLOG1(_T("Dropped request buffer"));
  }

  // Use the counter
  m_counter.Stop();

  // Last action in the server thread
  m_serverThread = 0;

	// Cleaning up the thread
  _endthreadex(0);
}

//////////////////////////////////////////////////////////////////////////
//
// STOPPING THE SERVER
//
//////////////////////////////////////////////////////////////////////////

void
HTTPServerSync::StopServer()
{
  AutoCritSec lock(&m_eventLock);
  DETAILLOG1(_T("Received a StopServer request"));

  // Make local copy of the server thread handle
  HANDLE close = m_serverThread;

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
    ServerEvent* event = new ServerEvent(_T("close"));
    SendEvent(it.second->m_port,it.second->m_baseURL,event);
  }
  // Try to remove all event streams
  while(!m_eventStreams.empty())
  {
    const EventStream* stream = m_eventStreams.begin()->second;
    CloseEventStream(stream);
  }

  // See if we have a running server-push-event heartbeat monitor
  if(m_eventEvent)
  {
    // Explicitly pulse the event heartbeat monitor
    // this abandons the monitor in one go!
    DETAILLOG1(_T("Abandon the server-push-events heartbeat monitor"));
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
    DETAILLOG1(_T("HTTP Request queue shutdown issued"));
  }
  else
  {
    ERRORLOG(result,_T("Cannot shutdown the HTTP request queue"));
  }
  m_requestQueue = NULL;

  // Wait for a maximum of 30 seconds for the queue to stop
  // All server action should be ended after that.
  for(int ind = 0; ind < 300; ++ind)
  {
    Sleep(100);
    // Wait till the breaking of the main loop
    if(m_serverThread == nullptr)
    {
      Sleep(100);
      if(close)
      {
        CloseHandle(close);
      }
      break;
    }
  }
  // Cleanup the sites and groups
  Cleanup();
}

// Create a new WebSocket in the subclass of our server
WebSocket*
HTTPServerSync::CreateWebSocket(XString p_uri)
{
  WebSocketServer* socket = new WebSocketServerSync(p_uri);

  // Connect the server logfile, and logging level
  socket->SetLogfile(m_log);
  socket->SetLogLevel(m_logLevel);

  return socket;
}

void
HTTPServerSync::InitializeHttpResponse(HTTP_RESPONSE* p_response,USHORT p_status,LPCSTR p_reason)
{
  RtlZeroMemory(p_response,sizeof(HTTP_RESPONSE));
  p_response->Version.MajorVersion = 1;
  p_response->Version.MinorVersion = 1;
  p_response->StatusCode   = p_status;
  p_response->pReason      = p_reason;
  p_response->ReasonLength = (USHORT)strlen(p_reason);
}

// Used for canceling a WebSocket for an event stream
void 
HTTPServerSync::CancelRequestStream(HTTP_OPAQUE_ID p_response,bool /*p_reset*/)
{
  // Cancel the outstanding request from the request queue
  ULONG result = HttpCancelHttpRequest(m_requestQueue,p_response,NULL);
  if(result == NO_ERROR)
  {
    DETAILLOG1(_T("Event stream connection closed"));
  }
  else
  {
    ERRORLOG(result,_T("Event stream incorrectly canceled"));
  }
}

// Receive incoming HTTP request
bool
HTTPServerSync::ReceiveIncomingRequest(HTTPMessage* p_message,Encoding p_encoding /*=Encoding::EN_ACP*/)
{
  bool   retval    = true;
  bool   reading   = true;
  ULONG  bytesRead = 0;
  size_t totalRead = 0;
  size_t mustRead  = p_message->GetContentLength();
  ULONG  entityBufferLength = INIT_HTTP_BUFFERSIZE;

  // Create a buffer + 1 extra byte for the closing 0
  PUCHAR entityBuffer = new uchar[(size_t)entityBufferLength + 1];
  if(entityBuffer == NULL)
  {
    ERRORLOG(ERROR_NOT_ENOUGH_MEMORY,_T("Out of memory"));
    return false;
  }

  // Check that we read accordingly to Content-Length or to EOF
  if(mustRead == 0L)
  {
#if defined _M_IX86
    mustRead = ULONG_MAX;
#else
    mustRead = ULLONG_MAX;
#endif
  }

  // Reading loop
  while(reading && totalRead < mustRead)
  {
    ULONG result = HttpReceiveRequestEntityBody(m_requestQueue
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
                              DETAILLOGV(_T("ReceiveRequestEntityBody [%d] bytes"),bytesRead);
                              totalRead += bytesRead;
                              break;
      case ERROR_HANDLE_EOF:  // Very last incoming body part
                              if(bytesRead)
                              {
                                entityBuffer[bytesRead] = 0;
                                p_message->AddBody(entityBuffer,bytesRead);
                                DETAILLOGV(_T("ReceiveRequestEntityBody [%d] bytes"),bytesRead);
                                totalRead += bytesRead;
                              }
                              reading = false;
                              break;
      default:                ERRORLOG(result,_T("HTTP Receive-request (entity body)"));
                              reading = false;
                              retval  = false;
                              break;
                              
    }
  }

  // Clean up buffer after reading
  delete [] entityBuffer;
  entityBuffer = nullptr;

  // In case of a POST, try to convert character set before submitting to site
  if(p_message->GetCommand() == HTTPCommand::http_post)
  {
    if(p_message->GetContentType().Find(_T("multipart")) <= 0)
    {
      HandleTextContent(p_message);
    }
  }
  DETAILLOGV(_T("Received %s message from: %s Size: %lu")
            ,headers[static_cast<unsigned>(p_message->GetCommand())]
            ,SocketToServer(p_message->GetSender()).GetString()
            ,p_message->GetBodyLength());

  // This message is read!
  p_message->SetReadBuffer(false);

  // Now also trace the request body of the message
  LogTraceRequestBody(p_message,p_encoding);

  // Receiving succeeded?
  return retval;
}

// Add a well known HTTP header to the response structure
void
HTTPServerSync::AddKnownHeader(HTTP_RESPONSE& p_response,HTTP_HEADER_ID p_header,LPCSTR p_value)
{
  p_response.Headers.KnownHeaders[p_header].pRawValue      = p_value;
  p_response.Headers.KnownHeaders[p_header].RawValueLength = (USHORT)strlen(p_value);
}

PHTTP_UNKNOWN_HEADER
HTTPServerSync::AddUnknownHeaders(UKHeaders& p_headers)
{
  // Something to do?
  if(p_headers.empty())
  {
    return nullptr;
  }
  // Alloc some space
  PHTTP_UNKNOWN_HEADER unknown = new HTTP_UNKNOWN_HEADER[p_headers.size()];

  unsigned ind = 0;
  unsigned len = 0;
  for(auto& header : p_headers)
  {
    AutoCSTR name(header.m_name);
    len = name.size();
    header.m_nameStr = name.grab();
    unknown[ind].NameLength = (USHORT)len;
    unknown[ind].pName      = header.m_nameStr;

    AutoCSTR value(header.m_value);
    len = value.size();
    header.m_valueStr = value.grab();
    unknown[ind].RawValueLength = (USHORT)len;
    unknown[ind].pRawValue      = header.m_valueStr;

    // next header
    ++ind;
  }
  return unknown;
}

// Sending a response as a chunk
void
HTTPServerSync::SendAsChunk(HTTPMessage* p_message,bool p_final /*= false*/)
{
  // Check if multi-part buffer or file
  FileBuffer* buffer = p_message->GetFileBuffer();
  if(!buffer->GetFileName().IsEmpty())
  {
    ERRORLOG(ERROR_INVALID_PARAMETER,_T("Send as chunk cannot send a file!"));
    return;
  }
  // Chunk encode the file buffer
  if(!buffer->ChunkedEncoding(p_final))
  {
    ERRORLOG(ERROR_NOT_ENOUGH_MEMORY,_T("Cannot chunk-encode the message for transfer-encoding!"));
  }

  // If we want to send a (g)zipped buffer, that should have been done already by now
  p_message->SetAcceptEncoding(_T(""));

  // Get the chunk number (first->next)
  unsigned chunk = p_message->GetChunkNumber();
  p_message->SetChunkNumber(++chunk);
  DETAILLOGV(_T("Transfer-encoding [Chunked] Sending chunk [%d]"),chunk);

  if(chunk == 1)
  {
    // Send the first chunk
    SendResponse(p_message);
  }
  else
  {
    // Send all next chunks
    HTTP_RESPONSE   response;
    HTTP_OPAQUE_ID  requestID = p_message->GetRequestHandle();
    int status = p_message->GetStatus();
    AutoCSTR stat(GetHTTPStatusText(status));

    InitializeHttpResponse(&response,static_cast<USHORT>(status),stat.cstr());
    SendResponseChunk(&response,requestID,buffer,p_final);
  }
  if(p_final)
  {
    // Do **NOT** send an answer twice
    p_message->SetHasBeenAnswered();
  }
}

// Sending response for an incoming message
void       
HTTPServerSync::SendResponse(HTTPMessage* p_message)
{
  HTTP_RESPONSE   response;
  HTTP_OPAQUE_ID  requestID   = p_message->GetRequestHandle();
  FileBuffer*     buffer      = p_message->GetFileBuffer();
  XString         contentType(_T("application/octet-stream")); 
  bool            moreData(false);

  // See if there is something to send
  if(requestID == NULL)
  {
    ERRORLOG(ERROR_INVALID_PARAMETER,_T("SendResponse: nothing to send"));
    return;
  }

  // Respond to general HTTP status
  int status = p_message->GetStatus();
  AutoCSTR stattxt(GetHTTPStatusText(status));
  AutoCSTR headerChallenge;

  // Initialize the HTTP response structure.
  InitializeHttpResponse(&response,static_cast<USHORT>(status),stattxt.cstr());

  // In case of a HTTP 401
  if(status == HTTP_STATUS_DENIED)
  {
    if(!p_message->GetXMLHttpRequest())
    {
      // See if the message already has an authentication scheme header
      XString challenge = p_message->GetHeader(_T("AuthenticationScheme"));
      if(challenge.IsEmpty())
      {
        // Add authentication scheme
        HTTPSite* site = p_message->GetHTTPSite();
        challenge = BuildAuthenticationChallenge(site->GetAuthenticationScheme()
                                                 ,site->GetAuthenticationRealm());
        headerChallenge = challenge;
        AddKnownHeader(response,HttpHeaderWwwAuthenticate,headerChallenge.cstr());
      }
    }
  }

  AutoCSTR headerDate(HTTPGetSystemTime());
  AddKnownHeader(response,HttpHeaderDate,headerDate.cstr());

  // Add a known header. (octet-stream or the message content type)
  if(!p_message->GetContentType().IsEmpty())
  {
    contentType = p_message->GetContentType();
  }
  else
  {
    XString cttype = p_message->GetHeader(_T("Content-type"));
    if(!cttype.IsEmpty())
    {
      contentType = cttype;
    }
  }
  p_message->DelHeader(_T("Content-Type"));
  AutoCSTR headerContentType(contentType);
  AddKnownHeader(response,HttpHeaderContentType,headerContentType.cstr());

  // Add the server header or suppress it
  AutoCSTR headerServer;
  switch(m_sendHeader)
  {
    case SendHeader::HTTP_SH_MICROSOFT:   // Do nothing, Microsoft will add the server header
                                          break;
    case SendHeader::HTTP_SH_MARLIN:      AddKnownHeader(response,HttpHeaderServer,MARLIN_SERVER_VERSION);
                                          break;
    case SendHeader::HTTP_SH_APPLICATION: headerServer = m_name;
                                          AddKnownHeader(response,HttpHeaderServer,headerServer.cstr());
                                          break;
    case SendHeader::HTTP_SH_WEBCONFIG:   headerServer = m_configServerName;
                                          AddKnownHeader(response,HttpHeaderServer,headerServer.cstr());
                                          break;
    case SendHeader::HTTP_SH_HIDESERVER:  // Fill header with empty string will suppress it
                                          AddKnownHeader(response,HttpHeaderServer,"");
                                          break;
  }
  p_message->DelHeader(_T("Server"));

  // Cookie settings
  bool cookiesHasSecure(false);
  bool cookiesHasHttp(false);
  bool cookiesHasSame(false);
  bool cookiesHasPath(false);
  bool cookiesHasDomain(false);
  bool cookiesHasExpires(false);
  bool cookiesHasMaxAge(false);

  bool cookieSecure(false);
  bool cookieHttpOnly(false);
  CookieSameSite cookieSameSite(CookieSameSite::NoSameSite);
  XString    cookiePath;
  XString    cookieDomain;
  int        cookieExpires = 0;
  int        cookieMaxAge  = 0;

  // Getting the site settings
  HTTPSite* site = p_message->GetHTTPSite();
  if(site)
  {
    cookiesHasSecure  = site->GetCookieHasSecure();
    cookiesHasHttp    = site->GetCookieHasHttpOnly();
    cookiesHasSame    = site->GetCookieHasSameSite();
    cookiesHasPath    = site->GetCookieHasPath();
    cookiesHasDomain  = site->GetCookieHasDomain();
    cookiesHasExpires = site->GetCookieHasExpires();
    cookiesHasMaxAge  = site->GetCookieHasMaxAge();

    cookieSecure      = site->GetCookiesSecure();
    cookieHttpOnly    = site->GetCookiesHttpOnly();
    cookieSameSite    = site->GetCookiesSameSite();
    cookiePath        = site->GetCookiesPath();
    cookieDomain      = site->GetCookiesDomain();
    cookieExpires     = site->GetCookiesExpires();
    cookieMaxAge      = site->GetCookiesMaxAge();
  }

  // Add cookies to the unknown response headers
  // Because we can have more than one Set-Cookie: header
  // and HTTP API just supports one set-cookie.
  UKHeaders ukheaders;
  Cookies& cookies = p_message->GetCookies();
  AutoCSTR headerCookie;
  if(cookies.GetCookies().empty())
  {
    XString cookie = p_message->GetHeader(_T("Set-Cookie"));
    if(!cookie.IsEmpty())
    {
      headerCookie = cookie;
      AddKnownHeader(response,HttpHeaderSetCookie,headerCookie.cstr());
    }
  }
  else
  {
    for(auto& cookie : cookies.GetCookies())
    {
      if(cookiesHasSecure)  cookie.SetSecure  (cookieSecure);
      if(cookiesHasHttp)    cookie.SetHttpOnly(cookieHttpOnly);
      if(cookiesHasSame)    cookie.SetSameSite(cookieSameSite);
      if(cookiesHasPath)    cookie.SetPath    (cookiePath);
      if(cookiesHasDomain)  cookie.SetDomain  (cookieDomain);
      if(cookiesHasMaxAge)  cookie.SetMaxAge  (cookieMaxAge);

      if(cookiesHasExpires && cookieExpires > 0)
      {
        SYSTEMTIME current;
        GetSystemTime(&current);
        AddSecondsToSystemTime(&current,&current,60 * (double)cookieExpires);
        cookie.SetExpires(&current);
      }

      ukheaders.push_back(UKHeader(_T("Set-Cookie"),cookie.GetSetCookieText()));
    }
  }
  p_message->DelHeader(_T("Set-Cookie"));


  // Add extra headers from the message, except for content-length
  p_message->DelHeader(_T("Content-Length"));

  HeaderMap* map = p_message->GetHeaderMap();
  for(HeaderMap::iterator it = map->begin(); it != map->end(); ++it)
  {
    ukheaders.push_back(UKHeader(it->first,it->second));
  }

  // Add other optional security headers like CORS etc.
  if(site)
  {
    site->AddSiteOptionalHeaders(ukheaders);
  }

  // Possible zip the contents, and add content-encoding header
  if(p_message->GetHTTPSite() && p_message->GetHTTPSite()->GetHTTPCompression() && buffer)
  {
    // But only if the client side requested it
    if(p_message->GetAcceptEncoding().Find(_T("gzip")) >= 0)
    {
      if(buffer->ZipBuffer())
      {
        DETAILLOGV(_T("GZIP the buffer to size: %lu"),buffer->GetLength());
        ukheaders.push_back(UKHeader(_T("Content-Encoding"),_T("gzip")));
      }
    }
  }

  // Now add all unknown headers to the response
  PHTTP_UNKNOWN_HEADER unknown = AddUnknownHeaders(ukheaders);
  response.Headers.UnknownHeaderCount = (USHORT) ukheaders.size();
  response.Headers.pUnknownHeaders    = unknown;

  // Calculate the content length
  XString contentLength;
  AutoCSTR headerContentLength;
  size_t totalLength = buffer ? buffer->GetLength() : 0;

  // Sync server providing our own error page
  if(totalLength == 0 && status >= HTTP_STATUS_BAD_REQUEST)
  {
    CString page;
    CString reason = GetHTTPStatusText(status);

    if(status >= HTTP_STATUS_SERVER_ERROR)
    {
      page.Format(m_serverErrorPage,status,reason.GetString());
      AutoCSTR cpage(page);
      buffer->SetBuffer((uchar*)cpage.cstr(),cpage.size());
    }
    else
    {
      page.Format(m_clientErrorPage,status,reason.GetString());
      AutoCSTR cpage(page);
      buffer->SetBuffer((uchar*)cpage.cstr(), cpage.size());
    }
    totalLength = buffer->GetLength();
  }

  if(p_message->GetChunkNumber())
  {
    moreData = true;
    AddKnownHeader(response,HttpHeaderTransferEncoding,"chunked");
  }
  else
  {
    // Now after the compression, add the total content length
  #ifdef _WIN64
    contentLength.Format(_T("%I64u"),totalLength);
  #else
    contentLength.Format(_T("%lu"),totalLength);
  #endif
    headerContentLength = contentLength;
    AddKnownHeader(response,HttpHeaderContentLength,headerContentLength.cstr());
  }

  // Dependent on the filling of FileBuffer
  // Send 1 or more buffers or the file
  if(buffer && buffer->GetHasBufferParts())
  {
    SendResponseBufferParts(&response,requestID,buffer,totalLength);
  }
  else if(buffer && buffer->GetFileName().IsEmpty())
  {
    SendResponseBuffer(&response,requestID,buffer,totalLength,moreData);
  }
  else
  {
    SendResponseFileHandle(&response,requestID,buffer);
  }
  if(GetLastError())
  {
    // Error handler
    XString message = GetLastErrorAsString();
    m_log->AnalysisLog(_T(__FUNCTION__), LogType::LOG_ERROR,true,_T("HTTP Answer [%d:%s]"),::GetLastError(),message.GetString());
  }

  // Possibly log and trace what we just sent
  LogTraceResponse(&response,p_message,p_message->GetEncoding());

  // Remove unknown header information
  delete [] unknown;

  if(!p_message->GetChunkNumber())
  {
	  // Do **NOT** send an answer twice
	  p_message->SetHasBeenAnswered();
  }
}

bool      
HTTPServerSync::SendResponseBuffer(PHTTP_RESPONSE p_response
                                  ,HTTP_OPAQUE_ID p_requestID
                                  ,FileBuffer*    p_buffer
                                  ,size_t         p_totalLength
                                  ,bool           p_moreData /*= false*/)
{
  uchar* entity       = NULL;
  DWORD  result       = 0;
  DWORD  bytesSent    = 0;
  size_t entityLength = 0;
  HTTP_DATA_CHUNK dataChunk;
  memset(&dataChunk,0,sizeof(HTTP_DATA_CHUNK));

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

  ULONG flags = p_moreData ? HTTP_SEND_RESPONSE_FLAG_MORE_DATA : HTTP_SEND_RESPONSE_FLAG_DISCONNECT;

  // Because the entity body is sent in one call, it is not
  // required to specify the Content-Length.
  result = HttpSendHttpResponse(m_requestQueue,      // ReqQueueHandle
                                p_requestID,         // Request ID
                                flags,               // Flags
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
    ERRORLOG(result,_T("SendHttpResponseBuffer"));
  }
  else
  {
    DETAILLOGV(_T("SendHttpResponseBuffer [%u] bytes sent"),bytesSent);
  }
  return (result == NO_ERROR);
}

void      
HTTPServerSync::SendResponseBufferParts(PHTTP_RESPONSE  p_response
                                       ,HTTP_OPAQUE_ID  p_request
                                       ,FileBuffer*     p_buffer
                                       ,size_t          p_totalLength)
{
  int    transmitPart = 0;
  uchar* entityBuffer = NULL;
  size_t entityLength = 0;
  size_t totalSent    = 0;
  DWORD  bytesSent    = 0;
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
    ULONG flags = (totalSent + entityLength) < p_totalLength ? HTTP_SEND_RESPONSE_FLAG_MORE_DATA : HTTP_SEND_RESPONSE_FLAG_DISCONNECT;
    DWORD  result = 0;

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
      ERRORLOG(result,_T("HTTP SendResponsePart error"));
      break;
    }
    else
    {
      DETAILLOGV(_T("HTTP SendResponsePart [%d] bytes sent"),entityLength);
    }
    // Next buffer part
    totalSent += entityLength;
    ++transmitPart;
  }
}

void      
HTTPServerSync::SendResponseFileHandle(PHTTP_RESPONSE p_response
                                      ,HTTP_OPAQUE_ID p_request
                                      ,FileBuffer*    p_buffer)
{
  DWORD  bytesSent = 0;
  HANDLE file      = NULL;
  HTTP_DATA_CHUNK dataChunk;

  // File to transmit
  if(p_buffer->OpenFile() == false)
  {
    ERRORLOG(::GetLastError(),_T("OpenFile for SendHttpResponse"));
    return;
  }
  // Get the file handle from buffer
  file = p_buffer->GetFileHandle();

  // See if file is under 4G large
  DWORD high = 0L;
  ULONG fileSize = GetFileSize(file,&high);
  if(high)
  {
    // 413: Request entity too large
    SendResponseError(p_response,p_request,m_clientErrorPage,413,_T("Request entity too large"));
    // Error state
    ERRORLOG(ERROR_INVALID_PARAMETER,_T("File to send is too big (>4G)"));
    // Close the file handle
    p_buffer->CloseFile();
    return;
  }

  // Preparing the cache-policy
  HTTP_CACHE_POLICY policy;
  policy.Policy        = m_policy;
  policy.SecondsToLive = m_secondsToLive;

  DWORD result = HttpSendHttpResponse(m_requestQueue,
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
    ERRORLOG(result,_T("SendHttpResponse"));
    p_buffer->CloseFile();
    return;
  }
  else
  {
    DETAILLOGS(_T("SendHttpResponse file: "),p_buffer->GetFileName());
    DETAILLOGV(_T("SendHttpResponse file header. Filesize: %d"),fileSize);
  }

  // Send entity body from a file handle.
  dataChunk.DataChunkType = HttpDataChunkFromFileHandle;
  dataChunk.FromFileHandle.ByteRange.StartingOffset.QuadPart = 0;
  dataChunk.FromFileHandle.ByteRange.Length.QuadPart = fileSize; // HTTP_BYTE_RANGE_TO_EOF
  dataChunk.FromFileHandle.FileHandle = file;

  ULONG bytes;
  result = HttpSendResponseEntityBody(m_requestQueue,
                                      p_request,
                                      HTTP_SEND_RESPONSE_FLAG_DISCONNECT, // This is the last send.
                                      1,                                  // Entity Chunk Count.
                                      &dataChunk,
                                      &bytes,
                                      NULL,
                                      0,
                                      NULL,
                                      NULL);
  if(result != NO_ERROR)
  {
    ERRORLOG(result,_T("SendResponseEntityBody for file"));
  }
  else
  {
    DETAILLOG1(_T("SendResponseEntityBody for file"));
  }
  // Now close our file handle
  p_buffer->CloseFile();
}

void      
HTTPServerSync::SendResponseChunk(PHTTP_RESPONSE  p_response
                                 ,HTTP_OPAQUE_ID  p_request
                                 ,FileBuffer*     p_buffer
                                 ,bool            p_last)
{
  uchar* entityBuffer = NULL;
  size_t entityLength = 0;
  DWORD  bytesSent    = 0;
  HTTP_DATA_CHUNK dataChunk;
  memset(&dataChunk,0,sizeof(HTTP_DATA_CHUNK));

  p_buffer->GetBuffer(entityBuffer,entityLength);
  if(entityBuffer)
  {
    // Add an entity chunk.
    dataChunk.DataChunkType           = HttpDataChunkFromMemory;
    dataChunk.FromMemory.pBuffer      = entityBuffer;
    dataChunk.FromMemory.BufferLength = (ULONG)entityLength;
    p_response->EntityChunkCount      = 1;
    p_response->pEntityChunks         = &dataChunk;

    // Flag to calculate the last sending part
    ULONG flags = p_last ?  0 : HTTP_SEND_RESPONSE_FLAG_MORE_DATA;

    // Next part to send
    DWORD result = HttpSendResponseEntityBody(m_requestQueue
                                             ,p_request
                                             ,flags
                                             ,1
                                             ,&dataChunk
                                             ,&bytesSent
                                             ,NULL
                                             ,NULL
                                             ,NULL
                                             ,NULL);
    if(result != NO_ERROR)
    {
      ERRORLOG(result,_T("HTTP SendResponsePart error"));
    }
    else
    {
      DETAILLOGV(_T("HTTP SendResponseChunk [%d] bytes sent"),entityLength);
    }
  }
}

void      
HTTPServerSync::SendResponseError(PHTTP_RESPONSE p_response
                                 ,HTTP_OPAQUE_ID p_request
                                 ,XString&       p_page
                                 ,int            p_error
                                 ,LPCTSTR        p_reason)
{
  DWORD result = 0;
  DWORD bytesSent = 0;
  HTTP_DATA_CHUNK dataChunk;
  XString sending;

  // Format our error page
  sending.Format(p_page,p_error,p_reason);

  // Add an entity chunk.
  dataChunk.DataChunkType           = HttpDataChunkFromMemory;
  dataChunk.FromMemory.pBuffer      = reinterpret_cast<void*>(const_cast<TCHAR*>(sending.GetString()));
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
                                HTTP_SEND_RESPONSE_FLAG_DISCONNECT, // Flags
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
    ERRORLOG(result,_T("SendHttpResponse"));
  }
  else
  {
    DETAILLOGV(_T("SendHttpResponse (serverpage) Bytes sent: %d"),bytesSent);
    // Possibly log & trace what we just sent
    LogTraceResponse(p_response,reinterpret_cast<uchar*>(const_cast<TCHAR*>(sending.GetString())),sending.GetLength());
  }
}

//////////////////////////////////////////////////////////////////////////
//
// EVENT STREAMS
//
//////////////////////////////////////////////////////////////////////////

// Init the stream response
bool
HTTPServerSync::InitEventStream(EventStream& p_stream)
{
  // First comment to push to the stream (not an event!)
  // Always UTF-8 compatible, so simple ANSI string
  char* init = ":init event-stream\r\n\r\n";
  int length = (int) strlen(init);

  // Initialize the HTTP response structure.
  InitializeHttpResponse(&p_stream.m_response,HTTP_STATUS_OK,"OK");

  // Add a known header.
  AddKnownHeader(p_stream.m_response,HttpHeaderContentType,"text/event-stream");

//   COMMENTED OUT: Leaks memory at logoff
//   // Add all extra headers
//   UKHeaders ukheaders;
//   if(p_stream.m_site)
//   {
//     p_stream.m_site->AddSiteOptionalHeaders(ukheaders);
//   }
//   // Now add all unknown headers to the response
//   PHTTP_UNKNOWN_HEADER unknown = AddUnknownHeaders(ukheaders);
//   p_stream.m_response.Headers.UnknownHeaderCount = (USHORT)ukheaders.size();
//   p_stream.m_response.Headers.pUnknownHeaders    = unknown;

  // Add an entity chunk.
  HTTP_DATA_CHUNK dataChunk;
  dataChunk.DataChunkType           = HttpDataChunkFromMemory;
  dataChunk.FromMemory.pBuffer      = reinterpret_cast<void*>(init);
  dataChunk.FromMemory.BufferLength = static_cast<unsigned>(length);
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
    ERRORLOG(result,_T("SendHttpResponse for init of event-stream"));
  }
  else
  {
    // Log & Trace what we just sent
    LogTraceResponse(&p_stream.m_response,reinterpret_cast<uchar*>(init),length);
  }
  return (result == NO_ERROR);
}

// Sending a chunk to an event stream
bool
HTTPServerSync::SendResponseEventBuffer(HTTP_OPAQUE_ID    p_requestID
                                       ,CRITICAL_SECTION* p_lock
                                       ,BYTE**            p_buffer
                                       ,size_t            p_length
                                       ,bool              p_continue /*=true*/)
{
  AutoCritSec lockme(p_lock);

  DWORD  result    = 0;
  DWORD  bytesSent = 0;
  HTTP_DATA_CHUNK dataChunk;

  // Only if a buffer present
  dataChunk.DataChunkType           = HttpDataChunkFromMemory;
  dataChunk.FromMemory.pBuffer      = reinterpret_cast<void*>(*p_buffer);
  dataChunk.FromMemory.BufferLength = static_cast<unsigned>(p_length);

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
    ERRORLOG(result,_T("HttpSendResponseEntityBody failed for SendEvent"));
  }
  else
  {
    DETAILLOGV(_T("HttpSendResponseEntityBody [%d] bytes sent"),p_length);
    LogTraceResponse(nullptr,reinterpret_cast<uchar*>(*p_buffer),static_cast<unsigned>(p_length));

    // Final closing of the connection
    if(p_continue == false)
    {
      DETAILLOG1(_T("Event stream connection closed"));
    }
  }
  return (result == NO_ERROR);
}

