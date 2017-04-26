/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: TestWebSocket.h
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
#include "TestClient.h"
#include "WebSocket.h"
#include "Analysis.h"

int
TestWebSocketAccept(void)
{
  bool result = false;
  CString clientkey = "dGhlIHNhbXBsZSBub25jZQ==";

  ClientWebSocket socket("ws://localhost/testing");
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

ClientWebSocket* g_socket = nullptr;

//////////////////////////////////////////////////////////////////////////
//
// CALLBACKS FOR WEBSOCKET
//
//////////////////////////////////////////////////////////////////////////

void
OnOpenWebsocket(WebSocket* /*p_socket*/)
{
  // Websocket has been opened
  // --- "---------------------------------------------- - ------
  printf("WebSocket gotten the 'OnOpen' event            : OK\n");
}

void
OnMessageWebsocket(WebSocket* p_socket)
{
  CString message;
  if(p_socket->ReadString(message))
  {
    // --- "---------------------------------------------- - ------
    printf("%-44s : SERVER\n",message.GetString());
  }
  else
  {
    // Websocket without a message string
    // --- "---------------------------------------------- - ------
    printf("WebSocket cannot get the UTF-8 message string  : ERROR\n");
  }
}

void
OnCloseWebsocket(WebSocket* /*p_socket*/)
{
  // Websocket has been opened
  // --- "---------------------------------------------- - ------
  printf("WebSocket gotten the 'OnClose' event           : OK\n");
}

//////////////////////////////////////////////////////////////////////////
//
// CREATE THE WEBSOCKET ON THE CLIENT SIDE

int
TestWebSocket(LogAnalysis* p_log)
{
  int errors = 1;
  CString uri;
  uri.Format("ws://%s:%d/MarlinTest/Sockets/socket_123",MARLIN_HOST,TESTING_HTTP_PORT);

  // Declare a websocket
  ClientWebSocket* socket = new ClientWebSocket(uri);
  g_socket = socket;

  // Connect our logging
  socket->SetLogfile(p_log);
  socket->SetLogging(p_log->GetDoLogging());

  // Connect our handlers
  socket->SetOnOpen   (OnOpenWebsocket);
  socket->SetOnMessage(OnMessageWebsocket);
  socket->SetOnClose  (OnCloseWebsocket);

  // Start the socket by opening
  if(socket->OpenSocket())
  {
    errors = 0;
  }
  // Websocket has been opened
  // --- "---------------------------------------------- - ------
  printf("WebSocket opened for ws://host:port/..._123    : %s\n",errors ? "ERROR" : "OK");

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
