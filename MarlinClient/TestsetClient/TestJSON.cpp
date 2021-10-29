/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: TestJSON.cpp
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
#include "JSONMessage.h"
#include "SOAPMessage.h"
#include "JSONPointer.h"
#include "JSONPath.h"
#include "HTTPClient.h"
#include "MultiPartBuffer.h"

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
                    "\t\"Applications\":{\n"
                    "\t\t\"Datasources\":{\n"
                    "\t\t\t\"Datasource\":[\n"
                    "\t\t\t\"develop\",\n"
                    "\t\t\t\"test\",\n"
                    "\t\t\t\"accept\",\n"
                    "\t\t\t\"production\"\n"
                    "\t\t\t]\n"
                    "\t\t},\n"
                    "\t\t\"Commercial\":{\n"
                    "\t\t\t\"Application\":[\n"
                    "\t\t\t\"Financials\",\n"
                    "\t\t\t\"Maintainance\",\n"
                    "\t\t\t\"Rentals\"\n"
                    "\t\t\t]\n"
                    "\t\t},\n"
                    "\t\t\"Default\":\"QL_Account\"\n"
                    "\t}\n"
                    "}";

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

int MultiJSON()
{
  int errors = 0;
  const char* buffer=
"------WebKitFormBoundarydZ38XIr1QTTS2IMb\r\n"
"Content-Disposition: form-data; name=\"277153\"\r\n"
"\r\n"
"{\"ID\":\"0\",\"Old: \":\"\",\"New\":\"\",\"Event\":\"Start\"}\r\n"
"------WebKitFormBoundarydZ38XIr1QTTS2IMb\r\n"
"Content-Disposition: form-data; name=\"44074\"\r\n"
"\r\n"
"{\"ID\":\"44074\",\"Old:\":\"bar2\",\"New\":\"bar5\",\"Event\":\"Blur\"}\r\n"
"------WebKitFormBoundarydZ38XIr1QTTS2IMb\r\n"
"Content-Disposition: form-data; name=\"277153\"\r\n"
"\r\n"
"{\"ID\":\"277153\",\"Old:\":\"\",\"New\":\"\",\"Event\":\"Click\"}\r\n"
"------WebKitFormBoundarydZ38XIr1QTTS2IMb\r\n"
"Content-Disposition: form-data; name=\"277153\"\r\n"
"\r\n"
"{\"ID\":\"0\",\"Old: \":\"\",\"New\":\"\",\"Event\":\"Sluit\"}\r\n"
"------WebKitFormBoundarydZ38XIr1QTTS2IMb--";

  int size = (int)strlen(buffer);
  xprintf("Buffer size : %d\n",size);

  FileBuffer buf;
  buf.AddBuffer((uchar*)buffer,size);
  CString contentType("multipart/form-data; boundary=----WebKitFormBoundarydZ38XIr1QTTS2IMb");

  MultiPartBuffer multi(FD_MULTIPART);
  multi.ParseBuffer(contentType,&buf);

  size_t parts = multi.GetParts();
  for(size_t ind = 0; ind < parts; ++ind)
  {
    MultiPart* part = multi.GetPart((int) ind);

    xprintf("Part %d: %s\n",(int)ind,part->GetData().GetString());
  }
  CString test = multi.GetPart(3)->GetData();
  if(test.Compare("{\"ID\":\"0\",\"Old: \":\"\",\"New\":\"\",\"Event\":\"Sluit\"}\r\n"))
  {
    xprintf("ERROR: Multipart buffer of 4 JSON's not working\n");
    ++errors;
  }
  // SUMMARY OF THE TEST
  // --- "---------------------------------------------- - ------
  printf("MultiPartBuffer with multiple JSON messages    : %s\n",errors ? "ERROR" : "OK");

  return errors;
}


int TestFindValue()
{
  int errors = 0;
  CString jsonstring = "{ \n"
                       "  \"name1\" : \"value1\"\n"
                       " ,\"name2\" : \"value2\"\n"
                       " ,\"name3\" : 33\n"
                       "}\n";
  JSONMessage json(jsonstring);

  JSONvalue* val2 = json.FindValue("name2");
  int val3 = json.GetValueInteger("name3");
  JSONvalue* obj1 = json.FindValue("name2",true,true);
  JSONpair*  pair = json.GetObjectElement(obj1,0);

  if(val2->GetString().Compare("value2"))
  {
    xprintf("ERROR: JSON FindValue cannot find the value by name\n");
    ++errors;
  }
  if(val3 != 33)
  {
    xprintf("ERROR: JSON GetValueInteger cannot find integer by name\n");
    ++errors;
  }
  if(pair->m_name.Compare("name1") || pair->m_value.GetString().Compare("value1"))
  {
    xprintf("ERROR: JSON GetObjectElement cannot find element by index\n");
    ++errors;
  }

  // Test arrays
  jsonstring = "{  \"two\":[201,202,203,204.5,205.6789]\n"
               ",\"three\":[301,302,303,304.5,305.6789]\n"
               ",\"four\" :[401,402,403,404.5,405.6789]\n"
               "}";
  JSONMessage two(jsonstring);
  JSONpair*  ar = two.FindPair("three");
  JSONvalue* vv = two.GetArrayElement(&ar->m_value,3);
  bcd   howmuch = vv->GetNumberBcd();

  if(howmuch != bcd("304.5"))
  {
    xprintf("ERROR: JSON GetArrayElement cannot find element by index\n");
    ++errors;
  }

  // SUMMARY OF THE TEST
  // --- "---------------------------------------------- - ------
  printf("JSON messages find value functions             : %s\n", errors ? "ERROR" : "OK");
  return errors;
}

int TestAddObject()
{
  JSONMessage json;

  JSONobject  object;
  JSONpair    pair1("one");
  JSONpair    pair2("two");
  JSONpair    pair3("three");
  pair1.SetValue(1);
  pair2.SetValue(2);
  pair3.SetValue(3);

  object.push_back(pair1);
  object.push_back(pair2);
  object.push_back(pair3);

  json.AddNamedObject("MyObject",object);

  JSONobject object2;
  JSONpair   pair4("four");
  JSONpair   pair5("five");
  JSONpair   pair6("six");
  pair4.SetValue(4);
  pair5.SetValue(5);
  pair6.SetValue(6);

  object2.push_back(pair4);
  object2.push_back(pair5);
  object2.push_back(pair6);

  json.AddNamedObject("MyObject",object2,true);

  json.SetWhitespace(true);
  CString total = json.GetJsonMessage(JsonEncoding::JENC_Plain);

  CString expected = "{\n"
                     "\t\"MyObject\":[\n"
                     "\t\t{\n"
                     "\t\t\t\"one\":1,\n"
                     "\t\t\t\"two\":2,\n"
                     "\t\t\t\"three\":3\n"
                     "\t\t},\n"
                     "\t\t{\n"
                     "\t\t\t\"four\":4,\n"
                     "\t\t\t\"five\":5,\n"
                     "\t\t\t\"six\":6\n"
                     "\t\t}\n"
                     "\t]\n"
                     "}";
  // Check that the expected JSON matches what we now it must be
  int errors = (total == expected) ? 0 : 1;

  if(errors)
  {
    xprintf("JSON string: \n%s\n", total.GetString());
  }
  // SUMMARY OF THE TEST
  // --- "---------------------------------------------- - ------
  printf("JSON message adding object with forced array   : %s\n", errors ? "ERROR" : "OK");
  return 0;
}

int TestJSONPointer(void)
{
  int errors = 0;
  CString jsonString = "{\n"
                       "\t\"MyObject\":[\n"
                       "\t\t{\n"
                       "\t\t\t\"one\":1,\n"
                       "\t\t\t\"two\":2,\n"
                       "\t\t\t\"three\":true\n"
                       "\t\t},\n"
                       "\t\t{\n"
                       "\t\t\t\"four\":\"The Fantastic 4\",\n"
                       "\t\t\t\"five\":5.17,\n"
                       "\t\t\t\"six\":6,\n"
                       "\t\t\t\"strange~ger\":42,\n"
                       "\t\t\t\"sp/1\":\"7.1.12\"\n"
                       "\t\t}\n"
                       "\t]\n"
                       "}";
  CString jsonCompressed(jsonString);
  jsonCompressed.Replace("\t","");
  jsonCompressed.Replace("\n","");

  JSONMessage json(jsonString);
  JSONPointer jp(&json,"/MyObject/0/two");

  int result = jp.GetResultInteger();
  errors += result != 2;

  jp.SetPointer("/MyObject/1/six");
  result = jp.GetResultInteger();
  errors += result != 6;
  errors += jp.GetType()   != JsonType::JDT_number_int;
  errors += jp.GetStatus() != JPStatus::JP_Match_number_int;

  jp.SetPointer("");
  CString resultJson = jp.GetResultForceToString();
  errors += resultJson.Compare(jsonCompressed) != 0;

  resultJson = jp.GetResultForceToString(true);
  errors += jsonString.Compare(resultJson) != 0;
  errors += jp.GetType()   != JsonType::JDT_object;
  errors += jp.GetStatus() != JPStatus::JP_Match_wholedoc;

  jp.SetPointer("/MyObject");
  errors += jp.GetResultArray()->size() != 2;
  errors += jp.GetType()   != JsonType::JDT_array;
  errors += jp.GetStatus() != JPStatus::JP_Match_array;

  jp.SetPointer("/MyObject/1");
  errors += jp.GetResultObject()->size() != 5;
  errors += jp.GetType()   != JsonType::JDT_object;
  errors += jp.GetStatus() != JPStatus::JP_Match_object;

  jp.SetPointer("/MyObject/1/five");
  bcd testnum = jp.GetResultBCD();
  errors += testnum != bcd("5.17");
  errors += jp.GetType()   != JsonType::JDT_number_bcd;
  errors += jp.GetStatus() != JPStatus::JP_Match_number_bcd;

  jp.SetPointer("/MyObject/1/four");
  CString four = jp.GetResultString();
  errors += four.Compare("The Fantastic 4") != 0;
  errors += jp.GetType()   != JsonType::JDT_string;
  errors += jp.GetStatus() != JPStatus::JP_Match_string;

  jp.SetPointer("/MyObject/0/three");
  errors += jp.GetResultConstant() != JsonConst::JSON_TRUE;
  errors += jp.GetType()   != JsonType::JDT_const;
  errors += jp.GetStatus() != JPStatus::JP_Match_constant;

  // Escape of an '/' char in a name
  jp.SetPointer("/MyObject/1/sp~11");
  CString version = jp.GetResultString();
  errors += version.Compare("7.1.12") != 0;

  // Escape of an '~' char in a name
  jp.SetPointer("/MyObject/1/strange~0ger");
  int reason = jp.GetResultInteger();
  errors += reason != 42;

  // SUMMARY OF THE TEST
  // --- "---------------------------------------------- - ------
  printf("JSONPointer checks on all types/objects/esc    : %s\n",errors ? "ERROR" : "OK");
  return errors;
}

int TestJSONPathInPointer(void)
{
  int errors = 0;

  CString jsonString = "{\n"
                       "\t\"MyObject\":[\n"
                       "\t\t{\n"
                       "\t\t\t\"one\":1,\n"
                       "\t\t\t\"two\":2,\n"
                       "\t\t\t\"three\":true\n"
                       "\t\t},\n"
                       "\t\t{\n"
                       "\t\t\t\"four\":\"The Fantastic 4\",\n"
                       "\t\t\t\"five\":5.17,\n"
                       "\t\t\t\"six\":6,\n"
                       "\t\t\t\"strange~ger\":42,\n"
                       "\t\t\t\"sp.1\":\"7.1.12\"\n"
                       "\t\t}\n"
                       "\t]\n"
                       "}";
  CString jsonCompressed(jsonString);
  jsonCompressed.Replace("\t","");
  jsonCompressed.Replace("\n","");

  JSONMessage json(jsonString);
  JSONPointer jp(&json,"$.MyObject.0.two");

  int result = jp.GetResultInteger();
  errors += result != 2;

  jp.SetPointer("$.MyObject.1.six");
  result = jp.GetResultInteger();
  errors += result != 6;
  errors += jp.GetType() != JsonType::JDT_number_int;
  errors += jp.GetStatus() != JPStatus::JP_Match_number_int;

  jp.SetPointer("$");
  CString resultJson = jp.GetResultForceToString();
  errors += resultJson.Compare(jsonCompressed) != 0;

  resultJson = jp.GetResultForceToString(true);
  errors += jsonString.Compare(resultJson) != 0;
  errors += jp.GetType() != JsonType::JDT_object;
  errors += jp.GetStatus() != JPStatus::JP_Match_wholedoc;

  jp.SetPointer("$.MyObject");
  errors += jp.GetResultArray()->size() != 2;
  errors += jp.GetType() != JsonType::JDT_array;
  errors += jp.GetStatus() != JPStatus::JP_Match_array;

  jp.SetPointer("$.MyObject.1");
  errors += jp.GetResultObject()->size() != 5;
  errors += jp.GetType() != JsonType::JDT_object;
  errors += jp.GetStatus() != JPStatus::JP_Match_object;

  jp.SetPointer("$.MyObject.1.five");
  bcd testnum = jp.GetResultBCD();
  errors += testnum != bcd("5.17");
  errors += jp.GetType() != JsonType::JDT_number_bcd;
  errors += jp.GetStatus() != JPStatus::JP_Match_number_bcd;

  jp.SetPointer("$.MyObject.1.four");
  CString four = jp.GetResultString();
  errors += four.Compare("The Fantastic 4") != 0;
  errors += jp.GetType() != JsonType::JDT_string;
  errors += jp.GetStatus() != JPStatus::JP_Match_string;

  jp.SetPointer("$.MyObject.0.three");
  errors += jp.GetResultConstant() != JsonConst::JSON_TRUE;
  errors += jp.GetType() != JsonType::JDT_const;
  errors += jp.GetStatus() != JPStatus::JP_Match_constant;

  // Escape of an '/' char in a name
  jp.SetPointer("$.MyObject.1.sp~11");
  CString version = jp.GetResultString();
  errors += version.Compare("7.1.12") != 0;

  // Escape of an '~' char in a name
  jp.SetPointer("$.MyObject.1.strange~0ger");
  int reason = jp.GetResultInteger();
  errors += reason != 42;

  // SUMMARY OF THE TEST
  // --- "---------------------------------------------- - ------
  printf("JSONPath checks on all types/objects/escapes   : %s\n",errors ? "ERROR" : "OK");

  return errors;
}

int TestJSONUnicode()
{
  int errors = 0;

  CString jsonString = "{ \"dollar\" : \"\\u00E9\\u00E9\\u006E \\u20AC = one €$\" }";
  JSONMessage json(jsonString);
  CString test = json.GetJsonMessage();
  CString expected("{\"dollar\":\"één € = one €$\"}");
  errors = test.Compare(expected) != 0;

  // SUMMARY OF THE TEST
  // --- "---------------------------------------------- - ------
  printf("JSON parser converts \\u escape sequences       : %s\n",errors ? "ERROR" : "OK");

  return errors;
}

int TestJSONPath(void)
{
  int errors = 0;

  CString jsonString = "{\t\"array\": [\n"
                       "\t\t{\t\"one\"  : 1  },\n"
                       "\t\t{\t\"two\"  : 2  },\n"
                       "\t\t{\t\"three\": 3  },\n"
                       "\t\t{\t\"four\" : 4  },\n"
                       "\t\t{\t\"five\" : 5  },\n"
                       "\t\t{\t\"six\"  : 6  },\n"
                       "\t\t{\t\"seven\": 7  },\n"
                       "\t\t{\t\"eight\": 8  },\n"
                       "\t\t{\t\"nine\" : 9  },\n"
                       "\t\t{\t\"ten\"  : 10 },\n"
                       "\t\t{\t\"eleven\": [\n"
                       "\t\t\t\t{\n"
                       "\t\t\t\t\t\"renault\": \"twingo\",\n"
                       "\t\t\t\t\t\"citroen\": \"ds\",\n"
                       "\t\t\t\t\t\"peugeot\": \"8005\"\n"
                       "\t\t\t\t},\n"
                       "\t\t\t\t{\n"
                       "\t\t\t\t\t\"opel\": \"astra\",\n"
                       "\t\t\t\t\t\"bmw\": \"3series\",\n"
                       "\t\t\t\t\t\"mercedes\": \"amg\"\n"
                       "\t\t\t\t},\n"
                       "\t\t\t\t{\n"
                       "\t\t\t\t\t\"ford\": \"focus\",\n"
                       "\t\t\t\t\t\"fiat\": \"750\"1,\n"
                       "\t\t\t\t\t\"seat\": \"leon\"\n"
                       "\t\t\t\t}\n"
                       "\t\t\t]\n"
                       "\t\t}\n"
                       "\t]\n"
                       "}";
  JSONMessage json(jsonString);
  JSONPath path(json,"$.array[4].five");

  int result = 0;
  if(path.GetStatus() == JPStatus::JP_Match_number_int)
  {
    result = path.GetFirstResult()->GetNumberInt();
  }
  errors = result != 5;

  path.SetPath("$.array[10].eleven[2].ford");
  if(path.GetStatus() != JPStatus::JP_INVALID)
  {
    CString name = path.GetFirstResult()->GetString();
    errors = name.Compare("focus") != 0;
  }

  path.SetPath("$.array[0:8:2]"); // selects 0,2,4,6
  errors += path.GetNumberOfMatches() != 4;
  errors += path.GetResult(0)->GetObject()[0].GetNumberInt() != 1;
  errors += path.GetResult(1)->GetObject()[0].GetNumberInt() != 3;
  errors += path.GetResult(2)->GetObject()[0].GetNumberInt() != 5;
  errors += path.GetResult(3)->GetObject()[0].GetNumberInt() != 7;

  path.SetPath("$.array[0:8:-2]"); // selects 8,6,4,2
  errors += path.GetNumberOfMatches() != 4;
  errors += path.GetResult(0)->GetObject()[0].GetNumberInt() != 8;
  errors += path.GetResult(1)->GetObject()[0].GetNumberInt() != 6;
  errors += path.GetResult(2)->GetObject()[0].GetNumberInt() != 4;
  errors += path.GetResult(3)->GetObject()[0].GetNumberInt() != 2;

  path.SetPath("$.array[10].eleven[0,1]");
  errors += path.GetNumberOfMatches() != 2;

  // SUMMARY OF THE TEST
  // --- "---------------------------------------------- - ------
  printf("JSONPath resolves all paths                    : %s\n",errors ? "ERROR" : "OK");

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
  errors += TestValue1("\"This is a string\\nwith\\r\\nseveral / and more \\\\ escape \\f sequences \\t in \\t it.\\r\\n\"");
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
  // Testing of an array
  errors += TestArray();
  // Testing the adding of an object
  errors += TestAddObject();


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
  CString glory    = glor ? glor->GetValue() : CString();
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
  // --- "---------------------------------------------- - ------
  printf("Basic tests on JSON object messages            : %s\n",errors ? "ERROR" : "OK");

  errors += MultiJSON();
  errors += TestFindValue();
  errors += TestJSONPointer();
  errors += TestJSONPathInPointer();
  errors += TestJSONUnicode();
  errors += TestJSONPath();

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
        if(msg == "{\"two\":[201,202,203,204.50,205.6789],\"three\":[301,302,303,304.50,305.6789]}")
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

  // --- "---------------------------------------------- - ------
  printf("Send: Json message send and received           : %s\n", errors ? "ERROR" : "OK");

  return errors;
}

int TestJsonData(HTTPClient* p_client)
{
  int errors = 0;
  CString url;
  url.Format("http://%s:%d/MarlinTest/Data?test=2&size=medium%%20large",MARLIN_HOST,TESTING_HTTP_PORT);

  // Test standard JSON
  xprintf("TESTING STANDARD JSON MESSAGE TO /MarlinTest/Data/\n");
  xprintf("==================================================\n");

  JSONMessage msg1("\"Test1\"",url);
  msg1.AddHeader("GUID","888-777-666");
  errors += DoSend(p_client,&msg1);

  JSONMessage msg2("\"Test2\"",url);
  msg2.AddHeader("GUID","888-777-666");
  errors += DoSend(p_client,&msg2);

  // Now Send again in UTF-16 Unicode
  xprintf("TESTING UNICODE JOSN MESSAGE TO /MarlinTest/Data/\n");
  xprintf("=================================================\n");


  p_client->SetTimeoutSend(100000);
  p_client->SetTimeoutReceive(100000);

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
