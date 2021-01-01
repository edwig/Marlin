/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: SiteHandlerOptions.cpp
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

    if(CheckCrossOriginSettings(p_message,comesFrom,reqMethod,reqHeader) == false)
    {
      p_message->SetCommand(HTTPCommand::http_response);
      m_site->SendResponse(p_message);
      return false;
    }
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

bool
SiteHandlerOptions::CheckCrossOriginSettings(HTTPMessage* p_message
                                            ,CString      p_origin
                                            ,CString      p_method
                                            ,CString      p_headers)
{
  // Check all requested header methods
  if(!CheckCORSOrigin (p_message,p_origin))  return false;
  if(!CheckCORSMethod (p_message,p_method))  return false;
  if(!CheckCORSHeaders(p_message,p_headers)) return false;

  // Adding the max age if any
  if(m_site->GetCORSMaxAge() > 0)
  {
    CString maxAge;
    maxAge.Format("%d",m_site->GetCORSMaxAge());
    p_message->AddHeader("Access-Control-Max-Age",maxAge,false);
  }

  if(m_site->GetCORSAllowCredentials())
  {
    p_message->AddHeader("Access-Control-Allow-Credentials","true",false);
  }
  return true;
}

// Check that we have same origin
bool 
SiteHandlerOptions::CheckCORSOrigin(HTTPMessage* p_message,CString p_origin)
{
  CString origin = m_site->GetCORSOrigin();
  p_origin.MakeLower();
  origin.MakeLower();

  // If all sites are allowed, just reflect the requested origin
  if(origin == "*")
  {
    return true;
  }
  else if(origin.Find(p_origin) == 0)
  {
    return true;
  }
  // This call is NOT allowed
  p_message->SetStatus(HTTP_STATUS_DENIED);
  CString error;
  error.Format("Call refused: Not same CORS origin. Site allows [%s] but call was from: %s",origin.GetString(),p_origin.GetString());
  SITE_ERRORLOG(ERROR_ACCESS_DENIED,error);
  return false;
}

// Check that the site has this HTTP Method
bool 
SiteHandlerOptions::CheckCORSMethod(HTTPMessage* p_message,CString p_method)
{
  CString handlers = m_site->GetAllowHandlers();
  if(!p_method.IsEmpty())
  {
    if(handlers.Find(p_method) < 0)
    {
      // This method is NOT allowed
      p_message->SetStatus(HTTP_STATUS_DENIED);
      CString error;
      error.Format("Call refused: CORS method [%s] not implemented by the server.",p_method.GetString());
      SITE_ERRORLOG(ERROR_ACCESS_DENIED,error);
      return false;
    }
  }
  // These are the allowed methods for this site
  p_message->AddHeader("Access-Control-Allow-Methods",handlers,false);
  return true;
}

// Check that we can handle these headers
bool 
SiteHandlerOptions::CheckCORSHeaders(HTTPMessage* p_message,CString p_headers)
{
  // Make requested and allowed headers lower case to compare them
  CString allowed = m_site->GetCORSHeaders();
  allowed.MakeLower();
  p_headers.MakeLower();

  // Split all headers
  std::vector<CString> map;
  SplitHeaders(p_headers,map);

  // See if headers where requested
  if(p_headers.IsEmpty())
  {
    return true;
  }

  // Check all headers
  for(auto& header : map)
  {
    if(!header.IsEmpty() && allowed.Find(header) < 0)
    {
      p_message->SetStatus(HTTP_STATUS_DENIED);
      CString error;
      error.Format("Call refused: header [%s] not allowed by the server",p_headers.GetString());
      SITE_ERRORLOG(ERROR_ACCESS_DENIED,error);
      return false;
    }
  }
  // These are the allowed methods for this site
  p_message->AddHeader("Access-Control-Allow-Headers",m_site->GetCORSHeaders(),false);
  return true;
}

void 
SiteHandlerOptions::SplitHeaders(CString p_headers,std::vector<CString>& p_map)
{
  while(!p_headers.IsEmpty())
  {
    int pos = p_headers.Find(',');
    if(pos > 0)
    {
      CString header = p_headers.Left(pos);
      p_headers = p_headers.Mid(pos + 1);
      header.Trim();
      if(!header.IsEmpty())
      {
        p_map.push_back(header);
      }
    }
    else
    {
      p_headers.Trim();
      if(!p_headers.IsEmpty())
      {
        p_map.push_back(p_headers);
      }
      return;
    }
  }
}
