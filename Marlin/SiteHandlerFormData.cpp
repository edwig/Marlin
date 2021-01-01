/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: SiteHandlerFormData.cpp
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
#include "SiteHandlerFormData.h"
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
SiteHandlerFormData::Handle(HTTPMessage* p_message)
{
  int errors = 0;
  CString message;

  CString contentType = p_message->GetContentType();
  FileBuffer* buffer  = p_message->GetFileBuffer();
  MultiPartBuffer multi(FD_UNKNOWN);

  if(buffer && !contentType.IsEmpty())
  {
    // Getting all parts from the HTTPMessage
    if(multi.ParseBuffer(contentType,buffer))
    {
      // Clear the message for an answer
      p_message->Reset();
      p_message->SetStatus(HTTP_STATUS_OK);

      // Do Pre-handling first
      PreHandleBuffer(p_message,&multi);

      // Cycle through all the parts
      size_t parts = multi.GetParts();
      for(size_t ind = 0; ind < parts; ++ind)
      {
        MultiPart* part = multi.GetPart((int)ind);
        if(part)
        {
          if(part->GetFileName().IsEmpty())
          {
            errors += HandleData(p_message,part);
          }
          else
          {
            errors += HandleFile(p_message,part);
          }
        }
        else
        {
          ++errors;
          SITE_ERRORLOG(ERROR_NO_DATA,"Internal error in form-data MultiPartBuffer");
        }
      }
      // Now ready with all the parts. Do the post-handling
      PostHandleBuffer(p_message,&multi);
    }
    else
    {
      ++errors;
      SITE_ERRORLOG(ERROR_NO_DATA,"Multi-part form-data not parsed. Internal error in MultiPartBuffer");
    }
  }
  else
  {
    ++errors;
    SITE_ERRORLOG(ERROR_NO_DATA,"NO legal multi-part buffer, or no multi-part content-type");
  }

  if(errors)
  {
    // Setting HTTP status "409 Resource Conflict"
    p_message->Reset();
    p_message->SetStatus(HTTP_STATUS_CONFLICT);
    return false;
  }
  return true;
}

// Does only logging.
// CAN BE OVERRIDDEN
int
SiteHandlerFormData::PreHandleBuffer(HTTPMessage* /*p_message*/,MultiPartBuffer* p_buffer)
{
  SITE_DETAILLOGV("Pre-handling form-data. Number of bufferparts: %d",p_buffer->GetParts());
  return 0;
}

// DOES NOTHING: OVVERRIDE ME!
// IMPLEMENT YOURSELF: YOUR IMPLEMENTATION HERE
int
SiteHandlerFormData::HandleData(HTTPMessage* /*p_message*/,MultiPart* p_part)
{
  SITE_DETAILLOGS("Handling form-data data-part: ",p_part->GetName());
  SITE_ERRORLOG(ERROR_BAD_COMMAND,"Default multipart/form-data data-handler. Override me!");
  return 1;
}

// DOES NOTHING: OVVERRIDE ME!
// IMPLEMENT YOURSELF: YOUR IMPLEMENTATION HERE
int
SiteHandlerFormData::HandleFile(HTTPMessage* /*p_message*/,MultiPart* p_part)
{
  SITE_DETAILLOGV("Handling form-data file-part: [%s] %s",p_part->GetName().GetString(),p_part->GetFileName().GetString());
  SITE_ERRORLOG(ERROR_BAD_COMMAND,"Default multipart/form-data file-handler. Override me!");
  return 1;
}

// Does only logging.
// CAN BE OVERRIDDEN
int
SiteHandlerFormData::PostHandleBuffer(HTTPMessage* /*p_message*/,MultiPartBuffer* p_buffer)
{
  SITE_DETAILLOGV("Post-handling form-data. Number of bufferparts: %d",p_buffer->GetParts());
  return 0;
}
