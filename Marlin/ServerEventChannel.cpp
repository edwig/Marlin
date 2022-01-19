/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: ServerEventChannel.cpp
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
#include "ServerEventChannel.h"
#include "ServerEventDriver.h"
#include "LongTermEvent.h"
#include "ConvertWideString.h"
#include "WebSocket.h"
#include "AutoCritical.h"
#include "Base64.h"
#include "CRC32.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// Logging via the server
#define DETAILLOG1(text)        m_server->DetailLog (__FUNCTION__,LogType::LOG_INFO,text)
#define DETAILLOGS(text,extra)  m_server->DetailLogS(__FUNCTION__,LogType::LOG_INFO,text,extra)
#define DETAILLOGV(text,...)    m_server->DetailLogV(__FUNCTION__,LogType::LOG_INFO,text,__VA_ARGS__)
#define WARNINGLOG(text,...)    m_server->DetailLogV(__FUNCTION__,LogType::LOG_WARN,text,__VA_ARGS__)
#define ERRORLOG(code,text)     m_server->ErrorLog  (__FUNCTION__,code,text)

// Socket message handlers

void EventChannelOnOpen(WebSocket* p_socket,WSFrame* p_event)
{
  ServerEventChannel* channel = reinterpret_cast<ServerEventChannel*>(p_socket->GetApplication());
  if(channel)
  {
    CString message(p_event->m_data);
    if(p_event->m_utf8)
    {
      message = DecodeStringFromTheWire(message);
    }
    channel->OnOpenSocket(p_socket);
    channel->OnOpen(message);
  }
}

void EventChannelOnMessage(WebSocket* p_socket,WSFrame* p_event)
{
  ServerEventChannel* channel = reinterpret_cast<ServerEventChannel*>(p_socket->GetApplication());
  if(channel)
  {
    CString message(p_event->m_data);
    if(p_event->m_utf8)
    {
      message = DecodeStringFromTheWire(message);
    }
    channel->OnMessage(message);
  }
}

void EventChannelOnBinary(WebSocket* p_socket,WSFrame* p_event)
{
  ServerEventChannel* channel = reinterpret_cast<ServerEventChannel*>(p_socket->GetApplication());
  if(channel)
  {
    channel->OnBinary(p_event->m_data,p_event->m_length);
  }
}

void EventChannelOnError(WebSocket* p_socket,WSFrame* p_event)
{
  ServerEventChannel* channel = reinterpret_cast<ServerEventChannel*>(p_socket->GetApplication());
  if(channel && p_event)
  {
    CString message(p_event->m_data);
    if(p_event->m_utf8)
    {
      message = DecodeStringFromTheWire(message);
    }
    channel->OnError(message);
  }
}

void EventChannelOnClose(WebSocket* p_socket,WSFrame* p_event)
{
  ServerEventChannel* channel = reinterpret_cast<ServerEventChannel*>(p_socket->GetApplication());
  if(channel)
  {
    CString message(p_event->m_data);
    if(p_event->m_utf8)
    {
      message = DecodeStringFromTheWire(message);
    }
    channel->OnCloseSocket(p_socket);
    channel->OnClose(message);
  }
}

//////////////////////////////////////////////////////////////////////////
//
// THE SERVER CHANNEL FOR EVENTS
//
//////////////////////////////////////////////////////////////////////////

ServerEventChannel::ServerEventChannel(ServerEventDriver* p_driver
                                      ,int     p_channel
                                      ,CString p_sessionName
                                      ,CString p_cookie
                                      ,CString p_token)
                   :m_driver(p_driver)
                   ,m_channel(p_channel)
                   ,m_name(p_sessionName)
                   ,m_cookie(p_cookie)
                   ,m_token(p_token)
{
  m_server = m_driver->GetHTTPServer();
  InitializeCriticalSection(&m_lock);
}

ServerEventChannel::~ServerEventChannel()
{
  CloseChannel();
  DeleteCriticalSection(&m_lock);
}

// Reset the channel. No more events to the server application
void 
ServerEventChannel::Reset()
{
  m_application = nullptr;
  m_appData     = NULL;
}

int 
ServerEventChannel::PostEvent(CString p_payload,CString p_sender,EvtType p_type /*= EvtType::EV_Message*/)
{
  AutoCritSec lock(&m_lock);

  // Create event and store at the back of the queue
  LTEvent* event   = new LTEvent();
  event->m_number  = ++m_maxNumber;
  event->m_sent    = 0; // Send to all clients
  event->m_type    = p_type;
  event->m_payload = p_payload;

  // Send just to this sender
  if(!p_sender.IsEmpty())
  {
    event->m_sent = ComputeCRC32(SENDER_RANDOM_NUMBER,p_sender.GetString(),p_sender.GetLength());
  }
  m_outQueue.push_back(event);

  return m_maxNumber;
}

// Remove events up to this enumerator
bool 
ServerEventChannel::RemoveEvents(int p_number)
{
  AutoCritSec lock(&m_lock);

  // Cannot be in the queue
  if(p_number <= m_minNumber || p_number > m_maxNumber)
  {
    return true;
  }
  // Remove all events with numbers smaller and equal to the current event
  if(!m_outQueue.empty())
  {
    LTEvent* first = m_outQueue.front();
    while(first->m_number <= p_number)
    {
      delete first;
      m_outQueue.pop_front();
      if(!m_outQueue.empty())
      {
        first = m_outQueue.front();
      }
      else break;
    }

    // Set new minimum number
    if(m_outQueue.empty())
    {
      m_minNumber = m_maxNumber;
    }
    else
    {
      m_minNumber = m_outQueue.front()->m_number - 1;
    }
  }
  return true;
}

CString 
ServerEventChannel::GetCookieToken()
{
  return m_cookie + ":" + m_token;
}

int
ServerEventChannel::GetQueueCount()
{
  AutoCritSec lock(&m_lock);
  return (int) m_outQueue.size();
}

// Count all 'opened' sockets and all SSE-streams
// Giving the fact that a channel is 'connected' to a number of clients
int
ServerEventChannel::GetClientCount()
{
  AutoCritSec lock(&m_lock);
  int count = 0;

  for(auto& socket : m_sockets)
  {
    count += socket.m_open == true ? 1 : 0;
  }
  count += (int) m_streams.size();

  return count;
}

bool 
ServerEventChannel::RegisterNewSocket(HTTPMessage* p_message,WebSocket* p_socket,bool p_check /*=false*/)
{
  AutoCritSec lock(&m_lock);

  if(p_check)
  {
    bool found = false;
    Cookies& cookies = p_message->GetCookies();
    for(auto& cookie : cookies.GetCookies())
    {
      if(m_cookie.CompareNoCase(cookie.GetName())  == 0 &&
         m_token .CompareNoCase(cookie.GetValue()) == 0 )
      {
        found = true;
      }
    }
    if(!found)
    {
      // Security breach!
      WARNINGLOG("Tried to start WebSocket without proper authentication: %s",m_name.GetString());
      return false;
    }
  }

  // Tell our socket to come back with THIS channel!
  p_socket->SetApplication(this);

  // Tell the socket our handlers
  p_socket->SetOnOpen   (EventChannelOnOpen);
  p_socket->SetOnMessage(EventChannelOnMessage);
  p_socket->SetOnBinary (EventChannelOnBinary);
  p_socket->SetOnError  (EventChannelOnError);
  p_socket->SetOnClose  (EventChannelOnClose);

  // Getting the senders URL + Citrix desktop
  CString url;
  CString sender  = SocketToServer(p_message->GetSender());
  CString desktop = p_message->GetHeader("desktop").Trim();
  url.Format("http://%s:c%s", sender.GetString(),desktop.GetString());
  url.MakeLower();

  // Check for a brute-force attack. If so, already logged to the server
  if (m_driver->CheckBruteForceAttack(url))
  {
    return false;
  }

  // Register our socket
  bool found = false;
  for(auto& sock : m_sockets)
  {
    if(sock.m_url.Compare(url) == 0)
    {
      found = true;
    }
  }
  if(!found)
  {
    EventWebSocket sock;
    sock.m_socket = p_socket;
    sock.m_url    = url;
    sock.m_sender = ComputeCRC32(SENDER_RANDOM_NUMBER,sender.GetString(),sender.GetLength());
    sock.m_open   = false;

    m_sockets.push_back(sock);
  }
  m_current = EventDriverType::EDT_Sockets;
  return true;
}

void
ServerEventChannel::OnOpenSocket(WebSocket* p_socket)
{
  AutoCritSec lock(&m_lock);

  for(auto& sock : m_sockets)
  {
    if(sock.m_socket == p_socket)
    {
      sock.m_open = true;
      m_openSeen  = true;
      m_closeSeen = false;
    }
  }
}

void
ServerEventChannel::OnCloseSocket(WebSocket* p_socket)
{
  AutoCritSec lock(&m_lock);

  for(auto& sock : m_sockets)
  {
    if(sock.m_socket == p_socket)
  {
      sock.m_open = false;
    m_closeSeen = true;
  }
}
}

bool 
ServerEventChannel::RegisterNewStream(HTTPMessage* p_message,EventStream* p_stream,bool p_check /*=false*/)
{
  AutoCritSec lock(&m_lock);

  if(p_check)
  {
    bool found = false;
    Cookies& cookies = p_message->GetCookies();
    for(auto& cookie : cookies.GetCookies())
    {
      if(m_cookie.CompareNoCase(cookie.GetName())  == 0 &&
         m_token .CompareNoCase(cookie.GetValue()) == 0 )
      {
        found = true;
      }
    }
    if(!found)
    {
      // Security breach!
      WARNINGLOG("Tried to start SSE stream without proper authentication: %s",m_name.GetString());
      return false;
    }
  }

  // Getting the senders URL + Citrix desktop
  CString url;
  CString sender = SocketToServer(p_message->GetSender());
  url.Format("http://%s:c%d",sender.GetString(),p_stream->m_desktop);
  url.MakeLower();

  // Check for a brute-force attack. If so, already logged to the server
  if(m_driver->CheckBruteForceAttack(url))
  {
    return false;
  }

  // Register our stream
  bool found = false;
  for(auto& stream : m_streams)
  {
    if(stream.m_url.Compare(url) == 0)
  {
      found = true;
    }
  }
  if(!found)
  {
    EventSSEStream stream;
    stream.m_stream = p_stream;
    stream.m_url    = url;
    stream.m_sender = ComputeCRC32(SENDER_RANDOM_NUMBER,sender.GetString(),sender.GetLength());

    m_streams.push_back(stream);
  }

  m_current = EventDriverType::EDT_ServerEvents;

  // SSE Channels have no client messages, so we generate one ourselves
  OnOpen("Started: " + url);
  return true;
}

bool 
ServerEventChannel::HandleLongPolling(SOAPMessage* p_message,bool p_check /*=false*/)
{
  if(p_check)
  {
    bool found = false;
    Cookies& cookies = p_message->GetCookies();
    for(auto& cookie : cookies.GetCookies())
    {
      if(m_cookie.CompareNoCase(cookie.GetName())  == 0 &&
          m_token.CompareNoCase(cookie.GetValue()) == 0)
      {
        found = true;
      }
    }
    if(!found)
    {
      // Security breach!
      WARNINGLOG("Tried to answer Long-Polling, but no proper authentication: %s",m_name.GetString());
      return false;
    }
  }
  bool    close   = p_message->GetParameterBoolean("CloseChannel");
  int     acknl   = p_message->GetParameterInteger("Acknowledged");
  CString message = p_message->GetParameter("Message");
  CString type    = p_message->GetParameter("Type");

  // Reset to response
  p_message->Reset(ResponseType::RESP_ACTION_RESP);

  // If we've got an incoming message as well
  if(!message.IsEmpty())
  {
    switch(LTEvent::StringToEventType(type))
    {
      case EvtType::EV_Open:    OnOpen   (message); break;
      case EvtType::EV_Message: OnMessage(message); break;
      case EvtType::EV_Binary:  OnBinary ((void*)message.GetString(),message.GetLength()); break;
      case EvtType::EV_Error:   OnError  (message); break;
      case EvtType::EV_Close:   OnClose  (message); break;
    }
  }

  // Remove all the events that the client has acknowledged
  if(acknl)
  {
    RemoveEvents(acknl);
  }

  // On the closing: acknowledge that we are closing
  if(close)
  {
    p_message->SetParameter("ChannelClosed",true);
    if(m_current == EventDriverType::EDT_LongPolling)
    {
      m_current = EventDriverType::EDT_NotConnected;
    }
    return true;
  }

  // We are now doing long-polling
  m_current = EventDriverType::EDT_LongPolling;

  AutoCritSec lock(&m_lock);

  // Make sure channel is now open on the server side and 'in-use'
  if(!m_openSeen)
  {
    OnOpen("");
  }
  // Return 'empty' or the oldest event
  if(m_outQueue.empty())
  {
    p_message->SetParameter("Empty",true);
  }
  else
  {
    LTEvent* event = m_outQueue.front();
    p_message->SetParameter("Number", event->m_number);
    p_message->SetParameter("Message",event->m_payload);
    p_message->SetParameter("Type",   LTEvent::EventTypeToString(event->m_type));
    event->m_sent++;
  }

  // Ready
  _time64(&m_lastSending);
  return true;
}

// Sending dispatcher: DriverMonitor calls this entry point
int
ServerEventChannel::SendChannel()
{
  AutoCritSec lock(&m_lock);
  int sent = 0;

  switch(m_current)
  {
    case EventDriverType::EDT_Sockets:       sent += SendQueueToSocket(); break;
    case EventDriverType::EDT_ServerEvents:  sent += SendQueueToStream(); break;
    case EventDriverType::EDT_LongPolling:   sent += LogLongPolling();    break;
    case EventDriverType::EDT_NotConnected:  sent += LogNotConnected();   break;
  }
  _time64(&m_lastSending);
  return sent;
}

// Try to sent as much as possible
// Channel already locked by SendChannel.
int
ServerEventChannel::SendQueueToSocket()
{
  int sent     = 0;
  int lastSent = 0;
  bool allok   = true;
  
  // No sockets connected. Nothing to send
  if(m_sockets.empty())
  {
    return 0;
  }

  for(auto& ltevent : m_outQueue)
  {
    allok = true;
    AllSockets::iterator it = m_sockets.begin();
    while(it != m_sockets.end())
    {
      if(it->m_open == true && (ltevent->m_sent == 0 || ltevent->m_sent == it->m_sender))
      {
        // Make sure channel is now open on the server side and 'in-use'
        if(!m_openSeen)
        {
          OnOpen("");
        }
        if(!it->m_socket->WriteString(ltevent->m_payload))
        {
          allok = false;
          CloseSocket(it->m_socket);
          it = m_sockets.erase(it);
          OnClose("");
          continue;
        }
      }
      ++it;
    }
    if(allok)
    {
      // Only if sent to all sockets
      ltevent->m_sent++;
      lastSent = ltevent->m_number;
      ++sent;
    }
  }
  if(lastSent)
  {
    RemoveEvents(lastSent);
  }
  return sent;
}

// Try to sent as much as possible
// Channel already locked by SendChannel.
int
ServerEventChannel::SendQueueToStream()
{
  int sent     = 0;
  int lastSent = 0;
  bool allok   = true;

  // No streams. nothing to send
  if(m_streams.empty())
  {
    return 0;
  }

  for(auto& ltevent : m_outQueue)
  {
    allok = true;
    AllStreams::iterator it = m_streams.begin();
    while (it != m_streams.end())
    {
      if(ltevent->m_sent == 0 || ltevent->m_sent == it->m_sender)
      {
      CString type = LTEvent::EventTypeToString(ltevent->m_type);
      ServerEvent* event = new ServerEvent(type);
      event->m_id = ltevent->m_number;
      if (ltevent->m_type == EvtType::EV_Binary)
      {
        event->m_data  = Base64::Encrypt(ltevent->m_payload);
      }
      else
      {
        // Simply send the payload text
        event->m_data = ltevent->m_payload;
      }

      // Make sure channel is now open on the server side and 'in-use'
      if (!m_openSeen)
      {
        OnOpen("");
      }
        if(!m_server->SendEvent(it->m_stream,event))
      {
        allok = false;
          CloseStream(it->m_stream);
        it = m_streams.erase(it);
        OnClose("");
        continue;
      }
      }
      ++it;
    }
    if(allok)
    {
      // Only OK if sent to all streams
      ltevent->m_sent++;
      lastSent = ltevent->m_number;
      ++sent;
    }
  }
  if(lastSent)
  {
    RemoveEvents(lastSent);
  }
  return sent;
}

int
ServerEventChannel::LogLongPolling()
{
  DETAILLOGV("Monitor calls channel [%s] Long-polling is active. %d events in the queue.",m_name.GetString(),(int)m_outQueue.size());
  return 0;
}

int
ServerEventChannel::LogNotConnected()
{
  DETAILLOGV("Monitor calls channel [%s] No connection (yet) and no long-polling active. %d events in the queue.",m_name.GetString(),(int)m_outQueue.size());
  return 0;
}

// Flushing a channel directly
// (Works only for sockets and streams)
bool 
ServerEventChannel::FlushChannel()
{
  SendChannel();
  return m_outQueue.empty();
}

void
ServerEventChannel::CloseChannel()
{
  AutoCritSec lock(&m_lock);

  // Begin with sending a close-event to the server application
  // event channels to the client possibly still open.
  if(m_closeSeen == false)
  {
    LTEvent* event = new LTEvent(EvtType::EV_Close);
    event->m_payload = "Channel closed";
    event->m_number  = ++m_maxNumber;
    event->m_sent    = m_appData;

    // Do not come here again
    m_closeSeen = true;
    // Tell it the server
    if(m_application)
    {
      try
      {
    (*m_application)(event);
  }
      catch(StdException& ex)
      {
        ERRORLOG(ERROR_APPEXEC_INVALID_HOST_STATE,ex.GetErrorMessage());
      }
    }
  }

  // Closing open channels to the client
  m_current = EventDriverType::EDT_NotConnected;
  for(auto& sock : m_sockets)
  {
    CloseSocket(sock.m_socket);
  }
  m_sockets.clear();

  for(auto& stream : m_streams)
  {
    CloseStream(stream.m_stream);
  }
  m_streams.clear();

  // Clean out the out queue
  for(auto& ltevent : m_outQueue)
  {
    delete ltevent;
  }
  m_outQueue.clear();

  // Clean out the input queue
  for(auto& ltevent : m_inQueue)
  {
    delete ltevent;
  }
  m_inQueue.clear();
}

void 
ServerEventChannel::CloseSocket(WebSocket* p_socket)
{
  DETAILLOGV("Closing WebSocket for event channel [%s] Queue size: %d",m_name.GetString(),(int)m_outQueue.size());
  p_socket->SendCloseSocket(WS_CLOSE_NORMAL,"ServerEventDriver is closing channel");
  p_socket->CloseSocket();
  m_server->UnRegisterWebSocket(p_socket);
}

void 
ServerEventChannel::CloseStream(EventStream* p_stream)
{
  DETAILLOGV("Closing EventStream for event channel [%s] Queue size: %d",m_name.GetString(),(int)m_outQueue.size());
  m_server->CloseEventStream(p_stream);
}

bool 
ServerEventChannel::ChangeEventPolicy(EVChannelPolicy p_policy,LPFN_CALLBACK p_application,UINT64 p_data)
{
  switch(p_policy)
  {
    case EVChannelPolicy::DP_TwoWayMessages:// Fall through
    case EVChannelPolicy::DP_Binary:        if(!m_streams.empty()) return false;
                                            break;
    case EVChannelPolicy::DP_HighSecurity:  if(!m_sockets.empty()) return false;
                                            break;
    case EVChannelPolicy::DP_Disconnected:  if(!m_streams.empty() || !m_sockets.empty()) return false;
                                            break;
    case EVChannelPolicy::DP_Immediate_S2C: // Fall through
    case EVChannelPolicy::DP_SureDelivery:  break;
  }
  // Finders, keepers
  m_appData     = p_data;
  m_policy      = p_policy;
  m_application = p_application;

  // And tell it our sockets (if any)
  for(auto& socket : m_sockets)
  {
    socket.m_socket->SetApplicationData(p_data);
  }
  return true;
}

// To be called by the socket handlers only!
void
ServerEventChannel::OnOpen(CString p_message)
{
  AutoCritSec lock(&m_lock);

  // Open seen. Do not generate again for this channel
  m_openSeen = true;

  LTEvent* event = new LTEvent;
  event->m_payload = p_message;
  event->m_type    = EvtType::EV_Open;
  event->m_number  = 0;
  event->m_sent    = m_appData;
  m_inQueue.push_back(event);
  m_driver->IncomingEvent();
}

// Called by socket handler and long-polling handler
void
ServerEventChannel::OnMessage(CString p_message)
{
  AutoCritSec lock(&m_lock);

  LTEvent* event = new LTEvent;
  event->m_payload = p_message;
  event->m_type    = EvtType::EV_Message;
  event->m_number  = 0;
  event->m_sent    = m_appData;
  m_inQueue.push_back(event);
  m_driver->IncomingEvent();
}

void
ServerEventChannel::OnError(CString p_message)
{
  AutoCritSec lock(&m_lock);

  LTEvent* event = new LTEvent;
  event->m_payload = p_message;
  event->m_type    = EvtType::EV_Error;
  event->m_number  = 0;
  event->m_sent    = m_appData;
  m_inQueue.push_back(event);
  m_driver->IncomingEvent();
}

void
ServerEventChannel::OnClose(CString p_message)
{
  AutoCritSec lock(&m_lock);

  LTEvent* event = new LTEvent;
  event->m_payload = p_message;
  event->m_type    = EvtType::EV_Close;
  event->m_number  = 0;
  event->m_sent    = m_appData;
  m_inQueue.push_back(event);
  m_driver->IncomingEvent();
}

void
ServerEventChannel::OnBinary(void* p_data,DWORD p_length)
{
  AutoCritSec lock(&m_lock);

  if(m_openSeen == false)
  {
    m_openSeen = true;
    OnOpen("OpenChannel");
  }
  LTEvent* event = new LTEvent;
  event->m_type    = EvtType::EV_Binary;
  event->m_number  = p_length;
  event->m_sent    = m_appData;
  char* buffer = event->m_payload.GetBufferSetLength(p_length);
  strcpy_s(buffer,p_length,(char*)p_data);
  event->m_payload.ReleaseBufferSetLength(p_length);

  m_inQueue.push_back(event);
  m_driver->IncomingEvent();
}

// Process the receiving part of the queue
// By pushing it into the threadpool
int
ServerEventChannel::Receiving()
{
  // Cannot process if no application pointer
  if(!m_application)
  {
    return 0;
  }

  AutoCritSec lock(&m_lock);
  ThreadPool* pool = m_server->GetThreadPool();
  int received = 0;

  EventQueue::iterator it = m_inQueue.begin();
  while(it != m_inQueue.end())
  {
    LTEvent* event = m_inQueue.front();
    if(!m_openSeen && event->m_type != EvtType::EV_Open)
    {
      LTEvent* open = new LTEvent(EvtType::EV_Open);
      pool->SubmitWork(m_application,open);
    }
    // Post it to the thread pool
    if(!pool->SubmitWork(m_application,event))
    {
      // Threadpool not yet open or posting did not go right
      // Leave the in-queue intact from this point on
      break;
    }
    it = m_inQueue.erase(it);
    ++received;
  }
  return received;
}
