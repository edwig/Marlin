/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: RunRedirect.cpp
//
// Marlin Component: Internet server/client
// 
// Copyright (c) 2014-2021 ir. W.E. Huisman
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
#include "RunRedirect.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

RunRedirect::RunRedirect()
            :m_ready(false)
            ,m_input(nullptr)
{
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

void RunRedirect::OnChildStarted(LPCSTR /*lpszCmdLine*/) 
{
  Acquire();
  m_lines = "";
  m_ready = false;
  FlushStdIn();
  Release();
}
void RunRedirect::OnChildStdOutWrite(LPCSTR lpszOutput) 
{
  CString standard(lpszOutput);
  Acquire();
  m_lines += standard;
  Release();
}

void 
RunRedirect::OnChildStdErrWrite(LPCSTR lpszOutput)
{
  CString standard(lpszOutput);
  Acquire();
  m_lines += standard;
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
CallProgram_For_String(LPCSTR p_program,LPCSTR p_commandLine,CString& p_result)
{
#ifndef MARLIN_USE_ATL_ONLY
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
#endif
  RunRedirect run;

  CString commandLine;

  // Result is initially empty
  p_result.Empty();

  // Create a new command line
  commandLine.Format("\"%s\" %s",p_program,p_commandLine);

  run.RunCommand(commandLine.GetString());
  while((run.IsEOF() == false) && (run.IsReady() == false))
  {
    Sleep(WAITTIME_STATUS);
  }
  p_result = run.m_lines;
  run.TerminateChildProcess();
  int exitcode = run.m_exitCode;
  return exitcode;
}

int
CallProgram_For_String(LPCSTR p_program,LPCSTR p_commandLine,LPCSTR p_stdInput,CString& p_result,int p_waittime)
{
#ifndef MARLIN_USE_ATL_ONLY
  AFX_MANAGE_STATE(AfxGetStaticModuleState());
#endif
  RunRedirect run;

  CString commandLine;

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
  p_result = run.m_lines;
  run.TerminateChildProcess();
  int exitcode = run.m_exitCode;
  return exitcode;
}


int 
CallProgram_For_String(LPCSTR p_program,LPCSTR p_commandLine,CString& p_result,int p_waittime)
{
#ifndef MARLIN_USE_ATL_ONLY
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
 #endif
  RunRedirect run;

  CString commandLine;

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
  p_result = run.m_lines;
  run.TerminateChildProcess();
  int exitcode = run.m_exitCode;
  return exitcode;
}

int 
CallProgram(LPCSTR p_program, LPCSTR p_commandLine)
{
#ifndef MARLIN_USE_ATL_ONLY
  AFX_MANAGE_STATE(AfxGetStaticModuleState());
#endif
  RunRedirect run;
  CString commandLine;

  // Create a new command line
  commandLine.Format("\"%s\" %s",p_program,p_commandLine);

  run.RunCommand(commandLine.GetString());
  while ((run.IsEOF() == false) && (run.IsReady() == false))
  {
    Sleep(WAITTIME_STATUS);
  }
  return run.m_exitCode;
}
