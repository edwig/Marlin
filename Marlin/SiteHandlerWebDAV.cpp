/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: SiteHandlerWebDAV.cpp
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
#include "SiteHandlerWebDAV.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

bool  
SiteHandlerWebDAV::PreHandle(HTTPMessage* /*p_message*/)
{
  // Cleanup need not be called after an error report
  m_site->SetCleanup(nullptr);

  // WebDAV requests should always be responded to
  return true;
}

// General WebDAV Dispatcher
// No need to write another one
bool
SiteHandlerWebDAV::Handle(HTTPMessage* p_message)
{
  switch(p_message->GetCommand())
  {
    case HTTPCommand::http_move:       WebDavMove  (p_message); break;
    case HTTPCommand::http_copy:       WebDavCopy  (p_message); break;
    case HTTPCommand::http_proppfind:  WebDavPFind (p_message); break;
    case HTTPCommand::http_proppatch:  WebDavPPatch(p_message); break;
    case HTTPCommand::http_mkcol:      WebDavMkCol (p_message); break;
    case HTTPCommand::http_lock:       WebDavLock  (p_message); break;
    case HTTPCommand::http_unlock:     WebDavUnlock(p_message); break;
    case HTTPCommand::http_search:     WebDavSearch(p_message); break;
    default:                           // Unhandled message type
                                       p_message->Reset();
                                       p_message->SetStatus(HTTP_STATUS_SERVER_ERROR);
                                       break;
  }
  return true;
}


void 
SiteHandlerWebDAV::PostHandle(HTTPMessage* p_message)
{
  if(!p_message->GetHasBeenAnswered())
  {
    // send our answer 
    p_message->SetCommand(HTTPCommand::http_response);
    m_site->SendResponse(p_message);
    SITE_DETAILLOGS("Answered a WEBDAV message from: ", SocketToServer(p_message->GetSender()));
  }
}

// IMPLEMENT YOURSELF:
// You will most likely want to override this message in a handler of your own
// to implement the CMS WebDAV protocol for yourself. Do not use this one!
//
// NOTE: You will most likely want to override ALL of these methods or none
// As all the WebDAV calls end up here!
//
void 
SiteHandlerWebDAV::WebDavMove(HTTPMessage* p_message)
{
  p_message->Reset();
  p_message->SetStatus(HTTP_STATUS_SERVER_ERROR);
}

void 
SiteHandlerWebDAV::WebDavCopy(HTTPMessage* p_message)
{
  p_message->Reset();
  p_message->SetStatus(HTTP_STATUS_SERVER_ERROR);
}

void 
SiteHandlerWebDAV::WebDavPFind(HTTPMessage* p_message)
{
  p_message->Reset();
  p_message->SetStatus(HTTP_STATUS_SERVER_ERROR);
}

void 
SiteHandlerWebDAV::WebDavPPatch(HTTPMessage* p_message)
{
  p_message->Reset();
  p_message->SetStatus(HTTP_STATUS_SERVER_ERROR);
}

void 
SiteHandlerWebDAV::WebDavMkCol(HTTPMessage* p_message)
{
  p_message->Reset();
  p_message->SetStatus(HTTP_STATUS_SERVER_ERROR);
}

void 
SiteHandlerWebDAV::WebDavLock(HTTPMessage* p_message)
{
  p_message->Reset();
  p_message->SetStatus(HTTP_STATUS_SERVER_ERROR);
}

void 
SiteHandlerWebDAV::WebDavUnlock(HTTPMessage* p_message)
{
  p_message->Reset();
  p_message->SetStatus(HTTP_STATUS_SERVER_ERROR);
}

void 
SiteHandlerWebDAV::WebDavSearch(HTTPMessage* p_message)
{
  p_message->Reset();
  p_message->SetStatus(HTTP_STATUS_SERVER_ERROR);
}
