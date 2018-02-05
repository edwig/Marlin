/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: TestBodyEncryption.cpp
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
#include "TestServer.h"
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
  CString message = p_message->GetBody();
  bool cypherResult = false;
  XMLMessage check;
  check.ParseMessage(message);
  XMLElement* cypher = check.FindElement("ds:CypherValue");
  if(cypher)
  {
    CString value = cypher->GetValue();
    if(value = "VcPqnfbozsqeFEGghOImY2ZlxjgA1xxnLnqOjglV3VD+4xqmxOe3WMRxlobfYmOJ23S0aS3aE"
               "/KrVOR15pfQINTmeBqOPcwbntbh5baefeyOK0+THsHHONYt7LJg3jP08G5DGat0LjdVukrSno"
               "4Wnc2wTTizMr2EL7/1ACrZbrG8tOqkghwScksUQc08cXh9vXTcWkk8Mn+d+fhrONzmGUkvOU8"
               "bW5UV4y14+SC5nffAADFX8O9m70Wqr169L6HNjqo1BuiGIhDr03kB87ZTSpql60DmsiUfYXxl"
               "3TJM47Z+G6tW7S8tLnWElIBXLzkcaGD/FAx0+JFVVOKFVBXAwG8mJflwzu0bsAlJSTmueTc17"
               "fvf0ddbq8GWcEvys4MLRdCVeInzvV6TLw21LAFXQwUeEitypfM3zOfZhCTwqdxKvKoAx8ZRWq"
               "0e5qlk7F+s")
    {
      cypherResult = true;
    }
  }
  // SUMMARY OF THE TEST
  // --- "---------------------------------------------- - ------
  qprintf("Total body encryption for SOAP message         : %s\n",cypherResult ? "OK" : "ERROR");

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
  xprintf("Incoming message in XML:\n%s\n", p_message->GetSoapMessage().GetString());

  // Get parameters from soap
  CString paramOne = p_message->GetParameter("One");
  CString paramTwo = p_message->GetParameter("Two");
  xprintf("Incoming parameter: %s = %s\n", "One", paramOne.GetString());
  xprintf("Incoming parameter: %s = %s\n", "Two", paramTwo.GetString());

  // reuse message for response
  CString response = "TestMessageResponse";

  p_message->Reset();
  p_message->SetSoapAction(response);

  // Do our work
  bool result = false;
  if (paramOne == "ABC" && paramTwo == "1-2-3")
  {
    paramOne = "DEF";
    paramTwo = "123";
    result   = true;
  }
  // SUMMARY OF THE TEST
  // --- "---------------------------------------------- - ------
  qprintf("SOAP Body encryption                           : %s\n", result ? "OK" : "ERROR");

  p_message->SetParameter("Three",paramOne);
  p_message->SetParameter("Four", paramTwo);
  xprintf("Outgoing parameter: %s = %s\n", "Three",paramOne.GetString());
  xprintf("Outgoing parameter: %s = %s\n", "Four", paramTwo.GetString());

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
TestBodyEncryption(HTTPServer* p_server)
{
  int error = 0;

  // If errors, change detail level
  doDetails = false;

  xprintf("TESTING BODY ENCRYPTION OF A SOAP MESSAGE FUNCTIONS OF THE HTTP SITE\n");
  xprintf("====================================================================\n");

  // Create URL channel to listen to "http://+:port/MarlinTest/BodyEncrypt/"
  // But WebConfig can override all values except for the callback function address
  CString url("/MarlinTest/BodyEncrypt/");
  HTTPSite* site = p_server->CreateSite(PrefixType::URLPRE_Strong,false,TESTING_HTTP_PORT,url);
  if (site)
  {
    // SUMMARY OF THE TEST
    // --- "--------------------------- - ------\n"
    qprintf("HTTPSite for body encryption: OK : %s\n",(LPCTSTR)site->GetPrefixURL());
  }
  else
  {
    ++error;
    xerror();
    qprintf("ERROR: Cannot create an HTTP site: %s\n",(LPCTSTR) url);
    return error;
  }

  site->SetHandler(HTTPCommand::http_post,new SiteHandlerSoapBodyEncrypt());

  // Modify the standard settings for this site
  site->AddContentType("","text/xml");
  site->AddContentType("xml","application/soap+xml");
  site->SetEncryptionLevel(XMLEncryption::XENC_Body);
  site->SetEncryptionPassword("ForEverSweet16");

  // new: Start the site explicitly
  if (site->StartSite())
  {
    xprintf("Site started correctly\n");
  }
  else
  {
    ++error;
    xerror();
    qprintf("ERROR STARTING SITE: %s\n",(LPCTSTR) url);
  }
  return error;
}

int 
AfterTestBodyEncryption()
{
  // SUMMARY OF THE TEST
  // ---- "---------------------------------------------- - ------
  qprintf("SOAP Body encryption tests                     : %s\n",totalChecks > 0 ? "ERROR" : "OK");
  return totalChecks > 0;
}
