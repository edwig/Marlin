/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: WebSocketServer.h
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
#pragma once
#include "WebSocket.h"

//////////////////////////////////////////////////////////////////////////
//
// SERVER MARLIN WebSocket
//
//////////////////////////////////////////////////////////////////////////

class WebSocketServer : public WebSocket
{
public:
  WebSocketServer(CString p_uri);
  virtual ~WebSocketServer();

  // Reset the socket
  virtual void Reset();
  // Open the socket
  virtual bool OpenSocket();
  // Close the socket unconditionally
  virtual bool CloseSocket();
  // Close the socket with a closing frame
  virtual bool SendCloseSocket(USHORT p_code,CString p_reason);
  // Write fragment to a WebSocket
  virtual bool WriteFragment(BYTE* p_buffer,DWORD p_length,Opcode p_opcode,bool p_last = true);
  // Register the server request for sending info
  virtual bool RegisterSocket(HTTPMessage* p_message);
  // Perform the server handshake
  virtual bool ServerHandshake(HTTPMessage* p_message);

  // Read a fragment from a WebSocket
  bool      ReadFragment(BYTE*& p_buffer,int64& p_length,Opcode& p_opcode,bool& p_last);
  // Send a 'ping' to see if other side still there
  bool      SendPing();
  // Send a 'pong' as an answer on a 'ping'
  void      SendPong(RawFrame* p_ping);
  // Encode raw frame buffer
  bool      EncodeFramebuffer(RawFrame* p_frame,Opcode p_opcode,bool p_mask,BYTE* p_buffer,int64 p_length,bool p_last);
  // Decode raw frame buffer (only m_data is filled)
  bool      DecodeFrameBuffer(RawFrame* p_frame,int64 p_length);
  // Store incoming raw frame buffer
  bool      StoreFrameBuffer(RawFrame* p_frame);
  // Get next frame to write to the stream
  WSFrame*  GetFrameToWrite();

protected:
  // Decode the incoming closing fragment before we call 'OnClose'
  void      DecodeCloseFragment(RawFrame* p_frame);

  // Private data for the server variant of the WebSocket
  HTTPServer*     m_server  { nullptr };
  HTTP_OPAQUE_ID  m_request { NULL    };
  // Asynchronous write buffer
  WSFrameStack    m_writing;
};

