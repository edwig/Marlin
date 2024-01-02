/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: LongPolling.h
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
#include "ServerEventChannel.h"
#include "ClientEventDriver.h"
#include "LongTermEvent.h"
#include "HTTPClient.h"

// First interval between polling in milliseconds
#define POLL_INTERVAL_START 100
#define POLL_INTERVAL_MAX   (60 * CLOCKS_PER_SEC)

enum class PollStatus
{
   PS_Error    = -1
  ,PS_Received = 0
  ,PS_Empty    = 1
  ,PS_Closing  = 2
};

class LogAnalysis;

class LongPolling
{
public:
  LongPolling();
 ~LongPolling();

  // SETTERS
  
  // MANDATORY: Set basic URL to ask for messages
  void SetURL(XString p_url);
  // MANDATORY: Set application callback
  void SetApplication(LPFN_EVENTCALLBACK p_callback,void* p_application);
  // OPTIONAL: Set a logfile
  void SetLogfile(LogAnalysis* p_logfile);
  // MANDATORY: Start the polling
  bool StartLongPolling(XString p_session,XString p_cookie,XString p_secret);
  // Stopping the long polling channel
  void StopLongPolling();

  // GETTERS
  bool GetIsReceiving();

  // Application posting an event to the server
  void PostEvent(LTEvent* p_event);
  // Registering an incoming event to the client queue
  void RegisterEvent(XString p_payload,EvtType p_type,int p_number = 0);
  // Only to be called by background thread
  void PollingThreadRunning();

private:
  void        Reset();
  PollStatus  AskForMessages(LTEvent* p_event = nullptr);
  int         SendCloseMessage();
  bool        StartPollingThread();
  void        AskingForMessages();

  XString m_url;
  XString m_session;
  XString m_cookie;
  XString m_secret;
  void*   m_application { nullptr };
  int     m_interval    { 0 };
  bool    m_receiving   { false };
  int     m_lastNumber  { 0 };
  bool    m_openSeen    { false };

  LPFN_EVENTCALLBACK  m_callback { nullptr };
  LogAnalysis*        m_logfile  { nullptr };
  HANDLE              m_event    { NULL };
  HANDLE              m_thread   { NULL };
  HTTPClient          m_client;
  CRITICAL_SECTION    m_lock;
};
