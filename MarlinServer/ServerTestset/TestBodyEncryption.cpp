/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: TestBodyEncryption.cpp
//
// Marlin Server: Internet server/client
// 
// Copyright (c) 2014-2022 ir. W.E. Huisman
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

class SiteHandlerSoapBodyEncrypt: public SiteHandlerSoap
{
public:
  bool  Handle(SOAPMessage* p_message);
protected:
  bool  Handle(HTTPMessage* p_message);
};

bool
SiteHandlerSoapBodyEncrypt::Handle(HTTPMessage* p_message)
{
  XString message = p_message->GetBody();
  bool cypherResult = false;
  XMLMessage check;
  check.ParseMessage(message);
  XMLElement* cypher = check.FindElement(_T("ds:CypherValue"));
  if(cypher)
  {
    XString value = cypher->GetValue();
    if(value == _T("VcPqnfbozsqeFEGghOImY2ZlxjgA1xxnLnqOjglV3VD+4xqmxOe3WMRxlobfYmOJ23S0aS3aE")
                _T("/KrVOR15pfQINTmeBqOPcwbntbh5baefeyOK0+THsHHONYt7LJg3jP08G5DGat0LjdVukrSno")
                _T("4Wnc2wTTizMr2EL7/1ACrZbrG8tOqkghwScksUQc08cXh9vXTcWkk8Mn+d+fhrONzmGUkvOU8")
                _T("bW5UV4y14+SC5nffAADFX8O9m70Wqr169L6HNjqo1BuiGIhDr03kB87ZTSpql60DmsiUfYXxl")
                _T("3TJM47Z+G6tW7S8tLnWElIBXLzkcaGD/FAx0+JFVVOKFVBXAwG8mJflwzu0bsAlJSTmueTc17")
                _T("fvf0ddbq8GWcEvys4MLRdCVeInzvV6TLw21LAFXQwUeEitypfM3zOfZhCTwqdxKvKoAx8ZRWq")
                _T("0e5qlk7F+s"))
    {
      cypherResult = true;
    }
  }
  // SUMMARY OF THE TEST
  // --- "---------------------------------------------- - ------
  qprintf(_T("Total body encryption for SOAP message         : %s\n"),cypherResult ? _T("OK") : _T("ERROR"));

  if(cypherResult)
  {
    --totalChecks;  
  }
  else
  {
    xerror();
  }

  // DO NOT FORGET TO CALL THE SOAP HANDLER!!
  // Now handle our message as a soap message
  return SiteHandlerSoap::Handle(p_message);
}

bool
SiteHandlerSoapBodyEncrypt::Handle(SOAPMessage* p_message)
{
  // Display incoming message
  xprintf(_T("Incoming message in XML:\n%s\n"), p_message->GetSoapMessage().GetString());

  // Get parameters from soap
  XString paramOne = p_message->GetParameter(_T("One"));
  XString paramTwo = p_message->GetParameter(_T("Two"));
  xprintf(_T("Incoming parameter: %s = %s\n"), _T("One"), paramOne.GetString());
  xprintf(_T("Incoming parameter: %s = %s\n"), _T("Two"), paramTwo.GetString());

  // reuse message for response
  XString response = _T("TestMessageResponse");

  p_message->Reset();
  p_message->SetSoapAction(response);

  // Do our work
  bool result = false;
  if (paramOne == _T("ABC") && paramTwo == _T("1-2-3"))
  {
    paramOne = _T("DEF");
    paramTwo = _T("123");
    result   = true;
  }
  // SUMMARY OF THE TEST
  // --- "---------------------------------------------- - ------
  qprintf(_T("SOAP Body encryption                           : %s\n"), result ? _T("OK") : _T("ERROR"));

  p_message->SetParameter(_T("Three"),paramOne);
  p_message->SetParameter(_T("Four"), paramTwo);
  xprintf(_T("Outgoing parameter: %s = %s\n"), _T("Three"),paramOne.GetString());
  xprintf(_T("Outgoing parameter: %s = %s\n"), _T("Four"), paramTwo.GetString());

  // Check done
  if(result) 
  {
    --totalChecks;
  }
  else
  {
    xerror();
  }
  return true;
}

int
TestMarlinServer::TestBodyEncryption()
{
  int error = 0;

  // If errors, change detail level
  m_doDetails = false;

  xprintf(_T("TESTING BODY ENCRYPTION OF A SOAP MESSAGE FUNCTIONS OF THE HTTP SITE\n"));
  xprintf(_T("====================================================================\n"));

  // Create URL channel to listen to "http://+:port/MarlinTest/BodyEncrypt/"
  // But WebConfig can override all values except for the callback function address
  XString url(_T("/MarlinTest/BodyEncrypt/"));
  HTTPSite* site = m_httpServer->CreateSite(PrefixType::URLPRE_Strong,false,m_inPortNumber,url);
  if (site)
  {
    // SUMMARY OF THE TEST
    // --- "--------------------------- - ------\n"
    qprintf(_T("HTTPSite for body encryption: OK : %s\n"),site->GetPrefixURL().GetString());
  }
  else
  {
    ++error;
    xerror();
    qprintf(_T("ERROR: Cannot create an HTTP site: %s\n"),url.GetString());
    return error;
  }

  site->SetHandler(HTTPCommand::http_post,new SiteHandlerSoapBodyEncrypt());

  // Modify the standard settings for this site
  site->AddContentType(_T(""),_T("text/xml"));
  site->AddContentType(_T("xml"),_T("application/soap+xml"));
  site->SetEncryptionLevel(XMLEncryption::XENC_Body);
  site->SetEncryptionPassword(_T("ForEverSweet16"));

  // new: Start the site explicitly
  if (site->StartSite())
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
TestMarlinServer::AfterTestBodyEncryption()
{
  // SUMMARY OF THE TEST
  // ---- "---------------------------------------------- - ------
  qprintf(_T("SOAP Body encryption tests                     : %s\n"),totalChecks > 0 ? _T("ERROR") : _T("OK"));
  return totalChecks > 0;
}
