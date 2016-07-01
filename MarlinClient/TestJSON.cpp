/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: TestJSON.cpp
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
#include "JSONMessage.h"
#include "SOAPMessage.h"
#include "HTTPClient.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

int TestValue1(CString p_object)
{
  JSONMessage json(p_object);

  if(json.GetErrorState())
  {
    printf("UNEXPECTED ERROR JSON object 1: %s\n",(LPCSTR)json.GetLastError());
    return 1;
  }

  // Reprint the message
  CString object = json.GetJsonMessage();

  if(object != p_object)
  {
    printf("JSON Parser error:\n%s\n%s\n",(LPCTSTR)p_object,(LPCTSTR)object);
    return 1;
  }

  return 0;
}

int TestValue1Double(CString p_object)
{
  JSONMessage json(p_object);

  if(json.GetErrorState())
  {
    printf("UNEXPECTED ERROR JSON object 1: %s\n",(LPCSTR)json.GetLastError());
    return 1;
  }

  // Reprint the message
  CString object = json.GetJsonMessage();

  // Engineering breaking criterion on small IEEE number
  if(abs(atof(object) - atof(p_object)) > 1.0E-13)
  {
    printf("JSON Parser error:\n%s\n%s\n",(LPCTSTR)p_object,(LPCTSTR)object);
    return 1;
  }

  return 0;
}

int TestArray(CString p_array)
{
  JSONMessage json(p_array);

  if(json.GetErrorState())
  {
    printf("UNEXPECTED ERROR JSON array 1: %s\n",(LPCSTR)json.GetLastError());
    return 1;
  }

  CString array = json.GetJsonMessage();

  if(p_array != array)
  {
    printf("JSON Parser error:\n%s\n%s\n",(LPCTSTR)p_array,(LPCTSTR)array);
    return 1;
  }
  return 0;
}

int TestObject(CString p_object)
{
  JSONMessage json(p_object);

  if(json.GetErrorState())
  {
    printf("UNEXPECTED ERROR JSON object 1: %s\n",(LPCSTR)json.GetLastError());
    return 1;
  }

  CString object = json.GetJsonMessage();

  if(object != p_object)
  {
    printf("JSON Parser error:\n%s\n%s\n",(LPCTSTR)p_object,(LPCTSTR)object);
    return 1;
  }
  return 0;
}

int TestArray()
{
  int errors = 0;
  CString msg1 = "<Applications>\n"
                 "  <Datasources>\n"
                 "     <Datasource>develop</Datasource>\n"
                 "     <Datasource>test</Datasource>\n"
                 "     <Datasource>accept</Datasource>\n"
                 "     <Datasource>production</Datasource>\n"
                 "  </Datasources>\n"
                 "  <Commercial>\n"
                 "     <Application>Financials</Application>\n"
                 "     <Application>Maintainance</Application>\n"
                 "     <Application>Rentals</Application>\n"
                 "  </Commercial>\n"
                 "  <Default>QL_Account</Default>\n"
                 "</Applications>";
  SOAPMessage soap1(msg1);
  JSONMessage json(&soap1);
  CString res = json.GetJsonMessage();
  xprintf(res);
  CString expect1 = "{\n"
                    " \"Applications\":{\n"
                    "   \"Datasources\":{\n"
                    "     \"Datasource\":\n"
                    "[      \"develop\"\n"
                    "      ,\"test\"\n"
                    "      ,\"accept\"\n"
                    "      ,\"production\"\n"
                    "]\n"
                    "    }\n"
                    "\n"
                    "  ,\"Commercial\":{\n"
                    "     \"Application\":\n"
                    "[      \"Financials\"\n"
                    "      ,\"Maintainance\"\n"
                    "      ,\"Rentals\"\n"
                    "]\n"
                    "    }\n"
                    "\n"
                    "  ,\"Default\":\"QL_Account\"\n"
                    "  }\n"
                    "}\n";

  // Check that the expected JSON matches what we now it must be
  errors += (res == expect1) ? 0 : 1;

  // Now transform back to SOAP
  SOAPMessage soap2(&json);
  CString msg2 = soap2.GetSoapMessage();
  
  CString expect2 = "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
                    "<s:Envelope>\n"
                    "  <s:Header />\n"
                    "  <s:Body>\n"
                    "    <Applications>\n"
                    "      <Datasources>\n"
                    "        <Datasource>develop</Datasource>\n"
                    "        <Datasource>test</Datasource>\n"
                    "        <Datasource>accept</Datasource>\n"
                    "        <Datasource>production</Datasource>\n"
                    "      </Datasources>\n"
                    "      <Commercial>\n"
                    "        <Application>Financials</Application>\n"
                    "        <Application>Maintainance</Application>\n"
                    "        <Application>Rentals</Application>\n"
                    "      </Commercial>\n"
                    "      <Default>QL_Account</Default>\n"
                    "    </Applications>\n"
                    "  </s:Body>\n"
                    "</s:Envelope>\n";

  // Check that the resulting SOAP matches what it must be
  errors += (msg2 == expect2) ? 0 : 1;

  // Possibly 0,1 or 2 errors
  return errors;
}

int TestJSON(void)
{
  int errors = 0;

  // Test whether we can make a JSON object
  errors += TestValue1("null");
  errors += TestValue1("true");
  errors += TestValue1("false");
  errors += TestValue1("");

  // Singular values
  errors += TestValue1("\"This is a regular string\"");
  errors += TestValue1("\"This is a string\\nwith\\r\\nseveral \\/ and more \\\\ escape \\f sequences \\t in \\t it.\\r\\n\"");
  errors += TestValue1("1234");
  errors += TestValue1("1234.56789");
  errors += TestValue1("-1234");
  errors += TestValue1Double("1.23456e12");
  errors += TestValue1Double("1.23456e-12");
  // Singular array
  errors += TestArray("[]");
  errors += TestArray("[true]");
  errors += TestArray("[true,\"string\"]");
  // Singular object
  errors += TestObject("{}");
  errors += TestObject("{\"one\":1}");
  errors += TestObject("{\"one\":1,\"two\":2}");
  // Compounded test
  errors += TestObject("{\"one\":1,\"two\":[1,2,3,4,5,\"six\",\"seven\"]}");
  // Testen van een array
  errors += TestArray();


  // TEST ROUNDTRIP SOAP -> JSON
  CString namesp("http://test.marlin.org/interface");
  CString action("FirstAction");
  SOAPMessage msg(namesp,action);
  msg.SetParameter("First",101);
  msg.SetParameter("Second",102);
  XMLElement* elem = msg.AddElement(NULL,"Third",(XmlDataType)XDT_String,"");
  if(elem)
  {
    msg.AddElement(elem,"Fortune",XDT_Integer,"1000000");
    msg.AddElement(elem,"Glory",  XDT_String, "Indiana Jones");
  }
  JSONMessage json(&msg);
  CString str = json.GetJsonMessage();
  errors += TestObject(str);

  // And test the way back
  SOAPMessage soap(&json);

  // Test if the ROUNDDTRIP SOAP -> JSON -> SOAP did work
  int FirstAction1 = soap.GetParameterInteger("First");
  int FirstAction2 = soap.GetParameterInteger("Second");
  XMLElement* fort = soap.FindElement("Fortune");
  XMLElement* glor = soap.FindElement("Glory");
  int FirstFortune = fort ? atoi(fort->GetValue()) : 0;
  CString glory    = glor ? glor->GetValue() : "";
  if(FirstAction1 != 101 || FirstAction2 != 102 || FirstFortune != 1000000 || glory.Compare("Indiana Jones"))
  {
    printf("ERROR: ROUND TRIP ENGINEERING SOAP -> JSON -> SOAP FAILED\n");
    ++errors;
  }

  // Check that soap faults will survive SOAP/JSON/SOAP roundtrip conversions
  SOAPMessage wrong("<body></body>");
  wrong.SetSoapVersion(SoapVersion::SOAP_12);
  wrong.SetFault("Client","Message","Wrong body","Empty Body");
  xprintf(wrong.GetSoapMessage());
  JSONMessage wrongtoo(&wrong);
  xprintf(wrongtoo.GetJsonMessage());
  SOAPMessage veryWrong(&wrongtoo);
  xprintf(veryWrong.GetSoapMessage());
  xprintf(veryWrong.GetFault());

  if(veryWrong.GetFaultCode()   != "Client"     ||
     veryWrong.GetFaultActor()  != "Message"    ||
     veryWrong.GetFaultString() != "Wrong body" ||
     veryWrong.GetFaultDetail() != "Empty Body"  )
  {
    ++errors;
  }

  // SUMMARY OF THE TEST
  // --- "--------------------------- - ------\n"
  printf("TEST JSON MESSAGES          : %s\n",errors ? "ERROR" : "OK");

  return errors;
}

int DoSend(HTTPClient* p_client,JSONMessage* p_msg)
{
  int errors = 1;

  if(p_client->Send(p_msg))
  {
    CString msg = p_msg->GetJsonMessage();
    JSONvalue& val = p_msg->GetValue();
    if(val.GetDataType() == JsonType::JDT_object)
    {
      if(val.GetObject()[0].m_name == "one")
      {
        if(msg == "{\"one\":[1,2,3,4,5]}")
        {
          --errors;
        }
      }
      if(val.GetObject()[0].m_name == "two")
      {
        if(msg == "{\"two\":[201,202,203,204.5,205.6789],\"three\":[301,302,303,304.5,305.6789]}")
        {
          --errors;
        }
      }
    }
  }
  else
  {
    BYTE* response = NULL;
    unsigned length = 0;
    p_client->GetResponse(response,length);
    printf("Message not sent!\n");
    printf("Service answer: %s\n",(char*)response);
    // Raw HTTP error
    printf("HTTP Client error: %s\n",p_client->GetStatusText().GetString());
  }
  return errors;
}

int TestJsonData(HTTPClient* p_client)
{
  int errors = 0;
  CString url = "http://" MARLIN_HOST ":1200/MarlinTest/Data?test=2&size=medium%20large";

  // Test standard JSON
  printf("TESTING STANDARD JSON MESSAGE TO /MarlinTest/Data/\n");
  printf("==================================================\n");

  JSONMessage msg1("\"Test1\"",url);
  msg1.AddHeader("GUID","888-777-666");
  errors += DoSend(p_client,&msg1);

  JSONMessage msg2("\"Test2\"",url);
  msg2.AddHeader("GUID","888-777-666");
  errors += DoSend(p_client,&msg2);

  // Now Send again in UTF-16 Unicode
  printf("TESTING UNICODE JOSN MESSAGE TO /MarlinTest/Data/\n");
  printf("=================================================\n");

  JSONMessage msg3("\"Test1\"",url);
  msg3.SetSendUnicode(true);
  msg3.AddHeader("GUID","888-777-666");
  errors += DoSend(p_client,&msg3);

  JSONMessage msg4("\"Test2\"",url);
  msg4.SetSendUnicode(true);
  msg4.AddHeader("GUID","888-777-666");
  errors += DoSend(p_client,&msg4);

  return errors;
}
