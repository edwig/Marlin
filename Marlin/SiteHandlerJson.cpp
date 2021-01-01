/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: SiteHandlerJson.cpp
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
#include "SiteHandlerJson.h"
#include "JSONMessage.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// Remember JSONMessage for this thread in the TLS
__declspec(thread) JSONMessage* g_jsonMessage = nullptr;

// A JSON handler is an override for the HTTP POST handler
// Most likely you need to write an overload of this one
// and provide a 'Handle' method yourself,
//
bool
SiteHandlerJson::PreHandle(HTTPMessage* p_message)
{
  // Setting the cleanup handler. 
  // Guarantee to return to this 'Cleanup', even if we do a SEH!!
  m_site->SetCleanup(this);

  // Create a JSON message for this thread
  g_jsonMessage = new JSONMessage(p_message);

  // Detect XML JSON errors
  if(g_jsonMessage->GetErrorState())
  {
    // Do not further process the JSON message
    // Send message of this request will send HTTP error 400 (Bad request) to the client
    return false;
  }

  // IMPLEMENT YOURSELF: Write your own access mechanism.
  // Maybe by writing an override to this method and calling this one first..
  
  // return true, to enter the default "Handle" for this message
  return true;
}

bool
SiteHandlerJson::Handle(HTTPMessage* p_message)
{
  if(g_jsonMessage)
  {
    return Handle(g_jsonMessage);
  }
  // Do the default handler (BAD REQUEST)
  return SiteHandler::Handle(p_message);
}

// Default is to do actually nothing, but to reset the message
// and send it empty handed back.
// YOU NEED TO OVERRIDE THIS METHOD!
bool
SiteHandlerJson::Handle(JSONMessage* p_message)
{
  // JSONMessage created in the pre-handle stage?
  if(p_message)
  {
    p_message->Reset();
    p_message->SetErrorstate(true);
    p_message->SetLastError("INTERNAL: Unhandled JSON request caught by base HTTPSite::SIteHandlerJson::Handle");
  }
  return true;
}

// Post handler sends the JSON message back, not the HTTPMessage
void
SiteHandlerJson::PostHandle(HTTPMessage* p_message)
{
  if(g_jsonMessage && !g_jsonMessage->GetHasBeenAnswered())
  {
    m_site->SendResponse(g_jsonMessage);
    p_message->SetHasBeenAnswered();
  }
}

void
SiteHandlerJson::CleanUp(HTTPMessage* p_message)
{
  // Cleanup the TLS handle of the JSON message
  if(g_jsonMessage)
  {
    // Check that we did send something
    if(!g_jsonMessage->GetHasBeenAnswered() &&
       !p_message->GetHasBeenAnswered())
    {
      m_site->SendResponse(g_jsonMessage);
    }
    p_message->SetHasBeenAnswered();

    // Cleanup the JSON message
    delete g_jsonMessage;
    g_jsonMessage = nullptr;
    return;
  }
  // Be really sure we did send a response!
  if (!p_message->GetHasBeenAnswered())
  {
    p_message->Reset();
    p_message->SetStatus(HTTP_STATUS_BAD_REQUEST);
    p_message->SetCommand(HTTPCommand::http_response);
    m_site->SendResponse(p_message);
  }
}
