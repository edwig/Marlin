/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: ServerEventChannel.cpp
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
#include "ServerEventChannel.h"
#include "ServerEventDriver.h"
#include "LongTermEvent.h"
#include "ConvertWideString.h"
#include "WebSocketMain.h"
#include "AutoCritical.h"
#include "Base64.h"
#include "CRC32.h"

#ifdef _AFX
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#endif

// Logging via the server
#define DETAILLOG1(text)        m_server->DetailLog (_T(__FUNCTION__),LogType::LOG_INFO,text)
#define DETAILLOGS(text,extra)  m_server->DetailLogS(_T(__FUNCTION__),LogType::LOG_INFO,text,extra)
#define DETAILLOGV(text,...)    m_server->DetailLogV(_T(__FUNCTION__),LogType::LOG_INFO,text,__VA_ARGS__)
#define WARNINGLOG(text,...)    m_server->DetailLogV(_T(__FUNCTION__),LogType::LOG_WARN,text,__VA_ARGS__)
#define ERRORLOG(code,text)     m_server->ErrorLog  (_T(__FUNCTION__),code,text)

// Socket message handlers

void EventChannelOnOpen(WebSocket* p_socket,const WSFrame* p_event)
{
  ServerEventChannel* channel = reinterpret_cast<ServerEventChannel*>(p_socket->GetApplication());
  if(channel)
  {
    XString message(p_event->m_data);
    if(p_event->m_utf8)
    {
      message = DecodeStringFromTheWire(message);
    }
    channel->OnOpenSocket(p_socket);
    channel->OnOpen(message);
  }
}

void EventChannelOnMessage(WebSocket* p_socket,const WSFrame* p_event)
{
  ServerEventChannel* channel = reinterpret_cast<ServerEventChannel*>(p_socket->GetApplication());
  if(channel)
  {
    XString message(p_event->m_data);
    if(p_event->m_utf8)
    {
      message = DecodeStringFromTheWire(message);
    }
    channel->OnMessage(message);
  }
}

void EventChannelOnBinary(WebSocket* p_socket,const WSFrame* p_event)
{
  ServerEventChannel* channel = reinterpret_cast<ServerEventChannel*>(p_socket->GetApplication());
  if(channel)
  {
    channel->OnBinary(p_event->m_data,p_event->m_length);
  }
}

void EventChannelOnError(WebSocket* p_socket,const WSFrame* p_event)
{
  ServerEventChannel* channel = reinterpret_cast<ServerEventChannel*>(p_socket->GetApplication());
  if(channel && p_event)
  {
    XString message(p_event->m_data);
    if(p_event->m_utf8)
    {
      message = DecodeStringFromTheWire(message);
    }
    channel->OnError(message);
  }
}

void EventChannelOnClose(WebSocket* p_socket,const WSFrame* p_event)
{
  ServerEventChannel* channel = reinterpret_cast<ServerEventChannel*>(p_socket->GetApplication());
  if(channel)
  {
    XString message(p_event->m_data);
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
                                      ,XString p_sessionName
                                      ,XString p_cookie
                                      ,XString p_token)
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

// Register a new event
// Called from the ServerEventDriver.
int 
ServerEventChannel::PostEvent(XString p_payload
                             ,XString p_sender
                             ,EvtType p_type     /*= EvtType::EV_Message*/
                             ,XString p_typeName /*= ""*/)
{
  // Create event and store at the back of the queue
  LTEvent* ltevent    = new LTEvent();
  ltevent->m_number   = ++m_maxNumber;
  ltevent->m_sent     = 0; // Send to all clients
  ltevent->m_type     = p_type;
  ltevent->m_typeName = p_typeName;
  ltevent->m_payload  = p_payload;

  // Send just to this sender
  if(!p_sender.IsEmpty())
  {
    ltevent->m_sent = ComputeCRC32(SENDER_RANDOM_NUMBER,p_sender.GetString(),p_sender.GetLength());
  }

  // Place in the queue (Shortest possible lock)
  {
    AutoCritSec lock(&m_lock);
    m_outQueue.push_back(ltevent);
  }
  if(m_driver->GetActive() == false)
  {
    // Directly send all pending events for this channel
    SendChannel();
  }
  return m_maxNumber;
}

void 
ServerEventChannel::PlaceInLongPollingQueue(LTEvent* p_event)
{
  // Place in the queue (Shortest possible lock)
  AutoCritSec lock(&m_lock);
  m_polQueue.push_back(p_event);
  if(m_polQueue.size() == 1)
  {
    m_minNumber = p_event->m_number;
  }
  m_usePolling = true;
}

bool
ServerEventChannel::GetNextOutgoingEvent(LTEvent*& p_event)
{
  AutoCritSec lock(&m_lock);
  if(!m_outQueue.empty())
  {
    p_event = m_outQueue.front();
    m_outQueue.pop_front();
    return true;
  }
  return false;
}

bool
ServerEventChannel::GetNextIncomingEvent(LTEvent*& p_event)
{
  AutoCritSec lock(&m_lock);
  if(!m_inQueue.empty())
  {
    p_event = m_inQueue.front();
    m_inQueue.pop_front();
    return true;
  }
  return false;
}

// Remove events up to this enumerator
// Used by the long-polling events stream
bool 
ServerEventChannel::RemoveEvents(int p_number)
{
  // Cannot be in the queue
  if(p_number < m_minNumber || p_number > m_maxNumber)
  {
    return true;
  }

  // Remove all events with numbers smaller and equal to the current event
  AutoCritSec lock(&m_lock);
  if(!m_polQueue.empty())
  {
    LTEvent* first = m_polQueue.front();
    while(first->m_number <= p_number)
    {
      delete first;
      m_polQueue.pop_front();
      if(!m_polQueue.empty())
      {
        first = m_polQueue.front();
      }
      else break;
    }

    // Set new minimum number
    if(m_polQueue.empty())
    {
      m_minNumber = 0;
    }
    else
    {
      m_minNumber = m_polQueue.front()->m_number;
    }
  }
  return true;
}

XString 
ServerEventChannel::GetCookieToken()
{
  return m_cookie + _T(":") + m_token;
}

int
ServerEventChannel::GetQueueCount()
{
  AutoCritSec lock(&m_lock);
  return (int) (m_outQueue.size() + m_inQueue.size());
}

// Count all 'opened' sockets and all SSE-streams
// Giving the fact that a channel is 'connected' to a number of clients
// Only looks at the reading state!
int
ServerEventChannel::GetClientCount()
{
  AutoCritSec lock(&m_lock);
  int count = 0;

  for(auto& socket : m_sockets)
  {
    if(socket.m_open && socket.m_socket->IsOpenForReading())
    {
      ++count;
    }
  }
  count += (int) m_streams.size();

  return count;
}

bool 
ServerEventChannel::RegisterNewSocket(HTTPMessage* p_message,WebSocket* p_socket,bool p_check /*=false*/)
{
  if(m_policy != EVChannelPolicy::DP_Binary         &&
     m_policy != EVChannelPolicy::DP_Immediate_S2C  &&
     m_policy != EVChannelPolicy::DP_TwoWayMessages &&
     m_policy != EVChannelPolicy::DP_SureDelivery)
  {
    // Policy breach!
    WARNINGLOG(_T("Tried to start WebSocket without proper policy on channel: %s"),m_name.GetString());
    return false;
  }

  // Getting the senders URL + Citrix desktop
  XString registration;
  XString address = SocketToServer(p_message->GetSender());
  UINT    desktop = p_message->GetRemoteDesktop();
  registration.Format(_T("S%s:D%d"),address.GetString(),desktop);
  unsigned sender = ComputeCRC32(SENDER_RANDOM_NUMBER,registration.GetString(),registration.GetLength());

  // Check for a brute-force attack. If so, already logged to the server
  if(m_driver->CheckBruteForceAttack(sender))
  {
    return false;
  }

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
      WARNINGLOG(_T("Tried to start WebSocket without proper authentication: %s"),m_name.GetString());
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

  // Register our socket
  EventWebSocket sock;
  sock.m_socket = p_socket;
  sock.m_sender = sender;
  sock.m_open   = false;

  AutoCritSec lock(&m_lock);
  m_sockets.push_back(sock);

  // We now do sockets
  m_current = (EventDriverType)(m_current | EDT_Sockets);

  // Reset close seen
  m_closeSeen = false;

  // Make sure we have an opening event
  OnOpen(XString(_T("Started: ")) + p_message->GetURL().GetString());

  return true;
}

void
ServerEventChannel::OnOpenSocket(const WebSocket* p_socket)
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
ServerEventChannel::OnCloseSocket(const WebSocket* p_socket)
{
  AutoCritSec lock(&m_lock);

  for(AllSockets::iterator it = m_sockets.begin(); it != m_sockets.end(); ++it)
  {
    if(it->m_socket == p_socket)
    {
      // Close and remove the socket
      CloseSocket(it->m_socket);
      it = m_sockets.erase(it);

      // Push "OnClose" for web application to handle
      OnClose(_T(""));
      break;
    }
  }
  // Register the new state
  if(m_sockets.empty())
  {
    m_closeSeen = true;
  }
}

bool 
ServerEventChannel::RegisterNewStream(HTTPMessage* p_message,EventStream* p_stream,bool p_check /*=false*/)
{
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
      WARNINGLOG(_T("Tried to start SSE stream without proper authentication: %s"),m_name.GetString());
      return false;
    }
  }

  if(m_policy != EVChannelPolicy::DP_HighSecurity   &&
     m_policy != EVChannelPolicy::DP_Immediate_S2C  &&
     m_policy != EVChannelPolicy::DP_NoSockets      &&
     m_policy != EVChannelPolicy::DP_SureDelivery)
  {
    // Policy breach!
    WARNINGLOG(_T("Tried to start SSE stream without proper policy on channel: %s"),m_name.GetString());
    return false;
  }

  // Getting the senders URL + Citrix desktop
  XString registration;
  XString address = SocketToServer(p_message->GetSender());
  registration.Format(_T("S%s:D%d"),address.GetString(),p_stream->m_desktop);
  unsigned sender = ComputeCRC32(SENDER_RANDOM_NUMBER, registration.GetString(), registration.GetLength());

  // Check for a brute-force attack. If so, already logged to the server
  if(m_driver->CheckBruteForceAttack(sender))
  {
    return false;
  }

  // Register our stream
  EventSSEStream stream;
  stream.m_stream = p_stream;
  stream.m_sender = sender;

  AutoCritSec lock(&m_lock);
  m_streams.push_back(stream);

  // We now do streams
  m_current = (EventDriverType)(m_current | EDT_ServerEvents);

  // Reset close seen
  m_closeSeen = false;

  // Make sure we have an opening event
  OnOpen(XString(_T("Started: ")) + p_message->GetURL().GetString());

  return true;
}

bool 
ServerEventChannel::HandleLongPolling(SOAPMessage* p_message,bool p_check /*=false*/)
{
  if(p_check)
  {
    bool found = false;
    Cookies& cookies = const_cast<Cookies&>(p_message->GetCookies());
    for(auto& cookie : cookies.GetCookies())
    {
      if(m_cookie.CompareNoCase(cookie.GetName()) == 0 &&
          m_token.CompareNoCase(cookie.GetValue()) == 0)
      {
        found = true;
      }
    }
    if(!found)
    {
      // Security breach!
      WARNINGLOG(_T("Tried to answer Long-Polling, but no proper authentication: %s"), m_name.GetString());
      return false;
    }
  }
  if(m_policy != EVChannelPolicy::DP_Disconnected   &&
     m_policy != EVChannelPolicy::DP_TwoWayMessages &&
     m_policy != EVChannelPolicy::DP_NoSockets      &&
     m_policy != EVChannelPolicy::DP_SureDelivery)
  {
    // Policy breach!
    WARNINGLOG(_T("Tried to send a LongPolling SOAP message without proper policy on channel: %s"), m_name.GetString());
    return false;
  }
  // Long polling now in use
  m_usePolling = true;

  bool    close   = p_message->GetParameterBoolean(_T("CloseChannel"));
  int     acknl   = p_message->GetParameterInteger(_T("Acknowledged"));
  XString message = p_message->GetParameter(_T("Message"));
  XString type    = p_message->GetParameter(_T("Type"));

  // Reset to response
  p_message->Reset(ResponseType::RESP_ACTION_RESP);

  // If we've got an incoming message as well
  if(!message.IsEmpty())
  {
    switch(LTEvent::StringToEventType(type))
    {
      case EvtType::EV_Open:    OnOpen   (message); break;
      case EvtType::EV_Message: OnMessage(message); break;
      case EvtType::EV_Binary:  OnBinary (reinterpret_cast<void*>(const_cast<TCHAR*>(message.GetString())),message.GetLength()); break;
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
    p_message->SetParameter(_T("ChannelClosed"), true);
    if(m_current & EDT_LongPolling)
    {
      m_current = (EventDriverType)(m_current & ~EDT_LongPolling);
      if(m_current == EventDriverType::EDT_NotConnected)
      {
        OnClose(_T(""));
        Receiving();
        CloseChannel();
      }
    }
    return true;
  }
  else
  {
    // Reset close seen
    m_closeSeen = false;
  }

  // We are now doing long-polling
  m_current = (EventDriverType)(m_current | EDT_LongPolling);

  // Make sure channel is now open on the server side and 'in-use'
  if(!m_openSeen)
  {
    // Make sure we have an opening event
    OnOpen(XString(_T("Started: ")) + p_message->GetURL().GetString());
    m_openSeen = true;
  }

  // Return 'empty' or the oldest event
  AutoCritSec lock(&m_lock);
  EventQueue* queue = nullptr;
  LTEvent*  ltevent = nullptr;

  if(m_polQueue.empty())
  {
    if(!GetNextOutgoingEvent(ltevent))
    {
      p_message->SetParameter(_T("Empty"),true);
    }
  }
  else
  {
    queue = &m_polQueue;
    ltevent = m_polQueue.front();
  }
  if(ltevent)
  {
    p_message->SetParameter(_T("Number"), ltevent->m_number);
    p_message->SetParameter(_T("Message"),ltevent->m_payload);
    p_message->SetParameter(_T("Type"), LTEvent::EventTypeToString(ltevent->m_type));

    delete ltevent;
  } 
  if(queue)
  {
    // Remove from the polling queue only
    queue->pop_front();
  }
  return true;
}

// Sending dispatcher: DriverMonitor calls this entry point
int
ServerEventChannel::SendChannel()
{
  int sent = 0;

  // See if we have something to do
  if(m_outQueue.empty())
  {
    return 0;
  }

  try
  {
    // Log the state if not-connected
    if(m_current == EDT_NotConnected)
    {
      LogNotConnected();
      return 0;
    }

    // Process the outgoing long-term event queue
    LTEvent* ltevent = nullptr;
    while(GetNextOutgoingEvent(ltevent))
    {
      if(m_current & EDT_Sockets)
      {
        // See if websocket channel still sane
        CheckChannel();
        sent += SendEventToSockets(ltevent);
      }
      if(m_current & EDT_ServerEvents)
      {
        sent += SendEventToStreams(ltevent);
      }
      if(m_current & EDT_LongPolling)
      {
        PlaceInLongPollingQueue(ltevent);
        LogLongPolling();
        ++sent;
      }
      else
      {
        delete ltevent;
      }
      ltevent = nullptr;
    }
  }
  catch(StdException& ex)
  {
    ERRORLOG(ERROR_UNHANDLED_EXCEPTION,_T("ServerEventChannel error while processing event queue: ") + ex.GetErrorMessage());
  }
  // Return number of sent events, as to scale the waiting
  // time for the ServerEventDrivers monitor
  return sent;
}

// Try to sent as much as possible
// Channel already locked by SendChannel.
int
ServerEventChannel::SendEventToSockets(LTEvent* p_event)
{
  int sent = 0;
  
  // No sockets connected. Nothing to send
  if(m_sockets.empty())
  {
    return 0;
  }

  AllSockets::iterator it = m_sockets.begin();
  while(it != m_sockets.end())
  {
    try
    {
      if(it->m_open == true && (p_event->m_sent == 0 || p_event->m_sent == it->m_sender))
      {
        // Check that we are addressed
        if(p_event->m_sent && (p_event->m_sent != it->m_sender))
        {
          ++it; // Next socket
          continue;
        }
        // Check if the socket was closed on the other side
        USHORT  code = 0;
        XString reason;
        it->m_socket->GetCloseSocket(code,reason);

        // See if it was closed by the client side
        if(code > 0 || !it->m_socket->IsOpenForWriting())
        {
          CloseSocket(it->m_socket);

          AutoCritSec lock(&m_lock);
          it = m_sockets.erase(it);
          // Push "OnClose" for web application to handle
          OnClose(_T(""));
          continue;
        }
        // Make sure channel is now open on the server side and 'in-use'
        if(!m_openSeen)
        {
          OnOpen(_T(""));
        }
        if(it->m_socket->WriteString(p_event->m_payload))
        {
          ++sent;
        }
        else
        {
          CloseSocket(it->m_socket);

          AutoCritSec lock(&m_lock);
          it = m_sockets.erase(it);
          continue;
        }
      }
    }
    catch(StdException& ex)
    {
      XString error;
      error.Format(_T("Error while sending event to WebSocket [%s] : %s"),m_name.GetString(),ex.GetErrorMessage().GetString());
      ERRORLOG(ERROR_INVALID_HANDLE,error);
      m_server->UnRegisterWebSocket(it->m_socket);
      AutoCritSec lock(&m_lock);
      it = m_sockets.erase(it);
      continue;
    }
    // Next socket
    ++it;
  }

  // No more sockets connected. Stop the queue if only sockets are used
  if(m_sockets.empty() && (m_current == EDT_Sockets))
  {
    OnClose(_T(""));
    return 0;
  }
  return sent;
}

// Try to sent as much as possible
// Channel already locked by SendChannel.
int
ServerEventChannel::SendEventToStreams(LTEvent* p_event)
{
  int sent = 0;

  // No streams. nothing to send
  if(m_streams.empty())
  {
    return 0;
  }

  AllStreams::iterator it = m_streams.begin();
  while (it != m_streams.end())
  {
    try
    {
      // Check that we are addressed
      if(p_event->m_sent && (p_event->m_sent != it->m_sender))
      {
        ++it; // Next stream
        continue;
      }

      // Check if the stream is still open
      // Could be closed by the server heartbeat
      if (!m_server->HasEventStream(it->m_stream))
      {
        it = m_streams.erase(it);
        // Push "OnClose" for web application to handle
        OnClose(_T(""));
        continue;
      }

      // Create SSE ServerEvent
      XString type = LTEvent::EventTypeToString(p_event->m_type);
      ServerEvent* event = new ServerEvent(type);
      event->m_id = p_event->m_number;
      if(p_event->m_type == EvtType::EV_Binary)
      {
        Base64 base;
        event->m_data = base.Encrypt(p_event->m_payload);
      }
      else
      {
        // Simply send the payload text
        event->m_data = p_event->m_payload;
        if(p_event->m_type == EvtType::EV_Message && !p_event->m_typeName.IsEmpty())
        {
          event->m_event = p_event->m_typeName;
        }
      }

      // Make sure channel is now open on the server side and 'in-use'
      if(!m_openSeen)
      {
        OnOpen(_T(""));
      }

      // Send the event
      if(m_server->SendEvent(it->m_stream, event))
      {
        ++sent;
      }
      else
      {
        CloseStream(it->m_stream);
        it = m_streams.erase(it);
        continue;
      }
    }
    catch(StdException& ex)
    {
      XString error;
      error.Format(_T("Error while sending event to SSE Stream [%s] : %s"),m_name.GetString(),ex.GetErrorMessage().GetString());
      ERRORLOG(ERROR_INVALID_HANDLE, error);
      it = m_streams.erase(it);
      continue;
    }
    // Next stream
    ++it;
  }
  // No more sockets connected. Stop the queue if only sockets are used
  if(m_streams.empty() && (m_current == EDT_ServerEvents))
  {
    OnClose(_T(""));
    return 0;
  }
  return sent;
}

void
ServerEventChannel::LogLongPolling()
{
  DETAILLOGV(_T("Monitor calls channel [%s] Long-polling is active. %d events in the polling queue."),m_name.GetString(),(int)m_polQueue.size());
}

void
ServerEventChannel::LogNotConnected()
{
  DETAILLOGV(_T("Monitor calls channel [%s] No connection (yet) and no long-polling active. %d events in the queue."),m_name.GetString(),(int)m_outQueue.size());
}

// Flushing a channel directly
// (Works only for sockets and streams)
bool 
ServerEventChannel::FlushChannel()
{
  // Send all sockets and streams
  SendChannel();

  // Wait until long-polling queue has been drained
  if(!m_polQueue.empty())
  {
    size_t size = m_polQueue.size();
    for(size_t index = 0;index < size;++index)
    {
      Sleep(CLOCKS_PER_SEC);
      if(m_polQueue.empty())
      {
        break;
      }
    }
  }

  // Are we empty?
  return m_outQueue.empty() && m_polQueue.empty();
}

void
ServerEventChannel::CloseChannel()
{
  AutoCritSec lock(&m_lock);

  // Begin with sending a close-event to the server application
  // event channels to the client possibly still open.
  if(m_closeSeen == false)
  {
    // Do not come here again
    m_closeSeen = true;

    // Tell it the server application
    if(m_application)
    {
      try
      {
        LTEvent* ltevent  = new LTEvent(EvtType::EV_Close);
        ltevent->m_payload = _T("Channel closed");
        ltevent->m_number  = ++m_maxNumber;
        ltevent->m_sent    = m_appData;

        // Incoming events are destroyed by the receiver
        (*m_application)(ltevent);
      }
      catch(StdException& ex)
      {
        ERRORLOG(ERROR_APPEXEC_INVALID_HOST_STATE,ex.GetErrorMessage());
      }
    }
  }

  // Closing open channels to the client
  m_current = EDT_NotConnected;
  for(const auto& sock : m_sockets)
  {
    CloseSocket(sock.m_socket);
  }
  m_sockets.clear();

  for(const auto& stream : m_streams)
  {
    CloseStream(stream.m_stream);
  }
  m_streams.clear();

  // Clean out the out queue
  for(const auto& ltevent : m_outQueue)
  {
    delete ltevent;
  }
  m_outQueue.clear();

  // Clean out the input queue
  for(const auto& ltevent : m_inQueue)
  {
    delete ltevent;
  }
  m_inQueue.clear();

  // Clean out the long-polling queue
  for(const auto& ltevent : m_polQueue)
  {
    delete ltevent;
  }
  m_polQueue.clear();
}

void 
ServerEventChannel::CloseSocket(WebSocket* p_socket)
{
  try
  {
    DETAILLOGV(_T("Closing WebSocket for event channel [%s] Queue size: %d"), m_name.GetString(),(int)m_outQueue.size());
    p_socket->SendCloseSocket(WS_CLOSE_NORMAL,_T("ServerEventDriver is closing channel"));
    Sleep(200); // Wait for close to be sent before deleting the socket
    p_socket->CloseSocket();
  }
  catch(StdException& ex)
  {
    XString error;
    error.Format(_T("Server event driver cannot close socket [%s] Error: %s"),m_name.GetString(),ex.GetErrorMessage().GetString());
    ERRORLOG(ERROR_INVALID_HANDLE,error);
  }
}

void 
ServerEventChannel::CloseStream(const EventStream* p_stream)
{
  try
  {
    DETAILLOGV(_T("Closing EventStream for event channel [%s] Queue size: %d"),m_name.GetString(),(int)m_outQueue.size());
    m_server->CloseEventStream(p_stream);
  }
  catch(StdException& ex)
  {
    ERRORLOG(ERROR_INVALID_HANDLE,_T("Server event driver cannot close EventStream: ") + ex.GetErrorMessage());
  }
  m_server->AbortEventStream(const_cast<EventStream*>(p_stream));
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
  AutoCritSec lock(&m_lock);
  for(auto& socket : m_sockets)
  {
    socket.m_socket->SetApplicationData(p_data);
  }
  return true;
}

// To be called by the socket handlers only!
void
ServerEventChannel::OnOpen(XString p_message)
{
  // Open seen. Do not generate again for this channel
  m_openSeen = true;

  LTEvent* ltevent   = new LTEvent;
  ltevent->m_payload = p_message;
  ltevent->m_type    = EvtType::EV_Open;
  ltevent->m_number  = 0;
  ltevent->m_sent    = m_appData;
  {
    AutoCritSec lock(&m_lock);
    m_inQueue.push_back(ltevent);
  }
  if(m_driver->GetActive())
  {
  	m_driver->IncomingEvent();
  }
  else
  {
  	Receiving();
  }
}

// Called by socket handler and long-polling handler
void
ServerEventChannel::OnMessage(XString p_message)
{
  LTEvent* ltevent   = new LTEvent;
  ltevent->m_payload = p_message;
  ltevent->m_type    = EvtType::EV_Message;
  ltevent->m_number  = 0;
  ltevent->m_sent    = m_appData;
  {
    AutoCritSec lock(&m_lock);
    m_inQueue.push_back(ltevent);
  }
  if(m_driver->GetActive())
  {
  	m_driver->IncomingEvent();
  }
  else
  {
  	Receiving();
  }
}

void
ServerEventChannel::OnError(XString p_message)
{
  LTEvent* ltevent   = new LTEvent;
  ltevent->m_payload = p_message;
  ltevent->m_type    = EvtType::EV_Error;
  ltevent->m_number  = 0;
  ltevent->m_sent    = m_appData;
  {
    AutoCritSec lock(&m_lock);
    m_inQueue.push_back(ltevent);
  }
  if(m_driver->GetActive())
  {
  	m_driver->IncomingEvent();
  }
  else
  {
  	Receiving();
  }
}

void
ServerEventChannel::OnClose(XString p_message)
{
  LTEvent* ltevent   = new LTEvent;
  ltevent->m_payload = p_message;
  ltevent->m_type    = EvtType::EV_Close;
  ltevent->m_number  = 0;
  ltevent->m_sent    = m_appData;
  {
    AutoCritSec lock(&m_lock);
    m_inQueue.push_back(ltevent);
  }
  if(m_driver->GetActive())
  {
  	m_driver->IncomingEvent();
  }
  else
  {
  	Receiving();
  }
}

void
ServerEventChannel::OnBinary(void* p_data,DWORD p_length)
{
  if(m_openSeen == false)
  {
    m_openSeen = true;
    OnOpen(_T("OpenChannel"));
  }
  LTEvent* ltevent  = new LTEvent;
  ltevent->m_type   = EvtType::EV_Binary;
  ltevent->m_number = p_length;
  ltevent->m_sent   = m_appData;
  TCHAR* buffer = ltevent->m_payload.GetBufferSetLength(p_length);
  _tcscpy_s(buffer,p_length,reinterpret_cast<TCHAR*>(p_data));
  ltevent->m_payload.ReleaseBufferSetLength(p_length);
  {
    AutoCritSec lock(&m_lock);
    m_inQueue.push_back(ltevent);
  }
  if(m_driver->GetActive())
  {
  	m_driver->IncomingEvent();
  }
  else
  {
  	Receiving();
  }
}

// Process the receiving part of the queue
// By pushing it into the threadpool
int
ServerEventChannel::Receiving()
{
  // Cannot process if no application pointer
  if(!m_application || m_inQueue.empty())
  {
    return 0;
  }

  ThreadPool* pool = m_server->GetThreadPool();
  int received = 0;

  LTEvent* ltevent = nullptr;
  while(GetNextIncomingEvent(ltevent))
  {
    if(!m_openSeen && ltevent->m_type != EvtType::EV_Open)
    {
      LTEvent* open = new LTEvent(EvtType::EV_Open);
      pool->SubmitWork(m_application,open);
    }
    // Post it to the thread pool
    if(!pool->SubmitWork(m_application,ltevent))
    {
      // Threadpool not yet open or posting did not go right
      // Leave the in-queue intact from this point on
      break;
    }
    ++received;
  }
  return received;
}

// Sanity check on the channel
void 
ServerEventChannel::CheckChannel()
{
  AutoCritSec lock(&m_lock);

  // See if we must close sockets
  AllSockets::iterator it = m_sockets.begin();
  while(it != m_sockets.end())
  {
    try
    {
      USHORT code = 0;
      XString reason;
      it->m_socket->GetCloseSocket(code,reason);

      // See if it was closed by the client side
      if(it->m_open == false || code > 0 || !it->m_socket->IsOpenForWriting())
      {
        CloseSocket(it->m_socket);

        it = m_sockets.erase(it);
        // Push "OnClose" for web application to handle
        OnClose(_T(""));
        continue;
      }
    }
    catch(StdException& ex)
    {
      ERRORLOG(ERROR_INVALID_HANDLE,_T("Invalid WebSocket memory registration: " + ex.GetErrorMessage()));
      it = m_sockets.erase(it);
      continue;
    }
    // Next socket
    ++it;
  }

  // See if we must close streams
  AllStreams::iterator st = m_streams.begin();
  while(st != m_streams.end())
  {
    try
    {
      // Check if the stream is still open
      // Could be closed by the server heartbeat
      if(!m_server->HasEventStream(st->m_stream))
      {
        st = m_streams.erase(st);
        // Push "OnClose" for web application to handle
        OnClose(_T(""));
        continue;
      }
    }
    catch (StdException& ex)
    {
      ERRORLOG(ERROR_INVALID_HANDLE, _T("Invalid SSE stream registration: " + ex.GetErrorMessage()));
      st = m_streams.erase(st);
      continue;
    }
    // Next stream
    ++st;
  }
}

// Sanity check on channel
bool
ServerEventChannel::CheckChannelPolicy()
{
  bool result = false;

  __try
  { 
    // Overdue closing of streams and sockets
    CheckChannel();

    switch(m_policy)
    {
      default:                                  [[fallthrough]];
      case EVChannelPolicy::DP_NoPolicy:        break;                                              // Wrong! Must have a policy to work
      case EVChannelPolicy::DP_Binary:          result = !m_sockets.empty();                        // Must have sockets to work
                                                break;
      case EVChannelPolicy::DP_HighSecurity:    result = !m_streams.empty();                        // Must have streams to work
                                                break;
      case EVChannelPolicy::DP_Disconnected:    result = m_sockets.empty() && m_streams.empty();    // Disconnected messages only (polling)
                                                break;
      case EVChannelPolicy::DP_Immediate_S2C:   result = !m_sockets.empty() || !m_streams.empty();  // Must have sockets or streams to work
                                                break;
      case EVChannelPolicy::DP_TwoWayMessages:  result = !m_sockets.empty() || m_usePolling;        // Must have socckets or polling to work
                                                break;
      case EVChannelPolicy::DP_NoSockets:       result = !m_streams.empty() || m_usePolling;        // Must have streams or polling to work
                                                break;
      case EVChannelPolicy::DP_SureDelivery:    result = !m_sockets.empty() ||                      // Must have sockets or streams or polling to work
                                                         !m_streams.empty() || m_usePolling;         
                                                break;
    };
  }
  __finally
  {
    // Currently not possible to check this, assume it to be OK.
    // Ignore any locking and memory problems
    result = true;
  }
  return result;
}
