/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: WebSocketServer.cpp
//
// Marlin Server: Internet server/client
// 
// Copyright (c) 2014-2021 ir. W.E. Huisman
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
#include "HTTPRequest.h"
#include "HTTPMessage.h"
#include "HTTPSite.h"
#include "AutoCritical.h"

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
// SERVER MARLIN WebSocket
//
//////////////////////////////////////////////////////////////////////////

WebSocketServer::WebSocketServer(CString p_uri)
                :WebSocket(p_uri)
{
}

WebSocketServer::~WebSocketServer()
{
  Reset();
}

void
WebSocketServer::Reset()
{
  WebSocket::Reset();

  // Nothing to do for ourselves (yet)
  HTTPRequest* request = reinterpret_cast<HTTPRequest*>(m_request);
  if(request)
  {
    request->CancelRequest();
  }
}

// Open the socket. 
// Already opened by RegisterWebSocket()
bool 
WebSocketServer::OpenSocket()
{
  return true;
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
WebSocketServer::SendCloseSocket(USHORT p_code,CString p_reason)
{
  if(m_openWriting == false)
  {
    return false;
  }
  int length = p_reason.GetLength() + 3;
  BYTE* buffer = new BYTE[length];

  // First two bytes of the payload contain the closing code
  buffer[0] = (p_code >> 8 ) & 0xFF;
  buffer[1] = (p_code & 0xFF);

  // Rest of the buffer contains the message
  strncpy_s((char*)&buffer[2],length - 2,p_reason.GetString(),p_reason.GetLength());

  // Make the message 1 frame maximum
  if(length >= 125)
  {
    buffer[125] = 0;
    length = 125;
  }

  // Remember our closing codes
  m_closingError = p_code;
  m_closing      = p_reason;

  // Try to write the fragment
  bool closeSent = WriteFragment(buffer,length,Opcode::SO_CLOSE);

  // Now close the socket 'officially'
  // Preventing from sending this message twice
  m_openWriting = false;

  // Remove buffer
  delete [] buffer;

  return closeSent;
}

// Write fragment to a WebSocket
bool 
WebSocketServer::WriteFragment(BYTE* p_buffer,DWORD p_length,Opcode p_opcode,bool p_last /*= true*/)
{
  // Check if we can write
  if(!m_server || !m_request)
  {
    return false;
  }

  // Store the buffer in a WSFrame for asynchronous storage
  WSFrame* frame  = new WSFrame();
  frame->m_final  = p_last;
  frame->m_utf8   = (p_opcode == Opcode::SO_UTF8);
  frame->m_length = p_length;
  frame->m_data   = (BYTE*) malloc(p_length + WS_OVERHEAD);
  memcpy_s(frame->m_data,p_length + WS_OVERHEAD,p_buffer,p_length);

  // Put it in the writing queue
  // While locking the queue
  AutoCritSec lock(&m_lock);
  m_writing.push_back(frame);

  // Re-start the writing process of the socket stream
  HTTPRequest* request = reinterpret_cast<HTTPRequest*>(m_request);
  request->FlushWebSocketStream();

  return true;
}

// Get next frame to write to the stream
WSFrame*
WebSocketServer::GetFrameToWrite()
{
  AutoCritSec lock(&m_lock);

  if(m_writing.empty())
  {
    return nullptr;
  }
  WSFrame* frame = m_writing.front();
  m_writing.pop_front();
  return frame;
}

// Read a fragment from a WebSocket
bool
WebSocketServer::ReadFragment(BYTE*& p_buffer,int64& p_length,Opcode& p_opcode,bool& p_last)
{
  AutoCritSec lock(&m_lock);

  // See if we did have an incoming fragment
  if(m_stack.empty()) return false;

  // Remove the already read fragments from the front of the deque
  while(m_stack.front()->m_isRead)
  {
    m_stack.pop_front();
  }

  // Still something left?
  if(m_stack.empty()) return false;

  // Get frame from the front of the deque
  RawFrame* frame = m_stack.front();

  // Find the data pointer
  int   begin  = frame->m_headerLength;
  int64 length = frame->m_payloadLength;

  // Copy to the outside world
  p_buffer = &frame->m_data[begin];
  p_length = length;
  p_opcode = frame->m_opcode;
  p_last   = frame->m_finalFrame;

  // Mark the fact that we just read it.
  frame->m_isRead = true;

  return true;
}

// Send a 'ping' to see if other side is still there
bool
WebSocketServer::SendPing()
{
  bool result = false;

  m_pongSeen = false;
  CString buffer("PING!");
  if(WriteFragment((BYTE*)buffer.GetString(),buffer.GetLength(),Opcode::SO_PING,true))
  {
    // LOG
  }
  else
  {
    // Cannot send a 'ping', close the WebSocket
  }
  return result;
}


// Send a 'pong' as an answer on a 'ping'
void    
WebSocketServer::SendPong(RawFrame* p_ping)
{
  // Prepare a pong frame from the ping frame
  if(WriteFragment(&p_ping->m_data[p_ping->m_headerLength]
                  ,(DWORD)p_ping->m_payloadLength
                  ,Opcode::SO_PONG
                  ,true))
  {
    // Log a ping-pong
  }
}

// Encode raw frame buffer
bool
WebSocketServer::EncodeFramebuffer(RawFrame* p_frame,Opcode p_opcode,bool p_mask,BYTE* p_buffer,int64 p_length,bool p_last)
{
  // Check if we have work to do
  if(p_frame == nullptr || p_buffer == nullptr)
  {
    return false;
  }
  // Setting the primary frame elements
  p_frame->m_finalFrame = p_last;
  p_frame->m_reserved1  = false;
  p_frame->m_reserved2  = false;
  p_frame->m_reserved3  = false;
  p_frame->m_opcode     = p_opcode;
  p_frame->m_masked     = p_mask;
  p_frame->m_payloadLength = p_length;

  int headerlength = WS_MIN_HEADER;
  if(p_length > 125)
  {
    headerlength += (p_length < 0xFFFF) ? 2 : 8;
  }
  int mask = headerlength;
  if(p_mask)
  {
    headerlength += 4;
  }
  p_frame->m_headerLength = headerlength;

  // Now allocate enough bytes for the data part to be sent
  p_frame->m_data = (BYTE*) malloc(headerlength + (int)p_length);

  // Coded payload_length
  int64 payload_length = p_length < 126 ? p_length : 126;
  if(p_length > 0xFFFF)
  {
    payload_length = 127;
  }

  // Create first two bytes
  BYTE byte1 = p_last ? 0x80 : 0;
  BYTE byte2 = p_mask ? 0x80 : 0;

  byte1 |= (int) p_opcode;
  byte2 |= payload_length;

  p_frame->m_data[0] = byte1;
  p_frame->m_data[1] = byte2;

  // Setting the extended payload length fields
  if(p_length > 125)
  {
    if(p_length < 0xFFFF)
    {
      p_frame->m_data[2] = (BYTE) (p_length >> 8) & 0xFF;
      p_frame->m_data[3] = (BYTE)  p_length & 0xFF;
    }
    else
    {
      p_frame->m_data[2] = (BYTE) (p_length >> 56) & 0xFF;
      p_frame->m_data[3] = (BYTE) (p_length >> 48) & 0xFF;
      p_frame->m_data[4] = (BYTE) (p_length >> 40) & 0xFF;
      p_frame->m_data[5] = (BYTE) (p_length >> 32) & 0xFF;
      p_frame->m_data[6] = (BYTE) (p_length >> 24) & 0xFF;
      p_frame->m_data[7] = (BYTE) (p_length >> 16) & 0xFF;
      p_frame->m_data[8] = (BYTE) (p_length >>  8) & 0xFF;
      p_frame->m_data[9] = (BYTE) (p_length & 0xFF);
    }
  }

  // Setting the mask in the data
  if(p_mask)
  {
    clock_t time = clock();
    srand((unsigned int)time);

    p_frame->m_data[mask    ] = (BYTE) (rand() / 128);
    p_frame->m_data[mask + 1] = (BYTE) (rand() / 128);
    p_frame->m_data[mask + 2] = (BYTE) (rand() / 128);
    p_frame->m_data[mask + 3] = (BYTE) (rand() / 128);
  }

  // Setting the data in the m_data part
  BYTE* pointer = &p_frame->m_data[headerlength];

  for(DWORD ind = 0;ind < p_length; ++ind)
  {
    if(p_mask)
    {
      // Masked data copy
      *pointer = p_buffer[ind] ^ p_frame->m_data[mask + (ind % 4)];
    }
    else
    {
      // Unmasked data copy
      *pointer = p_buffer[ind];
    }
    ++pointer;
  }

  return true;
}

// Decode raw frame buffer (only m_data is filled)
// The length parameter signifies the data length in m_data!!
bool
WebSocketServer::DecodeFrameBuffer(RawFrame* p_frame,int64 p_length)
{
  // No object or no data received
  if(p_frame == nullptr || p_frame->m_data == nullptr)
  {
    return false;
  }
  // Minimum header must be received
  if(p_length < WS_MIN_HEADER)
  {
    return false;
  }

  // Decode first two bytes
  BYTE byte1 = p_frame->m_data[0];
  BYTE byte2 = p_frame->m_data[1];

  // Begin payload is at this offset
  int payload_offset = 2;

  // Read opcode fields from the first octet
  p_frame->m_finalFrame = (byte1 >> 7) & 0x01;
  p_frame->m_reserved1  = (byte1 >> 6) & 0x01;
  p_frame->m_reserved2  = (byte1 >> 5) & 0x01;
  p_frame->m_reserved3  = (byte1 >> 4) & 0x01;
  p_frame->m_opcode     = static_cast<Opcode>(byte1 & 0x0F);

  // Read mask and payload length
  p_frame->m_masked     = (byte2 >> 7) & 0x01;
  int64 payloadlength   = (byte2 & 0x7F);

  if(payloadlength == 126)
  {
    // Check p_length at least 4 for the header
    if(p_length < 4)
    {
      return false;
    }

    payload_offset += 2;
    payloadlength = ((int64)p_frame->m_data[2] << 8) | 
                     (int64)p_frame->m_data[3];
  }
  if(payloadlength == 127)
  {
    // Check p_length at least 10 for the header
    if(p_length < 10)
    {
      return false;
    }
    payload_offset += 8;
    payloadlength = ((int64)p_frame->m_data[2] << 56) |
                    ((int64)p_frame->m_data[3] << 48) |
                    ((int64)p_frame->m_data[4] << 40) |
                    ((int64)p_frame->m_data[5] << 32) |
                    ((int64)p_frame->m_data[6] << 24) |
                    ((int64)p_frame->m_data[7] << 16) |
                    ((int64)p_frame->m_data[8] <<  8) |
                    ((int64)p_frame->m_data[9]);
  }

  // If masked, take those in account
  int mask = payload_offset;
  if(p_frame->m_masked)
  {
    payload_offset += 4;
  }

  // Save lengths
  p_frame->m_payloadLength = (int)payloadlength;
  p_frame->m_headerLength  = payload_offset;

  // Now check that we have indeed the promised data length
  // Otherwise we would write outside our buffered data!!
  if(payload_offset + payloadlength > p_length)
  {
    return false;
  }

  // See to the unmasking of the incoming data
  if(p_frame->m_masked)
  {
    BYTE* pointer = &(p_frame->m_data[payload_offset]);
    for(int64 ind = 0;ind < payloadlength; ++ind)
    {
      *pointer ^= p_frame->m_data[mask + (ind % 4)];
      ++pointer;
    }
  }

  // Set internal code. Unmasking has been done
  p_frame->m_internal = true;

  return true;
}

// Store incoming raw frame buffer
// Return value true  -> Handled
// Return value false -> Close is received
bool
WebSocketServer::StoreFrameBuffer(RawFrame* p_frame)
{
  // If it is a 'continuation frame', just store it
  // And go back right away to receive more frames!
  if(p_frame->m_finalFrame == false ||
     p_frame->m_opcode == Opcode::SO_CONTINU)
  {
    AutoCritSec lock(&m_lock);
    m_stack.push_back(p_frame);
    return true;
  }

  // If it is a 'pong' on a 'ping', do NOT store it
  // we ignore the contents and let it go
  if(p_frame->m_opcode == Opcode::SO_PONG)
  {
    m_pongSeen = true;
    delete p_frame;
    return true;
  }

  // If it is a 'ping', send a 'pong' back
  if(p_frame->m_opcode == Opcode::SO_PING)
  {
    SendPong(p_frame);
    delete p_frame;
    return true;
  }

  // See if it is a 'close' message
  if(p_frame->m_opcode == Opcode::SO_CLOSE)
  {
    DecodeCloseFragment(p_frame);
    OnClose();
    delete p_frame;
    // DONE WITH THE SOCKET!!
    return false;
  }

  // ALL OTHER OPCODES ARE STORED ON THE STACK AND HANDLED
  // BY THE OnMessage handler

  // Storing the frame in an extra guarded level
  {
    AutoCritSec lock(&m_lock);
    // Store the frame buffer
    m_stack.push_back(p_frame);
  }

  // Go handle the message
  OnMessage();

  return true;
}

// Perform the server handshake
bool
WebSocketServer::ServerHandshake(HTTPMessage* p_message)
{
  CString version   = p_message->GetHeader("Sec-WebSocket-Version");
  CString clientKey = p_message->GetHeader("Sec-WebSocket-Key");
  CString serverKey = ServerAcceptKey(clientKey);
  // Get optional extensions
  m_protocols  = p_message->GetHeader("Sec-WebSocket-Protocol");
  m_extensions = p_message->GetHeader("Sec-WebSocket-Extensions");

  // Change header fields
  p_message->DelHeader("Sec-WebSocket-Key");
  p_message->AddHeader("Sec-WebSocket-Accept",serverKey,false);

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
  return true;
}

// Decode the incoming closing fragment before we call 'OnClose'
void
WebSocketServer::DecodeCloseFragment(RawFrame* p_frame)
{
  int offset = p_frame->m_headerLength;

  // Get the error code
  m_closingError = p_frame->m_data[offset] << 8;
  m_closingError |= p_frame->m_data[offset + 1];

  char* buffer = new char[(int)p_frame->m_payloadLength];
  strncpy_s(buffer,(int)p_frame->m_payloadLength,(char *)&p_frame->m_data[offset + 2],(int)p_frame->m_payloadLength - 2);
  m_closing = buffer;
  delete[] buffer;
}

// Register the server request for sending info
bool 
WebSocketServer::RegisterSocket(HTTPMessage* p_message)
{
  // Register our server/request
  m_server  = p_message->GetHTTPSite()->GetHTTPServer();
  m_request = p_message->GetRequestHandle();

  // We are now opened for business
  m_openReading = true;
  m_openWriting = true;

  // Startup the socket by issuing a read and a write request
  HTTPRequest* request = reinterpret_cast<HTTPRequest*>(m_request);
  request->RegisterWebSocket(this);

  // Reset request handle in the message
  p_message->SetHasBeenAnswered();

  return true;
}


