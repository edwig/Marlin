/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: TestCookies.cpp
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
#include "HTTPMessage.h"
#include "SiteHandlerPut.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static int totalChecks = 1;

class SiteHandlerPutCookies: public SiteHandlerPut
{
public:
  bool Handle(HTTPMessage* p_message);
};

bool
SiteHandlerPutCookies::Handle(HTTPMessage* p_msg)
{
  bool result = false;
  CString testname;
  Cookie* cookie = p_msg->GetCookie(0);
  if(!cookie)
  {
    // SUMMARY OF THE TEST
    // --- "---------------------------------------------- - ------
    qprintf("Cookie test: no cookie received                : ERROR\n");
  }
  else
  {
    CString value = cookie->GetValue();
    CString name  = cookie->GetName();

    if(name == "GUID" && value == "1-2-3-4-5-6-7-0-7-6-5-4-3-2-1")
    {
      testname = "Cookie test simple";
      result = true;
    }

    // SUMMARY OF THE TEST
    // --- "---------------------------------------------- - ------
    qprintf("Cookie received: %-27s   : %s\n",testname.GetString(),result ? "OK" : "ERROR");
    if(!result) xerror();
  }

  // Test for the second cookie
  cookie = p_msg->GetCookie(1);
  if(cookie)
  {
    CString value = cookie->GetValue();
    CString name  = cookie->GetName();

    if(name == "BEAST" && value == "Monkey")
    {
      testname = "Multiple cookie test";
      result = true;
    }

    // SUMMARY OF THE TEST
    // --- "---------------------------------------------- - ------
    qprintf("Cookie received: %-27s   : %s\n",testname.GetString(),result ? "OK" : "ERROR");
    if(!result) xerror();
  }
  else
  {
    // SUMMARY OF THE TEST
    // --- "---------------------------------------------- - ------
    qprintf("No multiple cookie received                    : ERROR\n");
  }

  CString day = p_msg->GetHeader("EdosHeader");

  p_msg->Reset();
  p_msg->ResetCookies();
  p_msg->SetCookie("session","456");
  p_msg->SetCookie("SECRET","THIS IS THE BIG SECRET OF THE FAMILY HUISMAN","BLOKZIJL",true,true);
  p_msg->SetBody("Testing");

  if(day == "15-10-1959")
  {
    // Answer for the test client
    p_msg->AddHeader("EdosHeader","Thursday");
  }

  // Check done
  --totalChecks;

  return true;
}

int 
TestCookies(HTTPServer* p_server)
{
  int error = 0;
  // If errors, change detail level
  doDetails = false;

  xprintf("TESTING HTTP RECEIVER COOKIE FUNCTIONS OF THE HTTP SERVER\n");
  xprintf("=========================================================\n");

  // Create URL channel to listen to "http://+:port/MarlinTest/CookieTest/"
  // Callback function is no longer required!
  CString webaddress = "/MarlinTest/CookieTest/";
  HTTPSite* site = p_server->CreateSite(PrefixType::URLPRE_Strong,false,TESTING_HTTP_PORT,webaddress);
  if(site)
  {
    // SUMMARY OF THE TEST
    // --- "--------------------------- - ------\n"
    qprintf("HTTPSite for cookie tests   : OK : %s\n",(LPCTSTR)site->GetPrefixURL());
  }
  else
  {
    ++error;
    xerror();
    qprintf("ERROR: Cannot register a website for : %s\n",(LPCTSTR)webaddress);
    return error;
  }

  // Setting a site handler !!
  site->SetHandler(HTTPCommand::http_put,new SiteHandlerPutCookies());

  // Modify the standard settings for this site
  site->AddContentType("","text/xml");
  site->AddContentType("xml","application/soap+xml");

  // set automatic headers on this site
  site->SetXFrameOptions(XFrameOption::XFO_DENY,"");
  site->SetStrictTransportSecurity(16070400,true);
  site->SetXContentTypeOptions(true);
  site->SetXSSProtection(true,true);
  site->SetBlockCacheControl(true);
  
  // Server should forward all headers to the messages
  site->SetAllHeaders(true);

  // new: Start the site explicitly
  if(site->StartSite())
  {
    xprintf("Site started correctly\n");
  }
  else
  {
    ++error;
    xerror();
    qprintf("ERROR STARTING SITE: %s\n",(LPCTSTR)webaddress);
  }
  return error;
}

int
AfterTestCookies()
{
  if(totalChecks > 0)
  {
    // SUMMARY OF THE TEST
    // --- "---------------------------------------------- - ------
    qprintf("Not all cookie tests received                  : ERROR\n");
  }
  return totalChecks > 0;
}