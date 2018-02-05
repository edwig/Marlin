/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: EventStream.h
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

// Initial event keep alive milliseconds
constexpr auto DEFAULT_EVENT_KEEPALIVE = 10000;
// Initial event retry time in milliseconds
constexpr auto DEFAULT_EVENT_RETRYTIME = 3000;
// Maximum number of datachunks on a IIS event stream
// Theoretical max = 65535, but we stop before this is reached
constexpr auto MAX_DATACHUNKS = 65530;

class EventStream
{
public:
  SOCKADDR_IN6    m_sender;     // Stream originates from this address
  int             m_desktop;    // Stream originates from this desktop
  int             m_port;       // Port of the base URL of the stream
  CString         m_baseURL;    // Base URL of the stream
  CString         m_absPath;    // Absolute pathname of the URL
  HTTPSite*       m_site;       // HTTPSite that's handling the stream
  HTTP_RESPONSE   m_response;   // Response buffer
  HTTP_OPAQUE_ID  m_requestID;  // Outstanding HTTP request ID
  UINT            m_lastID;     // Last ID of this connection
  bool            m_alive;      // Connection still alive after sending
  __time64_t      m_lastPulse;  // Time of last sent event
  CString         m_user;       // For authenticated user
  long            m_chunks;     // Send chunk counter

  // Construct and init
  EventStream()
  {
    m_desktop   = 0;
    m_port      = 0;
    m_site      = nullptr;
    m_requestID = NULL;
    m_lastID    = 0;
    m_alive     = false;
    m_lastPulse = NULL;
    m_chunks    = 0;
  }
};

