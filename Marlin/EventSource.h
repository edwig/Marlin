/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: EventSource.h
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

//////////////////////////////////////////////////////////////////////////
// 
// MANUAL FOR EventSource
//
// 1: Define  your URL            e.g.  CString url("http://servermachine:1200/TestApp/");
// 2: Declare your HTTP Client    e.g.  HTTPClient client;
// 3: Create  your eventsource as e.g.  EventSource* source = client.CreateEventSource(url);
// 4: Set the OnOpen handler      e.g.  source->m_onopen    = OnOpen;
// 5: Set the OnMessage handler   e.g.  source->m_onmessage = OnMessage;
// 6: Set the OnError handler     e.g.  source->m_onerror   = OnError;
// 7: Set the OnClose handler     e.g.  source->m_onclose   = OnClose;
// 8: Set other handlers as well  e.g.  source->AddEventListener("other",OnOther);
// 9: Define your reconnect time  e.g.  source->SetReconnectionTime(2000);
// 
// Now turning the client into a listener
// source->EventSourceInit(false);
//
// ... DO THE NORMAL LOGIC OF YOUR APPLICATION
// ... NOTE: the OnMessage handler starts by way of the threadpool
//           all other handlers are excuted inplace by the calling thread
// 
// At closure time and cleaning up, stop your client
// client.StopClient();
//
#pragma once
#include "ServerEvent.h"
#include <wtypes.h>
#include <map>

// Forward declaration
class HTTPClient;
class ThreadPool;

// Reconnection times are in milliseconds
// DEFINE DEFAULT RECONNECTION TIME = 1 second
#define HTTP_RECONNECTION_TIME          1000
#define HTTP_RECONNECTION_TIME_MINIMUM    50 
#define HTTP_RECONNECTION_TIME_MAXIMUM  3000

typedef enum 
{
  CONNECTING = 0
 ,OPEN
 ,CLOSED
 ,CLOSED_BY_SERVER
}
ReadyState;

typedef void(* LPFN_EVENTHANDLER)(ServerEvent* p_event);

typedef struct _eventListener
{
  CString           m_event;
  LPFN_EVENTHANDLER m_handler;
  bool              m_capture;
}
EventListener;

typedef std::map<CString,EventListener> ListenerMap;

class EventSource
{
public:
  EventSource(HTTPClient* p_client,CString p_url);
 ~EventSource();

  // Init the event source
  bool        EventSourceInit(bool p_withCredentials = false);
  // Closing the event source. Stopping on next HTTP roundtrip
  void        Close();
  // Add event listner to the source
  bool        AddEventListener(CString p_event,LPFN_EVENTHANDLER p_handler,bool p_useCapture = false);

  // Setters
  void        SetSerialize(bool p_serialize);
  void        SetReconnectionTime(ULONG p_time);

  // Getters
  ReadyState  GetReadyState();
  bool        GetWithCredentials();
  ULONG       GetReconnectionTime();
  bool        GetSerialize();
  ULONG       GetLastEventID();

  // Public standard listeners
  LPFN_EVENTHANDLER  m_onopen;      // OPTIONAL
  LPFN_EVENTHANDLER  m_onmessage;   // MANDATORY TO SET!!
  LPFN_EVENTHANDLER  m_onerror;     // OPTIONAL
  LPFN_EVENTHANDLER  m_onclose;     // OPTIONAL
  // Extra (not in the SSE standard)
  LPFN_EVENTHANDLER  m_oncomment;   // OPTIONAL handle the keepalive
  LPFN_EVENTHANDLER  m_onretry;     // OPTIONAL handle the retry-setting

private:
  // HTTPClient will provide data for parser
  friend      HTTPClient;

  // Reset eventsource for reconnect
  void        Reset();
  // Set to connection state
  void        SetConnecting();
  // Handle the last-event-id of a generic listener
  void        HandleLastID(ServerEvent* p_event);
  // Parse incoming data
  void        Parse(BYTE* p_buffer,unsigned& p_length);
  // Parse buffer in string form
  void        Parse(CString& p_buffer);
  // Get one line from the incoming buffer
  bool        GetLine(CString& p_buffer,CString& p_line);
  // Dispatch this event
  void        DispatchEvent(CString* p_event,ULONG p_id,CString* p_data);

  // Standard handlers, if all else fails
  void        OnOpen   (ServerEvent* p_event);
  void        OnMessage(ServerEvent* p_event);
  void        OnError  (ServerEvent* p_event);
  void        OnClose  (ServerEvent* p_event);
  void        OnComment(ServerEvent* p_event);
  void        OnRetry  (ServerEvent* p_event);

  CString     m_url;
  bool        m_withCredentials;
  bool        m_serialize;
  ReadyState  m_readyState;
  ULONG       m_reconnectionTime; // No getters
  ULONG       m_lastEventID;      // No getters
  ListenerMap m_listeners;        // All listeners
  ThreadPool* m_pool;
  HTTPClient* m_client;
};

inline ReadyState
EventSource::GetReadyState()
{
  return m_readyState;
}

inline bool
EventSource::GetWithCredentials()
{
  return m_withCredentials;
}

inline ULONG
EventSource::GetReconnectionTime()
{
  return m_reconnectionTime;
}

inline void
EventSource::SetReconnectionTime(ULONG p_time)
{
  m_reconnectionTime = p_time;
}

inline void
EventSource::SetSerialize(bool p_serialize)
{
  m_serialize = p_serialize;
}

inline bool
EventSource::GetSerialize()
{
  return m_serialize;
}

inline ULONG
EventSource::GetLastEventID()
{
  return m_lastEventID;
}
