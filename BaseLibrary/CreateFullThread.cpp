/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: ExecuteProcess.cpp
//
// BaseLibrary: Indispensable general objects and functions
// 
// Copyright (c) 2014-2025 ir. W.E. Huisman
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
#include "CreateFullThread.h"

// Set a name on your thread
// By executing a fake SEH Exception
//
const DWORD MS_VC_EXCEPTION = 0x406D1388;

#pragma pack(push,8)
typedef struct tagTHREADNAME_INFO
{
  DWORD  dwType;       // Must be 0x1000.
  LPCSTR szName;       // Pointer to name (in user addr space).
  DWORD  dwThreadID;   // Thread ID (MAXDWORD=caller thread).
  DWORD  dwFlags;      // Reserved for future use, must be zero.
}
THREADNAME_INFO;
#pragma pack(pop)

void SetThreadName(char* threadName,DWORD dwThreadID)
{
  THREADNAME_INFO info;
  info.dwType = 0x1000;
  info.szName = threadName;
  info.dwThreadID = dwThreadID;
  info.dwFlags = 0;

  __try
  {
    RaiseException(MS_VC_EXCEPTION,0,sizeof(info) / sizeof(ULONG_PTR),reinterpret_cast<ULONG_PTR*>(&info));
  }
  __except(EXCEPTION_EXECUTE_HANDLER)
  {
    // Nothing done here: just name changed
    memset(&info,0,sizeof(THREADNAME_INFO));
  }
}

// Create a new thread that inherits the security context of the current process
// And use a non-memory leaking create function
//
HANDLE CreateFullThread(LPFN_THREADSTART p_startFunction
                       ,void*            p_context
                       ,unsigned*        p_threadID  /* = nullptr */
                       ,int              p_stacksize /* = 0       */)
{
  HANDLE result = 0L;
  // Get the current thread's security descriptor
  HANDLE hCurrentProcess = GetCurrentProcess();
  DWORD  dwSize = 0;

  // First call to get the required buffer size
  GetKernelObjectSecurity(hCurrentProcess,DACL_SECURITY_INFORMATION | SACL_SECURITY_INFORMATION,nullptr,0,&dwSize);
  PSECURITY_DESCRIPTOR pSD = (PSECURITY_DESCRIPTOR)LocalAlloc(LPTR,dwSize);
  bool doSecurity = GetKernelObjectSecurity(hCurrentProcess,DACL_SECURITY_INFORMATION | SACL_SECURITY_INFORMATION,pSD,dwSize,&dwSize) != 0;

  // Set up SECURITY_ATTRIBUTES
  SECURITY_ATTRIBUTES sa;
  sa.nLength              = sizeof(SECURITY_ATTRIBUTES);
  sa.lpSecurityDescriptor = pSD;
  sa.bInheritHandle       = TRUE;

  // Now create our thread
  unsigned ID = 0;
  result = reinterpret_cast<HANDLE>(_beginthreadex(doSecurity ? &sa : nullptr,p_stacksize,p_startFunction,p_context,0,&ID));

  if((result != INVALID_HANDLE_VALUE) && p_threadID)
  {
    *p_threadID = ID;
  }
  LocalFree(pSD);

  return result;  
}
