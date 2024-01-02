/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: TestCrackURL.cpp
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
#include "Stdafx.h"
#include "TestMarlinServer.h"
#include "CrackURL.h"
#include "WinHttp.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static int totalChecks = 5;

//////////////////////////////////////////////////////////////////////////
//
// Testframe
//
//////////////////////////////////////////////////////////////////////////

int  
TestMarlinServer::TestCrackURL()
{
  // SUMMARY OF THE TEST
  // --- "--------------------------- - ------\n"
  qprintf(_T("Test function CrackURL      : <+>"));

  CrackedURL url1(_T("http://server/index.html"));
  CrackedURL url2(_T("http://server:2108/path1/path2/pathname.pdf?val1=monkey&val2=nut&val3=mies#my_anchor"));
  CrackedURL url3(_T("http://server:2108/path1/path2/pathname.pdf?value"));

  bool res1 = url1.Valid();
  bool res2 = url2.Valid();
  if(!res1 || !res2)
  {
    qprintf(_T("broken. returns false. FixMe\n"));
    xerror();
    return 1;
  }
  --totalChecks;

  if(url2.m_scheme   != _T("http")     ||
     url2.m_host     != _T("server")   ||
     url2.m_port     != 2108       ||
     url2.m_path     != _T("/path1/path2/pathname.pdf") ||
     url2.m_anchor   != _T("my_anchor")  )
  {
    qprintf(_T("broken. Wrong parts. FixMe\n"));
    xerror();
    return 1;
  }
  --totalChecks;
  if(url2.m_parameters.size() == 3)
  {
    if(url2.m_parameters[0].m_key   != _T("val1")   ||
       url2.m_parameters[1].m_key   != _T("val2")   ||
       url2.m_parameters[2].m_key   != _T("val3")   ||
       url2.m_parameters[0].m_value != _T("monkey") ||
       url2.m_parameters[1].m_value != _T("nut")    ||
       url2.m_parameters[2].m_value != _T("mies")  )
    {
      qprintf(_T("broken. Wrong parameters. FixMe\n"));
      xerror();
      return 1;
    }
  }
  else
  {
    qprintf(_T("broken. Nr. of parameters is not 3. Fix me\n"));
    xerror();
    return 1;
  }
  --totalChecks;
  if(url3.m_parameters.size() == 1)
  {
    if(url3.m_parameters[0].m_key   != _T("value") ||
       url3.m_parameters[0].m_value != _T("")      )
    {
      qprintf(_T("Broken. Parameter without value doesn't work. FixMe\n"));
      xerror();
    }
  }
  else
  {
    qprintf(_T("Broken. Parameter without value doesn't work. FixMe\n"));
    xerror();
  }
  --totalChecks;
  CrackedURL url4(_T("https://server/test.html"));
  bool res3 = url4.Valid();
  if(!res3 || url4.m_port != INTERNET_DEFAULT_HTTPS_PORT)
  {
    qprintf(_T("broken. No default HTTPS port. Fix me\n"));
    xerror();
    return 1;
  }
  --totalChecks;
  qprintf(_T("OK\n"));
  return 0;
}

int
TestMarlinServer::AfterTestCrackURL()
{
  // SUMMARY OF THE TEST
  // ---- "---------------------------------------------- - ------
  qprintf(_T("CrackURL to path/parameters/anchor             : %s\n"),totalChecks > 0 ? _T("ERROR") : _T("OK"));
  return totalChecks > 0;
}
