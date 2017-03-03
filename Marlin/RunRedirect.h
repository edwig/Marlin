/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: RunRedirect.h
//
// Marlin Component: Internet server/client
// 
// Copyright (c) 2017 ir. W.E. Huisman
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
#include "Redirect.h"

// Miliseconds wait loop
#define WAITTIME_STATUS 25

int  CallProgram_For_String(LPCSTR p_programma,LPCSTR p_commandLine,CString& p_result);
int  CallProgram_For_String(LPCSTR p_programma,LPCSTR p_commandLine,CString& p_result,int p_waittime);


class RunRedirect : public CRedirect
{
  // INCLUDE_CLASSNAME(RunBriefRun)
public:
   RunRedirect();
  ~RunRedirect();

  void RunCommand(LPCSTR p_commandLine);

  // Virtual interface. Derived class must implement this!!
  void OnChildStarted    (LPCSTR lpszCmdLine);
  void OnChildStdOutWrite(LPCSTR lpszOutput); 
  void OnChildStdErrWrite(LPCSTR lpszOutput);
  void OnChildTerminate  ();
  bool IsReady();
  bool IsEOF();

  bool    m_ready;
  CString m_lines;
private:
  void    Acquire();
  void    Release();

  CRITICAL_SECTION  m_criticalSection;
};
