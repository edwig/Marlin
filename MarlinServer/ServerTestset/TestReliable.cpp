/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: TestReliable.cpp
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

static int totalChecks = 6;

class SiteHandlerSoapReliable: public SiteHandlerSoap
{
  // BEWARE:
  // RM PROTOCOL IS NOW HANDLED IN THE PreHandle
  // METHOD OF THE BASE CLASS !!!
public:
  bool  Handle(SOAPMessage* p_message);
};

bool
SiteHandlerSoapReliable::Handle(SOAPMessage* p_message)
{
  // Display incoming message
  xprintf(_T("Incoming message in XML:\n%s\n"),p_message->GetSoapMessage().GetString());

  // Get parameters from soap
  XString paramOne = p_message->GetParameter(_T("One"));
  XString paramTwo = p_message->GetParameter(_T("Two"));
  xprintf(_T("Incoming parameter: %s = %s\n"),_T("One"),paramOne.GetString());
  xprintf(_T("Incoming parameter: %s = %s\n"),_T("Two"),paramTwo.GetString());

  // TestSecurity(&msg);

  // reuse message for response
  XString response = _T("TestMessageResponse");

  p_message->Reset();
  p_message->SetSoapAction(response);

  // Do our work
  bool result = false;
  if(paramOne == _T("ABC") && paramTwo == _T("1-2-3"))
  {
    paramOne  = _T("DEF");
    paramTwo = _T("123");
    result    = true;
    --totalChecks;
  }
  // SUMMARY OF THE TEST
  // --- "---------------------------------------------- - ------
  qprintf(_T("SOAP message in WS-ReliableMessaging channel   : %s\n"), result ? _T("OK") : _T("ERROR"));
  if(!result) xerror();

  p_message->SetParameter(_T("Three"),paramOne);
  p_message->SetParameter(_T("Four"), paramTwo);
  xprintf(_T("Outgoing parameter: %s = %s\n"),_T("Three"),paramOne.GetString());
  xprintf(_T("Outgoing parameter: %s = %s\n"),_T("Four"), paramTwo.GetString());

  return true;
}

int
TestMarlinServer::TestReliable()
{
  int error = 0;

  // If errors, change detail level
  m_doDetails = false;

  xprintf(_T("TESTING RELIABLE MESSAGING FUNCTIONS OF THE HTTP SERVER\n"));
  xprintf(_T("=======================================================\n"));
  
  // Create URL channel to listen to "http://+:port/MarlinTest/Reliable/"
  // But WebConfig can override all values except for the callback function address
  XString url(_T("/MarlinTest/Reliable/"));
  HTTPSite* site = m_httpServer->CreateSite(PrefixType::URLPRE_Strong,false,m_inPortNumber,url);
  if (site)
  {
    // SUMMARY OF THE TEST
    // --- "--------------------------- - ------\n"
    qprintf(_T("HTTPSite reliable messaging : OK : %s\n"),site->GetPrefixURL().GetString());
  }
  else
  {
    ++error;
    xerror();
    qprintf(_T("ERROR: Cannot register a HTTP site for: %s\n"),url.GetString());
    return error;
  }

  site->SetHandler(HTTPCommand::http_post,new SiteHandlerSoapReliable());

  // Modify the standard settings for this site
  site->AddContentType(_T(""),_T("text/xml"));
  site->AddContentType(_T("xml"),_T("application/soap+xml"));
  site->SetReliable(true);

  // new: Start the site explicitly
  if(site->StartSite())
  {
    xprintf(_T("Site started correctly\n"));
  }
  else
  {
    ++error;
    xerror();
    qprintf(_T("ERROR STARTING SITE: %s\n"),url.GetString());
  }
  return error;
}

int
TestMarlinServer::TestReliableBA()
{
  int error = 0;

  // If errors, change detail level
  m_doDetails = false;

  xprintf(_T("TESTING RELIABLE MESSAGING FUNCTIONS OF THE HTTP SERVER with BASIC AUTHENTICATION\n"));
  xprintf(_T("=================================================================================\n"));

  // Create URL channel to listen to "http://+:port/MarlinTest/ReliableBA/"
  // But WebConfig can override all values except for the callback function address
  XString url(_T("/MarlinTest/ReliableBA/"));
  HTTPSite* site = m_httpServer->CreateSite(PrefixType::URLPRE_Strong,false,m_inPortNumber,url);
  if(site)
  {
    // SUMMARY OF THE TEST
    // --- "--------------------------- - ------\n"
    qprintf(_T("HTTPSite reliable messaging : OK : %s\n"),site->GetPrefixURL().GetString());
  }
  else
  {
    ++error;
    xerror();
    qprintf(_T("ERROR: Cannot register a HTTP site for: %s\n"),url.GetString());
    return error;
  }

  SiteHandlerSoapReliable* handler = new SiteHandlerSoapReliable();
  
  // Create and set handler
  handler->SetTokenProfile(true);
  // Getting the resulting soap-security object
  SOAPSecurity* secure = handler->GetSOAPSecurity();
  secure->SetUser(_T("marlin"));
  secure->SetPassword(_T("M@rl!nS3cr3t"));

  site->SetHandler(HTTPCommand::http_post,handler);

  // Modify the standard settings for this site
  site->AddContentType(_T(""),_T("text/xml"));
  site->AddContentType(_T("xml"),_T("application/soap+xml"));
  site->SetReliable(true);

  // new: Start the site explicitly
  if(site->StartSite())
  {
    xprintf(_T("Site started correctly\n"));
  }
  else
  {
    ++error;
    xerror();
    qprintf(_T("ERROR STARTING SITE: %s\n"),url.GetString());
  }
  return error;
}

int
TestMarlinServer::AfterTestReliable()
{
  // SUMMARY OF THE TEST
  // ---- "---------------------------------------------- - ------
  qprintf(_T("WS-ReliableMessaging testing                   : %s\n"),totalChecks > 0 ? _T("ERROR") : _T("OK"));
  return totalChecks > 0;
}
