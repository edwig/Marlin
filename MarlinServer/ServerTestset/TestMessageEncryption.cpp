/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: TestMessageEncryption.cpp
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
  CString message = p_message->GetBody();
  bool cypherResult = false;
  XMLMessage check;
  check.ParseMessage(message);
  XMLElement* cypher = check.FindElement("ds:CypherValue");
  if(cypher)
  {
    CString value = cypher->GetValue();
    if(value = "+YMEygqNvt7ZKFarTUa2qq6N9ZzFURb74TVbaAFwyKFkl75Tg0bU8pDPLDxYW1uBCzGJTqc4/xDounwbu"
               "8ksygubfcEGAX9q9UlT75E9Nk0qYqK7zys5zhwcd4W35DV3c5/hZf3kqXOx30TbJv8G/A8IF+JfGzXnQE"
               "c6ukBcrF2NXqkzfkThFZNq0slRvm37tMXWBWsP/FPh8lKqhUa7KdIMaQtnrdtXs7zkyBwnT+ab7XuJfUV"
               "8duz6QMA397Hhuly2GgkaERSP8IGSfMKewbfo7GKibRu3VRL96pna9qCPXx4toWDRUJN/TrTsIiWg1zW2"
               "v4YX7yYfPruo5oJlNbdKSXgQOhkJd0OcEbhYWzyyHsp2q342QhnJplkmjglP5pW6Yxuu51xfgNYTnGNto"
               "7wRUG9nOn6D2kgdGJUBu62a89yh7wrBwFdhy8U8kq45ldeifVi5UMlkYhK9YZNyONalGlWhrfU/PMZf8E"
               "tbs7/RRToICs6ydZAjZFNhs5z9dldbQbopMZOrf6tTBlFI1BaM+ByxHkWV8gxhhpEARsXNoaHAI93h7kv"
               "RlqGKRcgKthVbTVeRp8YQM4yx/zM//WYckz55VVAdWvzhniuiAB9VoOHzqL4txyGV94bt1rPBJROMilQz"
               "HmEghAzHceX9N/iiKw8L+NPiaGiAOuOi6fcGUL2K9UZjjlj+ECNH8H3/wRMGcQ3IZosPwVGKEK36GQVQr"
               "oZM5WL3Yg+hpy55UqvrEElNefpeedb2e7EVVa5HonYxPKK3NoEgBT5Bmh0s2trn5ObsWYvBEVQCYaDe0p"
               "UrQJkAhn5SAGRfAFMQMkBEmNUP+snhLPNxVnj9rZ1B6w1D/dRk3pLEpKPLuQFc22PVNdPjC/JBJcUWgfT"
               "aid2kxy6iLo2Dy3gKz/Wt0TW4Mphd8jr2iJ3GxWidSWltWM3/9mlINEr9ij9UJq5WTtd9bpDILgd1IveG"
               "cKemuMZN/UMOpdyPldE3U+XnZ9VGO1+Jgohm8SZcPSQH6JTAga65zqdRx1mNCM3sr0wo7/3RV4aHm7kyT"
               "uSE63cDF+x4CnpuAH3kljoQuJ7l+TsB7UW6GPusuzWviBhxkRj0iAwNdPaGcgTJ6zllb09huzNZ77m6So"
               "+b8A5YyfG1EwUE4+IAzPc91feJdjBoDZ4oRwyx42nSi2HQcebfAj8q5Amyc/avBxvoLELn+rjt1QL3IHU"
               "4tMLcbCt8K015FGIrOSAiRtpWNoujz6WljEzFVTeMGZmiPlIq8mldN34WtLOTEkcQK/i1WCxq0UTSWBJW"
               "6xFZ8z2xlzoj+yRbNVPZYzk/ucFr5gI+fYOaZXzCq26tCJOMrw66923JAqXgj9sVybxQUaSrUgONIM/0a"
               "eG/phQKDfg=" )
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
  xprintf("Incoming message in XML:\n%s\n", p_message->GetSoapMessage());

  // Get parameters from soap
  CString paramOne = p_message->GetParameter("One");
  CString paramTwo = p_message->GetParameter("Two");
  xprintf("Incoming parameter: %s = %s\n", "One", paramOne);
  xprintf("Incoming parameter: %s = %s\n", "Two", paramTwo);

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
    --totalChecks;
  }
  // SUMMARY OF THE TEST
  // --- "---------------------------------------------- - ------
  qprintf("Total encrypted message received contents      : %s\n", result ? "OK" : "ERROR");
  if(!result) xerror();

  p_message->SetParameter("Three", paramOne);
  p_message->SetParameter("Four",  paramTwo);
  xprintf("Outgoing parameter: %s = %s\n", "Three", paramOne);
  xprintf("Outgoing parameter: %s = %s\n", "Four",  paramTwo);

  return true;
}

int
TestMessageEncryption(HTTPServer* p_server)
{
  int error = 0;

  // If errors, change detail level
  doDetails = false;

  xprintf("TESTING MESSAGE ENCRYPTION OF A SOAP MESSAGE FUNCTIONS OF THE HTTP SITE\n");
  xprintf("=======================================================================\n");

  // Create URL channel to listen to "http://+:port/MarlinTest/MessageSigning/"
  // But WebConfig can override all values except for the callback function address
  CString url("/MarlinTest/MessageEncrypt/");
  HTTPSite* site = p_server->CreateSite(PrefixType::URLPRE_Strong,false,TESTING_HTTP_PORT,url);
  if (site)
  {
    // SUMMARY OF THE TEST
    // --- "--------------------------- - ------\n"
    qprintf("HTTPSite for full encryption: OK : %s\n",(LPCTSTR) site->GetPrefixURL());
  }
  else
  {
    ++error;
    xerror();
    qprintf("ERROR: Cannot create an HTTP site: %s\n",(LPCTSTR) url);
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
    qprintf("ERROR STARTING SITE: %s\n",(LPCTSTR) url);
  }
  return error;
}

int
AfterTestMessageEncryption()
{
  // SUMMARY OF THE TEST
  // ---- "---------------------------------------------- - ------
  qprintf("Total message encryption of SOAP messages      : %s\n", totalChecks > 0 ? "ERROR" : "OK");
  return totalChecks > 0;
}
