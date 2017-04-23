/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: SiteHandlerWebSocket.cpp
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
#include "SiteHandlerWebSocket.h"
#include "WebSocket.h"

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
  // Create socket by absolute path of the incoming URL
  ServerWebSocket* socket = new ServerWebSocket(p_message->GetAbsolutePath());

  // Also get the parameters (key & value)
  CrackedURL& url = p_message->GetCrackedURL();
  for(unsigned ind = 0; ind < url.GetParameterCount(); ++ind)
  {
    UriParam* parameter = url.GetParameter(ind);
    socket->AddParameter(parameter->m_key,parameter->m_value);
  }

  // Reset the message and prepare for protocol upgrade
  p_message->SetCommand(HTTPCommand::http_response);
  p_message->SetStatus(HTTP_STATUS_SWITCH_PROTOCOLS);
  p_message->GetFileBuffer()->Reset();

  // Perform the server handshake
  if(socket->ServerHandshake(p_message) == false)
  {
    p_message->Reset();
    p_message->SetStatus(HTTP_STATUS_BAD_REQUEST);
    return true;
  }

  // Our handler should be able to set message handlers
  // change the handshake headers and such or do the
  // Upgraded protocols for the version > 13!
  if(Handle(p_message,socket))
  {
    HTTP_REQUEST_ID request = p_message->GetRequestHandle();
    // Send the response to the client side
    // Confirming that we become a WebSocket
    if(request)
    {
      // Sending the switch protocols and handshake headers
      m_site->SendResponse(p_message);
    }

    // Register the request for the socket
    socket->RegisterServerRequest(m_site->GetHTTPServer(),request);

    // Guaranteed to be called on opening of the socket
    socket->OnOpen();

    // Now enter the read loop of the server
    m_site->GetHTTPServer()->ReceiveWebSocket(socket,request);

    // Guaranteed to be called on closing of the socket
    socket->OnClose();
  }
  // WebSocket is now done and closed
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
  if(p_message->GetRequestHandle())
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

