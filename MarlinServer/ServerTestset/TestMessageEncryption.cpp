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
  XMLElement* cypher = check.FindElement("ds:CypherValue");
  if(cypher)
  {
    XString value = cypher->GetValue();
    //          "---------------------------------------------------------------------------------"
    if(value == "+YMEygqNvt7ZKFarTUa2qq6N9ZzFURb74TVbaAFwyKFkl75Tg0bU8pDPLDxYW1uBCzGJTqc4/xDounwbu"
                "8ksygubfcEGAX9q9UlT75E9Nk0qYqK7zys5zhwcd4W35DV3c5/hZf3kqXOx30TbJv8G/A8IF+JfGzXnQE"
                "c6ukBcrF2NXqkzfkThFZNq0slRvm37tMXWBWsP/FPh8lKqhUa7KdIMaQtnrdtXs7zkyBwnT+ab7XuJfUV"
                "8duz6QMA397Hhuly2GgkaERSP8IGSfMKewbfo7GKibRu3VRL96pna9qCPXx4toWDRUJN/TrTsIiWg1zW2"
                "v4YX7yYfPruo5oJlNbdKSXgQOhkJd0OcEbhYWzyyHsp2q342QhnJplkmjglP5pW6Yxuu51xfgNYTnGNto"
                "7wRUG9nOn6D2kgdGJUBu62a89yh7wrBwFdhy8U8kq45ldeifVi5UMlkYhK9YZNyONalGlWhrfU/PMZf8E"
                "tbs7/RRToICs6ydZAjZFNhs5z9dldbQbopMZOrf6tTBlFI1N7YqPBR3rjhR4y0LB8riwERfjgLdnDKISd"
                "1odGUZHAT0viHbVgn/plgGZQlW4V/ZVELUrK0ETw49CHolOGCvukC9qxkUx3r0CiomxA97QPNwCOnb+ti"
                "9jhJO8+bmIZCXjHnMjj/Cva/X34zCOkaoYcBdNYD75hdW4VbS7mzKWvk6j3F6j1E7wIfYJwT8fu8GHKRe"
                "8yGqJ0wlHKwnSrt7LdxBkYovWB7+sBP06YCKDtNsNCeWoT5HINzM4khuiRUAGXUp6tp13ac1lDoQueCHW"
                "6QuPZpfd8sZMu7nZH1tvdWRMsT6OArXZ0IJ4qQj73ATbIAY0X8kKSoqOmjc4xI0uNQUM37OXJ/feYnI1a"
                "HArPNeFfBP1PkMZp8Rb2sgeo5CH+pg5Eix2QIgohTyBeE5vtNuDYbSZ/yvS9NDQHRAkjRCJvuMsMAEJ3A"
                "0vEV95ruA/B9eMebIaXr/UDiSRR5+xFPBjj+fLpqKcYavMCZxR+BSCpOKqbCZyPyEkn3UA8cvK4hjxLQl"
                "bhUMLiCNrKryY8FoIo7AwgoWvVRidlBEjm/Iby2ZovIVv69SntNoaiiKBFL7K6X/MoOKpWm92zPXjWaqW"
                "yUkPwJtDUWzUz0vbzd9C/Yue67bU0tfdh5I4ERonEwOGRWQrtZBGTFrfSLR4ROvIiwevQNVUDV8oCH+dd"
                "S4dN9ZIDbSna7h4RyR+p2CcLJ5SVgR/GvWFrltkWXgoLccJfMTYAAvdwa+k7GHMJ/Deq/NHRYxSYzD7c0"
                "kYzfOMye0fWrrGRgj/dY6wEu6PYMVnG2wl5Ff1/3LOkNY5HAyVmVEeE5gmeUwO5IB+l1PyGKI6CJdAs+r"
                "TCVse2zgeo=")
    {
      cypherResult = true;
      --totalChecks;
    }
  }
  // SUMMARY OF THE TEST
  // --- "---------------------------------------------- - ------
  qprintf("Total message encryption check                 : %s\n",cypherResult ? "OK" : "ERROR");
  if(!cypherResult) xerror();

  // DO NOT FORGET TO CALL THE SOAP HANDLER!!
  // Now handle our message as a soap message
  return SiteHandlerSoap::Handle(p_message);
}

bool
SiteHandlerSoapMsgEncrypt::Handle(SOAPMessage* p_message)
{
  // Display incoming message
  xprintf("Incoming message in XML:\n%s\n", p_message->GetSoapMessage().GetString());

  // Get parameters from soap
  XString paramOne = p_message->GetParameter("One");
  XString paramTwo = p_message->GetParameter("Two");
  xprintf("Incoming parameter: %s = %s\n", "One", paramOne.GetString());
  xprintf("Incoming parameter: %s = %s\n", "Two", paramTwo.GetString());

  // reuse message for response
  XString response = "TestMessageResponse";

  p_message->Reset();
  p_message->SetSoapAction(response);

  // Do our work
  bool result = false;
  if (paramOne == "ABC" && paramTwo == "1-2-3")
  {
    paramOne = "DEF";
    paramTwo = "123";
    result   = true;
    --totalChecks;
  }
  // SUMMARY OF THE TEST
  // --- "---------------------------------------------- - ------
  qprintf("Total encrypted message received contents      : %s\n", result ? "OK" : "ERROR");
  if(!result)
  {
    xerror();
  }
  p_message->SetParameter("Three", paramOne);
  p_message->SetParameter("Four",  paramTwo);
  xprintf("Outgoing parameter: %s = %s\n", "Three", paramOne.GetString());
  xprintf("Outgoing parameter: %s = %s\n", "Four",  paramTwo.GetString());

  return true;
}

int
TestMarlinServer::TestMessageEncryption()
{
  int error = 0;

  // If errors, change detail level
  m_doDetails = false;

  xprintf("TESTING MESSAGE ENCRYPTION OF A SOAP MESSAGE FUNCTIONS OF THE HTTP SITE\n");
  xprintf("=======================================================================\n");

  // Create URL channel to listen to "http://+:port/MarlinTest/MessageSigning/"
  // But WebConfig can override all values except for the callback function address
  XString url("/MarlinTest/MessageEncrypt/");
  HTTPSite* site = m_httpServer->CreateSite(PrefixType::URLPRE_Strong,false,m_inPortNumber,url);
  if (site)
  {
    // SUMMARY OF THE TEST
    // --- "--------------------------- - ------\n"
    qprintf("HTTPSite for full encryption: OK : %s\n",site->GetPrefixURL().GetString());
  }
  else
  {
    ++error;
    xerror();
    qprintf("ERROR: Cannot create an HTTP site: %s\n",url.GetString());
    return error;
  }

  site->SetHandler(HTTPCommand::http_post,new SiteHandlerSoapMsgEncrypt());

  // Modify the standard settings for this site
  site->AddContentType("","text/xml");
  site->AddContentType("xml","application/soap+xml");
  site->SetEncryptionLevel(XMLEncryption::XENC_Message);
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
    qprintf("ERROR STARTING SITE: %s\n",url.GetString());
  }
  return error;
}

int
TestMarlinServer::AfterTestMessageEncryption()
{
  // SUMMARY OF THE TEST
  // ---- "---------------------------------------------- - ------
  qprintf("Total message encryption of SOAP messages      : %s\n", totalChecks > 0 ? "ERROR" : "OK");
  return totalChecks > 0;
}
