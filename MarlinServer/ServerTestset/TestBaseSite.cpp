/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: TestBaseSite.cpp
//
// Marlin Server: Internet server/client
// 
// Copyright (c) 2014-2022 ir. W.E. Huisman
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
#include "SiteHandlerGet.h"
#include "SiteHandlerPut.h"
#include "SiteHandlerOptions.h"
#include "EnsureFile.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

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
  qprintf("HTTP Base GET operation on the base site       : %s\n", ok ? "OK" : "ERROR");

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
  qprintf("HTTP Base PUT operation on the base site       : %s\n",ok ? "OK" : "ERROR");

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

  CString url("/MarlinTest/Site/");

  xprintf("TESTING STANDARD GET FUNCTIONS OF THE HTTP SERVER\n");
  xprintf("=================================================\n");

  // Create URL channel to listen to "http://+:port/MarlinTest/Site/"
  HTTPSite* site = m_httpServer->CreateSite(PrefixType::URLPRE_Strong,false,m_inPortNumber,url);
  if(site)
  {
    // SUMMARY OF THE TEST
    // --- "---------------------------------------------- - ------
    qprintf("HTTPSite for base 'get' test: OK : %s\n",site->GetPrefixURL().GetString());
    qprintf("HTTPSite for base 'put' test: OK : %s\n",site->GetPrefixURL().GetString());
  }
  else
  {
    ++error;
    xerror();
    qprintf("ERROR: Cannot make a HTTP site for: %s\n",(LPCTSTR)url);
    return error;
  }

  // Setting the default GET and PUT handler for this site
  SiteHandlerGetBase* handlerGet = new SiteHandlerGetBase();
  SiteHandlerPutBase* handlerPut = new SiteHandlerPutBase();
  site->SetHandler(HTTPCommand::http_get,handlerGet);
  site->SetHandler(HTTPCommand::http_put,handlerPut);
  // add OPTIONS handler for CORS
  site->SetHandler(HTTPCommand::http_options,new SiteHandlerOptions());

#ifdef MARLIN_STANDALONE
  // Setting the virtual root directory
  CString root = MarlinConfig::GetExePath() + "site";
  EnsureFile ensure;
  root = ensure.ReduceDirectoryPath(root);
  root = "virtual://" + root;
  site->SetWebroot(root);
#endif

  // Site is using CORS for safety
  site->SetUseCORS(true);
  site->SetCORSOrigin("http://localhost");    // this machine
  site->SetCORSMaxAge(43200);                 // Half a day

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
TestMarlinServer::AfterTestBaseSite()
{
  // SUMMARY OF THE TEST
  // ---- "---------------------------------------------- - ------
  qprintf("Base site file tests (GET / PUT)               : %s\n",totalChecks > 0 ? "ERROR" : "OK");
  return totalChecks > 0;
}
