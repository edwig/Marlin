/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: SiteHandlerPost.cpp
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
#include "SiteHandlerPost.h"
#include "HTTPMessage.h"
#include "HTTPSite.h"
#include "HTTPServer.h"
#include <winhttp.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

bool
SiteHandlerPost::PreHandle(HTTPMessage* /*p_message*/)
{
  // Cleanup need not be called after an error report
  m_site->SetCleanup(nullptr);

  // Check server authorization for server GUID!!
  // XString author = p_message->GetCookieValue();
  // IMPLEMENT YOURSELF: Check authorization (HTTP_STATUS_DENIED)?

  // return true, to enter the default "Handle"
  return true;
}

// BEWARE!
// Not what you want to do here. So why is this code here?
// You should write your own override of this function, because this one 
// does not have any security checking whatsoever!!!!!
// But it shows, how it's done. So use this example
bool
SiteHandlerPost::Handle(HTTPMessage* p_message)
{
  // Getting the primary information
  XString pathname = m_site->GetWebroot() + p_message->GetAbsolutePath();
  XString body     = p_message->GetBody();

  // Reset the message
  p_message->Reset();
  p_message->GetFileBuffer()->ResetFilename();
  p_message->SetCommand(HTTPCommand::http_response);
  p_message->SetStatus(HTTP_STATUS_DENIED);

  if(body.IsEmpty())
  {
    p_message->SetStatus(HTTP_STATUS_BAD_REQUEST);
  }
  else
  {
    // Now do something with the body
    // If it's a script run it. etcetera...
    if(DoPostAction(pathname,body,p_message->GetCrackedURL()))
    {
      // Remember result
      p_message->SetStatus(HTTP_STATUS_OK);
    }
  }
  // Ready with the post
  return true;
}

void
SiteHandlerPost::PostHandle(HTTPMessage* p_message)
{
  if(p_message && !p_message->GetHasBeenAnswered())
  {
    // Send response back
    p_message->SetCommand(HTTPCommand::http_response);
    m_site->SendResponse(p_message);
  }
}

// DO NOTHING: OVVERRIDE ME!
// IMPLEMENT YOURSELF: YOUR IMPLEMENTATION HERE
bool
SiteHandlerPost::DoPostAction(XString p_filename,XString p_body,const CrackedURL& p_full)
{
  UNREFERENCED_PARAMETER(p_filename);
  UNREFERENCED_PARAMETER(p_body);
  UNREFERENCED_PARAMETER(p_full);
  return false;
}

