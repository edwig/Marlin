/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: TestClientCertificate.cpp
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
#include "SiteHandlerPut.h"
#include "SiteFilterClientCertificate.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

class SiteHandlerPutSecure : public SiteHandlerPut
{
protected:
  void PostHandle(HTTPMessage* p_message) override;
};

// BEWARE: in case of an incorrect client-certificate
// the site handlers will NEVER be called
// so we will never come to here!!
void
SiteHandlerPutSecure::PostHandle(HTTPMessage* p_message)
{
  SiteHandlerPut::PostHandle(p_message);
  bool result = p_message->GetStatus() == HTTP_STATUS_CREATED;

  // SUMMARY OF THE TEST
  // --- "--------------------------- - ------\n"
  printf("TEST CLIENT CERTIFICATE PUT : %s\n",result ? "OK" : "ERROR");
  if(!result) xerror();
}

int TestClientCertificate(HTTPServer* p_server)
{
  int error = 0;

  // If errors, change detail level
  doDetails = false;

  CString url("/MarlinTest/SecureClient/");

  xprintf("TESTING CLIENT CERTIFICATE FUNCTION OF THE HTTP SERVER\n");
  xprintf("======================================================\n");

  // Create HTTP site to listen to "https://+:443/MarlinTest/SecureClient/"
  // 
  HTTPSite* site = p_server->CreateSite(PrefixType::URLPRE_Strong,true,INTERNET_DEFAULT_HTTPS_PORT,url);
  if(site)
  {
    // SUMMARY OF THE TEST
    // --- "--------------------------- - ------\n"
    printf("HTTPSite client certificate : OK : %s\n",site->GetPrefixURL().GetString());
  }
  else
  {
    ++error;
    xerror();
    printf("ERROR: Cannot make a HTTP site for: %s\n",(LPCTSTR)url);
    return error;
  }

  // Setting the POST handler for this site
  site->SetHandler(HTTPCommand::http_put,new SiteHandlerPutSecure());

  CString certName   = "marlin";
  CString thumbprint = "db344064f2fac21318dd90f507fe78e81b031600";

  // Add a site filter for the client certificate
  SiteFilterClientCertificate* filter = new SiteFilterClientCertificate(10,"ClientCert");
  filter->SetClientCertificate(certName,thumbprint); // Here comes the name/thumbprint
  if(site->SetFilter(10,filter))
  {
    xprintf("Site filter for Client-Certificates set correctly. Thumbprint: %s\n",thumbprint);
  }
  else
  {
    printf("ERROR SETTING SITE FILTER FOR ClientCertificates\n");
  }

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
