/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: MarlinServerAppFactory.h
//
// Marlin Server: Internet server/client
// 
// Copyright (c) 2014-2022 ir. W.E. Huisman
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
#include "MarlinServer.h"
#include "TestMarlinServerAppFactory.h"
#include "TestMarlinServerApp.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// The one and only app-factory for creating server apps
TestMarlinServerAppFactory factory;

// Generally this will only create ONE object
//
ServerApp* 
TestMarlinServerAppFactory::CreateServerApp(IHttpServer* p_iis
                                           ,LPCTSTR      p_webroot
                                           ,LPCTSTR      p_appName)
{
  static TestMarlinServerAppPool* theOne = nullptr;

  LoadConstants(_T("MarlinServer"));

  // Generally we want only ONE app. Here we have one app and three sites
  if(theOne == nullptr)
  {
    theOne = new TestMarlinServerAppPool(p_iis, p_webroot, p_appName);
  }
  return theOne;
}
