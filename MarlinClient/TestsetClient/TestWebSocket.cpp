/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: TestWebSocket.h
//
// Marlin Server: Internet server/client
// 
// Copyright (c) 2015-2018 ir. W.E. Huisman
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
#include "TestClient.h"
#include "WebSocketServer.h"
#include "WebSocketClient.h"
#include "Analysis.h"

// Stand-alone test without client or server
// See if the handshake works as described in the IETF RFC 6455
int
TestWebSocketAccept(void)
{
  bool result = false;
  CString clientkey = "dGhlIHNhbXBsZSBub25jZQ==";

  WebSocketServer socket("ws://localhost/testing");
  CString serverkey = socket.ServerAcceptKey(clientkey);

  xprintf("Client Key: %s\n",clientkey.GetString());
  xprintf("Server Key: %s\n",serverkey.GetString());

  // See the example in RFC 6455 on page 24 of the IETF
  if(serverkey.Compare("s3pPLMBiTxaQ9kYGzzhZRbK+xOo=") == 0)
  {
    result = true;
  }

  // SUMMARY OF THE TEST
  // --- "---------------------------------------------- - ------
  printf("Testing the WebSocket Accept cookie RFC6455    : %s\n",result ? "OK" : "ERROR");

  return result ? 0 : 1;
}

WebSocketClient* g_socket = nullptr;

//////////////////////////////////////////////////////////////////////////
//
// CALLBACKS FOR WEBSOCKET
//
//////////////////////////////////////////////////////////////////////////

void
OnOpenWebsocket(WebSocket* /*p_socket*/,WSFrame* /*p_frame*/)
{
  // WebSocket has been opened
  // --- "---------------------------------------------- - ------
  printf("WebSocket gotten the 'OnOpen' event            : OK\n");
}

void
OnMessageWebsocket(WebSocket* /*p_socket*/,WSFrame* p_frame)
{
  CString message((char*)p_frame->m_data);

  if(!message.IsEmpty())
  {
    if(message.GetLength() < 100)
    {
      // Incoming Message from the server
      // --- "---------------------------------------------- - ------
      printf("%-46s : OK\n",message.GetString());
    }
    else
    {
      printf("ALL\n");
      printf(message);
      printf("END\n");
    }
  }
  else
  {
    // WebSocket without a message string
    // --- "---------------------------------------------- - ------
    printf("WebSocket cannot get the UTF-8 message string  : ERROR\n");
  }
}

void
OnErrorWebSocket(WebSocket* p_socket,WSFrame* p_frame)
{
  CString message;
  message.Format("ERROR from WebSocket at URI: %s\n",p_socket->GetURI().GetString());
  message += (char*) p_frame->m_data;
  message += "\n";

  printf(message);
}

void
OnCloseWebsocket(WebSocket* /*p_socket*/,WSFrame* p_frame)
{
  CString message((char*)p_frame->m_data);
  if(!message.IsEmpty())
  {
    printf("CLOSING message: %s\n",message.GetString());
  }

  // WebSocket has been closed
  // --- "---------------------------------------------- - ------
  printf("WebSocket gotten the 'OnClose' event           : OK\n");
}

//////////////////////////////////////////////////////////////////////////

CString 
GetLargeMessage()
{
  CString large;
  CString part1,part2,part3;

  for(unsigned ind = 0; ind < 60; ++ind)
  {
    part1 += "#";
    part2 += "$";
    part3 += "=";
  }

  large = part1 + "\n" + part2 + "\n" + part3 + "\n";

  part2.Empty();
  for(unsigned ind = 0; ind < 20; ++ind)
  {
    part2 += large;
  }

  large.Empty();
  for(unsigned ind = 0; ind < 20; ++ind)
  {
    large += part2;
    large += "*** end ***\n";
  }

  return large;
}

//////////////////////////////////////////////////////////////////////////
//
// CREATE THE WEBSOCKET ON THE CLIENT SIDE
//
//////////////////////////////////////////////////////////////////////////

int
TestWebSocket(LogAnalysis* p_log)
{
  int errors = 0;
  CString uri;
  uri.Format("ws://%s:%d/MarlinTest/Sockets/socket_123",MARLIN_HOST,TESTING_HTTP_PORT);

  // Independent 3th party test website, to check whether our WebSocket works correct
  // Comment out to test against our own Marlin server sockets
  // Works only for IIS Marlin, not for stand-alone Marlin
  // uri = "ws://echo.websocket.org";

  // Declare a WebSocket
  WebSocketClient* socket = new WebSocketClient(uri);
  g_socket = socket;

  // Connect our logging
  socket->SetLogfile(p_log);
  socket->SetLogLevel(p_log->GetLogLevel());

  // Connect our handlers
  socket->SetOnOpen   (OnOpenWebsocket);
  socket->SetOnMessage(OnMessageWebsocket);
  socket->SetOnError  (OnErrorWebSocket);
  socket->SetOnClose  (OnCloseWebsocket);

  // Start the socket by opening
  // Receiving thread is now running on the HTTPClient
  if(socket->OpenSocket() == false)
  {
    ++errors;
  }
  if(!socket->WriteString("Hello server, this is the client. Take one!"))
  {
    ++errors;
  }
  Sleep(500);
  if(!socket->WriteString("Hello server, this is the client. Take two!"))
  {
    ++errors;
  }
  Sleep(500);

  // Testing strings that are longer than the TCP/IP buffering for WebSockets
  // So strings longer than typical 16K bytes must be testable
  //   CString large = GetLargeMessage();
  //   if(!socket->WriteString(large))
  //   {
  //     ++errors;
  //   }
  //   Sleep(5000);

  if(!socket->SendCloseSocket(WS_CLOSE_NORMAL,"TestWebSocket did close the socket"))
  {
    ++errors;
  }

  // WebSocket has been opened
  // --- "---------------------------------------------- - ------
  printf("WebSocket tests for ws://host:port/..._123     : %s\n",errors ? "ERROR" : "OK");

  return errors;
}

int
TestCloseWebSocket()
{
  if(g_socket)
  {
    g_socket->CloseSocket();
    delete g_socket;
  }
  return 0;
}
