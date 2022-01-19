/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: ServerEventDriver.cpp
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
#include "ServerEventDriver.h"
#include "SiteHandlerOptions.h"
#include "HTTPServer.h"
#include "WebSocket.h"
#include "AutoCritical.h"

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

// A new WebSocket stream is incoming. A client has decided to use this method!
// So go deal with it: Destroy SSE stream.
bool
SiteHandlerEventSocket::Handle(HTTPMessage* p_message,WebSocket* p_socket)
{
  return m_driver->IncomingNewSocket(p_message,p_socket);
}

// A new SSE stream is incoming. A client has decided to use this method!
// So go deal with it: destroy WebSocket.
void
SiteHandlerEventStream::HandleStream(HTTPMessage* p_message,EventStream* p_stream)
{
  m_driver->IncomingNewStream(p_message,p_stream);
}

bool
SiteHandlerPolling::Handle(SOAPMessage* p_message)
{
  return m_driver->IncomingLongPoll(p_message);
}

//////////////////////////////////////////////////////////////////////////
//
// EVENT DRIVER
//
//////////////////////////////////////////////////////////////////////////


ServerEventDriver::ServerEventDriver()
{
  InitializeCriticalSection(&m_lock);
}

ServerEventDriver::~ServerEventDriver()
{
  Reset();
  DeleteCriticalSection(&m_lock);
}

// Register our main site in the server
// Needed for the handlers to find this object!
bool
ServerEventDriver::RegisterSites(HTTPServer* p_server,HTTPSite* p_site)
{
  m_server = p_server;
  m_site   = p_site;
  int started = 0;

  HTTPServer* server = m_site->GetHTTPServer();
  CString baseURL = m_site->GetSite();
  int  portNumber = m_site->GetPort();
  bool siteSecure = m_site->GetPrefixURL()[4] == 's';

  // Start socket site
  CString socketSiteURL = baseURL + "Sockets/";
  HTTPSite*  socketSite = server->CreateSite(PrefixType::URLPRE_Strong,siteSecure,portNumber,socketSiteURL,true);
  if(socketSite)
  {
    CString urlPrefix = socketSite->GetPrefixURL();
    server->DetailLog(__FUNCTION__,LogType::LOG_INFO,"Registered WebSocket EventDriver for: " + urlPrefix);

    SiteHandler* handler = new SiteHandlerEventSocket(this);
    socketSite->SetHandler(HTTPCommand::http_get,handler);
    socketSite->SetHandler(HTTPCommand::http_options,new SiteHandlerOptions());
    socketSite->SetAllHeaders(true);

    if(socketSite->StartSite()) ++started;
  }

  // Start SSE site
  CString eventsSiteURL = baseURL + "Events/";
  HTTPSite*  eventsSite = server->CreateSite(PrefixType::URLPRE_Strong,siteSecure,portNumber,eventsSiteURL,true);
  if(eventsSite)
  {
    CString urlPrefix = eventsSite->GetPrefixURL();
    server->DetailLog(__FUNCTION__,LogType::LOG_INFO,"Registered SSSE EventDriver for: " + urlPrefix);

    SiteHandler* handler = new SiteHandlerEventStream(this);
    eventsSite->SetHandler(HTTPCommand::http_get,handler);
    eventsSite->SetHandler(HTTPCommand::http_options,new SiteHandlerOptions());

    // Tell site we handle SSE streams
    eventsSite->SetIsEventStream(true);
    eventsSite->SetAllHeaders(true);
    eventsSite->AddContentType("txt","text/event-stream");

    // Server must now do keep-alive jobs for SSE streams
    server->SetEventKeepAlive(5000);

    // And start the site
    if(eventsSite->StartSite()) ++started;
  }

  // Start Polling site
  CString pollingSiteURL = baseURL + "Polling/";
  HTTPSite*  pollingSite = server->CreateSite(PrefixType::URLPRE_Strong,siteSecure,portNumber,pollingSiteURL,true);
  if(pollingSite)
  {
    CString urlPrefix = eventsSite->GetPrefixURL();
    server->DetailLog(__FUNCTION__,LogType::LOG_INFO,"Registered Long-Polling for: " + urlPrefix);

    SiteHandler* handler = new SiteHandlerPolling(this);
    pollingSite->SetHandler(HTTPCommand::http_post,handler);
    pollingSite->SetHandler(HTTPCommand::http_options,new SiteHandlerOptions());
    pollingSite->SetAllHeaders(true);
    pollingSite->AddContentType("xml","application/soap+xml");

    // And start the site
    if(pollingSite->StartSite()) ++started;
  }

  // Error handling
  if(started < 3)
  {
    ERRORLOG(ERROR_SERVICE_NOT_ACTIVE,"Not all three subsites (Sockets/Events/Polling) have been started for the ServerEventDriver!");
    return false;
  }
  return true;
}

int
ServerEventDriver::RegisterChannel(CString p_sessionName
                                  ,CString p_cookie
                                  ,CString p_token)
{
  AutoCritSec lock(&m_lock);

  // Test for duplicate registration
  ServerEventChannel* channel = FindSession(p_cookie,p_token);
  if(channel)
  {
    return 0;
  }
  // Make session and store with all sessions
  channel = new ServerEventChannel(this,++m_nextSession,p_sessionName,p_cookie,p_token);
  m_channels.insert(std::make_pair(m_nextSession,channel));

  // Extra lookups for incoming streams
  CString cookie = p_cookie + ":" + p_token;
  m_names  .insert(std::make_pair(p_sessionName,channel));
  m_cookies.insert(std::make_pair(cookie,channel));

  return m_nextSession;
}

// Force the authentication of the cookie
void
ServerEventDriver::SetForceAuthentication(bool p_force)
{
  m_force = p_force;
}

// Setting the brute force attack interval
bool
ServerEventDriver::SetBruteForceAttackInterval(int p_interval)
{
  if(p_interval >= BRUTEFORCE_INTERVAL_MIN && p_interval <= BRUTEFORCE_INTERVAL_MAX)
  {
    m_interval = p_interval;
    return true;
  }
  return false;
}

// Set or change the policy for a channel
// To be called after 'RegisterChannel' or on a later moment to change the policy
bool
ServerEventDriver::SetChannelPolicy(int              p_channel
                                   ,EVChannelPolicy  p_policy
                                   ,LPFN_CALLBACK    p_application /*= nullptr*/
                                   ,UINT64           p_data        /*= nullptr*/)
{
  ChannelMap::iterator it = m_channels.find(p_channel);

  if(it != m_channels.end())
  {
    return it->second->ChangeEventPolicy(p_policy,p_application,p_data);
  }
  return false;
}

// Flush messages as much as possible for a channel
bool
ServerEventDriver::FlushChannel(CString p_cookie,CString p_token)
{
  ServerEventChannel* session = FindSession(p_cookie,p_token);
  if(session)
  {
    return FlushChannel(session->GetChannel());
  }
  return false;
}

bool
ServerEventDriver::FlushChannel(int p_channel)
{
  AutoCritSec lock(&m_lock);

  ChannelMap::iterator it = m_channels.find(p_channel);
  if(it != m_channels.end())
  {
    ServerEventChannel* channel = it->second;
    return channel->FlushChannel();
  }
  return false;
}

// RemoveChannel (possibly at the end of an user session)
bool
ServerEventDriver::UnRegisterChannel(CString p_cookie,CString p_token,bool p_flush /*=true*/)
{
  ServerEventChannel* session = FindSession(p_cookie,p_token);
  if(session)
  {
    AutoCritSec lock(&m_lock);

    if(session->FlushChannel())
    {
      // If flushing is unsuccessful, the channel remains for later polling
      return UnRegisterChannel(session->GetChannel(),p_flush);
  }
  }
  return false;
}

bool
ServerEventDriver::UnRegisterChannel(int p_channel,bool p_flush /*=true*/)
{
  // See if we can flush the channel
  if(p_flush)
  {
    if(!FlushChannel(p_channel))
    {
      // If not, the channel stays for now
      return false;
    }
  }

  // Go on and lock the channels
  AutoCritSec lock(&m_lock);

  ChannelMap::iterator it = m_channels.find(p_channel);
  if(it != m_channels.end())
  {
    ServerEventChannel* channel = it->second;

    // Erase from names speedup
    ChanNameMap::iterator nm = m_names.find(channel->GetChannelName());
    if(nm != m_names.end())
    {
      m_names.erase(nm);
    }
    // Erase from cookies speedup
    ChanNameMap::iterator ck = m_cookies.find(channel->GetCookieToken());
    if(ck != m_cookies.end())
    {
      m_cookies.erase(ck);
    }

    // Now go close the channel
    channel->CloseChannel();

    // Delete the channel completely
    delete channel;
    m_channels.erase(it);
    return true;
  }
  return false;
}

// Start the event driver. Open for business if returned true
bool
ServerEventDriver::StartEventDriver()
{
  if(m_server && m_site)
  {
    m_active = true;
    StartEventThread();
  }
  return m_active;
}

// Stopping the event driver
bool
ServerEventDriver::StopEventDriver()
{
  DETAILLOG1("Stopping ServerEventDriver");

  // No more new postings from now on
  m_active = false;
  // Try to clear the queues one more time
  SetEvent(m_event);

  for(int index = 0;index < MONITOR_END_LOOPS;++index)
  {
    // See if monitor is already stopped sending
    if(m_thread == NULL)
    {
      break;
    }
    Sleep(MONITOR_END_WAITMS);
  }
  DETAILLOG1((m_thread == NULL) ? "EventDriver stopped" : "EventDriver still running!!");
  return (m_thread == NULL);
}

// Incoming new WebSocket
bool
ServerEventDriver::IncomingNewSocket(HTTPMessage* p_message,WebSocket* p_socket)
{
  if(!RegisterSocketByCookie(p_message,p_socket))
  {
    if(!RegisterSocketByRouting(p_message,p_socket))
    {
      CString errortext;
      errortext.Format("No registered session found for incoming socket on [%s]",p_socket->GetURI().GetString());
      ERRORLOG(ERROR_NOT_FOUND,errortext);
      return false;
    }
  }
  return true;
}

// Incoming new SSE Stream
void
ServerEventDriver::IncomingNewStream(HTTPMessage* p_message,EventStream* p_stream)
{
  if(!RegisterStreamByCookie(p_message,p_stream))
  {
    if(!RegisterStreamByRouting(p_message,p_stream))
    {
      CString errortext;
      errortext.Format("No registered session found for incoming stream on [%s]",p_stream->m_absPath.GetString());
      ERRORLOG(ERROR_NOT_FOUND,errortext);
      return;
    }
  }
  // Possibly sent messages to newfound channel right away
  SetEvent(m_event);
}

bool
ServerEventDriver::IncomingLongPoll(SOAPMessage* p_message)
{
  if(!HandlePollingByCookie(p_message))
  {
    if(!HandlePollingByRouting(p_message))
    {
      CString errortext;
      errortext.Format("No registered session found for long-polling message [%s]",p_message->GetAbsolutePath().GetString());
      ERRORLOG(ERROR_NOT_FOUND,errortext);
      return false;
    }
  }
  return true;
}

int
ServerEventDriver::PostEvent(int     p_session
                            ,CString p_payload
                            ,CString p_returnToSender /*= ""*/
                            ,EvtType p_type           /*= EvtType::EV_Message*/)
{
  int number = 0;
  if(m_active)
  {
    AutoCritSec lock(&m_lock);

    ServerEventChannel* session = FindSession(p_session);
    if(session)
    {
      number = session->PostEvent(p_payload,p_returnToSender,p_type);
      // Kick the worker bee to start sending
      ::SetEvent(m_event);
    }
  }
  return number;
}

// Incoming event. Called by the ServerEventChannel
void
ServerEventDriver::IncomingEvent()
{
  // Kick the worker bee to start receiving
  ::SetEvent(m_event);
}

// Returns the number of messages in the queue
// -1 if no proper channel number given
int
ServerEventDriver::GetChannelQueueCount(int p_channel)
{
  AutoCritSec lock(&m_lock);

  ChannelMap::iterator it = m_channels.find(p_channel);
  if(it != m_channels.end())
  {
    return it->second->GetQueueCount();
  }
  return -1;
}

// Returns the number of messages in the queue
// -1 if no proper channel session name given
int
ServerEventDriver::GetChannelQueueCount(CString p_session)
{
  AutoCritSec lock(&m_lock);

  ChanNameMap::iterator it = m_names.find(p_session);
  if(it != m_names.end())
  {
    return it->second->GetQueueCount();
  }
  return -1;
}

int
ServerEventDriver::GetChannelClientCount(int p_channel)
{
  AutoCritSec lock(&m_lock);

  ChannelMap::iterator it = m_channels.find(p_channel);
  if(it != m_channels.end())
  {
    return it->second->GetClientCount();
  }
  return 0;
}

int
ServerEventDriver::GetChannelClientCount(CString p_session)
{
  AutoCritSec lock(&m_lock);

  ChanNameMap::iterator it = m_names.find(p_session);
  if(it != m_names.end())
  {
    return it->second->GetClientCount();
  }
  return 0;
}

//////////////////////////////////////////////////////////////////////////
//
// PRIVATE
//
//////////////////////////////////////////////////////////////////////////

// Reset the driver
void  
ServerEventDriver::Reset()
{
  AutoCritSec lock(&m_lock);

  m_nextSession = 0;

  for(auto& session : m_channels)
  {
    session.second->Reset();
  }
  // Remove all session
  for(auto& session : m_channels)
  {
    delete session.second;
  }
  m_channels.clear();
  m_cookies.clear();
  m_names.clear();
}

// Find an event session
// Slow version on cookie/token combination
ServerEventChannel*
ServerEventDriver::FindSession(CString p_cookie,CString p_token)
{
  AutoCritSec lock(&m_lock);
  CString cookieToken = p_cookie + ":" + p_token;

  for(auto& session : m_channels)
  {
    if(session.second->GetCookieToken().Compare(cookieToken) == 0)
    {
      return session.second;
    }
    if(session.second->GetToken().Compare(p_token))
    {
      break;
    }
  }
  return nullptr;
}

// Find an event session
// Fast version on session number
ServerEventChannel*
ServerEventDriver::FindSession(int p_session)
{
  AutoCritSec lock(&m_lock);

  ChannelMap::iterator it = m_channels.find(p_session);
  if(it != m_channels.end())
  {
    return it->second;
  }
  return nullptr;
}

bool 
ServerEventDriver::RegisterSocketByCookie(HTTPMessage* p_message,WebSocket* p_socket)
{
  AutoCritSec lock(&m_lock);

  CString session;
  Cookies& cookies = p_message->GetCookies();
  for(auto& cookie : cookies.GetCookies())
  {
    session = cookie.GetName() + ":" + cookie.GetValue();
    ChanNameMap::iterator it = m_cookies.find(session);
    if(it != m_cookies.end())
    {
      return it->second->RegisterNewSocket(p_message,p_socket);
    }
  }
  return false;
}

bool 
ServerEventDriver::RegisterStreamByCookie(HTTPMessage* p_message,EventStream* p_stream)
{
  AutoCritSec lock(&m_lock);

  CString session;
  Cookies& cookies = p_message->GetCookies();
  for(auto& cookie : cookies.GetCookies())
  {
    session = cookie.GetName() + ":" + cookie.GetValue();
    ChanNameMap::iterator it = m_cookies.find(session);
    if(it != m_cookies.end())
    {
      return it->second->RegisterNewStream(p_message,p_stream);
    }
  }
  return false;
}

bool 
ServerEventDriver::HandlePollingByCookie(SOAPMessage* p_message)
{
  AutoCritSec lock(&m_lock);

  CString session;
  Cookies& cookies = p_message->GetCookies();
  for(auto& cookie : cookies.GetCookies())
  {
    session = cookie.GetName() + ":" + cookie.GetValue();
    ChanNameMap::iterator it = m_cookies.find(session);
    if(it != m_cookies.end())
    {
      return it->second->HandleLongPolling(p_message);
    }
  }
  return false;
}

bool 
ServerEventDriver::RegisterSocketByRouting(HTTPMessage* p_message,WebSocket* p_socket)
{
  AutoCritSec lock(&m_lock);

  // Finding the session name from the routing
  CString channel = FindChannel(p_message->GetRouting(),"Sockets");

  // Find channel by session name
  ChanNameMap::iterator it = m_names.find(channel);
  if(it != m_names.end())
  {
    // Register stream, but cookie needs to be checked!
    return it->second->RegisterNewSocket(p_message,p_socket,m_force);
  }
  return false;
}

bool
ServerEventDriver::RegisterStreamByRouting(HTTPMessage* p_message,EventStream* p_stream)
{
  AutoCritSec lock(&m_lock);

  // Finding the session name from the routing
  CString channel = FindChannel(p_message->GetRouting(),"Events");

  // Find channel by session name
  ChanNameMap::iterator it = m_names.find(channel);
  if(it != m_names.end())
  {
    // Register stream, but cookie needs to be checked!
    return it->second->RegisterNewStream(p_message,p_stream,m_force);
  }
  return false;
}

bool 
ServerEventDriver::HandlePollingByRouting(SOAPMessage* p_message)
{
  AutoCritSec lock(&m_lock);

  // Finding the channel name from the routing
  CString channel = FindChannel(p_message->GetRouting(),"Polling");

  // Find channel by session name
  ChanNameMap::iterator it = m_names.find(channel);
  if(it != m_names.end())
  {
    // Register stream, but cookie needs to be checked!
    return it->second->HandleLongPolling(p_message,m_force);
  }
  return false;
}

// Finding the session name from the routing
// Applications can do "BaseURL/Events/a/b/c" for session "a/b/c"
CString
ServerEventDriver::FindChannel(Routing& p_routing,CString p_base)
{
  CString session;
  bool found = false;
  for(auto& route : p_routing)
  {
    if(route.CompareNoCase(p_base) == 0)
    {
      found = true;
    }
    else if(found)
    {
      if(!session.IsEmpty())
      {
        session += '/';
      }
      session += route;
    }
  }
  return session;
}

// Brute force attack detection
// Sender must include: <server> <desktop> <user> in some fashion
bool
ServerEventDriver::CheckBruteForceAttack(CString p_sender)
{
  p_sender.MakeLower();

  SenderMap::iterator it = m_senders.find(p_sender);
  if(it != m_senders.end())
  {
    // Less than <interval> seconds ago.
    // Cannot make a connection again.
    if ((clock() - (clock_t)it->second) < m_interval)
    {
      m_server->DetailLogV(__FUNCTION__,LogType::LOG_ERROR,"BRUTE FORCE ATTACK FROM: %s",p_sender.GetString());
      return true;
    }
    // Longer than <interval> seconds ago. Reset sender.
    m_senders.erase(it);
    return false;
  }
  // Register the moment of connection
  m_senders[p_sender] = clock();
  return false;
}

//////////////////////////////////////////////////////////////////////////
//
// WORKER BEE
//
//////////////////////////////////////////////////////////////////////////

static unsigned int __stdcall StartingTheDriverThread(void* p_context)
{
  ServerEventDriver* driver = reinterpret_cast<ServerEventDriver*>(p_context);
  if(driver)
  {
    driver->EventThreadRunning();
  }
  return 0;
}

// Start a thread for the streaming websocket/server-push event interface
bool
ServerEventDriver::StartEventThread()
{
  // Create an event for the monitor
  if(!m_event)
  {
    m_event = CreateEvent(NULL,FALSE,FALSE,NULL);
  }

  if(m_thread == NULL || m_active == false)
  {
    // Thread for the client queue
    unsigned int threadID = 0;
    if((m_thread = (HANDLE)_beginthreadex(NULL,0,StartingTheDriverThread,(void*)(this),0,&threadID)) == INVALID_HANDLE_VALUE)
    {
      m_thread = NULL;
      threadID = 0;
      ERRORLOG(ERROR_SERVICE_NOT_ACTIVE,"Cannot start a thread for an ServerEventDriver.");
    }
    else
    {
      DETAILLOGV("Thread started with threadID [%d] for ServerEventDriver.",threadID);
      return true;
    }
  }
  return false;
}

// Main loop of the event runner
void
ServerEventDriver::EventThreadRunning()
{
  // Installing our SEH to exception translator
  _set_se_translator(SeTranslator);

  // Tell we are running
  m_active   = true;
  m_interval = MONITOR_INTERVAL_MIN;

  DETAILLOG1("ServerEventDriver monitor started.");
  do
  {
    DWORD waited = WaitForSingleObjectEx(m_event,m_interval,true);
    switch(waited)
    {
      case WAIT_TIMEOUT:        // Wake up once-in-a-while to be sure
      case WAIT_OBJECT_0:       // Explicit wake up by posting an event
                                SendChannels();
                                break;
      case WAIT_IO_COMPLETION:  // Some I/O completed
      case WAIT_FAILED:         // Failed for some reason
      case WAIT_ABANDONED:      // Interrupted for some reason
      default:                  // Start a new monitor / new event
                                break;
    }                   
  }
  while(m_active);

  // Reset the monitor
  CloseHandle(m_event);
  // Thread is now ready
  m_thread = NULL;
}

void 
ServerEventDriver::RecalculateInterval(int p_sent)
{
  if(p_sent)
  {
    m_interval = MONITOR_INTERVAL_MIN;
  }
  else
  {
    m_interval *= 2;
    if(m_interval > MONITOR_INTERVAL_MAX)
    {
      m_interval = MONITOR_INTERVAL_MAX;
    }
  }
  DETAILLOGV("ServerEventDriver monitor going to sleep. Back in [%d] milliseconds",m_interval);
}


void
ServerEventDriver::SendChannels()
{
  DETAILLOG1("ServerEventDriver monitor waking up. Sending to client channels.");
  int sent = 0;

  try
  {
    ChannelMap channels;
    // Create copy of the channels to be sending to.
    // This makes it possible to halfway through let channels be added or removed.
    {
      AutoCritSec lock(&m_lock);
      channels = m_channels;
    }

    // All outbound traffic
    for(auto& channel : channels)
    {
      sent += channel.second->SendChannel();
    }

    // All inbound traffic
    for(auto& channel : channels)
    {
      sent += channel.second->Receiving();
    }
  }
  catch(StdException& ex)
  {
    ERRORLOG(ERROR_UNHANDLED_EXCEPTION,"ServerEventDriver error: " + ex.GetErrorMessage());
  }
  RecalculateInterval(sent);
}
