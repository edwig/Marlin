/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: WebSocketServerSync.cpp
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
#include "stdafx.h"
#include "WebSocketServerSync.h"
#include "HTTPServer.h"
#include "HTTPSite.h"
#include "HTTPMessage.h"
#include "AutoCritical.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define DETAILLOG1(text)          if(MUSTLOG(HLL_LOGGING) && m_logfile) { DetailLog (_T(__FUNCTION__),LogType::LOG_INFO,text); }
#define DETAILLOGS(text,extra)    if(MUSTLOG(HLL_LOGGING) && m_logfile) { DetailLogS(_T(__FUNCTION__),LogType::LOG_INFO,text,extra); }
#define DETAILLOGV(text,...)      if(MUSTLOG(HLL_LOGGING) && m_logfile) { DetailLogV(_T(__FUNCTION__),LogType::LOG_INFO,text,__VA_ARGS__); }
#define ERRORLOG(code,text)       ErrorLog (_T(__FUNCTION__),code,text)

//////////////////////////////////////////////////////////////////////////
//
// WEBSOCKET MARLIN Synchronous mode
//
//////////////////////////////////////////////////////////////////////////

WebSocketServerSync::WebSocketServerSync(XString p_uri)
                    :WebSocketServer(p_uri)
{
}

WebSocketServerSync::~WebSocketServerSync()
{
  WebSocketServerSync::Reset();
}

void
WebSocketServerSync::Reset()
{
  WebSocket::Reset();

  m_server->CancelRequestStream(m_request);
}

// Register the server request for sending info
bool
WebSocketServerSync::RegisterSocket(HTTPMessage* p_message)
{
  // Register our server/request
  m_server  = p_message->GetHTTPSite()->GetHTTPServer();
  m_request = p_message->GetRequestHandle();

  // We are now opened for business
  m_openReading = true;
  m_openWriting = true;

  // Reset request handle in the message
  p_message->SetHasBeenAnswered();

  return true;
}

// Open the socket. 
// Already opened by RegisterWebSocket()
bool
WebSocketServerSync::OpenSocket()
{
  return true;
}

// Close the socket unconditionally
bool
WebSocketServerSync::CloseSocket()
{
  CloseForReading();
  CloseForWriting();
  return true;
}

// Write fragment to a WebSocket
bool 
WebSocketServerSync::WriteFragment(BYTE* p_buffer,DWORD p_length,Opcode /*p_opcode*/,bool p_last /*= true*/)
{
    // Check if we can write
  if(!m_server || !m_request)
  {
    return false;
  }

  // Fill in the structures
  HANDLE requestQueue = m_server->GetRequestQueue();
  ULONG  flags        = p_last ? HTTP_SEND_RESPONSE_FLAG_DISCONNECT : HTTP_SEND_RESPONSE_FLAG_MORE_DATA;
  ULONG  bytesSent    = 0;
  HTTP_DATA_CHUNK dataChunk;
  memset(&dataChunk,0,sizeof(HTTP_DATA_CHUNK));

  // Add an entity chunk.
  dataChunk.DataChunkType           = HttpDataChunkFromMemory;
  dataChunk.FromMemory.pBuffer      = p_buffer;
  dataChunk.FromMemory.BufferLength = (ULONG)p_length;
  
  // Send a new fragment out  
  ULONG result = HttpSendResponseEntityBody(requestQueue
                                           ,m_request
                                           ,flags
                                           ,1
                                           ,&dataChunk
                                           ,&bytesSent
                                           ,NULL
                                           ,NULL
                                           ,NULL
                                           ,NULL);

  return (result == NO_ERROR);
}
