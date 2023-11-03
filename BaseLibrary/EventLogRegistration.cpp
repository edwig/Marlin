/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: EventlogRegistration.h
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
#include "EventLogRegistration.h"
#include "GetLastErrorAsString.h"
#include "ServiceReporting.h"
#include "GetExePath.h"
#include <Shlwapi.h>
#include <strsafe.h>
#include <io.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// Name of the event log category in the WMI
static const  PTCHAR eventLogCategory = _T("Application");

// Register our DLL. Return '1' if successful, otherwise '0'
int
RegisterMessagesDllForService(XString p_serviceName,XString p_messageDLL,XString& p_error)
{
  p_error.Empty();

  // Record our service name for service reporting purposes, if not already set
  if(!g_svcname[0])
  {
    if(StringCchCopy(g_svcname,SERVICE_NAME_LENGTH,p_serviceName) != S_OK)
    {
      p_error = _T("The service name cannot be registered: ") + p_serviceName;
      return 0;
    }
  }

  // Construct absolute filename of the DLL
  XString pathname = GetExePath();
  if(pathname.IsEmpty())
  {
    p_error.Format(_T("No working directory found. Cannot install service: [%s] Error: %s\n")
                   ,p_serviceName.GetString()
                   ,GetLastErrorAsString().GetString());
    return 0;
  }
  // Add the message DLL filename
  pathname += p_messageDLL;

  // Diagnose the file for reading rights
  if(_taccess(pathname,4) != 0)
  {
    p_error.Format(_T("Cannot find the resource DLL [%s] or no reading rights on the filename\n"),pathname.GetString());
    p_error.AppendFormat(_T("Possible cause: %s\n"),GetLastErrorAsString().GetString()); 
  }

  HKEY   hk(nullptr);
  DWORD  dwDisp = 0;
  TCHAR  szBuf[MAX_PATH + 1] = _T("");
  size_t cchSize = MAX_PATH;

  // Create the event source as a subkey of the log. 
  StringCchPrintf(szBuf,cchSize,_T("SYSTEM\\CurrentControlSet\\Services\\EventLog\\%s\\%s"),eventLogCategory,p_serviceName.GetString()); 

  if(RegCreateKeyEx(HKEY_LOCAL_MACHINE
                   ,szBuf
                   ,0
                   ,NULL
                   ,REG_OPTION_NON_VOLATILE
                   ,KEY_WRITE
                   ,NULL
                   ,&hk
                   ,&dwDisp)) 
  {
    p_error.Format(_T("Could not create the registry key for the %s.\n"),p_messageDLL.GetString()); 
    return 0;
  }

  // Set the name of the message file. 
  if(RegSetValueEx(hk,                             // subkey handle 
                  _T("EventMessageFile"),          // value name 
                   0,                              // must be zero 
                   REG_EXPAND_SZ,                  // value type 
                   (LPBYTE) pathname.GetString(),  // pointer to value data 
                   (DWORD)  pathname.GetLength())) // data size
  {
    p_error = _T("Could not set the logging \"EventMessageFile\" value.\n"); 
    RegCloseKey(hk); 
    return 0;
  }

  // Set the supported event types (straight from winnt.h)
  DWORD dwData = EVENTLOG_SUCCESS          | EVENTLOG_ERROR_TYPE    | EVENTLOG_WARNING_TYPE | 
                 EVENTLOG_INFORMATION_TYPE | EVENTLOG_AUDIT_SUCCESS | EVENTLOG_AUDIT_FAILURE;

  if(RegSetValueEx(hk,                    // subkey handle 
                  _T("TypesSupported"),   // value name 
                   0,                     // must be zero 
                   REG_DWORD,             // value type 
                   (LPBYTE)&dwData,       // pointer to value data 
                   sizeof(DWORD)))        // length of value data 
  {
    p_error.Format(_T("Could not set the supported types for the %s.\n"),p_messageDLL.GetString()); 
    RegCloseKey(hk); 
    return 0;
  }
  RegCloseKey(hk); 

  // Success = 1!
  return 1;
}

bool 
UnRegisterMessagesDllForService(XString p_serviceName,XString& p_error)
{
  TCHAR szBuf[MAX_PATH];
  size_t cchSize = MAX_PATH;
  p_error.Empty();

  // Create the event source as a subkey of the log. 
  StringCchPrintf(szBuf,cchSize,_T("SYSTEM\\CurrentControlSet\\Services\\EventLog\\%s\\%s"),eventLogCategory,p_serviceName.GetString());

  // Windows Vista and higher: RegDeleteTree
  if(SHDeleteKey(HKEY_LOCAL_MACHINE,szBuf))
  {
    p_error.Format(_T("Could not delete the registry keys for the %s.\n"),p_serviceName.GetString());
    return false;
  }
  return true;
}
