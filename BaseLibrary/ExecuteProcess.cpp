/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: ExecuteProcess.cpp
//
// BaseLibrary: Indispensable general objects and functions
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
#include "ExecuteProcess.h"
#include "GetLastErrorAsString.h"
#include "GetExePath.h"
#include <io.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// STARTING AN OS PROGRAM
//
// Parameters                   Use of the parameter
// ---------------------------- ---------------------------------------------------------
// p_program                    Name of executable (relative or absolute path name)
//                              If empty, p_arguments start with the absolute program name
// p_arguments                  String with all the arguments to the program (can be empty)
// p_currentdir                 true  -> p_program is relative path name to executable
//                              false -> p_program is absolute path name
// p_errormessage               Prefix of the error message to return (or an empty string)
// p_showWindow                 MS-Windows SDK code to show a standard window
// p_waitForExit                Wait until the program is ready (exits) and record return value
// p_waitForIdle                Wait until Windows class enters the OnIdle cycle (program ready for input)
// p_factor                     > 0  -> Factor to multiply the timeout with (for large batches)
//                              0    -> Factor of 1 timeout
//                              < 0  -> Wait INFINITE time for program (e.g. interactive programs)
// p_threadID                   Resulting thread ID of the process (if we do not wait for exit)
//
// RETURN VALUE:
//
// Positive value               Result of the program
// Negative value               ERROR -> Evaluate / process / show the "p_foutboodschap"
//                              See the error codes in the *.h interface file!!
//
int ExecuteProcess(XString  p_program
                  ,XString  p_arguments
                  ,bool     p_currentdir
                  ,XString& p_errormessage                  // Prefix IN, Full error OUT
                  ,int      p_showWindow  /*= SW_HIDE */    // SW_SHOW / SW_HIDE etc
                  ,bool     p_waitForExit /*= false   */
                  ,bool     p_waitForIdle /*= false   */
                  ,unsigned p_factor      /*= 1       */
                  ,DWORD*   p_threadID    /*= NULL    */)
{
  XString program = p_program;
  if(p_currentdir)
  {
    XString path = GetExePath();
    program = path + p_program;

    if(_taccess(program,04) == -1)
    {
      // We cannot find the program file
      p_errormessage.AppendFormat(_T("\nCannot find the program file: '%s'"),p_program.GetString());
      return EXECUTE_NO_PROGRAM_FILE;
    }
  }
  PROCESS_INFORMATION	processInfo;
  ZeroMemory(&processInfo,sizeof(processInfo));
  STARTUPINFO	startupInfo;
  ZeroMemory(&startupInfo,sizeof(startupInfo));
  startupInfo.cb          = sizeof(startupInfo);
  startupInfo.dwFlags     = STARTF_USESHOWWINDOW;
  startupInfo.wShowWindow = (WORD) p_showWindow;

  // Construct program parameter and command-line
  // For CreateProcess it must be a buffer and in writable memory too.
  // The buffer **MUST** provide for extra space, as CreateProcess may alter the contents!!
  LPCTSTR lpProgram = program.GetString();
  TCHAR commandLine[MAX_COMMANDLINE];

  if(p_program.IsEmpty())
  {
    // Program must now be in the arguments list
    lpProgram = NULL;
    _tcscpy_s(commandLine,MAX_COMMANDLINE,p_arguments);
  }
  else
  {
    _stprintf_s(commandLine,MAX_COMMANDLINE,_T("\"%s\" %s"),lpProgram,p_arguments.GetString());
  }
  // Create the process and catch the return value if this succeeded
  BOOL res = CreateProcess(lpProgram              // Program to start or NULL
                          ,commandLine            // Command line with program or arguments only
                          ,NULL                   // Security
                          ,NULL                   // ThreadAttributes
                          ,FALSE                  // Inherit handles
                          ,NORMAL_PRIORITY_CLASS  // Priority
                          ,NULL                   // Environment
                          ,NULL            	      // Current directory
                          ,&startupInfo           // Startup info
                          ,&processInfo           // process info
                          );
  int exitCode = EXECUTE_NOT_STARTED;
  if(res)
  {
    // Reset exit code
    exitCode = EXECUTE_OK;

    // Give the new process the right to set the foreground window
    // Practical for all process started from the system tray
    AllowSetForegroundWindow(processInfo.dwProcessId);

    // Wait until the new process can handle input from other programs
    if(p_waitForIdle)
    {
      if(WaitForInputIdle(processInfo.hProcess,WAIT_FOR_INPUT_IDLE))
      {
        p_errormessage.AppendFormat(_T("Waiting too long on the new program: %s\n"),p_program.GetString());
        exitCode = EXECUTE_NO_INPUT_IDLE;
        p_waitForExit = false;
      }
    }
    // Waiting until the program is finished?
    if(p_waitForExit)
    {
      // Calculate our waiting time for the process
      // Large process could have a waiting factor (e.g. PDF/A conversions)
      DWORD waiting = WAIT_FOR_PROCESS_EXIT;
      if (p_factor == 0)     p_factor = 1;
      if (p_factor > 100000) p_factor = 100000;
      waiting *= p_factor;

      if(WaitForSingleObject(processInfo.hProcess,waiting) == WAIT_OBJECT_0)
      {
        GetExitCodeProcess(processInfo.hProcess,reinterpret_cast<LPDWORD>(&exitCode));
      }
      else
      {
        p_errormessage.AppendFormat(_T("\nABORT: Waited too long on the program to finish: %s"),p_program.GetString());
        exitCode = EXECUTE_TIMEOUT;
      }
    }
    // Keep the thread ID for the calling function if so requested
    if(p_threadID)
    {
      *p_threadID = processInfo.dwThreadId;
    }
    ::CloseHandle(processInfo.hProcess);
    ::CloseHandle(processInfo.hThread);
  }
  else
  {
    XString error = GetLastErrorAsString();
    p_errormessage.AppendFormat(_T("\nError while starting: %s"),p_program.GetString());
    p_errormessage.AppendFormat(_T("\nOS Error: %s"),error.GetString());
  }
  return exitCode;
}
