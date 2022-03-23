/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: WebSocketServerIIS.h
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
#pragma once
#include "WebSocket.h"

//////////////////////////////////////////////////////////////////////////
//
// SERVER IIS WebSocket
//
//////////////////////////////////////////////////////////////////////////

class WebSocketServerIIS: public WebSocket
{
public:
  WebSocketServerIIS(XString p_uri);
  virtual ~WebSocketServerIIS();

  // FUNCTIONS

  // Reset the socket
  virtual void Reset();
  // Open the socket
  virtual bool OpenSocket();
  // Close the socket unconditionally
  virtual bool CloseSocket();
  // Close the socket with a closing frame
  virtual bool SendCloseSocket(USHORT p_code,XString p_reason);
  // Write fragment to a WebSocket
  virtual bool WriteFragment(BYTE* p_buffer,DWORD p_length,Opcode p_opcode,bool p_last = true);
  // Register the server request for sending info
  virtual bool RegisterSocket(HTTPMessage* p_message);
  // Perform the server handshake
  virtual bool ServerHandshake(HTTPMessage* p_message);

  // To be called for ASYNC I/O completion!
  void    SocketReader(HRESULT p_error,DWORD p_bytes,BOOL p_utf8,BOOL p_final,BOOL p_close);
  void    SocketWriter(HRESULT p_error,DWORD p_bytes,BOOL p_utf8,BOOL p_final,BOOL p_close);
  // Socket listener, entered by the HTTPServerIIS only!!
  void    SocketListener();
  // Dispatch an extra write action
  void    SocketDispatch();
  void    PostCompletion(DWORD dwErrorCode,DWORD dwNumberOfBytes);
protected:
  // Decode the incoming close socket message
  bool    ReceiveCloseSocket();

  // Private data for the server variant of the WebSocket
  HTTPServer*         m_server     { nullptr };
  HTTP_OPAQUE_ID      m_request    { NULL    };
  // Private data for the IIS WebSocket variant
  IWebSocketContext*  m_iis_socket { nullptr };
  HANDLE              m_listener   { NULL    };
  // Asynchronous write buffer
  WSFrameStack        m_writing;
  bool                m_dispatched { false   };
};


