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
#include "HTTPServer.h"
#include "HTTPMessage.h"
#include "Base64.h"
#include "Crypto.h"
#include "AutoCritical.h"
#include <wincrypt.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//////////////////////////////////////////////////////////////////////////
//
// WEBSOCKET FRAMES
//
//////////////////////////////////////////////////////////////////////////

RawFrame::RawFrame()
{
  m_internal        = true;
  m_finalFrame      = true;
  m_reserved1       = false;
  m_reserved2       = false;
  m_reserved3       = false;
  m_opcode          = Opcode::SO_CONTINU;
  m_masked          = false;
  m_payloadLength   = 0;
  m_headerLength    = 0;
  m_data            = nullptr;
}

RawFrame::~RawFrame()
{
  if(m_data)
  {
    free(m_data);
    m_data = nullptr;
  }
}

//////////////////////////////////////////////////////////////////////////
//
// THE WEBSOCKET
//
//////////////////////////////////////////////////////////////////////////

WebSocket::WebSocket(CString p_uri)
          :m_uri(p_uri)
          ,m_open(false)
{
  m_fragmentsize  = WS_FRAGMENT_DEFAULT;
  m_keepalive     = WS_KEEPALIVE_TIME;
  // No handlers (yet)
  m_onopen        = nullptr;
  m_onmessage     = nullptr;
  m_onclose       = nullptr;
  // No closing error
  m_closingError  = 0;
  // Init synchronization
  InitializeCriticalSection(&m_lock);

  // Event to ping-pong on
  m_pingEvent = CreateEvent(NULL,FALSE,FALSE,NULL);
}

WebSocket::~WebSocket()
{
  Close();
  // Clean out synchronization
  DeleteCriticalSection(&m_lock);
}

void
WebSocket::Close()
{
  AutoCritSec lock(&m_lock);

  m_open = false;
  for(auto& frame : m_stack)
  {
    delete frame;
  }
  m_stack.clear();
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
  // Remove fragment from the stack
  AutoCritSec lock(&m_lock);

  if(!m_stack.empty())
  {
    m_stack.pop_front();
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

// Read a fragment from a WebSocket
bool 
WebSocket::ReadFragment(BYTE*& p_buffer,int64& p_length,Opcode& p_opcode,bool& p_last)
{
  AutoCritSec lock(&m_lock);

  // See if we did have an incoming fragment
  if(m_stack.empty())
  {
    return false;
  }

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

  return true;
}

// Drop fragment from WebSocket stack after it has been used
void 
WebSocket::DropFragment(BYTE* p_buffer)
{
  AutoCritSec lock(&m_lock);

  // See if we did have a fragment
  if(m_stack.empty())
  {
    return;
  }
  // Get frame from the front of the deque
  RawFrame* frame = m_stack.front();

  // See if it was the previously read fragment
  int begin = frame->m_headerLength;
  if(&frame->m_data[begin] == p_buffer)
  {
    // Pop from stack and destroy it
    m_stack.pop_front();
    delete frame;
  }
}

int
WebSocket::NumberOfFragments()
{
  AutoCritSec lock(&m_lock);
  return (int)m_stack.size();
}

// Read the extensions of the current frame
bool 
WebSocket::ReadExtensions(bool& p_extens1,bool& p_extens2,bool& p_extens3)
{
  AutoCritSec lock(&m_lock);

  // Reset the answer
  p_extens1 = false;
  p_extens2 = false;
  p_extens3 = false;

  // See if we did have an incoming fragment
  if(m_stack.empty())
  {
    return false;
  }

  // Get frame from the front of the deque
  RawFrame* frame = m_stack.front();

  // Get the reserved extension codes
  p_extens1 = frame->m_reserved1;
  p_extens2 = frame->m_reserved2;
  p_extens3 = frame->m_reserved3;

  return true;
}

// Encode raw frame buffer
bool
WebSocket::EncodeFramebuffer(RawFrame* p_frame,Opcode p_opcode,bool p_mask,BYTE* p_buffer,int64 p_length,bool p_last)
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
  p_frame->m_data = (BYTE*) malloc(headerlength + p_length);

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
WebSocket::DecodeFrameBuffer(RawFrame* p_frame,int64 p_length)
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
WebSocket::StoreFrameBuffer(RawFrame* p_frame)
{
  // If it is a 'continuation frame', just store it
  // And go back right away to receive more frames!
  if(p_frame->m_finalFrame == false ||
     p_frame->m_opcode     == Opcode::SO_CONTINU)
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
    if(m_pingEvent)
    {
      SetEvent(m_pingEvent);
    }
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

  // ALL OTHER OPCODES ARE STORED ON THE STACK AND HANDLED
  // BY THE OnMessage handler

  // Keep status for after the message handling
  bool result = p_frame->m_opcode != Opcode::SO_CLOSE;

  // Storing the frame in an extra guarded level
  {
    AutoCritSec lock(&m_lock);
    // Store the frame buffer
    m_stack.push_back(p_frame);
  }

  // Go handle the message
  if(result)
  {
    OnMessage();
  }
  else
  {
    // OnIncomingClose();
  }
  return result;
}

// Send a 'ping' to see if other side is still there
bool
WebSocket::SendPing(bool p_waitForPong /*= false*/)
{
  bool result = false;

  m_pongSeen = false;
  CString buffer("PING!");
  if(WriteFragment((BYTE*)buffer.GetString(),buffer.GetLength(),Opcode::SO_PING,true))
  {
    if(p_waitForPong && m_pingEvent)
    {
      if(WaitForSingleObject(m_pingEvent,m_pingTimeout) == WAIT_OBJECT_0)
      {
        result = m_pongSeen;
      }
    }
  }
  else
  {
    // Cannot send a 'ping', close the WebSocket
  }
  return result;
}


// Send a 'pong' as an answer on a 'ping'
void    
WebSocket::SendPong(RawFrame* p_ping)
{
  // Prepare a pong frame from the ping frame
  if(WriteFragment(&p_ping->m_data[p_ping->m_headerLength]
                  ,p_ping->m_payloadLength
                  ,Opcode::SO_PONG
                  ,true))
  {
    // Log a ping-pong
  }
}

// Close the socket with a closing frame
bool 
WebSocket::CloseSocket(USHORT p_code,CString p_reason)
{
  int length = p_reason.GetLength() + 3;
  BYTE* buffer = new BYTE[length];

  // First two bytes of the payload contain the closing code
  buffer[0] = (p_code >> 8 ) & 0xFF;
  buffer[1] = (p_code & 0xFF);

  // Rest of the buffer contains the message
  strncpy_s((char*)&buffer[2],length,p_reason.GetString(),p_reason.GetLength());

  // Make the message 1 frame maximum
  if(length >= 125)
  {
    buffer[125] = 0;
    length = 125;
  }

  // Try to write the fragment
  return WriteFragment(buffer,length,Opcode::SO_CLOSE);
}

// Generate a server key-answer
CString 
WebSocket::ServerAcceptKey(CString p_clientKey)
{
  // Step 1: Append WebSocket GUID. See RFC 6455. It's hard coded!!
  CString key = p_clientKey + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

  // Step 2: Take the SHA1 hash of the resulting string
  Crypto crypt;
  return crypt.Digest(key.GetString(),(size_t)key.GetLength(),CALG_SHA1);
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
ServerWebSocket::OpenSocket()
{
  return false;
}

bool
ServerWebSocket::WriteFragment(BYTE* p_buffer,int64 p_length,Opcode p_opcode,bool p_last /* = true */)
{
  RawFrame frame;
  EncodeFramebuffer(&frame,p_opcode,false,p_buffer,p_length,p_last);

  if(m_server)
  {
    if(m_server->SendSocket(frame,m_request))
    {
      return true;
    }
    // Cannot send to socket, close the socket.
  }
  return false;
}

// Add a URI parameter
void    
ServerWebSocket::AddParameter(CString p_name,CString p_value)
{
  p_name.MakeLower();
  m_parameters[p_name] = p_value;
}

// Find a URI parameter
CString 
ServerWebSocket::GetParameter(CString p_name)
{
  CString value;
  p_name.MakeLower();
  SocketParams::iterator it = m_parameters.find(p_name);
  if(it != m_parameters.end())
  {
    value = it->second;
  }
  return value;
}

// Perform the server handshake
bool
ServerWebSocket::ServerHandshake(HTTPMessage* p_message)
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

  // By default we accept all protocols and extensions
  // All versions of 13 (RFC 6455) and above
  if(atoi(version) < 13 || clientKey.IsEmpty())
  {
    return false;
  }
  return true;
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
  m_HTTPClient.Reset();
}

bool
ClientWebSocket::OpenSocket()
{
  m_open = false;

  // Totally reset of the HTTPClient
  m_HTTPClient.Reset();

  // GET this URI (ws[s]://resource) !!
  m_HTTPClient.SetVerb("GET");
  m_HTTPClient.SetURL(m_uri);

  // WebSocket headers
  m_HTTPClient.AddHeader("Upgrade","websocket");
  m_HTTPClient.AddHeader("Connection","upgrade");
  m_HTTPClient.AddHeader("Sec-WebSocket-Version","13"); // RFC 6455 !
  m_HTTPClient.AddHeader("Sec-WebSocket-Key",GenerateKey());
  if(!m_protocols.IsEmpty())
  {
    m_HTTPClient.AddHeader("Sec-WebSocket-Protocol",m_protocols);
  }
  if(!m_extensions)
  {
    m_HTTPClient.AddHeader("Sec-WebSocket-Extensions",m_extensions);
  }
  // Send a bare HTTP line, just with headers: no body!
  if(m_HTTPClient.Send())
  {
    if(m_HTTPClient.GetStatus() == HTTP_STATUS_SWITCH_PROTOCOLS)
    {
      if(m_HTTPClient.ReceiveWebSocket(this))
      {
        // If we come back here the receive thread is running
        m_open = true;
        OnOpen();
      }
    }
  }
  return m_open;
}

bool
ClientWebSocket::WriteFragment(BYTE* p_buffer,int64 p_length,Opcode p_opcode,bool p_last /* = true */)
{
  bool result = false;

  // Encode in a raw frame buffer.
  // We are the client, so mask the data on input (set to true)
  RawFrame frame;
  if(EncodeFramebuffer(&frame,p_opcode,true,p_buffer,p_length,p_last))
  {
    // Write more data to the request buffer
    result = m_HTTPClient.WriteRawFrame(&frame);
    if(!result)
    {
      m_open = false;
      OnClose();
    }
  }
  return result;
}

CString
ClientWebSocket::GenerateKey()
{
  // crank up the random generator
  clock_t time = clock();
  srand((unsigned int)time);

  // WebSocket key is 16 bytes random data
  BYTE key[16];
  for(int ind = 0;ind < 16; ++ind)
  {
    // Take random value between 0 and 255 inclusive
    key[ind] = (BYTE) (rand() / ((RAND_MAX + 1) / 0x100));
  }

  // Set key in a base64 encoded string
  Base64 base;
  char* buffer = m_socketKey.GetBufferSetLength((int)base.B64_length(16) + 1);
  base.Encrypt(key,16,(unsigned char*)buffer);
  m_socketKey.ReleaseBuffer();

  return m_socketKey;
}

