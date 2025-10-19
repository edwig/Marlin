/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: TestWebSocket.cpp
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
#include "TestMarlinServer.h"
#include "TestPorts.h"
#include <ServerApp.h>
#include <HTTPSite.h>
#include <WebSocketMain.h>
#include <SiteHandlerWebSocket.h>

// Open, close and 2 messages
int totalChecks = 4;

XString 
GenerateLargePushMessage()
{
  XString large;
  XString extra;

  for (int x = 0; x < 100; ++x)
  {
    for (int y = 0; y < 20; ++y)
    {
      extra.Format(_T("X%d-"), (x * 100) + y);
      large += extra;
    }
    large += _T("\n");
  }
  return large;
}


//////////////////////////////////////////////////////////////////////////
//
// BARE HANDLERS
//
//////////////////////////////////////////////////////////////////////////

void OnOpen(WebSocket* p_socket,const WSFrame* /*p_frame*/)
{
  qprintf(_T("TEST handler: Opened a websocket for: %s"),p_socket->GetIdentityKey().GetString());
  --totalChecks;
}

void OnMessage(WebSocket* p_socket,const WSFrame* p_frame)
{
  XString message(reinterpret_cast<TCHAR*>(p_frame->m_data));
  qprintf(_T("TEST handler: Incoming WebSocket [%s] message: %s"),p_socket->GetIdentityKey().GetString(),message.GetString());
  --totalChecks;

  if(message.CompareNoCase(_T("RequestClose")) == 0)
  {
    if(p_socket->SendCloseSocket(WS_CLOSE_NORMAL,_T("Marlin TestServer closing socket")))
    {
      qprintf(_T("TEST handler: Sent close message WS_CLOSE_NORMAL to: %s"),p_socket->GetIdentityKey().GetString());
    }
    if(!p_socket->CloseSocket())
    {
      qprintf(_T("TEST handler: Error closing the WebSocket for: %s"),p_socket->GetIdentityKey().GetString());
    }
  }
  else
  {
    // XString msg = GenerateLargePushMessage();
    // p_socket->WriteString(msg);
    p_socket->WriteString(_T("We are the server!"));
  }
}

void OnClose(WebSocket* p_socket,const WSFrame* p_frame)
{
  XString message(reinterpret_cast<TCHAR*>(p_frame->m_data));
  if(!message.IsEmpty())
  {
    qprintf(_T("TEST handler: Closing WebSocket message: %s"),message.GetString());
  }
  qprintf(_T("TEST handler: Closed the WebSocket for: %s"),p_socket->GetIdentityKey().GetString());
  --totalChecks;
}

//////////////////////////////////////////////////////////////////////////
// 
// Define our WebSocket handler class
//
//////////////////////////////////////////////////////////////////////////

class SiteHandlerTestSocket : public SiteHandlerWebSocket
{
public:
  explicit SiteHandlerTestSocket(TestMarlinServer* p_server) : m_server(p_server) {}
protected:
  virtual bool Handle(HTTPMessage* p_message,WebSocket* p_socket) override;
  TestMarlinServer* m_server;
};

bool
SiteHandlerTestSocket::Handle(HTTPMessage* p_message,WebSocket* p_socket)
{
  // We use the default WebSocket handshake
  // So we do not need the HTTPMessage parameter
  UNREFERENCED_PARAMETER(p_message);

  // We only set the message handlers of the socket
  p_socket->SetOnOpen(OnOpen);
  p_socket->SetOnMessage(OnMessage);
  p_socket->SetOnClose(OnClose);

  m_server->m_socket = p_socket->GetIdentityKey();

  // Returning a 'true' will trigger the handling!!
  return true;
}

int
TestMarlinServer::TestWebSocket()
{
  int error = 0;

  // If errors, change detail level
  m_doDetails = false;

  XString url(_T("/MarlinTest/Sockets/"));

  xprintf(_T("TESTING WEBSOCKET FUNCTIONS OF THE HTTP SERVER\n"));
  xprintf(_T("==============================================\n"));

  // Create URL channel to listen to "http://+:port/MarlinTest/Sockets/"
  HTTPSite* site = m_httpServer->CreateSite(PrefixType::URLPRE_Strong,false,TESTING_HTTP_PORT,url,true);
  if(site)
  {
    // --- "---------------------------- - ------
    qprintf(_T("HTTPSite for WebSockets     : OK : %s\n"),site->GetPrefixURL().GetString());
  }
  else
  {
    ++error;
    xerror();
    qprintf(_T("ERROR: Cannot make a HTTP site for: %s\n"),url.GetString());
    return error;
  }

  // Set a WebSocket handler on the GET handler of this site
  SiteHandlerTestSocket* handler = alloc_new SiteHandlerTestSocket(this);
  site->SetHandler(HTTPCommand::http_get,handler);

  // Start the site explicitly
  if(site->StartSite())
  {
    xprintf(_T("Site started correctly: %s\n"),url.GetString());
  }
  else
  {
    ++error;
    xerror();
    qprintf(_T("ERROR STARTING SITE: %s\n"),url.GetString());
  }
  return error;
}

void
TestMarlinServer::StopWebSocket()
{
  try
  {
    WebSocket* socket = m_httpServer->FindWebSocket(m_socket);
    if(socket)
    {
      if(socket->CloseSocket() == false)
      {
        xerror();
      }
      m_httpServer->UnRegisterWebSocket(socket);
    }
    socket = m_httpServer->FindWebSocket(m_socketSecure);
    if(socket)
    {
      if(socket->CloseSocket() == false)
      {
        xerror();
      }
      m_httpServer->UnRegisterWebSocket(socket);
    }
  }
  catch(StdException& ex)
  {
    xerror();
    qprintf(_T("ERROR while closing and unregistering the WebSocket\n"));
    qprintf(ex.GetErrorMessage());
  }
}


int 
TestMarlinServer::AfterTestWebSocket()
{
  // SUMMARY OF THE TEST
  // ---- "---------------------------------------------- - ------
  qprintf(_T("Serverside WebSocket tests                     : %s\n"),totalChecks > 0 ? _T("INCOMPLETE") : _T("OK"));
  return totalChecks > 0;
}
