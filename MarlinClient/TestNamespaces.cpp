/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: TestNamespaces.cpp
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
#include "Namespace.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

int Testsplit(CString p_soapAction,CString p_expect)
{
  int result = 0;
  CString namesp,action;
  printf("SOAPAction : %s\t",p_soapAction.GetString());
  if(SplitNamespaceAndAction(p_soapAction,namesp,action))
  {
    if(namesp == p_expect && action == "command")
    {
      printf("OK\n");
    }
    else
    {
      printf("Split namespace error: %s\n",p_soapAction.GetString());
      ++result;
    }
  }
  else
  {
    if(action != "command")
    {
      printf("Split namespace error: %s\n", p_soapAction.GetString());
      ++result;
    }
    else printf("OK\n");
  }
  return result;
}

int TestNamespaces(void)
{
  int errors = 0;
  CString left  ("http://Name.Test.lower\\something");
  CString right("https://NAME.test.LOWER/SomeThing/");

  printf("TESTING NAMESPACE FUNCTIONS:\n");
  printf("========================================================\n");
  xprintf("Left  namesp:  %s\n", left);
  xprintf("Right namesp: %s\n", right);
  xprintf("-------------------------------------------------------\n");
  int result = CompareNamespaces(left,right);
  printf("%s = %s\n",left.GetString(),result == 0 ? "OK" : "NAMESPACE ERROR");
  errors += result;

  printf("TESTING NAMESPACE + SOAP ACTION:\n");
  printf("========================================================\n");
  errors += Testsplit("\"http://server/uri/command/\"","http://server/uri");
  errors += Testsplit("http://server/uri/some#command","http://server/uri/some#");
  errors += Testsplit("command","");

  return errors;
}
