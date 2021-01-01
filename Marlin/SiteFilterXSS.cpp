/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: SiteFilterXSS.cpp
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
#include "SiteFilterXSS.h"
#include "SiteHandler.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

SiteFilterXSS::SiteFilterXSS(unsigned p_priority,CString p_name)
              :SiteFilter(p_priority,p_name)
{
}

SiteFilterXSS::~SiteFilterXSS()
{
}

// Override from SiteFilter::Handle
bool
SiteFilterXSS::Handle(HTTPMessage* p_message)
{
  CString referrer = p_message->GetReferrer();
  CString fullURL  = p_message->GetURL();
  
  // Unique call from the outside world is OK
  if(referrer.IsEmpty())
  {
    return true;
  }

  // Make comparable
  fullURL.MakeLower();
  referrer.MakeLower();

  // See to it that referrer is part of the current URL
  if(fullURL.Find(referrer) < 0)
  {
    int status = HTTP_STATUS_FORBIDDEN;
    // Tell the log that we are very sorry
    SITE_ERRORLOG(status,"Detected Cross-Site-Scripting from: " + referrer);

    // Bounce the HTTPMessage immediately as a 403: Status forbidden
    p_message->Reset();
    p_message->SetStatus(status);
    m_site->SendResponse(p_message);

    // Do NOT process this message again
    return false;
  }
  return true;
}

