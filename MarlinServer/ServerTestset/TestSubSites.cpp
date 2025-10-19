/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: TestSubSites.cpp
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
#include "TestPorts.h"
#include <HTTPSite.h>
#include <SiteHandlerSoap.h>

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
  XString paramOne = p_message->GetParameter(_T("One"));
  XString paramTwo = p_message->GetParameter(_T("Two"));
  xprintf(_T("Incoming parameter: %s = %s\n"),_T("One"),paramOne.GetString());
  xprintf(_T("Incoming parameter: %s = %s\n"),_T("Two"),paramTwo.GetString());

  // Forget
  p_message->Reset();

  // Do our work
  bool result = false;
  if(paramOne == _T("ABC") && paramTwo == _T("1-2-3"))
  {
    paramOne = _T("DEF");
    paramTwo = _T("123");
    result = true;
    --totalChecks;
  }
  // SUMMARY OF THE TEST
  // --- "---------------------------------------------- - ------
  qprintf(_T("Subsite SOAP handler on main site              : %s\n"),result ? _T("OK") : _T("ERROR"));
  xprintf(_T("Site of this message was    : %s\n"),p_message->GetHTTPSite()->GetSite().GetString());
  if(!result) xerror();

  // Set the result
  p_message->SetParameter(_T("Three"),paramOne);
  p_message->SetParameter(_T("Four"), paramTwo);
  xprintf(_T("Outgoing parameter: %s = %s\n"),_T("Three"),paramOne.GetString());
  xprintf(_T("Outgoing parameter: %s = %s\n"),_T("Four"), paramTwo.GetString());

  // Ready with the message.
  return true;
}

int 
TestMarlinServer::TestSubSites()
{
  int error = 0;

  // If errors, change detail level
  m_doDetails = false;

  XString url1(_T("/MarlinToken/One"));
  XString url2(_T("/MarlinToken/Two"));

  xprintf(_T("TESTING SUB-SITE FUNCTIONS OF THE HTTP SERVER\n"));
  xprintf(_T("=============================================\n"));

  // Create HTTP site to listen to "http://+:port/MarlinToken/one or two"
  // This is a subsite of another one, so 5th parameter is set to true
  HTTPSite* site1 = m_httpServer->CreateSite(PrefixType::URLPRE_Strong,false,TESTING_TOKEN_PORT,url1,true);
  if(site1)
  {
    // SUMMARY OF THE TEST
    // ---- "--------------------------- - ------\n"
    qprintf(_T("HTTP subsite TestToken/One  : OK : %s\n"),site1->GetPrefixURL().GetString());
  }
  else
  {
    ++error;
    xerror();
    qprintf(_T("ERROR: Cannot make a HTTP site for: %s\n"),url1.GetString());
    return error;
  }
  HTTPSite* site2 = m_httpServer->CreateSite(PrefixType::URLPRE_Strong,false,TESTING_TOKEN_PORT,url2,true);
  if(site2)
  {
    // SUMMARY OF THE TEST
    // ---- "--------------------------- - ------\n"
    qprintf(_T("HTTP subsite TestToken/Two  : OK : %s\n"),site2->GetPrefixURL().GetString());
  }
  else
  {
    ++error;
    xerror();
    qprintf(_T("ERROR: Cannot make a HTTP site for: %s\n"),url2.GetString());
    return error;
  }

  // Setting the POST handler for this site
  site1->SetHandler(HTTPCommand::http_post,alloc_new SiteHandlerSoapSubsite());
  site2->SetHandler(HTTPCommand::http_post,alloc_new SiteHandlerSoapSubsite());

  // Modify the standard settings for this site
  site1->AddContentType(true,_T("pos"),_T("text/xml"));
  site2->AddContentType(true,_T("pos"),_T("text/xml"));
  site1->AddContentType(true,_T("xml"),_T("application/soap+xml"));
  site2->AddContentType(true,_T("xml"),_T("application/soap+xml"));

  // Set sites to use NTLM authentication for the "MarlinTest" user
  // So we can get a different token, then the current server token
  site1->SetAuthenticationScheme(_T("NTLM"));
  site2->SetAuthenticationScheme(_T("NTLM"));
  site1->SetAuthenticationNTLMCache(true);
  site2->SetAuthenticationNTLMCache(true);

  // Start the sites explicitly
  if(site1->StartSite())
  {
    xprintf(_T("Site started correctly: %s\n"),url1.GetString());
  }
  else
  {
    ++error;
    xerror();
    qprintf(_T("ERROR STARTING SITE: %s\n"),url1.GetString());
  }

  if(site2->StartSite())
  {
    xprintf(_T("Site started correctly: %s\n"),url2.GetString());
  }
  else
  {
    ++error;
    xerror();
    qprintf(_T("ERROR STARTING SITE: %s\n"),url2.GetString());
  }

  return error;
}

// Testing whether we can correctly remove a subsite from the server
int
TestMarlinServer::StopSubsites()
{
  int error = 0;

  XString url1(_T("/MarlinToken"));
  XString url2(_T("/MarlinToken/One"));
  XString url3(_T("/MarlinToken/Two"));

  if(true)
  {
    // TEST TO REMOVE RECURSIVELY

    // Testing the main site. Should not be removed!!
    if(m_httpServer->DeleteSite(TESTING_TOKEN_PORT,url1))
    {
      qprintf(_T("ERROR Incorrectly removed a main site: %s\n"),url1.GetString());
      qprintf(_T("ERROR Other sites are dependent on it\n"));
      xerror();
      ++error;
    }
  }
  else
  {
    // TEST TO REMOVE IN-ORDER

    // Removing sub-sites. Should work
    if(m_httpServer->DeleteSite(TESTING_TOKEN_PORT,url2) == false)
    {
      qprintf(_T("ERROR Deleting site : %s\n"),url2.GetString());
      xerror();
      ++error;
    }
    if(m_httpServer->DeleteSite(TESTING_TOKEN_PORT,url3) == false)
    {
      qprintf(_T("ERROR Deleting site : %s\n"),url3.GetString());
      xerror();
      ++error;
    }

    // Now removing main site
    if(m_httpServer->DeleteSite(TESTING_TOKEN_PORT,url1) == false)
    {
      qprintf(_T("ERROR Deleting site : %s\n"),url1.GetString());
      xerror();
      ++error;
    }
  }
  return error;
}

int
TestMarlinServer::AfterTestSubSites()
{
  // SUMMARY OF THE TEST
  // ---- "---------------------------------------------- - ------
  qprintf(_T("All subsite tests received and tested          : %s\n"),totalChecks > 0 ? _T("ERROR") : _T("OK"));
  return totalChecks > 0;
}

