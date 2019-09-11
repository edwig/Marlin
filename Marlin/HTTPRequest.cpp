/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: HTTPRequest.cpp
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
#include <WebSocket.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define DETAILLOG1(text)          if(MUSTLOG(HLL_LOGGING) && m_server) { m_server->DetailLog (__FUNCTION__,LogType::LOG_INFO,text); }
#define DETAILLOGS(text,extra)    if(MUSTLOG(HLL_LOGGING) && m_server) { m_server->DetailLogS(__FUNCTION__,LogType::LOG_INFO,text,extra); }
#define DETAILLOGV(text,...)      if(MUSTLOG(HLL_LOGGING) && m_server) { m_server->DetailLogV(__FUNCTION__,LogType::LOG_INFO,text,__VA_ARGS__); }
#define WARNINGLOG(text,...)      if(MUSTLOG(HLL_LOGGING) && m_server) { m_server->DetailLogV(__FUNCTION__,LogType::LOG_WARN,text,__VA_ARGS__); }
#define ERRORLOG(code,text)       if(MUSTLOG(HLL_ERRORS)  && m_server) { m_server->ErrorLog  (__FUNCTION__,(code),(text)); }

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

  InitializeCriticalSection(&m_critical);
}

// DTOR
HTTPRequest::~HTTPRequest()
{
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
    free(m_sendBuffer);
    m_sendBuffer = nullptr;
  }
  if(m_unknown)
  {
    free(m_unknown);
    m_unknown = nullptr;
  }
  m_strings.clear();
}

// Setting a WebSocket
void 
HTTPRequest::RegisterWebSocket(WebSocketServer* p_socket)
{
  if(!m_socket)
  {
    // Remember our socket
    m_socket = p_socket;

    // Get it started with a read request
    StartSocketReceiveRequest();
  }
}

// Callback from I/O Completion port right out of the threadpool
/*static*/ void
HandleAsynchroneousIO(OVERLAPPED* p_overlapped)
{
  OutstandingIO* outstanding = reinterpret_cast<OutstandingIO*>(p_overlapped);
  HTTPRequest* request = outstanding->m_request;
  if(request)
  {
    request->HandleAsynchroneousIO(outstanding->m_action);
  }
}

// Callback from I/O Completion port 
void
HTTPRequest::HandleAsynchroneousIO(IOAction p_action)
{
  switch(p_action)
  {
    case IO_Nothing:    break;
    case IO_Request:    ReceivedRequest();  break;
    case IO_Reading:    ReceivedBodyPart(); break;
    case IO_Response:   SendResponseBody(); break;
    case IO_Writing:    SendBodyPart();     break;
    case IO_StartStream:StartedStream();    break;
    case IO_WriteStream:SendStreamPart();   break;
    case IO_ReadSocket: ReceivedWebSocket();break;
    case IO_WriteSocket:SendWebSocket();    break;
    case IO_Cancel:     Finalize();         break;
    default:            ERRORLOG(ERROR_INVALID_PARAMETER,"Unexpected outstanding async I/O");
  }
}

// Start a new request against the server
void 
HTTPRequest::StartRequest()
{
  // Buffer size for a new request
  DWORD size = INIT_HTTP_BUFFERSIZE;

  // Allocate the request object
  if(m_request == nullptr)
  {
    m_request = (PHTTP_REQUEST) calloc(1,size + 1);
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
    ERRORLOG(result,"Starting new HTTP Request");
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
  USES_CONVERSION;
  HANDLE accessToken = nullptr;

  // Get status from the OVERLAPPED result
  DWORD result = (DWORD) m_incoming.Internal & 0x0FFFF;
  // Catch normal abortion statuses
  // Test if server already stopped, and we are here because of the stopping
  if((result == ERROR_CONNECTION_INVALID || result == ERROR_OPERATION_ABORTED) ||
     (!m_server->GetIsRunning()))
  {
    // Server stopped by closing server handles
    DETAILLOG1("HTTP Server stopped in mainloop");
    Finalize();
    return;
  }
  // Catch ABNORMAL abortion status
  if(result != 0)
  {
    ERRORLOG(result,"Error receiving a HTTP request in mainloop");
    Finalize();
    return;
  }

  // Get the primary request-id for the request queue
  m_requestID = m_request->RequestId;

  // Grab the senders content
  CString   acceptTypes     = m_request->Headers.KnownHeaders[HttpHeaderAccept         ].pRawValue;
  CString   contentType     = m_request->Headers.KnownHeaders[HttpHeaderContentType    ].pRawValue;
  CString   contentLength   = m_request->Headers.KnownHeaders[HttpHeaderContentLength  ].pRawValue;
  CString   acceptEncoding  = m_request->Headers.KnownHeaders[HttpHeaderAcceptEncoding ].pRawValue;
  CString   cookie          = m_request->Headers.KnownHeaders[HttpHeaderCookie         ].pRawValue;
  CString   authorize       = m_request->Headers.KnownHeaders[HttpHeaderAuthorization  ].pRawValue;
  CString   modified        = m_request->Headers.KnownHeaders[HttpHeaderIfModifiedSince].pRawValue;
  CString   referrer        = m_request->Headers.KnownHeaders[HttpHeaderReferer        ].pRawValue;
  CString   rawUrl          = CW2A(m_request->CookedUrl.pFullUrl);
  PSOCKADDR sender          = m_request->Address.pRemoteAddress;
  PSOCKADDR receiver        = m_request->Address.pLocalAddress;
  int       remDesktop      = m_server->FindRemoteDesktop(m_request->Headers.UnknownHeaderCount
                                                         ,m_request->Headers.pUnknownHeaders);

  // If positive request ID received
  if(m_requestID)
  {
    // Log earliest as possible
    DETAILLOGV("Received HTTP call from [%s] with length: %I64u"
               ,SocketToServer((PSOCKADDR_IN6) sender).GetString()
               ,m_request->BytesReceived);

    // Log incoming request
    DETAILLOGS("Got a request for: ",rawUrl);

    // Trace the request in full
    m_server->LogTraceRequest(m_request,nullptr);
  }

  // Recording expected content length
  m_expect = atol(contentLength);

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
    CString absPath = CW2A(m_request->CookedUrl.pAbsPath);
    m_site = m_server->FindHTTPSite(m_site,absPath);
  }

  // Now check for authentication and possible send 401 back
  if(m_server->CheckAuthentication(m_request,(HTTP_OPAQUE_ID)this,m_site,rawUrl,authorize,accessToken) == false)
  {
    // Not authenticated, go back for next request
    return;
  }

  // Remember the context: easy in API 2.0
  if(callback == nullptr && m_site == nullptr)
  {
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
  if((type == HTTPCommand::http_get) && (eventStream || acceptTypes.Left(17).CompareNoCase("text/event-stream") == 0))
  {
    CString absolutePath = CW2A(m_request->CookedUrl.pAbsPath);
    EventStream* stream = m_server->SubscribeEventStream((PSOCKADDR_IN6) sender
                                                         ,remDesktop
                                                         ,m_site
                                                         ,m_site->GetSite()
                                                         ,absolutePath
                                                         ,(HTTP_OPAQUE_ID) this
                                                         ,accessToken);
    if(stream)
    {
      stream->m_baseURL = rawUrl;
      m_site->HandleEventStream(stream);
      return;
    }
  }

  // For all types of requests: Create the HTTPMessage
  m_message = new HTTPMessage(type,m_site);
  m_message->SetRequestHandle((HTTP_OPAQUE_ID)this);
  m_message->AddReference();
  // Enter our primary information from the request
  m_message->SetURL(rawUrl);
  m_message->SetReferrer(referrer);
  m_message->SetAuthorization(authorize);
  m_message->SetConnectionID(m_request->ConnectionId);
  m_message->SetContentType(contentType);
  m_message->SetContentLength((size_t)atoll(contentLength));
  m_message->SetAccessToken(accessToken);
  m_message->SetRemoteDesktop(remDesktop);
  m_message->SetSender  ((PSOCKADDR_IN6)sender);
  m_message->SetReceiver((PSOCKADDR_IN6)receiver);
  m_message->SetCookiePairs(cookie);
  m_message->SetAcceptEncoding(acceptEncoding);
  if(m_site->GetAllHeaders())
  {
    // If requested so, copy all headers to the message
    m_message->SetAllHeaders(&m_request->Headers);
  }
  else
  {
    // As a minimum, always add the unknown headers
    // in case of a 'POST', as the SOAPAction header is here too!
    m_message->SetUnknownHeaders(&m_request->Headers);
  }

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
      DETAILLOGV("Request VERB changed to: %s",m_message->GetVerb().GetString());
    }
  }

  // Remember the fact that we should read the rest of the message
  if(m_request->Flags & HTTP_REQUEST_FLAG_MORE_ENTITY_BODY_EXISTS)
  {
    // Read the body of the message, before we handle it
    StartReceiveRequest();
  }
  else
  {
    // Go straight on to the handling of the message
    m_site->HandleHTTPMessage(m_message);
  }
}

void 
HTTPRequest::StartReceiveRequest()
{
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
    ERRORLOG(result,"Error receiving HTTP request body");
    Finalize();
  }
}

// 2) Receive trailing request body
void 
HTTPRequest::ReceivedBodyPart()
{
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
    ERRORLOG(result,"Error receiving bodypart");
    m_server->RespondWithServerError(m_message,HTTP_STATUS_SERVER_ERROR,"Server error");
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
  // Now also trace the request body of the message
  m_server->LogTraceRequestBody(m_message->GetFileBuffer());

  // In case of a POST, try to convert character set before submitting to site
  if(m_message->GetCommand() == HTTPCommand::http_post)
  {
    if(m_message->GetContentType().Find("multipart") <= 0)
    {
      m_server->HandleTextContent(m_message);
    }
  }
  DETAILLOGV("Received %s message from: %s Size: %lu"
             ,headers[(unsigned)m_message->GetCommand()]
             ,(LPCTSTR)SocketToServer(m_message->GetSender())
             ,m_message->GetBodyLength());

  // Message is now complete
  // Go straight on to the handling of the message
  m_site->HandleHTTPMessage(m_message);
}

void
HTTPRequest::StartSendResponse()
{
  // Mark the fact that we begin responding
  m_responding = true;
  // Make sure we have the correct event action
  ResetOutstanding(m_writing);
  m_writing.m_action = IO_Response;

  // We promise to always call HttpSendResponseEntityBody
  ULONG flags = HTTP_SEND_RESPONSE_FLAG_MORE_DATA;

  // Place HTTPMessage in the response structure
  FillResponse(m_message->GetStatus());

  // Special case for WebSockets protocol
  if(m_response->StatusCode == HTTP_STATUS_SWITCH_PROTOCOLS)
  {
    m_writing.m_action = IO_Nothing;
    flags  |= HTTP_SEND_RESPONSE_FLAG_OPAQUE;
  }

  // Prepare our cache-policy
  m_policy.Policy        = m_server->GetCachePolicy();
  m_policy.SecondsToLive = m_server->GetCacheSecondsToLive();

  // Trace the principal response, before sending
  // Sometimes the async is so quick, we cannot trace it after the sending
  m_server->LogTraceResponse(m_response,nullptr);

  // Send the response
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

  // Check for error
  if(result != ERROR_IO_PENDING && result != NO_ERROR)
  {
    ERRORLOG(result,"Sending HTTP Response");
    Finalize();
  }
}

void
HTTPRequest::SendResponseBody()
{
  DWORD error = m_writing.Internal & 0x0FFFF;
  if(error)
  {
    ERRORLOG(error,"Error sending HTTP headers");
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
    ERRORLOG(result,"Sending response body part");
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
  // Check status of the OVERLAPPED structure
  DWORD error = m_writing.Internal & 0x0FFFF;
  if(error)
  {
    ERRORLOG(error,"While sending HTTP response part");
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
      // See if we need to send another chunk
      if(filebuf->GetHasBufferParts() &&
        (m_bufferpart < filebuf->GetNumberOfParts()))
      {
        SendResponseBody();
        return;
      }
    }
    // Possibly log and trace what we just sent
    m_server->LogTraceResponse(nullptr,filebuf);
  }

  // Message is done. Break the connection with the HTTPRequest
  m_message->SetHasBeenAnswered();

  FlushFileBuffers(m_server->GetRequestQueue());

  // End of the line for the whole request
  // We did send everything as an answer
  Finalize();
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
  // First comment to push to the stream (not an event!)
  CString init = m_server->GetEventBOM() ? ConstructBOM() : "";
  init += ":init event-stream\n";

  // Initialize the HTTP response structure.
  FillResponse(HTTP_STATUS_OK,true);

  // Add a known header.
  AddKnownHeader(HttpHeaderContentType,"text/event-stream");

  // Set init in the send buffer
  if(m_sendBuffer)
  {
    free(m_sendBuffer);
  }
  m_sendBuffer = (BYTE*) malloc(init.GetLength() + 1);
  memcpy_s(m_sendBuffer,init.GetLength() + 1,init.GetString(),init.GetLength() + 1);

  // Setup as a data-chunk info structure
  memset(&m_sendChunk,0,sizeof(HTTP_DATA_CHUNK));
  m_sendChunk.DataChunkType           = HttpDataChunkFromMemory;
  m_sendChunk.FromMemory.pBuffer      = m_sendBuffer;
  m_sendChunk.FromMemory.BufferLength = (ULONG)init.GetLength();

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

  DETAILLOGV("HTTP Response %d %s",m_response->StatusCode,m_response->pReason);

  // Check for error
  if(result != ERROR_IO_PENDING && result != NO_ERROR)
  {
    ERRORLOG(result,"Sending HTTP Response for event stream");
    Finalize();
  }

  // Log&Trace what we just send
  m_server->LogTraceResponse(m_response,m_sendBuffer,init.GetLength());
}

void
HTTPRequest::StartedStream()
{
  // Check status of the OVERLAPPED structure
  DWORD error = m_writing.Internal & 0x0FFFF;
  if(error)
  {
    ERRORLOG(error,"While starting HTTP stream");
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
HTTPRequest::SendResponseStream(const char* p_buffer
                               ,size_t      p_length
                               ,bool        p_continue /*=true*/)
{
  // Now begin writing our response body parts
  ResetOutstanding(m_writing);
  m_writing.m_action = IO_WriteStream;

  // Prepare send buffer
  memset(&m_sendChunk,0,sizeof(HTTP_DATA_CHUNK));
  PHTTP_DATA_CHUNK chunks = &m_sendChunk;
  USHORT chunkcount = 1;

  // Default is that we continue sending
  ULONG flags = p_continue ? HTTP_SEND_RESPONSE_FLAG_MORE_DATA : HTTP_SEND_RESPONSE_FLAG_DISCONNECT;

  // Set the send buffer
  if(m_sendBuffer)
  {
    free(m_sendBuffer);
  }
  m_sendBuffer = (BYTE*) malloc(p_length + 1);
  memcpy_s(m_sendBuffer,p_length + 1,p_buffer,p_length);
  m_sendBuffer[p_length] = 0;

  // Setup as a data-chunk info structure
  m_sendChunk.DataChunkType           = HttpDataChunkFromMemory;
  m_sendChunk.FromMemory.pBuffer      = m_sendBuffer;
  m_sendChunk.FromMemory.BufferLength = (ULONG)p_length;

  DETAILLOGV("Stream part [%d] bytes to send",p_length);
  m_server->LogTraceResponse(nullptr,m_sendBuffer,(int) p_length);

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
    ERRORLOG(result,"Sending stream part");
    m_responding = false;
    CancelRequest();
    return;
  }
  else
  {
    // Final closing of the connection
    if(p_continue == false)
    {
      DETAILLOG1("Stream connection closed");
    }
  }

  // Still responding or done?
  m_responding = p_continue;
}

void
HTTPRequest::SendStreamPart()
{
  // Check status of the OVERLAPPED structure
  DWORD error = m_writing.Internal & 0x0FFFF;
  if(error)
  {
    ERRORLOG(error,"While sending HTTP stream part");
    CancelRequest();
  }
  else
  {
    // Free the last send buffer and continue to send
    if(m_sendBuffer)
    {
      free(m_sendBuffer);
      m_sendBuffer = nullptr;
    }
  }
}

//////////////////////////////////////////////////////////////////////////
//
// READING AND WRITING for WEBSOCKET
//
//////////////////////////////////////////////////////////////////////////

void 
HTTPRequest::StartSocketReceiveRequest()
{
  ULONG size = m_socket->GetFragmentSize() + WS_OVERHEAD;

  // Make sure we have a buffer
  if(!m_readBuffer)
  {
    m_readBuffer = new BYTE[size];
  }
  // Set reading action
  ResetOutstanding(m_reading);
  m_reading.m_action = IO_ReadSocket;

  // Use these flags while reading the request body
  ULONG flags = 0;

  // Issue the asynchronous read request
  DWORD result = HttpReceiveRequestEntityBody(m_server->GetRequestQueue()
                                             ,m_requestID
                                             ,flags
                                             ,m_readBuffer
                                             ,size
                                             ,NULL
                                             ,&m_reading);
  if(result != ERROR_IO_PENDING && result != NO_ERROR)
  {
    m_socket->CloseForReading();
    ERRORLOG(result,"Error receiving WebSocket request stream");
    if(!m_socket->IsOpenForWriting())
    {
      Finalize();
    }
  }
}

void
HTTPRequest::ReceivedWebSocket()
{
  DWORD result = (DWORD)(m_reading.Internal & 0x0FFFF);
  if(result == ERROR_HANDLE_EOF || result == NO_ERROR)
  {
    // Store the result of the read action
    DWORD bytes = (DWORD)m_reading.InternalHigh;
    if(bytes)
    {
      m_readBuffer[bytes] = 0;

      // Transfer buffer to socket 
      RawFrame* frame = new RawFrame();
      frame->m_data   = m_readBuffer;
      m_readBuffer    = nullptr;

      // Decode the frame buffer
      if(m_socket->DecodeFrameBuffer(frame,bytes))
      {
        if(!m_socket->StoreFrameBuffer(frame))
        {
          // Closing of the read channel
        }
      }
      else
      {
        // Error on decoding the incoming frame
      }
    }
    if(result == NO_ERROR)
    {
      // (Re)start the next read request for the socket
      StartSocketReceiveRequest();
    }
  }
  else
  {
    m_socket->CloseForReading();
    ERRORLOG(result,"Error while receiving WebSocket");
    if(!m_socket->IsOpenForWriting())
    {
      Finalize();
    }
  }
}

// Flush the WebSocket stream
// Re-start the writing process on the socket stream
void 
HTTPRequest::FlushWebSocketStream()
{
  // See if there is more in the send queue of the socket
  WSFrame* frame = m_socket->GetFrameToWrite();
  if(frame == nullptr)
  {
    return;
  }

  // Set writing action
  ResetOutstanding(m_writing);
  m_writing.m_action = IO_WriteSocket;

  // Send more data, or this was the last in the channel
  ULONG flags = frame->m_final ? HTTP_SEND_RESPONSE_FLAG_DISCONNECT :
                                 HTTP_SEND_RESPONSE_FLAG_MORE_DATA  ;

  // Remember our send buffer
  m_sendBuffer = frame->m_data;

  // Transfer data to the send chunk
  USHORT chunkcount = 1;
  m_sendChunk.DataChunkType           = HttpDataChunkFromMemory;
  m_sendChunk.FromMemory.BufferLength = (ULONG)frame->m_length;
  m_sendChunk.FromMemory.pBuffer      = m_sendBuffer;

  // Free the WSFrame buffer, data is now held by the sendBuffer
  frame->m_data = nullptr;
  delete frame;

  // Write the frame 
  ULONG result = HttpSendResponseEntityBody(m_server->GetRequestQueue(),
                                            m_requestID,    // Our request
                                            flags,          // More/Last data
                                            chunkcount,     // Entity Chunk Count.
                                            &m_sendChunk,   // CHUNCK
                                            nullptr,        // Bytes
                                            nullptr,        // Reserved1
                                            0,              // Reserved2
                                            &m_writing,     // OVERLAPPED
                                            nullptr);       // LOGDATA
  // Check for error
  if(result != ERROR_IO_PENDING && result != NO_ERROR)
  {
    m_socket->CloseForWriting();
    ERRORLOG(result,"Writing to the WebSocket stream");
    if(!m_socket->IsOpenForReading())
    {
      Finalize();
    }
  }
}

void
HTTPRequest::SendWebSocket()
{
  // After sending to a WebSocket, we must free the RawFrame data member
  if(m_sendBuffer)
  {
    free(m_sendBuffer);
    m_sendBuffer = nullptr;
  }

  // Get the status word
  DWORD result = (DWORD)(m_writing.Internal & 0x0FFFF);
  if(result == ERROR_HANDLE_EOF || result == NO_ERROR)
  {
    // Log correct writing to the WebSocket

    // Re-issue a write command from the queue?
    FlushWebSocketStream();
  }
  else
  {
    m_socket->CloseForWriting();
    ERRORLOG(result,"Error while writing to WebSocket");
    if(!m_socket->IsOpenForReading())
    {
      Finalize();
    }
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
  AutoCritSec lock(&m_critical);

  // We might end up here sometimes twice, as a WebSocket can have 2 (TWO!) 
  // outstanding I/O requests (reading and writing). And a 'Cancel' on both
  // might trigger the finalize twice
  if(m_requestID == NULL)
  {
    return;
  }

  // Reset th request id 
  HTTP_SET_NULL_ID(&m_requestID);

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
    free(m_sendBuffer);
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
    m_message->DropReference();
    m_message = nullptr;
  }

  // Remove header strings
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
HTTPRequest::AddRequestString(CString p_string,const char*& p_buffer,USHORT& p_size)
{
  m_strings.push_back(p_string);
  CString& string = m_strings.back();
  p_buffer = string.GetString();
  p_size   = (USHORT) string.GetLength();
}

// Add a well known HTTP header to the response structure
void
HTTPRequest::AddKnownHeader(HTTP_HEADER_ID p_header,const char* p_value)
{
  const char* str = nullptr;
  USHORT size = 0;

  AddRequestString(p_value,str,size);
  m_response->Headers.KnownHeaders[p_header].pRawValue      = str;
  m_response->Headers.KnownHeaders[p_header].RawValueLength = (USHORT)size;
}

void
HTTPRequest::AddUnknownHeaders(UKHeaders& p_headers)
{
  // Something to do?
  if(p_headers.empty())
  {
    return;
  }
  // Allocate some space
  m_unknown = (PHTTP_UNKNOWN_HEADER) malloc((1 + p_headers.size()) * sizeof(HTTP_UNKNOWN_HEADER));

  unsigned ind = 0;
  for(auto& unknown : p_headers)
  {
    const char* string = nullptr;
    USHORT size = 0;

    CString name = unknown.first;
    AddRequestString(name,string,size);
    m_unknown[ind].NameLength = size;
    m_unknown[ind].pName = string;


    CString value = unknown.second;
    AddRequestString(value,string,size);
    m_unknown[ind].RawValueLength = size;
    m_unknown[ind].pRawValue = string;

    // next header
    ++ind;
  }
}

void
HTTPRequest::FillResponse(int p_status,bool p_responseOnly /*=false*/)
{
  // Initialize the response body
  if(m_response == nullptr)
  {
    m_response = new HTTP_RESPONSE();
    RtlZeroMemory(m_response,sizeof(HTTP_RESPONSE));
  }
  const char* text = GetHTTPStatusText(p_status);
  CString date = HTTPGetSystemTime();

  m_response->Version.MajorVersion = 1;
  m_response->Version.MinorVersion = 1;
  m_response->StatusCode   = (USHORT)p_status;
  m_response->pReason      = text;
  m_response->ReasonLength = (USHORT)strlen(text);

  // See if we are done (for event and socket streams)
  if(p_responseOnly)
  {
    return;
  }

  // Add content type as a known header. (octet-stream or the message content type)
  if(p_status != HTTP_STATUS_SWITCH_PROTOCOLS)
  {
    CString contentType("application/octet-stream");
    if(!m_message->GetContentType().IsEmpty())
    {
      contentType = m_message->GetContentType();
    }
    AddKnownHeader(HttpHeaderContentType,contentType);
  }

  // In case of a 401, we challenge to the client to identify itself
  if(m_message->GetStatus() == HTTP_STATUS_DENIED)
  {
    // See if the message already has an authentication scheme header
    CString challenge = m_message->GetHeader("AuthenticationScheme");
    if(challenge.IsEmpty())
    {
      // Add authentication scheme
      challenge = m_server->BuildAuthenticationChallenge(m_site->GetAuthenticationScheme()
                                                        ,m_site->GetAuthenticationRealm());
    }
    AddKnownHeader(HttpHeaderWwwAuthenticate,challenge);
    AddKnownHeader(HttpHeaderDate,date);

  }

  // Add the server header or suppress it
  switch(m_server->GetSendServerHeader())
  {
    case SendHeader::HTTP_SH_MICROSOFT:   // Do nothing, Microsoft will add the server header
                                          break;
    case SendHeader::HTTP_SH_MARLIN:      AddKnownHeader(HttpHeaderServer,MARLIN_SERVER_VERSION);
                                          break;
    case SendHeader::HTTP_SH_APPLICATION: AddKnownHeader(HttpHeaderServer,m_server->GetName());
                                          break;
    case SendHeader::HTTP_SH_WEBCONFIG:   AddKnownHeader(HttpHeaderServer,m_server->GetConfiguredName());
                                          break;
    case SendHeader::HTTP_SH_HIDESERVER:  // Fill header with empty string will suppress it
                                          AddKnownHeader(HttpHeaderServer,"");
                                          break;
  }

  // Add cookies to the unknown response headers
  // Because we can have more than one Set-Cookie: header
  // and HTTP API just supports one set-cookie.
  UKHeaders ukheaders;
  Cookies& cookies = m_message->GetCookies();
  for(auto& cookie : cookies.GetCookies())
  {
    ukheaders.insert(std::make_pair("Set-Cookie",cookie.GetSetCookieText()));
  }

  // Add Other unknown headers
  if(m_site)
  {
    m_site->AddSiteOptionalHeaders(ukheaders);
  }

  // Add extra headers from the message
  for(auto& header : *m_message->GetHeaderMap())
  {
    ukheaders.insert(std::make_pair(header.first,header.second));
  }

  // Possible zip the contents, and add content-encoding header
  FileBuffer* buffer = m_message->GetFileBuffer();
  if(m_site->GetHTTPCompression() && buffer)
  {
    // But only if the client side requested it
    if(m_message->GetAcceptEncoding().Find("gzip") >= 0)
    {
      if(buffer->ZipBuffer())
      {
        DETAILLOGV("GZIP the buffer to size: %lu",buffer->GetLength());
        ukheaders.insert(std::make_pair("Content-Encoding","gzip"));
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
      ERRORLOG(GetLastError(),"OpenFile for sending a HTTP 'GET' response");
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
    CString contentLength;
#ifdef _WIN64
    contentLength.Format("%I64u",totalLength);
#else
    contentLength.Format("%lu",totalLength);
#endif
    AddKnownHeader(HttpHeaderContentLength,contentLength);
  }
}

// E.G. for a WebSocket we need the bare response string in a buffer
CString
HTTPRequest::ResponseToString()
{
  CString string;
  string.Format("HTTP/%d.%d %d %s\r\n"
               ,m_response->Version.MajorVersion
               ,m_response->Version.MinorVersion
               ,m_response->StatusCode
               ,m_response->pReason);

  for(USHORT ind = 0; ind < m_response->Headers.UnknownHeaderCount; ++ind)
  {
    string.AppendFormat("%s: %s\r\n"
                       ,m_response->Headers.pUnknownHeaders[ind].pName
                       ,m_response->Headers.pUnknownHeaders[ind].pRawValue);
  }

  string += "\r\n";

  // And we remove the info from the request, to make it a 'bare' request
  delete  [] m_unknown;
  m_unknown = nullptr;
  m_response->Headers.pUnknownHeaders = nullptr;
  m_response->Headers.UnknownHeaderCount = 0;

  return string;
}

// Reset outstanding OVERLAPPED
void 
HTTPRequest::ResetOutstanding(OutstandingIO& p_outstanding)
{
  p_outstanding.Internal     = 0;
  p_outstanding.InternalHigh = 0;
  p_outstanding.Pointer      = nullptr;
}

// Cancel the request at the HTTP driver
void
HTTPRequest::CancelRequest()
{
  if(m_requestID)
  {
    // IO_Cancel will restart a new request for this object
    m_incoming.m_action = IO_Cancel;

    // Request cancellation
    ULONG result = HttpCancelHttpRequest(m_server->GetRequestQueue(),m_requestID,&m_incoming);
    if(result == NO_ERROR)
    {
      DETAILLOG1("Event stream connection closed");
    }
    else if(result != ERROR_IO_PENDING)
    {
      ERRORLOG(result,"Event stream incorrectly canceled");
    }
  }
  else
  {
    Finalize();
  }
}
