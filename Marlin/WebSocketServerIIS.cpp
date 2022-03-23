/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: WebSocketServerIIS.cpp
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
#include "stdafx.h"
#include "WebSocketServerIIS.h"
#include "HTTPServerIIS.h"
#include "HTTPMessage.h"
#include "HTTPSite.h"
#include "ServerApp.h"
#include "AutoCritical.h"
#include "ConvertWideString.h"
#include <iiswebsocket.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define DETAILLOG1(text)          if(MUSTLOG(HLL_LOGGING) && m_logfile) { DetailLog (__FUNCTION__,LogType::LOG_INFO,text); }
#define DETAILLOGS(text,extra)    if(MUSTLOG(HLL_LOGGING) && m_logfile) { DetailLogS(__FUNCTION__,LogType::LOG_INFO,text,extra); }
#define DETAILLOGV(text,...)      if(MUSTLOG(HLL_LOGGING) && m_logfile) { DetailLogV(__FUNCTION__,LogType::LOG_INFO,text,__VA_ARGS__); }
#define ERRORLOG(code,text)       ErrorLog (__FUNCTION__,code,text)

//////////////////////////////////////////////////////////////////////////
//
// SERVER WEBSOCKET
//
//////////////////////////////////////////////////////////////////////////

WebSocketServerIIS::WebSocketServerIIS(XString p_uri)
  :WebSocket(p_uri)
{
}

WebSocketServerIIS::~WebSocketServerIIS()
{
  Reset();
}

void
WebSocketServerIIS::Reset()
{
  // Reset the main class part
  WebSocket::Reset();

  // Reset our part
  m_parameters.clear();
  m_server = nullptr;
  m_iis_socket = nullptr;
  m_listener = NULL;

  // Remove writing stack
  for (auto& frame : m_writing)
  {
    delete frame;
  }
  m_writing.clear();
}

bool
WebSocketServerIIS::OpenSocket()
{
  if(m_server && m_iis_socket)
  {
    DETAILLOGV("Opening WebSocket [%s] on [%s]",m_key.GetString(),m_uri.GetString());
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
ServerWriteCompletion(HRESULT p_error,
                      VOID*   p_completionContext,
                      DWORD   p_bytes,
                      BOOL    p_utf8,
                      BOOL    p_final,
                      BOOL    p_close)
{
  WebSocketServerIIS* socket = reinterpret_cast<WebSocketServerIIS*>(p_completionContext);
  if(socket)
  {
    socket->SocketWriter(p_error,p_bytes,p_utf8,p_final,p_close);
  }
}

// Duties to perform after writing of a fragment is completed
void
WebSocketServerIIS::SocketWriter(HRESULT p_error
                                ,DWORD   p_bytes
                                ,BOOL  /*p_utf8*/
                                ,BOOL  /*p_final*/
                                ,BOOL    p_close)
{
  // Pop first WSFrame from the writing queue
  {
    AutoCritSec lock(&m_lock);
    if(!m_writing.empty())
    {
      WSFrame* frame = m_writing.front();
      delete frame;
      m_writing.pop_front();
    }
  }
  TRACE("WS BLOCK WRITTEN: %d\n",p_bytes);

  // Handle any error (if any)
  if(p_error)
  {
    DWORD error = (p_error & 0x0F);
    ERRORLOG(error,"Websocket failed to write fragment");
    OnError();
    CloseSocket();
    return;
  }
  if(p_close)
  {
    ReceiveCloseSocket();
    OnClose();
    CloseSocket();
    return;
  }

  // Ready with last write command
  m_dispatched = false;

  // See if we must dispatch an extra write command
  SocketDispatch();
}

// Low level WriteFragment
bool
WebSocketServerIIS::WriteFragment(BYTE*  p_buffer
                                 ,DWORD  p_length
                                 ,Opcode p_opcode
                                 ,bool   p_last /*=true */)
{
  // Check if we can write
  if(!m_server || !m_iis_socket)
  {
    return false;
  }

  // Store the buffer in a WSFrame for asynchronous storage
  WSFrame* frame  = new WSFrame();
  frame->m_utf8   = (p_opcode == Opcode::SO_UTF8);
  frame->m_length = p_length;
  frame->m_data   = (BYTE*)malloc(p_length + WS_OVERHEAD);
  frame->m_final  = p_last;
  memcpy_s(frame->m_data,p_length + WS_OVERHEAD,p_buffer,p_length);

  // Put it in the writing queue
  // While locking the queue
  bool dispatch_running = false;
  {
    AutoCritSec lock(&m_lock);
    m_writing.push_back(frame);
    dispatch_running = m_dispatched;
  }

  // Start dispatching write commands after last block is in the buffer
  if(!dispatch_running && p_last)
  {
    SocketDispatch();
  }
  return true;
}

void
WebSocketServerIIS::SocketDispatch()
{
  WSFrame* frame = nullptr;

  // Get from the writing queue
  // While locking the queue
  {
    AutoCritSec lock(&m_lock);
    if(m_writing.empty())
    {
      return;
    }
    m_dispatched = true;
    frame = m_writing.front();
  }

  // Issue a asynchronous write command for this buffer
  BOOL expected = FALSE;
  HRESULT hr = m_iis_socket->WriteFragment(frame->m_data
                                          ,&frame->m_length
                                          ,TRUE
                                          ,(BOOL)frame->m_utf8
                                          ,(BOOL)frame->m_final
                                          ,ServerWriteCompletion
                                          ,this
                                          ,&expected);
  if(FAILED(hr))
  {
    DWORD error = hr & 0x0F;
    ERRORLOG(error,"Websocket failed to register write command for a fragment");
  }
  if(!expected)
  {
    // Finished synchronized after all, perform after-writing actions
    SocketWriter(hr,frame->m_length,frame->m_utf8,frame->m_final,false);
  }
}

void WINAPI
ServerReadCompletionIIS(HRESULT p_error,
                        VOID*   p_completionContext,
                        DWORD   p_bytes,
                        BOOL    p_utf8,
                        BOOL    p_final,
                        BOOL    p_close)
{
  if(p_error == HRESULT_FROM_WIN32(ERROR_OPERATION_ABORTED))
  {
    // Aborted socket. Completion context can/will be totally wrong
    return;
  }
  // Try to get our context back
  WebSocketServerIIS* socket = reinterpret_cast<WebSocketServerIIS*>(p_completionContext);
  if(socket)
  {
    socket->SocketReader(p_error,p_bytes,p_utf8,p_final,p_close);
  }
}

void
WebSocketServerIIS::SocketReader(HRESULT p_error
                                ,DWORD   p_bytes
                                ,BOOL    p_utf8
                                ,BOOL    p_final
                                ,BOOL    p_close)
{
  // Handle any error (if any)
  if(p_error != S_OK)
  {
    DWORD error = (p_error & 0x0F);
    ERRORLOG(error,"Websocket failed to read fragment");
    CloseSocket();
    return;
  }

  // Outstanding read command on an already closed socket?
  if(m_iis_socket == nullptr)
  {
    // Unregister ourselves first
    m_server->UnRegisterWebSocket(this);
    // Object is now invalid (removed by server)
    return;
  }

  // Consolidate the reading buffer
  m_reading->m_length += p_bytes;
  m_reading->m_data[m_reading->m_length] = 0;
  m_reading->m_final = (p_final == TRUE);

  if(!p_final)
  {
    m_reading->m_data = (BYTE*)realloc(m_reading->m_data, m_reading->m_length + m_fragmentsize + WS_OVERHEAD);
  }

  // Setting the type of message
  if(p_close)
  {
    ReceiveCloseSocket();

    // Closing fragment is always ONE websocket frame, and thus always fits 
    // in the reading data buffer of two frames.
    strcpy_s((char*)m_reading->m_data,m_fragmentsize,m_closing.GetString());
    m_reading->m_length = m_closing.GetLength();
    m_reading->m_utf8   = true;
    m_reading->m_final  = true;

    // Store frame without UTF-8 conversion
    {
      AutoCritSec lock(&m_lock);
      m_frames.push_back(m_reading);
      m_reading = nullptr;
    }
    OnClose();
    CloseSocket();
    return;
  }
  else if(p_final)
  {
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

  if(m_server && m_iis_socket)
  {
    // Issue a new read command for a new buffer
    SocketListener();
  }
}

void
WebSocketServerIIS::SocketListener()
{
  if(!m_reading)
  {
    m_reading = new WSFrame();
    m_reading->m_length = 0;
    m_reading->m_data   = (BYTE*)malloc(m_fragmentsize + WS_OVERHEAD);
  }

  // Issue the Asynchronous read-a-fragment command to the Asynchronous I/O WebSocket
  BOOL  utf8 = FALSE;
  BOOL  last = FALSE;
  BOOL  isclosing = FALSE;
  BOOL  expected  = FALSE;

  m_reading->m_read = m_fragmentsize;

  HRESULT hr = m_iis_socket->ReadFragment(&m_reading->m_data[m_reading->m_length]
                                         ,&m_reading->m_read
                                         ,TRUE
                                         ,&utf8
                                         ,&last
                                         ,&isclosing
                                         ,ServerReadCompletionIIS
                                         ,this
                                         ,&expected);
  if(FAILED(hr))
  {
    DWORD error = hr & 0x0F;
    ERRORLOG(error,"Websocket failed to register read command for a fragment");
  }

  if(!expected)
  {
    // Was issued in-sync after all, so re-work the received data
    SocketReader(hr,m_reading->m_read,utf8,last,isclosing);
  }
}

bool
WebSocketServerIIS::SendCloseSocket(USHORT p_code,XString p_reason)
{
  int     length  = 0;
  uchar*  buffer  = nullptr;
  LPCWSTR pointer = L"";

  // Check if already closed
  if(m_iis_socket == nullptr)
  {
    return true;
  }

  if(TryCreateWideString(p_reason,"",false,&buffer,length))
  {
    pointer = (LPCWSTR)buffer;
  }

  // Still other parameters and reason to do
  BOOL expected = FALSE;
  XString message;
  HRESULT hr = S_FALSE;
  try
  {
    hr = m_iis_socket->SendConnectionClose(TRUE
                                          ,p_code
                                          ,pointer
                                          ,ServerWriteCompletion
                                          ,this
                                          ,&expected);
  }
  catch (StdException& ex)
  {
    UNREFERENCED_PARAMETER(ex);
    hr = S_FALSE;
  }
  delete[] buffer;
  if(FAILED(hr))
  {
    ERRORLOG(ERROR_INVALID_OPERATION,"Cannot send a 'close' message on the WebSocket [" + m_key + "] on [" + m_uri + "] " + message);
    return false;
  }
  if(!expected)
  {
    SocketWriter(hr,length,true,true,false);
  }
  DETAILLOGV("Sent a 'close' message [%d:%s] on WebSocket [%s] on [%s]",p_code,p_reason.GetString(),m_key.GetString(),m_uri.GetString());
  return true;
}

// Decode the incoming close socket message
bool
WebSocketServerIIS::ReceiveCloseSocket()
{
  USHORT  length = 0;
  LPCWSTR pointer = nullptr;

  // Reset the result
  m_closingError = 0;
  m_closing.Empty();

  // Getting the closing status in UTF-16 format (instead of UTF-8 from RFC 6455)
  HRESULT hr = m_iis_socket->GetCloseStatus(&m_closingError,&pointer,&length);
  if(SUCCEEDED(hr))
  {
    XString encoded;
    bool foundBom = false;
    if(TryConvertWideString((const uchar*)pointer,length,"",encoded,foundBom))
    {
      m_closing = encoded;
    }
    XString explanation = GetClosingErrorAsString();
    DETAILLOGV("Received closing message [%d:%s] -> [%s] on WebSocket [%s] on [%s]"
              ,m_closingError
              ,explanation.GetString()
              ,m_closing.GetString()
              ,m_key.GetString()
              ,m_uri.GetString());
    return true;
  }
  return false;
}

bool
WebSocketServerIIS::RegisterSocket(HTTPMessage*  p_message)
{
  // Remember our registration
  m_server  = p_message->GetHTTPSite()->GetHTTPServer();
  m_request = p_message->GetRequestHandle();

  // Reset the internal IIS socket pointer
  m_iis_socket = nullptr;

  // Now find the IWebSocketContext
  IHttpContext*  context = reinterpret_cast<IHttpContext*>(m_request);
  IHttpContext3* context3 = nullptr;
  HRESULT hr = HttpGetExtendedInterface(g_iisServer,context,&context3);
  if(SUCCEEDED(hr))
  {
    // Get Pointer to IWebSocketContext
    m_iis_socket = reinterpret_cast<IWebSocketContext*>(context3->GetNamedContextContainer()->GetNamedContext(IIS_WEBSOCKET));
    if(!m_iis_socket)
    {
      ERRORLOG(ERROR_FILE_NOT_FOUND,"Cannot upgrade to websocket!");
    }
    else
    {
      DETAILLOGV("HTTP connection upgraded to WebSocket for [%s] on [%s]",m_key.GetString(),m_uri.GetString());
      return true;
    }
  }
  else
  {
    ERRORLOG(ERROR_INVALID_FUNCTION,"IIS Extended socket interface not found!");
  }
  return false;
}

// Perform the server handshake
bool
WebSocketServerIIS::ServerHandshake(HTTPMessage* p_message)
{
  // Does nothing for IIS
  XString version   = p_message->GetHeader("Sec-WebSocket-Version");
  XString clientKey = p_message->GetHeader("Sec-WebSocket-Key");
  XString serverKey = ServerAcceptKey(clientKey);
  // Get optional extensions
  m_protocols  = p_message->GetHeader("Sec-WebSocket-Protocol");
  m_extensions = p_message->GetHeader("Sec-WebSocket-Extensions");

  // Change header fields
  p_message->DelHeader("Sec-WebSocket-Key");
  p_message->AddHeader("Sec-WebSocket-Accept",serverKey);

  // Remove general headers
  p_message->DelHeader("Sec-WebSocket-Version");
  p_message->DelHeader("cache-control");
  p_message->DelHeader("trailer");
  p_message->DelHeader("user-agent");
  p_message->DelHeader("host");

  // By default we accept all protocols and extensions
  // All versions of 13 (RFC 6455) and above
  if(atoi(version) < 13 || clientKey.IsEmpty())
  {
    return false;
  }

  // Remember our client key as the registration
  m_key = serverKey;

  return true;
}

VOID WINAPI OverlappedCompletion(DWORD dwErrorCode,DWORD dwNumberOfBytes,LPOVERLAPPED lpOverlapped)
{
  WebSocketServerIIS* socket = reinterpret_cast<WebSocketServerIIS*>(lpOverlapped);
  if(socket)
  {
    socket->PostCompletion(dwErrorCode, dwNumberOfBytes);
  }
}

void
WebSocketServerIIS::PostCompletion(DWORD dwErrorCode, DWORD /*dwNumberOfBytes*/)
{
  DETAILLOGV("Post completion of websocket [%s] Errorcode [%d]",m_key.GetString(),dwErrorCode);
}


// Close the socket
// Close the underlying TCP/IP connection
bool
WebSocketServerIIS::CloseSocket()
{
  if(m_iis_socket)
  {
    // Make local copies, so we are non-reentrant
    // Canceling I/O can make read/write closing end up here!!
    HTTPServer* server = m_server;
    IWebSocketContext* context = m_iis_socket;
    m_iis_socket = nullptr;

    // Try to gracefully close the WebSocket
    try
    {
      context->CancelOutstandingIO();

      context->CloseTcpConnection();
      Yield();
      Sleep(100);
      Yield();
    }
#ifndef MARLIN_USE_ATL_ONLY
    catch(CException& er)
    {
      ERRORLOG(12102,MessageFromException(er).GetString());
    }
#endif
    catch(StdException& ex)
    {
      ReThrowSafeException(ex);
      ERRORLOG(12102,ex.GetErrorMessage().GetString());
    }

    // Cancel the outstanding request altogether
    server->CancelRequestStream(m_request,false);

    DETAILLOGV("Closed WebSocket [%s] on [%s]",m_key.GetString(),m_uri.GetString());

    // Reduce memory, removing reading/writing stacks
    Reset();

    // Now find the IWebSocketContext
//     IHttpContext* contextIIS = reinterpret_cast<IHttpContext*>(m_request);
//     IHttpContext3* context3  = nullptr;
//     HRESULT hr = HttpGetExtendedInterface(g_iisServer,contextIIS,&context3);
//     if (SUCCEEDED(hr))
//     {
//       context3->PostCompletion(0,OverlappedCompletion,this);
//     }
    return true;
  }
  return false;
}

