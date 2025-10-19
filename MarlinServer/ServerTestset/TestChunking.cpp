/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: TestChunked.cpp
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
#include <SiteHandlerGet.h>
#include <io.h>

static int totalChecks = 0;

class SiteHandlerGetChunking : public SiteHandlerGet
{
protected:
  bool Handle(HTTPMessage* p_message);
};

bool
SiteHandlerGetChunking::Handle(HTTPMessage* p_message)
{
  // Check if releasenotes where requested
  Routing& routing = const_cast<Routing&>(p_message->GetRouting());
  if(routing.size() > 0)
  {
    if(routing.back().CompareNoCase(_T("releasenotes.txt")))
    {
      return false;
    }
  }

  bool result = false;
  XString filename(_T("C:\\Develop\\Marlin\\Documentation\\ReleaseNotes_v7.txt"));
  XString empty;

  // NOT Much here. Always returns the release message file
  p_message->Reset();
  p_message->GetFileBuffer()->SetFileName(filename);
  p_message->GetFileBuffer()->ReadFile();
  p_message->GetFileBuffer()->SetFileName(empty);

  bool chunk1 = m_site->SendAsChunk(p_message,false);

  p_message->Reset();
  p_message->GetFileBuffer()->SetFileName(filename);
  p_message->GetFileBuffer()->ReadFile();
  p_message->GetFileBuffer()->SetFileName(empty);

  bool chunk2 = m_site->SendAsChunk(p_message,true);

  if(chunk1 && chunk2)
  {
    result = true;
    --totalChecks;
  }

  // SUMMARY OF THE TEST
  // ---- "---------------------------------------------- - ------
  qprintf(_T("Chunked operation sending 2 chunks             : %s\n"), result ? _T("OK") : _T("ERROR"));

  return true;
}

int
TestMarlinServer::TestChunking()
{
  int error = 0;
  // If errors, change detail level
  m_doDetails = false;

  xprintf(_T("TESTING HTTP CHUNKED TRANSFER-ENCODING\n"));
  xprintf(_T("======================================\n"));

  // Create URL channel to listen to "http://+:port/MarlinTest/Chunking/"
  // Callback function is no longer required!
  XString webaddress = _T("/MarlinTest/Chunking/");
  HTTPSite* site = m_httpServer->CreateSite(PrefixType::URLPRE_Strong,false,TESTING_HTTP_PORT,webaddress,true);
  if (site)
  {
    // SUMMARY OF THE TEST
    // --- "--------------------------- - ------\n"
    qprintf(_T("HTTPSite Chunked transfer  : OK : %s\n"),site->GetPrefixURL().GetString());
  }
  else
  {
    ++error;
    xerror();
    qprintf(_T("ERROR: Cannot register a website for : %s\n"),webaddress.GetString());
    return error;
  }

  // Setting a site handler !!
  site->SetHandler(HTTPCommand::http_get, alloc_new SiteHandlerGetChunking());

  // new: Start the site explicitly
  if (site->StartSite())
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
TestMarlinServer::AfterTestChunking()
{
  // SUMMARY OF THE TEST
  // ---- "---------------------------------------------- - ------
  qprintf(_T("File sending with Transfer-encoding: chunked   : %s\n"), totalChecks > 0 ? _T("ERROR") : _T("OK"));
  return totalChecks > 0;
}
