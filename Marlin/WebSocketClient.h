/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: WebSocketClient.h
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
#pragma once
#include "WebSocketMain.h"

//////////////////////////////////////////////////////////////////////////
//
// FUNCTION PROTOTYPES for WebSocketClient
// Enables the dynamic loading on Windows8 and Windows10
// and fails silently on Windows 2008 / Windows 7
//
typedef HINTERNET(CALLBACK* WSOCK_COMPLETE)  (HINTERNET,DWORD_PTR);
typedef DWORD    (CALLBACK* WSOCK_CLOSE)     (HINTERNET,USHORT,PVOID,DWORD);
typedef DWORD    (CALLBACK* WSOCK_QUERYCLOSE)(HINTERNET,USHORT*,PVOID,DWORD,DWORD*);
typedef DWORD    (CALLBACK* WSOCK_SEND)      (HINTERNET,WINHTTP_WEB_SOCKET_BUFFER_TYPE,PVOID,DWORD);
typedef DWORD    (CALLBACK* WSOCK_RECEIVE)   (HINTERNET,PVOID,DWORD,DWORD*,WINHTTP_WEB_SOCKET_BUFFER_TYPE*);

//////////////////////////////////////////////////////////////////////////
//
// WEBSOCKET CLIENT
//
//////////////////////////////////////////////////////////////////////////

class WebSocketClient: public WebSocket
{
public:
  explicit WebSocketClient(XString p_uri);
  virtual ~WebSocketClient();

  // FUNCTIONS

  // Reset the socket
  virtual void Reset() override;
  // Open the socket
  virtual bool OpenSocket() override;
  // Close the socket
  virtual bool CloseSocket() override;
  // Close the socket with a closing frame
  virtual bool SendCloseSocket(USHORT p_code,XString p_reason) override;
  // Register the server request for sending info
  virtual bool RegisterSocket(HTTPMessage* p_message) override;
  // Send a ping/pong keep alive message
  virtual bool SendKeepAlive() override;
  // Write fragment to a WebSocket
  virtual bool WriteFragment(BYTE* p_buffer,DWORD p_length,Opcode p_opcode,bool p_last = true) override;
  // Generate a handshake key
  XString      GenerateKey();
  // Socket listener, entered by the StartClientListener only!
  void         SocketListener();

  // GETTERS

  // The handshake key the socket was started with
  XString      GetHandshakeKey();

protected:
  // Start listener thread for the client WebSocket
  bool         StartClientListner();
  // Decode the incoming close socket message
  bool         ReceiveCloseSocket();
  // Setting parameters for the client socket
  void         AddWebSocketHeaders();
  // Load function pointers from WinHTTP library
  void         LoadHTTPLibrary();
  void         FreeHTTPLibrary();

  // WinHTTP Client version of the WebSocket
  HINTERNET    m_socket   { NULL };   // Our socket handle for WinHTTP
  HANDLE       m_listener { NULL };   // Listener thread
  XString      m_socketKey;           // Given at the start
  HMODULE      m_winhttp  { NULL };   // Handle to the extended library

  // Pointers to the extended functions on Windows 8 / Windows 10
  WSOCK_COMPLETE    m_websocket_complete  { nullptr };
  WSOCK_CLOSE       m_websocket_close     { nullptr };
  WSOCK_QUERYCLOSE  m_websocket_queryclose{ nullptr };
  WSOCK_SEND        m_websocket_send      { nullptr };
  WSOCK_RECEIVE     m_websocket_receive   { nullptr };
};

inline XString
WebSocketClient::GetHandshakeKey()
{
  return m_socketKey;
}

// Register the server request for sending info
inline bool
WebSocketClient::RegisterSocket(HTTPMessage* /*p_message*/)
{
  // NO-OP for the client side
  return true;
}

