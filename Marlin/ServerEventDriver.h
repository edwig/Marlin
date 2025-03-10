/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: ServerEventDriver.h
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
#include "SiteHandler.h"
#include "SiteHandlerWebSocket.h"
#include "SiteHandlerSoap.h"
#include <map>

//////////////////////////////////////////////////////////////////////////
//
// http(s)://+:port/BaseURL/Sockets/<session>   -> Websockets base URL
// http(s)://+:port/BaseURL/Events/<session>    -> SSE base URL
// http(s)://+:port/BaseURL/Polling/<session>   -> Long polling base URL
//
// Session can be a <routing> of multiple paths e.g.
// https://server.com/Product/Events/database_name/john_doe
// Here the "database_name/john_doe" part is the session name
//
// Sessions can also be named by cookie/value combinations
// In this case we need not be concerned by routing
//
// http(s)://+:port/BaseURL/Sockets  + Cookie: USERGUID=123456-987654321-ABCDEFGH
// http(s)://+:port/BaseURL/Events   + Cookie: USERGUID=123456-987654321-ABCDEFGH
// http(s)://+:port/BaseURL/Polling  + Cookie: USERGUID=123456-987654321-ABCDEFGH
// 
// Here the USERGUID is the cookie name and the value is the part after the '='
// The cookie value denotes the user session.
// The Cookie/value method is the shortest, safest and has the highest performance
//
class HTTPServer;
class WebSocket;
class EventStream;
class ServerEventDriver;
class ServerEventChannel;

// Wait for a maximum of MONITOR_END_LOOPS * MONITOR_END_WAITS milliseconds
// to drain the sending queues for all client channels
#define MONITOR_END_LOOPS   100
#define MONITOR_END_WAITMS  100
// Monitor wakes up every x milliseconds, to be sure if we missed an event
#define MONITOR_INTERVAL_MIN      500
#define MONITOR_INTERVAL_MAX     (10 * CLOCKS_PER_SEC)
// Minimum seconds for a brute-force attack vector
#define BRUTEFORCE_INTERVAL_MIN  (3  * CLOCKS_PER_SEC)
#define BRUTEFORCE_INTERVAL_MAX  (60 * CLOCKS_PER_SEC)

// Handler for incoming WebSocket stream request
class SiteHandlerEventSocket : public SiteHandlerWebSocket
{
public:
  explicit SiteHandlerEventSocket(ServerEventDriver* p_driver) : m_driver(p_driver) {}

  virtual bool Handle(HTTPMessage* p_message,WebSocket* p_socket) override;
protected:
  ServerEventDriver* m_driver;
};

// Handler for incoming SSE stream request
class SiteHandlerEventStream : public SiteHandler
{
public:
  explicit SiteHandlerEventStream(ServerEventDriver* p_driver) : m_driver(p_driver) {}

  bool HandleStream(HTTPMessage* p_message,EventStream* p_stream) override;
private:
  ServerEventDriver* m_driver;
};

// Handler for incoming SOAP requests for long-polling
class SiteHandlerPolling : public SiteHandlerSoap
{
public:
  explicit SiteHandlerPolling(ServerEventDriver* p_driver) : m_driver(p_driver) {}

  bool Handle(SOAPMessage* p_message) override;
private:
  ServerEventDriver* m_driver;
};

//////////////////////////////////////////////////////////////////////////
//
// The Driver
//
//////////////////////////////////////////////////////////////////////////

using ChannelMap  = std::map<int,     ServerEventChannel*>;
using ChanNameMap = std::map<XString, ServerEventChannel*>;
using SenderMap   = std::map<unsigned,clock_t>;

class ServerEventDriver
{
public:
  ServerEventDriver();
 ~ServerEventDriver();

  // STARTING THE EVENT-DRIVER
  // Register our main site in the server
  bool  RegisterSites(HTTPServer* p_server,HTTPSite* p_site);
  // Create the three sites for the event driver for a user session
  int   RegisterChannel(XString p_sessionName,XString p_cookie,XString p_token,XString p_metadata = _T(""));
  // Force the authentication of the cookie
  void  SetForceAuthentication(bool p_force);
  // Setting the brute force attack interval
  bool  SetBruteForceAttackInterval(int p_interval);
  // Change the policy for a channel
  bool  SetChannelPolicy(int p_channel,EVChannelPolicy p_policy,LPFN_CALLBACK p_application = nullptr,UINT64 p_data = 0L);
  // Start the event driver. Open for business if returned true
  bool  StartEventDriver();
  // Stopping the event driver
  bool  StopEventDriver();
  // Check the event channel for proper working
  bool  CheckChannelPolicy(int m_channel);
  // Cookie timout in minutes
  void  SetCookieTimeout(int p_minutes);

  // Flush messages as much as possible for a channel
  bool  FlushChannel(XString p_cookie,XString p_token);
  bool  FlushChannel(int p_channel);
  // RemoveChannel (possibly at the end of an user session)
  bool  UnRegisterChannel(XString p_cookie,XString p_token,bool p_flush = true);
  bool  UnRegisterChannel(int p_channel,bool p_flush = true);
  // Incoming new Socket/SSE Stream
  bool  IncomingNewSocket(HTTPMessage* p_message,WebSocket*   p_socket);
  bool  IncomingNewStream(HTTPMessage* p_message,EventStream* p_stream);
  bool  IncomingLongPoll (SOAPMessage* p_message);

  // GETTERS
  HTTPServer* GetHTTPServer()           { return m_server;    }
  HTTPSite*   GetHTTPSite()             { return m_site;      }
  bool        GetActive()               { return m_active;    }
  bool        GetForceAuthentication()  { return m_force;     }
  size_t      GetNumberOfChannels()     { return m_channels.size(); }
  int         GetBruteForceInterval()   { return m_interval;  }
  int         GetChannelQueueCount (int     p_channel);
  int         GetChannelQueueCount (XString p_session);
  int         GetChannelClientCount(int     p_channel);
  int         GetChannelClientCount(XString p_session);

  // OUR WORKHORSE: Post an event to the client
  // If 'returnToSender' is filled, only this client will receive the message
  int   PostEvent(int p_session,XString p_payload,XString p_returnToSender = _T(""),EvtType p_type = EvtType::EV_Message,XString p_typeName = _T(""));

  // Main loop of the event runner. DO NOT CALL!
  void  EventThreadRunning();
  // Brute force attack detection. Called by the ServerEventChannel
  bool  CheckBruteForceAttack(unsigned p_sender);
  // Incoming event. Called by the ServerEventChannel
  void  IncomingEvent();

private:
  // Reset the driver
  void  Reset();
  // Start a thread for the streaming WebSocket/server-push event interface
  bool  StartEventThread();
  // Find a channel from the routing information
  XString FindChannel(const Routing& p_routing,XString p_base);

  // Find an event session
  ServerEventChannel* FindSession(XString p_cookie,XString p_token);
  ServerEventChannel* FindSession(int p_session);

  // Register incoming event/stream
  bool RegisterSocketByCookie (HTTPMessage* p_message,WebSocket*   p_socket);
  bool RegisterStreamByCookie (HTTPMessage* p_message,EventStream* p_stream);
  bool RegisterSocketByRouting(HTTPMessage* p_message,WebSocket*   p_socket);
  bool RegisterStreamByRouting(HTTPMessage* p_message,EventStream* p_stream);
  bool HandlePollingByCookie  (SOAPMessage* p_message);
  bool HandlePollingByRouting (SOAPMessage* p_message);

  // Working on the channels
  void SendChannels();
  void RecalculateInterval(int p_sent);

  // DATA
  HTTPServer*     m_server { nullptr };     // Our HTTP server
  HTTPSite*       m_site   { nullptr };     // Our Events site
  // Sessions
  bool            m_active { false   };     // Central queue is active
  bool            m_force  { false   };     // Force the authenticaiton
  int             m_nextSession  { 0 };     // Next session number
  ChannelMap      m_channels;               // All channels (by channel number)
  ChanNameMap     m_names;                  // Extra redundant lookup in the channels for speed by session-name
  ChanNameMap     m_cookies;                // Extra redundant lookup in the channels for speed by cookie:value
  int             m_cookieTimeout { 0 };    // Timeout for cookies in minutes
  // Brute force attack on the event channels
  SenderMap       m_senders;                // Last time of sender attach
  int             m_interval { 10 * CLOCKS_PER_SEC };
  // The worker bee
  HANDLE          m_thread { NULL };
  HANDLE          m_event  { NULL };
  // Metadata for secure cookie encryption
  // Requires that the metadata for all cookies are the same
  XString         m_metadata;
  // LOCKING
  CRITICAL_SECTION m_lock;
};
