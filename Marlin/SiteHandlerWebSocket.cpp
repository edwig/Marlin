/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: SiteHandlerWebSocket.cpp
//
// Marlin Server: Internet server/client
// 
// Copyright (c) 2014-2021 ir. W.E. Huisman
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
#include "SiteHandlerWebSocket.h"
#include "WebSocket.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// A WebSocket handler is in essence a 'GET' handler
// that will get upgraded to the WebSocket protocol.

bool
SiteHandlerWebSocket::PreHandle(HTTPMessage* /*p_message*/)
{
  // Cleanup need not be called after an error report
  m_site->SetCleanup(nullptr);

  // return true, to enter the default "Handle"
  return true;
}

bool     
SiteHandlerWebSocket::Handle(HTTPMessage* p_message)
{
  bool opened = false;
  HTTPServer* server = m_site->GetHTTPServer();
  CString uri = p_message->GetAbsolutePath();

  // Create socket by absolute path of the incoming URL
  WebSocket* socket = server->CreateWebSocket(uri);

  // Also get the parameters (key & value)
  CrackedURL& url = p_message->GetCrackedURL();
  for(unsigned ind = 0; ind < url.GetParameterCount(); ++ind)
  {
    UriParam* parameter = url.GetParameter(ind);
    socket->AddParameter(parameter->m_key,parameter->m_value);
  }

  // Reset the message and prepare for protocol upgrade
  if(socket->ServerHandshake(p_message))
  {
    p_message->SetCommand(HTTPCommand::http_response);
    p_message->SetStatus(HTTP_STATUS_SWITCH_PROTOCOLS);

    // Our handler should be able to set message handlers
    // change the handshake headers and such or do the
    // Upgraded protocols for the version > 13!
    if(Handle(p_message,socket))
    {
      // Handler must **NOT** handle the response sending!
      // Handler must **NOT** change the status
      HTTP_OPAQUE_ID request = p_message->GetRequestHandle();
      if(request && (p_message->GetStatus() == HTTP_STATUS_SWITCH_PROTOCOLS))
      {
        // Send the response to the client side, confirming that we become a WebSocket
        // Sending the switch protocols and handshake headers as a HTTP 101 status, 
        // but keep the channel OPEN
        m_site->SendResponse(p_message);
        // Flush socket, so object will be created in IIS and client receives confirmation
        server->FlushSocket(request,m_site->GetPrefixURL());

        // Find the internal structures for the server
        p_message->SetRequestHandle(request);
        if(socket->RegisterSocket(p_message))
        {
          if(socket->OpenSocket())
          {
            opened = true;
            // Register the socket at the server, so we can find it
            server->RegisterSocket(socket);
            SITE_DETAILLOGV("Opened a WebSocket [%s] on [%s]",socket->GetIdentityKey().GetString(),uri.GetString());
          }
          else
          {
            SITE_ERRORLOG(ERROR_FILE_NOT_FOUND,"Socket listener not started");
          }
        }
        p_message->SetHasBeenAnswered();
      }
      else
      {
        SITE_ERRORLOG(ERROR_INVALID_FUNCTION,"ServerApp should NOT handle the response message!");
      }
    }
    else
    {
      SITE_ERRORLOG(ERROR_INVALID_FUNCTION,"ServerApp could not register the WebSocket!");
    }
  }
  else
  {
    SITE_ERRORLOG(ERROR_PROTOCOL_UNREACHABLE,"Site could not accept the WebSocket handshake of the client!");
  }

  // If we did not open our WebSocket, remove it from memory
  if(!opened)
  {
    // Remove the socket
    delete socket;
    // Request status reset, so the request will get ended in the mainloop
    p_message->SetStatus(HTTP_STATUS_SERVICE_UNAVAIL);
  }
  return true;
}

// Default handler. Not what you want. Override it!!
// YOU SHOULD MAKE AN OVERRIDE OF THIS CLASS AND 
// AT LEAST ADD AN OnMessage HANDLER TO THE WEBSOCKET
bool
SiteHandlerWebSocket::Handle(HTTPMessage* p_message,WebSocket* /*p_socket*/)
{
  SITE_ERRORLOG(ERROR_INVALID_PARAMETER,"INTERNAL: Unhandled request caught by base HTTPSite::SiteHandlerWebSocket::Handle");
  p_message->Reset();
  p_message->SetStatus(HTTP_STATUS_SERVER_ERROR);
  return false;
}

void
SiteHandlerWebSocket::PostHandle(HTTPMessage* p_message)
{
  if(!p_message->GetHasBeenAnswered())
  {
    // If we come here, we are most definitely in error
    p_message->Reset();
    p_message->SetStatus(HTTP_STATUS_SERVER_ERROR);
    m_site->SendResponse(p_message);
  }
}

void
SiteHandlerWebSocket::CleanUp(HTTPMessage* p_message)
{
  // Check we did send something
  SiteHandler::CleanUp(p_message);
  // Nothing to do for memory
}

