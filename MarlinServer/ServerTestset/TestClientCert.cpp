/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: TestClientCertificate.cpp
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
#include "SiteHandlerGet.h"
#include "SiteFilterClientCertificate.h"
#include "ServerApp.h"
#include "TestPorts.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static int totalChecks = 1;

class SiteHandlerGetClientCert: public SiteHandlerGet
{
protected:
  void PostHandle(HTTPMessage* p_message) override;
};

// BEWARE: in case of an incorrect client-certificate
// the site handlers will NEVER be called
// so we will never come to here!!
void
SiteHandlerGetClientCert::PostHandle(HTTPMessage* p_message)
{
  SiteHandlerGet::PostHandle(p_message);
  bool result = p_message->GetStatus() == HTTP_STATUS_OK;

  // SUMMARY OF THE TEST
  // ---- "---------------------------------------------- - ------
  qprintf(_T("Client certificate at a HTTP GET operation     : %s\n"),result ? _T("OK") : _T("ERROR"));
  if(result) 
  {
    --totalChecks;
  }
  else
  {
    xerror();
  }
}

int 
TestMarlinServer::TestClientCertificate(bool p_standalone)
{
  int error = 0;

  // If errors, change detail level
  m_doDetails = false;

  XString url(_T("/SecureClientCert/"));

  xprintf(_T("TESTING CLIENT CERTIFICATE FUNCTION OF THE HTTP SERVER\n"));
  xprintf(_T("======================================================\n"));

  // Create HTTP site to listen to "https://+:1202/SecureClientCert/"
  // 
  HTTPSite* site = m_httpServer->CreateSite(PrefixType::URLPRE_Strong,true,TESTING_CLCERT_PORT,url);
  if(site)
  {
    // SUMMARY OF THE TEST
    //- --- "--------------------------- - ------\n"
    qprintf(_T("HTTPSite client certificate : OK : %s\n"),site->GetPrefixURL().GetString());
  }
  else
  {
    ++error;
    xerror();
    qprintf(_T("ERROR: Cannot make a HTTP site for: %s\n"),url.GetString());
    return error;
  }

  // Setting the POST handler for this site
  site->SetHandler(HTTPCommand::http_get,new SiteHandlerGetClientCert());

  // Standalone servers must filter on Client certificates
  if(p_standalone)
  {
    XString certName   = _T("MarlinClient");
    XString thumbprint = _T("8e02b7fe7d0e6a356d996664a542897fbae4d27e");

    // Add a site filter for the client certificate
    SiteFilterClientCertificate* filter = new SiteFilterClientCertificate(10,_T("ClientCert"));
    filter->SetClientCertificate(certName,thumbprint); // Here comes the name/thumbprint
    if(site->SetFilter(10,filter))
    {
      xprintf(_T("Site filter for Client-Certificates set correctly. Thumbprint: %s\n"),thumbprint.GetString());
    }
    else
    {
      qprintf(_T("ERROR SETTING SITE FILTER FOR ClientCertificates\n"));
    }
  }

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
TestMarlinServer::AfterTestClientCert()
{
  // SUMMARY OF THE TEST
  // ---- "---------------------------------------------- - ------
  qprintf(_T("Client certificate at file GET was tested      : %s"), totalChecks > 0 ? _T("ERROR") : _T("OK"));
  return totalChecks > 0;
}
