/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: TestCompression.cpp
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
#include "pch.h"
#include "TestMarlinServer.h"
#include "TestPorts.h"
#include <SiteHandlerGet.h>
#include <io.h>

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
  XString filename(_T("C:\\Develop\\Marlin\\Documentation\\ReleaseNotes_v1.txt"));

  // NOT Much here. Always returns the release message file
  p_message->Reset();
  p_message->GetFileBuffer()->SetFileName(filename);

  if(_taccess(filename,0) == 0)
  {
    result = true;
    --totalChecks;
  }

  // SUMMARY OF THE TEST
  // ---- "---------------------------------------------- - ------
  qprintf(_T("GZIP of a file at a HTTP GET operation         : %s\n"), result ? _T("OK") : _T("ERROR"));

  return true;
}

int
TestMarlinServer::TestCompression()
{
  int error = 0;
  // If errors, change detail level
  m_doDetails = false;

  xprintf(_T("TESTING HTTP GZIP COMPRESSION OF THE HTTP SERVER\n"));
  xprintf(_T("================================================\n"));

  // Create URL channel to listen to "http://+:port/MarlinTest/Compression/"
  // Callback function is no longer required!
  XString webaddress = _T("/MarlinTest/Compression/");
  HTTPSite* site = m_httpServer->CreateSite(PrefixType::URLPRE_Strong,false,TESTING_HTTP_PORT,webaddress,true);
  if(site)
  {
    // SUMMARY OF THE TEST
    // --- "--------------------------- - ------\n"
    qprintf(_T("HTTPSite GZIP compression   : OK : %s\n"),site->GetPrefixURL().GetString());
  }
  else
  {
    ++error;
    xerror();
    qprintf(_T("ERROR: Cannot register a website for : %s\n"),webaddress.GetString());
    return error;
  }

  // Setting a site handler !!
  site->SetHandler(HTTPCommand::http_get,alloc_new SiteHandlerGetCompress());

  // set compression on this site
  site->SetHTTPCompression(true);

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
TestMarlinServer::AfterTestCompression()
{
  // SUMMARY OF THE TEST
  // ---- "---------------------------------------------- - ------
  qprintf(_T("File compression with GZIP tested              : %s\n"), totalChecks > 0 ? _T("ERROR") : _T("OK"));
  return totalChecks > 0;
}
