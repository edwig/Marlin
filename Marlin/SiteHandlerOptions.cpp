/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: SiteHandlerOptions.cpp
//
// Marlin Server: Internet server/client
// 
// Copyright (c) 2015-2018 ir. W.E. Huisman
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
  if(p_message->GetAbsoluteResource() == "*")
  {
    // Request is for the total server HTTPSite
    server = true;
  }

  // The OPTIONS call is where we do the Pre-flight checks of CORS
  if(m_site->GetUseCORS())
  {
    // Read the message
    CString comesFrom = p_message->GetHeader("Origin");
    CString reqMethod = p_message->GetHeader("Access-Control-Request-Method");
    CString reqHeader = p_message->GetHeader("Access-Control-Request-Headers");
    p_message->Reset();

    CheckCrossOriginSettings(p_message,comesFrom,reqMethod,reqHeader);
  }
  else
  {
    // See RFC 2616 section 9.9 OPTIONS
    p_message->Reset();
  }

  // Empty the response part. Just to be sure!
  CString empty;
  p_message->GetFileBuffer()->Reset();
  p_message->SetFile(empty);
  // Set the allow header for this site
  if(server)
  {
    p_message->AddHeader("Allow",m_site->GetAllowHandlers());
  }

  // Ready with the options, continue processing
  return true;
}

void
SiteHandlerOptions::PostHandle(HTTPMessage* p_message)
{
  if(p_message && p_message->GetRequestHandle())
  {
    p_message->SetCommand(HTTPCommand::http_response);
    m_site->SendResponse(p_message);
    SITE_DETAILLOGS("Answered a OPTIONS message from: ",SocketToServer(p_message->GetSender()));
  }
}

//////////////////////////////////////////////////////////////////////////
//
// PRIVATE
// CORS checking following: 
// https://developer.mozilla.org/en-US/docs/Web/HTTP/Access_control_CORS
//
//////////////////////////////////////////////////////////////////////////

void
SiteHandlerOptions::CheckCrossOriginSettings(HTTPMessage* p_message
                                            ,CString      p_origin
                                            ,CString      p_method
                                            ,CString      p_headers)
{
  // Check all requested header methods
  if(!CheckCORSOrigin (p_message,p_origin))  return;
  if(!CheckCORSMethod (p_message,p_method))  return;
  if(!CheckCORSHeaders(p_message,p_headers)) return;

  // Adding the max age if any
  if(m_site->GetCORSMaxAge() > 0)
  {
    CString maxAge;
    maxAge.Format("%d",m_site->GetCORSMaxAge());
    p_message->AddHeader("Access-Control-Max-Age",maxAge,false);
  }
}

// Check that we have same origin
bool 
SiteHandlerOptions::CheckCORSOrigin(HTTPMessage* p_message,CString p_origin)
{
  CString origin = m_site->GetCORSOrigin();
  if(!origin.IsEmpty())
  {
    if(p_origin.CompareNoCase(origin))
    {
      // This call is NOT allowed
      p_message->SetStatus(HTTP_STATUS_DENIED);
      SITE_ERRORLOG(ERROR_ACCESS_DENIED,"Call refused: Not same CORS origin");
      return false;
    }
  }
  // All sites are allowed, or only the origin
  // Header "Access-Control-Allow-Origin" will be set by the site!

  return true;
}

// Check that the site has this HTTP Method
bool 
SiteHandlerOptions::CheckCORSMethod(HTTPMessage* p_message,CString p_method)
{
  if(!p_method.IsEmpty())
  {
    CString handlers = m_site->GetAllowHandlers();
    if(handlers.Find(p_method) < 0)
    {
      // This method is NOT allowed
      p_message->SetStatus(HTTP_STATUS_DENIED);
      SITE_ERRORLOG(ERROR_ACCESS_DENIED,"Call refused: CORS method not allowed");
      return false;
    }
    // These are the allowed methods for this site
    p_message->AddHeader("Access-Control-Allow-Methods",handlers,false);
  }
  return true;
}

// Check that we can handle these headers
bool 
SiteHandlerOptions::CheckCORSHeaders(HTTPMessage* p_message,CString p_headers)
{
  if(!p_headers.IsEmpty())
  {
    // CURRENTLY THERE IS NO MECHANISME TO CHECK THE ALLOWED HEADERS
    // SO WE JUST REFLECT WHAT WE'VE GOTTEN
    // Note 1: To implement all handlers must register there headers
    // Note 2: Checking on 'official' HTTP headers defies the protocol!
    p_message->AddHeader("Access-Control-Allow-Headers",p_headers,false);
  }
  return true;
}
