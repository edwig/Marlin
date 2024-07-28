/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: TestWS.cpp
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
#include "SOAPMessage.h"
#include "XMLMessage.h"
#include "TestClient.h"
#include "HTTPClient.h"
#include "WebServiceClient.h"
#include "GetLastErrorAsString.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

int
DoSend(HTTPClient& p_client,SOAPMessage* p_msg,const TCHAR* p_what,bool p_fault = false)
{
  bool result = false;

  // 1: CHECK SYNCHROON VERZONDEN. NORMAAL BERICHT
  if(p_client.Send(p_msg))
  {
    if(p_msg->GetFaultCode().IsEmpty())
    {
      xprintf(_T("Received: %s\n%s"),p_msg->GetSoapAction().GetString(),p_msg->GetSoapMessage().GetString());

      XString action  = p_msg->GetSoapAction();
      XString test    = p_msg->GetParameter(_T("Three"));
      int     number  = p_msg->GetParameterInteger(_T("Four"));
      XString testing = p_msg->GetParameter(_T("Testing"));

      xprintf(_T("Answer: %s : %s\n"),_T("Three"),test.GetString());
      xprintf(_T("Answer: %s : %d\n"),_T("Four"),number);

      // See if the answer is satisfying
      if(test == _T("DEF") || number == 123 && action == _T("TestMessageResponse") || testing == _T("OK"))
      {
        result = true;
      }
      // SUMMARY OF THE TEST
      // --- "---------------------------------------------- - ------
      _tprintf(_T("Send: SOAP Message %-27s : %s\n"),p_what,result ? _T("OK") : _T("ERROR"));
    }
    else 
    {
      if(p_fault)
      {
        if(p_msg->GetFaultCode()   == _T("FX1")    &&
           p_msg->GetFaultActor()  == _T("Server") &&
           p_msg->GetFaultString() == _T("Testing SOAP Fault") &&
           p_msg->GetFaultDetail() == _T("See if the SOAP Fault does work!"))
        {
          result = true;
        }
        // SUMMARY OF THE TEST
        // --- "---------------------------------------------- - ------
        _tprintf(_T("Send: SOAP Message %-27s : %s\n"),p_what,result ? _T("OK") : _T("ERROR"));
      }
      else
      {
        // But we where NOT expecting a FAULT!
        _tprintf(_T("Answer with error: %s\n"),p_msg->GetFault().GetString());

        _tprintf(_T("SOAP Fault code:   %s\n"),p_msg->GetFaultCode().GetString());
        _tprintf(_T("SOAP Fault actor:  %s\n"),p_msg->GetFaultActor().GetString());
        _tprintf(_T("SOAP Fault string: %s\n"),p_msg->GetFaultString().GetString());
        _tprintf(_T("SOAP FAULT detail: %s\n"),p_msg->GetFaultDetail().GetString());
      }
    }
  }
  else
  {
    // Possibly a SOAP fault as answer
    if(!p_msg->GetFaultCode().IsEmpty())
    {
      _tprintf(_T("SOAP Fault: %s\n"),p_msg->GetFault().GetString());
    }
    else
    {
      // Raw HTTP error
      BYTE* response = NULL;
      unsigned length = 0;
      p_client.GetResponse(response, length);
      _tprintf(_T("Message not sent!\n"));
      _tprintf(_T("Service answer: %s\n"),reinterpret_cast<TCHAR*>(response));
      _tprintf(_T("HTTP Client error: %s\n"), p_client.GetStatusText().GetString());
    }
  }
  // ready with the message
  delete p_msg;

  return result ? 0 : 1;
}

int
DoSendPrice(HTTPClient& p_client, SOAPMessage* p_msg,double p_price)
{
  bool result = false;

  // 1: CHECK SYNCHROON VERZONDEN. NORMAAL BERICHT
  if (p_client.Send(p_msg))
  {
    if(p_msg->GetFaultCode().IsEmpty())
    {
      double priceInclusive = p_msg->GetParameterDouble(_T("PriceInclusive"));
      if(priceInclusive - (p_price * 1.21)  < 0.005) // Dutch VAT percentage
      {
        result = true;
      }
    }
    else
    {
      _tprintf(_T("Answer with error: %s\n"), p_msg->GetFault().GetString());

      _tprintf(_T("SOAP Fault code:   %s\n"), p_msg->GetFaultCode().GetString());
      _tprintf(_T("SOAP Fault actor:  %s\n"), p_msg->GetFaultActor().GetString());
      _tprintf(_T("SOAP Fault string: %s\n"), p_msg->GetFaultString().GetString());
      _tprintf(_T("SOAP FAULT detail: %s\n"), p_msg->GetFaultDetail().GetString());
    }
  }
  else
  {
    // Possibly a SOAP fault as answer
    if(!p_msg->GetFaultCode().IsEmpty())
    {
      _tprintf(_T("SOAP Fault: %s\n"), p_msg->GetFault().GetString());
    }
    else
    {
      // Raw HTTP error
      BYTE* response = NULL;
      unsigned length = 0;
      p_client.GetResponse(response, length);
      _tprintf(_T("Message not sent!\n"));
      _tprintf(_T("Service answer: %s\n"),reinterpret_cast<TCHAR*>(response));
      _tprintf(_T("HTTP Client error: %s\n"), p_client.GetStatusText().GetString());
    }
  }
  // SUMMARY OF THE TEST
  // --- "---------------------------------------------- - ------
  _tprintf(_T("Send: SOAP datatype double calculation         : %s\n"), result ? _T("OK") : _T("ERROR"));

  // ready with the message
  delete p_msg;

  return (result == true) ? 0 : 1;
}

SOAPMessage*
CreateSoapMessage(XString       p_namespace
                 ,XString       p_command
                 ,XString       p_url
                 ,SoapVersion   p_version = SoapVersion::SOAP_12
                 ,XMLEncryption p_encrypt = XMLEncryption::XENC_Plain)
{
  // Build message
  SOAPMessage* msg = new SOAPMessage(p_namespace,p_command,p_version,p_url);

  msg->SetParameter(_T("One"),_T("ABC"));
  msg->SetParameter(_T("Two"),_T("1-2-3")); // "17" will stop the server
  XMLElement* param = msg->SetParameter(_T("Units"),_T(""));
  XMLElement* enh1 = msg->AddElement(param,_T("Unit"),XDT_String,_T(""));
  XMLElement* enh2 = msg->AddElement(param,_T("Unit"),XDT_String,_T(""));
  msg->SetElement(enh1,_T("Unitnumber"),12345);
  msg->SetElement(enh2,_T("Unitnumber"),67890);
  msg->SetAttribute(enh1,_T("Independent"),true);
  msg->SetAttribute(enh2,_T("Independent"),false);

  if(p_encrypt != XMLEncryption::XENC_Plain)
  {
    msg->SetSecurityLevel(p_encrypt);
    msg->SetSecurityPassword(_T("ForEverSweet16"));

    if(p_encrypt == XMLEncryption::XENC_Body)
    {
      msg->SetSigningMethod(CALG_RSA_SIGN);
    }
  }
  return msg;
}

SOAPMessage*
CreateSoapPriceMessage(XString p_namespace
                      ,XString p_command
                      ,XString p_url
                      ,double  p_price)
{
  SOAPMessage* msg = new SOAPMessage(p_namespace,p_command,SoapVersion::SOAP_12,p_url);
  msg->SetParameter(_T("Price"), p_price);
  return msg;
}

int
TestReliableMessaging(HTTPClient* p_client,XString p_namespace,XString p_action,XString p_url,bool p_tokenProfile)
{
  int errors = 0;
  int totalDone = 0;
  WebServiceClient client(p_namespace,p_url,_T(""),true);
  client.SetHTTPClient(p_client);
  client.SetLogAnalysis(p_client->GetLogging());
  
  // testing soap compression
  // client.SetSoapCompress(true);

  // Testing with WS-Security Token profile
  if(p_tokenProfile)
  {
    client.SetUser(_T("marlin"));
    client.SetPassword(_T("M@rl!nS3cr3t"));
    client.SetTokenProfile(true);
  }

  SOAPMessage* message = nullptr;

  try
  {
    // Client must be opened for RM protocol
    client.Open();

    for(int ind = 0; ind < NUM_RM_TESTS; ++ind)
    {
      // Make a copy of the message
      message = CreateSoapMessage(p_namespace,p_action,p_url);

      // Forced that it is not set to reliable. It doesn't have to, becouse the client.Send() will do that
      // message->SetReliability(true);
      xprintf(_T("Sending RM message: %s\n"),message->GetSoapMessage().GetString());

      if(client.Send(message))
      {
        xprintf(_T("Received: %s\n%s"),message->GetSoapAction().GetString(),message->GetSoapMessage().GetString());

        XString action = message->GetSoapAction();
        XString test   = message->GetParameter(_T("Three"));
        int     number = message->GetParameterInteger(_T("Four"));

        xprintf(_T("Answer: %s : %s\n"),_T("Three"),test.GetString());
        xprintf(_T("Answer: %s : %d\n"),_T("Four "),number);

        bool result = false;
        if(test == _T("DEF") || number == 123 && action == _T("TestMessageResponse"))
        {
          ++totalDone;
          result = true;
        }
        else
        {
          _tprintf(_T("Answer with fault: %s\n"),message->GetFault().GetString());
          result = false;
        }
        // SUMMARY OF THE TEST
        // --- "---------------------------------------------- - ------
        _tprintf(_T("Send: SOAP WS-ReliableMessaging                : %s\n"),result ? _T("OK") : _T("ERROR"));
        errors += result ? 0 : 1;
      }
      else
      {
        ++errors;
        _tprintf(_T("Message **NOT** correctly sent!\n"));

        _tprintf(_T("Error code WebServiceClient: %s\n"),client.GetErrorText().GetString());
        _tprintf(_T("Return message error:\n%s\n"),message->GetFault().GetString());
      }
      // Ready with the message
      if(message)
      {
        delete message;
        message = nullptr;
      }
    }
    // Must be closed to complete RM protocol
    client.Close();
  }
  catch(StdException& error)
  {
    ++errors;
    _tprintf(_T("ERROR received      : %s\n"),error.GetErrorMessage().GetString());
    _tprintf(_T("ERROR from WS Client: %s\n"),client.GetErrorText().GetString());
  }
  if(message)
  {
    delete message;
    message = nullptr;
  }
  if(errors)
  {
    return errors;
  }
  return (totalDone == NUM_RM_TESTS) ? 0 : 1;
}

int
DoSendByQueue(HTTPClient& p_client,XString p_namespace,XString p_action,XString p_url)
{
  int times = 20;
  XString name(_T("TestNumber"));

  for(int x = 1; x <= times; ++x)
  {
    SOAPMessage* message = CreateSoapMessage(p_namespace,p_action,p_url);
    message->SetParameter(name,x);
    p_client.AddToQueue(message);
  }
  // --- "---------------------------------------------- - ------
  _tprintf(_T("Send: [%d] messages added to the sending queue : OK\n"),times);

  return 0;
}

int
DoSendAsyncQueue(HTTPClient& p_client,XString p_namespace,XString p_url)
{
  int times = 10;
  XString resetMess(_T("MarlinReset"));
  XString textMess (_T("MarlinText"));
  
  SOAPMessage* reset1 = new SOAPMessage(p_namespace,resetMess,SoapVersion::SOAP_12,p_url);
  SOAPMessage* reset2 = new SOAPMessage(p_namespace,resetMess,SoapVersion::SOAP_12,p_url);
  reset1->SetParameter(_T("DoReset"),true);
  reset2->SetParameter(_T("DoReset"),true);

  p_client.AddToQueue(reset1);
  for(int ind = 0;ind < times; ++ind)
  {
    SOAPMessage* msg = new SOAPMessage(p_namespace,textMess,SoapVersion::SOAP_12,p_url);
    msg->SetParameter(_T("Text"),_T("This is a testing message to see if async SOAP messages are delivered."));
    p_client.AddToQueue(msg);
  }
  p_client.AddToQueue(reset2);

  // --- "---------------------------------------------- - ------
  _tprintf(_T("Send: Asynchronous messages added to the queue : OK\n"));

  return 0;
}

inline XString 
CreateURL(XString p_extra)
{
  XString url;
  url.Format(_T("http://%s:%d/MarlinTest/%s"),MARLIN_HOST,TESTING_HTTP_PORT,p_extra.GetString());
  return url;
}

int TestWebservices(HTTPClient& client)
{
  int errors = 0;
  extern XString logfileName;
  SOAPMessage* msg = nullptr;

  // Testing cookie function
  errors += TestCookies(client);

  // Standard values for messages
  XString namesp(_T("http://interface.marlin.org/services"));
  XString command(_T("TestMessage"));
  XString url(CreateURL(_T("Insecure")));

  // Test 1
  xprintf(_T("TESTING STANDARD SOAP MESSAGE TO /MarlinTest/Insecure/\n"));
  xprintf(_T("====================================================\n"));
  msg = CreateSoapMessage(namesp,command,url);
  errors += DoSend(client,msg,_T("insecure"));

  // Test 2
  xprintf(_T("TESTING BODY SIGNING SOAP TO /MarlinTest/BodySigning/\n"));
  xprintf(_T("===================================================\n"));
  url = CreateURL(_T("BodySigning"));
  msg = CreateSoapMessage(namesp,command,url,SoapVersion::SOAP_12, XMLEncryption::XENC_Signing);
  errors += DoSend(client,msg,_T("body signing"));

  // Test 3
  xprintf(_T("TESTING BODY ENCRYPTION SOAP TO /MarlinTest/BodyEncrypt/\n"));
  xprintf(_T("======================================================\n"));
  url = CreateURL(_T("BodyEncrypt"));
  msg = CreateSoapMessage(namesp,command,url, SoapVersion::SOAP_12, XMLEncryption::XENC_Body);
  errors += DoSend(client,msg,_T("body encrypting"));

  // Test 4
  xprintf(_T("TESTING WHOLE MESSAGE ENCRYPTION TO /MarlinTest/MessageEncrypt/\n"));
  xprintf(_T("=============================================================\n"));
  url = CreateURL(_T("MessageEncrypt"));
  msg = CreateSoapMessage(namesp,command,url,SoapVersion::SOAP_12,XMLEncryption::XENC_Message);
  errors += DoSend(client,msg,_T("message encrypting"));

  // Test 5
  xprintf(_T("TESTING RELIABLE MESSAGING TO /MarlinTest/Reliable/\n"));
  xprintf(_T("=================================================\n"));
  url = CreateURL(_T("Reliable"));
  errors += TestReliableMessaging(&client,namesp,command,url,false);

  // Test 6
  xprintf(_T("TESTING RELIABLE MESSAGING TO /MarlinTest/ReliableBA/ with WS-Security token profile\n"));
  xprintf(_T("=================================================\n"));
  url = CreateURL(_T("ReliableBA"));
  errors += TestReliableMessaging(&client,namesp,command,url,true);

  // Test 7
  xprintf(_T("TESTING THE TOKEN FUNCTION TO /MarlinTest/TestToken/\n"));
  xprintf(_T("====================================================\n"));
  url = CreateURL(_T("TestToken"));
  msg = CreateSoapMessage(namesp,command,url);
  client.SetSingleSignOn(true);
  XString user(_T("CERT6\\Beheerder"));
  XString password(_T("altijd"));
  client.SetUser(user);
  client.SetPassword(password);
  errors += DoSend(client,msg,_T("token testing"));
  client.SetSingleSignOn(false);

  // Test 8
  xprintf(_T("TESTING THE SUB-SITES FUNCTION TO /MarlinTest/TestToken/One/\n"));
  xprintf(_T("TESTING THE SUB-SITES FUNCTION TO /MarlinTest/TestToken/Two/\n"));
  xprintf(_T("============================================================\n"));
  XString url1 = CreateURL(_T("TestToken/One"));
  XString url2 = CreateURL(_T("TestToken/Two"));
  msg = CreateSoapMessage(namesp,command,url1);
  client.SetSingleSignOn(true);
  client.SetUser(user);
  client.SetPassword(password);
  errors += DoSend(client,msg,_T("single sign on"));
  msg = CreateSoapMessage(namesp,command,url2);
  errors += DoSend(client,msg,_T("single sign on"));
  client.SetSingleSignOn(false);

  // Test 9
  xprintf(_T("TESTING SOAP FAULT TO /MarlinTest/Insecure/\n"));
  xprintf(_T("=================================================\n"));
  url = CreateURL(_T("Insecure"));
  msg = CreateSoapMessage(namesp,command,url);
  msg->SetParameter(_T("TestFault"),true);
  errors += DoSend(client,msg,_T("soap fault"),true);

  // Test 10
  xprintf(_T("TESTING UNICODE SENDING TO /MarlinTest/Insecure/\n"));
  xprintf(_T("================================================\n"));
  url = CreateURL(_T("Insecure"));
  msg = CreateSoapMessage(namesp,command,url);
  msg->SetEncoding(Encoding::LE_UTF16);
  msg->SetContentType("application/soap+xml; charset=\"utf-16\"");

  errors += DoSend(client,msg,_T("sending unicode"));

  // Test 11
  xprintf(_T("TESTING FILTERING CAPABILITIES TO /MarlinTest/Filter/\n"));
  xprintf(_T("=====================================================\n"));
  url = CreateURL(_T("Filter"));
  msg = CreateSoapPriceMessage(namesp,command,url,456.78);
  errors += DoSendPrice(client,msg,456.78);
    
  // Test 12
  xprintf(_T("TESTING HIGH SPEED QUEUE TO /MarlinTest/Insecure/\n"));
  xprintf(_T("=================================================\n"));
  url = CreateURL(_T("Insecure"));
  errors += DoSendByQueue(client,namesp,command,url);

  // Test 13
  xprintf(_T("TESTING ASYNCHRONEOUS SOAP MESSAGES TO /MarlinTest/Asynchrone/\n"));
  xprintf(_T("==============================================================\n"));
  url = CreateURL(_T("Asynchrone"));
  errors += DoSendAsyncQueue(client,namesp,url);

  // Waiting for the queue to drain
  _tprintf(_T("\nWait for last test to complete.\n"));
  while(client.GetQueueSize())
  {
    Sleep(200);
  }

  _tprintf(_T("\n**READY**\n\nType a word and ENTER\n"));

  // Wait for key to occur
  // so the messages can be send and debugged :-)
  WaitForKey();

  _tprintf(_T("Stopping the client\n"));

  client.StopClient();

  _tprintf(_T("The client is %s\n"),client.GetIsRunning() ? _T("still running!\n") : _T("stopped.\n"));

  // Any error found
  XString boodschap;
  int error = client.GetError(&boodschap);
  if(error)
  {
    XString systeemText = GetLastErrorAsString(error);
    _tprintf(_T("ERROR! Code: %d = %s\nErrortext: %s\n"),error,systeemText.GetString(),boodschap.GetString());
  }
  return errors;
}
