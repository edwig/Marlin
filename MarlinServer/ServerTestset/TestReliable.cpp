/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: TestReliable.cpp
//
// Marlin Server: Internet server/client
// 
// Copyright (c) 2017 ir. W.E. Huisman
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
#include "TestServer.h"
#include "HTTPSite.h"
#include "SiteHandlerSoap.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static int totalChecks = 3;

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
  xprintf("Incoming message in XML:\n%s\n",p_message->GetSoapMessage().GetString());

  // Get parameters from soap
  CString paramOne = p_message->GetParameter("One");
  CString paramTwo = p_message->GetParameter("Two");
  xprintf("Incoming parameter: %s = %s\n","One",paramOne.GetString());
  xprintf("Incoming parameter: %s = %s\n","Two",paramTwo.GetString());

  // TestSecurity(&msg);

  // reuse message for response
  CString response = "TestMessageResponse";

  p_message->Reset();
  p_message->SetSoapAction(response);

  // Do our work
  bool result = false;
  if(paramOne == "ABC" && paramTwo == "1-2-3")
  {
    paramOne  = "DEF";
    paramTwo = "123";
    result    = true;
    --totalChecks;
  }
  // SUMMARY OF THE TEST
  // --- "---------------------------------------------- - ------
  qprintf("SOAP message in WS-ReliableMessaging channel   : %s\n", result ? "OK" : "ERROR");
  if(!result) xerror();

  p_message->SetParameter("Three",paramOne);
  p_message->SetParameter("Four", paramTwo);
  xprintf("Outgoing parameter: %s = %s\n","Three",paramOne.GetString());
  xprintf("Outgoing parameter: %s = %s\n","Four", paramTwo.GetString());

  return true;
}

int
TestReliable(HTTPServer* p_server)
{
  int error = 0;

  // If errors, change detail level
  doDetails = false;

  xprintf("TESTING RELIABLE MESSAGING FUNCTIONS OF THE HTTP SERVER\n");
  xprintf("=======================================================\n");
  
  // Create URL channel to listen to "http://+:port/MarlinTest/Reliable/"
  // But WebConfig can override all values except for the callback function address
  CString url("/MarlinTest/Reliable/");
  HTTPSite* site = p_server->CreateSite(PrefixType::URLPRE_Strong,false,TESTING_HTTP_PORT,url);
  if (site)
  {
    // SUMMARY OF THE TEST
    // --- "--------------------------- - ------\n"
    qprintf("HTTPSite reliable messaging : OK : %s\n",(LPCTSTR) site->GetPrefixURL());
  }
  else
  {
    ++error;
    xerror();
    qprintf("ERROR: Cannot register a HTTP site for: %s\n",(LPCTSTR) url);
    return error;
  }

  site->SetHandler(HTTPCommand::http_post,new SiteHandlerSoapReliable());

  // Modify the standard settings for this site
  site->AddContentType("","text/xml");
  site->AddContentType("xml","application/soap+xml");
  site->SetReliable(true);

  // new: Start the site explicitly
  if(site->StartSite())
  {
    xprintf("Site started correctly\n");
  }
  else
  {
    ++error;
    xerror();
    qprintf("ERROR STARTING SITE: %s\n",(LPCTSTR) url);
  }
  return error;
}

int
AfterTestReliable()
{
  // SUMMARY OF THE TEST
  // ---- "---------------------------------------------- - ------
  qprintf("WS-ReliableMessaging testing                   : %s\n",totalChecks > 0 ? "ERROR" : "OK");
  return totalChecks > 0;
}
