/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: HTTPRequest.cpp
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
#include "HTTPRequest.h"
#include "HTTPMessage.h"
#include "HTTPServer.h"
#include "HTTPSite.h"
#include "HTTPError.h"
#include "HTTPTime.h"
#include "WebSocketServer.h"
#include "AutoCritical.h"
#include "ConvertWideString.h"
#include <ServiceReporting.h>
#include <WebSocket.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define DETAILLOG1(text)          if(MUSTLOG(HLL_LOGGING) && m_server) { m_server->DetailLog (_T(__FUNCTION__),LogType::LOG_INFO,text); }
#define DETAILLOGS(text,extra)    if(MUSTLOG(HLL_LOGGING) && m_server) { m_server->DetailLogS(_T(__FUNCTION__),LogType::LOG_INFO,text,extra); }
#define DETAILLOGV(text,...)      if(MUSTLOG(HLL_LOGGING) && m_server) { m_server->DetailLogV(_T(__FUNCTION__),LogType::LOG_INFO,text,__VA_ARGS__); }
#define WARNINGLOG(text,...)      if(MUSTLOG(HLL_LOGGING) && m_server) { m_server->DetailLogV(_T(__FUNCTION__),LogType::LOG_WARN,text,__VA_ARGS__); }
#define ERRORLOG(code,text)       if(MUSTLOG(HLL_ERRORS)  && m_server) { m_server->ErrorLog  (_T(__FUNCTION__),(code),(text)); }

//////////////////////////////////////////////////////////////////////////

OutstandingIO::OutstandingIO(HTTPRequest* p_request)
              :m_request(p_request)
              ,m_action(IO_Nothing)
{
  Internal     = 0;
  InternalHigh = 0;
  Pointer      = nullptr;
  hEvent       = nullptr;
}


//////////////////////////////////////////////////////////////////////////
//
// THE REQUEST
//
//////////////////////////////////////////////////////////////////////////

// XTOR
HTTPRequest::HTTPRequest(HTTPServer* p_server)
            :m_server(p_server)
            ,m_incoming(this)
            ,m_reading(this)
            ,m_writing(this)
{
  // Derive logging from the server
  m_logLevel = m_server->GetLogLevel();

  // Initialize empty
  m_policy.Policy = HttpCachePolicyNocache;
  m_policy.SecondsToLive = 0;
  memset(&m_sendChunk,0,sizeof(m_sendChunk));

  InitializeCriticalSection(&m_critical);

  TRACE0("CTOR request\n");
}

// DTOR
HTTPRequest::~HTTPRequest()
{
  TRACE0("XTOR request\n");
  ClearMemory();
  DeleteCriticalSection(&m_critical);
}

// Remove all memory from the heap
void
HTTPRequest::ClearMemory()
{
  AutoCritSec lock(&m_critical);

  if(m_message)
  {
    m_message->DropReference();
    m_message = nullptr;
  }
  if(m_request)
  {
    free(m_request);
    m_request = nullptr;
  }
  if(m_response)
  {
    delete m_response;
    m_response = nullptr;
  }
  if(m_readBuffer)
  {
    delete m_readBuffer;
    m_readBuffer = nullptr;
  }
  if(m_sendBuffer)
  {
    delete[] m_sendBuffer;
    m_sendBuffer = nullptr;
  }
  if(m_unknown)
  {
    free(m_unknown);
    m_unknown = nullptr;
  }
  for(auto& str : m_strings)
  {
    delete[] str;
  }
  m_strings.clear();
}

// Callback from I/O Completion port right out of the threadpool
/*static*/ void
HandleAsynchroneousIO(OVERLAPPED* p_overlapped)
{
  TRACE0("Handle ASYNC I/O\n");

  OutstandingIO* outstanding = reinterpret_cast<OutstandingIO*>(p_overlapped);
  HTTPRequest* request = dynamic_cast<HTTPRequest*>(outstanding->m_request);
  if(request)
  {
    request->HandleAsynchroneousIO(outstanding->m_action);
  }
}

// Callback from I/O Completion port 
void
HTTPRequest::HandleAsynchroneousIO(IOAction p_action)
{
  TRACE0("Handle I/O Action\n");

  switch(p_action)
  {
    case IO_Nothing:    break;
    case IO_Request:    ReceivedRequest();  break;
    case IO_Reading:    ReceivedBodyPart(); break;
    case IO_Response:   SendResponseBody(); break;
    case IO_Writing:    SendBodyPart();     break;
    case IO_StartStream:StartedStream();    break;
    case IO_WriteStream:SendStreamPart();   break;
    case IO_Cancel:     Finalize();         break;
    default:            ERRORLOG(ERROR_INVALID_PARAMETER,_T("Unexpected outstanding async I/O"));
  }
}

// Start a new request against the server
void 
HTTPRequest::StartRequest()
{
  TRACE0("Start request\n");

  // Buffer size for a new request
  DWORD size = INIT_HTTP_BUFFERSIZE;

  // Allocate the request object
  if(m_request == nullptr)
  {
    m_request = (PHTTP_REQUEST) calloc(1,(size_t)size + 1);
  }
  if(!m_request)
  {
    // Error to the server log
    ERRORLOG(ERROR_NOT_ENOUGH_MEMORY,_T("Starting new HTTP Request"));
    Finalize();
    return;
  }
  // Set type of request
  m_incoming.m_action = IO_Request;
  // Get queue handle
  HANDLE queue = m_server->GetRequestQueue();
  // Reset the request id
  m_requestID  = HTTP_NULL_ID;

  // Sit tight for the next request
  ULONG result = HttpReceiveHttpRequest(queue,        // Request Queue
                                        HTTP_NULL_ID, // Request ID
                                        0,            // Do NOT copy the body
                                        m_request,    // HTTP request buffer
                                        size,         // request buffer length
                                        nullptr,      // bytes received
                                        &m_incoming); // LPOVERLAPPED
  // Check the result
  if(result != ERROR_IO_PENDING && result != NO_ERROR)
  {
    // Error to the server log
    ERRORLOG(result,_T("Starting new HTTP Request"));
    Finalize();
  }
  else
  {
    m_active = true;
  }
}

// Start response cycle from the HTTPServer / HTTPSite / SiteHandler
void
HTTPRequest::StartResponse(HTTPMessage* p_message)
{
  TRACE0("Start response\n");

  // CHECK if we send the same message!!
  if(p_message && p_message != m_message)
  {
    if(m_message)
    {
      m_message->SetHasBeenAnswered();
      m_message->DropReference();
    }
    m_message = p_message;
    m_message->AddReference();
  }
  // Start response
  StartSendResponse();
}

// Primarily receive a request
void
HTTPRequest::ReceivedRequest()
{
  TRACE0("Received request\n");

  // Check server pointer
  if(m_server == nullptr)
  {
    SvcReportErrorEvent(0,false,_T(__FUNCTION__),_T("FATAL ERROR: No server when receiving HTTP request!"));
    return;
  }
  HANDLE accessToken = nullptr;

  // Get status from the OVERLAPPED result
  DWORD result = (DWORD) m_incoming.Internal & 0x0FFFF;
  // Catch normal abortion statuses
  // Test if server already stopped, and we are here because of the stopping
  if((result == ERROR_CONNECTION_INVALID || result == ERROR_OPERATION_ABORTED) ||
     (!m_server->GetIsRunning()))
  {
    // Server stopped by closing server handles
    DETAILLOG1(_T("HTTP Server stopped in mainloop"));
    Finalize();
    return;
  }
  // Catch ABNORMAL abortion status
  if(result != 0)
  {
    ERRORLOG(result,_T("Error receiving a HTTP request in mainloop"));
    Finalize();
    return;
  }

  // Get the primary request-id for the request queue
  m_requestID = m_request->RequestId;

  // Grab the senders content
  XString   acceptTypes     = LPCSTRToString(m_request->Headers.KnownHeaders[HttpHeaderAccept         ].pRawValue);
  XString   contentType     = LPCSTRToString(m_request->Headers.KnownHeaders[HttpHeaderContentType    ].pRawValue);
  XString   contentLength   = LPCSTRToString(m_request->Headers.KnownHeaders[HttpHeaderContentLength  ].pRawValue);
  XString   acceptEncoding  = LPCSTRToString(m_request->Headers.KnownHeaders[HttpHeaderAcceptEncoding ].pRawValue);
  XString   cookie          = LPCSTRToString(m_request->Headers.KnownHeaders[HttpHeaderCookie         ].pRawValue);
  XString   authorize       = LPCSTRToString(m_request->Headers.KnownHeaders[HttpHeaderAuthorization  ].pRawValue);
  XString   modified        = LPCSTRToString(m_request->Headers.KnownHeaders[HttpHeaderIfModifiedSince].pRawValue);
  XString   referrer        = LPCSTRToString(m_request->Headers.KnownHeaders[HttpHeaderReferer        ].pRawValue);
  XString   rawUrl          = WStringToString(m_request->CookedUrl.pFullUrl);
  PSOCKADDR sender          = m_request->Address.pRemoteAddress;
  PSOCKADDR receiver        = m_request->Address.pLocalAddress;
  int       remDesktop      = m_server->FindRemoteDesktop(m_request->Headers.UnknownHeaderCount
                                                         ,m_request->Headers.pUnknownHeaders);
  size_t    contentLen      = (size_t)_ttoll(contentLength);

  // Our charset
  XString charset;
  Encoding encoding = Encoding::EN_ACP;

  // If positive request ID received
  if(m_requestID)
  {
    // Log earliest as possible
    DETAILLOGV(_T("Received HTTP %s call from [%s] with length: %I64u for: %s")
               ,GetHTTPVerb(m_request->Verb,m_request->pUnknownVerb).GetString()
               ,SocketToServer((PSOCKADDR_IN6)sender).GetString()
               ,m_request->BytesReceived
               ,rawUrl.GetString());

    // Find our charset
    charset = FindCharsetInContentType(contentType);
    if(!charset.IsEmpty())
    {
      encoding = (Encoding)CharsetToCodepage(charset);
    }

    // Trace the request in full
    if(m_server)
    {
      m_server->LogTraceRequest(m_request,nullptr,encoding);
    }
  }

  // Recording expected content length
  m_expect = _ttol(contentLength);

  // FInding the site
  bool eventStream = false;
  LPFN_CALLBACK callback = nullptr;
  m_site = reinterpret_cast<HTTPSite*>(m_request->UrlContext);
  if(m_site)
  {
    callback    = m_site->GetCallback();
    eventStream = m_site->GetIsEventStream();
  }

  // See if we must substitute for a sub-site
  if(m_server->GetHasSubsites())
  {
    XString absPath = WStringToString(m_request->CookedUrl.pAbsPath);
    m_site = m_server->FindHTTPSite(m_site,absPath);
  }

  // Check our authentication
  if(m_site->GetAuthentication())
  {
    // Now check for authentication and possible send 401 back
    if(m_server->CheckAuthentication(m_request,(HTTP_OPAQUE_ID)this,m_site,rawUrl,authorize,accessToken) == false)
    {
      // Not authenticated, go back for next request
      return;
    }
  }

  // Remember the context: easy in API 2.0
  if(callback == nullptr && m_site == nullptr)
  {
    if(m_message)
    {
      m_message->DropReference();
    }
    m_message = new HTTPMessage(HTTPCommand::http_response,HTTP_STATUS_NOT_FOUND);
    m_message->AddReference();
    m_message->SetRequestHandle((HTTP_OPAQUE_ID)this);
    StartSendResponse();
    return;
  }

  // Translate the command. Now reduced to just this switch
  HTTPCommand type = HTTPCommand::http_no_command;
  switch(m_request->Verb)
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
                            type = m_server->GetUnknownVerb(m_request->pUnknownVerb);
                            if(type == HTTPCommand::http_no_command)
                            {
                              // Non implemented like HttpVerbTRACK or other non-known verbs
                              if(m_message)
                              {
                                m_message->DropReference();
                              }
                              m_message = new HTTPMessage(HTTPCommand::http_response,HTTP_STATUS_NOT_SUPPORTED);
                              m_message->AddReference();
                              m_message->SetRequestHandle((HTTP_OPAQUE_ID)this);
                              StartSendResponse();
                              return;
                            }
                            break;
  }

  // Receiving the initiation of an event stream for the server
  acceptTypes.Trim();
  EventStream* stream = nullptr;
  if((type == HTTPCommand::http_get) && (eventStream || acceptTypes.Left(17).CompareNoCase(_T("text/event-stream")) == 0))
  {
    XString absolutePath = WStringToString(m_request->CookedUrl.pAbsPath);
    if(m_server->CheckUnderDDOSAttack((PSOCKADDR_IN6)sender,absolutePath))
    {
      return;
    }
    stream = m_server->SubscribeEventStream((PSOCKADDR_IN6) sender
                                            ,remDesktop
                                            ,m_site
                                            ,m_site->GetSite()
                                            ,absolutePath
                                            ,(HTTP_OPAQUE_ID) this
                                            ,accessToken);
  }

  // For all types of requests: Create the HTTPMessage
  if(m_message)
  {
    m_message->DropReference();
  }
  m_message = new HTTPMessage(type,m_site);
  m_message->SetRequestHandle((HTTP_OPAQUE_ID)this);
  m_message->AddReference();
  // Enter our primary information from the request
  m_message->SetURL(rawUrl);
  m_message->SetReferrer(referrer);
  m_message->SetAuthorization(authorize);
  m_message->SetConnectionID(m_request->ConnectionId);
  m_message->SetContentType(contentType);
  m_message->SetContentLength(contentLen);
  m_message->SetAccessToken(accessToken);
  m_message->SetRemoteDesktop(remDesktop);
  m_message->SetSender  ((PSOCKADDR_IN6)sender);
  m_message->SetReceiver((PSOCKADDR_IN6)receiver);
  m_message->SetCookiePairs(cookie);
  m_message->SetAcceptEncoding(acceptEncoding);
  m_message->SetAllHeaders(&m_request->Headers);
  m_message->SetUnknownHeaders(&m_request->Headers);
  m_message->SetEncoding(encoding);

  // Handle modified-since 
  // Rest of the request is then not needed any more
  if(type == HTTPCommand::http_get && !modified.IsEmpty())
  {
    m_message->SetHTTPTime(modified);
    if(m_server->DoIsModifiedSince(m_message))
    {
      // Answer already sent, go on to the next request
      return;
    }
  }

  // Find routing information within the site
  m_server->CalculateRouting(m_site,m_message);

  // Find X-HTTP-Method VERB Tunneling
  if(type == HTTPCommand::http_post && m_site->GetVerbTunneling())
  {
    if(m_message->FindVerbTunneling())
    {
      DETAILLOGV(_T("Request VERB changed to: %s"),m_message->GetVerb().GetString());
    }
  }

  // Go handle the stream if we got one
  if(stream)
  {
    stream->m_baseURL = rawUrl;
    if(!m_site->HandleEventStream(m_message,stream))
    {
      m_server->RemoveEventStream(stream);
      delete stream;
    }
    return;
  }

  // Remember the fact that we should read the rest of the message
  if((m_request->Flags & HTTP_REQUEST_FLAG_MORE_ENTITY_BODY_EXISTS) && contentLen > 0)
  {
    // Read the body of the message, before we handle it
    StartReceiveRequest();
  }
  else
  {
    // Go straight on to the handling of the message
    PostReceive();
  }
}

void 
HTTPRequest::StartReceiveRequest()
{
  TRACE0("Start received request\n");

  // Make sure we have a buffer
  if(!m_readBuffer)
  {
    m_readBuffer = new BYTE[INIT_HTTP_BUFFERSIZE];
  }
  // Set reading action
  ResetOutstanding(m_reading);
  m_reading.m_action = IO_Reading;

  // Use these flags while reading the request body
  ULONG flags = 0; //  HTTP_RECEIVE_REQUEST_ENTITY_BODY_FLAG_FILL_BUFFER;

  // Issue the async read request
  DWORD result = HttpReceiveRequestEntityBody(m_server->GetRequestQueue()
                                             ,m_requestID
                                             ,flags
                                             ,m_readBuffer
                                             ,INIT_HTTP_BUFFERSIZE
                                             ,NULL
                                             ,&m_reading);
  
  // See if we are ready reading the whole body
  if(result == ERROR_HANDLE_EOF)
  {
    // Ready reading the whole body
    PostReceive();
  }
  else if(result != ERROR_IO_PENDING && result != NO_ERROR)
  {
    ERRORLOG(result,_T("Error receiving HTTP request body"));
    Finalize();
  }
}

// 2) Receive trailing request body
void 
HTTPRequest::ReceivedBodyPart()
{
  TRACE0("Receive body part\n");

  DWORD result = (DWORD)(m_reading.Internal & 0x0FFFF);
  if(result == ERROR_HANDLE_EOF || result == NO_ERROR)
  {
    // Store the result of the read action
    DWORD bytes = (DWORD) m_reading.InternalHigh;
    if(bytes)
    {
      m_readBuffer[bytes] = 0;
      m_message->GetFileBuffer()->AddBuffer(m_readBuffer,bytes);
    }

    // See how far we have come, so we do not read past EOF
    size_t readSofar = m_message->GetFileBuffer()->GetLength();
    size_t mustRead  = m_message->GetContentLength();

    if(result == NO_ERROR && readSofar < mustRead)
    {
      // (Re)start the next read request
      StartReceiveRequest();
      return;
    }
  }
  else
  {
    ERRORLOG(result,_T("Error receiving bodypart"));
    m_server->RespondWithServerError(m_message,HTTP_STATUS_SERVER_ERROR,_T("Server error"));
    Finalize();
    return;
  }
  PostReceive();
}

// We have read the whole body of a message
// Now go processing it
void
HTTPRequest::PostReceive()
{
  TRACE0("Post Receive\n");

  // Now also trace the request body of the message
  m_server->LogTraceRequestBody(m_message);

  // In case of a POST, try to convert character set before submitting to site
  if(m_message->GetCommand() == HTTPCommand::http_post)
  {
    if(m_message->GetContentType().Find(_T("multipart")) <= 0)
    {
      m_server->HandleTextContent(m_message);
    }
  }
  DETAILLOGV(_T("Received %s message from: %s Size: %lu")
             ,headers[(unsigned)m_message->GetCommand()]
             ,SocketToServer(m_message->GetSender()).GetString()
             ,m_message->GetBodyLength());

  // Message is now complete
  // Go straight on to the handling of the message
  m_site->HandleHTTPMessage(m_message);
}

void
HTTPRequest::StartSendResponse()
{
  TRACE0("Start SendResponse\n");

  // Mark the fact that we begin responding
  m_responding = true;
  // Make sure we have the correct event action
  ResetOutstanding(m_writing);
  m_writing.m_action = IO_Response;

  // We promise to always call HttpSendResponseEntityBody
  ULONG flags = HTTP_SEND_RESPONSE_FLAG_DISCONNECT;
  
  if(m_message->GetFileBuffer()->GetLength() > 0 || 
     m_message->GetChunkNumber())
  {
    flags = HTTP_SEND_RESPONSE_FLAG_MORE_DATA;
  }
  else
  {
    m_writing.m_action = IO_Writing;
  }
  // Place HTTPMessage in the response structure
  FillResponse(m_message->GetStatus());

  // Prepare our cache-policy
  m_policy.Policy        = m_server->GetCachePolicy();
  m_policy.SecondsToLive = m_server->GetCacheSecondsToLive();

  // Special case for WebSockets protocol
  OutstandingIO* overlapped = &m_writing;
  if (m_response->StatusCode == HTTP_STATUS_SWITCH_PROTOCOLS)
  {
    // Keep the request/socket open
    flags = HTTP_SEND_RESPONSE_FLAG_MORE_DATA |
            HTTP_SEND_RESPONSE_FLAG_OPAQUE    |
            HTTP_SEND_RESPONSE_FLAG_BUFFER_DATA;
    m_writing.m_action     = IO_Nothing;
    m_policy.Policy        = HttpCachePolicyNocache;
    m_policy.SecondsToLive = 0;
    // Last action is non-overlapped.
    overlapped = nullptr;
  }

  // Trace the principal response, before sending
  // Sometimes the async is so quick, we cannot trace it after the sending
  m_server->LogTraceResponse(m_response,nullptr,m_message->GetEncoding());

  // Send the response
  ULONG result = HttpSendHttpResponse(m_server->GetRequestQueue(),    // ReqQueueHandle
                                      m_requestID,       // Request ID
                                      flags,             // Flags
                                      m_response,        // HTTP response
                                      &m_policy,         // Policy
                                      nullptr,           // bytes sent  (OPTIONAL)
                                      nullptr,           // pReserved2  (must be NULL)
                                      0,                 // Reserved3   (must be 0)
                                      overlapped,        // LPOVERLAPPED(OPTIONAL)
                                      nullptr);          // LogData     (must be NULL)

  TRACE1("Result of HttpSendHttpResponse: %d\n", result);

  // Check for error
  if(result != ERROR_IO_PENDING && result != NO_ERROR)
  {
    ERRORLOG(result,_T("Sending HTTP Response"));
    Finalize();
  }
  else if(m_response->StatusCode == HTTP_STATUS_SWITCH_PROTOCOLS)
  {
    DETAILLOG1(_T("Upgrading WebSocket. Ready with HTTP protocol."));
  }
}

void
HTTPRequest::SendResponseBody()
{
  TRACE0("Send ResponseBody\n");

  DWORD error = m_writing.Internal & 0x0FFFF;
  if(error)
  {
    ERRORLOG(error,_T("Error sending HTTP headers"));
    Finalize();
    return;
  }
  // Now begin writing our response body parts
  ResetOutstanding(m_writing);
  m_writing.m_action = IO_Writing;

  // Default is we send all data in one go
  ULONG flags = HTTP_SEND_RESPONSE_FLAG_DISCONNECT;

  // Prepare send buffer
  memset(&m_sendChunk,0,sizeof(HTTP_DATA_CHUNK));
  PHTTP_DATA_CHUNK chunks = &m_sendChunk;
  USHORT chunkcount = 1;

  // Find out which one of three actions we must do
  FileBuffer* filebuf = m_message->GetFileBuffer();

  if(filebuf->GetHasBufferParts())
  {
    // FILEBUFFER CONTAINS VARIOUS MEMORY CHUNKS
    // Send buffer in various chunks
    uchar* buffer = nullptr;
    size_t length = 0;
    filebuf->GetBufferPart(m_bufferpart,buffer,length);

    m_sendChunk.DataChunkType           = HttpDataChunkFromMemory;
    m_sendChunk.FromMemory.pBuffer      = buffer;
    m_sendChunk.FromMemory.BufferLength = (ULONG)length;

    // See if there are more buffer parts to come
    if(++m_bufferpart < filebuf->GetNumberOfParts())
    {
      flags = HTTP_SEND_RESPONSE_FLAG_MORE_DATA;
    }
  }
  else if(m_file)
  {
    // FILE BUFFER CONTAINS A FILE REFERENCE TO SENT
    // Send file in form of a file-handle to transmit

    m_sendChunk.DataChunkType = HttpDataChunkFromFileHandle;
    m_sendChunk.FromFileHandle.ByteRange.StartingOffset.QuadPart = 0;
    m_sendChunk.FromFileHandle.ByteRange.Length.QuadPart = HTTP_BYTE_RANGE_TO_EOF;
    m_sendChunk.FromFileHandle.FileHandle = m_file;
  }
  else if(filebuf->GetLength())
  {
    // FILEBUFFER CONTAINS EXACTLY ONE BUFFER AS AN OPTIMIZATION
    // Send buffer in one go
    uchar* buffer = nullptr;
    size_t length = 0;
    filebuf->GetBuffer(buffer,length);

    m_sendChunk.DataChunkType           = HttpDataChunkFromMemory;
    m_sendChunk.FromMemory.pBuffer      = buffer;
    m_sendChunk.FromMemory.BufferLength = (ULONG)length;
  }
  else
  {
    // Send EOF, just send the flag!
    chunkcount = 0;
    chunks = nullptr;
  }

  // See if it is the first chunk for a 'chunked' transfer
  if(m_message->GetChunkNumber())
  {
    flags = HTTP_SEND_RESPONSE_FLAG_MORE_DATA;
  }

  ULONG result = HttpSendResponseEntityBody(m_server->GetRequestQueue(),
                                            m_requestID,    // Our request
                                            flags,          // More/Last data
                                            chunkcount,     // Entity Chunk Count.
                                            chunks,         // CHUNCK
                                            nullptr,        // Bytes
                                            nullptr,        // Reserved1
                                            0,              // Reserved2
                                            &m_writing,     // OVERLAPPED
                                            nullptr);       // LOGDATA

  // Check for error
  if(result != ERROR_IO_PENDING && result != NO_ERROR)
  {
    ERRORLOG(result,_T("Sending response body part"));
    m_responding = false;
    Finalize();
    return;
  }

  // Still responding or done?
  m_responding = (flags == HTTP_SEND_RESPONSE_FLAG_MORE_DATA);
}

// At least one body part has been sent
// Now see if we are done, or start the next body part send
void
HTTPRequest::SendBodyPart()
{
  TRACE0("Send BodyPart\n");

  // Check status of the OVERLAPPED structure
  DWORD error = m_writing.Internal & 0x0FFFF;
  if(error)
  {
    ERRORLOG(error,_T("While sending HTTP response part"));
  }
  else
  {
    // Getting the file buffer
    FileBuffer* filebuf = m_message->GetFileBuffer();

    // See if we did a file sent
    if(m_file)
    {
      filebuf->CloseFile();
      m_file = nullptr;
    }

    if(m_responding)
    {
      // See if we need to send another chunk to send
      if(filebuf->GetHasBufferParts() &&
        (m_bufferpart < filebuf->GetNumberOfParts()))
      {
        SendResponseBody();
        return;
      }
    }
    // Possibly log and trace what we just sent
    m_server->LogTraceResponse(m_response,m_message);
  }

  FlushFileBuffers(m_server->GetRequestQueue());

  if(m_message->GetChunkNumber() == 0)
  {
    // Message is done. Break the connection with the HTTPRequest
    m_message->SetHasBeenAnswered();

    // End of the line for the whole request
    // We did send everything as an answer
    Finalize();
  }
  else if(m_chunkEvent)
  {
    // First chunk is sent, continue with "SendChunk"
    ::SetEvent(m_chunkEvent);
    m_chunkEvent = NULL;
  }
}

//////////////////////////////////////////////////////////////////////////
//
// OUTGOING STREAMS
//
//////////////////////////////////////////////////////////////////////////

// Start a response stream
void 
HTTPRequest::StartEventStreamResponse()
{
  TRACE0("Start EventStream Response\n");

  // First comment to push to the stream (not an event!)
  // Always UTF-8 compatible, so simple ANSI string
  char* init = ":init event-stream\r\n\r\n";
  int length = (int)strlen(init);

  // Initialize the HTTP response structure.
  FillResponse(HTTP_STATUS_OK,true);

  // Add a known header.
  AddKnownHeader(HttpHeaderContentType,_T("text/event-stream"));

  // Set init in the send buffer
  if(m_sendBuffer)
  {
    delete[] m_sendBuffer;
  }
  m_sendBuffer = new BYTE[length + 1];
  memcpy_s(m_sendBuffer,(size_t)(length  + 1),init,(size_t)length + 1);

  // Setup as a data-chunk info structure
  memset(&m_sendChunk,0,sizeof(HTTP_DATA_CHUNK));
  m_sendChunk.DataChunkType           = HttpDataChunkFromMemory;
  m_sendChunk.FromMemory.pBuffer      = m_sendBuffer;
  m_sendChunk.FromMemory.BufferLength = (ULONG)length;

  // Prepare send buffer
  m_response->EntityChunkCount = 1;
  m_response->pEntityChunks    = &m_sendChunk;

  // Preparing the cache-policy
  m_policy.Policy        = m_server->GetCachePolicy();
  m_policy.SecondsToLive = m_server->GetCacheSecondsToLive();

  // Because the entity body is sent in one call, it is not
  // required to specify the Content-Length.
  ULONG flags  = HTTP_SEND_RESPONSE_FLAG_MORE_DATA;

  ResetOutstanding(m_writing);
  m_writing.m_action = IO_StartStream;

  ULONG result = HttpSendHttpResponse(m_server->GetRequestQueue(),    // ReqQueueHandle
                                      m_requestID,       // Request ID
                                      flags,             // Flags
                                      m_response,        // HTTP response
                                      &m_policy,         // Policy
                                      nullptr,           // bytes sent  (OPTIONAL)
                                      nullptr,           // pReserved2  (must be NULL)
                                      0,                 // Reserved3   (must be 0)
                                      &m_writing,        // LPOVERLAPPED(OPTIONAL)
                                      nullptr);          // pReserved4  (must be NULL)

  DETAILLOGV(_T("HTTP Response %d %s"),m_response->StatusCode,m_response->pReason);

  // Check for error
  if(result != ERROR_IO_PENDING && result != NO_ERROR)
  {
    ERRORLOG(result,_T("Sending HTTP Response for event stream"));
    Finalize();
  }

  // Log&Trace what we just send
  m_server->LogTraceResponse(m_response,m_sendBuffer,length);
}

void
HTTPRequest::StartedStream()
{
  TRACE0("Started stream\n");

  // Check status of the OVERLAPPED structure
  DWORD error = m_writing.Internal & 0x0FFFF;
  if(error)
  {
    ERRORLOG(error,_T("While starting HTTP stream"));
    CancelRequest();
  }
  else
  {
    // Simply reset this request for writing
    ResetOutstanding(m_writing);
  }
}

// Send a response stream buffer.
void
HTTPRequest::SendResponseStream(BYTE*    p_buffer
                               ,size_t   p_length
                               ,bool     p_continue /*=true*/)
{
  TRACE0("Send Response stream\n");

  // Check server pointer
  if(m_server == nullptr)
  {
    SvcReportErrorEvent(0,false,_T(__FUNCTION__),_T("FATAL ERROR: No server when sending HTTP response stream!"));
    return;
  }

  // Now begin writing our response body parts
  ResetOutstanding(m_writing);
  m_writing.m_action = IO_WriteStream;

  // Prepare send buffer
  memset(&m_sendChunk,0,sizeof(HTTP_DATA_CHUNK));
  PHTTP_DATA_CHUNK chunks = &m_sendChunk;

  // Default is that we continue sending
  ULONG flags = p_continue ? HTTP_SEND_RESPONSE_FLAG_MORE_DATA : HTTP_SEND_RESPONSE_FLAG_DISCONNECT;

  // Set the send buffer in UTF-8 format
  if(m_sendBuffer)
  {
    delete[] m_sendBuffer;
  }
  m_sendBuffer = new BYTE[p_length + 1];
  memcpy_s(m_sendBuffer,p_length + 1,p_buffer,p_length);
  m_sendBuffer[p_length] = 0;

  // Setup as a data-chunk info structure
  m_sendChunk.DataChunkType           = HttpDataChunkFromMemory;
  m_sendChunk.FromMemory.pBuffer      = m_sendBuffer;
  m_sendChunk.FromMemory.BufferLength = (ULONG) p_length;

  DETAILLOGV(_T("Stream part [%d] bytes to send"),p_length);
  if(m_server)
  {
    USHORT chunkcount = 1;
    ULONG bytes = 0;
    m_server->LogTraceResponse(nullptr,m_sendBuffer,(int) p_length);
    ULONG result = HttpSendResponseEntityBody(m_server->GetRequestQueue(),
                                              m_requestID,    // Our request
                                              flags,          // More/Last data
                                              chunkcount,     // Entity Chunk Count.
                                              chunks,         // CHUNCK
                                              &bytes,         // Bytes
                                              nullptr,        // Reserved1
                                              0,              // Reserved2
                                              nullptr,        // OVERLAPPED. Was: &m_writing
                                              nullptr);       // LOGDATA

    // Check for error
//     if(result != ERROR_IO_PENDING && result != NO_ERROR)
//     {
//       ERRORLOG(result,_T("Sending stream part"));
//       m_responding = false;
//       CancelRequest();
//       return;
//     }
//     else
//     {
//       // Final closing of the connection
//       if(p_continue == false)
//       {
//         DETAILLOG1(_T("Stream connection closed"));
//       }
//     }
    if(result != NO_ERROR)
    {
      ERRORLOG(result,_T("While sending HTTP stream part"));
      CancelRequest();
    }
    else
    {
      // Free the last send buffer and continue to send
      if(m_sendBuffer)
      {
        delete[] m_sendBuffer;
        m_sendBuffer = nullptr;
      }
    }
    if(!p_continue)
    {
      Finalize();
    }
  }
  // Still responding or done?
  m_responding = p_continue;
}

// Only come her if OVERLAPPED parameter in "SendResponseStream" was used !!
// Currently not used!
void
HTTPRequest::SendStreamPart()
{
  TRACE0("Send stream part\n");

  // Check status of the OVERLAPPED structure
  DWORD error = m_writing.Internal & 0x0FFFF;
  if(error)
  {
    ERRORLOG(error,_T("While sending HTTP stream part"));
    CancelRequest();
  }
  else
  {
    // Free the last send buffer and continue to send
    if(m_sendBuffer)
    {
      delete[] m_sendBuffer;
      m_sendBuffer = nullptr;
    }
  }
  if(!m_responding)
  {
    Finalize();
  }
}

//////////////////////////////////////////////////////////////////////////
//
// SUB FUNCTIONS FOR RECEIVING AND SENDING
//
//////////////////////////////////////////////////////////////////////////

// making the request ready for the next round!
void
HTTPRequest::Finalize()
{
  TRACE0("Finalize\n");

  AutoCritSec lock(&m_critical);

  // We might end up here sometimes twice, as a WebSocket can have 2 (TWO!) 
  // outstanding I/O requests (reading and writing). And a 'Cancel' on both
  // might trigger the finalize twice
  if(m_requestID == NULL)
  {
    return;
  }

  // Reset th request id 
  // Retain the request ID for cancellation purposes!!
  // HTTP_SET_NULL_ID(&m_requestID);

  // Free the request memory
  if(m_request)
  {
    RtlZeroMemory(m_request,INIT_HTTP_BUFFERSIZE);
  }

  // Free the response memory
  if(m_response)
  {
    RtlZeroMemory(m_response,sizeof(HTTP_RESPONSE));
  }

  // free the read buffer
  if(m_readBuffer)
  {
    RtlZeroMemory(m_readBuffer,INIT_HTTP_BUFFERSIZE);
  }

  // free the send buffer
  if(m_sendBuffer)
  {
    delete[] m_sendBuffer;
    m_sendBuffer = nullptr;
  }

  // Free the sending buffer
  RtlZeroMemory(&m_sendChunk,sizeof(HTTP_DATA_CHUNK));

  // Remove unknown headers
  if(m_unknown)
  {
    free(m_unknown);
    m_unknown = nullptr;
  }

  // Free the message
  if(m_message)
  {
    if(m_message->DropReference())
    {
      m_message = nullptr;
    }
  }

  // Remove header strings
  for(auto& str : m_strings)
  {
    delete[] str;
  }
  m_strings.clear();

  // Reset parameters
  m_site       = nullptr;
  m_responding = false;
  m_file       = nullptr;
  m_bufferpart = 0;

  // Trigger next request for threadpool
  m_active = false;
}

// Add a request string for a header
void 
HTTPRequest::AddRequestString(XString p_string,LPSTR& p_buffer,USHORT& p_size)
{
  AutoCSTR str(p_string);
  int size = str.size();
  p_buffer = new char[size + 1];
  strncpy_s(p_buffer,size+1,str.cstr(),size);
  m_strings.push_back(p_buffer);
  p_size = (USHORT) size;
}

// Add a well known HTTP header to the response structure
void
HTTPRequest::AddKnownHeader(HTTP_HEADER_ID p_header,LPCTSTR p_value)
{
  LPSTR  str  = nullptr;
  USHORT size = 0;

  AddRequestString(p_value,str,size);
  m_response->Headers.KnownHeaders[p_header].pRawValue      = str;
  m_response->Headers.KnownHeaders[p_header].RawValueLength = size;
}

void
HTTPRequest::AddUnknownHeaders(UKHeaders& p_headers)
{
  TRACE0("Add Unknown Headers\n");

  // Something to do?
  if(p_headers.empty())
  {
    return;
  }
  // Allocate some space
  m_unknown = (PHTTP_UNKNOWN_HEADER) malloc((1 + p_headers.size()) * sizeof(HTTP_UNKNOWN_HEADER));
  if(!m_unknown)
  {
    return;
  }
  unsigned ind = 0;
  for(auto& header : p_headers)
  {
    LPSTR  buffer = nullptr;
    USHORT size   = 0;

    AddRequestString(header.m_name,buffer,size);
    m_unknown[ind].NameLength = size;
    m_unknown[ind].pName      = buffer;

    AddRequestString(header.m_value,buffer,size);
    m_unknown[ind].RawValueLength = size;
    m_unknown[ind].pRawValue      = buffer;

    // next header
    ++ind;
  }
}

void
HTTPRequest::FillResponse(int p_status,bool p_responseOnly /*=false*/)
{
  TRACE0("Fill Response\n");

  // Check site pointer
  if(m_site == nullptr)
  {
    SvcReportErrorEvent(0,false,_T(__FUNCTION__),_T("FATAL ERROR: No site when filling in HTTP response!"));
    return;
  }

  // Initialize the response body
  if(m_response == nullptr)
  {
    m_response = new HTTP_RESPONSE();
  }
  RtlZeroMemory(m_response,sizeof(HTTP_RESPONSE));
  XString text = GetHTTPStatusText(p_status);

  PSTR  stext = nullptr;
  USHORT size = 0;
  AddRequestString(text,stext,size);

  m_response->Version.MajorVersion = 1;
  m_response->Version.MinorVersion = 1;
  m_response->StatusCode   = (USHORT)p_status;
  m_response->pReason      = stext;
  m_response->ReasonLength = size;

  // See if we are done (for event and socket streams)
  if(p_responseOnly)
  {
    return;
  }

  // Add content type as a known header. (octet-stream or the message content type)
  if(p_status != HTTP_STATUS_SWITCH_PROTOCOLS)
  {
    XString contentType(_T("application/octet-stream"));
    if(!m_message->GetContentType().IsEmpty())
    {
      contentType = m_message->GetContentType();
    }
    else
    {
      XString cttype = m_message->GetHeader(_T("Content-type"));
      if(!cttype.IsEmpty())
      {
        contentType = cttype;
      }
    }
    m_message->DelHeader(_T("Content-Type"));
    AddKnownHeader(HttpHeaderContentType, contentType);
  }

  // In case of a 401, we challenge to the client to identify itself
  if(m_message->GetStatus() == HTTP_STATUS_DENIED)
  {
    if(!m_message->GetXMLHttpRequest())
    {
      // See if the message already has an authentication scheme header
      XString challenge = m_message->GetHeader(_T("AuthenticationScheme"));
      if(challenge.IsEmpty())
      {
        // Add authentication scheme
        HTTPSite* site = m_message->GetHTTPSite();
        challenge = m_server->BuildAuthenticationChallenge(site->GetAuthenticationScheme()
                                                          ,site->GetAuthenticationRealm());
        AddKnownHeader(HttpHeaderWwwAuthenticate,challenge);
      }
    }
    XString date = HTTPGetSystemTime();
    AddKnownHeader(HttpHeaderDate,date);
  }

  // Add the server header or suppress it
  switch(m_server->GetSendServerHeader())
  {
    case SendHeader::HTTP_SH_MICROSOFT:   // Do nothing, Microsoft will add the server header
                                          break;
    case SendHeader::HTTP_SH_MARLIN:      AddKnownHeader(HttpHeaderServer,_T(MARLIN_SERVER_VERSION));
                                          break;
    case SendHeader::HTTP_SH_APPLICATION: AddKnownHeader(HttpHeaderServer,m_server->GetName());
                                          break;
    case SendHeader::HTTP_SH_WEBCONFIG:   AddKnownHeader(HttpHeaderServer,m_server->GetConfiguredName());
                                          break;
    case SendHeader::HTTP_SH_HIDESERVER:  // Fill header with empty string will suppress it
                                          AddKnownHeader(HttpHeaderServer,_T(""));
                                          break;
  }
  m_message->DelHeader(_T("Server"));

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
  cookiesHasSecure  = m_site->GetCookieHasSecure();
  cookiesHasHttp    = m_site->GetCookieHasHttpOnly();
  cookiesHasSame    = m_site->GetCookieHasSameSite();
  cookiesHasPath    = m_site->GetCookieHasPath();
  cookiesHasDomain  = m_site->GetCookieHasDomain();
  cookiesHasExpires = m_site->GetCookieHasExpires();
  cookiesHasMaxAge  = m_site->GetCookieHasMaxAge();

  cookieSecure      = m_site->GetCookiesSecure();
  cookieHttpOnly    = m_site->GetCookiesHttpOnly();
  cookieSameSite    = m_site->GetCookiesSameSite();
  cookiePath        = m_site->GetCookiesPath();
  cookieDomain      = m_site->GetCookiesDomain();
  cookieExpires     = m_site->GetCookiesExpires();
  cookieMaxAge      = m_site->GetCookiesMaxAge();

  // Add cookies to the unknown response headers
  // Because we can have more than one Set-Cookie: header
  // and HTTP API just supports one set-cookie.
  UKHeaders ukheaders;
  Cookies& cookies = m_message->GetCookies();
  if(cookies.GetCookies().empty())
  {
    XString cookie = m_message->GetHeader(_T("Set-Cookie"));
    if(!cookie.IsEmpty())
    {
      AddKnownHeader(HttpHeaderSetCookie,cookie);
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
  m_message->DelHeader(_T("Set-Cookie"));

  // Add extra headers from the message, except for content-length
  m_message->DelHeader(_T("Content-Length"));

  if(p_status == HTTP_STATUS_SWITCH_PROTOCOLS)
  {
    FillResponseWebSocketHeaders(ukheaders);
  }
  else
  {
    for(auto& header : *m_message->GetHeaderMap())
    {
      ukheaders.push_back(UKHeader(header.first, header.second));
    }
  }

  // Add other optional security headers like CORS etc.
  m_site->AddSiteOptionalHeaders(ukheaders);

  // Possible zip the contents, and add content-encoding header
  FileBuffer* buffer = m_message->GetFileBuffer();
  if(m_site->GetHTTPCompression() && buffer)
  {
    // But only if the client side requested it
    if(m_message->GetAcceptEncoding().Find(_T("gzip")) >= 0)
    {
      if(buffer->ZipBuffer())
      {
        DETAILLOGV(_T("GZIP the buffer to size: %lu"),buffer->GetLength());
        ukheaders.push_back(UKHeader(_T("Content-Encoding"),_T("gzip")));
      }
    }
  }

  // Now add all unknown headers to the response
  AddUnknownHeaders(ukheaders);
  m_response->Headers.UnknownHeaderCount = (USHORT)ukheaders.size();
  m_response->Headers.pUnknownHeaders    = m_unknown;

  // See if this request must send a file as part of a 'GET'
  // Or if the content comes from the buffer
  size_t totalLength = 0;
  if(buffer && !buffer->GetFileName().IsEmpty())
  {
    // FILE BUFFER CONTAINS A FILE REFERENCE TO SENT
    // Send file in form of a file-handle to transmit
    if(buffer->OpenFile())
    {
      // OK: Get the file handle from the buffer
      m_file = buffer->GetFileHandle();

      // Getting the length even above 4GB
      DWORD  high  = 0L;
      totalLength  = GetFileSize(m_file,&high);
      totalLength += ((UINT64)high) << 32;
    }
    else
    {
      ERRORLOG(GetLastError(),_T("OpenFile for sending a HTTP 'GET' response"));
      m_response->StatusCode   = HTTP_STATUS_NOT_FOUND;
      m_response->pReason      = "File not found";
      m_response->ReasonLength = (USHORT) strlen(m_response->pReason);
    }
  }
  else
  {
    // Otherwise the content length comes from the buffer memory parts
    totalLength = buffer ? buffer->GetLength() : 0;
  }

  if(p_status != HTTP_STATUS_SWITCH_PROTOCOLS)
  {
    // Now after the compression, and after the calculation of the file length
    // add the total content length in the form of the content-length header
    XString contentLength;
#ifdef _WIN64
    contentLength.Format(_T("%I64u"),totalLength);
#else
    contentLength.Format(_T("%lu"),totalLength);
#endif
    AddKnownHeader(HttpHeaderContentLength,contentLength);
  }
}

void
HTTPRequest::FillResponseWebSocketHeaders(UKHeaders& p_headers)
{
  USHORT size = 0;
  PSTR   text = nullptr;

  XString orgstring = m_message->GetHeader(_T("Connection"));
  AddRequestString(orgstring,text,size);
  m_response->Headers.KnownHeaders[HttpHeaderConnection].pRawValue      = text;
  m_response->Headers.KnownHeaders[HttpHeaderConnection].RawValueLength = size;

  orgstring = m_message->GetHeader(_T("Upgrade"));
  AddRequestString(orgstring,text,size);
  m_response->Headers.KnownHeaders[HttpHeaderUpgrade].pRawValue         = text;
  m_response->Headers.KnownHeaders[HttpHeaderUpgrade].RawValueLength    = size;

  for(auto& header : *m_message->GetHeaderMap())
  {
    if(header.first.CompareNoCase(_T("Sec-WebSocket-Accept")) == 0)
    {
      p_headers.push_back(UKHeader(header.first, header.second));
    }
  }
}

// Reset outstanding OVERLAPPED
void 
HTTPRequest::ResetOutstanding(OutstandingIO& p_outstanding)
{
  TRACE0("Reset Outstanding overlapped I/O\n");

  p_outstanding.Internal     = 0;
  p_outstanding.InternalHigh = 0;
  p_outstanding.Pointer      = nullptr;
}

// Cancel the request at the HTTP driver
void
HTTPRequest::CancelRequest()
{
  TRACE0("Cancel Request\n");

  if(m_requestID)
  {
    // IO_Cancel will restart a new request for this object
    m_incoming.m_action = IO_Cancel;

    // Request cancellation
    ULONG result = HttpCancelHttpRequest(m_server->GetRequestQueue(),m_requestID,&m_incoming);
    if(result == NO_ERROR)
    {
      DETAILLOG1(_T("Event stream connection closed"));
    }
    else if(result != ERROR_IO_PENDING && 
            result != ERROR_CONNECTION_INVALID)
    {
      ERRORLOG(result,_T("Event stream incorrectly canceled"));
    }
  }
  else
  {
    Finalize();
  }
}
