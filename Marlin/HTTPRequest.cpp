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

// XTOR
HTTPRequest::HTTPRequest(HTTPServer* p_server)
            :m_server(p_server)
{
  // Resetting OVERLAPPED
  Internal     = 0L;
  InternalHigh = 0L;
  Offset       = 0;
  OffsetHigh   = 0;
  hEvent       = nullptr;

  // Derive logging from the server
  m_logging = m_server->GetDetailedLogging();
}

// DTOR
HTTPRequest::~HTTPRequest()
{
  ClearMemory();
}

void
HTTPRequest::Finalize()
{
  delete this;
}

// Remove all memory from the heap
void
HTTPRequest::ClearMemory()
{
  if(m_message)
  {
    delete m_message;
    m_message = nullptr;
  }
  if(m_request)
  {
    free(m_request);
    m_request = nullptr;
  }
  if(m_response)
  {
    free(m_response);
    m_response = nullptr;
  }
}

// Start a new request against the server
void 
HTTPRequest::StartRequest()
{
  // Allocate the request object
  DWORD size = sizeof(HTTP_REQUEST_V2);
  m_request  = (PHTTP_REQUEST)calloc(1,size);
  // Set type of request
  m_outstanding = IO_Request;
  // Get queue handle
  HANDLE queue = m_server->GetRequestQueue();

  // Sit tight for the next request
  ULONG result = HttpReceiveHttpRequest(queue,        // Request Queue
                                        HTTP_NULL_ID, // Request ID
                                        0,            // Flags
                                        m_request,    // HTTP request buffer
                                        size,         // request buffer length
                                        &m_bytes,     // bytes received
                                        this);        // LPOVERLAPPED
  // See what we got
  if(result == NO_ERROR)
  {
    // By chance a request just arrived synchronously
    ReceiveRequest();
  }
  else if(result != ERROR_IO_PENDING)
  {
    // Error to the server log
    ERRORLOG(result,"Starting new HTTP Request");
  }
}

// Callback from I/O Completion port
void 
HTTPRequest::HandleAsynchroneousIO()
{
  switch(m_outstanding)
  {
    case IO_Request: ReceiveRequest();  break;
    case IO_Reading: ReceiveBodyPart(); break;
    case IO_Respond: SendResponse();    break;
    case IO_Writing: SendBodyPart();    break;
  }
}

// Primarily receive a request
void
HTTPRequest::ReceiveRequest()
{
  USES_CONVERSION;
  HANDLE accessToken = NULL;

  // Test if server already stopped, and we are here because of the stopping
  if(!m_server->GetIsRunning())
  {
    DETAILLOG1("HTTPServer stopped in mainloop.");
    Finalize();
    return;
  }

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
            ,SocketToServer((PSOCKADDR_IN6)sender)
            ,m_request->BytesReceived);

  // Log incoming request
  DETAILLOGS("Got a request for: ",rawUrl);

  // FInding the site
  bool eventStream = false;
  LPFN_CALLBACK callback = nullptr;
  HTTPSite* site = reinterpret_cast<HTTPSite*>(m_request->UrlContext);
  if(site)
  {
    callback    = site->GetCallback();
    eventStream = site->GetIsEventStream();
  }

  // See if we must substitute for a sub-site
  if(m_server->GetHasSubsites())
  {
    CString absPath = CW2A(m_request->CookedUrl.pAbsPath);
    site = m_server->FindHTTPSite(site,absPath);
  }

  // Now check for authentication and possible send 401 back
  if(CheckAuthentication(site,rawUrl,accessToken) == false)
  {
    // Not authenticated, go back for next request
    return;
  }

  // Remember the context: easy in API 2.0
  if(callback == nullptr && site == nullptr)
  {
    m_message = new HTTPMessage(HTTPCommand::http_response,HTTP_STATUS_NOT_FOUND);
    m_message->SetRequestHandle(m_request->RequestId);
    m_server->SendResponse(site
                          ,m_message
                          ,HTTP_STATUS_NOT_FOUND
                          ,"URL not found"
                          ,(PSTR)rawUrl.GetString()
                          ,site->GetAuthenticationScheme());
    // Ready with this request
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
                              m_message->SetRequestHandle(m_request->RequestId);
                              m_server->RespondWithServerError(site,m_message,HTTP_STATUS_NOT_SUPPORTED,"Not implemented","");
                              // Ready with this request
                              return;
                            }
                            break;
  }

  // Receiving the initiation of an event stream for the server
  acceptTypes.Trim();
  if((type == HTTPCommand::http_get) && (eventStream || acceptTypes.Left(17).CompareNoCase("text/event-stream") == 0))
  {
    CString absolutePath = CW2A(m_request->CookedUrl.pAbsPath);
    EventStream* stream  = m_server->SubscribeEventStream(site,site->GetSite(),absolutePath,m_request->RequestId,accessToken);
    if(stream)
    {
      stream->m_baseURL = rawUrl;
      m_server->GetThreadPool()->SubmitWork(callback,(void*)stream);
      return;
    }
  }

  // For all types of requests: Create the HTTPMessage
  m_message = new HTTPMessage(type,site);
  m_message->SetURL(rawUrl);
  m_message->SetReferrer(referrer);
  m_message->SetAuthorization(authorize);
  m_message->SetRequestHandle(m_request->RequestId);
  m_message->SetConnectionID(m_request->ConnectionId);
  m_message->SetContentType(contentType);
  m_message->SetAccessToken(accessToken);
  m_message->SetRemoteDesktop(remDesktop);
  m_message->SetSender((PSOCKADDR_IN6)sender);
  m_message->SetReceiver((PSOCKADDR_IN6)receiver);
  m_message->SetCookiePairs(cookie);
  m_message->SetAcceptEncoding(acceptEncoding);
  if(site->GetAllHeaders())
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
  if(type == HTTPCommand::http_post && site->GetVerbTunneling())
  {
    if(m_message->FindVerbTunneling())
    {
      DETAILLOGV("Request VERB changed to: %s",m_message->GetVerb());
    }
  }

  // Remember the fact that we should read the rest of the message
  m_message->SetReadBuffer(m_request->Flags & HTTP_REQUEST_FLAG_MORE_ENTITY_BODY_EXISTS);

  // Hit the thread pool with this message
  m_server->GetThreadPool()->SubmitWork(callback,(void*)m_message);
}


// 2) Receive trailing request body
void 
HTTPRequest::ReceiveBodyPart()
{
  // Setup read buffer

  // Call to 
}



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
          m_message->SetRequestHandle(m_request->RequestId);
          m_server->RespondWithClientError(p_site,m_message,HTTP_STATUS_DENIED,"Not authenticated",p_site->GetAuthenticationScheme());
          break;
        }
        else if(auth->AuthStatus == HttpAuthStatusFailure)
        {
          // Second round. Still not authenticated. Drop the connection, better next time
          DETAILLOGS("Authentication failed for: ",p_rawUrl);
          DETAILLOGV("Authentication failed because of: %s",m_server->AuthenticationStatus(auth->SecStatus));
          m_message = new HTTPMessage(HTTPCommand::http_response,HTTP_STATUS_DENIED);
          m_message->SetRequestHandle(m_request->RequestId);
          m_server->RespondWithClientError(p_site,m_message,HTTP_STATUS_DENIED,"Not authenticated",p_site->GetAuthenticationScheme());
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
          m_message->SetRequestHandle(m_request->RequestId);
          m_server->RespondWithClientError(p_site,m_message,HTTP_STATUS_FORBIDDEN,"Forbidden","");
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
