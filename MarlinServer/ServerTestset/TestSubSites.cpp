/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: TestSubSites.cpp
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

static int totalChecks = 2;

class SiteHandlerSoapSubsite: public SiteHandlerSoap
{
protected:
  bool Handle(SOAPMessage* p_message);
};

bool
SiteHandlerSoapSubsite::Handle(SOAPMessage* p_message)
{
  // Get parameters from soap
  CString paramOne = p_message->GetParameter("One");
  CString paramTwo = p_message->GetParameter("Two");
  xprintf("Incoming parameter: %s = %s\n","One",paramOne);
  xprintf("Incoming parameter: %s = %s\n","Two",paramTwo);

  // Forget
  p_message->Reset();

  // Do our work
  bool result = false;
  if(paramOne == "ABC" && paramTwo == "1-2-3")
  {
    paramOne = "DEF";
    paramTwo = "123";
    result = true;
    --totalChecks;
  }
  // SUMMARY OF THE TEST
  // --- "---------------------------------------------- - ------
  qprintf("Subsite SOAP handler on main site              : %s\n",result ? "OK" : "ERROR");
  xprintf("Site of this message was    : %s\n",(LPCTSTR)p_message->GetHTTPSite()->GetSite());
  if(!result) xerror();

  // Set the result
  p_message->SetParameter("Three",paramOne);
  p_message->SetParameter("Four", paramTwo);
  xprintf("Outgoing parameter: %s = %s\n","Three",paramOne);
  xprintf("Outgoing parameter: %s = %s\n","Four", paramTwo);

  // Ready with the message.
  return true;
}

int TestSubSites(HTTPServer* p_server)
{
  int error = 0;

  // If errors, change detail level
  doDetails = false;

  CString url1("/MarlinTest/TestToken/One");
  CString url2("/MarlinTest/TestToken/Two");

  xprintf("TESTING SUB-SITE FUNCTIONS OF THE HTTP SERVER\n");
  xprintf("=============================================\n");

  // Create HTTP site to listen to "http://+:port/MarlinTest/TestToken/one or two"
  // This is a subsite of another one, so 5th parameter is set to true
  HTTPSite* site1 = p_server->CreateSite(PrefixType::URLPRE_Strong,false,TESTING_HTTP_PORT,url1,true);
  if(site1)
  {
    // SUMMARY OF THE TEST
    // --- "--------------------------- - ------\n"
    qprintf("HTTP subsite TestToken/One  : OK : %s\n",(LPCTSTR)site1->GetPrefixURL());
  }
  else
  {
    ++error;
    xerror();
    qprintf("ERROR: Cannot make a HTTP site for: %s\n",(LPCTSTR)url1);
    return error;
  }
  HTTPSite* site2 = p_server->CreateSite(PrefixType::URLPRE_Strong,false,TESTING_HTTP_PORT,url2,true);
  if(site2)
  {
    // SUMMARY OF THE TEST
    // --- "--------------------------- - ------\n"
    qprintf("HTTP subsite TestToken two  : OK : %s\n",(LPCTSTR)site2->GetPrefixURL());
  }
  else
  {
    ++error;
    xerror();
    qprintf("ERROR: Cannot make a HTTP site for: %s\n",(LPCTSTR)url2);
    return error;
  }

  // Testing the functionality that the check on the main site is correct!
  CString url3("/MarlinTest/Rubish/One");
  HTTPSite* site3 = p_server->CreateSite(PrefixType::URLPRE_Strong,false,TESTING_HTTP_PORT,url3,true);
  if(site3)
  {
    ++error;
    xerror();
    qprintf("ERROR: Unjust creation of a subsite: %s\n",(LPCTSTR)site3->GetPrefixURL());
    return error;
  }
  else
  {
    // SUMMARY OF THE TEST
    // --- "--------------------------- - ------\n"
    qprintf("Refused incorrect subsite   : OK : http://+:%d%s\n",TESTING_HTTP_PORT,(LPCTSTR)url3);
  }


  // Setting the POST handler for this site
  site1->SetHandler(HTTPCommand::http_post,new SiteHandlerSoapSubsite());
  site2->SetHandler(HTTPCommand::http_post,new SiteHandlerSoapSubsite());

  // Modify the standard settings for this site
  site1->AddContentType("","text/xml");
  site2->AddContentType("","text/xml");
  site1->AddContentType("xml","application/soap+xml");
  site2->AddContentType("xml","application/soap+xml");

  // Start the sites explicitly
  if(site1->StartSite())
  {
    xprintf("Site started correctly: %s\n",url1);
  }
  else
  {
    ++error;
    xerror();
    qprintf("ERROR STARTING SITE: %s\n",(LPCTSTR)url1);
  }

  if(site2->StartSite())
  {
    xprintf("Site started correctly: %s\n",url2);
  }
  else
  {
    ++error;
    xerror();
    qprintf("ERROR STARTING SITE: %s\n",(LPCTSTR)url2);
  }

  return error;
}

// Testing whether we can correctly remove a subsite from the server
int
StopSubsites(HTTPServer* p_server)
{
  int error = 0;

  CString url1("/MarlinTest/TestToken");
  CString url2("/MarlinTest/TestToken/One");
  CString url3("/MarlinTest/TestToken/Two");

  // Testing the main site. Should not be removed!!
  if(p_server->DeleteSite(TESTING_HTTP_PORT,url1))
  {
    qprintf("ERROR Incorrectly removed a main site: %s\n",(LPCTSTR)url1);
    qprintf("ERROR Other sites are dependend on it\n");
    xerror();
    ++error;
  }

  // Removing subsites. Should work
  if(p_server->DeleteSite(TESTING_HTTP_PORT,url2) == false)
  {
    qprintf("ERROR Deleting site : %s\n",(LPCTSTR)url2);
    xerror();
    ++error;
  }
  if(p_server->DeleteSite(TESTING_HTTP_PORT,url3) == false)
  {
    qprintf("ERROR Deleting site : %s\n",(LPCTSTR)url3);
    xerror();
    ++error;
  }

  // Now removing main site
  if(p_server->DeleteSite(TESTING_HTTP_PORT,url1) == false)
  {
    qprintf("ERROR Deleting site : %s\n",(LPCTSTR)url1);
    xerror();
    ++error;
  }
  return error;
}

int
AfterTestSubsites()
{
  // SUMMARY OF THE TEST
  // ---- "---------------------------------------------- - ------
  qprintf("All subsite tests received and tested          : %s\n",totalChecks > 0 ? "ERROR" : "OK");
  return totalChecks > 0;
}

