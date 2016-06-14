/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: TestToken.cpp
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
#include "SiteHandlerSoap.h"
#include "PrintToken.h"
#include "GetLastErrorAsString.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

class SiteHandlerSoapToken: public SiteHandlerSoap
{
public:
  bool  Handle(SOAPMessage* p_message);
private:
  void  TestSecurity(SOAPMessage* p_msg);
};

bool
SiteHandlerSoapToken::Handle(SOAPMessage* p_message)
{
  TestSecurity(p_message);
  return true;
}

void 
SiteHandlerSoapToken::TestSecurity(SOAPMessage* p_msg)
{
  HANDLE   file     = NULL;
  HANDLE   token    = p_msg->GetAccessToken();
  DWORD    length   = 0;
  HANDLE   htok     = NULL;
  CString  listing;

  // See if we have a token
  if(token == NULL)
  {
    printf("Not an authenticated call\n");
    return;
  }

  // Find the token of our server process

  if(!OpenThreadToken(GetCurrentThread(),MAXIMUM_ALLOWED,TRUE,&htok))
  {
    if(!OpenProcessToken(GetCurrentProcess(),MAXIMUM_ALLOWED,&htok))
    {
      printf("ERROR: OpenProcessToken failed: error code 0x%lx\n",GetLastError());
      xerror();
      return;
    }
  }

  listing = "\nPRIMARY SERVER TOKEN\n";

  printf("Processing primary server token\n");

  if(!DumpToken(listing,htok))
  {
    printf("ERROR: DumpTokenInformation for process failed\n");
    CloseHandle(htok);
    xerror();
    return;
  }
  CloseHandle(htok);
  listing += "\n\n";

  // Print The passed token
  printf("Processing received HTTP token\n");
  listing += "RECEIVED HTTP TOKEN\n";
  DumpToken(listing,token);

  // Check if token holds the 'owner' information
  GetTokenInformation(token,TokenOwner,NULL,0,&length);
  if(!length)
  {
    printf("ERROR: Cannot find the token owner, or token not present\n");
    xerror();
    return;
  }
  // Try to do everything in one call
  // Only CreateProcess has to be rewritten to CreateProcessAsUser(hToken,...)
  if(ImpersonateLoggedOnUser(token) == FALSE)
  {
    printf("ERROR: Cannot impersonate logged on user\n");
    // Cannot continue, otherwise a security breach could occur
    xerror();
    return;
  }

  // Create file name
  CString fileName = g_baseDir + "FileOwner.txt";

  // Try to open a file as another user
  file = CreateFile(fileName
                   ,GENERIC_READ | GENERIC_WRITE
                   ,FILE_SHARE_READ | FILE_SHARE_WRITE
                   ,NULL // &attrib
                   ,CREATE_ALWAYS
                   ,FILE_ATTRIBUTE_NORMAL
                   ,NULL);
  if(file == INVALID_HANDLE_VALUE)
  {
    // These errors are quite common, e.g. not started as an administrator
    // 1307 - ERROR_INVALID_OWNER
    // 1368 - ERROR_CANNOT_IMPERSONATE
    // 1346 - ERROR_BAD_IMPERSONATION_LEVEL
    xerror();
    int error = GetLastError();
    printf("ERROR: Creating file: %d : %s\n",error,GetLastErrorAsString(error).GetString());
  }
  else
  {
    DWORD written = 0;
    CString msg = p_msg->GetSoapMessage();
    msg.Replace("\n","\r\n");

    if(WriteFile(file,msg.GetString(),msg.GetLength(),&written,NULL) == FALSE)
    {
      printf("ERROR: Writing to file: %d\n",GetLastError());
      xerror();
    }
    else
    {
      printf("SOAP message written to: %s\n",(LPCTSTR)fileName);
    }

    listing.Replace("\n","\r\n");
    if(WriteFile(file,listing.GetString(),listing.GetLength(),&written,NULL) == FALSE)
    {
      printf("ERROR: Writing to file: %d\n",GetLastError());
      xerror();
    }
    else
    {
      printf("TOKEN Access written to: %s\n",(LPCTSTR)fileName);
    }
    // Close the file
    CloseHandle(file);
  }

  // Correctly came to the end
  p_msg->Reset();
  p_msg->SetParameter("Testing","OK");

  // NO LONGER NEEDED: SiteHandler does this now!
  // End of identity crisis :-)
  // RevertToSelf();
}

int 
TestToken(HTTPServer* p_server)
{
  int error = 0;

  // If errors, change detail level
  doDetails = false;

  CString url("/MarlinTest/TestToken/");

  xprintf("TESTING THE TOKEN FUNCTIONS OF THE HTTP SERVER\n");
  xprintf("==============================================\n");

  // Create URL channel to listen to "http://+:1200/MarlinTest/TestToken/"
  HTTPSite* site = p_server->CreateSite(PrefixType::URLPRE_Strong,false,TESTING_HTTP_PORT,url);
  if(site)
  {
    // SUMMARY OF THE TEST
    // --- "--------------------------- - ------\n"
    printf("HTTPSite DACL token testing : OK : %s\n",site->GetPrefixURL().GetString());
  }
  else
  {
    ++error;
    xerror();
    printf("ERROR: Cannot make a HTTP site for: %s\n",(LPCTSTR)url);
    return error;
  }

  // Setting the POST handler for this site
  site->SetHandler(HTTPCommand::http_post,new SiteHandlerSoapToken());

  // Modify the standard settings for this site
  site->AddContentType("","text/xml");
  site->AddContentType("xml","application/soap+xml");

  // Set site to use NTLM authentication for the "MerlinTest" user
  // So we can get a different token, then the current server token
  site->SetAuthenticationScheme("NTLM");
  site->SetAuthenticationNTLMCache(true);

  // Start the site explicitly
  if(site->StartSite())
  {
    xprintf("Site started correctly: %s\n",url);
  }
  else
  {
    ++error;
    xerror();
    printf("ERROR STARTING SITE: %s\n",(LPCTSTR)url);
  }
  return error;
}

