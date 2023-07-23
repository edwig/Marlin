/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: TestContract.cpp
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
#include "TestClient.h"
#include "WebServiceClient.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

bool 
SettingTheBaseLanguage(WebServiceClient& p_client,XString p_contract)
{
  XString command("MarlinFirst");
  XString language("Dutch");
  SOAPMessage msg1(p_contract,command);
  msg1.SetParameter("Language",language);
  msg1.SetParameter("Version",1);

  if(p_client.Send(&msg1))
  {
    if(msg1.GetParameterBoolean("Accepted") == true)
    {
      // --- "---------------------------------------------- - ------
      printf("Service contract: accepting base language      : OK\n");
      xprintf("Base language is: %s\n",language.GetString());
      return true;
    }
    else
    {
      // --- "---------------------------------------------- - ------
      printf("Service contract: accepted language wrong state: ERROR\n");
      xprintf(msg1.GetFault());
    }
  }
  else
  {
    // --- "---------------------------------------------- - ------
    printf("Service contract: Language message not sent!   : ERROR\n");
    xprintf(p_client.GetErrorText());
  }
  return false;
}

bool
SettingTheTranslateLanguage(WebServiceClient& p_client,XString p_contract)
{
  XString command("MarlinSecond");
  XString translate("Fran�ais");
  SOAPMessage msg2(p_contract,command);
  msg2.SetParameter("Translation",translate);

  if(p_client.Send(&msg2))
  {
    if(msg2.GetParameterBoolean("CanDo") == true)
    {
      // --- "---------------------------------------------- - ------
      printf("Service contract: set 'translate to' language  : OK\n");
      xprintf("%s\n",translate.GetString());
      return true;
    }
    else
    {
      // --- "---------------------------------------------- - ------
      printf("Service contract: cannot translate words!      : ERROR\n");
    }
  }
  else
  {
    // --- "---------------------------------------------- - ------
    printf("Serivce contract: 'translate to' not sent!     : ERROR\n");
    printf(p_client.GetErrorText());
  }
  return false;
}

bool
Translate(WebServiceClient& p_client
         ,XString p_contract
         ,XString p_word
         ,XString p_expected)
{
  XString command("MarlinThird");
  SOAPMessage msg3(p_contract,command);
  msg3.SetParameter("WordToTranslate",p_word);

  if(p_client.Send(&msg3))
  {
    XString todayString = msg3.GetParameter("TranslatedWord");
    
    // --- "---------------------------------------------- - ------
    xprintf("TRANSLATED [%s] to [%s]\n",p_word.GetString(),p_expected.GetString());

    if(todayString == p_expected)
    {
      // --- "---------------------------------------------- - ------
      printf("Service contract: correct translation          : OK\n");
      return true;
    }
    else
    {
      // --- "---------------------------------------------- - ------
      printf("Service contract: translation is wrong!        : ERROR\n");
    }
  }
  else
  {
    // --- "---------------------------------------------- - ------
    printf("Service contract: translation not sent!        : ERROR\n");
    printf(p_client.GetErrorText());
  }
  return false;
}

bool
TestWSDLDatatype(WebServiceClient& p_client,XString p_contract)
{
  XString command("MarlinFirst");
  XString language("Dutch");
  SOAPMessage msg1(p_contract,command);
  msg1.SetParameter("Language",language);
  msg1.SetParameter("Version","MyVersion");
  bool error = true;

  if(p_client.Send(&msg1))
  {
    if(msg1.GetParameterBoolean("Accepted") == true)
    {
      // --- "---------------------------------------------- - ------
      xprintf("Should not be accepted, because the parameter 'Version' contains an error!\n");
    }
    else
    {
      xprintf(msg1.GetFault());
      if(msg1.GetFaultCode()   == "Datatype"    &&
         msg1.GetFaultActor()  == "Client"      &&
         msg1.GetFaultString() == "Restriction" &&
         msg1.GetFaultDetail() == "Datatype check failed! Field: Version Value: MyVersion Result: Not an integer, but: MyVersion")
      {
        error = false;
      }
    }
  }
  else
  {
    printf(p_client.GetErrorText());
  }
  // --- "---------------------------------------------- - ------
  printf("Service contract: Testing WSDL datatypes       : %s\n", error ? "ERROR" : "OK");
  return error;
}

bool
TestWSDLFloating(WebServiceClient& p_client,XString p_contract)
{
  XString command("MarlinFifth");
  XString pi = "3.141592653589793";
  SOAPMessage msg(p_contract,command);
  XMLElement* param = msg.SetParameter("Parameters","");
  msg.AddElement(param,"Dialect", XDT_String,"Math");
  msg.AddElement(param,"Region",  XDT_String,"Europe");
  msg.AddElement(param,"PiApprox",XDT_Double,pi);
  XMLElement* data = msg.AddElement(param,"DataTypes",XDT_Complex,"");
  msg.AddElement(data,"MinLength",  XDT_Integer, "4");
  msg.AddElement(data,"MaxLength",  XDT_Integer,"20");
  msg.AddElement(data,"MaxDecimals",XDT_Integer,"16");

  XMLElement* dword = msg.SetParameter("DoubleWord","");
  msg.AddElement(dword,"Word",XDT_String,"PI");

  bool error = true;

  if(p_client.Send(&msg))
  {
    if(msg.GetParameter("TranslatedWord") == "pi")
    {
      error = false;
    }
  }
  // --- "---------------------------------------------- - ------
  printf("Service contract: Testing WSDL floating point 1: %s\n",error ? "ERROR" : "OK");
  return error;
}

bool
TestWSDLFloatingWrong(WebServiceClient& p_client,XString p_contract)
{
  XString command("MarlinFifth");
  XString pi = "3,141592653589793"; // THIS IS THE WRONG NUMBER !!
  SOAPMessage msg(p_contract,command);
  XMLElement* param = msg.SetParameter("Parameters","");
  msg.AddElement(param,"Dialect", XDT_String,"Math");
  msg.AddElement(param,"Region",  XDT_String,"Europe");
  msg.AddElement(param,"PiApprox",XDT_Double,pi);
  XMLElement* data = msg.AddElement(param,"DataTypes",XDT_Complex,"");
  msg.AddElement(data,"MinLength",  XDT_Integer,"4");
  msg.AddElement(data,"MaxLength",  XDT_Integer,"20");
  msg.AddElement(data,"MaxDecimals",XDT_Integer,"16");

  XMLElement* dword = msg.SetParameter("DoubleWord","");
  msg.AddElement(dword,"Word",XDT_String,"PI");

  bool error = true;

  if(p_client.Send(&msg))
  {
    // MUST GET A SOAP FAULT with this detailed information
    if(msg.GetFaultDetail().Find("Datatype check failed! Field: PiApprox") >= 0)
    {
      error = false;
    }
  }
  // --- "---------------------------------------------- - ------
  printf("Service contract: Testing WSDL floating point 2: %s\n",error ? "ERROR" : "OK");
  return error;
}



int TestContract(HTTPClient* p_client,bool p_json,bool p_tokenProfile)
{
  int errors = 6;
  XString logfileName = MarlinConfig::GetExePath() + "ClientLog.txt";

  XString url;
  XString wsdl;
  XString contract("http://interface.marlin.org/testing/");

//  url .Format("http://%s:%d/MarlinTest/TestInterface/",              MARLIN_HOST,TESTING_HTTP_PORT);
//  wsdl.Format("http://%s:%d/MarlinTest/TestInterface/MarlinWeb.wsdl",MARLIN_HOST,TESTING_HTTP_PORT);
  url .Format("http://%s:%d/MarlinTest/",                 MARLIN_HOST,TESTING_HTTP_PORT);
  wsdl.Format("http://%s:%d/MarlinTest/MarlinServer.wsdl",MARLIN_HOST,TESTING_HTTP_PORT);
  // JSON is simultaneous on different site
  if(p_json)
  {
    url += "Extra/";
  }

  xprintf("TESTING THE WebServiceClient ON THE FOLLOWING CONTRACT:\n");
  xprintf("Contract: %s\n",contract.GetString());
  xprintf("URL     : %s\n",url.GetString());
  xprintf("WSDL    : %s\n",wsdl.GetString());
  xprintf("---------------------------------------------------------\n");


  WebServiceClient client(contract,url,wsdl);

  if(p_tokenProfile)
  {
    client.SetUser("marlin");
    client.SetPassword("M@rl!nS3cr3t");
    client.SetTokenProfile(true);
  }

  if(p_client)
  {
    // Running in the context of an existing client
    client.SetHTTPClient(p_client);
    client.SetLogAnalysis(p_client->GetLogging());
    client.SetLogLevel(p_client->GetLogLevel());
  }
  else
  {
    // Running a stand-alone test set
    // Need a logfile before the call to "Open()"

    // Create log file and turn logging 'on'
    LogAnalysis log("TestHTTPClient");
    log.SetLogFilename(logfileName,true);
    log.SetLogLevel(HLL_LOGGING);
    log.SetDoTiming(true);
    log.SetDoEvents(false);
    log.SetCache(1000);

    client.SetLogAnalysis(&log);
    client.SetLogLevel(HLL_LOGGING);
  }

  // Do we do JSON translation
  client.SetJsonSoapTranslation(p_json);

  try
  {
    if(client.Open())
    {
      // Test that WSDL gets our datatype check
      if(TestWSDLDatatype(client,contract) == false)
      {
        --errors;
      }

      if(SettingTheBaseLanguage(client,contract))
      {
        if(SettingTheTranslateLanguage(client,contract))
        {
          // Now translate Dutch -> French
          if(Translate(client,contract,"altijd", "toujours")) --errors;
          if(Translate(client,contract,"maandag","lundi"   )) --errors;
          if(Translate(client,contract,"dinsdag",""        )) --errors;
        }
      }

      if(p_json)
      {
        errors -= 2;
      }
      else
      {
        // Test floating point correctly sent
        if(TestWSDLFloating(client,contract) == false)
        {
          --errors;
        }
        // Test with a wrong floating point
        if(TestWSDLFloatingWrong(client,contract) == false)
        {
          --errors;
        }
      }
    }
    client.Close();
  }
  catch(StdException& er)
  {
    ++errors;
    // --- "---------------------------------------------- - ------
    printf("Service contract: errors received              : ERROR\n");
    xprintf("%s\n",er.GetErrorMessage().GetString());
    xprintf("ERROR from WS Client: %s\n",client.GetErrorText().GetString());
  }

  if(errors == 0)
  {
    // --- "---------------------------------------------- - ------
    printf("Service contract: works fine!                  : OK\n");
    printf("tested UTF-8 for the West-European languages!  : OK\n");
  }
  return errors;
}
