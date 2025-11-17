/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: TestContract.cpp
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
#include "TestClient.h"
#include "WebServiceClient.h"
#include <XmlDataType.h>
#include <SOAPMessage.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

bool 
SettingTheBaseLanguage(WebServiceClient& p_client,XString p_contract)
{
  XString command(_T("MarlinFirst"));
  XString language(_T("Dutch"));
  SOAPMessage msg1(p_contract,command);
  msg1.SetParameter(_T("Language"),language);
  msg1.SetParameter(_T("Version"),1);

  if(p_client.Send(&msg1))
  {
    if(msg1.GetParameterBoolean(_T("Accepted")) == true)
    {
      // --- "---------------------------------------------- - ------
      _tprintf(_T("Service contract: accepting base language      : OK\n"));
      xprintf(_T("Base language is: %s\n"),language.GetString());
      return true;
    }
    else
    {
      // --- "---------------------------------------------- - ------
      _tprintf(_T("Service contract: accepted language wrong state: ERROR\n"));
      xprintf(msg1.GetFault());
    }
  }
  else
  {
    // --- "---------------------------------------------- - ------
    _tprintf(_T("Service contract: Language message not sent!   : ERROR\n"));
    xprintf(p_client.GetErrorText());
  }
  return false;
}

bool
SettingTheTranslateLanguage(WebServiceClient& p_client,XString p_contract)
{
  XString command(_T("MarlinSecond"));
  XString translate(_T("Français"));
  SOAPMessage msg2(p_contract,command);
  msg2.SetParameter(_T("Translation"),translate);

  if(p_client.Send(&msg2))
  {
    if(msg2.GetParameterBoolean(_T("CanDo")) == true)
    {
      // --- "---------------------------------------------- - ------
      _tprintf(_T("Service contract: set 'translate to' language  : OK\n"));
      xprintf(_T("%s\n"),translate.GetString());
      return true;
    }
    else
    {
      // --- "---------------------------------------------- - ------
      _tprintf(_T("Service contract: cannot translate words!      : ERROR\n"));
    }
  }
  else
  {
    // --- "---------------------------------------------- - ------
    _tprintf(_T("Serivce contract: 'translate to' not sent!     : ERROR\n"));
    _tprintf(p_client.GetErrorText());
  }
  return false;
}

bool
Translate(WebServiceClient& p_client
         ,XString p_contract
         ,XString p_word
         ,XString p_expected)
{
  XString command(_T("MarlinThird"));
  SOAPMessage msg3(p_contract,command);
  msg3.SetParameter(_T("WordToTranslate"),p_word);

  if(p_client.Send(&msg3))
  {
    XString todayString = msg3.GetParameter(_T("TranslatedWord"));
    
    // --- "---------------------------------------------- - ------
    xprintf(_T("TRANSLATED [%s] to [%s]\n"),p_word.GetString(),p_expected.GetString());

    if(todayString == p_expected)
    {
      // --- "---------------------------------------------- - ------
      _tprintf(_T("Service contract: correct translation          : OK\n"));
      return true;
    }
    else
    {
      // --- "---------------------------------------------- - ------
      _tprintf(_T("Service contract: translation is wrong!        : ERROR\n"));
    }
  }
  else
  {
    // --- "---------------------------------------------- - ------
    _tprintf(_T("Service contract: translation not sent!        : ERROR\n"));
    _tprintf(p_client.GetErrorText());
  }
  return false;
}

bool
TestWSDLDatatype(WebServiceClient& p_client,XString p_contract)
{
  XString command(_T("MarlinFirst"));
  XString language(_T("Dutch"));
  SOAPMessage msg1(p_contract,command);
  msg1.SetParameter(_T("Language"),language);
  msg1.SetParameter(_T("Version"),_T("MyVersion"));
  bool error = true;

  if(p_client.Send(&msg1))
  {
    if(msg1.GetParameterBoolean(_T("Accepted")) == true)
    {
      // --- "---------------------------------------------- - ------
      xprintf(_T("Should not be accepted, because the parameter 'Version' contains an error!\n"));
    }
    else
    {
      xprintf(msg1.GetFault());
      if(msg1.GetFaultCode()   == _T("Datatype")    &&
         msg1.GetFaultActor()  == _T("Client")      &&
         msg1.GetFaultString() == _T("Restriction") &&
         msg1.GetFaultDetail() == _T("Datatype check failed! Field: Version Value: MyVersion Result: Not an integer, but: MyVersion"))
      {
        error = false;
      }
    }
  }
  else
  {
    _tprintf(p_client.GetErrorText());
  }
  // --- "---------------------------------------------- - ------
  _tprintf(_T("Service contract: Testing WSDL datatypes       : %s\n"), error ? _T("ERROR") : _T("OK"));
  return error;
}

bool
TestWSDLFloating(WebServiceClient& p_client,XString p_contract)
{
  XString command(_T("MarlinFifth"));
  XString pi = _T("3.141592653589793");
  SOAPMessage msg(p_contract,command);
  XMLElement* param = msg.SetParameter(_T("Parameters"),_T(""));
  msg.AddElement(param,_T("Dialect"), _T("Math"));
  msg.AddElement(param,_T("Region"),  _T("Europe"));
  msg.AddElement(param,_T("PiApprox"),pi,XmlDataType::XDT_Double);
  XMLElement* data = msg.AddElement(param,_T("DataTypes"),_T(""),XmlDataType::XDT_Complex);
  msg.AddElement(data,_T("MinLength"),  _T("4"), XmlDataType::XDT_Integer);
  msg.AddElement(data,_T("MaxLength"),  _T("20"),XmlDataType::XDT_Integer);
  msg.AddElement(data,_T("MaxDecimals"),_T("16"),XmlDataType::XDT_Integer);

  XMLElement* dword = msg.SetParameter(_T("DoubleWord"),_T(""));
  msg.AddElement(dword,_T("Word"),_T("PI"));

  bool error = true;

  if(p_client.Send(&msg))
  {
    if(msg.GetParameter(_T("TranslatedWord")) == _T("pi"))
    {
      error = false;
    }
  }
  // --- "---------------------------------------------- - ------
  _tprintf(_T("Service contract: Testing WSDL floating point 1: %s\n"),error ? _T("ERROR") : _T("OK"));
  return error;
}

bool
TestWSDLFloatingWrong(WebServiceClient& p_client,XString p_contract)
{
  XString command(_T("MarlinFifth"));
  XString pi = _T("3,141592653589793"); // THIS IS THE WRONG NUMBER !!
  SOAPMessage msg(p_contract,command);
  XMLElement* param = msg.SetParameter(_T("Parameters"),_T(""));
  msg.AddElement(param,_T("Dialect"), _T("Math"));
  msg.AddElement(param,_T("Region"),  _T("Europe"));
  msg.AddElement(param,_T("PiApprox"),pi,XmlDataType::XDT_Double);
  XMLElement* data = msg.AddElement(param,_T("DataTypes"),_T(""),XmlDataType::XDT_Complex);
  msg.AddElement(data,_T("MinLength"),  _T("4"), XmlDataType::XDT_Integer);
  msg.AddElement(data,_T("MaxLength"),  _T("20"),XmlDataType::XDT_Integer);
  msg.AddElement(data,_T("MaxDecimals"),_T("16"),XmlDataType::XDT_Integer);

  XMLElement* dword = msg.SetParameter(_T("DoubleWord"),_T(""));
  msg.AddElement(dword,_T("Word"),_T("PI"));

  bool error = true;

  if(p_client.Send(&msg))
  {
    // MUST GET A SOAP FAULT with this detailed information
    if(msg.GetFaultDetail().Find(_T("Datatype check failed! Field: PiApprox")) >= 0)
    {
      error = false;
    }
  }
  // --- "---------------------------------------------- - ------
  _tprintf(_T("Service contract: Testing WSDL floating point 2: %s\n"),error ? _T("ERROR") : _T("OK"));
  return error;
}



int TestContract(HTTPClient* p_client,bool p_json,bool p_tokenProfile)
{
  int errors = 6;
  XString logfileName = MarlinConfig::GetExePath() + _T("ClientLog.txt");

  XString url;
  XString wsdl;
  XString contract(_T("http://interface.marlin.org/testing/"));

//  url .Format("http://%s:%d/MarlinTest/TestInterface/",              MARLIN_HOST,TESTING_HTTP_PORT);
//  wsdl.Format("http://%s:%d/MarlinTest/TestInterface/MarlinWeb.wsdl",MARLIN_HOST,TESTING_HTTP_PORT);
  url .Format(_T("http://%s:%d/MarlinTest/"),                 MARLIN_HOST,TESTING_HTTP_PORT);
  wsdl.Format(_T("http://%s:%d/MarlinTest/MarlinServer.wsdl"),MARLIN_HOST,TESTING_HTTP_PORT);
  // JSON is simultaneous on different site
  if(p_json)
  {
    url += _T("Extra/");
  }

  xprintf(_T("TESTING THE WebServiceClient ON THE FOLLOWING CONTRACT:\n"));
  xprintf(_T("Contract: %s\n"),contract.GetString());
  xprintf(_T("URL     : %s\n"),url.GetString());
  xprintf(_T("WSDL    : %s\n"),wsdl.GetString());
  xprintf(_T("---------------------------------------------------------\n"));


  WebServiceClient client(contract,url,wsdl);
  LogAnalysis* log = nullptr;

  if(p_tokenProfile)
  {
    client.SetUser(_T("marlin"));
    client.SetPassword(_T("M@rl!nS3cr3t"));
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
    log = LogAnalysis::CreateLogfile(_T("TestHTTPClient"));
    log->SetLogFilename(logfileName,true);
    log->SetLogLevel(HLL_LOGGING);
    log->SetDoTiming(true);
    log->SetDoEvents(false);
    log->SetCache(1000);

    client.SetLogAnalysis(log);
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
          if(Translate(client,contract,_T("altijd"), _T("toujours"))) --errors;
          if(Translate(client,contract,_T("maandag"),_T("lundi")   )) --errors;
          if(Translate(client,contract,_T("dinsdag"),_T("")        )) --errors;
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
    _tprintf(_T("Service contract: errors received              : ERROR\n"));
    xprintf(_T("%s\n"),er.GetErrorMessage().GetString());
    xprintf(_T("ERROR from WS Client: %s\n"),client.GetErrorText().GetString());
  }

  if(errors == 0)
  {
    // --- "---------------------------------------------- - ------
    _tprintf(_T("Service contract: works fine!                  : OK\n"));
    _tprintf(_T("tested UTF-8 for the West-European languages!  : OK\n"));
  }
  if(log)
  {
    LogAnalysis::DeleteLogfile(log);
  }
  return errors;
}
