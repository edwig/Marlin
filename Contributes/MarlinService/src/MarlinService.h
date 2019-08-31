#ifndef MARLINWEBSERVER_SERVICE_H
#define MARLINWEBSERVER_SERVICE_H
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

#pragma once
#include <HTTPServer.h>
//
// 
// Settings of the service
// 

// Internal name of the service
#define SERVICE_NAME             L"MarlinWebServer"

// Displayed name of the service
#define SERVICE_DISPLAY_NAME     L"Marlin based web server"
#define SERVICE_DESC             L"Marlin based web server"
// Service start options.
#define SERVICE_START_TYPE       SERVICE_AUTO_START

// List of service dependencies - "dep1\0dep2\0\0"
#define SERVICE_DEPENDENCIES     L"HTTP\0\0"

// The name of the account under which the service should run
#define SERVICE_ACCOUNT          NULL
#define SERVICE_ACCOUNT_T        WinLocalSystemSid
// The password to the service account name
#define SERVICE_PASSWORD         NULL

class MarlinService
{
public:
  MarlinService(PWSTR pszServiceName, 
    BOOL fCanStop = TRUE, 
    BOOL fCanShutdown = TRUE, 
    BOOL fCanPauseContinue = FALSE);

  ~MarlinService(void);
  
  BOOL Run();

private:
  // Start the service.
  void Start(DWORD dwArgc, PWSTR *pszArgv);
  // stop the service
  void Stop();

  // Set the service status and report the status to the SCM.
  void SetServiceStatus(DWORD dwCurrentState, 
    DWORD dwWin32ExitCode = NO_ERROR, 
    DWORD dwWaitHint = 0);

  // Entry point for the service. It registers the handler function for the 
  // service and starts the service.
  static void WINAPI ServiceMain(DWORD dwArgc, LPWSTR *lpszArgv);

  // The function is called by the SCM whenever a control code is sent to 
  // the service.
  static DWORD WINAPI ServiceCtrlHandler(DWORD dwCtrl, DWORD dwEventType, LPVOID lpEventData, LPVOID lpContext);

  void runtestset(HTTPServer* server);
private:
  // The name of the service
  PWSTR m_name;
  // The status of the service
  SERVICE_STATUS m_status;
  // The service status handle
  SERVICE_STATUS_HANDLE m_statusHandle;

  LogAnalysis m_logger;
  HTTPServer* m_marlinServer;
  ErrorReport m_errorReport;
};

#endif//MARLINWEBSERVER_SERVICE_H
