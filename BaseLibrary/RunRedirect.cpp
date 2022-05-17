/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: RunRedirect.cpp
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
#include "pch.h"
#include "RunRedirect.h"
#include <time.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

RunRedirect::RunRedirect(ULONG p_maxTime /*=INFINITE*/)
            :m_ready(false)
            ,m_input(nullptr)
{
  // Setting the maximum timeout only from this interval
  if(p_maxTime > 0 && p_maxTime < 0x7FFFFFFF)
  {
    m_timeoutChild = p_maxTime;
  }
  InitializeCriticalSection((LPCRITICAL_SECTION)&m_criticalSection);
}

RunRedirect::~RunRedirect()
{
  DeleteCriticalSection(&m_criticalSection);
}

void 
RunRedirect::RunCommand(LPCSTR p_commandLine)
{
  Acquire();
  StartChildProcess(p_commandLine,FALSE);
  Release();
}

void 
RunRedirect::RunCommand(LPCSTR p_commandLine,LPCSTR p_stdInput)
{
  Acquire();
  m_input = p_stdInput;
  StartChildProcess(p_commandLine,FALSE);
  Release();
}

void 
RunRedirect::RunCommand(LPCSTR p_commandLine,HWND p_console,UINT p_showWindow,BOOL p_waitForInputIdle)
{
  Acquire();
  m_console = p_console;
  if(StartChildProcess(p_commandLine,p_showWindow,p_waitForInputIdle) == FALSE)
  {
    // Do not continue waiting on the process
    m_ready = true;
  }
  Release();
}

void RunRedirect::OnChildStarted(LPCSTR /*lpszCmdLine*/) 
{
  Acquire();
  m_output.Empty();
  m_error.Empty();
  m_ready = false;
  FlushStdIn();
  Release();
}
void RunRedirect::OnChildStdOutWrite(LPCSTR lpszOutput) 
{
  Acquire();
  m_output += lpszOutput;
  if(m_console)
  {
    ::SendMessage(m_console,WM_CONSOLE_TEXT,0,(LPARAM)lpszOutput);
  }
  Release();
}

void 
RunRedirect::OnChildStdErrWrite(LPCSTR lpszOutput)
{
  Acquire();
  m_error += lpszOutput;
  if(m_console)
  {
    ::SendMessage(m_console,WM_CONSOLE_TEXT,1,(LPARAM)lpszOutput);
  }
  Release();
}

void RunRedirect::OnChildTerminate()
{
  Acquire();
  m_ready = true;
  Release();
}

bool RunRedirect::IsReady()
{
  Acquire();
  bool res = m_ready;
  Release();
  return res;
}

bool RunRedirect::IsEOF()
{
  Acquire();
  bool eof = m_eof_input > 0;
  Release();
  return eof;
}

void    
RunRedirect::Acquire()
{
  EnterCriticalSection(&m_criticalSection);
}

void    
RunRedirect::Release()
{
  LeaveCriticalSection(&m_criticalSection);
}

// Write to the STDIN after starting the program
// After writing we close (EOF) the input channel
void
RunRedirect::FlushStdIn()
{
  if(m_input)
  {
    if(WriteChildStdIn(m_input) == 0)
    {
      // Ready with the input channel
      CloseChildStdIn();
    }
    else
    {
      // Error. Stop as soon as possible
      m_ready = true;
    }
    m_input = nullptr;
  }
}

int 
CallProgram_For_String(LPCSTR p_program,LPCSTR p_commandLine,XString& p_result)
{
#ifdef _ATL
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
#endif
  RunRedirect run;

  XString commandLine;

  // Result is initially empty
  p_result.Empty();

  // Create a new command line
  commandLine.Format("\"%s\" %s",p_program,p_commandLine);

  run.RunCommand(commandLine.GetString());
  while((run.IsEOF() == false) && (run.IsReady() == false))
  {
    Sleep(WAITTIME_STATUS);
  }
  p_result = run.m_output;
  run.TerminateChildProcess();
  int exitcode = run.m_exitCode;
  return exitcode;
}

int
CallProgram_For_String(LPCSTR p_program,LPCSTR p_commandLine,LPCSTR p_stdInput,XString& p_result,int p_waittime)
{
#ifdef _ATL
  AFX_MANAGE_STATE(AfxGetStaticModuleState());
#endif
  RunRedirect run;

  XString commandLine;

  // Result is initially empty
  p_result = "";

  // Create a new command line
  commandLine.Format("\"%s\" %s",p_program,p_commandLine);

  clock_t start = clock();
  run.RunCommand(commandLine.GetString(),p_stdInput);
  while((run.IsEOF() == false) && (run.IsReady() == false))
  {
    Sleep(WAITTIME_STATUS);

    // Check if we are out of waittime
    clock_t now = clock();
    if((now - start) > p_waittime)
    {
      break;
    }
  }
  p_result = run.m_output;
  run.TerminateChildProcess();
  return run.m_exitCode;
}


int 
CallProgram_For_String(LPCSTR p_program,LPCSTR p_commandLine,XString& p_result,int p_waittime)
{
#ifdef _ATL
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
 #endif
  RunRedirect run;

  XString commandLine;

  // Result is initially empty
  p_result = "";

  // Create a new command line
  commandLine.Format("\"%s\" %s",p_program,p_commandLine);

  clock_t start = clock();
  run.RunCommand(commandLine.GetString());
  while((run.IsEOF() == false) && (run.IsReady() == false))
  {
    Sleep(WAITTIME_STATUS);

    // Check if we are out of waittime
    clock_t now = clock();
    if((now - start) > p_waittime)
    {
      break;
    }
  }
  p_result = run.m_output;
  run.TerminateChildProcess();
  return run.m_exitCode;
}

int 
CallProgram(LPCSTR p_program, LPCSTR p_commandLine)
{
#ifdef _ATL
  AFX_MANAGE_STATE(AfxGetStaticModuleState());
#endif
  RunRedirect run;
  XString commandLine;

  // Create a new command line
  commandLine.Format("\"%s\" %s",p_program,p_commandLine);

  run.RunCommand(commandLine.GetString());
  while ((run.IsEOF() == false) && (run.IsReady() == false))
  {
    Sleep(WAITTIME_STATUS);
  }
  return run.m_exitCode;
}

// Calling our program at last
int 
PosixCallProgram(XString  p_directory
                ,XString  p_programma
                ,XString  p_parameters
                ,XString  p_stdin
                ,XString& p_stdout
                ,XString& p_stderr
                ,HWND     p_console         /*= NULL    */
                ,UINT     p_showWindow      /*= SW_HIDE */
                ,BOOL     p_waitForIdle     /*= FALSE   */
                ,ULONG    p_maxRunningTime  /*= INFINITE*/
                ,RunRedirect** p_run        /*= nullptr */)
{
#ifdef _ATL
  AFX_MANAGE_STATE(AfxGetStaticModuleState());
#endif
  RunRedirect run(p_maxRunningTime);
  if(p_run)
  {
    *p_run = &run;
  }

  // Result is initially empty
  p_stdout.Empty();
  p_stderr.Empty();

  // Remove backslash
  p_directory.TrimRight('\\');

  // Create a new command line
  XString commandLine = p_directory + "\\" + p_programma;

  // Set console title
  if(p_console)
  {
    SendMessage(p_console,WM_CONSOLE_TITLE,0,(LPARAM)commandLine.GetString());
  }

  // Adding parameters
  commandLine  = "\"" + commandLine + "\" ";
  commandLine += p_parameters;

  // Start the command
  run.RunCommand(commandLine.GetString(),p_console,p_showWindow,p_waitForIdle);

  // Write to the standard input channel
  if(!p_stdin.IsEmpty())
  {
    run.WriteChildStdIn(p_stdin);
  }

  // Wait for the standard output/standard error to drain
  while((run.IsEOF() == false) && (run.IsReady() == false))
  {
    Sleep(WAITTIME_STATUS);
  }

  // Reset console title
  if (p_console)
  {
    SendMessage(p_console,WM_CONSOLE_TITLE,0,(LPARAM)"");
  }

  // Remember our output
  p_stdout = run.m_output;
  p_stderr = run.m_error;

  // And return the exit code
  return run.m_exitCode;
}

