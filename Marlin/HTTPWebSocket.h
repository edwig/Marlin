/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: HTTPWebSocket.h
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
//////////////////////////////////////////////////////////////////////////
//
// This class is a wrapper around the MS-WebSocket protocol component API
// Primary information for the protocol can be found from:
// https://msdn.microsoft.com/en-us/library/windows/desktop/hh437448(v=vs.85).aspx
//
//////////////////////////////////////////////////////////////////////////
#pragma once
#include "websocket.h"
#include <websocket.h>
#include <http.h>

class HTTPRequest;

class HTTPWebSocket
{
public:
  HTTPWebSocket(HTTPRequest* p_request);
 ~HTTPWebSocket();

  // Opening and closing the socket
  bool IsOpen();
  bool BeginServerHandshake(USHORT p_count,PHTTP_UNKNOWN_HEADER p_headers);
  bool EndServerHandshake();
  void Abort();

  // PRIMARY FUNCTIONS: Send & Receive

  // bool Send();

  // SETTERS
  bool SetSendBufferSize(ULONG p_size);
  bool SetReceiveBufferSize(ULONG p_size);
  bool SetKeepAlive(ULONG p_keepalive);

  // GETTERS
  ULONG GetSendBufferSize()    { return m_sendBufferSize;    };
  ULONG GetReceiveBufferSize() { return m_receiveBufferSize; };
  ULONG GetKeepAlive()         { return m_keepalive;         };
  DWORD_PTR GetProperty(WEB_SOCKET_PROPERTY_TYPE p_type);

private:
  bool CreateServerHandle();
  void DeleteServerHandle();

  // We belong to this request
  HTTPRequest* m_request     { nullptr };
  // The WebSocket handle to the OS
  WEB_SOCKET_HANDLE m_handle { NULL };
  // The properties for creating the handle
  ULONG m_receiveBufferSize  { WS_FRAGMENT_DEFAULT };
  ULONG m_sendBufferSize     { WS_FRAGMENT_DEFAULT };
  ULONG m_keepalive          { WS_KEEPALIVE_TIME   };
  // Handshake headers
  PWEB_SOCKET_HTTP_HEADER m_headers  { nullptr };
  PWEB_SOCKET_HTTP_HEADER m_response { nullptr };
  ULONG m_respCount { 0 };
  // The bare transfer buffer
  void* m_buffer { nullptr };
};

