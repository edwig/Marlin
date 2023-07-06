/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: TestWS.cpp
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
DoSend(HTTPClient& p_client,SOAPMessage* p_msg,char* p_what,bool p_fault = false)
{
  bool result = false;

  // 1: CHECK SYNCHROON VERZONDEN. NORMAAL BERICHT
  if(p_client.Send(p_msg))
  {
    if(p_msg->GetFaultCode().IsEmpty())
    {
      xprintf("Received: %s\n%s",p_msg->GetSoapAction().GetString(),p_msg->GetSoapMessage().GetString());

      XString action  = p_msg->GetSoapAction();
      XString test    = p_msg->GetParameter("Three");
      int     number  = p_msg->GetParameterInteger("Four");
      XString testing = p_msg->GetParameter("Testing");

      xprintf("Answer: %s : %s\n","Three",test.GetString());
      xprintf("Answer: %s : %d\n","Four",number);

      // See if the answer is satisfying
      if(test == "DEF" || number == 123 && action == "TestMessageResponse" || testing == "OK")
      {
        result = true;
      }
      // SUMMARY OF THE TEST
      // --- "---------------------------------------------- - ------
      printf("Send: SOAP Message %-27s : %s\n",p_what,result ? "OK" : "ERROR");
    }
    else 
    {
      if(p_fault)
      {
        if(p_msg->GetFaultCode()   == "FX1"    &&
           p_msg->GetFaultActor()  == "Server" &&
           p_msg->GetFaultString() == "Testing SOAP Fault" &&
           p_msg->GetFaultDetail() == "See if the SOAP Fault does work!")
        {
          result = true;
        }
        // SUMMARY OF THE TEST
        // --- "---------------------------------------------- - ------
        printf("Send: SOAP Message %-27s : %s\n",p_what,result ? "OK" : "ERROR");
      }
      else
      {
        // But we where NOT expecting a FAULT!
        printf("Answer with error: %s\n",(LPCTSTR)p_msg->GetFault());

        printf("SOAP Fault code:   %s\n",(LPCTSTR)p_msg->GetFaultCode());
        printf("SOAP Fault actor:  %s\n",(LPCTSTR)p_msg->GetFaultActor());
        printf("SOAP Fault string: %s\n",(LPCTSTR)p_msg->GetFaultString());
        printf("SOAP FAULT detail: %s\n",(LPCTSTR)p_msg->GetFaultDetail());
      }
    }
  }
  else
  {
    // Possibly a SOAP fault as answer
    if(!p_msg->GetFaultCode().IsEmpty())
    {
      printf("SOAP Fault: %s\n",p_msg->GetFault().GetString());
    }
    else
    {
      // Raw HTTP error
      BYTE* response = NULL;
      unsigned length = 0;
      p_client.GetResponse(response, length);
      printf("Message not sent!\n");
      printf("Service answer: %s\n", (char*)response);
      printf("HTTP Client error: %s\n", p_client.GetStatusText().GetString());
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
      double priceInclusive = p_msg->GetParameterDouble("PriceInclusive");
      if(priceInclusive - (p_price * 1.21)  < 0.005) // Dutch VAT percentage
      {
        result = true;
      }
    }
    else
    {
      printf("Answer with error: %s\n", (LPCTSTR)p_msg->GetFault());

      printf("SOAP Fault code:   %s\n", (LPCTSTR)p_msg->GetFaultCode());
      printf("SOAP Fault actor:  %s\n", (LPCTSTR)p_msg->GetFaultActor());
      printf("SOAP Fault string: %s\n", (LPCTSTR)p_msg->GetFaultString());
      printf("SOAP FAULT detail: %s\n", (LPCTSTR)p_msg->GetFaultDetail());
    }
  }
  else
  {
    // Possibly a SOAP fault as answer
    if(!p_msg->GetFaultCode().IsEmpty())
    {
      printf("SOAP Fault: %s\n", p_msg->GetFault().GetString());
    }
    else
    {
      // Raw HTTP error
      BYTE* response = NULL;
      unsigned length = 0;
      p_client.GetResponse(response, length);
      printf("Message not sent!\n");
      printf("Service answer: %s\n", (char*)response);
      printf("HTTP Client error: %s\n", p_client.GetStatusText().GetString());
    }
  }
  // SUMMARY OF THE TEST
  // --- "---------------------------------------------- - ------
  printf("Send: SOAP datatype double calculation         : %s\n", result ? "OK" : "ERROR");

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

  msg->SetParameter("One","ABC");
  msg->SetParameter("Two","1-2-3"); // "17" will stop the server
  XMLElement* param = msg->SetParameter("Units","");
  XMLElement* enh1 = msg->AddElement(param,"Unit",XDT_String,"");
  XMLElement* enh2 = msg->AddElement(param,"Unit",XDT_String,"");
  msg->SetElement(enh1,"Unitnumber",12345);
  msg->SetElement(enh2,"Unitnumber",67890);
  msg->SetAttribute(enh1,"Independent",true);
  msg->SetAttribute(enh2,"Independent",false);

  if(p_encrypt != XMLEncryption::XENC_Plain)
  {
    msg->SetSecurityLevel(p_encrypt);
    msg->SetSecurityPassword("ForEverSweet16");

    if(p_encrypt == XMLEncryption::XENC_Body)
    {
      // msg->SetSigningMethod(CALG_RSA_SIGN);
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
  msg->SetParameter("Price", p_price);
  return msg;
}

int
TestReliableMessaging(HTTPClient* p_client,XString p_namespace,XString p_action,XString p_url,bool p_tokenProfile)
{
  int errors = 0;
  int totalDone = 0;
  WebServiceClient client(p_namespace,p_url,"",true);
  client.SetHTTPClient(p_client);
  client.SetLogAnalysis(p_client->GetLogging());
  
  // testing soap compression
  // client.SetSoapCompress(true);

  // Testing with WS-Security Token profile
  if(p_tokenProfile)
  {
    client.SetUser("marlin");
    client.SetPassword("M@rl!nS3cr3t");
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
      xprintf("Sending RM message: %s\n",message->GetSoapMessage().GetString());

      if(client.Send(message))
      {
        xprintf("Received: %s\n%s",message->GetSoapAction().GetString(),message->GetSoapMessage().GetString());

        XString action = message->GetSoapAction();
        XString test   = message->GetParameter("Three");
        int     number = message->GetParameterInteger("Four");

        xprintf("Answer: %s : %s\n","Three",test.GetString());
        xprintf("Answer: %s : %d\n","Four ",number);

        bool result = false;
        if(test == "DEF" || number == 123 && action == "TestMessageResponse")
        {
          ++totalDone;
          result = true;
        }
        else
        {
          printf("Answer with fault: %s\n",message->GetFault().GetString());
          result = false;
        }
        // SUMMARY OF THE TEST
        // --- "---------------------------------------------- - ------
        printf("Send: SOAP WS-ReliableMessaging                : %s\n",result ? "OK" : "ERROR");
        errors += result ? 0 : 1;
      }
      else
      {
        ++errors;
        printf("Message **NOT** correctly sent!\n");

        printf("Error code WebServiceClient: %s\n",client.GetErrorText().GetString());
        printf("Return message error:\n%s\n",message->GetFault().GetString());
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
    printf("ERROR received      : %s\n",error.GetErrorMessage().GetString());
    printf("ERROR from WS Client: %s\n",client.GetErrorText().GetString());
  }
  if(message)
  {
    delete message;
    message = nullptr;
  }
  return (totalDone == NUM_RM_TESTS) ? 0 : 1;
}

int
DoSendByQueue(HTTPClient& p_client,XString p_namespace,XString p_action,XString p_url)
{
  int times = 20;
  XString name("TestNumber");

  for(int x = 1; x <= times; ++x)
  {
    SOAPMessage* message = CreateSoapMessage(p_namespace,p_action,p_url);
    message->SetParameter(name,x);
    p_client.AddToQueue(message);
  }
  // --- "---------------------------------------------- - ------
  printf("Send: [%d] messages added to the sending queue : OK\n",times);

  return 0;
}

int
DoSendAsyncQueue(HTTPClient& p_client,XString p_namespace,XString p_url)
{
  int times = 10;
  XString resetMess("MarlinReset");
  XString textMess ("MarlinText");
  
  SOAPMessage* reset1 = new SOAPMessage(p_namespace,resetMess,SoapVersion::SOAP_12,p_url);
  SOAPMessage* reset2 = new SOAPMessage(p_namespace,resetMess,SoapVersion::SOAP_12,p_url);
  reset1->SetParameter("DoReset",true);
  reset2->SetParameter("DoReset",true);

  p_client.AddToQueue(reset1);
  for(int ind = 0;ind < times; ++ind)
  {
    SOAPMessage* msg = new SOAPMessage(p_namespace,textMess,SoapVersion::SOAP_12,p_url);
    msg->SetParameter("Text","This is a testing message to see if async SOAP messages are delivered.");
    p_client.AddToQueue(msg);
  }
  p_client.AddToQueue(reset2);

  // --- "---------------------------------------------- - ------
  printf("Send: Asynchronous messages added to the queue : OK\n");

  return 0;
}

inline XString 
CreateURL(XString p_extra)
{
  XString url;
  url.Format("http://%s:%d/MarlinTest/%s",MARLIN_HOST,TESTING_HTTP_PORT,p_extra.GetString());
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
  XString namesp("http://interface.marlin.org/services");
  XString command("TestMessage");
  XString url(CreateURL("Insecure"));

  // Test 1
  xprintf("TESTING STANDARD SOAP MESSAGE TO /MarlinTest/Insecure/\n");
  xprintf("====================================================\n");
  msg = CreateSoapMessage(namesp,command,url);
  errors += DoSend(client,msg,(char*)"insecure");

  // Test 2
  xprintf("TESTING BODY SIGNING SOAP TO /MarlinTest/BodySigning/\n");
  xprintf("===================================================\n");
  url = CreateURL("BodySigning");
  msg = CreateSoapMessage(namesp,command,url,SoapVersion::SOAP_12, XMLEncryption::XENC_Signing);
  errors += DoSend(client,msg,(char*)"body signing");

  // Test 3
  xprintf("TESTING BODY ENCRYPTION SOAP TO /MarlinTest/BodyEncrypt/\n");
  xprintf("======================================================\n");
  url = CreateURL("BodyEncrypt");
  msg = CreateSoapMessage(namesp,command,url, SoapVersion::SOAP_12, XMLEncryption::XENC_Body);
  errors += DoSend(client,msg,(char*)"body encrypting");

  // Test 4
  xprintf("TESTING WHOLE MESSAGE ENCRYPTION TO /MarlinTest/MessageEncrypt/\n");
  xprintf("=============================================================\n");
  url = CreateURL("MessageEncrypt");
  msg = CreateSoapMessage(namesp,command,url, SoapVersion::SOAP_12, XMLEncryption::XENC_Message);
  errors += DoSend(client,msg,(char*)"message encrypting");

  // Test 5
  xprintf("TESTING RELIABLE MESSAGING TO /MarlinTest/Reliable/\n");
  xprintf("=================================================\n");
  url = CreateURL("Reliable");
  errors += TestReliableMessaging(&client,namesp,command,url,false);

  // Test 6
  xprintf("TESTING RELIABLE MESSAGING TO /MarlinTest/ReliableBA/ with WS-Security token profile\n");
  xprintf("=================================================\n");
  url = CreateURL("ReliableBA");
  errors += TestReliableMessaging(&client,namesp,command,url,true);

  // Test 7
  xprintf("TESTING THE TOKEN FUNCTION TO /MarlinTest/TestToken/\n");
  xprintf("====================================================\n");
  url = CreateURL("TestToken");
  msg = CreateSoapMessage(namesp,command,url);
  client.SetSingleSignOn(true);
  XString user("CERT6\\Beheerder");
  XString password("altijd");
  client.SetUser(user);
  client.SetPassword(password);
  errors += DoSend(client,msg,(char*)"token testing");
  client.SetSingleSignOn(false);

  // Test 8
  xprintf("TESTING THE SUB-SITES FUNCTION TO /MarlinTest/TestToken/One/\n");
  xprintf("TESTING THE SUB-SITES FUNCTION TO /MarlinTest/TestToken/Two/\n");
  xprintf("============================================================\n");
  XString url1 = CreateURL("TestToken/One");
  XString url2 = CreateURL("TestToken/Two");
  msg = CreateSoapMessage(namesp,command,url1);
  client.SetSingleSignOn(true);
  client.SetUser(user);
  client.SetPassword(password);
  errors += DoSend(client,msg,(char*)"single sign on");
  msg = CreateSoapMessage(namesp,command,url2);
  errors += DoSend(client,msg,(char*)"single sign on");
  client.SetSingleSignOn(false);

  // Test 9
  xprintf("TESTING SOAP FAULT TO /MarlinTest/Insecure/\n");
  xprintf("=================================================\n");
  url = CreateURL("Insecure");
  msg = CreateSoapMessage(namesp,command,url);
  msg->SetParameter("TestFault",true);
  errors += DoSend(client,msg,(char*)"soap fault",true);

  // Test 10
  xprintf("TESTING UNICODE SENDING TO /MarlinTest/Insecure/\n");
  xprintf("================================================\n");
  url = CreateURL("Insecure");
  msg = CreateSoapMessage(namesp,command,url);
  msg->SetSendUnicode(true);
  errors += DoSend(client,msg,(char*)"sending unicode");

  // Test 11
  xprintf("TESTING FILTERING CAPABILITIES TO /MarlinTest/Filter/\n");
  xprintf("=====================================================\n");
  url = CreateURL("Filter");
  msg = CreateSoapPriceMessage(namesp,command,url,456.78);
  errors += DoSendPrice(client,msg,456.78);
    
  // Test 12
  xprintf("TESTING HIGH SPEED QUEUE TO /MarlinTest/Insecure/\n");
  xprintf("=================================================\n");
  url = CreateURL("Insecure");
  errors += DoSendByQueue(client,namesp,command,url);

  // Test 13
  xprintf("TESTING ASYNCHRONEOUS SOAP MESSAGES TO /MarlinTest/Asynchrone/\n");
  xprintf("==============================================================\n");
  url = CreateURL("Asynchrone");
  errors += DoSendAsyncQueue(client,namesp,url);

  // Waiting for the queue to drain
  printf("\nWait for last test to complete.\n");
  while(client.GetQueueSize())
  {
    Sleep(200);
  }

  printf("\n**READY**\n\nType a word and ENTER\n");

  // Wait for key to occur
  // so the messages can be send and debugged :-)
  WaitForKey();

  printf("Stopping the client\n");

  client.StopClient();

  printf("The client is %s\n",client.GetIsRunning() ? "still running!\n" : "stopped.\n");

  // Any error found
  XString boodschap;
  int error = client.GetError(&boodschap);
  if(error)
  {
    XString systeemText = GetLastErrorAsString(error);
    printf("ERROR! Code: %d = %s\nErrortext: %s\n",error,systeemText.GetString(),boodschap.GetString());
  }
  return errors;
}
