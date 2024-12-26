/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: TestSecureSite.cpp
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
#include <SiteHandlerGet.h>
#include <SiteHandlerPut.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static int g_gotten = 0;
static int g_placed = 0;

class SiteHandlerGetSecure : public SiteHandlerGet
{
protected:
  virtual bool Handle(HTTPMessage* p_message);
};

bool
SiteHandlerGetSecure::Handle(HTTPMessage* p_message)
{
  bool result = SiteHandlerGet::Handle(p_message);

  if(p_message->GetStatus() == HTTP_STATUS_OK)
  {
    ++g_gotten;
  }
  qprintf(_T("File gotten with GET from secure site          : %s\n"), g_gotten > 0 ? _T("OK") : _T("ERROR"));
  return result;
}

class SiteHandlerPutSecure : public SiteHandlerPut
{
public:
  virtual bool Handle(HTTPMessage* p_message) override;
};

bool
SiteHandlerPutSecure::Handle(HTTPMessage* p_message)
{
  bool result = SiteHandlerPut::Handle(p_message);

  if(p_message->GetStatus() == HTTP_STATUS_OK)
  {
    ++g_placed;
  }
  qprintf(_T("File sent with PUT to secure site              : %s\n"), g_placed > 0 ? _T("OK") : _T("ERROR"));
  return result;
}

// TESTING A BASE SITE.
// RELIES ON THE DEFAULT 'GET' HANDLER OF THE SERVER

int 
TestMarlinServer::TestSecureSite(bool p_standalone)
{
  int error = 0;

  // If errors, change detail level
  m_doDetails = false;

  XString url(_T("/SecureTest/"));

  xprintf(_T("TESTING STANDARD GET FUNCTIONS OF THE HTTP SERVER\n"));
  xprintf(_T("=================================================\n"));

  // Create URL channel to listen to "https://+:port/SecureTest"
  HTTPSite* site = m_httpServer->CreateSite(PrefixType::URLPRE_Strong,true,TESTING_HTTPS_PORT,url);
  if(site)
  {
    // SUMMARY OF THE TEST
    // ---- "--------------------------- - ------
    qprintf(_T("HTTPSite for 'get' test     : OK : %s\n"),site->GetPrefixURL().GetString());
    qprintf(_T("HTTPSite for 'put' test     : OK : %s\n"),site->GetPrefixURL().GetString());
  }
  else
  {
    ++error;
    xerror();
    qprintf(_T("ERROR: Cannot make a HTTP site for: %s\n"),url.GetString());
    return error;
  }

  // Setting the default GET and PUT handler for this site
  SiteHandlerGetSecure* handlerGet = new SiteHandlerGetSecure();
  SiteHandlerPutSecure* handlerPut = new SiteHandlerPutSecure();
  site->SetHandler(HTTPCommand::http_get,handlerGet);
  site->SetHandler(HTTPCommand::http_put,handlerPut);

#ifdef MARLIN_STANDALONE
  // Setting the virtual root directory
  XString root = MarlinConfig::GetExePath() + _T("site");
  EnsureFile ensure;
  root = ensure.ReduceDirectoryPath(root);
  root = _T("virtual://") + root;
  site->SetWebroot(root);
#endif

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

// See if we did everything alright
int
TestMarlinServer::AfterTestSecureSite()
{
  // SUMMARY OF THE TEST
  //- --- "---------------------------------------------- - ------
  qprintf(_T("File gotten with GET from secure site          : %s"), g_gotten  > 0 ? _T("OK") : _T("ERROR"));
  return (g_gotten > 0) ? 0 : 1;
}

