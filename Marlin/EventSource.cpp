/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: EventSource.cpp
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
#include "EventSource.h"
#include "ServerEvent.h"
#include "ThreadPool.h"
#include "HTTPClient.h"
#include "LogAnalysis.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// Macro for logging
#define DETAILLOG(text) if(m_client->GetLogging()) m_client->GetLogging()->AnalysisLog(_T(__FUNCTION__),LogType::LOG_INFO, false,(text))
#define ERRORLOG(text)  if(m_client->GetLogging()) m_client->GetLogging()->AnalysisLog(_T(__FUNCTION__),LogType::LOG_ERROR,false,(text))

EventSource::EventSource(HTTPClient* p_client,XString p_url)
            :m_url(p_url)
            ,m_client(p_client)
            ,m_serialize(false)
            ,m_ownPool(false)
{
  Reset();
}

EventSource::~EventSource()
{
  if(m_pool && m_ownPool)
  {
    // Stop threadpool
    delete m_pool;
    m_pool = nullptr;
    m_ownPool = false;
  }
}

// Set an external threadpool
void
EventSource::SetThreadPool(ThreadPool* p_pool)
{
  if(m_pool && m_ownPool)
  {
    // Stop threadpool
    delete m_pool;
  }
  m_pool    = p_pool;
  m_ownPool = false;
}

bool
EventSource::EventSourceInit(bool p_withCredentials)
{
  // Remember credentials
  m_withCredentials = p_withCredentials;

  if(m_pool == nullptr)
  {
    m_pool = new ThreadPool(NUM_THREADS_MINIMUM,NUM_THREADS_MAXIMUM);
    m_ownPool = true;
  }
  // Now starting the event stream
  return m_client->StartEventStream(m_url);
}

void
EventSource::Reset()
{
  m_withCredentials  = false;
  m_readyState       = CLOSED;
  m_reconnectionTime = HTTP_RECONNECTION_TIME;
  m_lastEventID      = 0;
  m_onopen           = nullptr;
  m_onmessage        = nullptr;
  m_onerror          = nullptr;
  m_onclose          = nullptr;
  m_oncomment        = nullptr;
  m_onretry          = nullptr;

  // Clear the listeners map
  m_listeners.clear();
}

// Set to connection state for OnOpen
// On beginning or for a reconnect
void
EventSource::SetConnecting()
{
  if(m_readyState == CLOSED ||
     m_readyState == OPEN   ||
     m_readyState == CLOSED_BY_SERVER)
  {
    m_lastEventID = 0;
    m_readyState  = CONNECTING;
  }
}

// Set ready state and wait for HTTP round trip
// To close the HTTP push-event stream
void
EventSource::Close()
{
  m_readyState = CLOSED;
}

// Add event listener to the source
bool 
EventSource::AddEventListener(XString p_event,LPFN_EVENTHANDLER p_handler,bool p_useCapture /*=false*/)
{
  // Always search on lower case
  p_event.MakeLower();
  ListenerMap::iterator it = m_listeners.find(p_event);
  if(it != m_listeners.end())
  {
    // Event listener already exists
    return false;
  }

  // Handle special cases from the W3C Standard
  if(p_event.CompareNoCase(_T("onopen")) == 0) 
  {
    m_onopen = p_handler; 
    return true; 
  }
  if(p_event.CompareNoCase(_T("onerror")) == 0) 
  {
    m_onerror = p_handler;
    return true;
  }
  if(p_event.CompareNoCase(_T("onmessage")) == 0)
  {
    m_onmessage = p_handler;
    return true;
  }
  if(p_event.CompareNoCase(_T("onclose")) == 0)
  {
    m_onclose = p_handler;
    return true;
  }
  if(p_event.CompareNoCase(_T("oncomment")) == 0)
  {
    m_oncomment = p_handler;
    return true;
  }
  if(p_event.CompareNoCase(_T("onretry")) == 0)
  {
    m_onretry = p_handler;
    return true;
  }

  // Create listener and keep in the listeners map
  EventListener listener;
  listener.m_event   = p_event;
  listener.m_handler = p_handler;
  listener.m_capture = p_useCapture;

  m_listeners.insert(std::make_pair(p_event,listener));

  return true;
}

// Handle the last-event-id of a generic listener
void
EventSource::HandleLastID(const ServerEvent* p_event)
{
  if(p_event->m_id)
  {
    if(p_event->m_id > m_lastEventID)
    {
      m_lastEventID = p_event->m_id;
    }
  }
}

// Add application pointer
void
EventSource::SetApplicationData(void* p_data)
{
  m_appData = p_data;
}

void
EventSource::SetSecurity(XString p_cookie, XString p_secret)
{
  m_cookie = p_cookie;
  m_secret = p_secret;
}

//////////////////////////////////////////////////////////////////////////
//
// Default stubs for the standard listeners
//
//////////////////////////////////////////////////////////////////////////

void
EventSource::OnOpen(ServerEvent* p_event)
{
  // Handle last-event-id
  HandleLastID(p_event);

  if(m_readyState == CONNECTING)
  {
    m_readyState = OPEN;
  }
  // Handle onopen
  if(m_onopen)
  {
    (*m_onopen)(p_event,m_appData);
    return;
  }
  DETAILLOG(_T("EventSource: Unhandeled OnOpen event"));
  DETAILLOG(p_event->m_data);
  delete p_event;
}

void
EventSource::OnError(ServerEvent* p_event)
{
  // Handle last-event-id
  HandleLastID(p_event);

  // Handle on error
  if(m_onerror)
  {
    (*m_onerror)(p_event,m_appData);
    return;
  }
  DETAILLOG(_T("EventSource: Unhandeled OnError event"));
  DETAILLOG(p_event->m_data);
  delete p_event;
}

void
EventSource::OnMessage(ServerEvent* p_event)
{
  // Handle last-event-id
  HandleLastID(p_event);

  // Handle on message
  if(m_direct)
  {
    (*m_onmessage)(p_event, m_appData);
    return;
  }
  else if(m_onmessage)
  {
    // Submit to threadpool
    m_pool->SubmitWork(reinterpret_cast<LPFN_CALLBACK>(m_onmessage),reinterpret_cast<void*>(p_event));
    return;
  }
  DETAILLOG(_T("EventSource: Unhandeled OnMessage event"));
  DETAILLOG(p_event->m_data);
  delete p_event;
}

void
EventSource::OnClose(ServerEvent* p_event)
{
  // Handle last-event-id
  HandleLastID(p_event);

  // Handle on error
  if(m_onclose)
  {
    (*m_onclose)(p_event,m_appData);
    m_readyState = CLOSED_BY_SERVER;
  }
  else
  {
    m_readyState = CLOSED_BY_SERVER;
    DETAILLOG(_T("EventSource: Unhandeled OnClose event"));
    DETAILLOG(p_event->m_data);
    delete p_event;
  }
  // Do status in the client
  m_client->OnCloseSeen();
  
  m_eventName.Empty();
  m_eventData.Empty();
}

void
EventSource::OnComment(ServerEvent* p_event)
{
  // Handle last-event-id
  HandleLastID(p_event);

  // Handle oncomment
  if(m_oncomment)
  {
    (*m_oncomment)(p_event,m_appData);
    return;
  }
  // Do the default comment handler
  XString comment(_T("Eventsource. Comment data received: "));
  comment += p_event->m_data;
  DETAILLOG(comment);
  delete p_event;
}

void
EventSource::OnRetry(ServerEvent* p_event)
{
  // Handle last-event-id
  HandleLastID(p_event);

  // Handle onretry
  if(m_onretry)
  {
    (*m_onretry)(p_event,m_appData);
    return;
  }
  // Default = Log the retry
  XString retry(_T("Eventsource: Retry received: "));
  retry += p_event->m_data;
  DETAILLOG(retry);
  delete p_event;
}

//////////////////////////////////////////////////////////////////////////
//
// EVENT DATA PARSER
//
//////////////////////////////////////////////////////////////////////////

void
EventSource::Parse(BYTE* p_buffer,unsigned& p_length)
{
  // Only parse events if we have the 'open' state
  if(m_readyState != OPEN)
  {
    return;
  }
  // Getting the raw buffer
  XString buffer(p_buffer);

  // normalize lines
  if(buffer.Find('\r') >= 0)
  {
    buffer.Replace(_T("\r\n"),_T("\n"));
    buffer.Replace(_T("\n\r"),_T("\n"));
    buffer.Replace(_T("\r"),_T("\n"));
  }
  // Parse the buffer, probably shrinking it
  if(buffer.GetLength())
  {
    Parse(buffer);
  }
  // Save the last part of the buffer
  if(buffer.GetLength() == 0)
  {
    // Quick optimization: everything is processed
    p_length    = 0;
    p_buffer[0] = 0;
  }
  else if((ULONG)buffer.GetLength() < p_length)
  {
    // Place tail end of buffer back. Wait for more on HTTP connection
    memcpy_s(p_buffer,p_length,buffer.GetString(),buffer.GetLength());
    p_length = buffer.GetLength();
  }
}

// Parse buffer in string form
void
EventSource::Parse(XString& p_buffer)
{
  ULONG   id = 0;
  int     lineNo = 0;
  XString line;
  
  GetLine(p_buffer,line);

  // set the default event name
  if(m_eventName.IsEmpty())
  {
    m_eventName = _T("message");
  }

  // Try to do multiple events
  while(!line.IsEmpty())
  {
    XString field;
    XString value;
    int pos = line.Find(':');
    if(pos == 0)
    {
      XString event(_T("comment"));
      // Comment line found
      DispatchEvent(&event,0,&line);
      // Get next and continue
      GetLine(p_buffer,line);
      continue;
    }
    if(pos > 0)
    {
      field = line.Left(pos);
      value = line.Mid(pos + 1);
      // Remove 1 space at max!!
      if(value.GetAt(0) == ' ')
      {
        value = value.Mid(1);
      }
    }
    else //if(pos < 0)
    {
      field = line;
    }

    // Test for retry field
    if(field.CompareNoCase(_T("retry")) == 0)
    {
      // Retry time is in seconds, so calculate the clock ticks
      // we need to wait before reconnection.
      m_reconnectionTime = _ttoi(value) * CLOCKS_PER_SEC;
      if(m_reconnectionTime < HTTP_RECONNECTION_TIME_MINIMUM)
      {
        m_reconnectionTime = HTTP_RECONNECTION_TIME_MINIMUM;
      }
      if(m_reconnectionTime > HTTP_RECONNECTION_TIME_MAXIMUM)
      {
        m_reconnectionTime = HTTP_RECONNECTION_TIME_MAXIMUM;
      }
      DispatchEvent(&field,0,&value);
    }
    else if(field.CompareNoCase(_T("event")) == 0)
    {
      // Our event name
      m_eventName = value;
    }
    else if(field.CompareNoCase(_T("id")) == 0)
    {
      id = _ttoi(value);
      if(id == 0)
      {
        // Reset the event id
        // Works also for an empty line "id:"
        m_lastEventID = 0;
      }
      else if(id > m_lastEventID)
      {
        m_lastEventID = id;
      }
    }
    else if(field.CompareNoCase(_T("data")) == 0)
    {
      // Beginning of the data
      if(lineNo++)
      {
        m_eventData += _T("\n");
      }
      m_eventData += value;
    }
    // Get next line
    if(GetLine(p_buffer,line) == false)
    {
      // Dispatch only when we find an empty line
      DispatchEvent(&m_eventName,id,&m_eventData);
      // Reset line counter
      lineNo = 0;
      // Clear the event data: Could be another event in the stream buffer
      m_eventName.Empty();
      m_eventData.Empty();
      id = 0;

      // If buffer not yet drained, get a new line
      if(p_buffer.GetLength())
      {
        GetLine(p_buffer,line);
      }
    }
  } 
}

// Get one line from the incoming buffer
bool
EventSource::GetLine(XString& p_buffer,XString& p_line)
{
  // Test for a BOM at the beginning of the stream
  _TUCHAR* buf = reinterpret_cast<_TUCHAR*>(p_buffer.GetBuffer());
  if(*buf == 0xFE && *++buf == 0xFF)
  {
    p_buffer = p_buffer.Mid(2);
  }

  // Find newline
  int pos = p_buffer.Find('\n');
  // Could be "\n:keepalive etc"
  if(pos == 0 && p_buffer.GetLength() > 1)
  {
    p_line.Empty();
    p_buffer.TrimLeft(_T("\n"));
    return false;
  }
  if(pos > 0)
  {
    p_line   = p_buffer.Left(pos);
    p_buffer = p_buffer.Mid (pos + 1);
    return true;
  }
  // Empty the line
  p_line.Empty();

  // Last new line means dispatch it
  if(p_buffer == _T("\n"))
  {
    p_buffer.Empty();
    return false;
  }

  // return status (partly received HTTP buffer)
  return true;
}

// Dispatch this event
void
EventSource::DispatchEvent(XString* p_event,ULONG p_id,XString* p_data)
{
  // Check on correct ready state of the event source
  if(m_readyState == CLOSED ||
     m_readyState == CLOSED_BY_SERVER)
  {
    // Cannot handle events
    XString error;
    error.Format(_T("Internal error: Ready state = closed, but event received: %s:%s"),(*p_event).GetString(),(*p_data).GetString());
    ERRORLOG(error);
    return;
  }

  // TRACE("EVENT %s %d %s\n",*p_event,p_id,*p_data);

  // Construct event
  ServerEvent* theEvent = new ServerEvent();
  theEvent->m_event = *p_event;
  theEvent->m_id    =  p_id;
  theEvent->m_data  = *p_data;

  // Handle special case 'open'
  if(p_event->CompareNoCase(_T("open")) == 0)
  {
    OnOpen(theEvent);
    return;
  }

  // Now we must have seen an Open event
  if(m_readyState != OPEN)
  {
    XString error;
    error.Format(_T("Internal error: Ready state is not open, but event received: %s:%s"),
                 (*p_event).GetString(),(*p_data).GetString());
    ERRORLOG(error);
    delete theEvent;
    return;
  }

  // In case of a serialize action
  // use the event ID to remove older ID's
  if(m_serialize)
  {
    // Check event ID
    if(p_id && p_id < m_lastEventID)
    {
      // Already seen this event, skip it
      XString logMessage;
      logMessage.Format(_T("Dropped event with duplicate ID: %d:%s")
                        ,(int)p_id,(*p_event).GetString());
      delete theEvent;
      return;
    }
  }

  // Search on lowercase
  XString event = *p_event;
  event.MakeLower();

  // Handle special cases 'message' and 'error'
  if(event.Compare(_T("message")) == 0)
  {
    OnMessage(theEvent);
    return;
  }
  if(event.Compare(_T("error")) == 0)
  {
    OnError(theEvent);
    return;
  }
  if(event.CompareNoCase(_T("close")) == 0)
  {
    OnClose(theEvent);
    return;
  }
  if(event.CompareNoCase(_T("comment")) == 0)
  {
    OnComment(theEvent);
    return;
  }
  if(event.CompareNoCase(_T("retry")) == 0)
  {
    OnRetry(theEvent);
    return;
  }

  // Find event in the listeners map
  if(!p_event->IsEmpty())
  {
    ListenerMap::iterator it = m_listeners.find(event);
    if(it != m_listeners.end())
    {
      EventListener listener = it->second;
      if(listener.m_handler)
      {
        // Remember that we've seen this id
        HandleLastID(theEvent);

        // Async call
        m_pool->SubmitWork(reinterpret_cast<LPFN_CALLBACK>(listener.m_handler),reinterpret_cast<void*>(theEvent));
        return;
      }
    }
  }
  // Still no handler found for this event
  // So send it to the general 'OnMessage'  handler
  OnMessage(theEvent);
}
