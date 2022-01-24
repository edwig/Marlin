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

int TestHeaders(void)
{
  int errors = 0;

  CString namesp(DEFAULT_NAMESPACE);
  CString action("FirstAction");
  SOAPMessage msg(namesp,action,SoapVersion::SOAP_12);

  msg.AddHeader("Content-Type","application/soap+xml; charset=UTF-8; action=\"FirstAction\"");

  CString charset = FindCharsetInContentType(msg.GetHeader("content-type"));
  CString checkac = FindFieldInHTTPHeader(msg.GetHeader("content-type"),"Action");

  errors += charset.CompareNoCase("utf-8") != 0;
  errors += checkac.Compare("FirstAction") != 0;

  // Test empty
  CString test1 = FindFieldInHTTPHeader(msg.GetHeader("DoNotExist"),"ABC");

  errors += !test1.IsEmpty();

  // SUMMARY OF THE TEST
  // --- "---------------------------------------------- - ------
  printf("SOAPAction in Content-Type for SOAP 1.2        : %s\n", errors == 0 ? "OK" : "ERROR");

  return errors;
}
