/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: TestTime.cpp
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
#include "HTTPTime.h"
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

int  Test_HTTPTime()
{
  int errors = 0;
  SYSTEMTIME result;
  result.wDay       = 6;
  result.wDayOfWeek = 0;
  result.wYear      = 1994;
  result.wMonth     = 11;
  result.wHour      = 8;
  result.wMinute    = 49;
  result.wSecond    = 37;
  result.wMilliseconds = 0;

  // SUMMARY OF THE TEST
  // --- "--------------------------- - ------\n"
  qprintf("Test function HTTPTime      : ");

  // MS-Exchange server does this (non-conforming!) format
  CString string1("Sun, 06-Nov-1994 08:49:37 GMT");
  SYSTEMTIME time1;


  if(HTTPTimeToSystemTime(string1,&time1) == false)
  {
    qprintf("broken for MS-Exchange times!\n");
    xerror();
    ++errors;
  }
  if(memcmp(&result,&time1,sizeof(SYSTEMTIME)) != 0)
  {
    qprintf("wrong result for MS-Exchange times!\n");
    xerror();
    ++errors;
  }

  // Standard RFC822/RFC1123 time
  CString string2("Sun, 06 Nov 1994 08:49:37 GMT");
  SYSTEMTIME time2;

  if(HTTPTimeToSystemTime(string2,&time2) == false)
  {
    qprintf("broken for RFC 822 / RFC 1123 times!\n");
    xerror();
    ++errors;
  }
  if(memcmp(&result,&time2,sizeof(SYSTEMTIME)) != 0)
  {
    qprintf("wrong result for RFC 822/1123 times!\n");
    xerror();
    ++errors;
  }

  // Standard RFC 850 / RFC 1036 time
  CString string3("Sunday, 06 - Nov - 94 08:49:37 GMT");
  SYSTEMTIME time3;

  if(HTTPTimeToSystemTime(string3,&time3) == false)
  {
    qprintf("broken for RFC 850 / RFC 1036 times!\n");
    xerror();
    ++errors;
  }
  if(memcmp(&result,&time3,sizeof(SYSTEMTIME)) != 0)
  {
    qprintf("wrong result for RFC 850/1036 times!\n");
    xerror();
    ++errors;
  }

  // Standard ANSI C asctime format
  CString string4("Sun Nov  6 08:49:37 1994");
  SYSTEMTIME time4;

  if(HTTPTimeToSystemTime(string4,&time4) == false)
  {
    qprintf("broken for ANSI C asctime!\n");
    xerror();
    ++errors;
  }
  if(memcmp(&result,&time4,sizeof(SYSTEMTIME)) != 0)
  {
    qprintf("wrong result for ANSI C asctime!\n");
    xerror();
    ++errors;
  }

  // End reached with success
  if(errors == 0)
  {
    qprintf("OK\n");
  }
  return 0;
}

