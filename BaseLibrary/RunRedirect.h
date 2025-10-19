/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: RunRedirect.h
//
// Marlin Component: Internet server/client
// 
// Created: 2014-2025 ir. W.E. Huisman
// MIT License
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
int  CallProgram           (LPCTSTR p_program,LPCTSTR p_commandLine,bool p_show = false);
int  CallProgram_For_String(LPCTSTR p_program,LPCTSTR p_commandLine,XString& p_result,bool p_show = false);
int  CallProgram_For_String(LPCTSTR p_program,LPCTSTR p_commandLine,XString& p_result,int p_waittime,bool p_show = false);
int  CallProgram_For_String(LPCTSTR p_program,LPCTSTR p_commandLine,LPCTSTR p_stdInput,XString& p_result,int p_waittime,bool p_show = false);
int  CallProgram_For_String(LPCTSTR p_program,LPCTSTR p_commandLine,LPCTSTR p_stdInput,XString& p_result,XString& p_errors,int p_waittime,bool p_show = false);

int  PosixCallProgram(const XString& p_directory
                     ,const XString& p_programma
                     ,const XString& p_commandLine
                     ,const XString& p_charset
                     ,const XString& p_stdin
                     ,      XString& p_stdout
                     ,      XString& p_stderror
                     ,      HWND     p_console        = NULL
                     ,      UINT     p_showWindow     = SW_HIDE
                     ,      BOOL     p_waitForIdle    = FALSE
                     ,      ULONG    p_maxRunningTime = INFINITE
                     ,RunRedirect**  p_redirect  = nullptr);

class RunRedirect : public Redirect
{
public:
  explicit RunRedirect(ULONG p_maxTime = INFINITE);
 ~RunRedirect();

  void RunCommand(LPTSTR p_commandLine,bool p_show);
  void RunCommand(LPTSTR p_commandLine,LPTSTR p_stdInput,bool p_show);
  void RunCommand(LPTSTR p_commandLine,HWND p_console,UINT p_showWindow,BOOL p_waitForInputIdle);

  // Virtual interface. Derived class must implement this!!
  virtual void OnChildStarted    (LPCTSTR lpszCmdLine) override;
  virtual void OnChildStdOutWrite(LPCTSTR lpszOutput)  override; 
  virtual void OnChildStdErrWrite(LPCTSTR lpszOutput)  override;
  virtual void OnChildTerminate() override;
  bool IsReady();
  bool IsEOF();
  bool IsErrorEOF();
  bool IsReadyAndEOF();

  HWND    m_console { NULL };
  bool    m_ready;
  LPTSTR  m_input;
  XString m_output;
  XString m_error;
private:
  void    FlushStdIn();
};
