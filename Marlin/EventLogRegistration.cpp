/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: EventlogRegistration.h
//
// Marlin Server: Internet server/client
// 
// Copyright (c) 2015-2018 ir. W.E. Huisman
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
#include "EventLogRegistration.h"
#include "GetLastErrorAsString.h"
#include "ServiceReporting.h"
#include "WebConfig.h"
#include <strsafe.h>
#include <io.h>

// Name of the event log category in the WMI
static const char* eventLogCategory = "Application";

int
RegisterMessagesDllForService(CString p_serviceName,CString p_messageDLL,CString& p_error)
{
  p_error.Empty();

  // Record our service name for service reporting purposes
  StringCchCopy(g_svcname,SERVICE_NAME_LENGTH,p_serviceName);

  // Construct absolute filename of the DLL
  CString pathname = WebConfig::GetExePath();
  if(pathname.IsEmpty())
  {
    p_error.Format("No working directory found. Cannot install service: [%s] Error: %s\n"
                   ,p_serviceName.GetString()
                   ,GetLastErrorAsString().GetString());
    return 3;
  }
  // Add the message DLL filename
  pathname += p_messageDLL;

  // Diagnose the file for reading rights
  if(_access(pathname,4) != 0)
  {
    p_error.Format("Cannot find the resource DLL [%s] or no reading rights on the filename\n",pathname.GetString());
    p_error.AppendFormat("Possible cause: %s\n",GetLastErrorAsString().GetString()); 
  }

  HKEY hk; 
  DWORD dwData, dwDisp; 
  TCHAR szBuf[MAX_PATH]; 
  size_t cchSize = MAX_PATH;

  // Create the event source as a subkey of the log. 
  StringCchPrintf(szBuf,cchSize,"SYSTEM\\CurrentControlSet\\Services\\EventLog\\%s\\%s",eventLogCategory,g_svcname); 

  if (RegCreateKeyEx(HKEY_LOCAL_MACHINE
                    ,szBuf
                    ,0
                    ,NULL
                    ,REG_OPTION_NON_VOLATILE
                    ,KEY_WRITE
                    ,NULL
                    ,&hk
                    ,&dwDisp)) 
  {
    p_error.Format("Could not create the registry key for the %s.\n",p_messageDLL.GetString()); 
    return 0;
  }

  // Set the name of the message file. 
  if (RegSetValueEx(hk,                             // subkey handle 
                   "EventMessageFile",              // value name 
                    0,                              // must be zero 
                    REG_EXPAND_SZ,                  // value type 
                    (LPBYTE) pathname.GetString(),  // pointer to value data 
                    (DWORD)  pathname.GetLength())) // data size
  {
    p_error = "Could not set the logging \"EventMessageFile\" value.\n"; 
    RegCloseKey(hk); 
    return 0;
  }

  // Set the supported event types. 
  dwData = EVENTLOG_ERROR_TYPE | EVENTLOG_WARNING_TYPE | EVENTLOG_INFORMATION_TYPE; 

  if (RegSetValueEx(hk,      // subkey handle 
                   "TypesSupported",  // value name 
                    0,                 // must be zero 
                    REG_DWORD,         // value type 
                    (LPBYTE) &dwData,  // pointer to value data 
                    sizeof(DWORD)))    // length of value data 
  {
    p_error.Format("Could not set the supported types for the %s.\n",p_messageDLL.GetString()); 
    RegCloseKey(hk); 
    return 0;
  }
  RegCloseKey(hk); 

  // Success = 1!
  return 1;
}

bool 
UnRegisterMessagesDllForService(CString p_serviceName,CString& p_error)
{
  TCHAR szBuf[MAX_PATH];
  size_t cchSize = MAX_PATH;
  p_error.Empty();

  // Create the event source as a subkey of the log. 
  StringCchPrintf(szBuf,cchSize,"SYSTEM\\CurrentControlSet\\Services\\EventLog\\%s\\%s",eventLogCategory,g_svcname);

  // Windows Vista and higher: RegDeleteTree
  if(SHDeleteKey(HKEY_LOCAL_MACHINE,szBuf))
  {
    p_error.Format("Could not delete the registry keys for the %s.\n",p_serviceName.GetString());
    return false;
  }
  return true;
}
