/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: TestWebSocket.cpp
//
// Marlin Server: Internet server/client
// 
// Copyright (c) 2017 ir. W.E. Huisman
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
#include "TestServer.h"
#include "ServerApp.h"
#include "HTTPSite.h"
#include "WebSocket.h"
#include "SiteHandlerWebSocket.h"

// Open, close and 2 messages
int totalChecks = 4;

//////////////////////////////////////////////////////////////////////////
//
// BARE HANDLERS
//
//////////////////////////////////////////////////////////////////////////

void OnOpen(WebSocket* p_socket,WSFrame* /*p_frame*/)
{
  qprintf("Opened a websocket for: %s",p_socket->GetURI());
  --totalChecks;
}

void OnMessage(WebSocket* p_socket,WSFrame* p_frame)
{
  CString message((char*)p_frame->m_data);
  qprintf("Incoming WebSocket [%s] message: %s",p_socket->GetURI(),message);
  --totalChecks;
}

void OnClose(WebSocket* p_socket,WSFrame* p_frame)
{
  CString message((char*)p_frame->m_data);
  if(!message.IsEmpty())
  {
    qprintf("Closing WebSocket message: %s",message);
  }
  qprintf("Closed the websocket for: %s",p_socket->GetURI());
  --totalChecks;
}

//////////////////////////////////////////////////////////////////////////
// 
// Define our WebSocket handler class
//
//////////////////////////////////////////////////////////////////////////

class SiteHandlerTestSocket : public SiteHandlerWebSocket
{
protected:
  virtual bool Handle(HTTPMessage* p_message,WebSocket* p_socket);
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

  // Returning a 'true' will trigger the handling!!
  return true;
}

int
TestWebSocket(HTTPServer* p_server)
{
  int error = 0;

  // If errors, change detail level
  doDetails = false;

  CString url("/MarlinTest/Sockets/");

  xprintf("TESTING WEBSOCKET FUNCTIONS OF THE HTTP SERVER\n");
  xprintf("==============================================\n");

  // Create URL channel to listen to "http://+:port/MarlinTest/Sockets/"
  HTTPSite* site = p_server->CreateSite(PrefixType::URLPRE_Strong,false,TESTING_HTTP_PORT,url);
  if(site)
  {
    // SUMMARY OF THE TEST
    // --- "---------------------------------------------- - ------
    qprintf("HTTPSite for base 'get' test: OK : %s\n",site->GetPrefixURL().GetString());
    qprintf("HTTPSite for base 'put' test: OK : %s\n",site->GetPrefixURL().GetString());
  }
  else
  {
    ++error;
    xerror();
    qprintf("ERROR: Cannot make a HTTP site for: %s\n",(LPCTSTR)url);
    return error;
  }

  // Set a WebSocket handler on the GET handler of this site
  SiteHandlerTestSocket* handler = new SiteHandlerTestSocket();
  site->SetHandler(HTTPCommand::http_get,handler);

  // Start the site explicitly
  if(site->StartSite())
  {
    xprintf("Site started correctly: %s\n",url);
  }
  else
  {
    ++error;
    xerror();
    qprintf("ERROR STARTING SITE: %s\n",(LPCTSTR)url);
  }
  return error;
}

int
StopWebSocket(void)
{
  int errors = 1;
  WebSocket* socket = g_server->GetHTTPServer()->FindWebSocket("/MarlinTest/Socket/socket_123");
  if(socket)
  {
    if(socket->CloseSocket())
    {
      errors = 0;
    }
    g_server->GetHTTPServer()->UnRegisterWebSocket(socket);
  }
  return errors;
}

int 
AfterTestWebSocket(void)
{
  // SUMMARY OF THE TEST
  // ---- "---------------------------------------------- - ------
  qprintf("Serverside WebSocket tests                     : %s\n",totalChecks > 0 ? "INCOMPLETE" : "OK");
  return totalChecks > 0;
}
