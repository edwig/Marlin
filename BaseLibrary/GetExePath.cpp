/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: GetExePath.cpp
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
#include "BaseLibrary.h"
#include "GetExePath.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static TCHAR g_staticAddress;

HMODULE GetModuleHandle()
{
  // Getting the module handle, if any
  // If it fails, the process names will be retrieved
  // Thus we get the *.DLL handle in IIS instead of a
  // %systemdrive\system32\inetsrv\w3wp.exe path
  HMODULE loadModule = NULL;
  GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                    GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT
                   ,static_cast<LPCTSTR>(&g_staticAddress)
                   ,&loadModule);
  return loadModule;
}

XString GetExeFile()
{
  TCHAR buffer[_MAX_PATH + 1];

  HMODULE loadModule = GetModuleHandle();

  // Retrieve the path
  GetModuleFileName(loadModule,buffer,_MAX_PATH);
  return XString(buffer);
}

XString GetExePath()
{
  // Retrieve the path
  XString applicatiePlusPad = GetExeFile();

  int slashPositie = applicatiePlusPad.ReverseFind('\\');
  if(slashPositie == 0)
  {
    return _T("");
  }
  return applicatiePlusPad.Left(slashPositie + 1);
}

void
TerminateWithoutCleanup(int p_exitcode)
{
  //  Tha.. tha... that's all folks
  HANDLE hProcess = OpenProcess(PROCESS_TERMINATE,FALSE,GetCurrentProcessId());
  if(!hProcess || !TerminateProcess(hProcess,p_exitcode))
  {
    if(hProcess) CloseHandle(hProcess);
    _exit(p_exitcode);
  }
  // NO rights. Just an abort
  abort();
}

// WARNING:
// CANNOT BE CALLED FROM AN INTERNET IIS APPLICATION
void
CheckExePath(XString p_runtimer,XString p_productName)
{
  TCHAR buffer   [_MAX_PATH];
  TCHAR drive    [_MAX_DRIVE];
  TCHAR directory[_MAX_DIR];
  TCHAR filename [_MAX_FNAME];
  TCHAR extension[_MAX_EXT];

  GetModuleFileName(GetModuleHandle(NULL),buffer,_MAX_PATH);
  _tsplitpath_s(buffer,drive,_MAX_DRIVE,directory,_MAX_DIR,filename,_MAX_FNAME,extension,_MAX_EXT);

  XString runtimer = XString(filename) + XString(extension);
  XString title = _T("Installation");
  if(runtimer.CompareNoCase(p_runtimer))
  {
    XString errortext;
    errortext.Format(_T("You have started the '%s' program, but in fact it really is: '%s'\n"
                        "Your product %s cannot function properly because of this rename action.\n"
                        "\n"
                        "Contact your friendly (system) administrator about this problem.\n"
                        "Repair the installation and then retry this action.")
                       ,runtimer.GetString(),p_runtimer.GetString(),p_productName.GetString());
    ::MessageBox(NULL,errortext,title,MB_OK | MB_ICONERROR);
    TerminateWithoutCleanup(-4);
  }
}
