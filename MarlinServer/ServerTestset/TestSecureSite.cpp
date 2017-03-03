/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: TestSecureSite.cpp
//
// Marlin Server: Internet server/client
// 
// Copyright (c) 2017 ir. W.E. Huisman
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
#include "SiteHandlerGet.h"
#include "SiteHandlerPut.h"
#include "EnsureFile.h"

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
  return result;
}

// TESTING A BASE SITE.
// RELIES ON THE DEFAULT 'GET' HANDLER OF THE SERVER

int TestSecureSite(HTTPServer* p_server)
{
  int error = 0;

  // If errors, change detail level
  doDetails = false;

  CString url("/SecureTest/");

  xprintf("TESTING STANDARD GET FUNCTIONS OF THE HTTP SERVER\n");
  xprintf("=================================================\n");

  // Create URL channel to listen to "https://+:port/SecureTest"
  HTTPSite* site = p_server->CreateSite(PrefixType::URLPRE_Strong,true,TESTING_HTTPS_PORT,url);
  if(site)
  {
    // SUMMARY OF THE TEST
    // ---- "--------------------------- - ------
    qprintf("HTTPSite for 'get' test     : OK : %s\n",site->GetPrefixURL().GetString());
    qprintf("HTTPSite for 'put' test     : OK : %s\n",site->GetPrefixURL().GetString());
  }
  else
  {
    ++error;
    xerror();
    qprintf("ERROR: Cannot make a HTTP site for: %s\n",(LPCTSTR)url);
    return error;
  }

  // Setting the default GET and PUT handler for this site
  SiteHandlerGetSecure* handlerGet = new SiteHandlerGetSecure();
  SiteHandlerPutSecure* handlerPut = new SiteHandlerPutSecure();
  site->SetHandler(HTTPCommand::http_get,handlerGet);
  site->SetHandler(HTTPCommand::http_put,handlerPut);

#ifdef MARLIN_STANDALONE
  // Setting the virtual root directory
  CString root = WebConfig::GetExePath() + "site";
  EnsureFile ensure;
  root = ensure.ReduceDirectoryPath(root);
  root = "virtual://" + root;
  site->SetWebroot(root);
#endif

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

// See if we did everything alright
int
AfterTestSecureSite()
{
  // SUMMARY OF THE TEST
  //- --- "---------------------------------------------- - ------
  qprintf("File gotten with GET from secure site          : %s", g_gotten  > 0 ? "OK" : "ERROR");
  return (g_gotten > 0) ? 0 : 1;
}

