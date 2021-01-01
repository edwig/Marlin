/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: WebSocket.cpp
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
#include "WebSocket.h"
#include "AutoCritical.h"
#include "Crypto.h"
#include "ConvertWideString.h"
#include "GetLastErrorAsString.h"
#include "Version.h"
#include <wincrypt.h>
#include <vadefs.h>

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
          ,m_openReading(false)
          ,m_openWriting(false)
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

  m_openReading = false;
  m_openWriting = false;
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
  if(m_onopen)
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
    if(m_onclose && (m_openReading || m_openWriting))
    {
      DETAILLOGS("WebSocket OnClose called for: ",m_uri);
      (*m_onclose)(this,frame);
    }
    delete frame;
  }
  else
  {
    WSFrame empty;
    if(m_onclose && (m_openReading || m_openWriting))
    {
      DETAILLOGS("WebSocket OnClose called for: ",m_uri);
      (*m_onclose)(this,&empty);
    }
  }
  // OnClose can be called just once!
  m_openReading = false;
  m_openWriting = false;
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
  if(MUSTLOG(HLL_LOGBODY))
  {
    DETAILLOG1(p_string);
  }

  if(TryCreateWideString(p_string,"",false,&buffer,length))
  {
    bool foundBom = false;
    if(TryConvertWideString(buffer,length,"utf-8",encoded,foundBom))
    {
      DWORD toSend  = encoded.GetLength();
      DWORD total   = 0;
      BYTE* pointer = (BYTE*)encoded.GetString();

      if(MUSTLOG(HLL_TRACEDUMP))
      {
        m_logfile->AnalysisHex(__FUNCTION__,m_uri,(void*)pointer,toSend);
      }
  
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

  DETAILLOGS("Incoming message on WebSocket: ",m_uri);
  if(MUSTLOG(HLL_TRACEDUMP))
  {
    // This is what we get from 'the wire'
    m_logfile->AnalysisHex(__FUNCTION__,m_uri,p_frame->m_data,p_frame->m_length);
  }

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

  // This is the data, as we interpret it in MBCS
  if(MUSTLOG(HLL_LOGBODY))
  {
    DETAILLOG1((char*)p_frame->m_data);
  }
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

// Generate a server key-answer
CString
WebSocket::ServerAcceptKey(CString p_clientKey)
{
  // Step 1: Append WebSocket GUID. See RFC 6455. It's hard coded!!
  CString key = p_clientKey + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

  // Step 2: Take the SHA1 hash of the resulting string
  Crypto crypt;
  return crypt.Digest(key.GetString(),(size_t) key.GetLength(),CALG_SHA1);
}
