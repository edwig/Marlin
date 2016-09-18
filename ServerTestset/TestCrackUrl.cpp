/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: TestCrackURL.cpp
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
#include "Stdafx.h"
#include "TestServer.h"
#include "CrackURL.h"
#include "WinHttp.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//////////////////////////////////////////////////////////////////////////
//
// Testframe
//
//////////////////////////////////////////////////////////////////////////

int  Test_CrackURL()
{
  // SUMMARY OF THE TEST
  // --- "--------------------------- - ------\n"
  qprintf("Test function CrackURL      : <+>");

  CrackedURL url1("http://server/index.html");
  CrackedURL url2("http://edwig:password@server:2108/path1/path2/pathname.pdf?val1=monkey&val2=nut&val3=mies#my_anchor");
  CrackedURL url3("http://edwig:password@server:2108/path1/path2/pathname.pdf?value");

  bool res1 = url1.Valid();
  bool res2 = url2.Valid();
  if(!res1 || !res2)
  {
    qprintf("broken. returns false. FixMe\n");
    xerror();
    return 1;
  }
  if(url2.m_scheme   != "http"     ||
     url2.m_userName != "edwig"    ||
     url2.m_password != "password" ||
     url2.m_host     != "server"   ||
     url2.m_port     != 2108       ||
     url2.m_path     != "/path1/path2/pathname.pdf" ||
     url2.m_anchor   != "my_anchor"  )
  {
    qprintf("broken. Wrong parts. FixMe\n");
    xerror();
    return 1;
  }
  if(url2.m_parameters.size() == 3)
  {
    if(url2.m_parameters[0].m_key   != "val1"   ||
       url2.m_parameters[1].m_key   != "val2"   ||
       url2.m_parameters[2].m_key   != "val3"   ||
       url2.m_parameters[0].m_value != "monkey" ||
       url2.m_parameters[1].m_value != "nut"    ||
       url2.m_parameters[2].m_value != "mies"  )
    {
      qprintf("broken. Wrong parameters. FixMe\n");
      xerror();
      return 1;
    }
  }
  else
  {
    qprintf("broken. Nr. of parameters is not 3. Fix me\n");
    xerror();
    return 1;
  }
  if(url3.m_parameters.size() == 1)
  {
    if(url3.m_parameters[0].m_key   != "value" ||
       url3.m_parameters[0].m_value != ""      )
    {
      qprintf("Broken. Parameter without value doesn't work. FixMe\n");
      xerror();
    }
  }
  else
  {
    qprintf("Broken. Parameter without value doesn't work. FixMe\n");
    xerror();
  }
  CrackedURL url4("https://server/test.html");
  bool res3 = url4.Valid();
  if(!res3 || url4.m_port != INTERNET_DEFAULT_HTTPS_PORT)
  {
    qprintf("broken. No default HTTPS port. Fix me\n");
    xerror();
    return 1;
  }
  qprintf("OK\n");
  return 0;
}

