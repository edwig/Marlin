/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: SiteHandler.cpp
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
#include "SiteHandler.h"
#include "HTTPMessage.h"

#ifdef _AFX
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#endif

SiteHandler::SiteHandler()
{
}

// Delete the complete site handler chain
SiteHandler::~SiteHandler()
{
  if(m_next && m_nextowner)
  {
    delete m_next;
    m_next = nullptr;
  }
}

// Default nothing to do
// Implement your own for initialization reasons!
void
SiteHandler::OnStartSite()
{
}

// Default nothing to do
// Implement your own for initialization reasons!
void
SiteHandler::OnStopSite()
{
}

bool 
SiteHandler::HandleStream(HTTPMessage* /*p_message*/,EventStream* /*p_stream*/)
{
  // Nothing to do
  return false;
}

// Go handle this message
// This is the main loop of handling a message, and it 
// guarantees the calling of pre-handle / handle / post-handle order
void 
SiteHandler::HandleMessage(HTTPMessage* p_message)
{
  // Keep calling as long as it returns 'true'
  if(PreHandle(p_message))
  {
    if(Handle(p_message))
    {
      PostHandle(p_message);
    }
    else if(m_next)
    {
      m_next->HandleMessage(p_message);
    }
  }
  // Guaranteed to be always called
  CleanUp(p_message);
}
  
// Default pre handler. 
bool  
SiteHandler::PreHandle(HTTPMessage* /*p_message*/)
{
  // Cleanup need not be called after an error report
  m_site->SetCleanup(nullptr);
  return true;
}

// Default handler. Not what you want. Override it!!
bool     
SiteHandler::Handle(HTTPMessage* p_message)
{
  SITE_ERRORLOG(ERROR_INVALID_PARAMETER,_T("INTERNAL: Unhandled request caught by base HTTPSite::SiteHandler::Handle"));
  p_message->Reset();
  p_message->SetStatus(HTTP_STATUS_BAD_REQUEST);
  return true;
}

// Default post handler. Send back to the server
void
SiteHandler::PostHandle(HTTPMessage* p_message)
{
  if(p_message && !p_message->GetHasBeenAnswered())
  {
    p_message->SetCommand(HTTPCommand::http_response);
    m_site->SendResponse(p_message);
  }
}

void
SiteHandler::CleanUp(HTTPMessage* p_message)
{
  // Be really sure we did send a response!
  if(!p_message->GetHasBeenAnswered())
  {
    p_message->SetCommand(HTTPCommand::http_response);
    m_site->SendResponse(p_message);
  }
  // Nothing to do for memory cleanup
}

void         
SiteHandler::SetNextHandler(SiteHandler* p_next,bool p_owner)
{
  m_next      = p_next;
  m_nextowner = p_owner;
}
