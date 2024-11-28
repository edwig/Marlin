/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: HTTPManager.cpp
//
// Marlin Component: Internet server/client
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
#include "stdafx.h"
#include "HTTPManager.h"
#include "HTTPManagerDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// Does nothing for the HTTPManager application
void LoadConstants()
{
}

// HTTPManagerApp

BEGIN_MESSAGE_MAP(HTTPManagerApp, CWinApp)
	ON_COMMAND(ID_HELP, &CWinApp::OnHelp)
END_MESSAGE_MAP()


// HTTPManagerApp construction

HTTPManagerApp::HTTPManagerApp()
{
	// Place all significant initialization in InitInstance
}

HTTPManagerApp::~HTTPManagerApp()
{
  // Virtual destructor
}

// The one and only HTTPManagerApp object

HTTPManagerApp theApp;


// HTTPManagerApp initialization

BOOL HTTPManagerApp::InitInstance()
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

	AfxEnableControlContainer();

	// Create the shell manager, in case the dialog contains
	// any shell tree view or shell list view controls.
	CShellManager *pShellManager = new CShellManager;

	// Activate "Windows Native" visual manager for enabling themes in MFC controls
	CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerWindows));

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	// of your final executable, you should remove from the following
	// the specific initialization routines you do not need
	// Change the registry key under which our settings are stored
	// TODO: You should modify this string to be something appropriate
	// such as the name of your company or organization
	SetRegistryKey(_T("Marlin\\HTTPManager"));

  int mode = ParseCommandLine();
  if(mode == MODE_NONE)
  {
    if (::MessageBox(NULL,_T("Start the HTTPManager in the Microsoft-IIS mode?"),_T("Start mode"), MB_YESNO | MB_ICONINFORMATION) == IDYES)
    {
      mode = MODE_IIS;
    }
    else
    {
      mode = MODE_STANDALONE;
    }
  }
	HTTPManagerDlg dlg(mode == MODE_IIS);

	m_pMainWnd = &dlg;
	INT_PTR nResponse = dlg.DoModal();
	if (nResponse == IDOK || nResponse == IDCANCEL)
	{
		// Handled OK
	}
	else if (nResponse == -1)
	{
		TRACE(traceAppMsg, 0, _T("Warning: dialog creation failed, so application is terminating unexpectedly.\n"));
	}

	// Delete the shell manager created above.
	if (pShellManager != NULL)
	{
		delete pShellManager;
	}

	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	return FALSE;
}

// Simple way to get to the command line
// HTTPManager [/IIS] [/STANDALONE]
//
int
HTTPManagerApp::ParseCommandLine()
{
  int result = MODE_NONE;

  for (int i = 1; i < __argc; i++)
  {
    LPCTSTR lpszParam = __targv[i];

    if (lpszParam[0] == _T('-') || lpszParam[0] == '/')
    {
      if (_tcsncicmp(&lpszParam[1], _T("IIS"), 3) == 0)
      {
        result = MODE_IIS;
      }
      else if (_tcsncicmp(&lpszParam[1], _T("STANDALONE"), 10) == 0)
      {
        result = MODE_STANDALONE;
      }
    }
  }
  return result;
}
