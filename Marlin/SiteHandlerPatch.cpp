/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: SiteHandlerPatch.cpp
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
#include "SiteHandlerPatch.h"
#include "HTTPMessage.h"
#include "HTTPSite.h"
#include "HTTPServer.h"
#include <winhttp.h>
#include <io.h>

#ifdef _AFX
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#endif

bool
SiteHandlerPatch::PreHandle(HTTPMessage* /*p_message*/)
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
//
// In this example we prepend the given source to the http resource URL
bool
SiteHandlerPatch::Handle(HTTPMessage* p_message)
{
  // Getting the path of the file
  XString pathname = m_site->GetWebroot() + p_message->GetAbsoluteResource();

  p_message->Reset();
  p_message->SetStatus(HTTP_STATUS_DENIED);

  // Check existence
  if(_taccess(pathname,0) == 0)
  {
    DWORD error = 0;
    // Check for write access
    if(_taccess(pathname,2) == 0)
    {
      // Save file in temporary buffer
      FileBuffer tmpBuffer;
      tmpBuffer.SetFileName(pathname);
      if(tmpBuffer.ReadFile())
      {
        // Write the file in the HTTP message first
        if(p_message->GetFileBuffer()->WriteFile())
        {
          // Now append the original file
          FILE* file = nullptr;
          _tfopen_s(&file,pathname,_T("a"));
          if(file)
          {
            uchar* buffer = nullptr;
            size_t length = 0;
            tmpBuffer.GetBuffer(buffer,length);
            unsigned char* buf = static_cast<unsigned char*>(buffer);
            for(size_t index = 0; index < length; ++index,++buf)
            {
              if(fputc(*buf,file) == EOF)
              {
                error = GetLastError();
                break;
              }
            }
            if(fclose(file))
            {
              error = GetLastError();
            }
            if(error == 0)
            {
              p_message->SetStatus(HTTP_STATUS_OK);
              SITE_DETAILLOGS(_T("HTTP MERGED: "),pathname);
            }
            else
            {
              p_message->SetStatus(HTTP_STATUS_SERVER_ERROR);
            }
          }
          else
          {
            // Not opened for append
            p_message->SetStatus(HTTP_STATUS_SERVER_ERROR);
            error = GetLastError();
          }
        }
        else
        {
          p_message->SetStatus(HTTP_STATUS_DENIED);
          error = GetLastError();
        }
      }
      else
      {
        // Not read
        p_message->SetStatus(HTTP_STATUS_SERVER_ERROR);
        error = GetLastError();
      }
    }
    else
    {
      // Already HTTP_STATUS_DENIED
      error = ERROR_ACCESS_DENIED;
    }
    // else already HTTP_STATUS_DENIED
    if(error)
    {
      // Server error
      XString text;
      text.Format(_T("FAILED: HTTP PATCH: %s"),pathname.GetString());
      SITE_ERRORLOG(error,text);
    }
  }
  else
  {
    // File does not exist, or no read access
    p_message->SetStatus(HTTP_STATUS_NOT_FOUND);
    XString text;
    text.Format(_T("HTTP PATCH: File not found: %s\n"),pathname.GetString());
    SITE_ERRORLOG(ERROR_FILE_NOT_FOUND,text);
  }
  // Ready with the put
  return true;
}

void
SiteHandlerPatch::PostHandle(HTTPMessage* p_message)
{
  if(p_message && !p_message->GetHasBeenAnswered())
  {
    // send our answer straight away, deleting the msg object
    p_message->SetCommand(HTTPCommand::http_response);
    m_site->SendResponse(p_message);
  }
}

void
SiteHandlerPatch::CleanUp(HTTPMessage* p_message)
{
  // Check we did send something
  SiteHandler::CleanUp(p_message);
  // Nothing to do for memory cleanup
}

