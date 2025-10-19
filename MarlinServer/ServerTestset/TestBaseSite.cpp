/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: TestBaseSite.cpp
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
#include <SiteHandlerOptions.h>

static int totalChecks = 2;

class SiteHandlerGetBase : public SiteHandlerGet
{
protected:
  virtual bool Handle(HTTPMessage* p_message);
};

class SiteHandlerPutBase: public SiteHandlerPut
{
protected:
  virtual bool Handle(HTTPMessage* p_message);
};

bool
SiteHandlerGetBase::Handle(HTTPMessage* p_message)
{
  bool ok = false;
  bool result = SiteHandlerGet::Handle(p_message);

  if(p_message->GetStatus() == HTTP_STATUS_OK)
  {
    ok = true;
    --totalChecks;
  }

  // SUMMARY OF THE TEST
  // ---- "---------------------------------------------- - ------
  qprintf(_T("HTTP Base GET operation on the base site       : %s\n"), ok ? _T("OK") : _T("ERROR"));

  return result;
}

bool
SiteHandlerPutBase::Handle(HTTPMessage* p_message)
{
  bool ok = false;
  bool result = SiteHandlerPut::Handle(p_message);

  if(p_message->GetStatus() == HTTP_STATUS_CREATED)
  {
    ok = true;
    --totalChecks;
  }

  // SUMMARY OF THE TEST
  // ---- "---------------------------------------------- - ------
  qprintf(_T("HTTP Base PUT operation on the base site       : %s\n"),ok ? _T("OK") : _T("ERROR"));

  return result;
}

// TESTING A BASE SITE.
// RELIES ON THE DEFAULT 'GET' HANDLER OF THE SERVER

int 
TestMarlinServer::TestBaseSite()
{
  int error = 0;

  // If errors, change detail level
  m_doDetails = false;

  XString url(_T("/MarlinTest/Site/"));

  xprintf(_T("TESTING STANDARD GET FUNCTIONS OF THE HTTP SERVER\n"));
  xprintf(_T("=================================================\n"));

  // Create URL channel to listen to "http://+:port/MarlinTest/Site/"
  HTTPSite* site = m_httpServer->CreateSite(PrefixType::URLPRE_Strong,false,TESTING_HTTP_PORT,url,true);
  if(site)
  {
    // SUMMARY OF THE TEST
    // --- "---------------------------------------------- - ------
    qprintf(_T("HTTPSite for base 'get' test: OK : %s\n"),site->GetPrefixURL().GetString());
    qprintf(_T("HTTPSite for base 'put' test: OK : %s\n"),site->GetPrefixURL().GetString());
  }
  else
  {
    ++error;
    xerror();
    qprintf(_T("ERROR: Cannot make a HTTP site for: %s\n"),url.GetString());
    return error;
  }

  // Setting the default GET and PUT handler for this site
  SiteHandlerGetBase* handlerGet = alloc_new SiteHandlerGetBase();
  SiteHandlerPutBase* handlerPut = alloc_new SiteHandlerPutBase();
  site->SetHandler(HTTPCommand::http_get,handlerGet);
  site->SetHandler(HTTPCommand::http_put,handlerPut);
  // add OPTIONS handler for CORS
  site->SetHandler(HTTPCommand::http_options,alloc_new SiteHandlerOptions());

#ifdef MARLIN_STANDALONE
  // Setting the virtual root directory
  XString root = MarlinConfig::GetExePath() + _T("site");
  EnsureFile ensure;
  root = ensure.ReduceDirectoryPath(root);
  root = _T("virtual://") + root;
  site->SetWebroot(root);
#endif

  // Site is using CORS for safety
  site->SetUseCORS(true);
  site->SetCORSOrigin(_T("http://localhost"));    // this machine
  site->SetCORSMaxAge(43200);                     // Half a day

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
TestMarlinServer::AfterTestBaseSite()
{
  // SUMMARY OF THE TEST
  // ---- "---------------------------------------------- - ------
  qprintf(_T("Base site file tests (GET / PUT)               : %s\n"),totalChecks > 0 ? _T("ERROR") : _T("OK"));
  return totalChecks > 0;
}
