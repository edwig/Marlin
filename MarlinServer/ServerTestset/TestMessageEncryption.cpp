/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: TestMessageEncryption.cpp
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
#include <HTTPSite.h>
#include <SiteHandlerSoap.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static int totalChecks = 2;

class SiteHandlerSoapMsgEncrypt: public SiteHandlerSoap
{
public:
  bool  Handle(HTTPMessage* p_message);
  bool  Handle(SOAPMessage* p_message);
};

bool
SiteHandlerSoapMsgEncrypt::Handle(HTTPMessage* p_message)
{
  XString message = p_message->GetBody();
  bool cypherResult = false;
  XMLMessage check;
  check.ParseMessage(message);
  XMLElement* cypher = check.FindElement(_T("ds:CypherValue"));
  if(cypher)
  {
    XString value = cypher->GetValue();
    //             "---------------------------------------------------------------------------------"
    if(value == _T("+YMEygqNvt7ZKFarTUa2qkpueD3JJLP0rdRBWb0xm7O4JY+eBH4pfM3leSFUEXh4cPTAxtBryXiQv8Kyj")
                _T("U+hkhtW4IfgE1sAs1tMq0/XIday70nS033qWNGOsj2clIMXlYNbdbHhQwaxVokBUC5gGN2yM6cwhMUnVM")
                _T("bLPlLDu6ND8lZLAkz12RF5fuYqZG03gyKJkubTcRyo4hfNu9/gmnrvCvh9Lo8O5eOJV4ZRSlWkN9WP27A")
                _T("VN7vZOhAfFHPMiLHtI1XQpxtW7s9icjbmTCZqWCFUOlQ8G0boKVkd7fza8zHkLdohnOmAIcKMepwg8mjN")
                _T("wVNQ8LnHpU9ncKMMJgxq2BL5+CK9GtPHNo91oiZE9t+NE5OhxY1njklZ9qMt88/KqwwvzgeMHW7SDYyKR")
                _T("YpbsrQWqg5JUVvJu49Vb0A2sh1tHEPxEutnBDhV6moGwwcEwDO17QlGernxhsQUaa9IW69ihsFr4UITES")
                _T("Az2Qc+lPg6+yLgaVvKOtNNyZ/BfV1HBM/tDrUNaFbDz40P6vPUo8V30dCaXhHM4MyFyWMAVLCHx3I3xKt")
                _T("N3aT9Xmhyx8KBuqI6OLkc8WJpA8xl4zgMIeUvy40Ue9l31aAi9eZebWcLUX8sxyx0WVIU09/b6GuS+KCK")
                _T("hOhr0yEUUKI7Fz0SGz5VcyeLLN/GLaxajWNhHa4u0fx6IWsltUmSqqmo8dqfA2V+VTUWEUkC55xmTRSDf")
                _T("3lojHZDf0sM2aSeMl1zwoQJX0dHfdwCHvJJe5p0XXJpEY2gmC2kgk8cv90eF5cwjUgQj6xT1tAzhC9+fa")
                _T("bJ0gJ8eHdzy6AjXd5i5vhCvvfP++IAXywXzk3HIKHpV5YnCMnAZ7BWFP1KkjQWYG0QcX6dGK+OgNTU0pc")
                _T("A8K2al6/KmiP8H+ikaLrOTvJPO6007nHMZg8P8mx1wfl21A4nVAnxXm3CnzPg1ZHtpd9JBKA1/zcQ7fYG")
                _T("B/bkWAKOUZWueNkmUb4FnnbnopNbUnTf7WRIAlakzpi4ePPZZlSkyiI5FHdGqiIE5WdbIXF6Ql9iF+wYJ")
                _T("KHEqyeAqtmfKJ8YoKpQ+wAX79zJ3jhMxvAObHGjpdyOl53rY3h5SGV5yR2qp0cH7YYGqNcqt+YuCKCpkx")
                _T("8xD26HEfczO8cSUqVK0damPSgSOuaWDkEdp91B82+sgqLQgzAcF8iItrITdU63wBiuaCRuCt8KzGet2+n")
                _T("b0z1urOK+ZCWt37Tc3QAsQYvHbu6jgRHTOEfENvMH6viZnqdJ4SnuxvYBqYUugLS4H+aJcWTZTFDssip4")
                _T("Vt6eJ2NVLKmPGRTbm7Qef4l8ZjASX5BWhjvrxhAJTrOOQ7+tJk6cfXXWbkEFR5iOlw+XLGZ//SDv9WWeh")
                _T("HWpbbvaNh8="))
    {
      cypherResult = true;
      --totalChecks;
    }
  }
  // SUMMARY OF THE TEST
  // --- "---------------------------------------------- - ------
  qprintf(_T("Total message encryption check                 : %s\n"),cypherResult ? _T("OK") : _T("ERROR"));
  if(!cypherResult) xerror();

  // DO NOT FORGET TO CALL THE SOAP HANDLER!!
  // Now handle our message as a soap message
  return SiteHandlerSoap::Handle(p_message);
}

bool
SiteHandlerSoapMsgEncrypt::Handle(SOAPMessage* p_message)
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
    --totalChecks;
  }
  // SUMMARY OF THE TEST
  // --- "---------------------------------------------- - ------
  qprintf(_T("Total encrypted message received contents      : %s\n"), result ? _T("OK") : _T("ERROR"));
  if(!result)
  {
    xerror();
  }
  p_message->SetParameter(_T("Three"), paramOne);
  p_message->SetParameter(_T("Four"),  paramTwo);
  xprintf(_T("Outgoing parameter: %s = %s\n"), _T("Three"), paramOne.GetString());
  xprintf(_T("Outgoing parameter: %s = %s\n"), _T("Four"),  paramTwo.GetString());

  return true;
}

int
TestMarlinServer::TestMessageEncryption()
{
  int error = 0;

  // If errors, change detail level
  m_doDetails = false;

  xprintf(_T("TESTING MESSAGE ENCRYPTION OF A SOAP MESSAGE FUNCTIONS OF THE HTTP SITE\n"));
  xprintf(_T("=======================================================================\n"));

  // Create URL channel to listen to "http://+:port/MarlinTest/MessageSigning/"
  // But WebConfig can override all values except for the callback function address
  XString url(_T("/MarlinTest/MessageEncrypt/"));
  HTTPSite* site = m_httpServer->CreateSite(PrefixType::URLPRE_Strong,false,TESTING_HTTP_PORT,url,true);
  if (site)
  {
    // SUMMARY OF THE TEST
    // --- "--------------------------- - ------\n"
    qprintf(_T("HTTPSite for full encryption: OK : %s\n"),site->GetPrefixURL().GetString());
  }
  else
  {
    ++error;
    xerror();
    qprintf(_T("ERROR: Cannot create an HTTP site: %s\n"),url.GetString());
    return error;
  }

  site->SetHandler(HTTPCommand::http_post,new SiteHandlerSoapMsgEncrypt());

  // Modify the standard settings for this site
  site->AddContentType(true,_T("pos"),_T("text/xml"));
  site->AddContentType(true,_T("xml"),_T("application/soap+xml"));
  site->SetEncryptionLevel(XMLEncryption::XENC_Message);
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
TestMarlinServer::AfterTestMessageEncryption()
{
  // SUMMARY OF THE TEST
  // ---- "---------------------------------------------- - ------
  qprintf(_T("Total message encryption of SOAP messages      : %s\n"), totalChecks > 0 ? _T("ERROR") : _T("OK"));
  return totalChecks > 0;
}
