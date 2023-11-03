/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: Redirect.h
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
#include "AutoCritical.h"

// Command line length in NT technology
#define BUFFER_SIZE 8192
// Maximum wait 1 minute for input idle
#define MAXWAIT_FOR_INPUT_IDLE 60000
// After the process we must wait for the stdout to be completely read
#define DRAIN_STDOUT_MAXWAIT   10000
#define DRAIN_STDOUT_INTERVAL     50
// END-OF-TRANSMISSION is ASCII 4
#define EOT '\x4'

/////////////////////////////////////////////////////////////////////////////
// Redirect class

class Redirect
{
  // Construction
public:
  Redirect();
 ~Redirect();

  // Actual interface. Use these.
  BOOL StartChildProcess(LPTSTR lpszCmdLine,UINT uShowChildWindow = SW_HIDE,BOOL bWaitForInputIdle = FALSE);
  BOOL IsChildRunning() const;
  void TerminateChildProcess();
  int  WriteChildStdIn(PTCHAR lpszInput);
  void SetTimeoutIdle(ULONG p_timeout);
  void CloseChildStdIn();

  // Virtual interface. Derived class must implement this!!
  virtual void OnChildStarted    (PTCHAR lpszCmdLine) = 0;
  virtual void OnChildStdOutWrite(PTCHAR lpszOutput)  = 0;
  virtual void OnChildStdErrWrite(PTCHAR lpszOutput)  = 0;
  virtual void OnChildTerminate  ()                   = 0;

  mutable int m_exitCode;
  mutable int m_eof_input;
  mutable int m_eof_error;
  mutable int m_terminated;
protected:
  HANDLE m_hExitEvent;
  // Child input(stdin) & output(stdout, stderr) pipes
  HANDLE m_hStdIn, m_hStdOut, m_hStdErr;
  // Parent output(stdin) & input(stdout) pipe
  HANDLE m_hStdInWrite, m_hStdOutRead, m_hStdErrRead;
  // stdout, stderr write threads
  HANDLE m_hStdOutThread, m_hStdErrThread;
  // Monitoring thread
  HANDLE m_hProcessThread;
  // Child process handle
  HANDLE m_hChildProcess;
  // Max running time before we timeout on the child process
  ULONG  m_timeoutChild;
  // Max wait time for InputIdle status of the child process
  ULONG  m_timeoutIdle;

  HANDLE PrepAndLaunchRedirectedChild(PTCHAR lpszCmdLine
                                     ,HANDLE hStdOut
                                     ,HANDLE hStdIn
                                     ,HANDLE hStdErr
                                     ,UINT   uShowChildWindow  = SW_HIDE
                                     ,BOOL   bWaitForInputIdle = FALSE);

  BOOL m_bRunThread;
  static unsigned int WINAPI staticStdOutThread(void* pRedirect)
  { 
    Redirect* redir = reinterpret_cast<Redirect*>(pRedirect);
    return redir->StdOutThread(redir->m_hStdOutRead); 
  }
  static unsigned int WINAPI staticStdErrThread(void* pRedirect)
  { 
    Redirect* redir = reinterpret_cast<Redirect*>(pRedirect);
    return redir->StdErrThread(redir->m_hStdErrRead); 
  }
  static unsigned int WINAPI staticProcessThread(void* pRedirect)
  { 
    Redirect* redir = reinterpret_cast<Redirect*>(pRedirect);
    return redir->ProcessThread(); 
  }
  int StdOutThread(HANDLE hStdOutRead);
  int StdErrThread(HANDLE hStdErrRead);
  int ProcessThread();

protected:
  CRITICAL_SECTION m_critical;
};
