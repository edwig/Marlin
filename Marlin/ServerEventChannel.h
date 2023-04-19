/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: ServerEventChannel.h
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
#include "ThreadPool.h"
#include "LongTermEvent.h"
#include <deque>
#include <vector>

class HTTPServer;
class HTTPMessage;
class SOAPMessage;
class WebSocket;
class EventStream;
class ServerEventDriver;

// Types of channel connections
enum class EventDriverType
{
  EDT_NotConnected = 0
 ,EDT_Sockets      = 1
 ,EDT_ServerEvents = 2
 ,EDT_LongPolling  = 3
};

typedef struct _regSocket
{
  WebSocket* m_socket { nullptr };
  XString    m_url;
  UINT64     m_sender { 0       };
  bool       m_open   { false   };
}
EventWebSocket;

typedef struct _regStream
{
  EventStream* m_stream { nullptr };
  XString      m_url;
  UINT64       m_sender { 0 };
}
EventSSEStream;

using EventQueue = std::deque<LTEvent*>;
using AllStreams = std::vector<EventSSEStream>;
using AllSockets = std::vector<EventWebSocket>;


class ServerEventChannel
{
public:
  ServerEventChannel(ServerEventDriver* p_driver
                    ,int     p_channel
                    ,XString p_sesionName
                    ,XString p_cookie
                    ,XString p_token);
 ~ServerEventChannel();

  // Send from this channel (if possible)
  int  SendChannel();
  // Process the receiving part of the queue
  int  Receiving();
  // Sanity check on channel
  void CheckChannel();
  // Post a new event, giving a new event numerator
  int  PostEvent(XString p_payload,XString p_sender,EvtType p_type = EvtType::EV_Message,XString p_typeName = "");
  // Flushing a channel directly
  bool FlushChannel();
  // Closing an event channel
  void CloseChannel();
  // Reset the channel. No more events to the server application
  void Reset();

  // Register new incoming channels
  bool RegisterNewSocket(HTTPMessage* p_message,WebSocket*   p_socket,bool p_check = false);
  bool RegisterNewStream(HTTPMessage* p_message,EventStream* p_stream,bool p_check = false);
  bool HandleLongPolling(SOAPMessage* p_message,bool p_check = false);
  // Change event policy. The callback will receive the LTEVENT as the void* parameter
  bool ChangeEventPolicy(EVChannelPolicy p_policy,LPFN_CALLBACK p_application,UINT64 p_data);

  // GETTERS
  int     GetChannel()      { return m_channel; }
  XString GetChannelName()  { return m_name;    }
  XString GetToken()        { return m_token;   }
  XString GetCookieToken();
  int     GetQueueCount();
  int     GetClientCount();
  // Current active type
  EventDriverType GetDriverType() { return m_current; }

  // To be called by the socket handlers only!
  void    OnOpen   (XString p_message);
  void    OnMessage(XString p_message);
  void    OnError  (XString p_message);
  void    OnClose  (XString p_message);
  void    OnBinary (void* p_data,DWORD p_length);
  void    OnOpenSocket (WebSocket* p_socket);
  void    OnCloseSocket(WebSocket* p_socket);

private:
  void CloseSocket(WebSocket* p_socket);
  void CloseStream(EventStream* p_stream);
  int  SendQueueToSocket();
  int  SendQueueToStream();
  int  LogLongPolling();
  int  LogNotConnected();
  bool RemoveEvents(int p_number);

  // DATA
  XString             m_name;
  XString             m_cookie;
  XString             m_token;
  int                 m_channel     { 0 };
  bool                m_active      { false   };
  EVChannelPolicy     m_policy      { EVChannelPolicy::DP_SureDelivery   };
  EventDriverType     m_current     { EventDriverType::EDT_NotConnected };
  AllStreams          m_streams;
  AllSockets          m_sockets;
  // Application we are working for
  LPFN_CALLBACK       m_application { nullptr };
  UINT64              m_appData     { 0L      };
  // All events to be sent
  EventQueue          m_outQueue;
  int                 m_maxNumber   { 0 };
  int                 m_minNumber   { 0 };
  INT64               m_lastSending { 0 };
  // All incoming events from the client
  EventQueue          m_inQueue;
  bool                m_openSeen    { false };
  bool                m_closeSeen   { false };
  // We belong to this driver
  ServerEventDriver*  m_driver      { nullptr };
  HTTPServer*         m_server      { nullptr };
  // LOCKING
  CRITICAL_SECTION    m_lock;
};
