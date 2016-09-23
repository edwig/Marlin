/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: TestBaseSite.cpp
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
#include "SiteHandlerGet.h"
#include "SiteHandlerPut.h"
#include "EnsureFile.h"

// TESTING A BASE SITE.
// RELIES ON THE DEFAULT 'GET' HANDLER OF THE SERVER

int TestBaseSite(HTTPServer* p_server)
{
  int error = 0;

  // If errors, change detail level
  doDetails = false;

  CString url("/MarlinTest/Site/");

  xprintf("TESTING STANDARD GET FUNCTIONS OF THE HTTP SERVER\n");
  xprintf("=================================================\n");

  // Create URL channel to listen to "http://+:port/MarlinTest/Site/"
  HTTPSite* site = p_server->CreateSite(PrefixType::URLPRE_Strong,false,TESTING_HTTP_PORT,url);
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
  SiteHandlerGet* handlerGet = new SiteHandlerGet();
  SiteHandlerPut* handlerPut = new SiteHandlerPut();
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
