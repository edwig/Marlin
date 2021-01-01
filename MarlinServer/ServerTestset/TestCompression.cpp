/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: TestCompression.cpp
//
// Marlin Server: Internet server/client
// 
// Copyright (c) 2014-2021 ir. W.E. Huisman
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
#include "SiteHandlerGet.h"
#include <io.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static int totalChecks = 1;

class SiteHandlerGetCompress: public SiteHandlerGet
{
protected:
  bool Handle(HTTPMessage* p_message);
};

bool
SiteHandlerGetCompress::Handle(HTTPMessage* p_message)
{
  bool result = false;
  CString filename("C:\\Develop\\Marlin\\Documentation\\ReleaseNotes_v1.txt");

  // NOT Much here. Always returns the release message file
  p_message->Reset();
  p_message->GetFileBuffer()->SetFileName(filename);

  if(_access(filename,0) == 0)
  {
    result = true;
    --totalChecks;
  }

  // SUMMARY OF THE TEST
  // ---- "---------------------------------------------- - ------
  qprintf("GZIP of a file at a HTTP GET operation         : %s\n", result ? "OK" : "ERROR");

  return true;
}

int
TestMarlinServer::TestCompression()
{
  int error = 0;
  // If errors, change detail level
  m_doDetails = false;

  xprintf("TESTING HTTP GZIP COMPRESSION OF THE HTTP SERVER\n");
  xprintf("================================================\n");

  // Create URL channel to listen to "http://+:port/MarlinTest/Compression/"
  // Callback function is no longer required!
  CString webaddress = "/MarlinTest/Compression/";
  HTTPSite* site = m_httpServer->CreateSite(PrefixType::URLPRE_Strong,false,m_inPortNumber,webaddress);
  if(site)
  {
    // SUMMARY OF THE TEST
    // --- "--------------------------- - ------\n"
    qprintf("HTTPSite GZIP compression   : OK : %s\n",(LPCTSTR)site->GetPrefixURL());
  }
  else
  {
    ++error;
    xerror();
    qprintf("ERROR: Cannot register a website for : %s\n",(LPCTSTR)webaddress);
    return error;
  }

  // Setting a site handler !!
  site->SetHandler(HTTPCommand::http_get,new SiteHandlerGetCompress());

  // set compression on this site
  site->SetHTTPCompression(true);

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
TestMarlinServer::AfterTestCompression()
{
  // SUMMARY OF THE TEST
  // ---- "---------------------------------------------- - ------
  qprintf("File compression with GZIP tested              : %s\n", totalChecks > 0 ? "ERROR" : "OK");
  return totalChecks > 0;
}
