/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: TestAsynchrone.cpp
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
#include "TestMarlinServer.h"
#include "HTTPSite.h"
#include "SiteHandlerSoap.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static int highSpeed = 0;

class SiteHandlerSoapAsynchrone : public SiteHandlerSoap
{
public:
  bool  Handle(SOAPMessage* p_message);
private:
  int   m_default { 10 };  // 10 messages per test
  int   m_current {  0 };  // Currently received tests
};

// Handle the soap message.
// Expected is:
// "MarlinReset" with <DoReset>true</DoReset>
// 10 x a "MarlinText" message with a <Text>Some Text</Text> node
// "MarlinReset" with <DoReset>true</DoReset>
// 12 messages in total
bool
SiteHandlerSoapAsynchrone::Handle(SOAPMessage* p_message)
{
  // Display incoming message
  xprintf(_T("Incoming ASYNC message in XML:\n%s\n"),p_message->GetSoapMessage().GetString());

  // Get parameters from soap
  XString displayText = p_message->GetParameter(_T("Text"));
  bool    doReset     = p_message->GetParameterBoolean(_T("DoReset"));
  p_message->Reset();

  if(doReset)
  {
    xprintf(_T("Async reset received\n"));
    if(m_current == m_default)
    {
      // SUMMARY OF THE TEST
      // --- "---------------------------------------------- - ------
      qprintf(_T("Received all of the asynchronous SOAP messages : OK\n"));
      highSpeed = 1;
    }
    else if(m_current > 0)
    {
      // Some, but not all tests appeared before us
      xerror();
      // --- "---------------------------------------------- - ------
      qprintf(_T("Not all asynchronous SOAP messages received    : ERROR\n"));
    }
    m_current = 0;
  }
  if(!displayText.IsEmpty())
  {
    xprintf(_T("Async display text: %s\n"),displayText.GetString());
    ++m_current;
    // SUMMARY OF THE TEST
    // --- "---------------------------------------------- - ------
    qprintf(_T("Received an asynchronous SOAP message [%2.2d]     : OK\n"),m_current);
  }
  // Ready with the message.
  return true;
}

int
TestMarlinServer::TestAsynchrone()
{
  int error = 0;

  // If errors, change detail level
  m_doDetails = false;

  XString url(_T("/MarlinTest/Asynchrone/"));

  xprintf(_T("TESTING ASYNCHRONE SOAP RECEIVER FUNCTIONS OF THE HTTP SERVER\n"));
  xprintf(_T("=============================================================\n"));

  // Create URL channel to listen to "http://+:port/MarlinTest/Asynchrone/"
  // Callback function is no longer required!
  HTTPSite* site = m_httpServer->CreateSite(PrefixType::URLPRE_Strong,false,m_inPortNumber,url);
  if(site)
  {
    // SUMMARY OF THE TEST
    // --- "--------------------------- - ------\n"
    qprintf(_T("HTTPSite A-synchrone SOAP   : OK : %s\n"),site->GetPrefixURL().GetString());
  }
  else
  {
    ++error;
    xerror();
    qprintf(_T("ERROR: Cannot make a HTTP site for: %s\n"),url.GetString());
    return error;
  }

  // Setting the POST handler for this site
  SiteHandlerSoapAsynchrone* handler = new SiteHandlerSoapAsynchrone();
  site->SetHandler(HTTPCommand::http_post,handler);

  // Modify the standard settings for this site
  site->AddContentType(true,_T("pos"),_T("text/xml"));
  site->AddContentType(true,_T("xml"),_T("application/soap+xml"));

  // This is a asynchronous test site
  site->SetAsync(true);

  // Start the site explicitly
  if(site->StartSite())
  {
    xprintf(_T("Site started correctly: %s\n"),url.GetString());
  }
  else
  {
    ++error;
    xerror();
    qprintf(_T("ERROR STARTING SITE: %s\n"),url.GetString());
  }
  return error;
}

// Completeness check already on receive of last message!
int 
TestMarlinServer::AfterTestAsynchrone()
{
  // ---- "---------------------------------------------- - ------
  qprintf(_T("Received all of the asynchronous SOAP messages : %s\n"), highSpeed == 1 ? _T("OK") : _T("ERROR"));
  return highSpeed == 1 ? 0 : 1;
}
