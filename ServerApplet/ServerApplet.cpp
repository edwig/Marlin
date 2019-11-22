/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: ServerApplet.cpp
//
// Marlin Component: Internet server/client
// 
// Copyright (c) 2016 ir. W.E. Huisman
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
#include "ServerApplet.h"
#include "ServerAppletDlg.h"
#include "InstallDlg.h"
#include <GetLastErrorAsString.h>
#include <Version.h>
#include <WebConfig.h>
#include <ExecuteProcess.h>
#include <io.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

const char* PROGRAM_NAME = nullptr;  // The current program

// Default values
void LoadConstants()
{
  PROGRAM_NAME = "ServerApplet";     // The current program
  PRODUCT_NAME = "MarlinServer";     // Our server executable / DLL
  PRODUCT_SITE = "/MarlinTest/";     // Our default base-URL
}

// ServerAppletApp

BEGIN_MESSAGE_MAP(ServerAppletApp, CWinApp)
  ON_COMMAND(ID_HELP, &CWinApp::OnHelp)
END_MESSAGE_MAP()

// ServerAppletApp construction
ServerAppletApp::ServerAppletApp()
{
}

// The one and only ServerAppletApp object
ServerAppletApp theApp;

// ServerAppletApp initialization

BOOL ServerAppletApp::InitInstance()
{
  // InitCommonControlsEx() is required on Windows XP if an application
  // manifest specifies use of ComCtl32.dll version 6 or later to enable
  // visual styles.  Otherwise, any window creation will fail.
  INITCOMMONCONTROLSEX InitCtrls;
  InitCtrls.dwSize = sizeof(InitCtrls);
  // Set this to include all the common control classes you want to use
  // in your application.
  InitCtrls.dwICC = ICC_WIN95_CLASSES;
  InitCommonControlsEx(&InitCtrls);

  CWinApp::InitInstance();

  // Standard initialization
  // If you are not using these features and wish to reduce the size
  // of your final executable, you should remove from the following
  // the specific initialization routines you do not need
  // Change the registry key under which our settings are stored
  SetRegistryKey("Marlin\\ServerApplet");

  int res = ParseCommandLine();

  // Load names for Marlin
  LoadConstants();

  // Work to do for an installer program
  if(res > 0)
  {
    InstallDlg dlg(NULL,res);
    dlg.DoModal();
    PostQuitMessage(0);
    return TRUE;
  }

  // Default is to start the applet dialog
  ServerAppletDlg dlg;
  m_pMainWnd = &dlg;
  dlg.DoModal();
  // Since the dialog has been closed, return FALSE so that we exit the
  //  application, rather than start the application's message pump.
  return FALSE;
}

int
ServerAppletApp::StartProgram(CString& p_program
                             ,CString& p_arguments
                             ,bool     p_currentdir
                             ,CString& p_errormessage)
{
  CString path;
  if(p_currentdir)
  {
    path = WebConfig::GetExePath();
  }
  path += p_program;

  CString result;
  int exitCode = ExecuteProcess(p_program,p_arguments,p_currentdir,p_errormessage,SW_HIDE,true);
  if(exitCode < 0)
  {
    MessageBox(m_pMainWnd->GetSafeHwnd(),p_errormessage,"ERROR",MB_OK|MB_ICONERROR);
  }
  return exitCode;
}

// Simple way to get some arguments from the command line
// SERVERAPPLET /h /? /install /uninstall
//
BOOL 
ServerAppletApp::ParseCommandLine()
{
  int result = 0;

  for (int i=1; i < __argc; i++)
  {
    LPCSTR lpszParam = __argv[i];

    if (lpszParam[0] == '-' || lpszParam[0] == '/')
    {
      if(tolower(lpszParam[1]) =='h' || lpszParam[1] == '?')
      {
        MessageBox(NULL
                  ,"(De-)Installing can be done with the options /INSTALL of /UNINSTALL"
                  ,"ServerApplet"
                  ,MB_OK|MB_ICONINFORMATION);
        return 0;
      }
      if(_strnicmp(&lpszParam[1],"install",  7) == 0) result = 1;
      if(_strnicmp(&lpszParam[1],"uninstall",9) == 0) result = 2;
    }
  }
  return result;
}
