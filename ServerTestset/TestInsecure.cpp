/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: TestInsecure.cpp
//
// Marlin Server: Internet server/client
// 
// Copyright (c) 2016 ir. W.E. Huisman
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
  xprintf("Incoming message in XML:\n%s\n",p_message->GetSoapMessage());

  // Get parameters from soap
  CString paramOne = p_message->GetParameter("One");
  CString paramTwo = p_message->GetParameter("Two");
  bool    doFault  = p_message->GetParameterBoolean("TestFault");
  xprintf("Incoming parameter: %s = %s\n","One",paramOne);
  xprintf("Incoming parameter: %s = %s\n","Two",paramTwo);

  // Test Speed-queue
  int speed = atoi(p_message->GetParameter("TestNumber"));
  if(speed == 1 && (highSpeed == 0 || highSpeed == 20))
  {
    highSpeed = 1;
  }
  else if(speed == (highSpeed + 1))
  {
    // Next message
    highSpeed = speed;
  }
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
  }
  // SUMMARY OF THE TEST
  // --- "---------------------------------------------- - ------
  qprintf("Standard SOAP message on insecure site         : %s\n", result ? "OK" : "ERROR");
  if(!result) xerror();

  if(highSpeed == 20)
  {
    // SUMMARY OF THE TEST
    // --- "---------------------------------------------- - ------
    qprintf("High speed queue has received all messages     : OK\n");
    highSpeed = 0;
  }

  if(doFault)
  {
    p_message->SetFault("FX1","Server","Testing SOAP Fault","See if the SOAP Fault does work!");
    xprintf("Outgoing parameter is a soap fault: FX1\n");
  }
  else
  {
    p_message->SetParameter("Three",paramOne);
    p_message->SetParameter("Four",paramTwo);
    xprintf("Outgoing parameter: %s = %s\n","Three",paramOne);
    xprintf("Outgoing parameter: %s = %s\n","Four",paramTwo);
  }
  --totalChecks;
  // Ready with the message.
  return true;
}

int
TestInsecure(HTTPServer* p_server)
{
  int error = 0;

  // If errors, change detail level
  doDetails = false;

  CString url("/MarlinTest/Insecure/");

  xprintf("TESTING STANDARD SOAP RECEIVER FUNCTIONS OF THE HTTP SERVER\n");
  xprintf("===========================================================\n");

  // Create URL channel to listen to "http://+:port/MarlinTest/Insecure/"
  // Callback function is no longer required!
  HTTPSite* site = p_server->CreateSite(PrefixType::URLPRE_Strong,false,TESTING_HTTP_PORT,url);
  if (site)
  {
    // SUMMARY OF THE TEST
    // --- "--------------------------- - ------\n"
    qprintf("HTTPSite for insecure SOAP  : OK : %s\n",site->GetPrefixURL().GetString());
  }
  else
  {
    ++error;
    xerror();
    qprintf("ERROR: Cannot make a HTTP site for: %s\n",(LPCTSTR)url);
    return error;
  }

  // Setting the POST handler for this site
  SiteHandlerSoapInsecure* handler = new SiteHandlerSoapInsecure();
  site->SetHandler(HTTPCommand::http_post,handler);
  site->SetHandler(HTTPCommand::http_get, handler,false);

  // Modify the standard settings for this site
  site->AddContentType("","text/xml");
  site->AddContentType("xml","application/soap+xml");

  // Start the site explicitly
  if(site->StartSite())
  {
    xprintf("Site started correctly: %s\n",url);
  }
  else
  {
    ++error;
    xerror();
    qprintf("ERROR STARTING SITE: %s\n",(LPCTSTR)url);
  }
  return error;
}

int
AfterTestInsecure()
{
  if(totalChecks > 0)
  {
    // SUMMARY OF THE TEST
    // --- "---------------------------------------------- - ------
    qprintf("Not all insecure SOAP messages received        : ERROR\n");
  }
  return totalChecks > 0;
}