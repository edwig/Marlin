/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: ClientEventDriver.h
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
#include "ServerEventChannel.h"
#include "HTTPClient.h"
#include "ServerEvent.h"

//////////////////////////////////////////////////////////////////////////
//
// MANUAL to use the ClientEventDriver
// 
// A) Prepare your driver
//    1) Use "SetApplicationCallback" to connect your application
//    2) Use "SetThreadPool" if your application already has one
//    2) Use "SetLogfile"    if your application already has one
// 
// B) Start everything in one go with "StartEventDriver" OR
//
// C) Delayed start for an application (knowing the session-id later)
//    1) Set the "ServerURL"and "SetChannelPolicy" to prepare
//    2) Use "StartEventsForSession" when session-id is gotten from the server
//
// D) Optionally post event to server with "PostEventToServer"
//    But beware: Works only for WebSocket and LongPolling connections
//    SSE does **NOT** allow any postings
//
class EventSource;
class ThreadPool;
class LogAnalysis;
class WebSocketClient;
class LongPolling;

// Monitor wakes up every x milliseconds, to be sure if we missed an event
#define MONITOR_INTERVAL_MAX  10000
#define MONITOR_INTERVAL_MIN    100
// Polling interval for opening a SSE channel
#define SSE_OPEN_INTERVAL       100
#define SSE_OPEN_RETRIES        100
// Waiting for the end of the monitor thread
#define MONITOR_END_INTERVAL    100
#define MONITOR_END_RETRIES     100

class ClientEventDriver
{
public:
  ClientEventDriver();
 ~ClientEventDriver();

  // OPTIONAL: Set your thread pool
  void  SetThreadPool(ThreadPool* p_pool);
  // OPTIONAL: Set your logfile
  void  SetLogfile(LogAnalysis* p_logfile);
  // MANDATORY: Setting the application for which we work
  void  SetApplicationCallback(LPFN_EVENTCALLBACK p_callback,void* p_object);
  // OPTIONAL: Starting the driver in one go for the first (or only) session!
  bool  StartEventDriver(CString p_url,EVChannelPolicy p_policy,CString p_session,CString p_cookie,CString p_cookieValue);
  // OPTIONAL: Set policy to change on next re-connect
  void  SetChannelPolicy(EVChannelPolicy p_policy);
  // OPTIONAL: Set server URL to change on next re-connect
  void  SetServerURL(CString p_url);
  // OPTIONAL: Start with session (Call SetChannelPolicy and SetServerURL first)
  bool  StartEventsForSession(CString p_session,CString p_cookie,CString p_cookieValiue);
  // OPTIONAL: Stop an events session
  bool  StopEventsForSession();

  // Post application event to the server (WebSocket/polling only!!)
  void  PostEventToServer(LTEvent* p_event);
  // Event is incoming from socket/SSE/polling (only called internally)
  void  RegisterIncomingEvent(LTEvent* p_event,bool p_doLog = false);
  // Main loop of the event runner
  void  EventThreadRunning();
  // Is the event driver running?
  bool  GetIsRunning();
private:
  // Start a thread for the streaming websocket/server-push event interface
  bool  StartEventThread();
  // Sending to the application
  int   SendChannelsToApplication();
  int   SendChannelsToServer();
  void  RecalculateInterval(int p_sent);
  // Starting dispatcher + channels
  bool  StartDispatcher();
  bool  StartSocketChannel();
  bool  StartEventsChannel();
  bool  StartPollingChannel();
  // Stopping the channels
  void  StopSocketChannel();
  void  StopEventsChannel();
  void  StopPollingChannel();
  // Sending to the server
  void  SendToServer         (LTEvent* p_event);
  // Test if channels still open
  bool  TestDispatcher();
  // Closing everything down
  bool  CloseDown();

  CString          m_serverURL;
  CString          m_session;
  CString          m_cookie;
  CString          m_secret;
  bool             m_running    { false };
  bool             m_closeSeen  { false };
  EVChannelPolicy  m_policy     { EVChannelPolicy::DP_Disconnected };
  WebSocketClient* m_websocket  { nullptr };
  EventSource*     m_source     { nullptr };
  LongPolling*     m_polling    { nullptr };
  ThreadPool*      m_pool       { nullptr };
  LogAnalysis*     m_logfile    { nullptr };
  HANDLE           m_event      { NULL };
  HANDLE           m_thread     { NULL };
  int              m_outNumber  { 0    };
  int              m_inNumber   { 0    };
  int              m_interval   { MONITOR_INTERVAL_MIN };
  EventQueue       m_inQueue;
  EventQueue       m_outQueue;
  HTTPClient       m_client;
  CRITICAL_SECTION m_lockQueue;
  CRITICAL_SECTION m_lockChannel;
  // Callback for the application where the events will go
  LPFN_EVENTCALLBACK m_callback { nullptr };
  void*              m_object   { nullptr };
};
