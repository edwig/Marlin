/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: WebSocketServer.h
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
#include "HTTPSYS_Websocket.h"

// Documented by Microsoft on:
// https://docs.microsoft.com/en-us/windows/win32/api/websocket/ne-websocket-web_socket_property_type
#define WEBSOCKET_BUFFER_OVERHEAD     256
#define WEBSOCKET_BUFFER_MINIMUM      256
#define WEBSOCKET_BUFFSIZE_DEFAULT   4096
#define WEBSOCKET_SLEEP_AFTER_CLOSE   500

#define WEBSOCKET_KEEPALIVE_TIMEOUT   120    // Default ping/pong timeout
#define WEBSOCKET_KEEPALIVE_MINTIME    15    // Minimal ping/pong timeout
#define WEBSOCKET_KEEPALIVE_MAXTIME   160    // Maximal ping/pong timeout (Do not extend beyond 180 seconds!)

#define WEBSOCKETSERVER_IDENT  0x15101959    // Identity marker

//////////////////////////////////////////////////////////////////////////
//
// SERVER MARLIN WebSocket
//
//////////////////////////////////////////////////////////////////////////

class WebSocketServer : public WebSocket
{
public:
  explicit WebSocketServer(XString p_uri);
  virtual ~WebSocketServer();

  // Reset the socket
  virtual void Reset() override;
  // Open the socket
  virtual bool OpenSocket() override;
  // Close the socket unconditionally
  virtual bool CloseSocket() override;
  // Close the socket with a closing frame
  virtual bool SendCloseSocket(USHORT p_code,XString p_reason) override;
  // Write fragment to a WebSocket
  virtual bool WriteFragment(BYTE* p_buffer,DWORD p_length,Opcode p_opcode,bool p_last = true) override;
  // Send a ping/pong keep alive message
  virtual bool SendKeepAlive() override;

  // Register the server request for sending info
  virtual bool RegisterSocket(HTTPMessage* p_message) override;
  // Perform the server handshake
  virtual bool ServerHandshake(HTTPMessage* p_message) override;

  // SETTERS

  void SetRecieveBufferSize(ULONG p_size);
  void SetSendBufferSize(ULONG p_size);
  void SetDisableClientMasking(BOOL p_disable);
  void SetDisableUTF8Checking(BOOL p_disable);
  void SetKeepaliveInterval(ULONG p_interval);
  
  // GETTERS

  ULONG GetReceiveBufferSize()    { return m_ws_recv_buffersize;      }
  ULONG GetSendBufferSize()       { return m_ws_send_buffersize;      }
  BOOL  GetDisableClientMasking() { return m_ws_disable_masking;      }
  BOOL  GetDisableUTF8Checking()  { return m_ws_disable_utf8_verify;  }
  HTTP_OPAQUE_ID GetRequestID()   { return m_request;                 }

  // To be called for ASYNC I/O completion!
  void      SocketReader(HRESULT p_error, DWORD p_bytes, BOOL p_utf8, BOOL p_final, BOOL p_close);
  void      SocketWriter(HRESULT p_error, DWORD p_bytes, BOOL p_utf8, BOOL p_final, BOOL p_close);

  // Are we still alive?
  ULONG     m_ident { WEBSOCKETSERVER_IDENT };

protected:
  // Socket listener, entered by the HTTPServer only!!
  void      SocketListener();
  // Decode the incoming close socket message
  bool      ReceiveCloseSocket();
  // Create the buffer protocol handle
  bool      CreateServerHandle();
  // Complete the server handshake
  bool      CompleteHandshake();
  // Writing fragments dispatcher
  void      SocketDispatch();
  // SYSWebSocket still a valid object
  bool      SYSWebSocketValid();

  // Private data for the server variant of the WebSocket
  HTTPServer*         m_server  { nullptr };
  HTTP_OPAQUE_ID      m_request { NULL    };
  XString             m_subProtocol;
  // Asynchronous write buffer
  WSFrameStack        m_writing;
  bool                m_dispatched { false };
  bool                m_inClosing  { false };

  // Buffer translation  protocol handle
  WEB_SOCKET_HANDLE   m_handle    { NULL    };
  HTTPSYS_WebSocket*  m_websocket { nullptr };

  // Buffer parameters
  ULONG m_ws_recv_buffersize      { WEBSOCKET_BUFFSIZE_DEFAULT  };
  ULONG m_ws_send_buffersize      { WEBSOCKET_BUFFSIZE_DEFAULT  };
  BOOL  m_ws_disable_masking      { FALSE   };
  BOOL  m_ws_disable_utf8_verify  { TRUE    };
  ULONG m_ws_keepalive_interval   { WEBSOCKET_KEEPALIVE_TIMEOUT };
};

