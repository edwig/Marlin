/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: TestBodySigning.cpp
//
// Marlin Server: Internet server/client
// 
// Copyright (c) 2015-2018 ir. W.E. Huisman
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
  xprintf("Incoming message in XML:\n%s\n",(LPCTSTR) p_message->GetSoapMessage());

  // Get parameters from soap
  CString paramOne = p_message->GetParameter("One");
  CString paramTwo = p_message->GetParameter("Two");
  xprintf("Incoming parameter: %s = %s\n","One",paramOne.GetString());
  xprintf("Incoming parameter: %s = %s\n","Two",paramTwo.GetString());

  // Test Body signing
  bool bodyResult = false;
  XMLElement* signature = p_message->FindElement("ds:SignatureValue");
  if(signature)
  {
    // BEWARE: Value is dependent on the message from Client:TestInsecure
    //         AND the body singing password in that file and this one!!
    CString sigValue = signature->GetValue();
    if(sigValue == "cr4Eci/L8w+riKG+hK/v2kuIJGo=")
    {
      bodyResult = true;
      --totalChecks;
    }
  }
  // SUMMARY OF THE TEST
  // --- "---------------------------------------------- - ------
  qprintf("SOAP body signing value check                  : %s\n", bodyResult ? "OK" : "ERROR");

  if(!bodyResult) xerror();

  // reuse message for response
  CString response = "TestMessageResponse";

  p_message->Reset();
  p_message->SetSoapAction(response);

  // Do our work
  bool result = false;
  if(paramOne == "ABC" && paramTwo == "1-2-3")
  {
    paramOne = "DEF";
    paramTwo = "123";
    result   = true;
    --totalChecks;
  }
  // SUMMARY OF THE TEST
  // --- "---------------------------------------------- - ------
  qprintf("SOAP body signing message check                : %s\n", result ? "OK" : "ERROR");

  p_message->SetParameter("Three",paramOne);
  p_message->SetParameter("Four", paramTwo);
  xprintf("Outgoing parameter: %s = %s\n","Three",paramOne.GetString());
  xprintf("Outgoing parameter: %s = %s\n","Four" ,paramTwo.GetString());

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

  xprintf("TESTING BODY SIGNING OF A SOAP MESSAGE FUNCTIONS OF THE HTTP SITE\n");
  xprintf("=================================================================\n");

  // Create URL channel to listen to "http://+:port/MarlinTest/BodySigning/"
  // But WebConfig can override all values except for the callback function address
  CString url("/MarlinTest/BodySigning/");
  HTTPSite* site = m_httpServer->CreateSite(PrefixType::URLPRE_Strong,false,m_inPortNumber,url);
  if (site)
  {
    // SUMMARY OF THE TEST
    // --- "--------------------------- - ------\n"
    qprintf("HTTPSite for body signing   : OK : %s\n",(LPCTSTR)site->GetPrefixURL());
  }
  else
  {
    ++error;
    xerror();
    qprintf("ERROR: Cannot create an HTTP site: %s\n",(LPCTSTR)url);
    return error;
  }
  site->SetHandler(HTTPCommand::http_post,new SiteHandlerSoapBodySign());

  // Modify the standard settings for this site
  site->AddContentType("","text/xml");
  site->AddContentType("xml","application/soap+xml");
  site->SetEncryptionLevel(XMLEncryption::XENC_Signing);
  site->SetEncryptionPassword("ForEverSweet16");

  // new: Start the site explicitly
  if(site->StartSite())
  {
    xprintf("Site started correctly\n");
  }
  else
  {
    ++error;
    xerror();
    qprintf("ERROR STARTING SITE: %s\n",(LPCTSTR)url);
  }
  return error;
}

int 
TestMarlinServer::AfterTestBodySigning()
{
  // SUMMARY OF THE TEST
  // ---- "---------------------------------------------- - ------
  qprintf("SOAP Body signing test                         : %s\n", totalChecks > 0 ? "ERROR" : "OK");
  return totalChecks > 0;
}