/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: SiteHandlerConnect.cpp
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
#include "SiteHandlerConnect.h"
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
SiteHandlerConnect::PreHandle(HTTPMessage* /*p_message*/)
{
  // Cleanup need not be called after an error report
  m_site->SetCleanup(nullptr);

  // Most likely to be answered
  return true;
}

// Most likely the ONLY connect handler you will ever want
// Nothing more to be done. See RFC 2616 to confirm this.
bool
SiteHandlerConnect::Handle(HTTPMessage* p_message)
{
  p_message->Reset();
  // See RFC 2616 section 9.9 CONNECT
  p_message->SetCommand(HTTPCommand::http_response);
  p_message->SetStatus(HTTP_STATUS_OK);
  // Empty the response part. Just to be sure!
  CString empty;
  p_message->GetFileBuffer()->Reset();
  p_message->SetFile(empty);

  // Ready with the connect, continue processing
  return true;
}

// Send response and log a HEAD message
void
SiteHandlerConnect::PostHandle(HTTPMessage* p_message)
{
  // send our answer straight away, if not already done
  if(p_message && !p_message->GetHasBeenAnswered())
  {
    p_message->SetCommand(HTTPCommand::http_response);
    m_site->SendResponse(p_message);
    SITE_DETAILLOGS("Answered a CONNECT message from: ", SocketToServer(p_message->GetSender()));
  }
}
