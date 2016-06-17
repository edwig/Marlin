/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: SiteHandlerOptions.cpp
//
// Marlin Server: Internet server/client
// 
// Copyright (c) 2016 ir. W.E. Huisman
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
#include "SiteHandlerOptions.h"
#include "HTTPMessage.h"
#include "HTTPSite.h"
#include "HTTPServer.h"
#include <winhttp.h>
#include <io.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

bool
SiteHandlerOptions::PreHandle(HTTPMessage* /*p_message*/)
{
  // Cleanup need not be called after an error report
  m_site->SetCleanup(nullptr);

  // Options requests should always be responded to
  return true;
}

// Most likely the ONLY options handler you will ever want
// Nothing more to be done. See RFC 2616 to confirm this.
//
// HOWEVER:
// You could write an overload to handle the content-type
// answer for a specific URL request, so your clients
// can request the content-type of any non-standard URL
bool
SiteHandlerOptions::Handle(HTTPMessage* p_message)
{
  bool server = false;

  // See if it is a server ping
  if(p_message->GetAbsolutePath() == "*")
  {
    // Request is for the total server HTTPSite
    server = true;
  }

  // See RFC 2616 section 9.8 TRACE
  p_message->SetCommand(HTTPCommand::http_response);
  p_message->SetStatus(HTTP_STATUS_OK);
  // Empty the response part. Just to be sure!
  CString empty;
  p_message->GetFileBuffer()->Reset();
  p_message->SetFile(empty);
  // Set the allow header for this site
  if(server)
  {
    p_message->AddHeader("Allow",m_site->GetAllowHandlers());
  }
  // Ready with the trace, continue processing
  return true;
}

void
SiteHandlerOptions::PostHandle(HTTPMessage* p_message)
{
  p_message->SetCommand(HTTPCommand::http_response);
  m_site->SendResponse(p_message);
  SITE_DETAILLOGS("Answered a OPTIONS message from: ",SocketToServer(p_message->GetSender()));
}
