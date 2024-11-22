/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: LongPolling.cpp
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
#include "LongPolling.h"
#include "LogAnalysis.h"
#include "SOAPMessage.h"
#include "AutoCritical.h"
#include "Base64.h"

#ifdef _AFX
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#endif

// Logging via the client
#define DETAILLOG1(text)        if(m_logfile) m_logfile->AnalysisLog(_T(__FUNCTION__),LogType::LOG_INFO,false,text)
#define DETAILLOGS(text,extra)  if(m_logfile) m_logfile->AnalysisLog(_T(__FUNCTION__),LogType::LOG_INFO,true, text,extra)
#define DETAILLOGV(text,...)    if(m_logfile) m_logfile->AnalysisLog(_T(__FUNCTION__),LogType::LOG_INFO,true, text,__VA_ARGS__)
#define WARNINGLOG(text,...)    if(m_logfile) m_logfile->AnalysisLog(_T(__FUNCTION__),LogType::LOG_WARN,true, text,__VA_ARGS__)
#define ERRORLOG(code,text)     if(m_logfile) m_logfile->AnalysisLog(_T(__FUNCTION__),LogType::LOG_ERROR,true,text,code)

static XString polling_namespace = _T("http://www.marlin.org/polling");

LongPolling::LongPolling()
{
  InitializeCriticalSection(&m_lock);
}

LongPolling::~LongPolling()
{
  Reset();
  DeleteCriticalSection(&m_lock);
}

bool
LongPolling::StartLongPolling(XString p_session,XString p_cookie,XString p_secret)
{
  m_session = p_session;
  m_cookie  = p_cookie;
  m_secret  = p_secret;

  // Check for minimum requirements
  if(m_url.IsEmpty() || m_application == nullptr)
  {
    return false;
  }

  DETAILLOGV(_T("Starting long-polling on: %s"),m_url.GetString());

  // Start polling thread and fire first message
  if(StartPollingThread())
  {
    ::SetEvent(m_event);
    return true;
  }
  return false;
}

// Stopping the long polling channel
void 
LongPolling::StopLongPolling()
{
  DETAILLOG1(_T("Stopping the long-polling: Send closing message."));
  int status = SendCloseMessage();
  // Log the status
  DETAILLOGV(_T("Long polling closed with status: %d"),status);

  // Kick the worker bee out of work
  m_receiving = false;
  ::SetEvent(m_event);

  // Wait for end of the action
  for(int ind = 0; ind < MONITOR_END_RETRIES; ++ind)
  {
    Sleep(MONITOR_END_INTERVAL);
    if(m_thread == NULL)
    {
      break;
    }
  }
  if(m_thread)
  {
    // Since waiting on the thread did not work, we must preemptively terminate it.
#pragma warning(disable:6258)
    TerminateThread(m_thread,3);
    m_thread = NULL;
  }
}

void
LongPolling::SetURL(XString p_url)
{
  m_url = p_url;
  DETAILLOGV(_T("Registered long-polling URL: %s"),m_url.GetString());
}

void 
LongPolling::SetApplication(LPFN_EVENTCALLBACK p_callback,void* p_application)
{
  m_callback    = p_callback;
  m_application = p_application;
  if(m_callback && m_application)
  {
    DETAILLOG1(_T("Registered application callback."));
  }
}

void 
LongPolling::SetLogfile(LogAnalysis* p_logfile)
{
  m_logfile = p_logfile;
  m_client.SetLogging(m_logfile);
}

bool 
LongPolling::GetIsReceiving()
{
  return m_receiving;
}

void 
LongPolling::RegisterEvent(XString p_payload,EvtType p_type,int p_number /*=0*/)
{
  LTEvent* event = new LTEvent();
  event->m_payload = p_payload;
  event->m_type    = p_type;
  event->m_number  = p_number;
  event->m_sent    = 0;

  DETAILLOGV(_T("Register incoming event [%d] %s"),p_number,p_payload.GetString());

  if(m_callback && m_application)
  {
    // Keep the score of the last number
    if(p_number > m_lastNumber)
    {
      m_lastNumber = p_number;
    }
    // Sent to the driver
    (*m_callback)(m_application,event);
    return;
  }
  // Not sent, so destroy again
  delete event;
}

// Application posting an event to the server
void 
LongPolling::PostEvent(LTEvent* p_event)
{
  DETAILLOGV(_T("Post event to server: %s"),p_event->m_payload.GetString());

  switch(p_event->m_type)
  {
    case EvtType::EV_Open:    // [[fallthrough]]
    case EvtType::EV_Message: // [[fallthrough]]
    case EvtType::EV_Error:   AskForMessages(p_event);
                              break;
    case EvtType::EV_Binary:  // Cannot send a binary
                              break;
    case EvtType::EV_Close:   // Sending a close message
                              StopLongPolling();
                              break;
  }
}

//////////////////////////////////////////////////////////////////////////
//
// PRIVATE
//
//////////////////////////////////////////////////////////////////////////

void
LongPolling::Reset()
{
  if(m_receiving)
  {
    StopLongPolling();
  }
  m_url.Empty();
  m_application = nullptr;
  m_interval    = 0;
  m_receiving   = false;
  m_logfile     = nullptr;
  m_lastNumber  = 0;

  m_client.Reset();
}

// Ask the server for more messages in the queue for this client
// Returns magic code to the caller
// -1  : Error occurred in the HTTP handling
// 0   : Legal messages received. Call again. Maybe more messages
// 1   : Empty status received. Kick the interval to wait longer
// 2   : Legal close-channel message (end of session)
PollStatus
LongPolling::AskForMessages(LTEvent* p_event /*=nullptr*/)
{
  AutoCritSec lock(&m_lock);

  XString url(m_url);
  XString action(_T("GetMessage"));
  url += _T("/") + action;
  SOAPMessage msg(polling_namespace,action,SoapVersion::SOAP_12,url);
  msg.SetParameter(_T("Acknowledged"),m_lastNumber);
  msg.SetCookie(m_cookie,m_secret);

  // Add an optional message to the server
  if(p_event)
  {
    msg.SetParameter(_T("Type"),   LTEvent::EventTypeToString(p_event->m_type));
    if(p_event->m_type == EvtType::EV_Binary)
    {
      Base64 base;
      msg.SetParameter(_T("Message"),base.Encrypt(p_event->m_payload));
    }
    else
    {
      msg.SetParameter(_T("Message"), p_event->m_payload);
    }
  }

  DETAILLOG1(_T("Asking server for message."));

  bool result = m_client.Send(&msg);
  if(result)
  {
    if(msg.GetFaultActor().IsEmpty())
    {
      // Legal answer received!
      bool empty = msg.GetParameterBoolean(_T("Empty"));
      int number = msg.GetParameterInteger(_T("Number"));
      XString eventType = msg.GetParameter(_T("Type"));
      XString payload   = msg.GetParameter(_T("Message"));
      EvtType type      = LTEvent::StringToEventType(eventType);

      // OK. End of queue reached. Wait longer
      if(empty)
      {
        return PollStatus::PS_Empty;
      }
      if(!m_openSeen && type != EvtType::EV_Open)
      {
        RegisterEvent(_T(""),EvtType::EV_Open,0);
        m_openSeen = true;
      }
      // Post event
      RegisterEvent(payload,type,number);
      // Ready state after an event
      return type == EvtType::EV_Close ? PollStatus::PS_Closing : PollStatus::PS_Received;
    }
    else
    {
      // Error in handling SOAP message
      XString fault = msg.GetFault();
      RegisterEvent(fault,EvtType::EV_Error);
    }
  }
  else
  {
    // No answer or channel now closed
    XString error,message;
    m_client.GetError(&error);
    message.Format(_T("Error while asking for message. HTTP status [%d] %s"),m_client.GetStatus(),error.GetString());
    RegisterEvent(message,EvtType::EV_Error);
  }
  // Error occurred in HTTP channel
  return PollStatus::PS_Error;
}

// Sending last closing message
// DOES NOT HANDLE INCOMING PAYLOAD!!
// 1: Correctly closed
// 0: Unexpected answer, but closed
// -1: Message incorrectly handled, could be closed on server side
int
LongPolling::SendCloseMessage()
{
  AutoCritSec lock(&m_lock);

  XString url(m_url);
  XString action(_T("GetMessage"));
  url += _T("/") + action;
  SOAPMessage msg(polling_namespace,action,SoapVersion::SOAP_12,url);
  msg.SetParameter(_T("Acknowledged"),m_lastNumber);
  msg.SetParameter(_T("CloseChannel"), true);
  msg.SetCookie(m_cookie,m_secret);

  bool result = m_client.Send(&msg);
  if(result)
  {
    if(msg.GetFaultCode().IsEmpty())
    {
      // Legally closed
      bool closed = msg.GetParameterBoolean(_T("ChannelClosed"));
      return closed == true ? 1 : 0;
    }
    else
    {
      // Error in handling SOAP message
      XString fault = msg.GetFault();
      // Log fault
      return -1;
    }
  }
  else
  {
    // No answer or channel now closed
    // Which was our intention anyway!!
    return 1;
  }
}

//////////////////////////////////////////////////////////////////////////
//
// WORKER BEE
//
//////////////////////////////////////////////////////////////////////////

static unsigned int __stdcall StartingThePollingThread(void* p_context)
{
  LongPolling* polling = reinterpret_cast<LongPolling*>(p_context);
  if(polling)
  {
    polling->PollingThreadRunning();
  }
  return 0;
}

bool
LongPolling::StartPollingThread()
{
  // Create an event for the monitor
  if(!m_event)
  {
    m_event = CreateEvent(NULL,FALSE,FALSE,NULL);
  }

  if(m_thread == NULL || m_receiving == false)
  {
    // Thread for the client queue
    unsigned int threadID = 0;
    if((m_thread = reinterpret_cast<HANDLE>(_beginthreadex(NULL,0,StartingThePollingThread,reinterpret_cast<void*>(this),0,&threadID))) == INVALID_HANDLE_VALUE)
    {
      m_thread = NULL;
      threadID = 0;
      ERRORLOG(ERROR_SERVICE_NOT_ACTIVE, _T("Code [%d] Cannot start a thread for an Long-Polling channel."));
    }
    else
    {
      DETAILLOGV(_T("Thread started with threadID [%d] for Long-Polling channel."), threadID);
      return true;
    }
  }
  return false;
}

void
LongPolling::PollingThreadRunning()
{
    // Installing our SEH to exception translator
  _set_se_translator(SeTranslator);

  // Tell we are running
  m_receiving = true;
  DETAILLOG1(_T("Polling monitor started."));

  m_interval = POLL_INTERVAL_START;

  // DETAILLOG1("ServerEventDriver monitor started.");
  do
  {
    // Wake every now and then and send events to the application
    DWORD waited = WaitForSingleObjectEx(m_event,m_interval,true);
    switch(waited)
    {
      case WAIT_TIMEOUT:        // Wake up once-in-a-while to be sure
                                AskingForMessages();
                                break;
      case WAIT_OBJECT_0:       // Explicit wake up by posting an event
                                // Go check the m_receiving member!!
                                break;
      case WAIT_IO_COMPLETION:  // Some I/O completed
      case WAIT_FAILED:         // Failed for some reason
      case WAIT_ABANDONED:      // Interrupted for some reason
      default:                  // Start a new monitor / new event
                                break;
    }
  }
  while(m_receiving);

  // Reset the monitor
  CloseHandle(m_event);
  // Thread is now ready
  m_thread = NULL;

  DETAILLOG1(_T("Polling monitor stopped."));
}

void
LongPolling::AskingForMessages()
{
  PollStatus status(PollStatus::PS_Empty);

  // GO POLLING
  do
  {
    status = AskForMessages();
    if(status == PollStatus::PS_Received)
    {
      // Reset interval to begin at start at first empty
      m_interval = POLL_INTERVAL_START;
    }
  }
  while(status == PollStatus::PS_Received);

  // Handle post polling status
  if(status == PollStatus::PS_Empty)
  {
    // kick the interval to be bigger
    m_interval *= 2;
    if(m_interval > POLL_INTERVAL_MAX)
    {
      m_interval = POLL_INTERVAL_MAX;
    }
    DETAILLOGV(_T("Long-polling back in [%d] milliseconds."),m_interval);
  }
  else
  {
    // Status = PS_Closed  or PS_Error (HTTP / OS error)
    // Stopping the long-polling
    m_receiving = false;
  }
}
