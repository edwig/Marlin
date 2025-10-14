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

//////////////////////////////////////////////////////////////////////////

// The following function frees memory allocated in the BuildRestrictedSD() function
VOID FreeRestrictedSD(PVOID ptr)
{
  if(ptr)
  {
    HeapFree(GetProcessHeap(),0,ptr);
  }
}

PVOID BuildRestrictedSD(PSECURITY_DESCRIPTOR pSD)
{
  DWORD  dwAclLength;
  PSID   pAuthenticatedUsersSID = NULL;
  PACL   pDACL = NULL;
  BOOL   bResult = FALSE;
  SID_IDENTIFIER_AUTHORITY siaNT = SECURITY_NT_AUTHORITY;

  __try
  {

    // initialize the security descriptor
    if(!InitializeSecurityDescriptor(pSD,SECURITY_DESCRIPTOR_REVISION))
    {
      ATLTRACE("InitializeSecurityDescriptor() failed with error %d\n",GetLastError());
      __leave;
    }

    // obtain a sid for the Authenticated Users Group
    if(!AllocateAndInitializeSid(&siaNT,1,
                                 SECURITY_AUTHENTICATED_USER_RID,0,0,0,0,0,0,0,
                                 &pAuthenticatedUsersSID))
    {
      ATLTRACE("AllocateAndInitializeSid() failed with error %d\n",GetLastError());
      __leave;
    }

    // NOTE:
    // 
    // The Authenticated Users group includes all user accounts that
    // have been successfully authenticated by the system. If access
    // must be restricted to a specific user or group other than 
    // Authenticated Users, the SID can be constructed using the
    // LookupAccountSid() API based on a user or group name.

    // calculate the DACL length
    // add space for Authenticated Users group ACE
    dwAclLength = sizeof(ACL)
                  + sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD)
                  + GetLengthSid(pAuthenticatedUsersSID);

    // allocate memory for the DACL
    pDACL = (PACL)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,dwAclLength);
    if(!pDACL)
    {
      ATLTRACE("HeapAlloc() failed with error %d\n",GetLastError());
      __leave;
    }

    // initialize the DACL
    if(!InitializeAcl(pDACL,dwAclLength,ACL_REVISION))
    {
      ATLTRACE("InitializeAcl() failed with error %d\n",GetLastError());
      __leave;
    }

    // add the Authenticated Users group ACE to the DACL with
    // GENERIC_READ, GENERIC_WRITE, and GENERIC_EXECUTE access
    if(!AddAccessAllowedAce(pDACL
                           ,ACL_REVISION,
                            GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE,
                            pAuthenticatedUsersSID))
    {
      ATLTRACE("AddAccessAllowedAce() failed with error %d\n",GetLastError());
      __leave;
    }

    // set the DACL in the security descriptor
    if(!SetSecurityDescriptorDacl(pSD,TRUE,pDACL,FALSE))
    {
      ATLTRACE("SetSecurityDescriptorDacl() failed with error %d\n",GetLastError());
      __leave;
    }

    bResult = TRUE;
  }
  __finally
  {
    if(pAuthenticatedUsersSID)
    {
      FreeSid(pAuthenticatedUsersSID);
    }
  }

  if(bResult == FALSE)
  {
    if(pDACL)
    { 
      FreeRestrictedSD(pDACL);
    }
    pDACL = NULL;
  }
  return (PVOID)pDACL;
}

// Create a new thread that inherits the security context of the current process
// And use a non-memory leaking create function
//
HANDLE CreateFullThread(LPFN_THREADSTART p_startFunction
                       ,void*            p_context
                       ,unsigned*        p_threadID  /* = nullptr */
                       ,int              p_stacksize /* = 0       */
                       ,bool             p_inherit   /* = false   */
                       ,bool             p_suspended /* = false   */)
{
  HANDLE result = 0L;
  unsigned initflag = p_suspended ? CREATE_SUSPENDED : 0;

  // Create security descriptor
  // Thread should inherit the security rights of the current process
  SECURITY_DESCRIPTOR sd;
  SECURITY_ATTRIBUTES sa;
  LPVOID psd = BuildRestrictedSD(&sd);
  sa.nLength              = sizeof(sa);
  sa.lpSecurityDescriptor = &sd;
  sa.bInheritHandle       = p_inherit ? TRUE : FALSE;

  // Now create our thread
  unsigned ID = 0;
  result = reinterpret_cast<HANDLE>(_beginthreadex(psd ? &sa : nullptr  // Inherited rights, or default
                                                  ,p_stacksize          // Extra stacksize
                                                  ,p_startFunction      // Thread start
                                                  ,p_context            // Context
                                                  ,initflag             // Start directly or suspend
                                                  ,&ID));               // Created ID is returned here
  if((result != INVALID_HANDLE_VALUE) && p_threadID)
  {
    // propagate our new thread ID
    *p_threadID = ID;
  }
  // Security descriptor destruction
  FreeRestrictedSD(psd);

  return result;  
}
