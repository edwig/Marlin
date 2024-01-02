/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: SiteHandlerGet.cpp
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
#include "SiteHandlerGet.h"
#include "HTTPMessage.h"
#include "HTTPSite.h"
#include <WinFile.h>
#include <winhttp.h>
#include <io.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

bool
SiteHandlerGet::PreHandle(HTTPMessage* /*p_message*/)
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
SiteHandlerGet::Handle(HTTPMessage* p_message)
{
  p_message->SetCommand(HTTPCommand::http_response);

  // Getting the path of the file
  WinFile ensure;
  XString resource = p_message->GetAbsoluteResource();
  // See to transformations of derived handler
  FileNameTransformations(resource);
  // Convert into a filename
  XString pathname = m_site->GetWebroot() + ensure.FileNameFromResourceName(resource);

  // Finding and setting the content type
  XString content  = m_site->GetContentTypeByResourceName(pathname);
  p_message->SetContentType(content);

  // Check existence
  if(_taccess(pathname,0) == 0)
  {
    // Check for read access
    if(_taccess(pathname,4) == 0 || FileNameRestrictions(pathname))
    {
      p_message->GetFileBuffer()->SetFileName(pathname);
      p_message->SetStatus(HTTP_STATUS_OK);
      SITE_DETAILLOGS(_T("HTTP GET: "),pathname);
    }
    else 
    {
      p_message->SetStatus(HTTP_STATUS_DENIED);
    }
  }
  else
  {
    // File does not exist, or no read access
    p_message->SetStatus(HTTP_STATUS_NOT_FOUND);
    XString text;
    text.Format(_T("HTTP GET: File not found: %s"),pathname.GetString());
    SITE_ERRORLOG(ERROR_FILE_NOT_FOUND,text);
  }
  // Ready with the get
  return true;
}

void
SiteHandlerGet::PostHandle(HTTPMessage* p_message)
{
  if(!p_message->GetHasBeenAnswered())
  {
    m_site->SendResponse(p_message);
  }
}

void
SiteHandlerGet::CleanUp(HTTPMessage* p_message)
{
  // Check we did send something
  SiteHandler::CleanUp(p_message);
  // Nothing to do for memory
}

// Diverse checks on the filename.
// Returns true if filename is altered
bool 
SiteHandlerGet::FileNameTransformations(XString & p_filename)
{
  // Almost **ALL** webservers in the world fall back to 'index.html'
  // if no resource given for a site or a subsite
  if(p_filename.Right(1) == '/')
  {
    p_filename += BASE_INDEX_PAGE;
    return true;
  }
  return FileNameRestrictions(p_filename);
}

// Default restrictions on the filename. Does nothing here!
// You can implement your own override,
// e.g. for implementing restrictions on subdirectories
bool 
SiteHandlerGet::FileNameRestrictions(XString& /*p_filename*/)
{
  // Nothing done here!
  return false;
}
