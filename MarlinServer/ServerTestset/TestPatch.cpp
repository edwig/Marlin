/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: TestPatch.cpp
//
// Marlin Server: Internet server/client
// 
// Copyright (c) 2015-2018 ir. W.E. Huisman
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
#include "SiteHandlerPatch.h"
#include "SiteHandlerOptions.h"
#include "SiteFilter.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static int totalChecks = 2;

//////////////////////////////////////////////////////////////////////////
//
// FIRST DEFINE PATCH HANDLER
//
//////////////////////////////////////////////////////////////////////////

class SiteHandlerPatchMe: public SiteHandlerPatch
{
protected:
  bool Handle(HTTPMessage* p_message);
};

bool
SiteHandlerPatchMe::Handle(HTTPMessage* p_message)
{
  int errors = 0;
  HTTPMessage* msg = const_cast<HTTPMessage*>(p_message);
  CString body = msg->GetBody();
  xprintf("HTTP PATCH VERB URL [%s] FROM: %s\n",(LPCTSTR)msg->GetURL(),(LPCTSTR)SocketToServer(msg->GetSender()));
  xprintf("%s\n",(LPCTSTR)body);

  CrackedURL& url = msg->GetCrackedURL();
  CString bloodType    = url.GetParameter("type");
  CString rhesusFactor = url.GetParameter("rhesus");

  // Reset our HTTP message
  p_message->Reset();
  p_message->GetFileBuffer()->Reset();
  p_message->GetFileBuffer()->ResetFilename();

  // Test URL parameters
  if(bloodType != "ab" || rhesusFactor != "neg")
  {
    ++errors;
    xerror();
  }
  else
  {
    --totalChecks;
    p_message->SetBody("AB-\n");
  }

  // Check done

  // SUMMARY OF THE TEST
  // --- "---------------------------------------------- - ------
  qprintf("Less known HTTP verbs (PATCH)                  : %s\n",errors ? "ERROR" : "OK");

  return true;
}

//////////////////////////////////////////////////////////////////////////
//
// CREATE THE SITE
//
//////////////////////////////////////////////////////////////////////////

int
TestMarlinServer::TestPatch()
{
  int error = 0;

  // If errors, change detail level
  m_doDetails = false;

  CString url("/MarlinTest/Patching/");

  xprintf("TESTING HTTP PATCH VERB FUNCTION OF THE HTTP SERVER\n");
  xprintf("===================================================\n");

  // Create URL channel to listen to "http://+:port/MarlinTest/Patching/"
  // Callback function is no longer required!
  HTTPSite* site = m_httpServer->CreateSite(PrefixType::URLPRE_Strong,false,m_inPortNumber,url);
  if(site)
  {
    // SUMMARY OF THE TEST
    // --- "--------------------------- - ------\n"
    qprintf("HTTPSite for PATCH/OPTIONS  : OK : %s\n",site->GetPrefixURL().GetString());
  }
  else
  {
    ++error;
    xerror();
    qprintf("ERROR: Cannot make a HTTP site for: %s\n",(LPCTSTR)url);
    return error;
  }

  // Setting the POST handler for this site
  // And let the HTTP OPTIONS command reveal the fact that we handle patch calls
  site->SetHandler(HTTPCommand::http_patch,  new SiteHandlerPatchMe());
  site->SetHandler(HTTPCommand::http_options,new SiteHandlerOptions());
  // And also handle verb tunneling requests for the HTTP PATCH command
  // To test with or without verb tunneling, turn 'true' to 'false'
  site->SetVerbTunneling(true);

  // Modify the standard settings for this site
  site->AddContentType("","text/xml");
  site->AddContentType("xml","application/soap+xml");

  // Start the site explicitly
  if(site->StartSite())
  {
    xprintf("Site started correctly: %s\n",url.GetString());
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
TestMarlinServer::AfterTestPatch()
{
  // SUMMARY OF THE TEST
  //- --- "---------------------------------------------- - ------
  qprintf("HTTP PATCH verb tested                         : %s\n",totalChecks > 0 ? "ERROR" : "OK");
  return totalChecks > 0;
}
