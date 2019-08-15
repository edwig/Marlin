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
#include <HTTPServerMarlin.h>
#include <HTTPSite.h>
#include <SiteHandlerGet.h>
#include <SiteHandlerHead.h>
#include "MarlinService.h"

#ifdef _DEBUG
#define DEBUG_NEW new( _NORMAL_BLOCK, __FILE__, __LINE__)
#define new DEBUG_NEW
#endif

MarlinService* s_service = NULL;
//

MarlinService::MarlinService(PWSTR pszServiceName,
                 BOOL fCanStop,
                 BOOL fCanShutdown,
                 BOOL fCanPauseContinue)
:m_marlinServer(NULL)
 ,m_logger("MarlinWebServer")
{
  m_name = (pszServiceName == NULL) ? L"" : pszServiceName;

  m_statusHandle = NULL;

  // The service runs in its own process.
  m_status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;

  // The service is starting.
  m_status.dwCurrentState = SERVICE_START_PENDING;

  // The accepted commands of the service.
  DWORD dwControlsAccepted = 0;// SERVICE_ACCEPT_SESSIONCHANGE;
  if (fCanStop) 
    dwControlsAccepted |= SERVICE_ACCEPT_STOP;
  if (fCanShutdown) 
    dwControlsAccepted |= SERVICE_ACCEPT_SHUTDOWN;
  if (fCanPauseContinue) 
    dwControlsAccepted |= SERVICE_ACCEPT_PAUSE_CONTINUE;
  m_status.dwControlsAccepted = dwControlsAccepted;

  m_status.dwWin32ExitCode = NO_ERROR;
  m_status.dwServiceSpecificExitCode = 0;
  m_status.dwCheckPoint = 0;
  m_status.dwWaitHint = 0;

  CString logfileName = WebConfig::GetExePath() + "WebServerLog.txt";
  int loglevel = HLL_TRACE; // HLL_ERRORS / HLL_LOGGING / HLL_LOGBODY / HLL_TRACE / HLL_TRACEDUMP
  m_logger.SetLogFilename(logfileName);
  m_logger.SetLogLevel(loglevel);
  m_logger.SetDoTiming(true);
  m_logger.SetDoEvents(false);
	m_logger.SetLogRotation(true);
  m_logger.SetCache(1000);
  m_logger.AnalysisLog(__FUNCTION__, LogType::LOG_INFO, false, _T("MarlinService in ctor"));
}

//

MarlinService::~MarlinService(void)
{
  m_logger.AnalysisLog(__FUNCTION__, LogType::LOG_INFO, false, _T("MarlinService in dtor"));
}


//

BOOL
MarlinService::Run()
{
  s_service = this;

  SERVICE_TABLE_ENTRYW serviceTable[] = {
    { m_name, ServiceMain },
    { NULL, NULL }
  };

  return StartServiceCtrlDispatcherW(serviceTable);
}

//

void WINAPI
MarlinService::ServiceMain(DWORD dwArgc, PWSTR *pszArgv)
{
  s_service->m_statusHandle = RegisterServiceCtrlHandlerExW(s_service->m_name, ServiceCtrlHandler, s_service);
  if (s_service->m_statusHandle == NULL) {
    throw GetLastError();
  }

  s_service->Start(dwArgc, pszArgv);
}


//

DWORD WINAPI
MarlinService::ServiceCtrlHandler(DWORD dwCtrl, DWORD /*dwEventType*/, LPVOID /*lpEventData*/, LPVOID lpContext)
{
  MarlinService* pThis = static_cast<MarlinService*>(lpContext);
  switch (dwCtrl)
  {
  case SERVICE_CONTROL_STOP:
    pThis->Stop();
    break;
  case SERVICE_CONTROL_PAUSE:
  case SERVICE_CONTROL_CONTINUE:
  case SERVICE_CONTROL_SHUTDOWN:
  case SERVICE_CONTROL_INTERROGATE:
  default:
    break;
  }
  return NO_ERROR;
}


//

void
MarlinService::Start(DWORD /*dwArgc*/, PWSTR* /*pszArgv*/)
{
  try {
    SetServiceStatus(SERVICE_START_PENDING);

    m_logger.AnalysisLog(__FUNCTION__, LogType::LOG_INFO, false, _T("starting"));

    SetServiceStatus(SERVICE_START_PENDING);
    m_marlinServer = new HTTPServerMarlin("MarlinWebServer");
    // connect log to the server
    m_marlinServer->SetLogging(&m_logger);
    m_marlinServer->SetLogLevel(m_logger.GetLogLevel());

    // See to it that we have error reporting
    m_marlinServer->SetErrorReport(&m_errorReport);

    // SETTING THE MANDATORY MEMBERS
    m_marlinServer->SetWebroot(WebConfig::GetExePath());

    bool result = false;
    SetServiceStatus(SERVICE_START_PENDING);
    if (m_marlinServer->GetLastError() == NO_ERROR) {
      // Running our server
      m_marlinServer->Run();

      // Wait max 3 seconds for the server to enter the main loop
      // After this the initialization of the server has been done
      for(int ind = 0; ind < 30; ++ind) {
        if(m_marlinServer->GetIsRunning()) {
          result = true;
          break;
        }
        SetServiceStatus(SERVICE_START_PENDING);
        Sleep(100);
      }
    }
    if (!result)
      throw 2;

    CString siteUrl = "/marlin/";
    // create site and add get handler
    HTTPSite* site = m_marlinServer->CreateSite(PrefixType::URLPRE_Strong,false,80,siteUrl);
    SiteHandler* handler = new SiteHandlerGet();
    site->SetHandler(HTTPCommand::http_get,handler);
		handler = new SiteHandlerHead();
    site->SetHandler(HTTPCommand::http_head,handler);
    // Server should forward all headers to the messages
    site->SetAllHeaders(true);

    // Start the site
    SetServiceStatus(SERVICE_START_PENDING);
    site->StartSite();

    SetServiceStatus(SERVICE_RUNNING);
  } catch (DWORD dwError) {
    m_logger.AnalysisLog(__FUNCTION__, LogType::LOG_INFO, true, "Service failed to start: %d.", dwError);

    SetServiceStatus(SERVICE_STOPPED, dwError);
  } catch (...) {
    m_logger.AnalysisLog(__FUNCTION__, LogType::LOG_INFO, false, "Service failed to start.");

    SetServiceStatus(SERVICE_STOPPED);
  }
}


//

void
MarlinService::Stop()
{
  DWORD dwOriginalState = m_status.dwCurrentState;
  try {
    SetServiceStatus(SERVICE_STOP_PENDING);
    m_logger.AnalysisLog(__FUNCTION__, LogType::LOG_INFO, false, "MarlinService in OnStop");

    if (m_marlinServer) {
      m_marlinServer->StopServer();
      delete m_marlinServer;
      m_marlinServer = NULL;
    }

    m_logger.AnalysisLog(__FUNCTION__, LogType::LOG_INFO, false, "completed");

    m_logger.ForceFlush();

    SetServiceStatus(SERVICE_STOPPED);
  } catch (DWORD dwError) {
    m_logger.AnalysisLog(__FUNCTION__, LogType::LOG_INFO, true, "Service failed to stop: %d.", dwError);

    SetServiceStatus(dwOriginalState);
  } catch (...) {
    m_logger.AnalysisLog(__FUNCTION__, LogType::LOG_INFO, false, "Service failed to stop.");

    SetServiceStatus(dwOriginalState);
  }
}

//

void
MarlinService::SetServiceStatus(DWORD dwCurrentState,
                               DWORD dwWin32ExitCode,
                               DWORD dwWaitHint)
{
  static DWORD dwCheckPoint = 1;

  // Fill in the SERVICE_STATUS structure of the service.
  m_status.dwCurrentState = dwCurrentState;
  m_status.dwWin32ExitCode = dwWin32ExitCode;
  m_status.dwWaitHint = dwWaitHint;

  m_status.dwCheckPoint = 
    ((dwCurrentState == SERVICE_RUNNING) ||
    (dwCurrentState == SERVICE_STOPPED)) ? 
    0 : dwCheckPoint++;

  // Report the status of the service to the SCM.
  ::SetServiceStatus(m_statusHandle, &m_status);
}
