/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: WebSocket.cpp
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
#include "WebSocket.h"

WebSocket::WebSocket(CString p_uri)
          :m_uri(p_uri)
          ,m_open(false)
{
  m_fragmentsize = WS_FRAGMENT_DEFAULT;
  m_keepalive    = WS_KEEPALIVE_TIME;
  // No handlers (yet)
  m_onopen    = nullptr;
  m_onmessage = nullptr;
  m_onclose   = nullptr;
}

WebSocket::~WebSocket()
{
  Close();
}

void
WebSocket::Close()
{

}

void
WebSocket::OnOpen()
{
  if(m_onopen)
  {
    (*m_onopen)(this);
  }
}

void
WebSocket::OnMessage()
{
  if(m_onmessage)
  {
    (*m_onmessage)(this);
  }
}

void
WebSocket::OnClose()
{
  if(m_onclose)
  {
    (*m_onclose)(this);
  }
}

// Encode raw frame buffer
void
WebSocket::EncodeFramebuffer(RawFrame* p_frame,bool p_mask,BYTE* p_buffer,DWORD p_length,bool p_last)
{

}

// Decode raw frame buffer (only m_data is filled)
void
WebSocket::DecodeFrameBuffer(RawFrame* p_frame,DWORD p_length)
{

}

//////////////////////////////////////////////////////////////////////////
//
// SERVER WEBSOCKET
//
//////////////////////////////////////////////////////////////////////////

ServerWebSocket::ServerWebSocket(CString p_uri)
                :WebSocket(p_uri)
{
}

ServerWebSocket::~ServerWebSocket()
{
}

bool
ServerWebSocket::WriteFragment(BYTE* p_buffer,DWORD p_length,Opcode p_opcode,bool p_last /* = true */)
{
  return false;
}

//////////////////////////////////////////////////////////////////////////
//
// CLIENT WEBSOCKET
//
//////////////////////////////////////////////////////////////////////////

ClientWebSocket::ClientWebSocket(CString p_uri)
                :WebSocket(p_uri)
{
}

ClientWebSocket::~ClientWebSocket()
{
}

bool
ClientWebSocket::WriteFragment(BYTE* p_buffer,DWORD p_length,Opcode p_opcode,bool p_last /* = true */)
{
  return false;
}