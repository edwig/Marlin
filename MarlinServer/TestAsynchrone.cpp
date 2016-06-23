/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: TestAsynchrone.cpp
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
  xprintf("Incoming ASYNC message in XML:\n%s\n",p_message->GetSoapMessage());

  // Get parameters from soap
  CString displayText = p_message->GetParameter("Text");
  bool    doReset     = p_message->GetParameterBoolean("DoReset");
  p_message->Reset();

  if(doReset)
  {
    xprintf("Async reset received\n");
    if(m_current == m_default)
    {
      // SUMMARY OF THE TEST
      // --- "--------------------------- - ------\n"
      printf("TEST ASYNC SOAP             : OK\n");
    }
    else if(m_current > 0)
    {
      // Some, but not all tests appeared before us
      xerror();
      printf("TEST ASYNC SOAP             : ERROR\n");
    }
    m_current = 0;
  }
  if(!displayText.IsEmpty())
  {
    xprintf("Async display text: %s\n",(LPCTSTR)displayText);
    ++m_current;
  }
  // Ready with the message.
  return true;
}

int
TestAsynchrone(HTTPServer* p_server)
{
  int error = 0;

  // If errors, change detail level
  doDetails = false;

  CString url("/MarlinTest/Asynchrone/");

  xprintf("TESTING ASYNCHRONE SOAP RECEIVER FUNCTIONS OF THE HTTP SERVER\n");
  xprintf("=============================================================\n");

  // Create URL channel to listen to "http://+:1200/MarlinTest/Asynchrone/"
  // Callback function is no longer required!
  HTTPSite* site = p_server->CreateSite(PrefixType::URLPRE_Strong,false,TESTING_HTTP_PORT,url);
  if(site)
  {
    // SUMMARY OF THE TEST
    // --- "--------------------------- - ------\n"
    printf("HTTPSite A-synchrone SOAP   : OK : %s\n",site->GetPrefixURL().GetString());
  }
  else
  {
    ++error;
    xerror();
    printf("ERROR: Cannot make a HTTP site for: %s\n",(LPCTSTR)url);
    return error;
  }

  // Setting the POST handler for this site
  SiteHandlerSoapAsynchrone* handler = new SiteHandlerSoapAsynchrone();
  site->SetHandler(HTTPCommand::http_post,handler);

  // Modify the standard settings for this site
  site->AddContentType("","text/xml");
  site->AddContentType("xml","application/soap+xml");

  // This is a asynchroneous test site
  site->SetAsync(true);

  // Start the site explicitly
  if(site->StartSite())
  {
    xprintf("Site started correctly: %s\n",url);
  }
  else
  {
    ++error;
    xerror();
    printf("ERROR STARTING SITE: %s\n",(LPCTSTR)url);
  }
  return error;
}
