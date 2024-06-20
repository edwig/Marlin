/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: TestBodySigning.cpp
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

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static int totalChecks = 2;

class SiteHandlerSoapBodySign: public SiteHandlerSoap
{
public:
  bool  Handle(SOAPMessage* p_message);
};

bool 
SiteHandlerSoapBodySign::Handle(SOAPMessage* p_message)
{
  // Display incoming message
  xprintf(_T("Incoming message in XML:\n%s\n"),p_message->GetSoapMessage().GetString());

  // Get parameters from soap
  XString paramOne = p_message->GetParameter(_T("One"));
  XString paramTwo = p_message->GetParameter(_T("Two"));
  xprintf(_T("Incoming parameter: %s = %s\n"),_T("One"),paramOne.GetString());
  xprintf(_T("Incoming parameter: %s = %s\n"),_T("Two"),paramTwo.GetString());

  // Test Body signing
  bool bodyResult = false;
  XMLElement* signature = p_message->FindElement(_T("ds:SignatureValue"));
  if(signature)
  {
    // BEWARE: Value is dependent on the message from Client:TestInsecure
    //         AND the body singing password in that file and this one!!
    XString sigValue = signature->GetValue();
    if(sigValue == _T("d37d76e087e7d8cd79deae39cc1e39b481bac420"))
    {
      bodyResult = true;
      --totalChecks;
    }
  }
  // SUMMARY OF THE TEST
  // --- "---------------------------------------------- - ------
  qprintf(_T("SOAP body signing value check                  : %s\n"), bodyResult ? _T("OK") : _T("ERROR"));

  if(!bodyResult) xerror();

  // reuse message for response
  XString response = _T("TestMessageResponse");

  p_message->Reset();
  p_message->SetSoapAction(response);

  // Do our work
  bool result = false;
  if(paramOne == _T("ABC") && paramTwo == _T("1-2-3"))
  {
    paramOne = _T("DEF");
    paramTwo = _T("123");
    result   = true;
    --totalChecks;
  }
  // SUMMARY OF THE TEST
  // --- "---------------------------------------------- - ------
  qprintf(_T("SOAP body signing message check                : %s\n"), result ? _T("OK") : _T("ERROR"));

  p_message->SetParameter(_T("Three"),paramOne);
  p_message->SetParameter(_T("Four"), paramTwo);
  xprintf(_T("Outgoing parameter: %s = %s\n"),_T("Three"),paramOne.GetString());
  xprintf(_T("Outgoing parameter: %s = %s\n"),_T("Four") ,paramTwo.GetString());

  if(!result) xerror();

  // TESTING ERROR REPORTING
  // char* test = NULL;
  // *test = 'a';

  // Continue processing
  return true;
}

int
TestMarlinServer::TestBodySigning()
{
  int error = 0;

  // If errors, change detail level
  m_doDetails = false;

  xprintf(_T("TESTING BODY SIGNING OF A SOAP MESSAGE FUNCTIONS OF THE HTTP SITE\n"));
  xprintf(_T("=================================================================\n"));

  // Create URL channel to listen to "http://+:port/MarlinTest/BodySigning/"
  // But WebConfig can override all values except for the callback function address
  XString url(_T("/MarlinTest/BodySigning/"));
  HTTPSite* site = m_httpServer->CreateSite(PrefixType::URLPRE_Strong,false,m_inPortNumber,url);
  if (site)
  {
    // SUMMARY OF THE TEST
    // --- "--------------------------- - ------\n"
    qprintf(_T("HTTPSite for body signing   : OK : %s\n"),site->GetPrefixURL().GetString());
  }
  else
  {
    ++error;
    xerror();
    qprintf(_T("ERROR: Cannot create an HTTP site: %s\n"),url.GetString());
    return error;
  }
  site->SetHandler(HTTPCommand::http_post,new SiteHandlerSoapBodySign());

  // Modify the standard settings for this site
  site->AddContentType(true,_T("pos"),_T("text/xml"));
  site->AddContentType(true,_T("xml"),_T("application/soap+xml"));
  site->SetEncryptionLevel(XMLEncryption::XENC_Signing);
  site->SetEncryptionPassword(_T("ForEverSweet16"));

  // new: Start the site explicitly
  if(site->StartSite())
  {
    xprintf(_T("Site started correctly\n"));
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
TestMarlinServer::AfterTestBodySigning()
{
  // SUMMARY OF THE TEST
  // ---- "---------------------------------------------- - ------
  qprintf(_T("SOAP Body signing test                         : %s\n"), totalChecks > 0 ? _T("ERROR") : _T("OK"));
  return totalChecks > 0;
}