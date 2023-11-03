/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: TestMessageEncryption.cpp
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
    //          "---------------------------------------------------------------------------------"
    if(value == _T("+YMEygqNvt7ZKFarTUa2qq6N9ZzFURb74TVbaAFwyKFkl75Tg0bU8pDPLDxYW1uBCzGJTqc4/xDounwbu")
                _T("8ksygubfcEGAX9q9UlT75E9Nk0qYqK7zys5zhwcd4W35DV3c5/hZf3kqXOx30TbJv8G/A8IF+JfGzXnQE")
                _T("c6ukBcrF2NXqkzfkThFZNq0slRvm37tMXWBWsP/FPh8lKqhUa7KdIMaQtnrdtXs7zkyBwnT+ab7XuJfUV")
                _T("8duz6QMA397Hhuly2GgkaERSP8IGSfMKewbfo7GKibRu3VRL96pna9qCPXx4toWDRUJN/TrTsIiWg1zW2")
                _T("v4YX7yYfPruo5oJlNbdKSXgQOhkJd0OcEbhYWzyyHsp2q342QhnJplkmjglP5pW6Yxuu51xfgNYTnGNto")
                _T("7wRUG9nOn6D2kgdGJUBu62a89yh7wrBwFdhy8U8kq45ldeifVi5UMlkYhK9YZNyONalGlWhrfU/PMZf8E")
                _T("tbs7/RRToICs6ydZAjZFNhs5z9dldbQbopMZOrf6tTBlFI1N7YqPBR3rjhR4y0LB8riwERfjgLdnDKISd")
                _T("1odGUZHAT0viHbVgn/plgGZQlW4V/ZVELUrK0ETw49CHolOGCvukC9qxkUx3r0CiomxA97QPNwCOnb+ti")
                _T("9jhJO8+bmIZCXjHnMjj/Cva/X34zCOkaoYcBdNYD75hdW4VbS7mzKWvk6j3F6j1E7wIfYJwT8fu8GHKRe")
                _T("8yGqJ0wlHKwnSrt7LdxBkYovWB7+sBP06YCKDtNsNCeWoT5HINzM4khuiRUAGXUp6tp13ac1lDoQueCHW")
                _T("6QuPZpfd8sZMu7nZH1tvdWRMsT6OArXZ0IJ4qQj73ATbIAY0X8kKSoqOmjc4xI0uNQUM37OXJ/feYnI1a")
                _T("HArPNeFfBP1PkMZp8Rb2sgeo5CH+pg5Eix2QIgohTyBeE5vtNuDYbSZ/yvS9NDQHRAkjRCJvuMsMAEJ3A")
                _T("0vEV95ruA/B9eMebIaXr/UDiSRR5+xFPBjj+fLpqKcYavMCZxR+BSCpOKqbCZyPyEkn3UA8cvK4hjxLQl")
                _T("bhUMLiCNrKryY8FoIo7AwgoWvVRidlBEjm/Iby2ZovIVv69SntNoaiiKBFL7K6X/MoOKpWm92zPXjWaqW")
                _T("yUkPwJtDUWzUz0vbzd9C/Yue67bU0tfdh5I4ERonEwOGRWQrtZBGTFrfSLR4ROvIiwevQNVUDV8oCH+dd")
                _T("S4dN9ZIDbSna7h4RyR+p2CcLJ5SVgR/GvWFrltkWXgoLccJfMTYAAvdwa+k7GHMJ/Deq/NHRYxSYzD7c0")
                _T("kYzfOMye0fWrrGRgj/dY6wEu6PYMVnG2wl5Ff1/3LOkNY5HAyVmVEeE5gmeUwO5IB+l1PyGKI6CJdAs+r")
                _T("TCVse2zgeo="))
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
  HTTPSite* site = m_httpServer->CreateSite(PrefixType::URLPRE_Strong,false,m_inPortNumber,url);
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
  site->AddContentType(_T(""),_T("text/xml"));
  site->AddContentType(_T("xml"),_T("application/soap+xml"));
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
