/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: TestFormData.cpp
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
#include "HTTPClient.h"
#include "HTTPMessage.h"
#include "SOAPMessage.h"
#include "MultiPartBuffer.h"

CString file = "..\\Documentation\\HTML5-eventsource.js";

CString GetJsonString()
{
  // TEST ROUNDTRIP SOAP -> JSON
  CString namesp("http://test.marlin.org/interface");
  CString action("FirstAction");
  SOAPMessage msg(namesp,action);
  msg.SetParameter("First", 101);
  msg.SetParameter("Second",102);
  XMLElement* elem = msg.AddElement(NULL,"Third",(XmlDataType)XDT_String,"");
  if(elem)
  {
    msg.AddElement(elem,"Fortune",XDT_Integer,"1000000");
    msg.AddElement(elem,"Glory",  XDT_String, "Indiana Jones");
  }
  JSONMessage json(&msg);
  json.SetWhitespace(true);
  CString str = json.GetJsonMessage();

  return str;
}

int 
TestFormDataMP(HTTPClient* p_client)
{
  CString data = GetJsonString();

  MultiPartBuffer buffer(FD_MULTIPART);
  buffer.AddPart("json", "application/json",data);
  buffer.AddPart("empty","text/html",       ""); 
  buffer.AddFile("..\\Documentation\\eventsource.js","application/js",file);
  // Try to transport the filetimes to the server
  // BEWARE: Some servers do not respect the filetimes attributes
  //         or even crash on it (WCF .NET returns HTTP status 500)
  buffer.SetFileExtensions(true);

  CString url;
  url.Format("http://%s:%d/MarlinTest/FormData",MARLIN_HOST,TESTING_HTTP_PORT);
  HTTPMessage msg(HTTPCommand::http_post,url);
  msg.SetMultiPartFormData(&buffer);

  xprintf("TESTING FORM-DATA MULTPART BUFFER FUNCTION TO /MarlinTest/FormData\n");
  xprintf("==================================================================\n");
  bool result = p_client->Send(&msg);

  // SUMMARY OF THE TEST
  // --- "---------------------------------------------- - ------
  printf("Send: Multi-part form-data message             : %s\n",result ? "OK" : "ERROR");

  if(result == false)
  {
    CString error;
    p_client->GetError(&error);
    // SUMMARY OF THE TEST
    // --- "--------------------------- - ------\n"
    printf("HTTP ERROR                  : %s\n",(LPCTSTR)p_client->GetStatusText());
    printf("HTTP CLIENT Message         : %s\n",(LPCTSTR)error);
  }
  return result ? 0 : 1;
}

int 
TestFormDataUE(HTTPClient* p_client)
{
  MultiPartBuffer buffer(FD_URLENCODED);
  buffer.AddPart("one","text/html","Monkey-Nut-Tree");
  buffer.AddPart("two","text/html","Tree-Leaf-Root");
  buffer.AddPart("three","text/rtf","normal{\\b bold} and {\\i italic}");

  CString url;
  url.Format("http://%s:%d/MarlinTest/FormData",MARLIN_HOST,TESTING_HTTP_PORT);
  HTTPMessage msg(HTTPCommand::http_post,url);
  msg.SetMultiPartFormData(&buffer);

  xprintf("TESTING FORM-DATA URLENCODED BUFFER FUNCTION TO /MarlinTest/FormData\n");
  xprintf("====================================================================\n");
  bool result = p_client->Send(&msg);

  // SUMMARY OF THE TEST
  // --- "---------------------------------------------- - ------
  printf("Send: Multipart urlencoded message             : %s\n",result ? "OK" : "ERROR");

  if(result == false)
  {
    CString error;
    p_client->GetError(&error);
    // SUMMARY OF THE TEST
    // --- "--------------------------- - ------\n"
    printf("HTTP ERROR                  : %s\n",(LPCTSTR)p_client->GetStatusText());
    printf("HTTP CLIENT Message         : %s\n",(LPCTSTR)error);
  }
  return result ? 0 : 1;
}

int TestFormData(HTTPClient* p_client)
{
  int errors = 0;

  errors += TestFormDataMP(p_client);
  errors += TestFormDataUE(p_client);

  return errors;
}
