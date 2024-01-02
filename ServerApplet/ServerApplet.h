/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: ServerApplet.cpp
//
// Marlin Component: Internet server/client
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
#pragma once

#include "resource.h"       // main symbols

extern LPCTSTR PROGRAM_NAME;     // The current program
extern LPCTSTR PRODUCT_NAME;     // Our server executable / DLL
extern LPCTSTR PRODUCT_SITE;     // our default product site

class ServerAppletApp : public CWinApp
{
public:
	ServerAppletApp();

  // Overrides
	public:
	BOOL    InitInstance();
  int     StartProgram(XString&  p_program
                      ,XString&  p_arguments
                      ,bool      p_currentdir
                      ,XString&  p_errormessage);
  // Implementation
	DECLARE_MESSAGE_MAP()

private:
  BOOL ParseCommandLine();
};

extern ServerAppletApp theApp;
