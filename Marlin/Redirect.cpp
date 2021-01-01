/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: Redirect.cpp
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
#include "Redirect.h"
#include <process.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// This class is build from the following MSDN article.
// Q190351 HOWTO: Spawn Console Processes with Redirected Standard Handles.

/////////////////////////////////////////////////////////////////////////////
// CRedirect class

CRedirect::CRedirect()
{
  // Initialisation.
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
  m_terminated      = false;
}

CRedirect::~CRedirect()
{
  TerminateChildProcess();
}

// Create standard handles, try to start child from command line.

BOOL 
CRedirect::StartChildProcess(LPCSTR lpszCmdLine, BOOL bShowChildWindow)
{
  HANDLE hProcess = ::GetCurrentProcess();

  m_eof_input = 0;
  m_exitCode  = 0;
  // Set up the security attributes struct.
  SECURITY_ATTRIBUTES sa;
  ::ZeroMemory(&sa, sizeof(SECURITY_ATTRIBUTES));
  sa.nLength= sizeof(SECURITY_ATTRIBUTES);
  sa.lpSecurityDescriptor = NULL;
  sa.bInheritHandle = TRUE;

  HANDLE hStdInWriteTmp, hStdOutReadTmp, hStdErrReadTmp;

  // Create the child stdin pipe.
  VERIFY(::CreatePipe(&m_hStdIn, &hStdInWriteTmp, &sa, 0));

  // Create the child stdout pipe.
  VERIFY(::CreatePipe(&hStdOutReadTmp, &m_hStdOut, &sa, 0));

  // Create the child stderr pipe.
  VERIFY(::CreatePipe(&hStdErrReadTmp, &m_hStdErr, &sa, 0));

  // Create new stdin write, stdout and stderr read handles.
  // Set the properties to FALSE. Otherwise, the child inherits the
  // properties and, as a result, non-closeable handles to the pipes
  // are created.

  VERIFY(::DuplicateHandle(hProcess, hStdInWriteTmp,
    hProcess, &m_hStdInWrite, 0, FALSE, DUPLICATE_SAME_ACCESS));

  VERIFY(::DuplicateHandle(hProcess, hStdOutReadTmp,
    hProcess, &m_hStdOutRead, 0, FALSE, DUPLICATE_SAME_ACCESS));

  VERIFY(::DuplicateHandle(hProcess, hStdErrReadTmp,
    hProcess, &m_hStdErrRead, 0, FALSE, DUPLICATE_SAME_ACCESS));

  // Close inheritable copies of the handles you do not want to be
  // inherited.

  VERIFY(::CloseHandle(hStdInWriteTmp));
  VERIFY(::CloseHandle(hStdOutReadTmp));
  VERIFY(::CloseHandle(hStdErrReadTmp));

  // Start child process with redirected stdout, stdin & stderr
  m_hChildProcess = PrepAndLaunchRedirectedChild(lpszCmdLine,
    m_hStdOut, m_hStdIn, m_hStdErr, bShowChildWindow);

  if (m_hChildProcess == NULL)
  {
    TCHAR lpszBuffer[BUFFER_SIZE];
    sprintf_s(lpszBuffer, BUFFER_SIZE, "Unable to start %s\n", lpszCmdLine);
    OnChildStdOutWrite(lpszBuffer);

    // close all handles and return FALSE
    VERIFY(::CloseHandle(m_hStdIn));
    m_hStdIn = NULL;
    VERIFY(::CloseHandle(m_hStdOut));
    m_hStdOut = NULL;
    VERIFY(::CloseHandle(m_hStdErr));
    m_hStdErr = NULL;

    return FALSE;
  }

  unsigned int dwThreadID;
  m_bRunThread = TRUE;

  // Create Exit event
  m_hExitEvent = ::CreateEvent(NULL, TRUE, FALSE, NULL);
  VERIFY(m_hExitEvent != NULL);

  // Launch the thread that read the child stdout.
  m_hStdOutThread = (HANDLE)_beginthreadex(NULL, 0, staticStdOutThread,(LPVOID)this, 0, &dwThreadID);
  VERIFY(m_hStdOutThread != NULL);

  // Launch the thread that read the child stderr.
  m_hStdErrThread = (HANDLE)_beginthreadex(NULL, 0, staticStdErrThread,(LPVOID)this, 0, &dwThreadID);
  VERIFY(m_hStdErrThread != NULL);

  // Launch the thread that monitoring the child process.
  m_hProcessThread = (HANDLE)_beginthreadex(NULL, 0, staticProcessThread,(LPVOID)this, 0, &dwThreadID);
  VERIFY(m_hProcessThread != NULL);

  // Virtual function to notify derived class that the child is started.
  OnChildStarted(lpszCmdLine);

  return TRUE;
}

// Check if the child process is running. 
// On NT/2000 the handle must have PROCESS_QUERY_INFORMATION access.

BOOL CRedirect::IsChildRunning() const
{
  DWORD dwExitCode;
  if (m_hChildProcess == NULL) return FALSE;
  ::GetExitCodeProcess(m_hChildProcess, &dwExitCode);
  m_exitCode = dwExitCode;
  return (dwExitCode == STILL_ACTIVE) ? TRUE: FALSE;
}

void CRedirect::TerminateChildProcess()
{
  if(m_terminated)
  {
    return;
  }
  // We ar now in Terminate function
  // never come here twice
  m_terminated = true;

  // Tell the threads to exit and wait for process thread to die.
  m_bRunThread = FALSE;
  ::SetEvent(m_hExitEvent);

  // Check the process thread.
  if (m_hProcessThread != NULL)
  {
    ASSERT(::WaitForSingleObject(m_hProcessThread, 1000) != WAIT_TIMEOUT);
    m_hProcessThread = NULL;
  }

  // Close all child handles first.
  if (m_hStdIn != NULL)
  {
    VERIFY(::CloseHandle(m_hStdIn));
    m_hStdIn = NULL;
  }
  if (m_hStdOut != NULL)
  {
    VERIFY(::CloseHandle(m_hStdOut));
    m_hStdOut = NULL;
  }
  if (m_hStdErr != NULL)
  {
    VERIFY(::CloseHandle(m_hStdErr));
    m_hStdErr = NULL;
  }
  // Close all parent handles.
  if (m_hStdInWrite != NULL)
  {
    VERIFY(::CloseHandle(m_hStdInWrite));
    m_hStdInWrite = NULL;
  }
  if (m_hStdOutRead != NULL)
  {
    VERIFY(::CloseHandle(m_hStdOutRead));
    m_hStdOutRead = NULL;
  }
  if (m_hStdErrRead != NULL)
  {
    VERIFY(::CloseHandle(m_hStdErrRead));
    m_hStdErrRead = NULL;
  }
  // Stop the stdout read thread.
  if (m_hStdOutThread != NULL)
  {
    VERIFY(::WaitForSingleObject(m_hStdOutThread, 1000) != WAIT_TIMEOUT);
    m_hStdOutThread = NULL;
  }

  // Stop the stderr read thread.
  if (m_hStdErrThread != NULL)
  {
    VERIFY(::WaitForSingleObject(m_hStdErrThread, 1000) != WAIT_TIMEOUT);
    m_hStdErrThread = NULL;
  }
  // Stop the child process if not already stopped.
  // It's not the best solution, but it is a solution.
  // On Win98 it may crash the system if the child process is the COMMAND.COM.
  // The best way is to terminate the COMMAND.COM process with an "exit" command.

  if (IsChildRunning())
  {
    VERIFY(::TerminateProcess(m_hChildProcess, 1));
    VERIFY(::WaitForSingleObject(m_hChildProcess, 1000) != WAIT_TIMEOUT);
  }
  CloseHandle(m_hChildProcess);
  m_hChildProcess = NULL;

  // cleanup the exit event
  if (m_hExitEvent != NULL)
  {
    VERIFY(::CloseHandle(m_hExitEvent));
    m_hExitEvent = NULL;
  }
}

// Launch the process that you want to redirect.

HANDLE CRedirect::PrepAndLaunchRedirectedChild(LPCSTR lpszCmdLine
                                              ,HANDLE hStdOut
                                              ,HANDLE hStdIn
                                              ,HANDLE hStdErr
                                              ,BOOL   bShowChildWindow)
{
  //HANDLE hProcess = ::GetCurrentProcess();

  PROCESS_INFORMATION pi;

  // Set up the start up info struct.
  STARTUPINFO si;
  ::ZeroMemory(&si, sizeof(STARTUPINFO));
  si.cb = sizeof(STARTUPINFO);
  si.dwFlags = STARTF_USESTDHANDLES|STARTF_USESHOWWINDOW;
  si.hStdOutput = hStdOut;
  si.hStdInput  = hStdIn;
  si.hStdError  = hStdErr;

  // Use this if you want to show the child.
  si.wShowWindow = bShowChildWindow ? SW_SHOW: SW_HIDE;
  // Note that dwFlags must include STARTF_USESHOWWINDOW if you want to
  // use the wShowWindow flags.

  // Create the NULL security token for the process
  // On NT/2000 the handle must have PROCESS_QUERY_INFORMATION access.
  // This is made using an empty security descriptor. It is not the same
  // as using a NULL pointer for the security attribute!

  PSECURITY_DESCRIPTOR lpSD = new SECURITY_DESCRIPTOR;
  VERIFY(::InitializeSecurityDescriptor(lpSD, SECURITY_DESCRIPTOR_REVISION));
  VERIFY(::SetSecurityDescriptorDacl(lpSD, -1, 0, 0));

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
  if (lpSA != NULL)
  {
    delete lpSA;
  }
  if (lpSD != NULL)
  {
    delete lpSD;
  }
  // Return if an error occurs.
  if (!bResult) 
  {
    return FALSE;
  }
  // Close any unnecessary handles.
  VERIFY(::CloseHandle(pi.hThread));

  // Save global child process handle to cause threads to exit.
  return pi.hProcess;
}

BOOL CRedirect::m_bRunThread = TRUE;

// Thread to read the child stdout.

int 
CRedirect::StdOutThread(HANDLE hStdOutRead)
{
  DWORD nBytesRead;
  CHAR  lpszBuffer[10];
  CHAR  lineBuffer[BUFFER_SIZE+10];
  char* linePointer = lineBuffer;

  while(true)
  {
    nBytesRead = 0;
    if (!::ReadFile(hStdOutRead, lpszBuffer, 1, &nBytesRead, NULL) || !nBytesRead)
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
    if(lpszBuffer[0] == '\004')
    {
      // EOT encountered: End of transmission channel
      // from redirected child
      m_eof_input = 1;
      break;
    }
    *linePointer++ = lpszBuffer[0];
    if(lpszBuffer[0] == '\n' || ((linePointer - lineBuffer) > BUFFER_SIZE))
    {
      // Virtual function to notify derived class that
      // characters are writted to stdout.
      *linePointer = 0;
      OnChildStdOutWrite(lineBuffer);
      linePointer = lineBuffer;
    }
  }
  return 0;
}

// Thread to read the child stderr.

int 
CRedirect::StdErrThread(HANDLE hStdErrRead)
{
  DWORD nBytesRead;
  CHAR  lpszBuffer[10];
  CHAR  lineBuffer[BUFFER_SIZE+1];
  char* linePointer = lineBuffer;

  while (m_bRunThread)
  {
    if (!::ReadFile(hStdErrRead, lpszBuffer, 1,	&nBytesRead, NULL) || !nBytesRead)
    {
      // pipe done - normal exit path.
      // Partial input line left hanging?
      if(linePointer != lineBuffer)
      {
        *linePointer = 0;
        OnChildStdErrWrite(lineBuffer);
      }
      break;			
    }
    if(lpszBuffer[0] == '\004')
    {
      // EOT encountered: End of transmission channel
      // from redirected child
      break;
    }
    *linePointer++ = lpszBuffer[0];
    if(lpszBuffer[0] == '\n' || ((linePointer - lineBuffer) > BUFFER_SIZE))
    {
      // Virtual function to notify derived class that
      // characters are writted to stdout.
      *linePointer = 0;
      OnChildStdErrWrite(lpszBuffer);
      linePointer = lineBuffer;
    }
  }
  return 0;
}

// Thread to monitoring the child process.

int 
CRedirect::ProcessThread()
{
  HANDLE hWaitHandle[2];
  hWaitHandle[0] = m_hExitEvent;
  hWaitHandle[1] = m_hChildProcess;
  int returnValue = -1;

  while (m_bRunThread)
  {
    switch (::WaitForMultipleObjects(2, hWaitHandle, FALSE, 50))
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
      case WAIT_TIMEOUT:      break;
      default:                m_bRunThread = FALSE;
                              returnValue = -1;
                              break;
    }
  }
  // Virtual function to notify derived class that child process is terminated.
  // Application must call TerminateChildProcess() but not direcly from this thread!
  OnChildTerminate();

  // Close the stdout stream, so stdout thread can finish
  if (m_hStdOut != NULL)
  {
    VERIFY(::CloseHandle(m_hStdOut));
    m_hStdOut = NULL;
  }
  if(returnValue == -1)
  {
    TerminateChildProcess();
  }
  return returnValue;
}

// Function that write to the child stdin.

int
CRedirect::WriteChildStdIn(LPCSTR lpszInput)
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
CRedirect::CloseChildStdIn()
{
  if(m_hStdInWrite != NULL)
  {
    VERIFY(::CloseHandle(m_hStdInWrite));
    m_hStdInWrite = NULL;
  }
}
