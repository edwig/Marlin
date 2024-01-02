/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: TestToken.cpp
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
#include "HTTPSite.h"
#include "SiteHandlerSoap.h"
#include "PrintToken.h"
#include "GetLastErrorAsString.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static int totalChecks = 8;

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
  // Check done
  --totalChecks;

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
  int      errors   = 0;
  XString  fileName = MarlinConfig::GetExePath() + _T("FileOwner.txt");
  XString  listing;

  // See if we have a token
  if(token == NULL)
  {
    // --- "---------------------------------------------- - ------
    qprintf(_T("Test token: Not an authenticated call          : ERROR\n"));
    xerror();
    ++errors;
    goto END;
  }
  --totalChecks;

  // Find the token of our server process

  if(!OpenThreadToken(GetCurrentThread(),MAXIMUM_ALLOWED,TRUE,&htok))
  {
    if(!OpenProcessToken(GetCurrentProcess(),MAXIMUM_ALLOWED,&htok))
    {
      // --- "---------------------------------------------- - ------
      qprintf(_T("OpenProcessToken failed                        : ERROR\n"));
      xprintf(_T("Error code 0x%lx\n"),GetLastError());
      ++errors;
      xerror();
      goto END;
    }
  }
  --totalChecks;

  listing = _T("\nPRIMARY SERVER TOKEN\n");

  xprintf(_T("Processing primary server token\n"));

  if(!DumpToken(listing,htok))
  {
    // --- "---------------------------------------------- - ------
    qprintf(_T("DumpTokenInformation for process failed        : ERROR\n"));
    CloseHandle(htok);
    xerror();
    ++errors;
    goto END;
  }
  --totalChecks;
  CloseHandle(htok);
  listing += _T("\n\n");

  // Print The passed token
  xprintf(_T("Processing received HTTP token\n"));
  listing += _T("RECEIVED HTTP TOKEN\n");
  DumpToken(listing,token);

  // Check if token holds the 'owner' information
  GetTokenInformation(token,TokenOwner,NULL,0,&length);
  if(!length)
  {
    // --- "---------------------------------------------- - ------
    qprintf(_T("Cannot find the token owner, or no token       : ERROR\n"));
    xerror();
    ++errors;
    goto END;
  }
  --totalChecks;
  // Try to do everything in one call
  // Only CreateProcess has to be rewritten to CreateProcessAsUser(hToken,...)
  if(ImpersonateLoggedOnUser(token) == FALSE)
  {
    // --- "---------------------------------------------- - ------
    qprintf(_T("Cannot impersonate logged on user by token     : ERROR\n"));
    // Cannot continue, otherwise a security breach could occur
    xerror();
    ++errors;
    goto END;
  }

  --totalChecks;
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
    // --- "---------------------------------------------- - ------
    qprintf(_T("Cannot create file for file token owner        : ERROR\n"));
    xprintf(_T("ERROR %d : %s\n"),error,GetLastErrorAsString(error).GetString());
  }
  else
  {
    DWORD written = 0;
    XString msg = p_msg->GetSoapMessage();
    msg.Replace(_T("\n"),_T("\r\n"));

    if(WriteFile(file,msg.GetString(),msg.GetLength(),&written,NULL) == FALSE)
    {
      // --- "---------------------------------------------- - ------
      qprintf(_T("Writing to file. Error [%d]                    : ERROR\n"),GetLastError());
      xerror();
      ++errors;
    }
    else
    {
      --totalChecks;
      xprintf(_T("SOAP message written to: %s\n"),fileName.GetString());
    }

    listing.Replace(_T("\n"),_T("\r\n"));
    if(WriteFile(file,listing.GetString(),listing.GetLength(),&written,NULL) == FALSE)
    {
      // --- "---------------------------------------------- - ------
      qprintf(_T("Writing to file. Error [%d]                    : ERROR\n"),GetLastError());
      xerror();
      ++errors;
    }
    else
    {
      --totalChecks;
      xprintf(_T("TOKEN Access written to: %s\n"),fileName.GetString());
    }
    // Close the file
    CloseHandle(file);
  }

END:
  // Correctly came to the end
  p_msg->Reset();
  p_msg->SetParameter(_T("Testing"),errors ? _T("ERRORS") : _T("OK"));

  // --- "---------------------------------------------- - ------
  qprintf(_T("Token handling on incoming message             : %s\n"),errors ? _T("ERROR") : _T("OK"));

  // NO LONGER NEEDED: SiteHandler does this now!
  // End of identity crisis :-)
  // RevertToSelf();
}

int 
TestMarlinServer::TestToken()
{
  int error = 0;

  // If errors, change detail level
  m_doDetails = false;

  XString url(_T("/MarlinTest/TestToken/"));

  xprintf(_T("TESTING THE TOKEN FUNCTIONS OF THE HTTP SERVER\n"));
  xprintf(_T("==============================================\n"));

  // Create URL channel to listen to "http://+:port/MarlinTest/TestToken/"
  HTTPSite* site = m_httpServer->CreateSite(PrefixType::URLPRE_Strong,false,m_inPortNumber,url);
  if(site)
  {
    // SUMMARY OF THE TEST
    // --- "--------------------------- - ------\n"
    qprintf(_T("HTTPSite DACL token testing : OK : %s\n"),site->GetPrefixURL().GetString());
  }
  else
  {
    ++error;
    xerror();
    qprintf(_T("ERROR: Cannot make a HTTP site for: %s\n"),url.GetString());
    return error;
  }

  // Setting the POST handler for this site
  site->SetHandler(HTTPCommand::http_post,new SiteHandlerSoapToken());

  // Modify the standard settings for this site
  site->AddContentType(_T(""),_T("text/xml"));
  site->AddContentType(_T("xml"),_T("application/soap+xml"));

  // Set site to use NTLM authentication for the "MerlinTest" user
  // So we can get a different token, then the current server token
  site->SetAuthenticationScheme(_T("NTLM"));
  site->SetAuthenticationNTLMCache(true);

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
TestMarlinServer::AfterTestToken()
{
  // SUMMARY OF THE TEST
  // ---- "---------------------------------------------- - ------
  qprintf(_T("Testing of authentication token capabilities   : %s\n"),totalChecks > 0 ? _T("ERROR") : _T("OK"));
  return totalChecks > 0;
}
