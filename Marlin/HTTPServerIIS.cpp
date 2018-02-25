/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: HTTPServerIIS.cpp
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
// HTTP Server using IIS Modules
//
//////////////////////////////////////////////////////////////////////////
//
#include "stdafx.h"
#include "HTTPServerIIS.h"
#include "HTTPSiteIIS.h"
#include "AutoCritical.h"
#include "WebServiceServer.h"
#include "HTTPURLGroup.h"
#include "HTTPError.h"
#include "GetLastErrorAsString.h"
#include "MarlinModule.h"
#include "EnsureFile.h"
#include "WebSocket.h"
#include "ConvertWideString.h"
#include <httpserv.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// Logging macro's
#define DETAILLOG1(text)          if(MUSTLOG(HLL_LOGGING) && m_log) { DetailLog (__FUNCTION__,LogType::LOG_INFO,text); }
#define DETAILLOGS(text,extra)    if(MUSTLOG(HLL_LOGGING) && m_log) { DetailLogS(__FUNCTION__,LogType::LOG_INFO,text,extra); }
#define DETAILLOGV(text,...)      if(MUSTLOG(HLL_LOGGING) && m_log) { DetailLogV(__FUNCTION__,LogType::LOG_INFO,text,__VA_ARGS__); }
#define WARNINGLOG(text,...)      if(MUSTLOG(HLL_LOGGING) && m_log) { DetailLogV(__FUNCTION__,LogType::LOG_WARN,text,__VA_ARGS__); }
#define ERRORLOG(code,text)       ErrorLog (__FUNCTION__,code,text)
#define HTTPERROR(code,text)      HTTPError(__FUNCTION__,code,text)

HTTPServerIIS::HTTPServerIIS(CString p_name)
              :HTTPServer(p_name)
{
  m_counter.Start();
}

HTTPServerIIS::~HTTPServerIIS()
{
  // Cleanup the server objects
  Cleanup();
}

CString
HTTPServerIIS::GetVersion()
{
  return CString(MARLIN_SERVER_VERSION " on Microsoft IIS");
}

// Initialise a HTTP server
bool
HTTPServerIIS::Initialise()
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

  // STEP 3: Set the hard limits
  InitHardLimits();

  // STEP 4: Init the response headers to send
  InitHeaders();

  // We are airborne!
  return (m_initialized = true);
}


// Cleanup the server
void
HTTPServerIIS::Cleanup()
{
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

  // Closing the logging file
  if(m_log && m_logOwner)
  {
    delete m_log;
    m_log = NULL;
  }
}
void
HTTPServerIIS::InitLogging()
{
  // Check for a logging object
  if(m_log == NULL)
  {
    // Create a new one
    m_log = new LogAnalysis(m_name);
    m_logOwner = true;
  }
  // If you want to tweak the loglevel
  // you will need a "Logfile.config" in your logging directory
}

// Initialise general server header settings
// Can only be overriden from within your ServerApp.
void
HTTPServerIIS::InitHeaders()
{
  CString headertype;
  switch(m_sendHeader)
  {
    case SendHeader::HTTP_SH_MICROSOFT:   headertype = "Microsoft defined server header"; break;
    case SendHeader::HTTP_SH_MARLIN:      headertype = "Standard Marlin server header";   break;
    case SendHeader::HTTP_SH_APPLICATION: headertype = "Application's server name";       break;
    case SendHeader::HTTP_SH_WEBCONFIG:   headertype = "Configured header type name";     break;
    case SendHeader::HTTP_SH_HIDESERVER:  headertype = "Hide server response header";     break;
  }
  DETAILLOGV("Header type: %s",headertype.GetString());
}

// Initialise the hard server limits in bytes
// You can only change the hard limits from within your ServerApp
void
HTTPServerIIS::InitHardLimits()
{
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
HTTPServerIIS::InitThreadpoolLimits(int& p_minThreads,int& p_maxThreads,int& p_stackSize)
{
  // Does nothing. Cannot get the limits from a web.config!
  // If you want to change the limits, do this from within your ServerApp!
  UNREFERENCED_PARAMETER(p_maxThreads);
  UNREFERENCED_PARAMETER(p_minThreads);
  UNREFERENCED_PARAMETER(p_stackSize);
}

// Initialise the servers webroot
void 
HTTPServerIIS::InitWebroot(CString p_webroot)
{
  // Directly set webroot from IIS
  // If you want to change the webroot, do this from within your ServerApp!

  EnsureFile ensure(p_webroot);
  int er = ensure.CheckCreateDirectory();
  if(er)
  {
    ERRORLOG(er,"Cannot reach server root directory: " + p_webroot);
  }
  m_webroot = p_webroot;
}

// Running the server 
void
HTTPServerIIS::Run()
{
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

  // START OUR MAIN LOOP
  m_running = true;
  DETAILLOG1("HTTPServerIIS started");
}

// Stop the server
void
HTTPServerIIS::StopServer()
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
    if(socket)
    {
      socket->CloseSocket();
      UnRegisterWebSocket(socket);
    }
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
    DETAILLOG1("Abandon the server-push-events heartbeat monitor");
    SetEvent(m_eventEvent);
  }

  // mainloop will stop on next iteration
  m_running = false;

  // Wait for a maximum of 30 seconds for the queue to stop
  // All server action should be ended after that.
  for(int ind = 0; ind < 300; ++ind)
  {
    Sleep(100);
    // Wait till the  event Monitor has stopped
    if(m_eventMonitor == nullptr)
    {
      break;
    }
  }
  // Cleanup the sites and groups
  Cleanup();
}

// Create a site to bind the traffic to
HTTPSite*
HTTPServerIIS::CreateSite(PrefixType    p_type
                         ,bool          p_secure
                         ,int           p_port
                         ,CString       p_baseURL
                         ,bool          p_subsite  /* = false */
                         ,LPFN_CALLBACK p_callback /* = NULL  */)
{
  // USE OVERRIDES FROM WEBCONFIG 
  // BUT USE PROGRAM'S SETTINGS AS DEFAULT VALUES
  
  if(p_type != PrefixType::URLPRE_Strong || p_secure ||
     p_port != INTERNET_DEFAULT_HTTP_PORT)
  {
    DETAILLOG1("In IIS the prefix type, https and port number are controlled by IIS Admin, and cannot be set!");
  }
  // Create our URL prefix
  CString prefix = CreateURLPrefix(p_type,p_secure,p_port,p_baseURL);
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
    HTTPSiteIIS* registeredSite = new HTTPSiteIIS(this,p_port,p_baseURL,prefix,mainSite,p_callback);
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
HTTPServerIIS::DeleteSite(int p_port,CString p_baseURL,bool p_force /*=false*/)
{
  AutoCritSec lock(&m_sitesLock);
  // Default result
  bool result = false;

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
          return false;
        }
      }
    }
    // And remove from the site map
    DETAILLOGS("Removed site: ",site->GetPrefixURL());
    delete site;
    m_allsites.erase(it);
    result = true;
  }
  return result;
}

//////////////////////////////////////////////////////////////////////////
//
// FUNCTIONS FOR IIS
//
//////////////////////////////////////////////////////////////////////////

// Building the essential HTTPMessage from the request area
HTTPMessage* 
HTTPServerIIS::GetHTTPMessageFromRequest(IHttpContext* p_context
                                        ,HTTPSite*     p_site
                                        ,PHTTP_REQUEST p_request
                                        ,EventStream*& p_stream)
{
  USES_CONVERSION;

  // Grab the senders content
  CString   acceptTypes    = p_request->Headers.KnownHeaders[HttpHeaderAccept         ].pRawValue;
  CString   contentType    = p_request->Headers.KnownHeaders[HttpHeaderContentType    ].pRawValue;
  CString   contentLength  = p_request->Headers.KnownHeaders[HttpHeaderContentLength  ].pRawValue;
  CString   acceptEncoding = p_request->Headers.KnownHeaders[HttpHeaderAcceptEncoding ].pRawValue;
  CString   cookie         = p_request->Headers.KnownHeaders[HttpHeaderCookie         ].pRawValue;
  CString   authorize      = p_request->Headers.KnownHeaders[HttpHeaderAuthorization  ].pRawValue;
  CString   modified       = p_request->Headers.KnownHeaders[HttpHeaderIfModifiedSince].pRawValue;
  CString   referrer       = p_request->Headers.KnownHeaders[HttpHeaderReferer        ].pRawValue;
  CString   rawUrl         = CW2A(p_request->CookedUrl.pFullUrl);
  PSOCKADDR sender         = p_request->Address.pRemoteAddress;
  int       remDesktop     = FindRemoteDesktop(p_request->Headers.UnknownHeaderCount
                                              ,p_request->Headers.pUnknownHeaders);

  // Log earliest as possible
  DETAILLOGV("Received HTTP call from [%s] with length: %s"
             ,SocketToServer((PSOCKADDR_IN6)sender).GetString()
             ,contentLength.GetString());

  // Log incoming request
  DETAILLOGS("Got a request for: ",rawUrl);

  // Trace the request in full
  LogTraceRequest(p_request,nullptr);

  // See if we must substitute for a sub-site
  if(m_hasSubsites)
  {
    CString absPath = CW2A(p_request->CookedUrl.pAbsPath);
    p_site = FindHTTPSite(p_site,absPath);
  }

  HANDLE token = NULL;
  if(!CheckAuthentication(p_request,(HTTP_OPAQUE_ID) p_context,p_site,rawUrl,authorize,token))
  {
    // No authentication, answer already sent
    return nullptr;
  }

  // Translate the command. Now reduced to just this switch
  HTTPCommand type = HTTPCommand::http_no_command;
  switch(p_request->Verb)
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
                            type = GetUnknownVerb(p_request->pUnknownVerb);
                            if(type == HTTPCommand::http_no_command)
                            {
                              ERRORLOG(ERROR_INVALID_PARAMETER,"Unknown HTTP Verb");
                              HTTPMessage msg(HTTPCommand::http_response,HTTP_STATUS_NOT_SUPPORTED);
                              msg.SetRequestHandle((HTTP_OPAQUE_ID) p_context);
                              msg.SetHTTPSite(p_site);
                              RespondWithServerError(&msg,HTTP_STATUS_NOT_SUPPORTED,"Not implemented");
                              return nullptr;
                            }
                            break;
  }

  // Receiving the initiation of an event stream for the server
  acceptTypes.Trim();
  if((type == HTTPCommand::http_get) && 
     (p_site->GetIsEventStream() || acceptTypes.Left(17).CompareNoCase("text/event-stream") == 0))
  {
    CString absolutePath = CW2A(p_request->CookedUrl.pAbsPath);
    EventStream* stream = SubscribeEventStream((PSOCKADDR_IN6) sender
                                               ,remDesktop
                                               ,p_site
                                               ,p_site->GetSite()
                                               ,absolutePath
                                               ,(HTTP_OPAQUE_ID) p_context
                                               ,NULL);
    if(stream)
    {
      // Getting the impersonated user
      IHttpUser* user = p_context->GetUser();
      if(user)
      {
        stream->m_user = CW2A(user->GetRemoteUserName());
      }
      stream->m_baseURL = rawUrl;
      DETAILLOGV("Accepted an event-stream for SSE (Server-Sent-Events) from %s/%s",stream->m_user.GetString(),rawUrl.GetString());
      // To do for this stream, not for a message
      p_site->HandleEventStream(stream);

      // Return the fact that the request turned into a stream
      p_stream = stream;
      return nullptr;
    }
  }

  // For all types of requests: Create the HTTPMessage
  HTTPMessage* message = new HTTPMessage(type,p_site);
  message->SetURL(rawUrl);
  message->SetReferrer(referrer);
  message->SetAuthorization(authorize);
  message->SetConnectionID(p_request->ConnectionId);
  message->SetContentType(contentType);
  message->SetRemoteDesktop(remDesktop);
  message->SetSender((PSOCKADDR_IN6)sender);
  message->SetCookiePairs(cookie);
  message->SetAcceptEncoding(acceptEncoding);
  message->SetRequestHandle((HTTP_OPAQUE_ID)p_context);

  // Finding the impersonation access token (if any)
  FindingAccessToken(p_context,message);

  if(p_site->GetAllHeaders())
  {
    // If requested so, copy all headers to the message
    message->SetAllHeaders(&p_request->Headers);
  }
  else
  {
    // As a minimum, always add the unknown headers
    // in case of a 'POST', as the SOAPAction header is here too!
    message->SetUnknownHeaders(&p_request->Headers);
  }

  // Handle modified-since 
  // Rest of the request is then not needed any more
  if(type == HTTPCommand::http_get && !modified.IsEmpty())
  {
    message->SetHTTPTime(modified);
    if(DoIsModifiedSince(message))
    {
      delete message;
      return nullptr;
    }
  }

  // Find X-HTTP-Method VERB Tunneling
  if(type == HTTPCommand::http_post && p_site->GetVerbTunneling())
  {
    if(message->FindVerbTunneling())
    {
      DETAILLOGV("Request VERB changed to: %s",message->GetVerb().GetString());
    }
  }

  // Chunks already read by IIS, so just copy them in the message
  if(p_request->EntityChunkCount)
  {
    ReadEntityChunks(message,p_request);
  }

  // Remember the fact that we should read the rest of the message
  message->SetReadBuffer(p_request->Flags & HTTP_REQUEST_FLAG_MORE_ENTITY_BODY_EXISTS,atoi(contentLength));

  // This is the result
  return message;
}

// Reading the first chunks directly from the request handle from IIS
void
HTTPServerIIS::ReadEntityChunks(HTTPMessage* p_message,PHTTP_REQUEST p_request)
{
  unsigned count = p_request->EntityChunkCount;

  // Read all chunks
  for(unsigned ind = 0; ind < count; ++ind)
  {
    PHTTP_DATA_CHUNK chunk = &p_request->pEntityChunks[ind];
    switch(chunk->DataChunkType)
    {
      case HttpDataChunkFromMemory:            p_message->GetFileBuffer()->AddBuffer((uchar*)chunk->FromMemory.pBuffer
                                                                                    ,chunk->FromMemory.BufferLength);
                                               break;
      case HttpDataChunkFromFileHandle:        // Should not happen upon receive
                                               [[fallthrough]];
      case HttpDataChunkFromFragmentCache:     [[fallthrough]];
      case HttpDataChunkFromFragmentCacheEx:   ERRORLOG(87,"Unhandled HTTP chunk type from IIS");
                                               break;
    }
  }
  // Log & Trace the chunks that we just read 
  LogTraceRequestBody(p_message->GetFileBuffer());
}

// Receive incoming HTTP request (p_request->Flags > 0)
bool
HTTPServerIIS::ReceiveIncomingRequest(HTTPMessage* p_message)
{
  UNREFERENCED_PARAMETER(p_message);
  IHttpContext* context     = reinterpret_cast<IHttpContext*>(p_message->GetRequestHandle());
  IHttpRequest* httpRequest = context->GetRequest();
  size_t      contentLength = p_message->GetContentLength();

  // Reading the buffer
  FileBuffer* fbuffer = p_message->GetFileBuffer();
  if(fbuffer->AllocateBuffer(contentLength))
  {
    uchar* buffer    = nullptr;
    size_t size      = 0;
    DWORD  received  = 0;
    DWORD  total     = 0;
    DWORD  remaining = 0;
    BOOL   complete  = FALSE;
    uchar* totbuffer = nullptr;

    // Get the just allocated buffer
    fbuffer->GetBuffer(buffer,size);
    totbuffer = buffer;

    // Loop until we've got all data parts
    do
    {
      received = 0;
      HRESULT hr = httpRequest->ReadEntityBody(buffer,(DWORD)size,FALSE,&received,&complete);
      if(!SUCCEEDED(hr))
      {
        if(HRESULT_CODE(hr) == ERROR_MORE_DATA)
        {
          // Strange, but this code is used to determine that there is NO more data
          // The boolean 'complete' status is **NOT** used! (that one is for async completion!)
          break;
        }
        ERRORLOG(HRESULT_CODE(hr),"Cannot read incoming HTTP buffer");
        return false;
      }
      total  += received; // Total received bytes
      buffer += received; // Advance buffer pointer
      size   -= received; // Size left in the buffer

      // Still to be received from the IIS request
      remaining = httpRequest->GetRemainingEntityBytes();
    }
    while(remaining && received);

    // Check if we received the total predicted message
    if(total < contentLength)
    {
      ERRORLOG(ERROR_INVALID_DATA,"Total received message shorter dan 'ContentLength' header.");
      totbuffer[total] = 0;
    }
    totbuffer[contentLength] = 0;
  }

  // Now also trace the request body of the message
  LogTraceRequestBody(fbuffer);

  return true;
}

// Create a new WebSocket in the subclass of our server
WebSocket* 
HTTPServerIIS::CreateWebSocket(CString p_uri)
{
  WebSocketServerIIS* socket = new WebSocketServerIIS(p_uri);

  // Connect the server logfile, and loglevel
  socket->SetLogfile(m_log);
  socket->SetLogLevel(m_logLevel);

  return socket;
}

// Receive the WebSocket stream and pass on the the WebSocket
void
HTTPServerIIS::ReceiveWebSocket(WebSocket* p_socket,HTTP_OPAQUE_ID /*p_request*/)
{
  WebSocketServerIIS* socket = reinterpret_cast<WebSocketServerIIS*>(p_socket);
  if(socket)
  {
    // Enter the ASYNC listener loop of the websocket
    socket->SocketListener();
  }
}

bool
HTTPServerIIS::FlushSocket(HTTP_OPAQUE_ID p_request)
{
  IHttpContext*     context  = reinterpret_cast<IHttpContext*>(p_request);
  IHttpResponse*    response = context->GetResponse();
  IHttpCachePolicy* policy   = response->GetCachePolicy();
  DWORD bytesSent = 0;

  // Do not buffer. Also turn dynamic compression OFF in IIS-Admin!
  response->DisableBuffering();    // Disable buffering
  response->DisableKernelCache(9); // 9 = HANDLER_HTTPSYS_UNFRIENDLY
  policy->DisableUserCache();      // Disable user caching

  HRESULT hr = response->Flush(FALSE,TRUE,&bytesSent);
  if(hr != S_OK)
  {
    ERRORLOG(GetLastError(),"Flushing WebSocket failed!");
    CancelRequestStream(p_request);
    return false;
  }
  return true;
}

// Finding the impersonation access token
void
HTTPServerIIS::FindingAccessToken(IHttpContext* p_context,HTTPMessage* p_message)
{
  // In case of authentication done: get the authentication token
  HANDLE token = NULL;
  IHttpUser* user = p_context->GetUser();
  if(user)
  {
    // Make duplicate of the token, otherwise IIS will crash!
    if(DuplicateTokenEx(user->GetImpersonationToken()
                        ,TOKEN_DUPLICATE | TOKEN_IMPERSONATE | TOKEN_ALL_ACCESS | TOKEN_READ | TOKEN_WRITE
                        ,NULL
                        ,SecurityImpersonation
                        ,TokenImpersonation
                        ,&token) == FALSE)
    {
      token = NULL;
    }
    else
    {
      // Store the context with the message, so we can handle all derived messages
      p_message->SetAccessToken(token);
    }
  }
}

// Init the stream response
bool
HTTPServerIIS::InitEventStream(EventStream& p_stream)
{
  IHttpContext*    context = (IHttpContext*)p_stream.m_requestID;
  IHttpResponse*  response = context->GetResponse();
  IHttpCachePolicy* policy = response->GetCachePolicy();

  // First comment to push to the stream (not an event!)
  CString init = m_eventBOM ? ConstructBOM() : "";
  init += ":init event-stream\r\n\r\n";

  response->SetStatus(HTTP_STATUS_OK,"OK");

  // Add event/stream header
  SetResponseHeader(response,HttpHeaderContentType,"text/event-stream",true);

  HTTP_DATA_CHUNK dataChunk;
  DWORD bytesSent = 0;

  // Only if a buffer present
  dataChunk.DataChunkType = HttpDataChunkFromMemory;
  dataChunk.FromMemory.pBuffer = (void*)init.GetString();
  dataChunk.FromMemory.BufferLength = (ULONG)init.GetLength();

  // Buffering should be turned off, so chunks get written right away
  response->DisableBuffering();    // Disable buffering
  response->DisableKernelCache(9); // 9 = HANDLER_HTTPSYS_UNFRIENDLY
  policy->DisableUserCache();      // Disable user caching

  HRESULT hr = response->WriteEntityChunks(&dataChunk,1,FALSE,true,&bytesSent);
  if(hr != S_OK)
  {
    ERRORLOG(GetLastError(),"HttpSendResponseEntityBody failed initialisation of an event stream");
  }
  else
  {
    // Increment chunk count
    ++p_stream.m_chunks;

    DETAILLOGV("WriteEntityChunks for event stream sent [%d] bytes",bytesSent);
    // Possibly log and trace what we just sent
    LogTraceResponse(response->GetRawHttpResponse(),(unsigned char*)init.GetString(),init.GetLength());

    hr = response->Flush(false,true,&bytesSent);
  }
  return (hr == S_OK);
}

void
HTTPServerIIS::SetResponseHeader(IHttpResponse* p_response,CString p_name,CString p_value,bool p_replace)
{
  if(p_response->SetHeader(p_name,p_value,(USHORT)p_value.GetLength(),p_replace) != S_OK)
  {
    DWORD   val   = GetLastError();
    CString error = GetLastErrorAsString(val);
    CString bark;
    bark.Format("Cannot set HTTP response header [%s] to value [%s] : %s",p_name.GetString(),p_value.GetString(),error.GetString());
    ERRORLOG(val,bark);
  }
}

void 
HTTPServerIIS::SetResponseHeader(IHttpResponse* p_response,HTTP_HEADER_ID p_id,CString p_value,bool p_replace)
{
  if(p_response->SetHeader(p_id,p_value,(USHORT)p_value.GetLength(),p_replace) != S_OK)
  {
    DWORD   val   = GetLastError();
    CString error = GetLastErrorAsString(val);
    CString bark;
    bark.Format("Cannot set HTTP response header [%d] to value [%s] : %s",p_id,p_value.GetString(),error.GetString());
    ERRORLOG(val,bark);
  }
}

void
HTTPServerIIS::AddUnknownHeaders(IHttpResponse* p_response,UKHeaders& p_headers)
{
  // Something to do?
  if(p_headers.empty())
  {
    return;
  }
  for(UKHeaders::iterator it = p_headers.begin();it != p_headers.end(); ++it)
  {
    CString name  = it->first;
    CString value = it->second;

    SetResponseHeader(p_response,name,value,false);
  }
}

// Setting the overall status of the response message
void
HTTPServerIIS::SetResponseStatus(IHttpResponse* p_response,USHORT p_status,CString p_statusMessage)
{
  p_response->SetStatus(p_status,p_statusMessage);
}

// Sending response for an incoming message
void       
HTTPServerIIS::SendResponse(HTTPMessage* p_message)
{
  IHttpContext*   context     = reinterpret_cast<IHttpContext*>(p_message->GetRequestHandle());
  IHttpResponse*  response    = context ? context->GetResponse() : nullptr;
  FileBuffer*     buffer      = p_message->GetFileBuffer();
  CString         contentType("application/octet-stream"); 
  bool            moredata    = false;

  // See if there is something to send
  if(context == nullptr || response == nullptr)
  {
    ERRORLOG(ERROR_INVALID_PARAMETER,"SendResponse: nothing to send");
    return;
  }

  // Respond to general HTTP status
  int status = p_message->GetStatus();

  // Protocol switch must keep the channel open (here for: WebSocket!)
  if(status == HTTP_STATUS_SWITCH_PROTOCOLS)
  {
    moredata = true;
  }

  // Setting the response status
  SetResponseStatus(response,(USHORT) p_message->GetStatus(),GetHTTPStatusText(p_message->GetStatus()));

  // In case of a 401, we challenge to the client to identify itself
  if (p_message->GetStatus() == HTTP_STATUS_DENIED)
  {
    // See if the message already has an authentication scheme header
    CString challenge = p_message->GetHeader("AuthenticationScheme");
    if(challenge.IsEmpty())
    {
      // Add authentication scheme
      HTTPSite* site = p_message->GetHTTPSite();
      challenge = BuildAuthenticationChallenge(site->GetAuthenticationScheme()
                                              ,site->GetAuthenticationRealm());
    }
    SetResponseHeader(response,HttpHeaderWwwAuthenticate, challenge,true);
  }

  // In case we want IIS to handle the response, and we do nothing!
  // This status get's caught in the MarlinModule::GetCompletionStatus
  if(status == HTTP_STATUS_CONTINUE)
  {
    // Log that we do not do this message, but we pass it on to IIS
    DETAILLOGV("We do **NOT** handle this url: ",p_message->GetURL().GetString());

    // Do **NOT** send an answer twice
    p_message->SetHasBeenAnswered();

    return;
  }

  // Add a known header. (octet-stream or the message content type)
  if(!p_message->GetContentType().IsEmpty())
  {
    contentType = p_message->GetContentType();
  }

  // AddKnownHeader(response,HttpHeaderContentType,contentType);
  SetResponseHeader(response,HttpHeaderContentType,contentType,true);

  // Add the server header or suppress it
  switch(m_sendHeader)
  {
    case SendHeader::HTTP_SH_MICROSOFT:   // Do nothing, Microsoft will add the server header
                                          break;
    case SendHeader::HTTP_SH_MARLIN:      SetResponseHeader(response,HttpHeaderServer,MARLIN_SERVER_VERSION,true);
                                          break;
    case SendHeader::HTTP_SH_APPLICATION: SetResponseHeader(response,HttpHeaderServer,m_name,true);
                                          break;
    case SendHeader::HTTP_SH_WEBCONFIG:   SetResponseHeader(response,HttpHeaderServer,m_configServerName,true);
                                          break;
    case SendHeader::HTTP_SH_HIDESERVER:  // Fill header with empty string will suppress it
                                          SetResponseHeader(response,HttpHeaderServer,"",true);
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
  AddUnknownHeaders(response,ukheaders);

  // Now after the compression, add the total content length
  CString contentLength;
  size_t totalLength = buffer ? buffer->GetLength() : 0;
#ifdef _WIN64
  contentLength.Format("%I64u",totalLength);
#else
  contentLength.Format("%lu",totalLength);
#endif
  SetResponseHeader(response,HttpHeaderContentLength,contentLength,true);

  // Dependent on the filling of FileBuffer
  // Send 1 or more buffers or the file
  if(buffer->GetHasBufferParts())
  {
    SendResponseBufferParts(response,buffer,totalLength,moredata);
  }
  else if(buffer->GetFileName().IsEmpty())
  {
    SendResponseBuffer(response,buffer,totalLength,moredata);
  }
  else
  {
    SendResponseFileHandle(response,buffer,moredata);
  }
  if(GetLastError())
  {
    // Error handler
    CString message = GetLastErrorAsString(tls_lastError);
    m_log->AnalysisLog(__FUNCTION__, LogType::LOG_ERROR,true,"HTTP Answer [%d:%s]",GetLastError(),message.GetString());
    // Reset the last error
    SetError(NO_ERROR);
  }

  // Possibly log and trace what we just sent
  LogTraceResponse(response->GetRawHttpResponse(),buffer);

  // Do **NOT** send an answer twice
  p_message->SetHasBeenAnswered();
}

// Subfunctions for SendResponse
bool
HTTPServerIIS::SendResponseBuffer(IHttpResponse*  p_response
                                 ,FileBuffer*     p_buffer
                                 ,size_t          p_totalLength
                                 ,bool            p_more /*= false*/)
{
  uchar* entity       = NULL;
  DWORD  result       = 0;
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

    DWORD sent = 0L;
    BOOL  completion = false;
    HRESULT hr = p_response->WriteEntityChunks(&dataChunk,1,false,p_more,&sent,&completion);
    if(SUCCEEDED(hr))
    {
      DETAILLOGV("ResponseBuffer [%ul] bytes sent",entityLength);
    }
    else
    {
      result = GetLastError();
      ERRORLOG(result,"ResponseBuffer");
    }
  }
  return (result == NO_ERROR);
}

void
HTTPServerIIS::SendResponseBufferParts(IHttpResponse* p_response
                                      ,FileBuffer*    p_buffer
                                      ,size_t         p_totalLength
                                      ,bool           p_moredata /*= false*/)
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
    }
    // Flag to calculate the last sending part
    bool moreData = p_moredata;
    if(p_moredata == false)
    {
      moreData = (totalSent + entityLength) < p_totalLength;
    }
    BOOL completion = false;

    HRESULT hr = p_response->WriteEntityChunks(&dataChunk,1,false,moreData,&bytesSent,&completion);

    if(SUCCEEDED(hr))
    {
      DETAILLOGV("HTTP ResponseBufferPart [%d] bytes sent",bytesSent);
    }
    else
    {
      DWORD result = GetLastError();
      ERRORLOG(result,"HTTP ResponseBufferPart error");
      break;
    }
    // Next buffer part
    totalSent += entityLength;
    ++transmitPart;
  }
}

void
HTTPServerIIS::SendResponseFileHandle(IHttpResponse* p_response,FileBuffer* p_buffer,bool p_more /*= false*/)
{
  DWORD  result    = 0;
  HANDLE file      = NULL;
  HTTP_DATA_CHUNK dataChunk;
  memset(&dataChunk,0,sizeof(HTTP_DATA_CHUNK));

  // File to transmit
  if(p_buffer->OpenFile() == false)
  {
    ERRORLOG(GetLastError(),"OpenFile for SendHttpResponse");
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
    SendResponseError(p_response,m_clientErrorPage,413,"Request entity too large");
    // Error state
    ERRORLOG(ERROR_INVALID_PARAMETER,"File to send is too big (>4G)");
    // Close the file handle
    p_buffer->CloseFile();
    return;
  }

  // Send entity body from a file handle.
  dataChunk.DataChunkType = HttpDataChunkFromFileHandle;
  dataChunk.FromFileHandle.ByteRange.StartingOffset.QuadPart = 0;
  dataChunk.FromFileHandle.ByteRange.Length.QuadPart = fileSize; // HTTP_BYTE_RANGE_TO_EOF;
  dataChunk.FromFileHandle.FileHandle = file;

  DWORD sent = 0L;
  BOOL  completion = false;

  // Writing the chunk
  HRESULT hr = p_response->WriteEntityChunks(&dataChunk,1,false,p_more,&sent,&completion);
  if(SUCCEEDED(hr))
  {
    DETAILLOGV("SendResponseEntityBody for file. Bytes: %ul",fileSize);
  }
  else
  {
    result = GetLastError();
    ERRORLOG(result,"SendResponseEntityBody for file");
  }
  // Now close our file handle
  p_buffer->CloseFile();
}

void
HTTPServerIIS::SendResponseError(IHttpResponse* p_response
                                ,CString&       p_page
                                ,int            p_error
                                ,const char*    p_reason)
{
  DWORD result = 0;
  CString sending;
  HTTP_DATA_CHUNK dataChunk;
  memset(&dataChunk,0,sizeof(HTTP_DATA_CHUNK));

  // Format our error page
  sending.Format(p_page,p_error,p_reason);

  // Add an entity chunk.
  dataChunk.DataChunkType           = HttpDataChunkFromMemory;
  dataChunk.FromMemory.pBuffer      = (void*) sending.GetString();
  dataChunk.FromMemory.BufferLength = sending.GetLength();

  DWORD sent = 0L;
  BOOL  completion = false;

  // Writing the chunk
  HRESULT hr = p_response->WriteEntityChunks(&dataChunk,1,false,false,&sent,&completion);
  if(SUCCEEDED(hr))
  {
    DETAILLOGV("SendResponseError. Bytes sent: %d",sent);
    if(completion == false)
    {
      ERRORLOG(ERROR_INVALID_FUNCTION,"ResponseFileHandle: But IIS does not expect to deliver the contents!!");
    }
    // Possibly log and trace what we just sent
    LogTraceResponse(p_response->GetRawHttpResponse(),(unsigned char*)sending.GetString(),sending.GetLength());
  }
  else
  {
    result = GetLastError();
    ERRORLOG(result,"SendResponseEntityBody for file");
  }
}

// Sending a chunk to an event stream
bool 
HTTPServerIIS::SendResponseEventBuffer(HTTP_OPAQUE_ID p_response
                                      ,const char*    p_buffer
                                      ,size_t         p_length
                                      ,bool           p_continue /*= true*/)
{
  DWORD  bytesSent = 0;
  HTTP_DATA_CHUNK dataChunk;
  IHttpContext*   context  = (IHttpContext*)p_response;
  IHttpResponse*  response = context->GetResponse();

  // Only if a buffer present
  dataChunk.DataChunkType           = HttpDataChunkFromMemory;
  dataChunk.FromMemory.pBuffer      = (void*)p_buffer;
  dataChunk.FromMemory.BufferLength = (ULONG)p_length;

  HRESULT hr = response->WriteEntityChunks(&dataChunk,1,FALSE,p_continue,&bytesSent);
  if(hr != S_OK)
  {
    ERRORLOG(GetLastError(),"WriteEntityChunks failed for SendEvent");
  }
  else
  {
    DETAILLOGV("WriteEntityChunks for event stream sent [%d] bytes",p_length);
    hr = response->Flush(false,p_continue,&bytesSent);
    if(hr != S_OK)
    {
      ERRORLOG(GetLastError(),"Flushing event stream failed!");
    }
    // Possibly log and trace what we just sent
    LogTraceResponse(nullptr,(unsigned char*) p_buffer,(int) p_length);
  }
  // Final closing of the connection
  if(p_continue == false)
  {
    CancelRequestStream(p_response);
  }
  return (hr == S_OK);
}

// Used for canceling a WebSocket or an event stream
void
HTTPServerIIS::CancelRequestStream(HTTP_OPAQUE_ID p_response)
{
  IHttpContext*  context = (IHttpContext*)p_response;
  IHttpResponse* response = context->GetResponse();
  
  try
  {
    // Now ready with the IIS context. Original request is finished
    context->IndicateCompletion(RQ_NOTIFICATION_FINISH_REQUEST);
    // Set disconnection
    response->SetNeedDisconnect();

    DETAILLOG1("Event/Socket connection closed");
  }
  catch(...)
  {
    ERRORLOG(ERROR_INVALID_PARAMETER,"Cannot close Event/WebSocket stream!");
  }
}

