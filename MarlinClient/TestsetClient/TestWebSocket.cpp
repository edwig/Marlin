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

int g_closed = 0;

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
OnMessageWebsocket(WebSocket* p_socket,WSFrame* p_frame)
{
  CString message;
  
  if(p_frame)
  {
    message = ((char*)p_frame->m_data);
  }
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
//       printf("ALL\n");
//       printf(message);
//       printf("END\n");
      // WebSocket without a message string
      // --- "---------------------------------------------- - ------
      printf("%-28s Length: %9d : OK\n",p_socket->GetIdentityKey().GetString(),message.GetLength());
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
  message.Format("ERROR from WebSocket at URI: %s\n",p_socket->GetIdentityKey().GetString());
  if(p_frame)
  {
    message += (char*)p_frame->m_data;
  }
  message += "\n";

  printf(message);
}

void
OnCloseWebsocket(WebSocket* /*p_socket*/,WSFrame* p_frame)
{
  if (p_frame)
  {
    CString message((char*)p_frame->m_data);
    if (!message.IsEmpty())
    {
      printf("CLOSING message: %s\n", message.GetString());
    }
  }

  // Mark as closed
  g_closed = 1;

  // WebSocket has been closed
  // --- "---------------------------------------------- - ------
  printf("WebSocket gotten the 'OnClose' event           : OK\n");
}

//////////////////////////////////////////////////////////////////////////

CString 
CreateLargeMessage()
{
  CString large;

  for(unsigned ind = 0; ind < 200; ++ind)
  {
    for(unsigned num = 0; num < 20; ++num)
    {
      large.AppendFormat("#%d$",(ind * 20) + num);
    }
    large += "=\n";
  }
  large += "*** end ***\n";

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
  else
  {
    if(!socket->WriteString("Hello server, this is the client. Take one!"))
    {
      ++errors;
    }
    else
    {
      if (!socket->WriteString("Hello server, this is the client. Take two!"))
      {
        ++errors;
      }
      else
      {
        // Testing strings that are longer than the TCP/IP buffering for WebSockets
        // So strings longer than typical 8K bytes must be transportable
        // This string is 23501 bytes long
        CString large = CreateLargeMessage();
        if(!socket->WriteString(large))
        {
           ++errors;
        }
      }
    }
    socket->WriteString("RequestClose");
  }

  // Waiting for server write to drain
  int seconds = 2;
  // --- "---------------------------------------------- - ------
  printf("WebSocket waiting for the server %d seconds     : OK\n", seconds);
  Sleep(seconds * CLOCKS_PER_SEC);

  CString key = socket->GetIdentityKey();

  if(!g_closed)
  {
    if (!socket->SendCloseSocket(WS_CLOSE_NORMAL, "TestWebSocket did close the socket"))
    {
      ++errors;
    }
  }

  // WebSocket has been opened
  // --- "---------------------------------------------- - ------
  printf("WebSocket %-28s tests   : %s\n",key.GetString(),errors ? "ERROR" : "OK");

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
