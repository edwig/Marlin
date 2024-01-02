/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: WebSocketServer.cpp
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
#include "WebSocketServer.h"
#include "WebSocketContext.h"
#include "HTTPRequest.h"
#include "HTTPMessage.h"
#include "HTTPSite.h"
#include "AutoCritical.h"
#include "ConvertWideString.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define DETAILLOG1(text)          if(MUSTLOG(HLL_LOGGING) && m_logfile) { DetailLog (_T(__FUNCTION__),LogType::LOG_INFO,text); }
#define DETAILLOGS(text,extra)    if(MUSTLOG(HLL_LOGGING) && m_logfile) { DetailLogS(_T(__FUNCTION__),LogType::LOG_INFO,text,extra); }
#define DETAILLOGV(text,...)      if(MUSTLOG(HLL_LOGGING) && m_logfile) { DetailLogV(_T(__FUNCTION__),LogType::LOG_INFO,text,__VA_ARGS__); }
#define ERRORLOG(code,text)       ErrorLog (_T(__FUNCTION__),code,text)

//////////////////////////////////////////////////////////////////////////
//
// SERVER MARLIN WebSocketServer
//
//////////////////////////////////////////////////////////////////////////

WebSocketServer::WebSocketServer(XString p_uri)
                :WebSocket(p_uri)
{
  m_context = new WebSocketContext;
}

WebSocketServer::~WebSocketServer()
{
  WebSocketServer::Reset();
}

void
WebSocketServer::Reset()
{
  WebSocket::Reset();

  if(m_context)
  {
    delete m_context;
    m_context = nullptr;
  }
}

// Open the socket. 
// Issue the first read-event
bool 
WebSocketServer::OpenSocket()
{
  if(m_server)
  {
    DETAILLOGS(_T("Opening WebSocket: "),m_uri);
    SocketListener();

    // Change state to opened
    m_openReading = true;
    m_openWriting = true;

    OnOpen();

    return true;
  }
  return false;
}

void WINAPI
ServerReadCompletion(HRESULT p_error,
                     VOID*   p_completionContext,
                     DWORD   p_bytes,
                     BOOL    p_utf8,
                     BOOL    p_final,
                     BOOL    p_close)
{
  WebSocketServer* socket = reinterpret_cast<WebSocketServer*>(p_completionContext);
  if(socket)
  {
    socket->SocketReader(p_error,p_bytes,p_utf8,p_final,p_close);
  }
}

void
WebSocketServer::SocketReader(HRESULT p_error
                             ,DWORD   p_bytes
                             ,BOOL    p_utf8
                             ,BOOL    p_final
                             ,BOOL    p_close)
{
  // Handle any error (if any)
  if(p_error != S_OK)
  {
    DWORD error = (p_error & 0x0F);
    ERRORLOG(error,_T("Websocket failed to read fragment"));
    CloseSocket();
    return;
  }

  // Consolidate the reading buffer
  m_reading->m_length += p_bytes;
  m_reading->m_data[m_reading->m_length] = 0;
  m_reading->m_final = (p_final == TRUE);

  if(!p_final)
  {
    m_reading->m_data = reinterpret_cast<BYTE*>(realloc(m_reading->m_data,static_cast<size_t>(m_reading->m_length) + static_cast<size_t>(m_fragmentsize) + WS_OVERHEAD));
  }

  // Setting the type of message
  if(p_close)
  {
    ReceiveCloseSocket();
    AutoCSTR closing(m_closing);

    m_reading->m_utf8   = true;
    m_reading->m_final  = true;
    m_reading->m_data   = (BYTE*)closing.grab();
    m_reading->m_length = m_closing.GetLength();

    // Store frame without UTF-8 conversion
    {
      AutoCritSec lock(&m_lock);
      m_frames.push_back(m_reading);
      m_reading = nullptr;
    }
  }
  else if(p_final)
  {
    // Store or append the fragment
    // Store the current fragment we just did read
    StoreWSFrame(m_reading);

    // Decide what to do and which handler to call
    if(p_utf8)
    {
      OnMessage();
    }
    else
    {
      OnBinary();
    }
  }

  // What to do at the end
  if(p_close)
  {
    if(m_openWriting)
    {
      XString reason;
      reason.Format(_T("WebSocket [%s] closed."),m_uri.GetString());
      SendCloseSocket(WS_CLOSE_NORMAL,reason);
    }
    OnClose();
    CloseSocket();
  }
  else
  {
    // Issue a new read command for a new buffer
    SocketListener();
  }
}

void
WebSocketServer::SocketListener()
{
  if(!m_reading)
  {
    m_reading = new WSFrame();
    m_reading->m_length = 0;
    m_reading->m_data   = reinterpret_cast<BYTE*>(malloc(static_cast<size_t>(m_fragmentsize) + WS_OVERHEAD));
  }

  // Issue the Asynchronous read-a-fragment command to the Asynchronous I/O WebSocket
  BOOL  utf8 = FALSE;
  BOOL  last = FALSE;
  BOOL  isclosing = FALSE;
  BOOL  expected  = FALSE;

  m_reading->m_read = m_fragmentsize;

  if(!m_reading->m_data)
  {
    ERRORLOG(ERROR_NOT_ENOUGH_MEMORY,_T("Reading websocket data."));
    CloseSocket();
    return;
  }

  HRESULT hr = m_context->ReadFragment(&m_reading->m_data[m_reading->m_length]
                                      ,&m_reading->m_read
                                      ,TRUE
                                      ,&utf8
                                      ,&last
                                      ,&isclosing
                                      ,ServerReadCompletion
                                      ,this
                                      ,&expected);
  if(FAILED(hr))
  {
    DWORD error = hr & 0x0F;
    ERRORLOG(error,_T("Websocket failed to register read command for a fragment"));
    CloseSocket();
    return;
  }

  if(!expected)
  {
    // Was issued in-sync after all, so re-work the received data
    m_reading->m_length += m_reading->m_read;
    SocketReader(hr,m_reading->m_length,utf8,last,isclosing);
  }
}

// Decode the incoming close socket message
bool
WebSocketServer::ReceiveCloseSocket()
{
  USHORT  length = 0;
  LPCWSTR pointer = nullptr;

  // Reset the result
  m_closingError = 0;
  m_closing.Empty();

  // Getting the closing status in UTF-16 format (instead of UTF-8 from RFC 6455)
  HRESULT hr = m_context->GetCloseStatus(&m_closingError,&pointer,&length);
  if(SUCCEEDED(hr))
  {
#ifdef UNICODE
    m_closing = pointer;
#else
    XString encoded;
    bool foundBom = false;
    if(TryConvertWideString(reinterpret_cast<const uchar*>(pointer),length,"",encoded,foundBom))
    {
      m_closing = encoded;
    }
#endif
    DETAILLOGV(_T("Received closing message [%d:%s] on WebSocket: %s"),m_closingError,m_closing.GetString(),m_uri.GetString());
    return true;
  }
  return false;
}

// Close the socket unconditionally
bool 
WebSocketServer::CloseSocket()
{
  CloseForReading();
  CloseForWriting();
  return true;
}

// Close the socket with a closing frame
bool 
WebSocketServer::SendCloseSocket(USHORT /*p_code*/,XString /*p_reason*/)
{
  return true;
}

// Write fragment to a WebSocket
bool 
WebSocketServer::WriteFragment(BYTE* /*p_buffer*/,DWORD /*p_length*/,Opcode /*p_opcode*/,bool /*p_last*/ /*= true*/)
{
  return true;
}

// Perform the server handshake
//
// THIS IS THE CLIENT SIDE. COMING IN THROUGH OUR MESSAGE
// Sec-WebSocket-Key: 2onFzTWu2gdaaB2qJBVF6w==
// Connection: Upgrade
// Upgrade: websocket
// Sec-WebSocket-Version: 13
//
// THIS SHOULD BE THE RESULT FROM THE CREATED SERVER-SIDE SOCKET
// Sec-WebSocket-Accept: 4VXbyKjRjv+1EdqmmujUgKvJ5jY=
// Connection: Upgrade
// Upgrade: websocket
//
bool
WebSocketServer::ServerHandshake(HTTPMessage* p_message)
{
  m_server   = p_message->GetHTTPSite()->GetHTTPServer();
  m_logfile  = m_server->GetLogfile();
  m_logLevel = m_server->GetLogLevel();

  XString wsVersion   (_T("Sec-WebSocket-Version"));
  XString wsClientKey (_T("Sec-WebSocket-Key"));
  XString wsConnection(_T("Connection"));
  XString wsUpgrade   (_T("Upgrade"));
  XString wsHost      (_T("Host"));

  XString version    = p_message->GetHeader(wsVersion);
  XString clientKey  = p_message->GetHeader(wsClientKey);
  XString connection = p_message->GetHeader(wsConnection);
  XString upgrade    = p_message->GetHeader(wsUpgrade);
  XString host       = p_message->GetHeader(wsHost);

  // Get optional extensions
  m_protocols  = p_message->GetHeader(_T("Sec-WebSocket-Protocol"));
  m_extensions = p_message->GetHeader(_T("Sec-WebSocket-Extensions"));

  // Remove general headers
  p_message->DelHeader(_T("Sec-WebSocket-Key"));
  p_message->DelHeader(_T("Sec-WebSocket-Version"));
  p_message->DelHeader(_T("Sec-WebSocket-Protocol"));
  p_message->DelHeader(_T("Sec-WebSocket-Extensions"));
  p_message->DelHeader(_T("cache-control"));
  p_message->DelHeader(_T("trailer"));
  p_message->DelHeader(_T("user-agent"));
  p_message->DelHeader(_T("host"));
  p_message->DelHeader(_T("content-length"));

  // By default we accept all protocols and extensions
  // All versions of 13 (RFC 6455) and above
  if(_ttoi(version) < 13 || clientKey.IsEmpty())
  {
    return false;
  }

  // Construct the WEBSOCKET protocol headers from the client
  WEB_SOCKET_HTTP_HEADER clientHeaders[5];
  AutoCSTR ckNamed(wsClientKey);
  AutoCSTR ckValue(clientKey);
  AutoCSTR coNamed(wsConnection);
  AutoCSTR coValue(connection);
  AutoCSTR upNamed(wsUpgrade);
  AutoCSTR upValue(upgrade);
  AutoCSTR vsNamed(wsVersion);
  AutoCSTR vsValue(version);
  AutoCSTR hoNamed(wsHost);
  AutoCSTR hoValue(host);

  clientHeaders[0].pcName        = (PCHAR) ckNamed.cstr();
  clientHeaders[0].ulNameLength  = (ULONG) ckNamed.size();
  clientHeaders[0].pcValue       = (PCHAR) ckValue.cstr();
  clientHeaders[0].ulValueLength = (ULONG) ckValue.size();

  clientHeaders[1].pcName        = (PCHAR) coNamed.cstr();
  clientHeaders[1].ulNameLength  = (ULONG) coNamed.size();
  clientHeaders[1].pcValue       = (PCHAR) coValue.cstr();
  clientHeaders[1].ulValueLength = (ULONG) coValue.size();
  
  clientHeaders[2].pcName        = (PCHAR) upNamed.cstr();
  clientHeaders[2].ulNameLength  = (ULONG) upNamed.size();
  clientHeaders[2].pcValue       = (PCHAR) upValue.cstr();
  clientHeaders[2].ulValueLength = (ULONG) upValue.size();
  
  clientHeaders[3].pcName        = (PCHAR) vsNamed.cstr();
  clientHeaders[3].ulNameLength  = (ULONG) vsNamed.size();
  clientHeaders[3].pcValue       = (PCHAR) vsValue.cstr();
  clientHeaders[3].ulValueLength = (ULONG) vsValue.size();

  clientHeaders[4].pcName        = (PCHAR) hoNamed.cstr();
  clientHeaders[4].ulNameLength  = (ULONG) hoNamed.size();
  clientHeaders[4].pcValue       = (PCHAR) hoValue.cstr();
  clientHeaders[4].ulValueLength = (ULONG) hoValue.size();

  WEB_SOCKET_HTTP_HEADER* serverheaders = nullptr;
  ULONG serverheadersCount = 0L;
  AutoCSTR protocols(m_protocols);

  bool result = m_context->PerformHandshake(clientHeaders,5,&serverheaders,&serverheadersCount,protocols.cstr());
  if(result)
  {
    // Record the handshake in our HTTPMessage
    DETAILLOG1(_T("WebSocketHandshake succeeded"));
    for (unsigned header = 0; header < serverheadersCount; ++header)
    {
      p_message->AddHeader(LPCSTRToString(serverheaders[header].pcName)
                          ,LPCSTRToString(serverheaders[header].pcValue));
    }
  }
  else
  {
    ERRORLOG(ERROR_PROCESS_ABORTED,_T("WebSocketHandshake failed"));
  }
  return result;
}

// Register the server request for sending info
bool 
WebSocketServer::RegisterSocket(HTTPMessage* p_message)
{
  // Register our server/request
  HTTPRequest* req = reinterpret_cast<HTTPRequest*>(p_message->GetRequestHandle());
  if(req)
  {
    m_request = req->GetRequest();
  }

  // We are now opened for business
  m_openReading = true;
  m_openWriting = true;

  // Reset request handle in the message
  p_message->SetHasBeenAnswered();

  if(m_context->CompleteHandshake())
  {
    DETAILLOG1(_T("WebSocket handshake completed."));
  }

  return true;
}


