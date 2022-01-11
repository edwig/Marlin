/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: SiteHandlerSoap.cpp
//
// Marlin Server: Internet server/client
// 
// Copyright (c) 2014-2022 ir. W.E. Huisman
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
#include "SiteHandlerSoap.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// Remember SOAPMessage for this thread
__declspec(thread) SOAPMessage* g_soapMessage = nullptr;

// Destructor
SiteHandlerSoap::~SiteHandlerSoap()
{
  if(m_soapSecurity)
  {
    delete m_soapSecurity;
    m_soapSecurity = nullptr;
  }
}

// A SOAP handler is an override for the HTTP POST handler
// Most likely you need to write an overload of this one
// and provide a 'Handle' method yourself,
// as long as you leave the PreHandle here intact
// or copy the workings of the PreHandle
//
bool
SiteHandlerSoap::PreHandle(HTTPMessage* p_message)
{
  // Setting the cleanup handler. 
  // Guarantee to return to this 'Cleanup', even if we do a SEH!!
  m_site->SetCleanup(this);

  // Create a soap message for this thread
  // Remove old message in case of re-entrancy
  if(g_soapMessage)
  {
    delete g_soapMessage;
  }
  g_soapMessage = new SOAPMessage(p_message);

  // Detect XML SOAP errors
  if(g_soapMessage->GetInternalError() != XmlError::XE_NoError)
  {
    CString msg = g_soapMessage->GetInternalErrorString();
    if(g_soapMessage->GetFaultCode().IsEmpty())
    {
      g_soapMessage->Reset();
      g_soapMessage->SetFault("XML","Client","XML parsing error",msg);
      SITE_ERRORLOG(ERROR_BAD_ARGUMENTS,"Expected a SOAP message, but no valid XML message found");
    }
    m_site->SendResponse(g_soapMessage);
    p_message->SetHasBeenAnswered();
    return false;
  }

  SITE_DETAILLOGS("Received SOAP message: ",g_soapMessage->GetSoapAction());

  // Check WS-Security token profile
  if(m_soapSecurity)
  {
    if(!m_soapSecurity->CheckSecurity(g_soapMessage))
    {
      // Invalid user. Status denied message returned
      SITE_ERRORLOG(ERROR_ACCESS_DENIED,"SOAP Message with WS-Security token profile, but no valid access");
      p_message->SetStatus(HTTP_STATUS_DENIED);
      m_site->SendResponse(p_message);
      g_soapMessage->SetHasBeenAnswered();
      return false;
    }
  }

  if(m_site && (m_site->GetEncryptionLevel() != XMLEncryption::XENC_Plain))
  {
    if(m_site->HttpSecurityCheck(p_message,g_soapMessage))
    {
      // Ready for WS-Security protocol. Do not continue
      return false;
    }
  }
  // Has to cooperate with the server for the WS-RM protocol
  if(m_site && m_site->GetReliable())
  {
    if(m_site->HttpReliableCheck(g_soapMessage))
    {
      // Ready for WS-ReliableMessaging protocol. Do not continue
      return false;
    }
  }

  // IMPLEMENT YOURSELF: Write your own access mechanism.
  // Maybe by writing an override to this method and calling this one first..
  
  // returning true, to enter the default "Handle" of the (overrided) class
  return true;
}

bool
SiteHandlerSoap::Handle(HTTPMessage* p_message)
{
  if(g_soapMessage)
  {
    bool result = Handle(g_soapMessage);
    if(g_soapMessage->GetHasBeenAnswered())
    {
      p_message->SetHasBeenAnswered();
    }
    return result;
  }
  // Do the default handler (BAD REQUEST)
  return SiteHandler::Handle(p_message);
}

// Default is to do actually nothing, but to reset the message
// and send it empty handed back.
// YOU NEED TO OVERRIDE THIS METHOD!
bool
SiteHandlerSoap::Handle(SOAPMessage* p_message)
{
  // SOAPMessage created in the pre-handle stage?
  if(p_message)
  {
    p_message->Reset();
    p_message->SetFault("Critical","Server","Unimplemented","INTERNAL: Unhandled request caught by base HTTPSite::SiteHandlerSoap::Handle");
  }
  return true;
}

// Post handler sends the SOAP message back, not the HTTPMessage
void
SiteHandlerSoap::PostHandle(HTTPMessage* p_message)
{
  // Check that we did send something
  if(g_soapMessage && !g_soapMessage->GetHasBeenAnswered())
  {
    m_site->SendResponse(g_soapMessage);
    p_message->SetHasBeenAnswered();
  }
}

void
SiteHandlerSoap::CleanUp(HTTPMessage* p_message)
{
  // Cleanup the TLS handle of the soap message
  if(g_soapMessage)
  {
    // Check that we did send something
    if(!g_soapMessage->GetHasBeenAnswered() &&
       !p_message->GetHasBeenAnswered())
    {
      m_site->SendResponse(g_soapMessage);
    }
    p_message->SetHasBeenAnswered();

    // Cleanup the SOAP message
    delete g_soapMessage;
    g_soapMessage = nullptr;
    return;
  }
  // Be really sure we did send a response!
  if (!p_message->GetHasBeenAnswered())
  {
    p_message->Reset();
    p_message->SetStatus(HTTP_STATUS_BAD_REQUEST);
    p_message->SetCommand(HTTPCommand::http_response);
    m_site->SendResponse(p_message);
  }
}

//////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////

// Handle WS-Security token profile tokens
void  
SiteHandlerSoap::SetTokenProfile(bool p_token)
{
  if(p_token)
  {
    if(m_soapSecurity == nullptr)
    {
      m_soapSecurity = new SOAPSecurity();
    }
  }
  else
  {
    if(m_soapSecurity)
    {
      delete m_soapSecurity;
      m_soapSecurity = nullptr;
    }
  }
}

SOAPSecurity* 
SiteHandlerSoap::GetSOAPSecurity()
{
  return m_soapSecurity;
}
