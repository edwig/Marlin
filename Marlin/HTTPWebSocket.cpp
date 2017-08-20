/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: HTTPWebSocket.cpp
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
#include "HTTPWebSocket.h"

HTTPWebSocket::HTTPWebSocket(HTTPRequest* p_request)
              :m_request(p_request)
{
}

HTTPWebSocket::~HTTPWebSocket()
{
  DeleteServerHandle();
  if(m_buffer)
  {
    free(m_buffer);
    m_buffer = nullptr;
  }
  if(m_headers)
  {
    delete [] m_headers;
  }
  // Go finalize the request
  m_request->CancelRequest();
}

void
HTTPWebSocket::DeleteServerHandle()
{
  if(m_handle)
  {
    WebSocketDeleteHandle(m_handle);
    m_handle = NULL;
  }
}

// SETTERS

bool 
HTTPWebSocket::SetSendBufferSize(ULONG p_size)
{
  if(m_handle) return false;
  if(p_size < WS_FRAGMENT_MINIMUM) p_size = WS_FRAGMENT_MINIMUM;
  if(p_size > WS_FRAGMENT_MAXIMUM) p_size = WS_FRAGMENT_MAXIMUM;
  m_sendBufferSize = p_size;
  return true;
}

bool 
HTTPWebSocket::SetReceiveBufferSize(ULONG p_size)
{
  if(m_handle) return false;
  if(p_size < WS_FRAGMENT_MINIMUM) p_size = WS_FRAGMENT_MINIMUM;
  if(p_size > WS_FRAGMENT_MAXIMUM) p_size = WS_FRAGMENT_MAXIMUM;
  m_receiveBufferSize = p_size;
  return true;
}

bool 
HTTPWebSocket::SetKeepAlive(ULONG p_keepalive)
{
  if(m_handle) return false;
  if(p_keepalive < WS_KEEPALIVE_MINIMUM) p_keepalive = WS_KEEPALIVE_MINIMUM;
  if(p_keepalive > WS_KEEPALIVE_MAXIMUM) p_keepalive = WS_KEEPALIVE_MAXIMUM;
  m_keepalive = p_keepalive;
  return true;
}

// GETTERS

DWORD_PTR
HTTPWebSocket::GetProperty(WEB_SOCKET_PROPERTY_TYPE p_type)
{
  DWORD_PTR value = 0;
  ULONG value_size = 0;
  HRESULT res = WebSocketGetGlobalProperty(p_type,&value,&value_size);
  if(SUCCEEDED(res))
  {
    return value;
  }
  return 0;
}

// INITIALIZATION

bool 
HTTPWebSocket::CreateServerHandle()
{
  // Set up our properties array
  WEB_SOCKET_PROPERTY properties[] =
  {
    { WEB_SOCKET_RECEIVE_BUFFER_SIZE_PROPERTY_TYPE, 0,    sizeof(DWORD_PTR) },
    { WEB_SOCKET_SEND_BUFFER_SIZE_PROPERTY_TYPE,    0,    sizeof(DWORD_PTR) },
    { WEB_SOCKET_ALLOCATED_BUFFER_PROPERTY_TYPE,    0,    sizeof(DWORD_PTR) },
    { WEB_SOCKET_KEEPALIVE_INTERVAL_PROPERTY_TYPE,  0,    sizeof(DWORD_PTR) },
  };

  m_buffer = malloc(m_receiveBufferSize + m_sendBufferSize + 300);

  properties[0].pvValue = (void*) (DWORD_PTR) m_receiveBufferSize;
  properties[1].pvValue = (void*) (DWORD_PTR) m_sendBufferSize;
  properties[2].pvValue = m_buffer;
  properties[3].pvValue = (void*) (DWORD_PTR) m_keepalive;

  HRESULT res = WebSocketCreateServerHandle(nullptr,0,&m_handle);
  return SUCCEEDED(res);
}

bool
HTTPWebSocket::IsOpen()
{
  return m_handle != NULL;
}

bool
HTTPWebSocket::BeginServerHandshake(USHORT p_count,PHTTP_UNKNOWN_HEADER p_headers)
{
  // Be sure we have an opened WebSocket handle
  if(!m_handle)
  {
    if(!CreateServerHandle())
    {
      return false;
    }
  }

  // Setup the input request headers
  m_headers = new WEB_SOCKET_HTTP_HEADER[p_count + 2];
  for(USHORT ind = 0;ind < p_count; ++ind)
  {
    m_headers[ind].pcName = (PCHAR) p_headers[ind].pName;
    m_headers[ind].ulNameLength =   p_headers[ind].NameLength;

    m_headers[ind].pcValue = (PCHAR) p_headers[ind].pRawValue;
    m_headers[ind].ulValueLength =   p_headers[ind].RawValueLength;
  }

  m_headers[p_count]    .pcName  = "connection";
  m_headers[p_count]    .pcValue = "upgrade";
  m_headers[p_count + 1].pcName  = "upgrade";
  m_headers[p_count + 1].pcValue = "websocket";

  m_headers[p_count].ulNameLength  = 10;
  m_headers[p_count].ulValueLength = 7;
  m_headers[p_count + 1].ulNameLength  = 7;
  m_headers[p_count + 1].ulValueLength = 9;

  // protocols not implemented yet
  PCSTR subProtocol = "";
  PCSTR extensions  = "";

  // Perform the server handshake
  HRESULT res = WebSocketBeginServerHandshake(m_handle,subProtocol,&extensions,0,m_headers,p_count + 2,&m_response,&m_respCount);

  return SUCCEEDED(res);
}

bool 
HTTPWebSocket::EndServerHandshake()
{
  HRESULT res = WebSocketEndServerHandshake(m_handle);
  return SUCCEEDED(res);
}

void 
HTTPWebSocket::Abort()
{
  if(m_handle)
  {
    WebSocketAbortHandle(m_handle);
  }
  m_handle = NULL;
}

