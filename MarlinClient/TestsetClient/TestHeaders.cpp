/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: TestHeaders.cpp
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
#include "SOAPMessage.h"
#include "ConvertWideString.h"
#include <ServiceQuality.h>

int TestSOAPHeaders()
{
  int errors = 0;

  XString namesp(DEFAULT_NAMESPACE);
  XString action("FirstAction");
  SOAPMessage msg(namesp, action, SoapVersion::SOAP_12);

  msg.AddHeader("Content-Type","application/soap+xml; charset=UTF-8; action=\"FirstAction\"");

  XString charset = FindCharsetInContentType(msg.GetHeader("content-type"));
  XString checkac = FindFieldInHTTPHeader(msg.GetHeader("content-type"), "Action");

  errors += charset.CompareNoCase("utf-8") != 0;
  errors += checkac.Compare("FirstAction") != 0;

  // Test empty
  XString test1 = FindFieldInHTTPHeader(msg.GetHeader("DoNotExist"), "ABC");

  errors += !test1.IsEmpty();

  // SUMMARY OF THE TEST
  // --- "---------------------------------------------- - ------
  printf("SOAPAction in Content-Type for SOAP 1.2        : %s\n", errors == 0 ? "OK" : "ERROR");

  return errors;
}

int TestHTTPHeaders()
{
  int errors = 0;

  // Search case insensitive
  XString header("application/soap+xml; charset=UTF-8; action=\"FirstAction\"");
  XString newheader1 = SetFieldInHTTPHeader(header,"action", "http://test.marlin.org/Function");
  XString newheader2 = SetFieldInHTTPHeader(header,"charset","UTF16");
  XString newheader3 = SetFieldInHTTPHeader("first","field","The Value");
  XString newheader4 = SetFieldInHTTPHeader("first","field","something");
  errors += newheader1.Compare("application/soap+xml; charset=UTF-8; action=\"http://test.marlin.org/Function\"") != 0;
  errors += newheader2.Compare("application/soap+xml; charset=UTF16; action=\"FirstAction\"") != 0;
  errors += newheader3.Compare("first; field=\"The Value\"") != 0;
  errors += newheader4.Compare("first; field=something") != 0;

  // SUMMARY OF THE TEST
  // --- "---------------------------------------------- - ------
  printf("Setting and replacing fields in HTTP headers   : %s\n", errors == 0 ? "OK" : "ERROR");

  return errors;
}

int TestServiceQuality()
{
  int errors = 0;

  XString header = "text/*;q=0.3, text/html;q=0.7, text/html;level=1, text/html;level=2;q=0.4, */*;q=0.5";
  ServiceQuality serv(header);

  // Check first option
  QualityOption* opt = serv.GetOptionByPreference(0);
  if(opt)
  {
    errors += opt->m_field    .CompareNoCase("text/html") != 0;
    errors += opt->m_extension.CompareNoCase("level") != 0;
    errors += opt->m_value    .CompareNoCase("1") != 0;
  }
  else ++errors;

  // Check find by name
  int pref = serv.GetPreferenceByName("*/*");
  errors += pref != 50;

  XString second = serv.GetStringByPreference(1);
  errors += second.Compare("text/html");

  // SUMMARY OF THE TEST
  // --- "---------------------------------------------- - ------
  printf("Recognizing service quality in HTTP headers    : %s\n", errors == 0 ? "OK" : "ERROR");

  return errors;
}

int TestHeaders(void)
{
  int errors = 0;
  errors += TestSOAPHeaders();
  errors += TestHTTPHeaders();
  errors += TestServiceQuality();
  return errors;
}
