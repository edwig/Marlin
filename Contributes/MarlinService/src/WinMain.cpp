/*
MIT License

Copyright (c) 2019 Yuguo Zhang 

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
#include "pch.h"
#include "MarlinService.h"
#include "utils.h"

#ifdef _DEBUG
static HANDLE g_hlogfile = INVALID_HANDLE_VALUE;
static void setupDebug(PCSTR pszFileName);
static void flushDebug();

#define DEBUG_NEW new( _NORMAL_BLOCK, __FILE__, __LINE__)
#define new DEBUG_NEW
#endif

// run as windows service

int
runAsDaemon(LPTSTR /*lpCmdLine*/)
{
  MarlinService service(SERVICE_NAME);
  if (!service.Run()) {
    //utils::LogFmt(L"Service failed to run");
  }
  return 0;
}

// run as normal application

int
runAsApp(HINSTANCE /*hInstance*/, PSTR lpCmdLine)
{
  if ((strlen(lpCmdLine)>0) && ((lpCmdLine[0] == '-' || (lpCmdLine[0] == '/')))) {
    if (strnicmp("install", lpCmdLine+1, 7) == 0) {
      if (utils::InstallService(
        SERVICE_NAME,
        SERVICE_DISPLAY_NAME,
        SERVICE_DESC,
        SERVICE_START_TYPE,
        SERVICE_DEPENDENCIES,
        SERVICE_ACCOUNT,
        SERVICE_PASSWORD
        )) {
        utils::MsgBoxFmt(MB_OK | MB_ICONINFORMATION, L"Maintenance", L"service [%s] is installed.", SERVICE_DISPLAY_NAME);
      }
      return 0;
    } else if (strnicmp("remove", lpCmdLine+1, 6) == 0) {
      if (utils::UninstallService(SERVICE_NAME))
        utils::MsgBoxFmt(MB_OK | MB_ICONINFORMATION, L"Maintenance", L"service [%s] is removed.", SERVICE_DISPLAY_NAME);
      return 0;
    } else {
			//nothing to do
      return 1;
    }
  } else if (strlen(lpCmdLine) == 0) {
    utils::MsgBoxFmt(MB_OK | MB_ICONINFORMATION, L"Maintenance",
		        L"supported arguments:\n\n"
						L"-install\tinstall service\n"
						L"-remove\tuninstall service");
  }

  return 9;
}

//

extern "C" int WINAPI
WinMain(HINSTANCE hInstance,
          HINSTANCE,
          PSTR lpCmdLine,
          int)
{
  BOOL asservice = (utils::GetCurrentSessionId()==0) && utils::AmIWellKnown(SERVICE_ACCOUNT_T);

  int iRet = 0;
#ifdef _DEBUG
  CHAR szPath[MAX_PATH] = { 0 };
  ::GetModuleFileNameA(hInstance, szPath, MAX_PATH);
  CHAR* p = szPath + strlen(szPath);
  while (*p != '\\') p--;
  *p = '\0';
  strcat(szPath, asservice ? "\\MarlinService_dbg_diag_srv.log" : "\\MarlinService_dbg_diag_client.log");
  setupDebug(szPath);
  //atexit(flushDebug);
  char* leaker = new char[64];
  strcpy(leaker, "C++ leaker");
  leaker = (PCHAR)malloc(sizeof(CHAR) * 64);
  strcpy(leaker, "C leaker");
#endif
  if (asservice) {
    iRet = runAsDaemon(lpCmdLine);
  } else {
    iRet = runAsApp(hInstance, lpCmdLine);
  }

  return iRet;
}


#ifdef _DEBUG
//

static void
setupDebug(PCSTR pszFileName)
{
  g_hlogfile = CreateFileA(pszFileName, GENERIC_WRITE, FILE_SHARE_WRITE, 
    NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

  _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE );
  _CrtSetReportFile(_CRT_WARN, g_hlogfile);
  _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE );
  _CrtSetReportFile(_CRT_ERROR, g_hlogfile);
  _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE );
  _CrtSetReportFile(_CRT_ASSERT, g_hlogfile);

  _CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
}

//

static void
flushDebug()
{
  _CrtDumpMemoryLeaks();
  if (g_hlogfile && g_hlogfile != INVALID_HANDLE_VALUE) {
    ::CloseHandle(g_hlogfile);
  }
}

#endif
