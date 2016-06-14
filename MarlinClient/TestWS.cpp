/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: TestWS.cpp
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
DoSend(HTTPClient& p_client,SOAPMessage* p_msg,bool p_fault = false)
{
  bool result = false;

  // 1: CHECK SYNCHROON VERZONDEN. NORMAAL BERICHT
  if(p_client.Send(p_msg))
  {
    if(p_msg->GetFaultCode().IsEmpty())
    {
      xprintf("Received: %s\n%s",p_msg->GetSoapAction(),p_msg->GetSoapMessage());

      CString action  = p_msg->GetSoapAction();
      CString test    = p_msg->GetParameter("Three");
      int     number  = p_msg->GetParameterInteger("Four");
      CString testing = p_msg->GetParameter("Testing");

      xprintf("Answer: %s : %s\n","Three",test);
      xprintf("Answer: %s : %d\n","Four",number);

      // See if the answer is satisfying
      if(test == "DEF" || number == 123 && action == "TestMessageResponse" || testing == "OK")
      {
        result = true;
      }
      // SUMMARY OF THE TEST
      // --- "--------------------------- - ------\n"
      printf("TEST SOAP MESSAGE           : %s\n",result ? "OK" : "ERROR");
    }
    else if(!p_fault)
    {
      // Only show it if we were not expecting a fault
      printf("Answer with error: %s\n",(LPCTSTR)p_msg->GetFault());

      printf("SOAP Fault code:   %s\n",(LPCTSTR)p_msg->GetFaultCode());
      printf("SOAP Fault actor:  %s\n",(LPCTSTR)p_msg->GetFaultActor());
      printf("SOAP Fault string: %s\n",(LPCTSTR)p_msg->GetFaultString());
      printf("SOAP FAULT detail: %s\n",(LPCTSTR)p_msg->GetFaultDetail());
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

  return (result != p_fault) ? 0 : 1;
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
  // --- "--------------------------- - ------\n"
  printf("TEST SOAP MESSAGE           : %s\n", result ? "OK" : "ERROR");

  // ready with the message
  delete p_msg;

  return (result == true) ? 0 : 1;
}

SOAPMessage*
CreateSoapMessage(CString       p_namespace
                 ,CString       p_command
                 ,CString       p_url
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
CreateSoapPriceMessage(CString p_namespace
                      ,CString p_command
                      ,CString p_url
                      ,double  p_price)
{
  SOAPMessage* msg = new SOAPMessage(p_namespace,p_command,SoapVersion::SOAP_12,p_url);
  msg->SetParameter("Price", p_price);
  return msg;
}

int
TestReliableMessaging(HTTPClient* p_client,CString p_namespace,CString p_action,CString p_url)
{
  int errors = 0;
  int totalDone = 0;
  WebServiceClient client(p_namespace,p_url,"",true);
  client.SetHTTPClient(p_client);
  client.SetLogAnalysis(p_client->GetLogging());
  // testing soap compression
  // client.SetSoapCompress(true);

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
      xprintf("Sending RM message: %s\n",message->GetSoapMessage());

      if(client.Send(message))
      {
        xprintf("Received: %s\n%s",message->GetSoapAction(),message->GetSoapMessage());

        CString action = message->GetSoapAction();
        CString test   = message->GetParameter("Three");
        int     number = message->GetParameterInteger("Four");

        xprintf("Answer: %s : %s\n","Three",test);
        xprintf("Answer: %s : %d\n","Four ",number);

        bool result = false;
        if(test == "DEF" || number == 123 && action == "TestMessageResponse")
        {
          ++totalDone;
          result = true;
        }
        // SUMMARY OF THE TEST
        // --- "--------------------------- - ------\n"
        printf("TEST SOAP MESSAGE           : %s\n",result ? "OK" : "ERROR");
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
  catch(CString& error)
  {
    ++errors;
    printf("ERROR received      : %s\n",error.GetString());
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
DoSendByQueue(HTTPClient& p_client,CString p_namespace,CString p_action,CString p_url)
{
  int times = 20;
  CString name("TestNumber");

  for(int x = 1; x <= times; ++x)
  {
    SOAPMessage* message = CreateSoapMessage(p_namespace,p_action,p_url);
    message->SetParameter(name,x);
    p_client.AddToQueue(message);
  }
  printf("Message added [%d] times to the sending queue.\n",times);

  return 0;
}

int TestWebservices(HTTPClient& client)
{
  int errors = 0;
  extern CString logfileName;

  // Testing cookie function
  // errors += TestCookies(client);

  // Standard values for messages
  CString namesp("http://interface.marlin.org/services");
  CString command("TestMessage");
  CString url("http://" MARLIN_HOST ":1200/MarlinTest/Insecure");

//   // Test 1
//   printf("TESTING STANDARD SOAP MESSAGE TO /MarlinTest/Insecure/\n");
//   printf("====================================================\n");
//   SOAPMessage* msg = CreateSoapMessage(namesp,command,url);
//   errors += DoSend(client,msg);
// 
//   // Test 2
//   printf("TESTING BODY SIGNING SOAP TO /MarlinTest/BodySigning/\n");
//   printf("===================================================\n");
//   url = "http://localhost:1200/MarlinTest/BodySigning";
//   msg = CreateSoapMessage(namesp,command,url,SoapVersion::SOAP_12, XMLEncryption::XENC_Signing);
//   errors += DoSend(client,msg);
// 
//   // Test 3
//   printf("TESTING BODY ENCRYPTION SOAP TO /MarlinTest/BodyEncrypt/\n");
//   printf("======================================================\n");
//   url = "http://localhost:1200/MarlinTest/BodyEncrypt";
//   msg = CreateSoapMessage(namesp,command,url, SoapVersion::SOAP_12, XMLEncryption::XENC_Body);
//   errors += DoSend(client,msg);
// 
//   // Test 4
//   printf("TESTING WHOLE MESSAGE ENCRYPTION TO /MarlinTest/MessageEncrypt/\n");
//   printf("=============================================================\n");
//   url = "http://localhost:1200/MarlinTest/MessageEncrypt";
//   msg = CreateSoapMessage(namesp,command,url, SoapVersion::SOAP_12, XMLEncryption::XENC_Message);
//   errors += DoSend(client,msg);
// 
//   // Test 4
//   printf("TESTING RELIABLE MESSAGING TO /MarlinTest/Reliable/\n");
//   printf("=================================================\n");
//   url = "http://localhost:1200/MarlinTest/Reliable";
//   errors += TestReliableMessaging(&client,namesp,command,url);
// 
//   // Test 5
//   printf("TESTING THE TOKEN FUNCTION TO /MarlinTest/TestToken/\n");
//   printf("====================================================\n");
//   url = "http://localhost:1200/MarlinTest/TestToken";
//   msg = CreateSoapMessage(namesp,command,url);
//   client.SetSingleSignOn(true);
//   errors += DoSend(client,msg);
//   client.SetSingleSignOn(false);
// 
//   // Test 6
//   printf("TESTING THE SUB-SITES FUNCTION TO /MarlinTest/TestToken/One/\n");
//   printf("TESTING THE SUB-SITES FUNCTION TO /MarlinTest/TestToken/Two/\n");
//   printf("============================================================\n");
//   CString url1 = "http://localhost:1200/MarlinTest/TestToken/One";
//   CString url2 = "http://localhost:1200/MarlinTest/TestToken/Two";
//   msg = CreateSoapMessage(namesp,command,url1);
//   client.SetSingleSignOn(true);
//   errors += DoSend(client,msg);
//   msg = CreateSoapMessage(namesp,command,url2);
//   errors += DoSend(client,msg);
//   client.SetSingleSignOn(false);
// 
//   // Test 7
//   printf("TESTING SOAP FAULT TO /MarlinTest/Insecure/\n");
//   printf("=================================================\n");
//   url = "http://localhost:1200/MarlinTest/Insecure/";
//   msg = CreateSoapMessage(namesp,command,url);
//   msg->SetParameter("TestFault",true);
//   errors += DoSend(client,msg,true);
// 
//   // Test 8
//   printf("TESTING UNICODE SENDING TO /MarlinTest/Insecure/\n");
//   printf("================================================\n");
//   url = "http://localhost:1200/MarlinTest/Insecure/";
//   msg = CreateSoapMessage(namesp,command,url);
//   msg->SetSendUnicode(true);
//   errors += DoSend(client,msg);
// 
//   // Test 9
//   printf("TESTING FILTERING CAPABILITIES TO /MarlinTest/Filter/\n");
//   printf("=====================================================\n");
//   url = "http://localhost:1200/MarlinTest/Filter/";
//   msg = CreateSoapPriceMessage(namesp,command,url,456.78);
//   errors += DoSendPrice(client,msg,456.78);

  // Test 10
  printf("TESTING HIGH SPEED QUEUE TO /MarlinTest/Insecure/\n");
  printf("===============================================\n");
  url = "http://" MARLIN_HOST ":1200/MarlinTest/Insecure";
  errors += DoSendByQueue(client,namesp,command,url);

  printf("\nWait for last test to complete. Type a word and ENTER\n");

  // Wait for key to occur
  // so the messages can be send and debugged :-)
  WaitForKey();

  printf("Stopping the client\n");

  client.StopClient();

  printf("The client is %s\n",client.GetIsRunning() ? "still running!\n" : "stopped.\n");

  // Any error found
  CString boodschap;
  int error = client.GetError(&boodschap);
  if(error)
  {
    CString systeemText = GetLastErrorAsString(error);
    printf("ERROR! Code: %d = %s\nErrortext: %s\n",error,systeemText.GetString(),boodschap.GetString());
  }
  return errors;
}
