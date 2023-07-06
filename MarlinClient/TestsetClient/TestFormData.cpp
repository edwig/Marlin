/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: TestFormData.cpp
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
#include "HTTPClient.h"
#include "HTTPMessage.h"
#include "SOAPMessage.h"
#include "JSONMessage.h"
#include "MultiPartBuffer.h"

// Beware Debugging starting directory MUST be "$(OutDir)"
XString file = "..\\Documentation\\HTML5-eventsource.js";

XString GetJsonString()
{
  // TEST ROUNDTRIP SOAP -> JSON
  XString namesp("http://test.marlin.org/interface");
  XString action("FirstAction");
  SOAPMessage msg(namesp,action);
  msg.SetParameter("First", 101);
  msg.SetParameter("Second",102);
  XMLElement* elem = msg.AddElement(NULL,"Third",(XmlDataType)XDT_String,"");
  if(elem)
  {
    msg.AddElement(elem,"Fortune",XDT_Integer,"1000000");
    msg.AddElement(elem,"Glory",  XDT_String, "Indiana Jones");
    msg.AddElement(elem,"Diacrit",XDT_String, "éáíóúëäïöüèàìòùêâîôû");
  }
  JSONMessage json(&msg);
  json.SetWhitespace(true);
  XString str = json.GetJsonMessage();

  return str;
}

int 
TestFormDataMP(HTTPClient* p_client)
{
  XString data = GetJsonString();

  MultiPartBuffer buffer(FormDataType::FD_MULTIPART);
  MultiPart* part1 = buffer.AddPart("json", "application/json",data);
  MultiPart* part2 = buffer.AddPart("empty","text/html",       ""); 
  MultiPart* part3 = buffer.AddFile("eventsource","application/js",file);

  // Test if we created all parts
  if(part1 == nullptr || part2 == nullptr || part3 == nullptr)
  {
    // --- "---------------------------------------------- - ------
    printf("MultiPartBuffer creation data+file check       : ERROR\n");
    return 1;
  }
  // Try to transport the filetimes to the server
  // BEWARE: Some servers do not respect the filetimes attributes
  //         or even crash on it (WCF .NET returns HTTP status 500)
  buffer.SetFileExtensions(true);

  XString url;
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
    XString error;
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
  MultiPartBuffer buffer(FormDataType::FD_URLENCODED);
  buffer.AddPart("one","text/html","Monkey-Nut-Tree");
  buffer.AddPart("two","text/html","Tree-Leaf-Root");
  buffer.AddPart("three","text/rtf","normal{\\b bold} and {\\i italic}");

  XString url;
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
    XString error;
    p_client->GetError(&error);
    // SUMMARY OF THE TEST
    // --- "--------------------------- - ------\n"
    printf("HTTP ERROR                  : %s\n",(LPCTSTR)p_client->GetStatusText());
    printf("HTTP CLIENT Message         : %s\n",(LPCTSTR)error);
  }
  return result ? 0 : 1;
}

int TestFD()
{
  XString filename("C:\\TEMP\\mpbuffer.txt");
  FILE* fp = nullptr;
  char buffer[4004];
  XString contenttype("multipart/form-data boundary=\"------WebKitFormBoundaryBFCjeYoxVSC92Luo\"");

  fopen_s(&fp, filename, "rb");
  if (fp)
  {
    fread(buffer, 1, 4000, fp);
    buffer[2063] = 0;
    FileBuffer fb;
    // fb.AddBuffer((uchar*)buffer, 2063);
    fb.SetBuffer((uchar*) buffer, 2063);

    MultiPartBuffer mpb(FormDataType::FD_UNKNOWN);
    mpb.ParseBuffer(contenttype, &fb);

    printf("Number of parts: %d\n",(int) mpb.GetParts());
  }
  return 0;
}

int TestFormData(HTTPClient* p_client)
{
  int errors = 0;

  errors += TestFD();
  errors += TestFormDataMP(p_client);
  errors += TestFormDataUE(p_client);

  return errors;
}
