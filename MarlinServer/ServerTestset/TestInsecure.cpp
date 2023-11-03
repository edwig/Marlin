/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: TestInsecure.cpp
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
#include "TestMarlinServer.h"
#include "HTTPSite.h"
#include "SiteHandlerSoap.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static int highSpeed   = 0;
static int totalChecks = 23;

class SiteHandlerSoapInsecure: public SiteHandlerSoap
{
public:
  bool  Handle(SOAPMessage* p_message);
};

// Handle the soap message.
// It's already in the m_soapMessage member
bool
SiteHandlerSoapInsecure::Handle(SOAPMessage* p_message)
{
  // Display incoming message
  xprintf(_T("Incoming message in XML:\n%s\n"),p_message->GetSoapMessage().GetString());

  // Get parameters from soap
  XString paramOne = p_message->GetParameter(_T("One"));
  XString paramTwo = p_message->GetParameter(_T("Two"));
  bool    doFault  = p_message->GetParameterBoolean(_T("TestFault"));
  xprintf(_T("Incoming parameter: %s = %s\n"),_T("One"),paramOne.GetString());
  xprintf(_T("Incoming parameter: %s = %s\n"),_T("Two"),paramTwo.GetString());

  // Test Speed-queue
  int speed = _ttoi(p_message->GetParameter(_T("TestNumber")));
  if(speed == 1 && (highSpeed == 0 || highSpeed == 20))
  {
    highSpeed = 1;
  }
  else if(speed > highSpeed)
  {
    // Next message
    highSpeed = speed;
  }
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
  }
  // SUMMARY OF THE TEST
  // --- "---------------------------------------------- - ------
  qprintf(_T("Standard SOAP message on insecure site         : %s\n"), result ? _T("OK") : _T("ERROR"));
  if(!result) xerror();

  if(highSpeed == 20)
  {
    // SUMMARY OF THE TEST
    // --- "---------------------------------------------- - ------
    qprintf(_T("High speed queue has received all messages     : OK\n"));
  }

  if(doFault)
  {
    p_message->SetFault(_T("FX1"),_T("Server"),_T("Testing SOAP Fault"),_T("See if the SOAP Fault does work!"));
    xprintf(_T("Outgoing parameter is a soap fault: FX1\n"));
  }
  else
  {
    p_message->SetParameter(_T("Three"),paramOne);
    p_message->SetParameter(_T("Four"),paramTwo);
    xprintf(_T("Outgoing parameter: %s = %s\n"),_T("Three"),paramOne.GetString());
    xprintf(_T("Outgoing parameter: %s = %s\n"),_T("Four"),paramTwo.GetString());
  }
  --totalChecks;
  // Ready with the message.
  return true;
}

int
TestMarlinServer::TestInsecure()
{
  int error = 0;

  // If errors, change detail level
  m_doDetails = false;

  XString url(_T("/MarlinTest/Insecure/"));

  xprintf(_T("TESTING STANDARD SOAP RECEIVER FUNCTIONS OF THE HTTP SERVER\n"));
  xprintf(_T("===========================================================\n"));

  // Create URL channel to listen to "http://+:port/MarlinTest/Insecure/"
  // Callback function is no longer required!
  HTTPSite* site = m_httpServer->CreateSite(PrefixType::URLPRE_Strong,false,m_inPortNumber,url);
  if (site)
  {
    // SUMMARY OF THE TEST
    // --- "--------------------------- - ------\n"
    qprintf(_T("HTTPSite for insecure SOAP  : OK : %s\n"),site->GetPrefixURL().GetString());
  }
  else
  {
    ++error;
    xerror();
    qprintf(_T("ERROR: Cannot make a HTTP site for: %s\n"),url.GetString());
    return error;
  }

  // Setting the POST handler for this site
  SiteHandlerSoapInsecure* handler = new SiteHandlerSoapInsecure();
  site->SetHandler(HTTPCommand::http_post,handler);
  site->SetHandler(HTTPCommand::http_get, handler,false);

  // Modify the standard settings for this site
  site->AddContentType(_T(""),_T("text/xml"));
  site->AddContentType(_T("xml"),_T("application/soap+xml"));

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

int
TestMarlinServer::AfterTestInsecure()
{
  // SUMMARY OF THE TEST
  // ---- "---------------------------------------------- - ------
  qprintf(_T("SOAP messages to insecure site                 : %s\n"),totalChecks > 0 ? _T("ERROR") : _T("OK"));
  qprintf(_T("All highspeed queue messages received          : %s\n"),(highSpeed % 20) == 0 ? _T("OK") : _T("ERROR"));
  return totalChecks > 0;
}