/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: TestCookies.cpp
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
#include "GetLastErrorAsString.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

int
DoTestApplication(HTTPMessage* p_msg)
{
  SOAPMessage msg(p_msg);

  XString text1 = msg.GetCookie(_T("session"));
  XString text2 = msg.GetCookie(_T("SECRET"),_T("BLOKZIJL")); // SHOULD NOT WORK 

  int error1 = text1 != _T("456");
  int error2 = text2 != _T("");

  // SUMMARY OF THE TEST
  // --- "---------------------------------------------- - ------
  _tprintf(_T("Getting application cookie                     : %s\n"),error1 ? _T("ERROR") : _T("OK"));
  _tprintf(_T("Getting secured application cookie             : %s\n"),error2 ? _T("ERROR") : _T("OK"));

  return error1 + error2;
}

int
DoSend(HTTPClient& p_client,HTTPMessage* p_msg,XString /*p_expected*/)
{
  int  errors    = 0;
  bool encryptie = false;
  bool success   = false;
  
  // Sending and testing
  success = p_client.Send(p_msg);
  if(success)
  {
    XString test = p_msg->GetCookieValue();
    if(test != _T("456"))
    {
      success = false;
      ++errors;
    }
    const Cookie* cookie = p_msg->GetCookie(0);
    if(cookie && cookie->GetName() == _T("session"))
    {
      if(cookie->GetValue() != _T("456"))
      {
        success = false;
        ++errors;
      }
    }
    cookie = p_msg->GetCookie(1);
    if(cookie && cookie->GetName() == _T("SECRET"))
    {
      XString decrypt =  cookie->GetValue(_T("BLOKZIJL"));
      if(decrypt == _T("THIS IS THE BIG SECRET OF THE FAMILY HUISMAN"))
      {
        encryptie = true;
      }
      else ++errors;
    } 
    else ++errors;

    // Check extra header
    bool extraHeader = false;
    XString dayOfTheWeek = p_msg->GetHeader(_T("EdosHeader"));
    if(dayOfTheWeek != _T("Thursday"))
    {
      ++errors;
    }
    else extraHeader = true;
    // --- "---------------------------------------------- - ------
    _tprintf(_T("HTTP testing non-standard headers received     : %s\n"),extraHeader ? _T("OK") : _T("ERROR"));
  }
  else ++errors;

  // SUMMARY OF THE TEST
  // --- "---------------------------------------------- - ------
  _tprintf(_T("HTTPMessage cookie received                    : %s\n"),success   ? _T("OK") : _T("ERROR"));
  _tprintf(_T("HTTPMessage encrypted cookie received          : %s\n"),encryptie ? _T("OK") : _T("ERROR"));

  // Now test application secure cookies
  errors += DoTestApplication(p_msg);

  xprintf(_T("\n"));
  xprintf(_T("READING ALL HTTP HEADERS FOR THIS SITE\n"));
  xprintf(_T("--------------------------------------\n"));
  HeaderMap& all = p_client.GetResponseHeaders();
  HeaderMap::iterator it = all.begin();
  while(it != all.end())
  {
    xprintf(_T("%s : %s\n"),it->first.GetString(),it->second.GetString());
    ++it;
  }
  xprintf(_T("--------------------------------------\n"));
  xprintf(_T("\n"));

  delete p_msg;
  return errors;
}

int 
TestCookies(HTTPClient& p_client)
{
  int errors = 0;

  // Standard values
  XString url;
  url.Format(_T("http://%s:%d/MarlinTest/CookieTest/"),MARLIN_HOST,TESTING_HTTP_PORT);

  // Test 1
  xprintf(_T("TESTING STANDARD HTTP MESSAGE TO /MarlinTest/CookieTest/\n"));
  xprintf(_T("======================================================\n"));
  HTTPMessage* msg = new HTTPMessage(HTTPCommand::http_put,url);
  msg->AddHeader(_T("Content-type"),_T("text/xml; charset=utf-8"));
  msg->SetBody(_T("<Test><Een>1</Een></Test>\n"),_T("utf-8"));
  msg->SetCookie(_T("GUID"),_T("1-2-3-4-5-6-7-0-7-6-5-4-3-2-1"));
  msg->SetCookie(_T("BEAST"),_T("Monkey"));

  // Set extra headers
  msg->AddHeader(_T("EdosHeader"),_T("15-10-1959"));

  errors += DoSend(p_client,msg,_T("456"));
 
  return errors;
}
