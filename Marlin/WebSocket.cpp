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
#include "HTTPSite.h"
#include "HTTPMessage.h"
#include "HTTPError.h"
#include "Base64.h"
#include "Crypto.h"
#include "AutoCritical.h"
#include "ConvertWideString.h"
#include "MarlinModule.h"
#include "GetLastErrorAsString.h"
#include "ServerApp.h"
#include <wincrypt.h>
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
// WEBSOCKET FRAMES
//
//////////////////////////////////////////////////////////////////////////

RawFrame::RawFrame()
{
  m_internal        = true;
  m_isRead          = false;
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

WSFrame::WSFrame()
{
}

WSFrame::~WSFrame()
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
  Reset();
  // Init synchronization
  InitializeCriticalSection(&m_lock);
  InitializeCriticalSection(&m_disp);
}

WebSocket::~WebSocket()
{
  Close();
  // Clean out synchronization
  DeleteCriticalSection(&m_lock);
  DeleteCriticalSection(&m_disp);
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

  for(auto& frame : m_frames)
  {
    delete frame;
  }
  m_frames.clear();
  m_messageNumber = 0;

  if(m_reading)
  {
    delete m_reading;
    m_reading = nullptr;
  }
}

void
WebSocket::Reset()
{
  m_fragmentsize   = WS_FRAGMENT_DEFAULT;
  m_keepalive      = WS_KEEPALIVE_TIME;
  m_pingTimeout    = 30000;  // Default ping timeout from IIS
  m_closingTimeout = 10000;  // Wait timeout for 'close' message
  // No handlers (yet)
  m_onopen        = nullptr;
  m_onmessage     = nullptr;
  m_onclose       = nullptr;
  // No closing error
  m_closingError  = 0;
  m_closing.Empty();
  // Extensions empty
  m_protocols.Empty();
  m_extensions.Empty();
  // Ping-Pong
  m_pongSeen = false;
}

//////////////////////////////////////////////////////////////////////////
//
// SETTERS & GETTERS
//
//////////////////////////////////////////////////////////////////////////

inline void
WebSocket::SetFragmentSize(ULONG p_fragment)
{
  if(p_fragment < WS_FRAGMENT_MINIMUM)
  {
    p_fragment = WS_FRAGMENT_MINIMUM;
  }
  if(p_fragment > WS_FRAGMENT_MAXIMUM)
  {
    p_fragment = WS_FRAGMENT_MAXIMUM;
  }
  m_fragmentsize = p_fragment;

  DETAILLOGV("WebSocket TCP/IP fragment size set to: %d",m_fragmentsize);
}

void
WebSocket::SetKeepalive(unsigned p_miliseconds)
{
  if(p_miliseconds < WS_KEEPALIVE_MINIMUM)
  {
    p_miliseconds = WS_KEEPALIVE_MINIMUM;
  }
  if(p_miliseconds > WS_KEEPALIVE_MAXIMUM)
  {
    p_miliseconds = WS_KEEPALIVE_MAXIMUM;
  }
  m_keepalive = p_miliseconds;
  
  DETAILLOGV("WebSocket keep-alive time set to: %d",m_keepalive);
}

void
WebSocket::SetClosingTimeout(unsigned p_timeout)
{
  if(p_timeout < WS_CLOSING_MINIMUM)
  {
    p_timeout = WS_CLOSING_MINIMUM;
  }
  if(p_timeout > WS_CLOSING_MAXIMUM)
  {
    p_timeout = WS_CLOSING_MAXIMUM;
  }
  m_closingTimeout = p_timeout;

  DETAILLOGV("WebSocket closing timeout set to: %d",m_closingTimeout);
}

// Add a URI parameter
void
WebSocket::AddParameter(CString p_name,CString p_value)
{
  p_name.MakeLower();
  m_parameters[p_name] = p_value;
}

// Find a URI parameter
CString
WebSocket::GetParameter(CString p_name)
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

//////////////////////////////////////////////////////////////////////////
//
// LOGGING
//
//////////////////////////////////////////////////////////////////////////

void
WebSocket::DetailLog(const char* p_function,LogType p_type,const char* p_text)
{
  if(MUSTLOG(HLL_LOGGING) && m_logfile)
  {
    m_logfile->AnalysisLog(p_function,p_type,false,p_text);
  }
}

void
WebSocket::DetailLogS(const char* p_function,LogType p_type,const char* p_text,const char* p_extra)
{
  if(MUSTLOG(HLL_LOGGING) && m_logfile)
  {
    CString text(p_text);
    text += p_extra;

    m_logfile->AnalysisLog(p_function,p_type,false,text);
  }
}

void
WebSocket::DetailLogV(const char* p_function,LogType p_type,const char* p_text,...)
{
  if(MUSTLOG(HLL_LOGGING) && m_logfile)
  {
    va_list varargs;
    va_start(varargs,p_text);
    CString text;
    text.FormatV(p_text,varargs);
    va_end(varargs);

    m_logfile->AnalysisLog(p_function,p_type,false,text);
  }
}

// Error logging to the log file
void
WebSocket::ErrorLog(const char* p_function,DWORD p_code,CString p_text)
{
  bool result = false;

  if(m_logfile)
  {
    p_text.AppendFormat(" Error [%d] %s",p_code,GetLastErrorAsString(p_code).GetString());
    result = m_logfile->AnalysisLog(p_function,LogType::LOG_ERROR,false,p_text);

    WSFrame* frame  = new WSFrame();
    frame->m_data   = (BYTE*) _strdup(p_text.GetString());
    frame->m_length = p_text.GetLength();
    frame->m_utf8   = true;
    frame->m_final  = true;
    
    // Store frame and call onError handler
    StoreWSFrame(frame);
    OnError();
  }

#ifdef _DEBUG
  // nothing logged
  if(!result)
  {
    // What can we do? As a last result: print to stdout
    printf(MARLIN_SERVER_VERSION " Error [%d] %s\n",p_code,(LPCTSTR)p_text);
  }
#endif
}

//////////////////////////////////////////////////////////////////////////
// 
// MESSAGE HANDLERS
// Connect handlers before opening a socket!
//
//////////////////////////////////////////////////////////////////////////

void
WebSocket::OnOpen()
{
  WSFrame frame;
  if(m_onopen && m_open)
  {
    DETAILLOGS("WebSocket OnOpen called for: ",m_uri);
    (*m_onopen)(this,&frame);
  }
}

void
WebSocket::OnMessage()
{
  WSFrame* frame = GetWSFrame();

  if(frame)
  {
    if(m_onmessage)
    {
      DETAILLOGS("WebSocket OnMessage called for: ",m_uri);
      (*m_onmessage)(this,frame);
    }
    delete frame;
  }
}

void
WebSocket::OnBinary()
{
  WSFrame* frame = GetWSFrame();

  if(frame)
  {
    if(m_onbinary)
    {
      DETAILLOGS("WebSocket OnBinary called for: ",m_uri);
      (*m_onbinary)(this,frame);
    }
    delete frame;
  }
}

void
WebSocket::OnError()
{
  WSFrame* frame = GetWSFrame();
  if(m_onerror)
  {
    DETAILLOGS("WebSocket OnError called for: ",m_uri);
    (*m_onerror)(this,frame);
  }
  delete frame;
}

void
WebSocket::OnClose()
{
  WSFrame* frame = GetWSFrame();

  if(frame)
  {
    if(m_onclose && m_open)
    {
      DETAILLOGS("WebSocket OnClose called for: ",m_uri);
      (*m_onclose)(this,frame);
    }
    delete frame;
  }
  else
  {
    WSFrame empty;
    if(m_onclose && m_open)
    {
      DETAILLOGS("WebSocket OnClose called for: ",m_uri);
      (*m_onclose)(this,&empty);
    }
  }
  // OnClose can be called just once!
  m_open = false;
}

//////////////////////////////////////////////////////////////////////////
//
// HIGH LEVEL INTERFACE: READING/WRITING of STRING / BINARY OBJECT
//
//////////////////////////////////////////////////////////////////////////

// Write as an UTF-8 string to the WebSocket
bool 
WebSocket::WriteString(CString p_string)
{
  CString encoded;
  // Now encode MBCS to UTF-8
  uchar* buffer = nullptr;
  int    length = 0;
  bool   result = false;

  DETAILLOGS("Outgoing message on WebSocket: ",m_uri);
  DETAILLOG1(p_string);

  if(TryCreateWideString(p_string,"",false,&buffer,length))
  {
    bool foundBom = false;
    if(TryConvertWideString(buffer,length,"utf-8",encoded,foundBom))
    {
      DWORD toSend  = encoded.GetLength();
      DWORD total   = 0;
      BYTE* pointer = (BYTE*)encoded.GetString();

      do
      {
        // Calculate the length of the next fragment
        bool last = true;
        DWORD toWrite = toSend - total;
        if(toWrite >= m_fragmentsize)
        {
          toWrite = m_fragmentsize;
          last    = false;
        }

        // Sent out the next fragment
        if(!WriteFragment(&pointer[total],toWrite,Opcode::SO_UTF8,last))
        {
          break;
        }
        // Bookkeeping of the total amount of sent bytes
        total += toWrite;
        result = true;
      }
      while(total < toSend);
    }
    delete [] buffer;
  }
  return result;
}

// Write as a binary object to the channel
bool 
WebSocket::WriteObject(BYTE* p_buffer,int64 p_length)
{
  // See if we have work to do
  if(p_buffer == nullptr || p_length == 0)
  {
    return false;
  }

  // How much did we already write out
  int64 total = 0;

  do 
  {
    // Calculate the length of the next fragment
    bool  last = true;
    DWORD toWrite = (DWORD)(p_length - total);
    if(toWrite > m_fragmentsize)
    {
      toWrite = m_fragmentsize;
      last    = false;
    }
    // Write out
    if(!WriteFragment(&p_buffer[total],toWrite,Opcode::SO_BINARY,last))
    {
      return false;
    }
    // Keep track of amount written
    total += toWrite;
  } 
  while(total < p_length);

  return true;
}

//////////////////////////////////////////////////////////////////////////
//
// LOW-LEVEL INTERFACE
//
//////////////////////////////////////////////////////////////////////////

// Get the first frame
WSFrame*
WebSocket::GetWSFrame()
{
  AutoCritSec lock(&m_lock);

  if(m_frames.empty())
  {
    return nullptr;
  }
  WSFrame* frame = m_frames.front();
  m_frames.pop_front();

  return frame;
}

// Store an incoming WSframe 
void    
WebSocket::StoreWSFrame(WSFrame*& p_frame)
{
  AutoCritSec lock(&m_lock);

  if(p_frame->m_utf8 && p_frame->m_final)
  {
    // Convert UTF-8 back to MBCS
    ConvertWSFrameToMBCS(p_frame);
  }
  // Actual store
  m_frames.push_back(p_frame);

  // Reset the origin of the stored message
  p_frame = nullptr;
}

// Append UTF-8 text to last frame on the stack, or just store it
void
WebSocket::StoreOrAppendWSFrame(WSFrame*& p_frame)
{
  AutoCritSec lock(&m_lock);

  // Try to get the Top-Of-Stack (TOS)
  WSFrame* last = nullptr;
  if(!m_frames.empty())
  {
    last = m_frames.back();
  }

  // See if we must append the text frame to the last
  if(last && last->m_utf8 && !last->m_final)
  {
    // Append the frame
    DWORD length = last->m_length + p_frame->m_length;
    last->m_data = (BYTE*) realloc(last->m_data,length + 1);
    memcpy_s(&last->m_data[last->m_length],p_frame->m_length + 1,p_frame->m_data,p_frame->m_length + 1);
    last->m_length = length;

    // Now convert to total back to MBCS
    if(p_frame->m_final)
    {
      ConvertWSFrameToMBCS(last);
    }
    // Done with the frame
    delete p_frame;
    p_frame = nullptr;
  }
  else
  {
    // Store the frame
    StoreWSFrame(p_frame);
  }
}

// Convert the UTF-8 in a frame back to MBCS
void
WebSocket::ConvertWSFrameToMBCS(WSFrame* p_frame)
{
  // Convert UTF-8 back to MBCS
  uchar*  buffer_utf8 = nullptr;
  int     length_utf8 = 0;
  CString input(p_frame->m_data);
  if(TryCreateWideString(input,"utf-8",false,&buffer_utf8,length_utf8))
  {
    CString encoded;
    bool foundBom = false;
    if(TryConvertWideString(buffer_utf8,length_utf8,"",encoded,foundBom))
    {
      int len = encoded.GetLength() + 1;
      p_frame->m_data = (BYTE*) realloc(p_frame->m_data,len);
      strcpy_s((char*)p_frame->m_data,len,encoded.GetString());
    }
  }
  delete [] buffer_utf8;

  DETAILLOGS("Incoming message on WebSocket: ",m_uri);
  DETAILLOG1((char*)p_frame->m_data);
}

bool
WebSocket::GetCloseSocket(USHORT& p_code,CString& p_reason)
{
  p_code   = m_closingError;
  p_reason = m_closing;

  return p_code > 0;
}

// Perform the server handshake
bool
WebSocket::ServerHandshake(HTTPMessage* /*p_message*/)
{
  // Does nothing 
  return true;
}

//////////////////////////////////////////////////////////////////////////
//
// SERVER MARLIN WebSocket
//
//////////////////////////////////////////////////////////////////////////

// FOR WEBSOCKETS TO WORK ON THE STAND-ALONE MARLIN
// IT NEEDS TO BE REWRIITEN TO DO ASYNC I/O THROUGHOUT THE SERVER!

WebSocketServer::WebSocketServer(CString p_uri)
                :WebSocket(p_uri)
{
}

WebSocketServer::~WebSocketServer()
{
}

void
WebSocketServer::Reset()
{
  WebSocket::Reset();

  // Nothing to do for ourselves (yet)
}

void StartSocket(void* p_context)
{
  WebSocketServer* socket = reinterpret_cast<WebSocketServer*>(p_context);
  socket->StartSocket();
}

void
WebSocketServer::StartSocket()
{
  m_server->ReceiveWebSocket(this,m_request);
}

// Open the socket
bool 
WebSocketServer::OpenSocket()
{
  if(m_server && m_request)
  {
    m_server->GetThreadPool()->SubmitWork(::StartSocket,this);
    return true;
  }
  return false;
}

// Close the socket unconditionally
bool 
WebSocketServer::CloseSocket()
{
  return false;
}

// Close the socket with a closing frame
bool 
WebSocketServer::SendCloseSocket(USHORT p_code,CString p_reason)
{
  if(m_open == false)
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
  m_open = false;

  // Remove buffer
  delete [] buffer;

  return closeSent;
}

// Write fragment to a WebSocket
bool 
WebSocketServer::WriteFragment(BYTE* /*p_buffer*/,DWORD /*p_length*/,Opcode /*p_opcode*/,bool /*p_last*/ /*= true*/)
{
  return false;
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
  int   begin = frame->m_headerLength;
  int64 length = frame->m_payloadLength;

  // Copy to the outside world
  p_buffer = &frame->m_data[begin];
  p_length = length;
  p_opcode = frame->m_opcode;
  p_last = frame->m_finalFrame;

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

// Generate a server key-answer
CString
WebSocketServer::ServerAcceptKey(CString p_clientKey)
{
  // Step 1: Append WebSocket GUID. See RFC 6455. It's hard coded!!
  CString key = p_clientKey + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

  // Step 2: Take the SHA1 hash of the resulting string
  Crypto crypt;
  return crypt.Digest(key.GetString(),(size_t)key.GetLength(),CALG_SHA1);
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
  m_protocols = p_message->GetHeader("Sec-WebSocket-Protocol");
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
  // Keep the receiver address as a two way socket
  memcpy(&m_socket,p_message->GetReceiver(),sizeof(SOCKADDR_IN6));

  // Reset request handle in the message
  p_message->SetRequestHandle(NULL);

  return true;
}


//////////////////////////////////////////////////////////////////////////
//
// SERVER WEBSOCKET
//
//////////////////////////////////////////////////////////////////////////

WebSocketServerIIS::WebSocketServerIIS(CString p_uri)
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
  m_server     = nullptr;
  m_iis_socket = nullptr;
  m_listener   = NULL;
}

bool
WebSocketServerIIS::OpenSocket()
{
  if(m_server && m_iis_socket)
  {
    DETAILLOGS("Opening WebSocket: ",m_uri);
    SocketListener();
    OnOpen();
    return (m_open = true);
  }
  return false;
}

// Close the socket
// Close the underlying TCP/IP connection
bool
WebSocketServerIIS::CloseSocket()
{
  if(m_server && m_iis_socket)
  {
    // Try to gracefully close the WebSocket
    try
    {
      m_iis_socket->CloseTcpConnection();
    }
    catch(...)
    {
      // result = false;
    }

    // Cancel the outstanding request altogether
    m_server->CancelRequestStream(m_request);

    DETAILLOGS("Closed WebSocket: ",m_uri);

    // Unregister ourselves first
    m_server->UnRegisterWebSocket(this);
    // Object is now invalid (removed by server)
    return true;
  }
  m_server     = nullptr;
  m_iis_socket = nullptr;

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
                                ,DWORD /*p_bytes*/
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

  // Handle any error (if any)
  if(p_error)
  {
    DWORD error = (p_error & 0x0F);
    ERRORLOG(error,"Websocket failed to write fragment");
    CloseSocket();
    return;
  }
  if(p_close)
  {
    ReceiveCloseSocket();
    if(m_open)
    {
      CString reason;
      reason.Format("WebSocket [%s] closed.",m_uri.GetString());
      SendCloseSocket(WS_CLOSE_NORMAL,reason);
    }
    OnClose();
    CloseSocket();
  }
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

  // Store the buffer in a WSFrame for async storage
  WSFrame* frame  = new WSFrame();
  frame->m_utf8   = (p_opcode == Opcode::SO_UTF8);
  frame->m_length = p_length;
  frame->m_data   = (BYTE*)malloc(p_length + WS_OVERHEAD);
  memcpy_s(frame->m_data,p_length + WS_OVERHEAD,p_buffer,p_length);

  // Put it in the writing queue
  // While locking the queue
  {
    AutoCritSec lock(&m_lock);
    m_writing.push_back(frame);
  }

  // Issue a async write command for this buffer
  BOOL expected = FALSE;
  HRESULT hr  = m_iis_socket->WriteFragment(frame->m_data
                                           ,&frame->m_length
                                           ,TRUE
                                           ,(BOOL)frame->m_utf8
                                           ,(BOOL)p_last
                                           ,ServerWriteCompletion
                                           ,this
                                           ,&expected);
  if(FAILED(hr))
  {
    DWORD error = hr & 0x0F;
    ERRORLOG(error,"Websocket failed to register write command for a fragment");
    CloseSocket();
    return false;
  }
  if(!expected)
  {
    // Finished synchronized after all, perform after-writing actions
    SocketWriter(hr,frame->m_length,frame->m_utf8,p_last,false);
  }

  return true;
}

void WINAPI 
ServerReadCompletion(HRESULT p_error,
                     VOID*   p_completionContext,
                     DWORD   p_bytes,
                     BOOL    p_utf8,
                     BOOL    p_final,
                     BOOL    p_close)
{
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
  if(p_error)
  {
    DWORD error = (p_error & 0x0F);
    ERRORLOG(error,"Websocket failed to read fragment");
    CloseSocket();
    return;
  }

  // Consolidate the reading buffer
  m_reading->m_data[p_bytes] = 0;
  m_reading->m_data   = (BYTE*)realloc(m_reading->m_data,p_bytes + 1);
  m_reading->m_length = p_bytes;
  m_reading->m_final  = p_final == TRUE;

  // Setting the type of message
  if(p_close)
  {
    ReceiveCloseSocket();
    m_reading->m_utf8   = true;
    m_reading->m_final  = true;
    m_reading->m_data   = (BYTE*) _strdup(m_closing.GetString());
    m_reading->m_length = m_closing.GetLength();

    // Store frame without UTF-8 conversion
    {
      AutoCritSec lock(&m_lock);
      m_frames.push_back(m_reading);
      m_reading = nullptr;
    }
  }
  else
  {
    // Store or append the fragment
    if(p_utf8)
    {
      StoreOrAppendWSFrame(m_reading);
    }
    else
    {
      // Store the current fragment we just did read
      StoreWSFrame(m_reading);
    }

    // Decide what to do and which handler to call
    if(!p_utf8)
    {
      OnBinary();
    }
    else if(p_utf8 && p_final)
    {
      OnMessage();
    }
  }

  // What to do at the end
  if(p_close)
  {
    if(m_open)
    {
      CString reason;
      reason.Format("WebSocket [%s] closed.",m_uri.GetString());
      SendCloseSocket(WS_CLOSE_NORMAL,reason);
    }
    OnClose();
    CloseSocket();
  }
  else
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
    m_reading           = new WSFrame();
    m_reading->m_length = m_fragmentsize;
    m_reading->m_data   = (BYTE*) malloc(m_fragmentsize + WS_OVERHEAD);
  }
    
  // Issue the Async read-a-fragment command to the Async I/O WebSocket
  BOOL utf8      = FALSE;
  BOOL last      = FALSE;
  BOOL isclosing = FALSE;
  BOOL expected  = FALSE;
  HRESULT hr = m_iis_socket->ReadFragment(m_reading->m_data
                                         ,&m_reading->m_length
                                         ,TRUE
                                         ,&utf8
                                         ,&last
                                         ,&isclosing
                                         ,ServerReadCompletion
                                         ,this
                                         ,&expected);
  if(FAILED(hr))
  {
    DWORD error = hr & 0x0F;
    ERRORLOG(error,"Websocket failed to register read command for a fragment");
    CloseSocket();
  }
  if(!expected)
  {
    // Was issued in-sync after all, so re-work the received data
    SocketReader(hr,m_reading->m_length,utf8,last,isclosing);
  }
}

bool
WebSocketServerIIS::SendCloseSocket(USHORT p_code,CString p_reason)
{
  int     length  = 0;
  uchar*  buffer  = nullptr;
  LPCWSTR pointer = nullptr;

  if(TryCreateWideString(p_reason,"",false,&buffer,length))
  {
    pointer = (LPCWSTR) buffer;
  }
  // Still other parameters and reason to do
  BOOL expected = FALSE;
  HRESULT hr = m_iis_socket->SendConnectionClose(TRUE
                                                ,p_code
                                                ,pointer
                                                ,ServerWriteCompletion
                                                ,this
                                                ,&expected);
  delete [] buffer;
  if(FAILED(hr))
  {
    ERRORLOG(ERROR_INVALID_OPERATION,"Cannot send a 'close' message on the WebSocket: " + m_uri);
    return false;
  }
  if(!expected)
  {
    SocketWriter(hr,length,true,true,false);
  }
  DETAILLOGV("Sent a 'close' message [%d:%s] on WebSocket: %s",p_code,p_reason.GetString(),m_uri.GetString());
  return true;
}

// Decode the incoming close socket message
bool
WebSocketServerIIS::ReceiveCloseSocket()
{
  USHORT  length  = 0;
  LPCWSTR pointer = nullptr;

  // Reset the result
  m_closingError = 0;
  m_closing.Empty();

  // Getting the closing status in UTF-16 format (instead of UTF-8 from RFC 6455)
  HRESULT hr = m_iis_socket->GetCloseStatus(&m_closingError,&pointer,&length);
  if(SUCCEEDED(hr))
  {
    CString encoded;
    bool foundBom = false;
    if(TryConvertWideString((const uchar*)pointer,length,"",encoded,foundBom))
    {
      m_closing = encoded;
    }
    DETAILLOGV("Received closing message [%d:%s] on WebSocket: %s",m_closingError,m_closing.GetString(),m_uri.GetString());
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
    m_iis_socket = (IWebSocketContext *)context3->GetNamedContextContainer()->GetNamedContext(IIS_WEBSOCKET);
    if(!m_iis_socket)
    {
      ERRORLOG(ERROR_FILE_NOT_FOUND,"Cannot upgrade to websocket!");
    }
    else
    {
      DETAILLOGS("HTTP connection upgraded to WebSocket for: ",m_uri);
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
WebSocketServerIIS::ServerHandshake(HTTPMessage* /*p_message*/)
{
  // Does nothing for IIS
  return true;
}

//////////////////////////////////////////////////////////////////////////
//
// CLIENT WEBSOCKET
//
//////////////////////////////////////////////////////////////////////////

WebSocketClient::WebSocketClient(CString p_uri)
                :WebSocket(p_uri)
{
}

WebSocketClient::~WebSocketClient()
{
  CloseSocket();
}

void
WebSocketClient::Reset()
{
  // Reset the main class part
  WebSocket::Reset();

  // Reset our part
  m_socket   = NULL;
  m_listener = NULL;
  m_socketKey.Empty();
}

bool
WebSocketClient::OpenSocket()
{
  // Reset the socket
  m_open = false;

  // Totally reset of the HTTPClient
  HTTPClient client;

  // Connect the logging
  client.SetLogging(m_logfile);
  client.SetLogLevel(m_logLevel);

  // GET this URI (ws[s]://resource) !!
  client.SetVerb("GET");
  client.SetURL(m_uri);

  // Add extra protocol headers
  if(!m_protocols.IsEmpty())
  {
    client.AddHeader("Sec-WebSocket-Protocol",m_protocols);
  }
  if(!m_extensions.IsEmpty())
  {
    client.AddHeader("Sec-WebSocket-Extensions",m_extensions);
  }
  // We need all headers for the handshake
  client.SetReadAllHeaders(true);

  // We will do the WebSocket handshake on this client!
  // This keeps open the request handle for output as well
  client.SetWebsocketHandshake(true);

  // Set WebSocket options
  client.SetWSClosingTimeout(m_closingTimeout);
  client.SetWSKeepAlive(m_keepalive);

  // Send a bare HTTP line, just with headers: no body!
  if(client.Send())
  {
    // Wait for the opening of the WebSocket
    if(client.GetStatus() == HTTP_STATUS_SWITCH_PROTOCOLS)
    {
      // Switch the handles from WinHTTP to WinSocket
      HINTERNET handle = client.GetWebsocketHandle();
      m_socket = WinHttpWebSocketCompleteUpgrade(handle,NULL);
      if(m_socket)
      {
        // Close client handle first. HTTP is no longer needed
        WinHttpCloseHandle(handle);

        // Trying to start the listener
        if(StartClientListner())
        {
          // If we come back here the receive thread is running
          DETAILLOGS("WebSocket open for: ",m_uri);
          m_open = true;
          OnOpen();
        }
      }
      else
      {
        DWORD error = GetLastError();
        CString message;
        message.Format("Socket upgrade to WinSocket failed [%d] %s\n",error,GetHTTPErrorText(error).GetString());
        ERRORLOG(ERROR_PROTOCOL_UNREACHABLE,message);
      }
    }
    else
    {
      // No switching of protocols. Really strange to succeed in the HTTP call.
      ERRORLOG(ERROR_PROTOCOL_UNREACHABLE,"Server cannot switch from HTTP to WebSocket protocol");
    }
  }
  else
  {
    // Error handling. Socket not found on URI
    CString error;
    error.Format("WebSocket protocol not found on URI [%s] HTTP status [%d]",m_uri.GetString(),client.GetStatus());
    ERRORLOG(ERROR_NOT_FOUND,error);
  }
  return m_open;
}

// Close the socket
// Close the underlying TCP/IP connection
bool 
WebSocketClient::CloseSocket()
{
  // Hard close
  if(m_socket)
  {
    DETAILLOGS("Hard TCP/IP close for WebSocket: ",m_uri);
    WinHttpCloseHandle(m_socket);

    DWORD wait = 10;
    while(m_listener && wait <= 640)
    {
      Sleep(wait);
      wait *= 2;
    }
    m_socket = NULL;
  }
  return true;
}

// Close the socket with a closing frame
bool 
WebSocketClient::SendCloseSocket(USHORT p_code,CString p_reason)
{
  if(m_socket)
  {
    // Check if p_reason is shorter then or equal to 123 bytes 
    DWORD length = p_reason.GetLength();
    if(length > WS_CLOSE_MAXIMUM)
    {
      length = WS_CLOSE_MAXIMUM;
    }
    DETAILLOGV("Send close WebSocket [%d:%s] for: %s",p_code,p_reason.GetString(),m_uri.GetString());
    DWORD error = WinHttpWebSocketClose(m_socket,p_code,(void*)p_reason.GetString(),length);
    if(error)
    {
      // We could be in a tight spot (socket already closed)
      if(error == ERROR_INVALID_OPERATION)
      {
        return true;
      }
      // Failed to send a close socket message.
      CString message = "Failed to send WebSocket 'close' message: " + GetLastErrorAsString(error);
      ERRORLOG(error,message);
      return false;
    }
    else if(!m_closingError)
    {
      if(ReceiveCloseSocket() && m_closingError)
      {
        // The other side acknowledged the fact that they did close also
        CloseSocket();
      }
    }
    else
    {
      // It was an answer on an incoming 'close' message
      // We did our answering, so close completely
      CloseSocket();
    }
    OnClose();
    return true;
  }
  return false;
}

// Decode the incoming close socket message
bool
WebSocketClient::ReceiveCloseSocket()
{
  BYTE  reason[WS_CLOSE_MAXIMUM + 1];
  DWORD received = 0;

  if(m_socket)
  {
    DWORD error = WinHttpWebSocketQueryCloseStatus(m_socket
                                                  ,&m_closingError
                                                  ,&reason
                                                  ,WS_CLOSE_MAXIMUM + 1
                                                  ,&received);
    if(error == ERROR_INVALID_OPERATION)
    {
      // No closing frame received yet
      m_closingError = 0;
      m_closing.Empty();
      return false;
    }
    else
    {
      reason[received] = 0;
      m_closing = reason;
      DETAILLOGV("Closing WebSocket frame received [%d:%s]",m_closingError,m_closing.GetString());
    }
    return true;
  }
  return false;
}

// Writing one fragment out to the WebSocket handle
// Works regarding the type of fragment
bool
WebSocketClient::WriteFragment(BYTE* p_buffer,DWORD p_length,Opcode p_opcode,bool p_last /* = true */)
{
  // See if we can do anything at all
  if(!m_socket)
  {
    return false;
  }

  WINHTTP_WEB_SOCKET_BUFFER_TYPE type = WINHTTP_WEB_SOCKET_UTF8_MESSAGE_BUFFER_TYPE;
  if(p_opcode == Opcode::SO_UTF8)
  {
    type = p_last ? WINHTTP_WEB_SOCKET_UTF8_MESSAGE_BUFFER_TYPE : 
                    WINHTTP_WEB_SOCKET_UTF8_FRAGMENT_BUFFER_TYPE;
  }
  else if(p_opcode == Opcode::SO_BINARY)
  {
    type = p_last ? WINHTTP_WEB_SOCKET_BINARY_MESSAGE_BUFFER_TYPE : 
                    WINHTTP_WEB_SOCKET_BINARY_FRAGMENT_BUFFER_TYPE;
  }
  else if(p_opcode == Opcode::SO_CLOSE)
  {
    type = WINHTTP_WEB_SOCKET_CLOSE_BUFFER_TYPE;
  }

  DWORD error = WinHttpWebSocketSend(m_socket,type,p_buffer,p_length);
  if(error)
  {
    ERRORLOG(error,"ERROR while sending to WebSocket: " + m_uri);
    switch(error)
    {
      case ERROR_INVALID_OPERATION: // Socket closed
                                    CloseSocket();
                                    return false;
      case ERROR_INVALID_PARAMETER: // Buffer not matched
                                    return false;
    }
  }
  else
  {
    DETAILLOGV("WebSocket sent type: %d Bytes: %d to: %s",type,p_length,m_uri.GetString());
  }
  return (error == ERROR_SUCCESS);
}

void
WebSocketClient::SocketListener()
{
  // Wait a short time for the OnOpen event to have been handled
  // Immediately after the OnOpen, the "m_open" goes to 'true'
  for(unsigned wait = 10; wait <= 320; wait *= 2)
  {
    if(m_open) break;
    Sleep(wait);
  }

  do 
  {
    if(!m_reading)
    {
      m_reading = new WSFrame;
      m_reading->m_data = (BYTE*)malloc(m_fragmentsize + WS_OVERHEAD);
    }

    DWORD bytesRead = 0;
    WINHTTP_WEB_SOCKET_BUFFER_TYPE type;
    DWORD error = WinHttpWebSocketReceive(m_socket
                                         ,&m_reading->m_data[m_reading->m_length]
                                         ,m_fragmentsize
                                         ,&bytesRead
                                         ,&type);
    if(error)
    {
      if(error == ERROR_WINHTTP_OPERATION_CANCELLED)
      {
        // Socket handle got closed. Object is invalid now. Bail out
        return;
      }
      ERRORLOG(error,"ERROR while receiving from WebSocket: " + m_uri);
      CloseSocket();
    }
    else
    {
      DETAILLOGV("WebSocket receive type: %d Bytes: %d to: %s",type,bytesRead,m_uri.GetString());

      bool final = (type == WINHTTP_WEB_SOCKET_BINARY_MESSAGE_BUFFER_TYPE) ||
                   (type == WINHTTP_WEB_SOCKET_UTF8_MESSAGE_BUFFER_TYPE)   ||
                   (type == WINHTTP_WEB_SOCKET_CLOSE_BUFFER_TYPE);
      bool utf8  = (type == WINHTTP_WEB_SOCKET_UTF8_MESSAGE_BUFFER_TYPE)   ||
                   (type == WINHTTP_WEB_SOCKET_UTF8_FRAGMENT_BUFFER_TYPE);

      if(final)
      {
        // Fragment is complete
        m_reading->m_final   = final;
        m_reading->m_utf8    = utf8;
        m_reading->m_length += bytesRead;
        m_reading->m_data[m_reading->m_length] = 0;

        StoreWSFrame(m_reading);

        if(type == WINHTTP_WEB_SOCKET_CLOSE_BUFFER_TYPE)
        {
          ReceiveCloseSocket();
          if(m_open)
          {
            SendCloseSocket(WS_CLOSE_NORMAL,"Socket closed!");
          }
          OnClose();
          CloseSocket();
        }
        else if(utf8)
        {
          OnMessage();
        }
        else
        {
          OnBinary();
        }
      }
      else
      {
        // Reading another fragment
        if(utf8)
        {
          // Just append another fragment after the current one
          // And keep reading until we find the final UTF-8 fragment
          DWORD newsize = m_reading->m_length + bytesRead + m_fragmentsize + WS_OVERHEAD;
          m_reading->m_data = (BYTE*)realloc(m_reading->m_data,newsize);
          m_reading->m_length += bytesRead;
        }
        else
        {
          // Partial fragment for a binary buffer 
          // Store the fragment and call the handler
          m_reading->m_final  = false;
          m_reading->m_utf8   = false;
          m_reading->m_length = bytesRead;
          m_reading->m_data[bytesRead] = 0;
          StoreWSFrame(m_reading);
          OnBinary();
        }
      }
    }
  } 
  while (m_open);

  OnClose();

  // Ready with this listener
  m_listener = NULL;
}

unsigned int __stdcall StartingClientListenerThread(void* p_context)
{
  WebSocketClient* client = reinterpret_cast<WebSocketClient*>(p_context);
  client->SocketListener();
  return 0;
}

// Starting a listener for the WebSocket
bool
WebSocketClient::StartClientListner()
{
  if(m_listener == NULL)
  {
    // Thread for the client queue
    unsigned int threadID = 0;
    if((m_listener = (HANDLE)_beginthreadex(NULL,0,StartingClientListenerThread,(void *)(this),0,&threadID)) == INVALID_HANDLE_VALUE)
    {
      m_listener = NULL;
      ERRORLOG(GetLastError(),"Cannot start client listener thread for a WebSocket");
    }
    else
    {
      DETAILLOGV("Thread started with threadID [%d] for WebSocket stream.",threadID);
      return true;
    }
  }
  return false;
}

// Client key generator for the WebSocket protocol
// Now using the built-in key generator of WinHTTP
CString
WebSocketClient::GenerateKey()
{
  // crank up the random generator
  clock_t time = clock();
  srand((unsigned int)time);

  // WebSocket key is 16 bytes random data
  BYTE key[16];
  for(int ind = 0; ind < 16; ++ind)
  {
    // Take random value between 0 and 255 inclusive
    key[ind] = (BYTE)(rand() / ((RAND_MAX + 1) / 0x100));
  }

  // Set key in a base64 encoded string
  Base64 base;
  char* buffer = m_socketKey.GetBufferSetLength((int)base.B64_length(16) + 1);
  base.Encrypt(key,16,(unsigned char*)buffer);
  m_socketKey.ReleaseBuffer();

  DETAILLOGS("Generated client WebSocket key: ",m_socketKey);
  return m_socketKey;
}
