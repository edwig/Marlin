/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: SiteHandlerPut.cpp
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
#include "SiteHandlerPut.h"
#include "HTTPMessage.h"
#include "HTTPSite.h"
#include "HTTPServer.h"
#include <WinFile.h>
#include <winhttp.h>

#ifdef _AFX
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#endif

bool
SiteHandlerPut::PreHandle(HTTPMessage* /*p_message*/)
{
  // Cleanup need not be called after an error report
  m_site->SetCleanup(nullptr);

  // Check server authorization for server GUID!!
  // XString author = p_message->GetCookieValue();
  // IMPLEMENT YOURSELF: Check authorization (HTTP_STATUS_DENIED)?

  // return true, to enter the default "Handle"
  return true;
}

// Handle our PUT request
bool
SiteHandlerPut::Handle(HTTPMessage* p_message)
{
  int status = 0;

  // Getting the primary information
  WinFile ensure;
  XString resource = m_site->GetWebroot() + p_message->GetAbsolutePath();
  ensure.SetFilenameFromResource(resource);
  ensure.CreateDirectory();
  // Getting the resulting filename
  XString pathname = ensure.GetFilename();

  FileBuffer* buf = p_message->GetFileBuffer();
  if(buf)
  {
    buf->SetFileName(pathname);
    if(buf->WriteFile())
    {
      // Success!!!
      status = HTTP_STATUS_CREATED;
      SITE_DETAILLOGS(_T("Connection HTTP PUT "),pathname);
    }
    else
    {
      status = HTTP_STATUS_FORBIDDEN;
      DWORD error = GetLastError();
      XString message;
      message.Format(_T("HTTP PUT: File not written: %s\n"),pathname.GetString());
      SITE_ERRORLOG(error,message);
    }
    // RESET THE BUFFER AS FAST AS POSSIBLE
    buf->Reset();
    // Reset the filename
    buf->ResetFilename();
  }
  else
  {
    status = HTTP_STATUS_BAD_REQUEST;
    // No file to put for this absolute path
    XString message;
    message.Format(_T("HTTP PUT: No file given for: %s\n"),pathname.GetString());
    SITE_ERRORLOG(ERROR_NOT_FOUND,message);
  }
  // Remember result
  p_message->Reset();
  p_message->SetCommand(HTTPCommand::http_response);
  p_message->SetStatus(status);
  p_message->SetContentLength(0);
  
  // Ready with the put
  return true;
}

void 
SiteHandlerPut::PostHandle(HTTPMessage* p_message)
{
  if(p_message && !p_message->GetHasBeenAnswered())
  {
    // send our answer straight away
    m_site->SendResponse(p_message);
  }
}
