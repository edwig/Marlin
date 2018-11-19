/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: TestCookies.cpp
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

  CString text1 = msg.GetCookie("session");
  CString text2 = msg.GetCookie("SECRET","BLOKZIJL"); // SHOULD NOT WORK 

  int error1 = text1 != "456";
  int error2 = text2 != "";

  // SUMMARY OF THE TEST
  // --- "---------------------------------------------- - ------
  printf("Getting applciation cookie                     : %s\n",error1 ? "ERROR" : "OK");
  printf("Getting secured application cookie             : %s\n",error2 ? "ERROR" : "OK");

  return error1 + error2;
}

int
DoSend(HTTPClient& p_client,HTTPMessage* p_msg,CString p_expected)
{
  int  errors    = 0;
  bool encryptie = false;
  bool success   = false;
  
  // Setting the header to read all headers
  p_client.SetReadAllHeaders(true);
  
  success = p_client.Send(p_msg);
  if(success)
  {
    CString test = p_msg->GetCookieValue();
    if(test != "456")
    {
      success = false;
      ++errors;
    }
    Cookie* cookie = p_msg->GetCookie(0);
    if(cookie && cookie->GetName() == "session")
    {
      if(cookie->GetValue() != "456")
      {
        success = false;
        ++errors;
      }
    }
    cookie = p_msg->GetCookie(1);
    if(cookie && cookie->GetName() == "SECRET")
    {
      CString decrypt =  cookie->GetValue("BLOKZIJL");
      if(decrypt == "THIS IS THE BIG SECRET OF THE FAMILY HUISMAN")
      {
        encryptie = true;
      }
      else ++errors;
    } 
    else ++errors;

    // Check extra header
    bool extraHeader = false;
    CString dayOfTheWeek = p_msg->GetHeader("EdosHeader");
    if(dayOfTheWeek != "Thursday")
    {
      ++errors;
    }
    else extraHeader = true;
    // --- "---------------------------------------------- - ------
    printf("HTTP testing non-standard headers received     : %s\n",extraHeader ? "OK" : "ERROR");
  }
  else ++errors;

  // SUMMARY OF THE TEST
  // --- "---------------------------------------------- - ------
  printf("HTTPMessage cookie received                    : %s\n",success   ? "OK" : "ERROR");
  printf("HTTPMessage encrypted cookie received          : %s\n",encryptie ? "OK" : "ERROR");

  // Now test application secure cookies
  errors += DoTestApplication(p_msg);

  xprintf("\n");
  xprintf("READING ALL HTTP HEADERS FOR THIS SITE\n");
  xprintf("--------------------------------------\n");
  ResponseMap& all = p_client.GetAllHeadersMap();
  ResponseMap::iterator it = all.begin();
  while(it != all.end())
  {
    xprintf("%s : %s\n",it->first.GetString(),it->second.GetString());
    ++it;
  }
  xprintf("--------------------------------------\n");
  xprintf("\n");

  // Reset the all-headers reading
  p_client.SetReadAllHeaders(false);

  delete p_msg;
  return errors;
}

int 
TestCookies(HTTPClient& p_client)
{
  int errors = 0;

  // Standard values
  CString url;
  url.Format("http://%s:%d/MarlinTest/CookieTest/",MARLIN_HOST,TESTING_HTTP_PORT);

  // Test 1
  xprintf("TESTING STANDARD HTTP MESSAGE TO /MarlinTest/CookieTest/\n");
  xprintf("======================================================\n");
  HTTPMessage* msg = new HTTPMessage(HTTPCommand::http_put,url);
  msg->SetBody("<Test><Een>1</Een></Test>\n");
  msg->SetCookie("GUID","1-2-3-4-5-6-7-0-7-6-5-4-3-2-1");
  msg->SetCookie("BEAST","Monkey");

  // Set extra headers
  msg->AddHeader("EdosHeader","15-10-1959");

  errors += DoSend(p_client,msg,"456");
 
  return errors;
}

int 
TestCookie(HTTPMessage* p_msg,CString p_meta,CString p_expect)
{
  CString value = p_msg->GetCookieValue("SESSIONCOOKIE",p_meta);
  return value.Compare(p_expect) != 0;
}


int 
TestCookiesOverwrite()
{
  int errors = 0;

  // Standard values
  CString url;
  url.Format("http://%s:%d/MarlinTest/CookieTest/",MARLIN_HOST,TESTING_HTTP_PORT);

  // Test 1
  xprintf("TESTING HTTP COOKIES ON /MarlinTest/CookieTest/\n");
  xprintf("===============================================\n");
  HTTPMessage* msg = new HTTPMessage(HTTPCommand::http_put,url);

  msg->SetCookie("SESSIONCOOKIE","123456","meta",false,true);
  errors += TestCookie(msg,"meta","123456");

  msg->SetCookie("SESSIONCOOKIE","","",false,true);
  errors += TestCookie(msg,"","");

  msg->SetCookie("SESSIONCOOKIE","654321","",false,true);
  errors += TestCookie(msg,"","654321");

  delete msg;

  return errors;
}

