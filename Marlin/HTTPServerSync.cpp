/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: HTTPServerSync.cpp
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
#include "HTTPServerSync.h"
#include "HTTPSiteMarlin.h"
#include "AutoCritical.h"
#include "WebServiceServer.h"
#include "HTTPURLGroup.h"
#include "HTTPError.h"
#include "GetLastErrorAsString.h"
#include "ConvertWideString.h"

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

HTTPServerSync::HTTPServerSync(CString p_name)
               :HTTPServerMarlin(p_name)
{
}

HTTPServerSync::~HTTPServerSync()
{
  // Cleanup the server objects
  Cleanup();
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
HTTPServerSync::Cleanup()
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
  EventStream* stream = reinterpret_cast<EventStream*>(p_argument);
  if (stream)
  {
    HTTPSite* site = stream->m_site;
    if (site)
    {
      site->HandleEventStream(stream);
    }
  }
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
    if((m_serverThread = (HANDLE)_beginthreadex(NULL,0,StartingTheServer,(void *)(this),0,&threadID)) == INVALID_HANDLE_VALUE)
    {
      m_serverThread = NULL;
      tls_lastError = GetLastError();
      ERRORLOG(tls_lastError,"Cannot create a thread for the MarlinServer.");
    }
  }
}

// Running the main loop of the WebServer
void
HTTPServerSync::RunHTTPServer()
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
    CString   referrer       = request->Headers.KnownHeaders[HttpHeaderReferer        ].pRawValue;
    CString   rawUrl         = CW2A(request->CookedUrl.pFullUrl);
    PSOCKADDR sender         = request->Address.pRemoteAddress;
    int       remDesktop     = FindRemoteDesktop(request->Headers.UnknownHeaderCount
                                                ,request->Headers.pUnknownHeaders);

    // Log earliest as possible
    DETAILLOGV("Received HTTP call from [%s] with length: %I64u"
              ,SocketToServer((PSOCKADDR_IN6)sender).GetString()
              ,request->BytesReceived);

    // Test if server already stopped, and we are here because of the stopping
    if(m_running == false)
    {
      DETAILLOG1("HTTPServer stopped in mainloop.");
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
              HTTPMessage msg(HTTPCommand::http_response,HTTP_STATUS_DENIED);
              msg.SetRequestHandle(request->RequestId);
              msg.SetHTTPSite(site);
              RespondWithClientError(&msg,HTTP_STATUS_DENIED,"Not authenticated");
              // Go to next request
              HTTP_SET_NULL_ID(&requestId);
            }
            else if(auth->AuthStatus == HttpAuthStatusFailure)
            {
              // Second round. Still not authenticated. Drop the connection, better next time
              DETAILLOGS("Authentication failed for: ",rawUrl);
              DETAILLOGV("Authentication failed because of: %s",AuthenticationStatus(auth->SecStatus).GetString());
              HTTPMessage msg(HTTPCommand::http_response,HTTP_STATUS_DENIED);
              msg.SetRequestHandle(request->RequestId);
              msg.SetHTTPSite(site);
              RespondWithClientError(&msg,HTTP_STATUS_DENIED,"Not authenticated");
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
                                    RespondWithServerError(&msg,HTTP_STATUS_NOT_SUPPORTED,"Not implemented");
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
          EventStream* stream = SubscribeEventStream(site,site->GetSite(),absolutePath,request->RequestId,accessToken);
          if(stream)
          {
            // Remember our URL
            stream->m_baseURL = rawUrl;
            // Check for a correct callback
            callback = callback ? callback : HTTPSiteCallbackEvent;
            m_pool.SubmitWork(callback,(void*)stream);
            HTTP_SET_NULL_ID(&requestId);
            continue;
          }
        }

        // For all types of requests: Create the HTTPMessage
        message = new HTTPMessage(type,site);
        message->SetURL(rawUrl);
        message->SetReferrer(referrer);
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
            DETAILLOGV("Request VERB changed to: %s",message->GetVerb().GetString());
          }
        }

        // Remember the fact that we should read the rest of the message
        message->SetReadBuffer(request->Flags & HTTP_REQUEST_FLAG_MORE_ENTITY_BODY_EXISTS);

        // Hit the thread pool with this message
        callback = callback ? callback : HTTPSiteCallbackMessage;
        m_pool.SubmitWork(callback,(void*)message);

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

//////////////////////////////////////////////////////////////////////////
//
// STOPPING THE SERVER
//
//////////////////////////////////////////////////////////////////////////

void
HTTPServerSync::StopServer()
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

void
HTTPServerSync::InitializeHttpResponse(HTTP_RESPONSE* p_response,USHORT p_status,PSTR p_reason)
{
  RtlZeroMemory(p_response,sizeof(HTTP_RESPONSE));
  p_response->StatusCode = p_status;
  p_response->pReason = p_reason;
  p_response->ReasonLength = (USHORT)strlen(p_reason);
}

// Used for canceling a WebSocket for an event stream
void 
HTTPServerSync::CancelRequestStream(HTTP_REQUEST_ID p_response)
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
bool
HTTPServerSync::ReceiveIncomingRequest(HTTPMessage* p_message)
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

  // In case of a POST, try to convert character set before submitting to site
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
HTTPServerSync::AddKnownHeader(HTTP_RESPONSE& p_response,HTTP_HEADER_ID p_header,const char* p_value)
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
  PHTTP_UNKNOWN_HEADER header = new HTTP_UNKNOWN_HEADER[p_headers.size()];

  unsigned ind = 0;
  UKHeaders::iterator it = p_headers.begin();
  while(it != p_headers.end())
  {
    CString name  = it->first;
    CString value = it->second;

    header[ind].NameLength      = (USHORT)name.GetLength();
    header[ind].RawValueLength  = (USHORT)value.GetLength();
    header[ind].pName           = name.GetString();
    header[ind].pRawValue       = value.GetString();

    // next header
    ++ind;
    ++it;
  }
  return header;
}

// Sending response for an incoming message
void       
HTTPServerSync::SendResponse(HTTPMessage* p_message)
{
  HTTP_RESPONSE   response;
  CString         challenge;
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

  // Initialize the HTTP response structure.
  InitializeHttpResponse(&response,(USHORT)status,(PSTR) GetHTTPStatusText(status));

  // Add a known header. (octet-stream or the message content type)
  if(!p_message->GetContentType().IsEmpty())
  {
    contentType = p_message->GetContentType();
  }
  AddKnownHeader(response,HttpHeaderContentType,contentType);

  // In case of a HTTP 401
  if(status == HTTP_STATUS_DENIED)
  {
    // Add authentication scheme
    HTTPSite* site = p_message->GetHTTPSite();
    challenge = BuildAuthenticationChallenge(site->GetAuthenticationScheme()
                                            ,site->GetAuthenticationRealm());
    AddKnownHeader(response,HttpHeaderWwwAuthenticate,challenge);
  }

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
    m_log->AnalysisLog(__FUNCTION__, LogType::LOG_ERROR,true,"HTTP Answer [%d:%s]",GetLastError(),message.GetString());
    // Reset the last error
    SetError(NO_ERROR);
  }
  // Remove unknown header information
  delete [] unknown;

  // Do **NOT** send an answer twice
  p_message->SetRequestHandle(NULL);
}

bool      
HTTPServerSync::SendResponseBuffer(PHTTP_RESPONSE   p_response
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
HTTPServerSync::SendResponseBufferParts(PHTTP_RESPONSE  p_response
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
HTTPServerSync::SendResponseFileHandle(PHTTP_RESPONSE   p_response
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
HTTPServerSync::SendResponseError(PHTTP_RESPONSE  p_response
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
    DETAILLOGV("SendHttpResponse (serverpage) Bytes sent: %d",bytesSent);
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

// Sending a chunk to an event stream
bool
HTTPServerSync::SendResponseEventBuffer(HTTP_REQUEST_ID  p_requestID
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

