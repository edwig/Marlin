/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: TestFormData.cpp
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
#include "HTTPClient.h"
#include "HTTPMessage.h"
#include "SOAPMessage.h"
#include "JSONMessage.h"
#include "MultiPartBuffer.h"

// Beware Debugging starting directory MUST be "$(OutDir)"
XString file = _T("..\\Documentation\\HTML5-eventsource.js");

XString GetJsonString()
{
  // TEST ROUNDTRIP SOAP -> JSON
  XString namesp(_T("http://test.marlin.org/interface"));
  XString action(_T("FirstAction"));
  SOAPMessage msg(namesp,action);
  msg.SetParameter(_T("First"), 101);
  msg.SetParameter(_T("Second"),102);
  XMLElement* elem = msg.AddElement(NULL,_T("Third"),(XmlDataType)XDT_String,_T(""));
  if(elem)
  {
    msg.AddElement(elem,_T("Fortune"),XDT_Integer,_T("1000000"));
    msg.AddElement(elem,_T("Glory"),  XDT_String, _T("Indiana Jones"));
    msg.AddElement(elem,_T("Diacrit"),XDT_String, _T("éáíóúëäïöüèàìòùêâîôû"));
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
  const MultiPart* part1 = buffer.AddPart(_T("json"), _T("application/json"),data);
  const MultiPart* part2 = buffer.AddPart(_T("empty"),_T("text/html"),       _T("")); 
  const MultiPart* part3 = buffer.AddFile(_T("eventsource"),_T("application/js"),file);

  // Test if we created all parts
  if(part1 == nullptr || part2 == nullptr || part3 == nullptr)
  {
    // --- "---------------------------------------------- - ------
    _tprintf(_T("MultiPartBuffer creation data+file check       : ERROR\n"));
    return 1;
  }
  // Try to transport the filetimes to the server
  // BEWARE: Some servers do not respect the filetimes attributes
  //         or even crash on it (WCF .NET returns HTTP status 500)
  buffer.SetFileExtensions(true);

  XString url;
  url.Format(_T("http://%s:%d/MarlinTest/FormData"),MARLIN_HOST,TESTING_HTTP_PORT);
  HTTPMessage msg(HTTPCommand::http_post,url);
  msg.SetMultiPartFormData(&buffer);

  xprintf(_T("TESTING FORM-DATA MULTPART BUFFER FUNCTION TO /MarlinTest/FormData\n"));
  xprintf(_T("==================================================================\n"));
  bool result = p_client->Send(&msg);

  // SUMMARY OF THE TEST
  // --- "---------------------------------------------- - ------
  _tprintf(_T("Send: Multi-part form-data message             : %s\n"),result ? _T("OK") : _T("ERROR"));

  if(result == false)
  {
    XString error;
    p_client->GetError(&error);
    // SUMMARY OF THE TEST
    // --- "--------------------------- - ------\n"
    _tprintf(_T("HTTP ERROR                  : %s\n"),p_client->GetStatusText().GetString());
    _tprintf(_T("HTTP CLIENT Message         : %s\n"),error.GetString());
  }
  return result ? 0 : 1;
}

int 
TestFormDataUE(HTTPClient* p_client)
{
  MultiPartBuffer buffer(FormDataType::FD_URLENCODED);
  buffer.AddPart(_T("one"),_T("text/html"),_T("Monkey-Nut-Tree"));
  buffer.AddPart(_T("two"),_T("text/html"),_T("Tree-Leaf-Root"));
  buffer.AddPart(_T("three"),_T("text/rtf"),_T("normal{\\b bold} and {\\i italic}"));

  XString url;
  url.Format(_T("http://%s:%d/MarlinTest/FormData"),MARLIN_HOST,TESTING_HTTP_PORT);
  HTTPMessage msg(HTTPCommand::http_post,url);
  msg.SetMultiPartFormData(&buffer);

  xprintf(_T("TESTING FORM-DATA URLENCODED BUFFER FUNCTION TO /MarlinTest/FormData\n"));
  xprintf(_T("====================================================================\n"));
  bool result = p_client->Send(&msg);

  // SUMMARY OF THE TEST
  // --- "---------------------------------------------- - ------
  _tprintf(_T("Send: Multipart urlencoded message             : %s\n"),result ? _T("OK") : _T("ERROR"));

  if(result == false)
  {
    XString error;
    p_client->GetError(&error);
    // SUMMARY OF THE TEST
    // --- "--------------------------- - ------\n"
    _tprintf(_T("HTTP ERROR                  : %s\n"),p_client->GetStatusText().GetString());
    _tprintf(_T("HTTP CLIENT Message         : %s\n"),error.GetString());
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
