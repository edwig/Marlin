/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: HTTPRequest.cpp
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
#include "HTTPRequest.h"
#include "HTTPMessage.h"
#include "HTTPServer.h"
#include "HTTPSite.h"
#include "HTTPError.h"
#include "AutoCritical.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define DETAILLOG1(text)          if(m_logging && m_server) { m_server->DetailLog (__FUNCTION__,LogType::LOG_INFO,text); }
#define DETAILLOGS(text,extra)    if(m_logging && m_server) { m_server->DetailLogS(__FUNCTION__,LogType::LOG_INFO,text,extra); }
#define DETAILLOGV(text,...)      if(m_logging && m_server) { m_server->DetailLogV(__FUNCTION__,LogType::LOG_INFO,text,__VA_ARGS__); }
#define WARNINGLOG(text,...)      if(m_logging && m_server) { m_server->DetailLogV(__FUNCTION__,LogType::LOG_WARN,text,__VA_ARGS__); }
#define ERRORLOG(code,text)       if(m_server) m_server->ErrorLog (__FUNCTION__,(code),(text))

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
  m_logging = m_server->GetDetailedLogging();

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
  if(m_unknown)
  {
    free(m_unknown);
    m_unknown = nullptr;
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
    case IO_Request: TRACE("Receive request\n");
                     ReceivedRequest();  break;
    case IO_Reading: TRACE("Do reading request body\n");
                     ReceivedBodyPart(); break;
    case IO_Response:TRACE("Send response body\n");
                     SendResponseBody(); break;
    case IO_Writing: TRACE("Did write response body part\n");
                     SendBodyPart();     break;
    default:         ERRORLOG(ERROR_INVALID_PARAMETER,"Unexpected outstanding async I/O");
  }
}

// Start a new request against the server
void 
HTTPRequest::StartRequest()
{
  // Buffersize
  DWORD size = INIT_HTTP_BUFFERSIZE;

  // Allocate the request object
  if(m_request == nullptr)
  {
    m_request = (PHTTP_REQUEST) malloc(size + 1);
    RtlZeroMemory(m_request,size);
  }  
  // Set type of request
  m_incoming.m_action = IO_Request;
  // Get queue handle
  HANDLE queue = m_server->GetRequestQueue();

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
      m_message->SetRequestHandle(NULL);
      m_message->DropReference();
    }
    m_message = p_message;
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

  // Log earliest as possible
  DETAILLOGV("Received HTTP call from [%s] with length: %I64u"
            ,SocketToServer((PSOCKADDR_IN6)sender).GetString()
            ,m_request->BytesReceived);

  // Log incoming request
  DETAILLOGS("Got a request for: ",rawUrl);

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
  if(CheckAuthentication(m_site,rawUrl,accessToken) == false)
  {
    // Not authenticated, go back for next request
    return;
  }

  // Remember the context: easy in API 2.0
  if(callback == nullptr && m_site == nullptr)
  {
    m_message = new HTTPMessage(HTTPCommand::http_response,HTTP_STATUS_NOT_FOUND);
    m_message->AddReference();
    m_message->SetRequestHandle((HTTP_REQUEST_ID)this);
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
                              m_message->SetRequestHandle((HTTP_REQUEST_ID)this);
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
    EventStream* stream  = m_server->SubscribeEventStream(m_site,m_site->GetSite(),absolutePath,m_request->RequestId,accessToken);
    if(stream)
    {
      stream->m_baseURL = rawUrl;
      m_server->GetThreadPool()->SubmitWork(callback,(void*)stream);
      return;
    }
  }

  // For all types of requests: Create the HTTPMessage
  m_message = new HTTPMessage(type,m_site);
  m_message->SetRequestHandle((HTTP_REQUEST_ID)this);
  m_message->AddReference();
  // Enter our primary information from the request
  m_message->SetURL(rawUrl);
  m_message->SetReferrer(referrer);
  m_message->SetAuthorization(authorize);
  m_message->SetConnectionID(m_request->ConnectionId);
  m_message->SetContentType(contentType);
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
  DWORD bytes = 0;

  // Issue the async read request
  DWORD result = HttpReceiveRequestEntityBody(m_server->GetRequestQueue()
                                             ,m_requestID
                                             ,flags
                                             ,m_readBuffer
                                             ,INIT_HTTP_BUFFERSIZE
                                             ,&bytes
                                             ,&m_reading);
  
  if(result != ERROR_IO_PENDING)
  {
    // See if we are ready reading the whole body
    if(result == ERROR_HANDLE_EOF || 
       result == ERROR_CONNECTION_INVALID || 
       ((m_reading.Internal & 0xFFFF) == ERROR_NO_MORE_ITEMS))
    {
      // Ready reading the whole body
      PostReceive();
    }
    // Check for error
    else if(result != NO_ERROR)
    {
      ERRORLOG(result,"Start receiving request body");
      m_server->RespondWithServerError(m_message,HTTP_STATUS_SERVER_ERROR,"Server error","");
    }
    else // NO_ERROR
    {
      if(bytes)
      {
        m_readBuffer[bytes] = 0;
        m_message->GetFileBuffer()->AddBuffer(m_readBuffer,bytes);
      }
      // Ready reading the whole body
      PostReceive();
    }
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
    m_readBuffer[m_reading.InternalHigh] = 0;
    m_message->GetFileBuffer()->AddBuffer(m_readBuffer,m_reading.InternalHigh);

    if(result == NO_ERROR)
    {
      // Start the next read request
      StartReceiveRequest();
      return;
    }
  }
  else
  {
    ERRORLOG(result,"Error receiving bodypart");
    m_server->RespondWithServerError(m_message,HTTP_STATUS_SERVER_ERROR,"Server error","");
  }
  PostReceive();
}

// We have read the whole body of a message
// Now go processing it
void
HTTPRequest::PostReceive()
{
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

  // Place HTTPMessage in the response structure
  FillResponse();

  // Prepare our cache-policy
  m_policy.Policy        = m_server->GetCachePolicy();
  m_policy.SecondsToLive = m_server->GetCacheSecondsToLive();

  // We promise to always call HttpSendResponseEntityBody
  ULONG flags = HTTP_SEND_RESPONSE_FLAG_MORE_DATA;

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
  
  DETAILLOGV("HTTP Response %d %s",m_response->StatusCode,m_response->pReason);

  // Check for error
  if(result != ERROR_IO_PENDING && result != NO_ERROR)
  {
    ERRORLOG(result,"Sending HTTP Response");
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
    StartRequest();
    return;
  }
  // Now begin writing our response body parts
  ResetOutstanding(m_writing);
  m_writing.m_action = IO_Writing;

  // Default is we send all data in one go
  ULONG flags = HTTP_SEND_RESPONSE_FLAG_DISCONNECT;

  // Prepare send buffer
  memset(&m_sendBuffer,0,sizeof(HTTP_DATA_CHUNK));
  PHTTP_DATA_CHUNK chunks = &m_sendBuffer;
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

    m_sendBuffer.DataChunkType           = HttpDataChunkFromMemory;
    m_sendBuffer.FromMemory.pBuffer      = buffer;
    m_sendBuffer.FromMemory.BufferLength = (ULONG)length;

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

    m_sendBuffer.DataChunkType = HttpDataChunkFromFileHandle;
    m_sendBuffer.FromFileHandle.ByteRange.StartingOffset.QuadPart = 0;
    m_sendBuffer.FromFileHandle.ByteRange.Length.QuadPart = HTTP_BYTE_RANGE_TO_EOF;
    m_sendBuffer.FromFileHandle.FileHandle = m_file;
  }
  else if(filebuf->GetLength())
  {
    // FILEBUFFER CONTAINS EXACTLY ONE BUFFER AS AN OPTIMIZATION
    // Send buffer in one go
    uchar* buffer = nullptr;
    size_t length = 0;
    filebuf->GetBuffer(buffer,length);

    m_sendBuffer.DataChunkType           = HttpDataChunkFromMemory;
    m_sendBuffer.FromMemory.pBuffer      = buffer;
    m_sendBuffer.FromMemory.BufferLength = (ULONG)length;
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
  }

  // Still responding or done?
  m_responding = (flags == HTTP_SEND_RESPONSE_FLAG_MORE_DATA);
}

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
  }

  // Message is done. Break the connection with the HTTPRequest
  m_message->SetRequestHandle(NULL);

  // End of the line for the whole request
  // We did send everything as an answer
  Finalize();

  // Restart the request
  StartRequest();
}

// making the request ready for the next round!
void
HTTPRequest::Finalize()
{
  AutoCritSec lock(&m_critical);

  TRACE("FINALIZE\n");

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

  // Free the sending buffer
  RtlZeroMemory(&m_sendBuffer,sizeof(HTTP_DATA_CHUNK));

  // Remove unknown headers
  if(m_unknown)
  {
    free(m_unknown);
    m_unknown = nullptr;
  }

  // Free the message
  if(m_message)
  {
    m_message->SetRequestHandle(NULL);
    m_message->DropReference();
    m_message = nullptr;
  }

  // Reset parameters
  m_site       = nullptr;
  m_mayreceive = false;
  m_responding = false;
  m_file       = nullptr;
  m_bufferpart = 0;

  // Remove header strings
  m_strings.clear();
}

//////////////////////////////////////////////////////////////////////////
//
// SUB FUNCTIONS FOR RECEIVING AND SENDING
//
//////////////////////////////////////////////////////////////////////////

// Sub procedures for the handlers
bool 
HTTPRequest::CheckAuthentication(HTTPSite* p_site,CString& p_rawUrl,HANDLE& p_token)
{
  bool doReceive = true;

  if(m_request->RequestInfoCount > 0)
  {
    for(unsigned ind = 0; ind < m_request->RequestInfoCount; ++ind)
    {
      if(m_request->pRequestInfo[ind].InfoType == HttpRequestInfoTypeAuth)
      {
        // Default is failure
        doReceive = false;

        PHTTP_REQUEST_AUTH_INFO auth = (PHTTP_REQUEST_AUTH_INFO)m_request->pRequestInfo[ind].pInfo;
        if(auth->AuthStatus == HttpAuthStatusNotAuthenticated)
        {
          // Not (yet) authenticated. Back to the client for authentication
          DETAILLOGS("Not yet authenticated for: ",p_rawUrl);
          m_message = new HTTPMessage(HTTPCommand::http_response,HTTP_STATUS_DENIED);
          m_message->AddReference();
          m_message->SetRequestHandle((HTTP_REQUEST_ID)this);
          m_server->RespondWithClientError(m_message,HTTP_STATUS_DENIED,"Not authenticated",p_site->GetAuthenticationScheme());
          break;
        }
        else if(auth->AuthStatus == HttpAuthStatusFailure)
        {
          // Second round. Still not authenticated. Drop the connection, better next time
          DETAILLOGS("Authentication failed for: ",p_rawUrl);
          DETAILLOGV("Authentication failed because of: %s",m_server->AuthenticationStatus(auth->SecStatus).GetString());
          m_message = new HTTPMessage(HTTPCommand::http_response,HTTP_STATUS_DENIED);
          m_message->AddReference();
          m_message->SetRequestHandle((HTTP_REQUEST_ID)this);
          m_server->RespondWithClientError(m_message,HTTP_STATUS_DENIED,"Not authenticated",p_site->GetAuthenticationScheme());
          break;
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
          m_message = new HTTPMessage(HTTPCommand::http_response,HTTP_STATUS_FORBIDDEN);
          m_message->AddReference();
          m_message->SetRequestHandle((HTTP_REQUEST_ID)this);
          m_server->RespondWithClientError(m_message,HTTP_STATUS_FORBIDDEN,"Forbidden","");
          break;
        }
      }
      else if(m_request->pRequestInfo[ind].InfoType == HttpRequestInfoTypeSslProtocol)
      {
        // Only exists on Windows 10 / Server 2016
        if(m_server->GetDetailedLogging())
        {
          PHTTP_SSL_PROTOCOL_INFO sslInfo = (PHTTP_SSL_PROTOCOL_INFO)m_request->pRequestInfo[ind].pInfo;
          m_server->LogSSLConnection(sslInfo);
        }
      }
    }
  }
  return doReceive;
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
HTTPRequest::FillResponse()
{
  // Initialize the response body
  if(m_response == nullptr)
  {
    m_response = new HTTP_RESPONSE();
    RtlZeroMemory(m_response,sizeof(HTTP_RESPONSE));
  }
  unsigned status  = m_message->GetStatus();
  const char* text = GetHTTPStatusText(status);

  m_response->StatusCode   = (USHORT)status;
  m_response->pReason      = text;
  m_response->ReasonLength = (USHORT)strlen(text);

  // Add content type as a known header. (octet-stream or the message content type)
  CString contentType("application/octet-stream");
  if(!m_message->GetContentType().IsEmpty())
  {
    contentType = m_message->GetContentType();
  }
  AddKnownHeader(HttpHeaderContentType,contentType);

  // In case of a 401, we challenge to the client to identify itself
  if(m_message->GetStatus() == HTTP_STATUS_DENIED)
  {
    // Add authentication scheme
    CString challenge = m_server->BuildAuthenticationChallenge(m_site->GetAuthenticationScheme()
                                                              ,m_site->GetAuthenticationRealm());
    AddKnownHeader(HttpHeaderWwwAuthenticate,challenge);
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
  if(!buffer->GetFileName().IsEmpty())
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

// Reset outstanding OVERLAPPED
void 
HTTPRequest::ResetOutstanding(OutstandingIO& p_outstanding)
{
  p_outstanding.Internal     = 0;
  p_outstanding.InternalHigh = 0;
  p_outstanding.Pointer      = nullptr;
}
