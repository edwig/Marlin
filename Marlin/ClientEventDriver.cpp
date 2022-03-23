/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: ClientEventDriver.cpp
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
#include "stdafx.h"
#include "ClientEventDriver.h"
#include "WebSocketClient.h"
#include "EventSource.h"
#include "LongPolling.h"
#include "AutoCritical.h"
#include "LogAnalysis.h"
#include "Base64.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// Logging via the client
#define DETAILLOG1(text)        if(m_logfile) m_logfile->AnalysisLog(__FUNCTION__,LogType::LOG_INFO,false,text)
#define DETAILLOGS(text,extra)  if(m_logfile) m_logfile->AnalysisLog(__FUNCTION__,LogType::LOG_INFO,true,text,extra)
#define DETAILLOGV(text,...)    if(m_logfile) m_logfile->AnalysisLog(__FUNCTION__,LogType::LOG_INFO,true,text,__VA_ARGS__)
#define WARNINGLOG(text,...)    if(m_logfile) m_logfile->AnalysisLog(__FUNCTION__,LogType::LOG_WARN,true,text,__VA_ARGS__)
#define ERRORLOG(code,text)     if(m_logfile) m_logfile->AnalysisLog(__FUNCTION__,LogType::LOG_ERROR,true,text,code)

ClientEventDriver::ClientEventDriver()
{
  InitializeCriticalSection(&m_lockQueue);
  InitializeCriticalSection(&m_lockChannel);
}

ClientEventDriver::~ClientEventDriver()
{
  CloseDown();
  DeleteCriticalSection(&m_lockQueue);
  DeleteCriticalSection(&m_lockChannel);
}

// Setting the application for which we work
// This will be the dispatcher that we use.
void
ClientEventDriver::SetApplicationCallback(LPFN_EVENTCALLBACK p_callback, void* p_object)
{
  m_callback = p_callback;
  m_object   = p_object;
  
  DETAILLOG1("Application callback is set.");
}

// Starting the driver in one go!
bool
ClientEventDriver::StartEventDriver(XString p_url,EVChannelPolicy p_policy,XString p_session,XString p_cookie,XString p_cookieValue)
{
  m_serverURL = p_url;
  m_policy    = p_policy;
  m_session   = p_session;
  m_cookie    = p_cookie;
  m_secret    = p_cookieValue;

  if(m_callback && m_object)
  {
    DETAILLOGV("EventDriver started for [%s:%s] for session [%s:%s=%s]"
              ,m_serverURL.GetString()
              ,LTEvent::ChannelPolicyToString(m_policy).GetString()
              ,m_session.GetString()
              ,m_cookie.GetString()
              ,m_secret.GetString());
    return TestDispatcher();
  }
  return false;
}

// Set policy to change on next re-connect
void  
ClientEventDriver::SetChannelPolicy(EVChannelPolicy p_policy)
{
  m_policy = p_policy;
  DETAILLOGV("Set channel policy to: %s", LTEvent::ChannelPolicyToString(p_policy).GetString());
}

// Set server URL to change on next re-connect
void  
ClientEventDriver::SetServerURL(XString p_url)
{
  m_serverURL = p_url;
  DETAILLOGV("Set event URL to: %s",m_serverURL.GetString());
}

// Start with session (Call SetChannelPolicy and SetServerURL first)
bool
ClientEventDriver::StartEventsForSession(XString p_session,XString p_cookie,XString p_cookieValue)
{
  if(m_policy   != EVChannelPolicy::DP_NoPolicy &&
    !m_serverURL.IsEmpty() && 
     m_callback != nullptr &&
     m_object   != nullptr )
  {
    DETAILLOGV("Start event channel for [%s:%s=%s]",m_session.GetString(),m_cookie.GetString(),m_secret.GetString());

    AutoCritSec lock(&m_lockChannel);

    StopEventsForSession();
    m_session = p_session;
    m_cookie  = p_cookie;
    m_secret  = p_cookieValue;
    return TestDispatcher();
  }
  return false;
}

// OPTIONAL: Stop an events session
bool
ClientEventDriver::StopEventsForSession()
{
  DETAILLOGV("Stop event channel for [%s:%s=%s]", m_session.GetString(), m_cookie.GetString(), m_secret.GetString());
  
  if(!m_closeSeen)
  {
    LTEvent* event = new LTEvent(EvtType::EV_Close);
    try
    {
    (*m_callback)(m_object,event);
    }
    catch(StdException& ex)
    {
      ERRORLOG(ERROR_APPEXEC_INVALID_HOST_STATE,ex.GetErrorMessage());
    }
    m_closeSeen = true;
  }
  m_session.Empty();
  m_cookie .Empty();
  m_secret .Empty();

  return CloseDown();
}

// OPTIONALLY: Set your thread pool
void
ClientEventDriver::SetThreadPool(ThreadPool* p_pool)
{
  m_pool = p_pool;
  DETAILLOG1("Thread pool set");
}

// OPTIONALLY: Set your logfile
void
ClientEventDriver::SetLogfile(LogAnalysis* p_logfile)
{
  m_logfile = p_logfile;
}

// Post application event to the server (WebSocket/polling only!!)
void
ClientEventDriver::PostEventToServer(LTEvent* p_event)
{
  AutoCritSec lock(&m_lockQueue);
  p_event->m_number = ++m_outNumber;
  m_outQueue.push_back(p_event);

  // Kick the event thread, sending to the server
  ::SetEvent(m_event);
  DETAILLOGV("Post event [%d] to server",m_outNumber);
}

// Event is incoming from socket/SSE/polling
void
ClientEventDriver::RegisterIncomingEvent(LTEvent* p_event,bool p_doLog /*=false*/)
{
  AutoCritSec lock(&m_lockQueue);

  if(p_event->m_number <= 0)
  {
    p_event->m_number = ++m_inNumber;
  }
  DETAILLOGV("Client register incoming event: %d",p_event->m_number);
  if(p_doLog)
  {
    // Logging for long-polling
    DETAILLOGV("Register [%s] event [%d:%s]",LTEvent::EventTypeToString(p_event->m_type).GetString(),m_inNumber,p_event->m_payload.GetString());
  }

  if(m_thread && m_event) 
  {
    // Keep event
    m_inQueue.push_back(p_event);
    // Kick the event thread, sending to the application
    ::SetEvent(m_event);
    return;
  }
  // LOG!! Incoming event without a running event driver.
  //       Mostly the 'OnClose' message
  DETAILLOGV("Lost incoming event [%d:%s] [%s]"
             ,p_event->m_number
             ,LTEvent::EventTypeToString(p_event->m_type).GetString()
             ,p_event->m_payload.GetString());
  if(m_callback)
  {
    try
    {
      (*m_callback)(m_object,p_event);
    }
    catch(StdException& ex)
    {
      ERRORLOG(ERROR_APPEXEC_INVALID_HOST_STATE,ex.GetErrorMessage());
    }
  }
  else
  {
    delete p_event;
  }
}

bool
ClientEventDriver::GetIsRunning()
{
  return m_running;
}

//////////////////////////////////////////////////////////////////////////
//
// PRIVATE
//
//////////////////////////////////////////////////////////////////////////

bool
ClientEventDriver::CloseDown()
{
  DETAILLOG1("Stopping the event driver monitor");

  // Ready
  m_running = false;

  // Free the worker thread
  ::SetEvent(m_event);

  // wait for the queue to stop
  for(int waiting = 0;waiting++ < MONITOR_END_RETRIES;++waiting)
  {
    Sleep(MONITOR_END_INTERVAL);
    if(!m_thread)
    {
      break;
    }
  }
  if(m_thread)
  {
    TerminateThread(m_thread, 3);
    m_thread = NULL;
  }

  DETAILLOG1("Stopping the event channels");

  // Close websocket
  if(m_websocket)
  {
    StopSocketChannel();
    delete m_websocket;
    m_websocket = nullptr;
  }

  // Stop the SSE channel
  if(m_source)
  {
    m_client.StopClient();
    m_source = nullptr;
  }

  // Stop the Long Polling
  if(m_polling)
  {
    m_polling->StopLongPolling();
    delete m_polling;
    m_polling = nullptr;
  }

  // Reset event counters
  m_inNumber  = 0;
  m_outNumber = 0;

  // Success if monitor is stopped indeed
  return (m_thread == NULL);
}

bool
ClientEventDriver::StartDispatcher()
{
  bool result = false;

  // Make sure we have a worker bee.
  if(!m_thread)
  {
    StartEventThread();
  }

  DETAILLOGV("Starting event monitor in [%s] policy",LTEvent::ChannelPolicyToString(m_policy).GetString());

  m_closeSeen = false;
  switch(m_policy)
  {
    case EVChannelPolicy::DP_NoPolicy:      break;
    case EVChannelPolicy::DP_Binary:        result = StartSocketChannel();  break;
    case EVChannelPolicy::DP_Disconnected:  result = StartPollingChannel(); break;
    case EVChannelPolicy::DP_HighSecurity:  result = StartEventsChannel();  break;
    case EVChannelPolicy::DP_Immediate_S2C: result = StartSocketChannel();
                                            if(!result)
                                            {
                                              result = StartEventsChannel();
                                            }
                                            break;
    case EVChannelPolicy::DP_TwoWayMessages:result = StartSocketChannel();
                                            if(!result)
                                            {
                                              result = StartPollingChannel();
                                            }
                                            break;
    case EVChannelPolicy::DP_NoSockets:     result = StartEventsChannel();
                                            if(!result)
                                            {
                                              result = StartPollingChannel();
                                            }
                                            break;
    case EVChannelPolicy::DP_SureDelivery:  result = StartSocketChannel();
                                            if(!result)
                                            {
                                              result = StartEventsChannel();
                                              if(!result)
                                              {
                                                result = StartPollingChannel();
                                              }
                                            }
                                            break;
    default:                                break;
  }
  return result;
}

bool
ClientEventDriver::TestDispatcher()
{
  AutoCritSec lock(&m_lockChannel);

  bool restart = false;

  if(m_websocket)
  {
    restart = !m_websocket->IsOpenForReading();
  }
  if(m_source)
  {
    restart = m_source->GetReadyState() != OPEN;
  }
  if(m_polling)
  {
    restart = !m_polling->GetIsReceiving();
  }

  if(!m_websocket && !m_source && !m_polling)
  {
    if(!m_session.IsEmpty() && !m_cookie.IsEmpty() && !m_secret.IsEmpty())
    {
      restart = true;
    }
  }

  if(restart)
  {
    DETAILLOG1("Re-Starting the client-event-driver");
    CloseDown();
    return StartDispatcher();
  }
  return true;
}

//////////////////////////////////////////////////////////////////////////
//
// WEBSOCKETS
//
//////////////////////////////////////////////////////////////////////////

static void OnWebsocketOpen(WebSocket* p_socket, WSFrame* p_event)
{
  ClientEventDriver* driver = reinterpret_cast<ClientEventDriver*>(p_socket->GetApplication());

  LTEvent* event = new LTEvent();
  event->m_type    = EvtType::EV_Open;
  event->m_sent    = 0;
  event->m_number  = 0;
  event->m_payload = p_event->m_data;

  driver->RegisterIncomingEvent(event);
}

static void OnWebsocketMessage(WebSocket* p_socket, WSFrame* p_event)
{
  ClientEventDriver* driver = reinterpret_cast<ClientEventDriver*>(p_socket->GetApplication());

  LTEvent* event = new LTEvent();
  event->m_type    = EvtType::EV_Message;
  event->m_sent    = 0;
  event->m_number  = 0;
  event->m_payload = p_event->m_data;

  driver->RegisterIncomingEvent(event);
}

static void OnWebsocketBinary(WebSocket* p_socket, WSFrame* p_event)
{
  ClientEventDriver* driver = reinterpret_cast<ClientEventDriver*>(p_socket->GetApplication());

  LTEvent* event = new LTEvent();
  event->m_type    = EvtType::EV_Binary;
  event->m_sent    = 0;
  event->m_number  = 0;
  // Put the binary buffer forcibly in a XString!
  char* buf = event->m_payload.GetBufferSetLength(p_event->m_length);
  memcpy_s(buf,p_event->m_length,(const char*)p_event->m_data,p_event->m_length);
  event->m_payload.ReleaseBufferSetLength(p_event->m_length);

  driver->RegisterIncomingEvent(event);
}

static void OnWebsocketError(WebSocket* p_socket, WSFrame* p_event)
{
  ClientEventDriver* driver = reinterpret_cast<ClientEventDriver*>(p_socket->GetApplication());

  LTEvent* event = new LTEvent();
  event->m_type    = EvtType::EV_Error;
  event->m_sent    = 0;
  event->m_number  = 0;
  event->m_payload = p_event->m_data;

  driver->RegisterIncomingEvent(event);
}

static void OnWebsocketClose(WebSocket* p_socket, WSFrame* p_event)
{
  ClientEventDriver* driver = reinterpret_cast<ClientEventDriver*>(p_socket->GetApplication());

  LTEvent* event = new LTEvent();
  event->m_type    = EvtType::EV_Close;
  event->m_sent    = 0;
  event->m_number  = 0;
  event->m_payload = p_event->m_data;

  driver->RegisterIncomingEvent(event);
}

bool
ClientEventDriver::StartSocketChannel()
{
  XString url = m_serverURL + "Sockets/" + m_session;
//   url.Replace("http://", "ws://");
//   url.Replace("https://","wss://");

  // Create client side of the websocket and connect to this driver
  m_websocket = new WebSocketClient(url);
  m_websocket->SetApplication(this);

  // Connect the logfile
  if(m_logfile)
  {
    m_websocket->SetLogfile(m_logfile);
    m_websocket->SetLogLevel(m_logfile->GetLogLevel());
  }

  // Connect our handlers
  m_websocket->SetOnOpen   (OnWebsocketOpen);
  m_websocket->SetOnMessage(OnWebsocketMessage);
  m_websocket->SetOnBinary (OnWebsocketBinary);
  m_websocket->SetOnError  (OnWebsocketError);
  m_websocket->SetOnClose  (OnWebsocketClose);

  // Add authorization
  XString value = m_cookie + "=" + m_secret;
  m_websocket->AddHeader("Cookie",value);

  DETAILLOGV("Staring WebSocket channel on [%s]",url.GetString());

  // Start the socket by opening
  // Receiving thread is now running on the HTTPClient
  return m_websocket->OpenSocket();
}

//////////////////////////////////////////////////////////////////////////
//
// Server-Sent-Event channel (SSE)
//
//////////////////////////////////////////////////////////////////////////

static void OnEventOpen(ServerEvent* p_event,void* p_data)
{
  ClientEventDriver* driver = reinterpret_cast<ClientEventDriver*>(p_data);
  if(driver)
  {
    LTEvent* event = new LTEvent();
    event->m_type    = EvtType::EV_Open;
    event->m_sent    = 0;
    event->m_number  = p_event->m_id;
    event->m_payload = p_event->m_data;

    driver->RegisterIncomingEvent(event);
  }
  delete p_event;
}

static void OnEventMessage(ServerEvent* p_event,void* p_data)
{
  ClientEventDriver* driver = reinterpret_cast<ClientEventDriver*>(p_data);
  if(driver)
  {
    LTEvent* event = new LTEvent();
    event->m_type    = EvtType::EV_Message;
    event->m_sent    = 0;
    event->m_number  = p_event->m_id;
    event->m_payload = p_event->m_data;

    driver->RegisterIncomingEvent(event);
  }
  delete p_event;
}

static void OnEventBinary(ServerEvent* p_event, void* p_data)
{
  ClientEventDriver* driver = reinterpret_cast<ClientEventDriver*>(p_data);
  if (driver)
  {
    LTEvent* event   = new LTEvent();
    event->m_type    = EvtType::EV_Binary;
    event->m_sent    = 0;
    event->m_number  = p_event->m_id;
    event->m_payload = Base64::Decrypt(p_event->m_data);

    driver->RegisterIncomingEvent(event);
  }
  delete p_event;
}

static void OnEventError(ServerEvent* p_event,void* p_data)
{
  ClientEventDriver* driver = reinterpret_cast<ClientEventDriver*>(p_data);
  if(driver)
  {
    LTEvent* event = new LTEvent();
    event->m_type    = EvtType::EV_Error;
    event->m_sent    = 0;
    event->m_number  = p_event->m_id;
    event->m_payload = p_event->m_data;

    driver->RegisterIncomingEvent(event);
  }
  delete p_event;
}

static void OnEventClose(ServerEvent* p_event,void* p_data)
{
  ClientEventDriver* driver = reinterpret_cast<ClientEventDriver*>(p_data);
  if(driver)
  {
    LTEvent* event = new LTEvent();
    event->m_type    = EvtType::EV_Close;
    event->m_sent    = 0;
    event->m_number  = p_event->m_id;
    event->m_payload = p_event->m_data;

    driver->RegisterIncomingEvent(event);
  }
  delete p_event;
}

bool
ClientEventDriver::StartEventsChannel()
{
  XString url = m_serverURL + "Events/" + m_session;
  m_source = m_client.CreateEventSource(url);

  // Until server tells us otherwise, wait 2 seconds before each reconnect
  m_source->SetReconnectionTime(2000);
  m_source->SetDirectMessage(true);   // We will handle parallel messages
  m_source->SetApplicationData(this); // Return to this object

  // Set the standard SSE handlers (NO binary handler!)
  m_source->m_onopen    = OnEventOpen;
  m_source->m_onmessage = OnEventMessage;
  m_source->m_onerror   = OnEventError;
  m_source->m_onclose   = OnEventClose;
  // Not a standard listener: Implement binary ourselves
  m_source->AddEventListener("binary",OnEventBinary);

  // Set our thread pool if any 
  // Otherwise the event source will create a new one
  if(m_pool)
  {
    m_source->SetThreadPool(m_pool);
  }
  // Set our logfile, otherwise we will have NO logging
  if(m_logfile)
  {
    m_client.SetLogging(m_logfile);
  }
  // Add cookie to the channel
  m_source->SetSecurity(m_cookie,m_secret);

  DETAILLOGV("Staring SSE channel on [%s]",url.GetString());

  // Start SSE and wait till it opens (max 10 seconds)
  if(m_source->EventSourceInit(true))
  {
    int waiting = 0;
    while (m_source->GetReadyState() != OPEN)
    {
      Sleep(SSE_OPEN_INTERVAL);
      if(++waiting > SSE_OPEN_RETRIES)
      {
        break;
      }
    }
  }
  return m_source->GetReadyState() == OPEN;
}

//////////////////////////////////////////////////////////////////////////
//
// LONG POLLING
//
//////////////////////////////////////////////////////////////////////////

void OnPollingEvent(void* p_application,LTEvent* p_event)
{
  ClientEventDriver* driver = reinterpret_cast<ClientEventDriver*>(p_application);
  if(driver)
  {
    driver->RegisterIncomingEvent(p_event,true);
    return;
  }
  delete p_event;
}

bool
ClientEventDriver::StartPollingChannel()
{
  XString url = m_serverURL + "Polling/" + m_session;

  m_polling = new LongPolling();
  m_polling->SetURL(url);
  m_polling->SetApplication(OnPollingEvent,this);
  m_polling->SetLogfile(m_logfile);

  DETAILLOGV("Starting long-polling channel on [%s]",url.GetString());

  return m_polling->StartLongPolling(m_session,m_cookie,m_secret);
}

//////////////////////////////////////////////////////////////////////////
// 
// CLIENT is sending event TO the server
//
//////////////////////////////////////////////////////////////////////////

// Sending to the server
void  
ClientEventDriver::SendToServer(LTEvent* p_event)
{
  if(m_websocket)
  {
    switch(p_event->m_type)
    {
      case EvtType::EV_Message: m_websocket->WriteString(p_event->m_payload);
                                break;
      case EvtType::EV_Binary:  m_websocket->WriteObject((BYTE*)p_event->m_payload.GetString(),p_event->m_payload.GetLength());
                                break;
      case EvtType::EV_Close:   m_websocket->SendCloseSocket(WS_CLOSE_NORMAL,p_event->m_payload);
                                // Restart channel!!
                                TestDispatcher(); 
                                break;
    }
  }
  else if(m_polling)
  {
    m_polling->PostEvent(p_event);
  }
  else // m_source or no channel
  {
    // LOG: Cannot send
  }
  // Event was taken from the queue, must delete now
  delete p_event;
}

//////////////////////////////////////////////////////////////////////////
//
// EVENT HANDLING
//
//////////////////////////////////////////////////////////////////////////

static unsigned int __stdcall StartingTheDriverThread(void* p_context)
{
  ClientEventDriver* driver = reinterpret_cast<ClientEventDriver*>(p_context);
  if(driver)
  {
    driver->EventThreadRunning();
  }
  return 0;
}

// Start a thread for the streaming websocket/server-push event interface
bool
ClientEventDriver::StartEventThread()
{
  // Create an event for the monitor
  if(!m_event)
  {
    m_event = CreateEvent(NULL,FALSE,FALSE,NULL);
  }

  if(m_thread == NULL || m_running == false)
  {
    // Thread for the client queue
    unsigned int threadID = 0;
    if((m_thread = (HANDLE)_beginthreadex(NULL,0,StartingTheDriverThread,(void*)(this),0,&threadID)) == INVALID_HANDLE_VALUE)
    {
      m_thread = NULL;
      threadID = 0;
      ERRORLOG(ERROR_SERVICE_NOT_ACTIVE,"Code [%d] Cannot start a thread for an ClientEventDriver.");
    }
    else
    {
      DETAILLOGV("Thread started with threadID [%d] for ClientEventDriver.",threadID);
      return true;
    }
  }
  return false;
}

// Main loop of the event runner
void
ClientEventDriver::EventThreadRunning()
{
  // Installing our SEH to exception translator
  _set_se_translator(SeTranslator);

  // Tell we are running
  m_running  = true;
  m_interval = MONITOR_INTERVAL_MIN;

  DETAILLOG1("ServerEventDriver monitor started.");
  do
  {
    int sent = 0;
    // Wake every now and then and send events to the application
    DWORD waited = WaitForSingleObjectEx(m_event,m_interval,true);
    switch(waited)
    {
      case WAIT_TIMEOUT:        // Wake up once-in-a-while to be sure
      case WAIT_OBJECT_0:       // Explicit wake up by posting an event
                                if(m_running)
                                {
                                  sent += SendChannelsToApplication();
                                  sent += SendChannelsToServer();
                                  RecalculateInterval(sent);
                                }
                                break;
      case WAIT_IO_COMPLETION:  // Some I/O completed
      case WAIT_FAILED:         // Failed for some reason
      case WAIT_ABANDONED:      // Interrupted for some reason
      default:                  // Start a new monitor / new event
                                break;
    }
    // Test is channels still open...
    if(m_running)
    {
      TestDispatcher();
    }
  }
  while(m_running);

  // Reset the monitor
  CloseHandle(m_event);
  m_event = NULL;

  // Thread is now ready
  m_thread = NULL;
}

void
ClientEventDriver::RecalculateInterval(int p_sent)
{
  if(p_sent)
  {
    m_interval = MONITOR_INTERVAL_MIN;
  }
  else
  {
    // Queue idle, wait longer
    m_interval *= 2;
    if(m_interval > MONITOR_INTERVAL_MAX)
    {
      m_interval = MONITOR_INTERVAL_MAX;
    }
  }
  DETAILLOGV("Monitor going to sleep for [%d] milliseconds",m_interval);
}

// Sending to the server
int
ClientEventDriver::SendChannelsToServer()
{
  int sent = 0;

  // Works only for WebSockets or Long-Polling
  if(!m_websocket && !m_polling)
  {
    return sent;
  }

  // Loop through the out-queue
  while (true)
  {
    LTEvent* event = nullptr;
    // Scope for temporary lock on the queue
    {
      AutoCritSec lock(&m_lockQueue);
      if(!m_outQueue.empty())
      {
        event = m_outQueue.front();
        m_outQueue.pop_front();
      }
      else
      {
        break;
      }
    }
    if(event)
    {
      // Send this event to the server
      SendToServer(event);
      ++sent;
    }
    // Possibly stopped by the application
    if (!m_running)
    {
      break;
    }
  }
  DETAILLOGV("Sent [%d] events to the server",sent);
  return sent;
}

// Sending to the application
int
ClientEventDriver::SendChannelsToApplication()
{
  int sent = 0;

  // Loop through the in-queue
  while(true)
  {
    LTEvent* event = nullptr;
    // Scope for temporary lock on the queue
    {
      AutoCritSec lock(&m_lockQueue);
      if(!m_inQueue.empty())
      {
        event = m_inQueue.front();
        m_inQueue.pop_front();
      }
      else
      {
        break;
      }
    }
    if(event)
    {
      if(event->m_type == EvtType::EV_Close)
      {
        m_closeSeen = true;
      }
      try
      {
      // Callback the application with the event
      (*m_callback)(m_object, event);
      ++sent;
      }
      catch(StdException& ex)
      {
        ERRORLOG(ERROR_APPEXEC_INVALID_HOST_STATE,ex.GetErrorMessage());
      }
    }
    // Possibly stopped by the application
    if(!m_running)
    {
      break;
    }
  }
  DETAILLOGV("Sent [%d] events to the application", sent);
  return sent;
}

//////////////////////////////////////////////////////////////////////////
//
// STOPPING
//
//////////////////////////////////////////////////////////////////////////

void
ClientEventDriver::StopSocketChannel()
{
  m_websocket->SendCloseSocket(WS_CLOSE_NORMAL,"ClientEventDriver closes socket");
}

void
ClientEventDriver::StopEventsChannel()
{
  m_client.Reset();
}

void
ClientEventDriver::StopPollingChannel()
{
  m_polling->StopLongPolling();
}
