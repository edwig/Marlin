/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: TestContract.cpp
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
#include "TestClient.h"
#include "WebServiceClient.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

bool 
SettingTheBaseLanguage(WebServiceClient& p_client,CString p_contract)
{
  CString command("MarlinFirst");
  CString language("Dutch");
  SOAPMessage msg1(p_contract,command);
  msg1.SetParameter("Language",language);
  msg1.SetParameter("Version",1);

  if(p_client.Send(&msg1))
  {
    if(msg1.GetParameterBoolean("Accepted") == true)
    {
      printf("Set base      language to: %s\n",(LPCTSTR)language);
      return true;
    }
    else
    {
      printf("ERROR: No parameter 'Accepted' or in wrong state.\n");
      printf(msg1.GetFault());
    }
  }
  else
  {
    printf(p_client.GetErrorText());
  }
  return false;
}

bool
SettingTheTranslateLanguage(WebServiceClient& p_client,CString p_contract)
{
  CString command("MarlinSecond");
  CString translate("Français");
  SOAPMessage msg2(p_contract,command);
  msg2.SetParameter("Translation",translate);

  if(p_client.Send(&msg2))
  {
    if(msg2.GetParameterBoolean("CanDo") == true)
    {
      printf("Set translate language to: %s\n",(LPCTSTR)translate);
      return true;
    }
    else
    {
      printf("ERROR: Cannot translate words!\n");
    }
  }
  else
  {
    printf(p_client.GetErrorText());
  }
  return false;
}

bool
Translate(WebServiceClient& p_client
         ,CString p_contract
         ,CString p_word
         ,CString p_expected)
{
  CString command("MarlinThird");
  SOAPMessage msg3(p_contract,command);
  msg3.SetParameter("WordToTranslate",p_word);

  if(p_client.Send(&msg3))
  {
    CString todayString = msg3.GetParameter("TranslatedWord");
    
    printf("TRANSLATED [%s] to [%s]\n",(LPCTSTR)p_word,(LPCTSTR)p_expected);

    if(todayString == p_expected)
    {
      return true;
    }
    else
    {
      printf("WORD translation is wrong!\n");
    }
  }
  else
  {
    printf(p_client.GetErrorText());
  }
  return false;
}

bool
TestWSDLDatatype(WebServiceClient& p_client,CString p_contract)
{
  CString command("MarlinFirst");
  CString language("Dutch");
  SOAPMessage msg1(p_contract,command);
  msg1.SetParameter("Language",language);
  msg1.SetParameter("Version","MyVersion");

  if(p_client.Send(&msg1))
  {
    if(msg1.GetParameterBoolean("Accepted") == true)
    {
      printf("Should not be accepted, because the parameter 'Version' contains an error!\n");
      return false;
    }
    else
    {
      xprintf(msg1.GetFault());
      if(msg1.GetFaultCode()   != "Datatype" ||
         msg1.GetFaultActor()  != "Client"   ||
         msg1.GetFaultString() != "Version"  ||
         msg1.GetFaultDetail() != "Datatype check failed: Not an integer, but: MyVersion")
      {
        return false;
      }
      return true;
    }
  }
  else
  {
    printf(p_client.GetErrorText());
  }
  return false;
}

int TestContract(HTTPClient* p_client,bool p_json)
{
  int errors = 4;
  extern CString logfileName;

  CString contract("http://interface.marlin.org/testing/");
  CString url ("http://" MARLIN_HOST ":1200/MarlinTest/TestInterface/");
  CString wsdl("http://" MARLIN_HOST ":1200/MarlinTest/TestInterface/MarlinWeb.wsdl");

  // Json is simultane on different site
  if(p_json)
  {
    url += "Extra/";
  }

  printf("TESTING THE WebServiceClient ON THE FOLLOWING CONTRACT:\n");
  printf("Contract: %s\n",(LPCTSTR)contract);
  printf("URL     : %s\n",(LPCTSTR)url);
  printf("WSDL    : %s\n",(LPCTSTR)wsdl);
  printf("---------------------------------------------------------\n");


  WebServiceClient client(contract,url,wsdl);

  if(p_client)
  {
    // Running in the context of an existing client
    client.SetHTTPClient(p_client);
    client.SetLogAnalysis(p_client->GetLogging());
    client.SetDetailLogging(p_client->GetDetailLogging());
  }
  else
  {
    // Running a stand-alone testset
    // Need a logfile before the call to "Open()"

    // Create log file and turn logging 'on'
    LogAnalysis log("TestHTTPClient");
    log.SetLogFilename(logfileName,true);
    log.SetDoLogging(true);
    log.SetDoTiming(true);
    log.SetDoEvents(false);
    log.SetCache(1000);

    client.SetLogAnalysis(&log);
    client.SetDetailLogging(true);
  }

  // Do we do JSON translation
  client.SetJsonSoapTranslation(p_json);

  try
  {
    client.Open();

    if(client.GetIsOpen())
    {
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

      // Test that WSDL gets our datatype check
      if(TestWSDLDatatype(client,contract)) 
      {
        --errors;
      }
    }
    client.Close();
  }
  catch(CString& error)
  {
    ++errors;
    printf("ERROR received      : %s\n",error.GetString());
    printf("ERROR from WS Client: %s\n",client.GetErrorText().GetString());
  }



  if(errors == 0)
  {
    printf("\nWORKS FINE!\n");
    printf("Also tested UTF-8 for the West-European languages!\n");
  }
  return errors;
}
