/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: ExecuteShell.cpp
//
// BaseLibrary: Indispensable general objects and functions
// 
// Copyright (c) 2014-2024 ir. W.E. Huisman
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
#include "ExecuteShell.h"
#include <shellapi.h>

// General function to start a 'command' option in the MS-Windows shell
// Adds error handling stuff if needed.
// Normally used for actions like 'open' or 'print'
// If you need to spawn a new executable properly: have a look at "ExecuteProcess"
bool
ExecuteShell(XString  p_command
            ,XString  p_program
            ,XString  p_arguments
            ,HWND     p_parent
            ,int      p_show
            ,XString* p_error /*= nullptr*/)
{
  HINSTANCE exec = ::ShellExecute(p_parent,p_command,p_program,p_arguments,NULL,p_show);
  __int64 res = (__int64)(exec);
  if(res <= 32)
  {
    if(p_error)
    {
      CString reason;
      switch(res)
      {
        case ERROR_FILE_NOT_FOUND:     reason = _T("File not found");                   break;
        case ERROR_PATH_NOT_FOUND:     reason = _T("Directory path not found");         break;
        case ERROR_BAD_FORMAT:         reason = _T("File format is bad/corrupt");       break;
        case SE_ERR_ACCESSDENIED:      reason = _T("No rights to access program/file"); break;
        case SE_ERR_ASSOCINCOMPLETE:   reason = _T("Incomplete file association");      break;
        case SE_ERR_DLLNOTFOUND:       reason = _T("DLL file not found");               break;
        case SE_ERR_OOM:               reason = _T("Out of Memory");                    break;
        case SE_ERR_SHARE:             reason = _T("File sharing error");               break;
      }
      // Gathering the error
      *p_error  = _T("Error at Shell-Execute: ");
      *p_error += reason + _T("\n");
      // If amorphous command, add it
      if(p_command.GetLength())
      {
        *p_error += p_command + _T(": ");
      }
      // Add program and pathname
      *p_error +=  p_program + _T(" ") + p_arguments;
    }
    return false;
  }
  return true;
}
