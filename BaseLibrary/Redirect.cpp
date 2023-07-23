/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: Redirect.cpp
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
#include "Redirect.h"
#include "AutoCritical.h"
#include <process.h>
#include <assert.h>
#include <time.h>
#include <corecrt_io.h>
#include <fcntl.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#ifdef _DEBUG
#define verify(x)  assert(x)
#else
#define verify(x)  (x)
#endif

// This class is build from the following MSDN article.
// Q190351 HOWTO: Spawn Console Processes with Redirected Standard Handles.

/////////////////////////////////////////////////////////////////////////////
// Redirect class

Redirect::Redirect()
{
  // Initialization.
  m_hStdIn          = NULL;
  m_hStdOut         = NULL;
  m_hStdErr         = NULL;
  m_hStdInWrite     = NULL;
  m_hStdOutRead     = NULL;
  m_hStdErrRead     = NULL;
  m_hChildProcess   = NULL;
  m_hStdOutThread   = NULL;
  m_hStdErrThread   = NULL;
  m_hProcessThread  = NULL;
  m_hExitEvent      = NULL;
  m_bRunThread      = FALSE;
  m_exitCode        = 0;
  m_eof_input       = 0;
  m_eof_error       = 0;
  m_timeoutChild    = INFINITE;
  m_timeoutIdle     = MAXWAIT_FOR_INPUT_IDLE;
  m_terminated      = false;

  InitializeCriticalSection((LPCRITICAL_SECTION)&m_critical);
}

Redirect::~Redirect()
{
  TerminateChildProcess();
  DeleteCriticalSection(&m_critical);
}

// Max waiting time for input-idle status of the child process
void 
Redirect::SetTimeoutIdle(ULONG p_timeout)
{
  m_timeoutIdle = p_timeout;
}

// Create standard handles, try to start child from command line.
BOOL 
Redirect::StartChildProcess(LPCSTR lpszCmdLine,UINT uShowChildWindow /*=SW_HIDE*/,BOOL bWaitForInputIdle /*=FALSE*/)
{
  HANDLE hProcess = ::GetCurrentProcess();

  m_eof_input = 0;
  m_eof_error = 0;
  m_exitCode  = 0;

  // Set up the security attributes struct.
  SECURITY_ATTRIBUTES sa;
  ::ZeroMemory(&sa, sizeof(SECURITY_ATTRIBUTES));
  sa.nLength= sizeof(SECURITY_ATTRIBUTES);
  sa.lpSecurityDescriptor = NULL;
  sa.bInheritHandle = TRUE;

  HANDLE hStdInWriteTmp = NULL;
  HANDLE hStdOutReadTmp = NULL;
  HANDLE hStdErrReadTmp = NULL;

  // Create the child stdin pipe.
  verify(::CreatePipe(&m_hStdIn, &hStdInWriteTmp, &sa, 0));

  // Create the child stdout pipe.
  verify(::CreatePipe(&hStdOutReadTmp, &m_hStdOut, &sa, 0));

  // Create the child stderr pipe.
  verify(::CreatePipe(&hStdErrReadTmp, &m_hStdErr, &sa, 0));

  // Create new stdin write, stdout and stderr read handles.
  // Set the properties to FALSE. Otherwise, the child inherits the
  // properties and, as a result, non-closeable handles to the pipes
  // are created.
  verify(::DuplicateHandle(hProcess,hStdInWriteTmp,hProcess,&m_hStdInWrite,0,FALSE,DUPLICATE_SAME_ACCESS));
  verify(::DuplicateHandle(hProcess,hStdOutReadTmp,hProcess,&m_hStdOutRead,0,FALSE,DUPLICATE_SAME_ACCESS));
  verify(::DuplicateHandle(hProcess,hStdErrReadTmp,hProcess,&m_hStdErrRead,0,FALSE,DUPLICATE_SAME_ACCESS));

  // Close inheritable copies of the handles you do not want to be
  // inherited.
  verify(::CloseHandle(hStdInWriteTmp));
  verify(::CloseHandle(hStdOutReadTmp));
  verify(::CloseHandle(hStdErrReadTmp));

  // Start child process with redirected stdout, stdin & stderr
  m_hChildProcess = PrepAndLaunchRedirectedChild(lpszCmdLine,
                                                 m_hStdOut, 
                                                 m_hStdIn, 
                                                 m_hStdErr, 
                                                 uShowChildWindow,
                                                 bWaitForInputIdle);

  if(m_hChildProcess == NULL)
  {
    // If we cannot start a child process: write to the standard error ourselves
    TCHAR lpszBuffer[BUFFER_SIZE];
    sprintf_s(lpszBuffer, BUFFER_SIZE, "Unable to start: %s\n", lpszCmdLine);
    OnChildStdErrWrite(lpszBuffer);

    // close all handles and return FALSE
    verify(::CloseHandle(m_hStdIn));
    m_hStdIn = NULL;
    verify(::CloseHandle(m_hStdOut));
    m_hStdOut = NULL;
    verify(::CloseHandle(m_hStdErr));
    m_hStdErr = NULL;

    m_terminated = 1;
    m_eof_input  = 1;
    m_eof_error  = 1;

    return FALSE;
  }

  unsigned int dwThreadID;
  m_bRunThread = TRUE;

  // Create Exit event
  m_hExitEvent = ::CreateEvent(NULL, TRUE, FALSE, NULL);
  verify(m_hExitEvent != NULL);

  // Launch the thread that read the child stdout.
  m_hStdOutThread = (HANDLE)_beginthreadex(NULL, 0, staticStdOutThread,(LPVOID)this, 0, &dwThreadID);
  verify(m_hStdOutThread != NULL);

  // Launch the thread that read the child stderr.
  m_hStdErrThread = (HANDLE)_beginthreadex(NULL, 0, staticStdErrThread,(LPVOID)this, 0, &dwThreadID);
  verify(m_hStdErrThread != NULL);

  // Launch the thread that monitoring the child process.
  m_hProcessThread = (HANDLE)_beginthreadex(NULL, 0, staticProcessThread,(LPVOID)this, 0, &dwThreadID);
  verify(m_hProcessThread != NULL);

  // Virtual function to notify derived class that the child is started.
  // Does things like flushing the standard input stream
  OnChildStarted(lpszCmdLine);

  return TRUE;
}

// Check if the child process is running. 
// On NT/2000 the handle must have PROCESS_QUERY_INFORMATION access.
BOOL 
Redirect::IsChildRunning() const
{
  AutoCritSec lock((LPCRITICAL_SECTION)&m_critical);

  DWORD dwExitCode;
  if(m_hChildProcess == NULL)
  {
    return FALSE;
  }
  ::GetExitCodeProcess(m_hChildProcess, &dwExitCode);
  m_exitCode = dwExitCode;
  return (dwExitCode == STILL_ACTIVE) ? TRUE: FALSE;
}

void 
Redirect::TerminateChildProcess()
{
  // We are now in Terminate function
  // never come here twice
  if(m_terminated)
  {
    return;
  }
  m_terminated = true;

  // Check the process thread.
  if(m_hProcessThread != NULL)
  {
    // Tell the threads to exit and wait for process thread to die.
    m_bRunThread = FALSE;
    ::SetEvent(m_hExitEvent);

    WaitForSingleObject(m_hProcessThread,1000);
    m_hProcessThread = NULL;
  }

  // Close all child handles first.
  {
    AutoCritSec lock((LPCRITICAL_SECTION)&m_critical);
    if(m_hStdIn != NULL)
    {
      CloseHandle(m_hStdIn);
      m_hStdIn = NULL;
    }
    if(m_hStdOut != NULL)
    {
      CloseHandle(m_hStdOut);
      m_hStdOut = NULL;
    }
    if(m_hStdErr != NULL)
    {
      CloseHandle(m_hStdErr);
      m_hStdErr = NULL;
    }

    // Close all parent handles.
    if(m_hStdInWrite != NULL)
    {
      CloseHandle(m_hStdInWrite);
      m_hStdInWrite = NULL;
    }
    if(m_hStdOutRead != NULL)
    {
      CloseHandle(m_hStdOutRead);
      m_hStdOutRead = NULL;
    }
    if(m_hStdErrRead != NULL)
    {
      CloseHandle(m_hStdErrRead);
      m_hStdErrRead = NULL;
    }
  }

  // Wait for the stdout to drain
  int maxWaittime = DRAIN_STDOUT_MAXWAIT;
  while(maxWaittime >= 0)
  {
    if(!m_hStdOutRead)
    {
      break;
    }
    Sleep(DRAIN_STDOUT_INTERVAL);
    maxWaittime -= DRAIN_STDOUT_INTERVAL;
  }

  // Wait for the stderr to drain
  maxWaittime = DRAIN_STDOUT_MAXWAIT;
  while(maxWaittime >= 0)
  {
    if(!m_hStdErrRead)
    {
      break;
    }
    Sleep(DRAIN_STDOUT_INTERVAL);
    maxWaittime -= DRAIN_STDOUT_INTERVAL;
  }

  // Stop the stdout read thread.
  if(m_hStdOutThread != NULL)
  {
    WaitForSingleObject(m_hStdOutThread, 1000);
    m_hStdOutThread = NULL;
  }

  // Stop the stderr read thread.
  if(m_hStdErrThread != NULL)
  {
    WaitForSingleObject(m_hStdErrThread, 1000);
    m_hStdErrThread = NULL;
  }
  // Stop the child process if not already stopped.
  // It's not the best solution, but it is a solution.
  // On Win98 it may crash the system if the child process is the COMMAND.COM.
  // The best way is to terminate the COMMAND.COM process with an "exit" command.

  if (IsChildRunning())
  {
    TerminateProcess   (m_hChildProcess, 1);
    WaitForSingleObject(m_hChildProcess, 1000);
  }
  m_hChildProcess = NULL;

  // cleanup the exit event
  {
    AutoCritSec lock((LPCRITICAL_SECTION)&m_critical);
    if(m_hExitEvent != NULL)
    {
      CloseHandle(m_hExitEvent);
      m_hExitEvent = NULL;
    }
  }
}

// Launch the process that you want to redirect.
HANDLE 
Redirect::PrepAndLaunchRedirectedChild(LPCSTR lpszCmdLine
                                      ,HANDLE hStdOut
                                      ,HANDLE hStdIn
                                      ,HANDLE hStdErr
                                      ,UINT   uShowChildWindow
                                      ,BOOL   bWaitForInputIdle)
{
  // Resulting process information
  PROCESS_INFORMATION pi;
  ::ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));

  // Set up the start up info structure
  STARTUPINFO si;
  ::ZeroMemory(&si, sizeof(STARTUPINFO));
  si.cb = sizeof(STARTUPINFO);
  si.dwFlags = STARTF_USESTDHANDLES|STARTF_USESHOWWINDOW;
  si.hStdOutput = hStdOut;
  si.hStdInput  = hStdIn;
  si.hStdError  = hStdErr;

  // Use this if you want to show the child.
  // But it cannot get any larger than SW_MAX
  // Note that dwFlags must include STARTF_USESHOWWINDOW if you want to
  // use the wShowWindow flags.
  if(uShowChildWindow > SW_MAX)
  {
    uShowChildWindow = SW_HIDE;
  }
  si.wShowWindow = (WORD)uShowChildWindow;
   
  // Create the NULL security token for the process
  // On NT/2000 the handle must have PROCESS_QUERY_INFORMATION access.
  // This is made using an empty security descriptor. It is not the same
  // as using a NULL pointer for the security attribute!

  PSECURITY_DESCRIPTOR lpSD = new SECURITY_DESCRIPTOR;
  verify(::InitializeSecurityDescriptor(lpSD, SECURITY_DESCRIPTOR_REVISION));
#pragma warning(disable: 6248) // Setting ZERO DACL.
  verify(::SetSecurityDescriptorDacl(lpSD, -1, 0, 0));

  LPSECURITY_ATTRIBUTES lpSA = new SECURITY_ATTRIBUTES;
  lpSA->nLength = sizeof(SECURITY_ATTRIBUTES);
  lpSA->lpSecurityDescriptor = lpSD;
  lpSA->bInheritHandle = TRUE;

  // Try to spawn the process.
  BOOL bResult = ::CreateProcess(NULL
                                ,(char*)lpszCmdLine
                                ,lpSA
                                ,NULL
                                ,TRUE
                                ,CREATE_NEW_CONSOLE
                                ,NULL
                                ,NULL
                                ,&si
                                ,&pi);

  // Cleanup memory allocation
  if(lpSA != NULL)
  {
    delete lpSA;
  }
  if(lpSD != NULL)
  {
    delete lpSD;
  }
  // Return if an error occurs.
  if(!bResult) 
  {
    return NULL;
  }

  // Close any unnecessary handles.
  verify(::CloseHandle(pi.hThread));

  // Wait for the process so that it can begin processing the standard input
  if(bWaitForInputIdle && pi.hProcess)
  {
    ::WaitForInputIdle(pi.hProcess,m_timeoutIdle);
  }

  // Save global child process handle to cause threads to exit.
  return pi.hProcess;
}

// Thread to read the child stdout.
int 
Redirect::StdOutThread(HANDLE hStdOutRead)
{
  DWORD nBytesRead;
  CHAR  lpszBuffer[10];
  CHAR  lineBuffer[BUFFER_SIZE + 10] = {0} ;
  char* linePointer = lineBuffer;
  //    FOR DEBUGGING: See below
  //    int   i = 0;

  while(true)
  {
    nBytesRead = 0;
    if(!::ReadFile(hStdOutRead, lpszBuffer, 1, &nBytesRead, NULL) || !nBytesRead || lpszBuffer[0] == EOT)
    {
      // pipe done - normal exit path.
      // Partial input line left hanging?
      if(linePointer != lineBuffer)
      {
        *linePointer = 0;
        OnChildStdOutWrite(lineBuffer);
      }
      m_eof_input = 1;
      break;
    }
    // Add to line
    *linePointer++ = lpszBuffer[0];

    // Add end-of-line or line overflow, write to listener
    if(lpszBuffer[0] == '\n' || ((linePointer - lineBuffer) > BUFFER_SIZE))
    {
      // USED FOR DEBUGGING PURPOSES ONLY!!
      // So we can detect the draining of the standard output from the process
//       if(i++ % 20 == 0)
//       {
//         Sleep(1);
//       }
      
      // Virtual function to notify derived class that
      // characters are written to stdout.
      *linePointer = 0;
      OnChildStdOutWrite(lineBuffer);
      linePointer = lineBuffer;
    }
  }
  m_hStdOutThread = NULL;
  return 0;
}

// Thread to read the child stderr.

int 
Redirect::StdErrThread(HANDLE hStdErrRead)
{
  DWORD nBytesRead;
  CHAR  lpszBuffer[10];
  CHAR  lineBuffer[BUFFER_SIZE + 10] = { 0 };
  char* linePointer = lineBuffer;
  //    FOR DEBUGGING: See below
  //    int   i = 0;

  while(m_bRunThread)
  {
    if(!::ReadFile(hStdErrRead,lpszBuffer,1,&nBytesRead,NULL) || !nBytesRead || lpszBuffer[0] == EOT)
    {
      // pipe done - normal exit path.
      // Partial input line left hanging?
      if(linePointer != lineBuffer)
      {
        *linePointer = 0;
        OnChildStdErrWrite(lineBuffer);
      }
      m_eof_error = 1;
      break;
    }
    // Add to line: caller sees stdout AND stderr alike
    *linePointer++ = lpszBuffer[0];

    // Add end-of-line or line overflow, write to listener
    if(lpszBuffer[0] == '\n' || ((linePointer - lineBuffer) > BUFFER_SIZE))
    {
      // USED FOR DEBUGGING PURPOSES ONLY!!
      // So we can detect the draining of the standard output from the process
//       if(i++ % 20 == 0)
//       {
//         Sleep(1);
//       }

      // Virtual function to notify derived class that
      // characters are written to stdout.
      *linePointer = 0;
      OnChildStdErrWrite(lineBuffer);
      linePointer = lineBuffer;
    }
  }
  m_hStdErrThread = NULL;
  return 0;
}

// Thread to monitoring the child process.

int 
Redirect::ProcessThread()
{
  HANDLE hWaitHandle[2];
  hWaitHandle[0] = m_hExitEvent;
  hWaitHandle[1] = m_hChildProcess;
  int returnValue = -1;

  // Starting time of our process thread
  clock_t start = clock();

  while(m_bRunThread)
  {
    switch(::WaitForMultipleObjects(2, hWaitHandle, FALSE, 50))
    {
      case WAIT_OBJECT_0 + 0:	// exit on event
                              if(m_bRunThread == FALSE)
                              {
                                returnValue = 0;
                              }
                              break;
      case WAIT_OBJECT_0 + 1:	// child process exit
                              if(m_bRunThread == TRUE)
                              {
                                returnValue = 0;
                              }
                              m_bRunThread = FALSE;
                              break;
      case WAIT_FAILED:       returnValue = GetLastError();
                              m_bRunThread = FALSE;
                              break;
      case WAIT_TIMEOUT:      if((ULONG)((clock() - start)) > m_timeoutChild)
                              {
                                m_bRunThread = FALSE;
                              }
                              break;
      default:                m_bRunThread = FALSE;
                              returnValue = -1;
                              break;
    }
  }
  // Virtual function to notify derived class that child process is terminated.
  // Application must call TerminateChildProcess() but not direcly from this thread!
  OnChildTerminate();
  
  // We are ready running
  m_bRunThread = NULL;
  return returnValue;
}

// Function that writes to the child stdin.

int
Redirect::WriteChildStdIn(LPCSTR lpszInput)
{
  DWORD nBytesWrote;
  DWORD Length = (DWORD) strlen(lpszInput);
  if (m_hStdInWrite != NULL && Length > 0)
  {
    if(!::WriteFile(m_hStdInWrite, lpszInput, Length, &nBytesWrote, NULL))
    {
      if(::GetLastError() == ERROR_NO_DATA)
      {
        // Pipe was closed (do nothing).
        return 0;
      }
      else
      {
        // Call GetLastError() to get the error
        return -1;
      }
    }
  }
  return 0;
}

void
Redirect::CloseChildStdIn()
{
  AutoCritSec lock((LPCRITICAL_SECTION)&m_critical);

  if(m_hStdInWrite != NULL)
  {
    CloseHandle(m_hStdInWrite);
    m_hStdInWrite = NULL;
  }
}
