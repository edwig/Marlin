/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: RunRedirect.h
//
// Marlin Component: Internet server/client
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
#pragma once
#include "Redirect.h"

// Milliseconds wait loop
#define WAITTIME_STATUS 50

#define WM_CONSOLE_TITLE (WM_USER + 1)
#define WM_CONSOLE_TEXT  (WM_USER + 2)

class RunRedirect;

// All global 'CallProgram' variants
int  CallProgram           (LPCSTR p_program,LPCSTR p_commandLine,bool p_show = false);
int  CallProgram_For_String(LPCSTR p_program,LPCSTR p_commandLine,XString& p_result,bool p_show = false);
int  CallProgram_For_String(LPCSTR p_program,LPCSTR p_commandLine,XString& p_result,int p_waittime,bool p_show = false);
int  CallProgram_For_String(LPCSTR p_program,LPCSTR p_commandLine,LPCSTR p_stdInput,XString& p_result,int p_waittime,bool p_show = false);
int  CallProgram_For_String(LPCSTR p_program,LPCSTR p_commandLine,LPCSTR p_stdInput,XString& p_result,XString& p_errors,int p_waittime,bool p_show = false);

int  PosixCallProgram(XString  p_directory
                     ,XString  p_programma
                     ,XString  p_commandLine
                     ,XString  p_stdin
                     ,XString& p_stdout
                     ,XString& p_stderror
                     ,HWND     p_console        = NULL
                     ,UINT     p_showWindow     = SW_HIDE
                     ,BOOL     p_waitForIdle    = FALSE
                     ,ULONG    p_maxRunningTime = INFINITE
                     ,RunRedirect** p_redirect  = nullptr);

class RunRedirect : public Redirect
{
public:
  explicit RunRedirect(ULONG p_maxTime = INFINITE);
 ~RunRedirect();

  void RunCommand(LPCSTR p_commandLine,bool p_show);
  void RunCommand(LPCSTR p_commandLine,LPCSTR p_stdInput,bool p_show);
  void RunCommand(LPCSTR p_commandLine,HWND p_console,UINT p_showWindow,BOOL p_waitForInputIdle);

  // Virtual interface. Derived class must implement this!!
  virtual void OnChildStarted    (LPCSTR lpszCmdLine) override;
  virtual void OnChildStdOutWrite(LPCSTR lpszOutput)  override; 
  virtual void OnChildStdErrWrite(LPCSTR lpszOutput)  override;
  virtual void OnChildTerminate() override;
  bool IsReady();
  bool IsEOF();
  bool IsErrorEOF();
  bool IsReadyAndEOF();

  HWND    m_console { NULL };
  bool    m_ready;
  LPCTSTR m_input;
  XString m_output;
  XString m_error;
private:
  void    FlushStdIn();
};
