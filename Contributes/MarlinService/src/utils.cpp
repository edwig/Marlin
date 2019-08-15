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
#include <shellapi.h>
#include <HTTPClient.h>
#include "utils.h"

#ifdef _DEBUG
#define DEBUG_NEW new( _NORMAL_BLOCK, __FILE__, __LINE__)
#define new DEBUG_NEW
#endif


//

static BOOL
GetTokenInfo(HANDLE hToken, TOKEN_USER** ppTokenUser) 
{
  DWORD error, tokenInfoLen;
  if (GetTokenInformation(hToken, TokenUser, NULL, 0, &tokenInfoLen) || (error = GetLastError()) != ERROR_INSUFFICIENT_BUFFER) {
    return FALSE;
  }

  VOID *tokenInfoPtr;
  if ((tokenInfoPtr = HeapAlloc(GetProcessHeap(), 0, tokenInfoLen)) == NULL) {
    return FALSE;
  }

  if (!GetTokenInformation(hToken, TokenUser, tokenInfoPtr, tokenInfoLen, &tokenInfoLen)) {
    HeapFree(GetProcessHeap(), 0, (LPVOID)tokenInfoPtr);
    return FALSE;
  }
  *ppTokenUser = static_cast<TOKEN_USER*>(tokenInfoPtr);
  return TRUE;
}

// check if current process is running under specified account

BOOL
utils::AmIWellKnown(WELL_KNOWN_SID_TYPE sidType)
{
  BOOL bYes = FALSE;
  HANDLE hProcess = ::GetCurrentProcess();
  if (hProcess) {
    HANDLE hToken = NULL;
    if (OpenProcessToken(hProcess, TOKEN_QUERY, &hToken)) {
      TOKEN_USER *pTokenUser = 0;
      if (GetTokenInfo(hToken, &pTokenUser)) {
        DWORD dwSize = SECURITY_MAX_SID_SIZE;
        BYTE sid[SECURITY_MAX_SID_SIZE] = {0};
        if (CreateWellKnownSid(sidType, NULL, sid, &dwSize)) {
          bYes = EqualSid(pTokenUser->User.Sid, sid);
        }
        HeapFree(GetProcessHeap(), 0, (LPVOID)pTokenUser);
      }
      CloseHandle(hToken);
    }
    CloseHandle(hProcess);
  }
  return bYes;
}


//

DWORD
utils::GetCurrentSessionId()
{
  DWORD dwSessionId = 0;
  if (ProcessIdToSessionId(GetCurrentProcessId(), &dwSessionId)) {
    return dwSessionId;
  }
  return 0xFFFFFFFF;
}

//

BOOL
utils::InstallService(PWSTR pszServiceName,
               PWSTR pszDisplayName,
               PWSTR pszDescription,
               DWORD dwStartType,
               PWSTR pszDependencies,
               PWSTR pszAccount,
               PWSTR pszPassword)
{
  wchar_t szPath[MAX_PATH];
  SC_HANDLE schSCManager = NULL;
  SC_HANDLE schService = NULL;
  BOOL succeeded = FALSE;
  if (GetModuleFileNameW(NULL, szPath, ARRAYSIZE(szPath)) == 0) {
    utils::MsgBoxFmt(MB_OK|MB_ICONERROR, L"Error", L"GetModuleFileName failed w/err 0x%08lx\n", GetLastError());
    goto Cleanup;
  }

  // Open the local default service control manager database
  schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT | SC_MANAGER_CREATE_SERVICE);
  if (schSCManager == NULL) {
    utils::MsgBoxFmt(MB_OK|MB_ICONERROR, L"Error", L"OpenSCManager failed w/err 0x%08lx\n", GetLastError());
    goto Cleanup;
  }

  // Install the service into SCM by calling CreateService
  schService = CreateServiceW(
    schSCManager,                   // SCManager database
    pszServiceName,                 // Name of service
    pszDisplayName,                 // Name to display
    SERVICE_QUERY_STATUS,           // Desired access
    SERVICE_WIN32_OWN_PROCESS,      // Service type
    dwStartType,                    // Service start type
    SERVICE_ERROR_NORMAL,           // Error control type
    szPath,                         // Service's binary
    NULL,                           // No load ordering group
    NULL,                           // No tag identifier
    pszDependencies,                // Dependencies
    pszAccount,                     // Service running account
    pszPassword                     // Password of the account
    );
  if (schService == NULL) {
    utils::MsgBoxFmt(MB_OK|MB_ICONERROR, L"Error", L"CreateService failed w/err 0x%08lx\n", GetLastError());
    goto Cleanup;
  } else {
    succeeded = TRUE;
    //service created, reopen to change config
    CloseServiceHandle(schService);
    schService = OpenServiceW(schSCManager, pszServiceName, SERVICE_CHANGE_CONFIG|READ_CONTROL|WRITE_DAC);
    if (schService != NULL) {
      SERVICE_DESCRIPTIONW sd;
      sd.lpDescription = pszDescription;
      if ( !ChangeServiceConfig2W(schService, SERVICE_CONFIG_DESCRIPTION, &sd) ) {
        utils::MsgBoxFmt(MB_OK|MB_ICONERROR, L"Error", L"ChangeServiceConfig2 failed w/err 0x%08lx\n", GetLastError());
      }
    } else {
      utils::MsgBoxFmt(MB_OK|MB_ICONERROR, L"Error", L"OpenService failed w/err 0x%08lx\n", GetLastError());
    }
  }
  
Cleanup:
  if (schService) {
    CloseServiceHandle(schService);
    schService = NULL;
  }
  // Centralized cleanup for all allocated resources.
  if (schSCManager) {
    CloseServiceHandle(schSCManager);
    schSCManager = NULL;
  }
  return succeeded;
}


//
//

BOOL
utils::UninstallService(PWSTR pszServiceName)
{
  BOOL flag = FALSE;
  SC_HANDLE schSCManager = NULL;
  SC_HANDLE schService = NULL;
  SERVICE_STATUS ssSvcStatus = {};
  // Open the local default service control manager database
  schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
  if (schSCManager == NULL) {
    utils::MsgBoxFmt(MB_OK|MB_ICONERROR, L"Error", L"OpenSCManager failed w/err 0x%08lx\n", GetLastError());
    goto Cleanup;
  }

  // Open the service with delete, stop, and query status permissions
  schService = OpenServiceW(schSCManager, pszServiceName, SERVICE_STOP | SERVICE_QUERY_STATUS | DELETE);
  if (schService == NULL) {
    utils::MsgBoxFmt(MB_OK|MB_ICONERROR, L"Error", L"OpenService failed w/err 0x%08lx\n", GetLastError());
    goto Cleanup;
  }

  // Try to stop the service
  if (ControlService(schService, SERVICE_CONTROL_STOP, &ssSvcStatus)) {
    //wprintf(L"Stopping %s.", pszServiceName);
    Sleep(1000);
    while (QueryServiceStatus(schService, &ssSvcStatus)) {
      if (ssSvcStatus.dwCurrentState == SERVICE_STOP_PENDING) {
        //wprintf(L".");
        Sleep(500);
      }
      else break;
    }

    if (ssSvcStatus.dwCurrentState == SERVICE_STOPPED) {
      //MsgBoxFmt(MB_OK|MB_ICONERROR, L"Error", L"\n%s is stopped.\n", pszServiceName);
    } else {
      utils::MsgBoxFmt(MB_OK|MB_ICONERROR, L"Error", L"\n%s failed to stop.\n", pszServiceName);
    }
  }

  // Now remove the service by calling DeleteService.
  if (!DeleteService(schService)) {
    utils::MsgBoxFmt(MB_OK|MB_ICONERROR, L"Error", L"DeleteService failed w/err 0x%08lx\n", GetLastError());
    goto Cleanup;
  } else {
    flag = TRUE;
  }

Cleanup:
  // Centralized cleanup for all allocated resources.
  if (schSCManager) {
    CloseServiceHandle(schSCManager);
    schSCManager = NULL;
  }
  if (schService) {
    CloseServiceHandle(schService);
    schService = NULL;
  }
  return flag;
}

//

INT
utils::MsgBoxFmt(UINT uiStyle, LPCWSTR pwszTitle, WCHAR* pwszFmt, ...)
{
  va_list marker;
  WCHAR wszBuffer[1024];
  HWND hwndFocus = GetFocus();

  va_start(marker, pwszFmt);
  _vsnwprintf(wszBuffer, 1024, pwszFmt, marker);
  va_end(marker);

  wszBuffer[1024-1] = L'\0';

  INT ret = MessageBoxW(NULL, wszBuffer, pwszTitle, uiStyle);
  SetFocus(hwndFocus);
  return ret;
}

//

INT
utils::MsgBoxFmt(UINT uiStyle, const char *pszTitle, char *pszFmt, ...)
{
  va_list	marker;
  char		szBuffer[1024];
  HWND hwndFocus = GetFocus();

  va_start(marker, pszFmt);
  _vsnprintf(szBuffer, 1024, pszFmt, marker);
  va_end(marker);

  szBuffer[1024-1] = '\0';

  INT ret = MessageBoxA(NULL, szBuffer, pszTitle, uiStyle);
  SetFocus(hwndFocus);
  return ret;
}


//

void
utils::LogFmt(WCHAR* pwszFmt, ...)
{
  va_list marker;
  WCHAR wszBuffer[4096];

  va_start(marker, pwszFmt);
  _vsnwprintf(wszBuffer, 4096, pwszFmt, marker);
  va_end(marker);

  wszBuffer[4096-1] = L'\0';

  OutputDebugStringW(wszBuffer);
}

//

void
utils::LogFmt(char *pszFmt, ...)
{
  va_list	marker;
  char		szBuffer[4096];

  va_start(marker, pszFmt);
  _vsnprintf(szBuffer, 4096, pszFmt, marker);
  va_end(marker);

  szBuffer[4096-1] = '\0';

  OutputDebugStringA(szBuffer);
}

