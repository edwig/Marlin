/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: TestCompression.cpp
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
#include "SiteHandlerGet.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

class SiteHandlerGetCompress: public SiteHandlerGet
{
public:
  bool Handle(HTTPMessage* p_message);
};

bool
SiteHandlerGetCompress::Handle(HTTPMessage* p_message)
{
  CString filename("..\\Documentation\\ReleaseNotes_v1.txt");

  // NOT Much here. Always returns the release message file
  p_message->Reset();
  p_message->GetFileBuffer()->SetFileName(filename);

  // SUMMARY OF THE TEST
  // --- "--------------------------- - ------\n"
  printf("TEST GZIP GET OF FILE       : OK\n");

  return true;
}

int
TestCompression(HTTPServer* p_server)
{
  int error = 0;
  // If errors, change detail level
  doDetails = false;

  xprintf("TESTING HTTP GZIP COMPRESSION OF THE HTTP SERVER\n");
  xprintf("================================================\n");

  // Create URL channel to listen to "http://+:1200/MarlinTest/Compression/"
  // Callback function is no longer required!
  CString webaddress = "/MarlinTest/Compression/";
  HTTPSite* site = p_server->CreateSite(PrefixType::URLPRE_Strong,false,TESTING_HTTP_PORT,webaddress);
  if(site)
  {
    // SUMMARY OF THE TEST
    // --- "--------------------------- - ------\n"
    printf("HTTPSite GZIP compression   : OK : %s\n",(LPCTSTR)site->GetPrefixURL());
  }
  else
  {
    ++error;
    xerror();
    printf("ERROR: Cannot register a website for : %s\n",(LPCTSTR)webaddress);
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
    printf("ERROR STARTING SITE: %s\n",(LPCTSTR)webaddress);
  }
  return error;
}
