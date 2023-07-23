/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: EventSource.h
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

//////////////////////////////////////////////////////////////////////////
// 
// MANUAL FOR EventSource
//
// 1: Define  your URL            e.g.  XString url("http://servermachine:1200/TestApp/Events");
// 2: Declare your HTTP Client    e.g.  HTTPClient client;
// 3: Create  your eventsource as e.g.  EventSource* source = client.CreateEventSource(url);
// 4: Set the OnOpen handler      e.g.  source->m_onopen    = OnOpen;
// 5: Set the OnMessage handler   e.g.  source->m_onmessage = OnMessage;
// 6: Set the OnError handler     e.g.  source->m_onerror   = OnError;
// 7: Set the OnClose handler     e.g.  source->m_onclose   = OnClose;
// 8: Set other handlers as well  e.g.  source->AddEventListener("other",OnOther);
// 9: Define your reconnect time  e.g.  source->SetReconnectionTime(2000);
// 10: OPTIONALLY: connect pool   e.g.  source->SetThreadPool(p_myPool);
// 
// Now turning the client into a listener
// source->EventSourceInit(false);
//
// ... DO THE NORMAL LOGIC OF YOUR APPLICATION
// ... NOTE: the OnMessage handler starts by way of the threadpool
//           all other handlers are executed in-place by the calling thread
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

typedef void(* LPFN_EVENTHANDLER)(ServerEvent* p_event,void* p_data);

typedef struct _eventListener
{
  XString           m_event;
  LPFN_EVENTHANDLER m_handler { nullptr };
  bool              m_capture { false   };
}
EventListener;

typedef std::map<XString,EventListener> ListenerMap;

class EventSource
{
public:
  EventSource(HTTPClient* p_client,XString p_url);
 ~EventSource();

  // Init the event source
  bool        EventSourceInit(bool p_withCredentials = false);
  // Closing the event source. Stopping on next HTTP round trip
  void        Close();
  // Add event listener to the source
  bool        AddEventListener(XString p_event,LPFN_EVENTHANDLER p_handler,bool p_useCapture = false);
  // Add application pointer
  void        SetApplicationData(void* p_data);

  // Setters
  void        SetSerialize(bool p_serialize);
  void        SetReconnectionTime(ULONG p_time);
  void        SetThreadPool(ThreadPool* p_pool);
  void        SetDirectMessage(bool p_direct);
  void        SetSecurity(XString p_cookie,XString p_secret);

  // Getters
  ReadyState  GetReadyState();
  bool        GetWithCredentials();
  ULONG       GetReconnectionTime();
  bool        GetSerialize();
  ULONG       GetLastEventID();
  XString     GetCookieName();
  XString     GetCookieValue();

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
  void        HandleLastID(const ServerEvent* p_event);
  // Parse incoming data
  void        Parse(BYTE* p_buffer,unsigned& p_length);
  // Parse buffer in string form
  void        Parse(XString& p_buffer);
  // Get one line from the incoming buffer
  bool        GetLine(XString& p_buffer,XString& p_line);
  // Dispatch this event
  void        DispatchEvent(XString* p_event,ULONG p_id,XString* p_data);

  // Standard handlers, if all else fails
  void        OnOpen   (ServerEvent* p_event);
  void        OnMessage(ServerEvent* p_event);
  void        OnError  (ServerEvent* p_event);
  void        OnClose  (ServerEvent* p_event);
  void        OnComment(ServerEvent* p_event);
  void        OnRetry  (ServerEvent* p_event);

  XString     m_url;
  bool        m_withCredentials;
  bool        m_serialize;
  bool        m_ownPool;
  XString     m_cookie;
  XString     m_secret;
  ReadyState  m_readyState;
  ULONG       m_reconnectionTime; // No getters
  ULONG       m_lastEventID;      // No getters
  ListenerMap m_listeners;        // All listeners
  ThreadPool* m_pool    { nullptr };
  HTTPClient* m_client  { nullptr };
  void*       m_appData { nullptr };
  bool        m_direct  { false   };
  // Incoming event
  XString     m_eventName;
  XString     m_eventData;
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

inline void
EventSource::SetDirectMessage(bool p_direct)
{
  m_direct = p_direct;
}

inline XString
EventSource::GetCookieName()
{
  return m_cookie;
}

inline XString
EventSource::GetCookieValue()
{
  return m_secret;
}
