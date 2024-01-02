
/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: SiteHandlerJson2Soap.cpp
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
#include "SiteHandlerJson2Soap.h"
#include "SiteHandlerSoap.h"
#include "WebServiceServer.h"
#include "HTTPSite.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// A JSON2SOAP handler is an override for the HTTP GET handler
// Most likely you will only need to set it to a HTTPSite
// that already has a SiteHandlerSoap as it's POST handler
//
bool
SiteHandlerJson2Soap::PreHandle(HTTPMessage* p_message)
{
  // Setting the cleanup handler. 
  // Guarantee to return to this 'Cleanup', even if we do a SEH!!
  m_site->SetCleanup(this);

  if(p_message->GetContentType().Find(_T("json")) > 0)
  {
    // Create an EMPTY JSON/SOAP message for this thread, forcing version 1.2
    g_soapMessage = new SOAPMessage(p_message);
    g_soapMessage->SetSoapVersion(SoapVersion::SOAP_12);

    // CONVERT url parameters to SOAPMessage
    g_soapMessage->Url2SoapParameters(p_message->GetCrackedURL());

    // IMPLEMENT YOURSELF: Write your own access mechanism.
    // Maybe by writing an override to this method and calling this one first..
  }
  // returning true, to enter the default "Handle" of the (overridden) class
  return true;
}

// Handle the request
bool     
SiteHandlerJson2Soap::Handle(HTTPMessage* p_message)
{
  if(g_soapMessage)
  {
    return Handle(g_soapMessage);
  }

  HTTPSite* site = p_message->GetHTTPSite();
  WebServiceServer* server = reinterpret_cast<WebServiceServer*>(site->GetPayload());
  if(server)
  {
    return server->ProcessGet(p_message);
  }
  // Should never come to here
  // Handle as a BAD_REQUEST
  return SiteHandler::Handle(p_message);
}

// Default is to handle the JSON GET call as a SOAP message
// Precondition: Site must have a SOAP handler as a registered 'POST' handler
bool
SiteHandlerJson2Soap::Handle(SOAPMessage* p_message)
{
  // Find the SOAP handler, and process the CONVERTED soap message
  // through the standard POST handler of the site
  SiteHandler* handler = m_site->GetSiteHandler(HTTPCommand::http_post);
  if(handler)
  {
    SiteHandlerSoap* soapHandler = reinterpret_cast<SiteHandlerSoap*>(handler);
    return soapHandler->Handle(p_message);
  }

  // Handler on a site without a SOAP POST handler
  if(p_message)
  {
    p_message->Reset();
    p_message->SetFault(_T("Critical"),_T("Server"),_T("Unimplemented")
                       ,_T("INTERNAL: Unhandled request caught by base HTTPSite::SiteHandlerJson2Soap::Handle"));
  }
  return true;
}

void 
SiteHandlerJson2Soap::PostHandle(HTTPMessage* p_message)
{
  // CONVERT SOAP Message to JSON message
  if(g_soapMessage && !g_soapMessage->GetHasBeenAnswered())
  {
    JSONMessage jsonMessage(g_soapMessage);
    if(m_site->SendResponse(&jsonMessage))
    {
      p_message    ->SetHasBeenAnswered();
      g_soapMessage->SetHasBeenAnswered();
    }
  }
}

void 
SiteHandlerJson2Soap::CleanUp(HTTPMessage* p_message)
{
  // Cleanup the TLS handle of the soap message
  if(g_soapMessage)
  {
    if(!g_soapMessage->GetHasBeenAnswered() &&
       !p_message->GetHasBeenAnswered())
    {
      JSONMessage jsonMessage(g_soapMessage);
      m_site->SendResponse(&jsonMessage);
    }
    p_message->SetHasBeenAnswered();
    // Cleanup the SOAP message
    delete g_soapMessage;
    g_soapMessage = nullptr;
    return;
  }
  // Be really sure we did send a response!
  if(!p_message->GetHasBeenAnswered())
  {
    p_message->Reset();
    p_message->SetStatus(HTTP_STATUS_BAD_REQUEST);
    p_message->SetCommand(HTTPCommand::http_response);
    m_site->SendResponse(p_message);
  }
}
