/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: TestCookies.cpp
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
#include "HTTPMessage.h"
#include "SiteHandlerPut.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static int totalChecks = 2;

class SiteHandlerPutCookies: public SiteHandlerPut
{
public:
  bool Handle(HTTPMessage* p_message);
};

bool
SiteHandlerPutCookies::Handle(HTTPMessage* p_msg)
{
  bool result = false;
  XString testname;
  Cookie* cookie = p_msg->GetCookie(0);
  if(!cookie)
  {
    // SUMMARY OF THE TEST
    // --- "---------------------------------------------- - ------
    qprintf(_T("Cookie test: no cookie received                : ERROR\n"));
    xerror();
  }
  else
  {
    XString value = cookie->GetValue();
    XString name  = cookie->GetName();

    if(name == _T("GUID") && value == _T("1-2-3-4-5-6-7-0-7-6-5-4-3-2-1"))
    {
      testname = _T("Cookie test simple");
      result = true;
      --totalChecks;
    }
    else
    {
      xerror();
    }

    // SUMMARY OF THE TEST
    // --- "---------------------------------------------- - ------
    qprintf(_T("Cookie received: %-27s   : %s\n"),testname.GetString(),result ? _T("OK") : _T("ERROR"));
  }

  // Test for the second cookie
  cookie = p_msg->GetCookie(1);
  if(cookie)
  {
    XString value = cookie->GetValue();
    XString name  = cookie->GetName();

    if(name == _T("BEAST") && value == _T("Monkey"))
    {
      testname = _T("Multiple cookie test");
      result = true;
      --totalChecks;
    }
    else
    {
      xerror();
    }
    // SUMMARY OF THE TEST
    // --- "---------------------------------------------- - ------
    qprintf(_T("Cookie received: %-27s   : %s\n"),testname.GetString(),result ? _T("OK") : _T("ERROR"));
  }
  else
  {
    // SUMMARY OF THE TEST
    // --- "---------------------------------------------- - ------
    qprintf(_T("No multiple cookie received                    : ERROR\n"));
    xerror();
  }

  XString day = p_msg->GetHeader(_T("EdosHeader"));

  p_msg->Reset();
  p_msg->ResetCookies();
  p_msg->SetCookie(_T("session"),_T("456"));
  p_msg->SetCookie(_T("SECRET"),_T("THIS IS THE BIG SECRET OF THE FAMILY HUISMAN"),_T("BLOKZIJL"),true,true);
  p_msg->SetBody(_T("Testing"));

  if(day == _T("15-10-1959"))
  {
    // Answer for the test client
    p_msg->AddHeader(_T("EdosHeader"),_T("Thursday"));
  }
  return true;
}

int 
TestMarlinServer::TestCookies()
{
  int error = 0;
  // If errors, change detail level
  m_doDetails = false;

  xprintf(_T("TESTING HTTP RECEIVER COOKIE FUNCTIONS OF THE HTTP SERVER\n"));
  xprintf(_T("=========================================================\n"));

  // Create URL channel to listen to "http://+:port/MarlinTest/CookieTest/"
  // Callback function is no longer required!
  XString webaddress = _T("/MarlinTest/CookieTest/");
  HTTPSite* site = m_httpServer->CreateSite(PrefixType::URLPRE_Strong,false,m_inPortNumber,webaddress);
  if(site)
  {
    // SUMMARY OF THE TEST
    // --- "--------------------------- - ------\n"
    qprintf(_T("HTTPSite for cookie tests   : OK : %s\n"),site->GetPrefixURL().GetString());
  }
  else
  {
    ++error;
    xerror();
    qprintf(_T("ERROR: Cannot register a website for : %s\n"),webaddress.GetString());
    return error;
  }

  // Setting a site handler !!
  site->SetHandler(HTTPCommand::http_put,new SiteHandlerPutCookies());

  // Modify the standard settings for this site
  site->AddContentType(_T(""),_T("text/xml"));
  site->AddContentType(_T("xml"),_T("application/soap+xml"));

  // set automatic headers on this site
  site->SetXFrameOptions(XFrameOption::XFO_DENY,_T(""));
  site->SetStrictTransportSecurity(16070400,true);
  site->SetXContentTypeOptions(true);
  site->SetXSSProtection(true,true);
  site->SetBlockCacheControl(true);
  
  // new: Start the site explicitly
  if(site->StartSite())
  {
    xprintf(_T("Site started correctly\n"));
  }
  else
  {
    ++error;
    xerror();
    qprintf(_T("ERROR STARTING SITE: %s\n"),webaddress.GetString());
  }
  return error;
}

int
TestMarlinServer::AfterTestCookies()
{
  // SUMMARY OF THE TEST
  // ---- "---------------------------------------------- - ------
  qprintf(_T("Cookies & multiple-cookies test                : %s\n"), totalChecks > 0 ? _T("ERROR") : _T("OK"));
  return totalChecks > 0;
}