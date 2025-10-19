/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: RunRedirect.cpp
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
#include "pch.h"
#include "RunRedirect.h"
#include "AutoCritical.h"
#include <time.h>

RunRedirect::RunRedirect(ULONG p_maxTime /*=INFINITE*/)
            :m_ready(false)
            ,m_input(nullptr)
{
  // Setting the maximum timeout only from this interval
  if(p_maxTime > 0 && p_maxTime < 0x7FFFFFFF)
  {
    m_timeoutChild = p_maxTime;
  }
}

RunRedirect::~RunRedirect()
{
}

void 
RunRedirect::RunCommand(LPTSTR p_commandLine,bool p_show)
{
  AutoCritSec lock((LPCRITICAL_SECTION)&m_critical);
  m_ready = StartChildProcess(p_commandLine,p_show ? SW_SHOW : SW_HIDE) == FALSE;
}

void 
RunRedirect::RunCommand(LPTSTR p_commandLine,LPTSTR p_stdInput,bool p_show)
{
  AutoCritSec lock(&m_critical);
  m_input = p_stdInput;
  m_ready = StartChildProcess(p_commandLine,p_show ? SW_SHOW : SW_HIDE,TRUE) == FALSE;
}

void 
RunRedirect::RunCommand(LPTSTR p_commandLine,HWND p_console,UINT p_showWindow,BOOL p_waitForInputIdle)
{
  AutoCritSec lock(&m_critical);
  m_console = p_console;
  m_ready   = StartChildProcess(p_commandLine,p_showWindow,p_waitForInputIdle) == FALSE;
}

void RunRedirect::OnChildStarted(LPCTSTR /*lpszCmdLine*/) 
{
  AutoCritSec lock(&m_critical);
  m_output.Empty();
  m_error.Empty();
  m_ready = false;
  FlushStdIn();
}

void RunRedirect::OnChildStdOutWrite(LPCTSTR lpszOutput) 
{
  AutoCritSec lock(&m_critical);
  m_output += lpszOutput;
  if(m_console)
  {
    ::SendMessage(m_console,WM_CONSOLE_TEXT,0,(LPARAM)lpszOutput);
  }
}

void 
RunRedirect::OnChildStdErrWrite(LPCTSTR lpszOutput)
{
  AutoCritSec lock(&m_critical);
  m_error += lpszOutput;
  if(m_console)
  {
    ::SendMessage(m_console,WM_CONSOLE_TEXT,1,(LPARAM)lpszOutput);
  }
}

void RunRedirect::OnChildTerminate()
{
  AutoCritSec lock(&m_critical);
  m_ready = true;

  // Write an END-OF-TRANSMISSION after the output, so the
  // Redirect scanner can stop reading
  if(m_hStdOut != NULL)
  {
    TCHAR buf[1] =  { EOT };
    ::WriteFile(m_hStdOut,&buf,sizeof(TCHAR),NULL,NULL);
  }
  if(m_hStdErr != NULL)
  {
    TCHAR buf[1] = { EOT };
    ::WriteFile(m_hStdErr,&buf,sizeof(TCHAR),NULL,NULL);
  }
}

bool RunRedirect::IsReady()
{
  AutoCritSec lock(&m_critical);

  return m_ready;
}

bool RunRedirect::IsEOF()
{
  AutoCritSec lock(&m_critical);
  return m_eof_input > 0;
}

bool RunRedirect::IsErrorEOF()
{
  AutoCritSec lock(&m_critical);
  return m_eof_error > 0;
}

bool RunRedirect::IsReadyAndEOF()
{
  AutoCritSec lock(&m_critical);

  return (m_ready && (m_eof_input > 0) && (m_eof_error > 0));
}

// Write to the STDIN after starting the program
// After writing we close (EOF) the input channel
void
RunRedirect::FlushStdIn()
{
  if(m_input)
  {
    if(WriteChildStdIn(m_input) != 0)
    {
      // Error. Stop as soon as possible
      m_ready = true;
      m_eof_input = 1;
      m_eof_error = 1;
    }
    // Ready with the input channel
    CloseChildStdIn();
    m_input = nullptr;
  }
}

int 
CallProgram_For_String(LPCTSTR p_program,LPCTSTR p_commandLine,XString& p_result,bool p_show /*= false*/)
{
#ifdef _AFX
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
#endif
  RunRedirect run;

  XString commandLine;

  // Result is initially empty
  p_result.Empty();

  // Create a new command line
  commandLine.Format(_T("\"%s\" %s"),p_program,p_commandLine);

  run.RunCommand(const_cast<LPTSTR>(commandLine.GetString()),p_show);
  while(!run.IsReadyAndEOF())
  {
    Sleep(WAITTIME_STATUS);
  }
  run.TerminateChildProcess();
  p_result = run.m_output;
  int exitcode = run.m_exitCode;
  return exitcode;
}

int
CallProgram_For_String(LPCTSTR p_program,LPCTSTR p_commandLine,LPTSTR p_stdInput,XString& p_result,int p_waittime,bool p_show /*= false*/)
{
#ifdef _AFX
  AFX_MANAGE_STATE(AfxGetStaticModuleState());
#endif
  RunRedirect run;

  XString commandLine;

  // Result is initially empty
  p_result = _T("");

  // Create a new command line
  commandLine.Format(_T("\"%s\" %s"),p_program,p_commandLine);

  clock_t start = clock();
  run.RunCommand(const_cast<LPTSTR>(commandLine.GetString()),p_stdInput,p_show);
  while(!run.IsReadyAndEOF())
  {
    Sleep(WAITTIME_STATUS);

    // Check if we are out of waittime
    clock_t now = clock();
    if((now - start) > p_waittime)
    {
      break;
    }
  }
  run.TerminateChildProcess();
  p_result = run.m_output;
  return run.m_exitCode;
}

int
CallProgram_For_String(LPCTSTR p_program,LPCTSTR p_commandLine,LPCTSTR p_stdInput,XString& p_result,XString& p_errors,int p_waittime,bool p_show /*= false*/)
{
#ifdef _AFX
  AFX_MANAGE_STATE(AfxGetStaticModuleState());
#endif
  RunRedirect run;

  XString commandLine;

  // Result is initially empty
  p_result = _T("");

  // Create a new command line
  commandLine.Format(_T("\"%s\" %s"),p_program,p_commandLine);

  clock_t start = clock();
  run.RunCommand(const_cast<LPTSTR>(commandLine.GetString())
                ,const_cast<LPTSTR>(p_stdInput)
                ,p_show);
  while(!run.IsReadyAndEOF())
  {
    Sleep(WAITTIME_STATUS);

    // Check if we are out of waittime
    clock_t now = clock();
    if((now - start) > p_waittime)
    {
      break;
    }
  }
  run.TerminateChildProcess();
  p_result = run.m_output;
  p_errors = run.m_error;
  return run.m_exitCode;
}

int
CallProgram_For_String(LPCTSTR p_program,LPCTSTR p_commandLine,XString& p_result,int p_waittime,bool p_show /*= false*/)
{
#ifdef _AFX
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
 #endif
  RunRedirect run;

  XString commandLine;

  // Result is initially empty
  p_result = _T("");

  // Create a new command line
  commandLine.Format(_T("\"%s\" %s"),p_program,p_commandLine);

  clock_t start = clock();
  run.RunCommand(const_cast<LPTSTR>(commandLine.GetString()),p_show);
  while(!run.IsReadyAndEOF())
  {
    Sleep(WAITTIME_STATUS);

    // Check if we are out of waittime
    clock_t now = clock();
    if((now - start) > p_waittime)
    {
      break;
    }
  }
  run.TerminateChildProcess();
  p_result = run.m_output;
  return run.m_exitCode;
}

int 
CallProgram(LPCTSTR p_program,LPCTSTR p_commandLine,bool p_show /*= false*/)
{
#ifdef _AFX
  AFX_MANAGE_STATE(AfxGetStaticModuleState());
#endif
  RunRedirect run;
  XString commandLine;

  // Create a new command line
  commandLine.Format(_T("\"%s\" %s"),p_program,p_commandLine);

  run.RunCommand(const_cast<LPTSTR>(commandLine.GetString()),p_show);
  while(!run.IsReadyAndEOF())
  {
    Sleep(WAITTIME_STATUS);
  }
  run.TerminateChildProcess();
  return run.m_exitCode;
}

// Calling our program at last
int 
PosixCallProgram(const XString& p_directory
                ,const XString& p_programma
                ,const XString& p_parameters
                ,const XString& p_charset
                ,const XString& p_stdin
                ,      XString& p_stdout
                ,      XString& p_stderr
                ,      HWND     p_console         /*= NULL    */
                ,      UINT     p_showWindow      /*= SW_HIDE */
                ,      BOOL     p_waitForIdle     /*= FALSE   */
                ,      ULONG    p_maxRunningTime  /*= INFINITE*/
                ,RunRedirect**  p_run             /*= nullptr */)
{
#ifdef _AFX
  AFX_MANAGE_STATE(AfxGetStaticModuleState());
#endif
  RunRedirect run(p_maxRunningTime);
  if(p_run)
  {
    *p_run = &run;
  }
  if(!p_charset.IsEmpty())
  {
    run.SetStreamCharset(p_charset);
  }
  // Result is initially empty
  p_stdout.Empty();
  p_stderr.Empty();

  // Remove backslash
  bool slash = (p_directory.Right(1) == _T("\\"));

  // Create a new command line
  XString commandLine = p_directory + (slash ? _T("") : _T("\\")) + p_programma;

  // Set console title
  if(p_console)
  {
    SendMessage(p_console,WM_CONSOLE_TITLE,0,(LPARAM)commandLine.GetString());
  }

  // Adding parameters
  commandLine  = XString(_T("\"")) + commandLine + _T("\" ");
  commandLine += p_parameters;

  // Start the command
  run.RunCommand(const_cast<PTCHAR>(commandLine.GetString()),p_console,p_showWindow,p_waitForIdle);

  // Write to the standard input channel
  if(!p_stdin.IsEmpty())
  {
    run.WriteChildStdIn(const_cast<PTCHAR>(p_stdin.GetString()));
  }

  // Wait for the standard output/standard error to drain
  while(!run.IsReadyAndEOF())
  {
    Sleep(WAITTIME_STATUS);
  }
  run.TerminateChildProcess();

  // Reset console title
  if(p_console)
  {
    SendMessage(p_console,WM_CONSOLE_TITLE,0,(LPARAM)_T(""));
  }

  // Remember our output
  p_stdout = run.m_output;
  p_stderr = run.m_error;

  // Reset the RunRedirect pointer for our caller!
  if(p_run)
  {
    *p_run = nullptr;
  }

  // And return the exit code
  return run.m_exitCode;
}
