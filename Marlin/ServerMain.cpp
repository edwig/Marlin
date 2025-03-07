/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: ServerMain.cpp
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
// FUNCTION
//
// Control code for the NT-Service of the product
// Service can be started/stopped or installed or removed
// Service can be run as a stand-alone server
//
#include "stdafx.h"
#include "Marlin.h"
#include "ServerMain.h"
#include "MarlinConfig.h"
#include "AppConfig.h"
#include "MarlinServer.h"
#include <strsafe.h>
#include "version.h"
#include "GetLastErrorAsString.h"
#include "AutoCritical.h"
#include "ExecuteProcess.h"
#include "RunRedirect.h"
#include "EventLogRegistration.h"
#include "ServiceReporting.h"
#include <winsvc.h>
#include "winerror.h"
#include <VersionHelpers.h>
#include <io.h>

#ifdef _AFX
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#endif

// Each buffer in a event buffer array has a limit of 32K characters
// See MSDN: ReportEvent function
#define EVENTBUFFER     (32 * 1024)

// Global status. Must be global to report to the SCM
SERVICE_STATUS          g_svcStatus; 
SERVICE_STATUS_HANDLE   g_svcStatusHandle; 
HANDLE                  g_svcStopEvent = NULL;
SERVICE_STATUS_PROCESS  g_sspStatus; 
int                     g_runAsService = RUNAS_IISAPPPOOL;
XString                 g_serverName;
XString                 g_baseURL;

// Handle to service manager and service
SC_HANDLE g_schSCManager = NULL;
SC_HANDLE g_schService   = NULL;

// Stand alone program mutex names for start/stop handling
TCHAR  g_eventNameRunning     [SERVICE_NAME_LENGTH];
TCHAR  g_mutexNamePendingStart[SERVICE_NAME_LENGTH];
TCHAR  g_mutexNameRunning     [SERVICE_NAME_LENGTH];
TCHAR  g_mutexNamePendingStop [SERVICE_NAME_LENGTH];

HANDLE g_mtxStarting = NULL;
HANDLE g_mtxRunning  = NULL;
HANDLE g_mtxStopping = NULL;
HANDLE g_eventSource = NULL;

// THE MAIN PROGRAM

int _tmain(int argc,TCHAR* argv[],TCHAR* /*envp[]*/)
{
  HMODULE hModule = ::GetModuleHandle(NULL);
  bool standAloneStart = false;

  if(hModule == NULL)
  {
    _tprintf(_T("Fatal error: Windows-OS GetModuleHandle failed\n"));
    return 1;
  }

  // Load ServerApp constants before anything else
  LoadConstants(argv[0]);

  // Read the products config file
  ReadConfig();

  // If command-line parameter command is given like "install"
  // handle that command instead of starting the service
  // Otherwise, the service is probably being started by the SCM.
  if(argc >= 2)
  {
    if(lstrcmp(argv[1],_T("/?")) == 0 || lstrcmpi(argv[1],_T("help")) == 0)
    {
      PrintCopyright();
      PrintHelp();
      return 5;
    }
    else if(lstrcmpi(argv[1],_T("install")) == 0)
    {
      if(argc != 4)
      {
        PrintHelp();
        return 6;
      }
      PrintCopyright();
      if(g_runAsService == RUNAS_NTSERVICE)
      {
        return SvcInstall(argv[2],argv[3]);
      }
      _tprintf(_T("Can only 'install' for an NT-Service configuration!\n"));
      return -1;
    }
    else if(lstrcmpi(argv[1],_T("uninstall")) == 0)
    {
       PrintCopyright();
      if(g_runAsService == RUNAS_NTSERVICE)
      {
        return SvcDelete();
      }
      DeleteEventLogRegistration();
      _tprintf(_T("Can only 'uninstall' for an NT-Service configuration!\n"));
      _tprintf(_T("But un-installed the WMI event-log registration.\n"));
      return -1;
   }
    else if(lstrcmpi(argv[1],_T("start")) == 0)
    {
      PrintCopyright();
      switch(g_runAsService)
      {
        case RUNAS_STANDALONE: return StandAloneStart();
        case RUNAS_NTSERVICE:  return SvcStart();
        case RUNAS_IISAPPPOOL: return StartIISApp();
      }
    }
    else if(lstrcmpi(argv[1],_T("stop")) == 0)
    {
      PrintCopyright();
      switch(g_runAsService)
      {
        case RUNAS_STANDALONE: return StandAloneStop();
        case RUNAS_NTSERVICE:  return SvcStop();
        case RUNAS_IISAPPPOOL: return StopIISApp();
      }
    }
    else if(lstrcmpi(argv[1],_T("query")) == 0)
    {
      PrintCopyright();
      // SERVICE_STOPPED           0x00000001
      // SERVICE_START_PENDING     0x00000002
      // SERVICE_STOP_PENDING      0x00000003
      // SERVICE_RUNNING           0x00000004
      // SERVICE_CONTINUE_PENDING  0x00000005
      // SERVICE_PAUSE_PENDING     0x00000006
      // SERVICE_PAUSED            0x00000007
      switch(g_runAsService)
      {
        case RUNAS_STANDALONE: return QueryServiceStandAlone();
        case RUNAS_NTSERVICE:  return QueryService();
        case RUNAS_IISAPPPOOL: return QueryIISApp();
      }
    }
    else if(lstrcmpi(argv[1],_T("restart")) == 0)
    {
      PrintCopyright();
      switch(g_runAsService)
      {
        case RUNAS_STANDALONE:if(StandAloneStop() == 0)
                              {
                                return StandAloneStart();
                              }
        case RUNAS_NTSERVICE: if(SvcStop() == 0)
                              {
                                return SvcStart();
                              }
                              break;
        case RUNAS_IISAPPPOOL:if(StopIISApp() == 0)
                              {
                                return StartIISApp();
                              }
                              break;
      }
      // Error code from SvcStop
      return SERVICE_STOPPED;
    }
    else if(lstrcmpi(argv[1],g_svcname) == 0)
    {
      // Start argument is service name
      // Used for stand-alone service start
      standAloneStart = true;
    }
    else if (lstrcmpi(argv[1], _T("debug")) == 0)
    {
      standAloneStart = true;
      g_runAsService  = RUNAS_STANDALONE;
    }
  }

  // STARTED WITHOUT ARGUMENTS OR AS A STANDALONE SERVICE
  BOOL started = FALSE;

  if(g_runAsService == RUNAS_NTSERVICE)
  {
    // This process has one (1) service
    SERVICE_TABLE_ENTRY DispatchTable[] = 
    { 
      { g_svcname, (LPSERVICE_MAIN_FUNCTION) SvcMain }, 
      { NULL,      NULL                              } 
    }; 
    // This call returns when the service has stopped. 
    // The process should simply terminate when the call returns.
    started = StartServiceCtrlDispatcher(DispatchTable); 
  }
  else if(g_runAsService == RUNAS_STANDALONE)
  {
    if(standAloneStart)
    {
      // This call returns when the service has stopped. 
      // The process should simply terminate when the call returns.
      started = SvcInitStandAlone();
    }
    else
    {
      started = FALSE;
      SetLastError(ERROR_INVALID_PARAMETER);
    }
  }
  else if(g_runAsService == RUNAS_IISAPPPOOL)
  {
    // Should not come to here
  }

  // If in error : Show what we know about it
  if(started == FALSE)
  { 
    XString reason;
    XString error = XString(_T("The start of the  ")) + XString(PRODUCT_NAME) +  _T(" service has failed.");
    int errorCode = GetLastError();
    switch(errorCode)
    {
      case ERROR_INVALID_DATA: 
           reason = _T("Intern: function SvcMain not found"); 
           break;
      case ERROR_SERVICE_ALREADY_RUNNING:
           reason = _T("De service was already started");
           break;
      case ERROR_FAILED_SERVICE_CONTROLLER_CONNECT:
           reason = _T("No connection with the service-control-manager");
           break;
      case ERROR_INVALID_PARAMETER:
           reason = _T("No (known) service name given at the start of the server");
           break;
      default:
           reason = _T("Unknown error");
           break;
    }
    // Tell it to the WMI log
    SvcReportErrorEvent(0,false,_T("main"),error); 
    // Tell it to the user
    _tprintf(_T("%s\n%s\n\n"),PRODUCT_DISPLAY_NAME,PRODUCT_COPYRIGHT);
    _tprintf(_T("ERROR : %s\n")
             _T("REASON: %s.\n")
             ,error.GetString()
             ,reason.GetString());
  }
  return 0;
}

void ReadConfig()
{
  AppConfig config(_T(""));
  config.ReadConfig();

  int instance = config.GetParameterInteger(SECTION_APPLICATION,_T("Instance"),1);
  _stprintf_s(g_svcname,SERVICE_NAME_LENGTH,_T("%s_%d_v%s"),PRODUCT_NAME,instance,PRODUCT_VERSION);

  // Run as a service or as a stand-alone program?
  g_runAsService = config.GetParameterInteger(SECTION_APPLICATION,_T("RunAsService"),RUNAS_NTSERVICE);

  // Global server name = IIS application pool
  g_serverName = config.GetParameterString(SECTION_APPLICATION,_T("Name"),PRODUCT_NAME);
  g_baseURL    = config.GetParameterString(SECTION_APPLICATION,_T("BaseURL"),_T("/"));

  // Make the names?
  if(g_runAsService == RUNAS_STANDALONE)
  {
    // Controlling event of the service
    _tcsncpy_s(g_eventNameRunning,g_svcname,     SERVICE_NAME_LENGTH);
    // Status mutexes
    _tcsncpy_s(g_mutexNameRunning,g_svcname,     SERVICE_NAME_LENGTH);
    _tcscat_s (g_mutexNameRunning,               SERVICE_NAME_LENGTH,_T("_Running"));
    _tcsncpy_s(g_mutexNamePendingStart,g_svcname,SERVICE_NAME_LENGTH);
    _tcscat_s (g_mutexNamePendingStart,          SERVICE_NAME_LENGTH,_T("_PendingStart"));
    _tcsncpy_s(g_mutexNamePendingStop, g_svcname,SERVICE_NAME_LENGTH);
    _tcscat_s (g_mutexNamePendingStop,           SERVICE_NAME_LENGTH,_T("_PendingStop"));
  }
}

void PrintCopyright()
{
  _tprintf(_T("%s\n%s\n\n"),PRODUCT_DISPLAY_NAME,PRODUCT_COPYRIGHT);
  fflush(stdout);
  CheckPlatform();
}

void PrintHelp()
{
  _tprintf(_T("%s [command [[username password]]\n"),PRODUCT_NAME);
  _tprintf(_T("Without a command the server will start in the SCM\n"));
  _tprintf(_T("Commands are:\n"));
  _tprintf(_T("- help or /?    This help page\n"));
  _tprintf(_T("- install       Install the %s Service (needs username password)\n"),PRODUCT_NAME);
  _tprintf(_T("- uninstall     Remove  the %s Service from the system\n"),PRODUCT_NAME);
  _tprintf(_T("- query         Query   the %s Service status\n"),PRODUCT_NAME);
  _tprintf(_T("- start         Start   the %s Service\n"),PRODUCT_NAME);
  _tprintf(_T("- stop          Stop    the %s Service\n"),PRODUCT_NAME);
  _tprintf(_T("- restart       Bounce  the %s Service\n"),PRODUCT_NAME);
}

#pragma warning (disable: 4996)

// Check platform always before the server does it's 'install' action
// So we will always see this sad message.
void
CheckPlatform()
{
  if(IsWindows7OrGreater() == false)
  {
    // Windows95, Windows98, WindowsME (VER_PLATFORM_WIN32_NT)
    // Windows NT 3 (dwMajorVersion == 3)
    // Windows NT 4 (dwMajorVersion == 4)
    // Windows 2000 (dwMajorVersion == 5 && dwMinorVersion == 0) // DisconnectEx voor sockets !!!!
    XString msg;
    msg.Format(_T("You are running a version of the MS-Windows operating system with a non-supported TCP/IP queuing mechanism.\n")
               _T("%s only works on the Windows-7, 8, 8.1 & 10 and on the Windows 2008, 2012, 2016 Server platforms.\n")
               _T("Sorry for the inconvenience. Please contact ir. W.E. Huisman.\n"),PRODUCT_NAME);
    _tprintf(msg);
    MessageBox(NULL,msg,PRODUCT_NAME,MB_OK|MB_ICONERROR);
    _exit(-3);
  }
}

// Purpose:     Entry point for the service
// Parameters:  dwArgc - Number of arguments in the lpszArgv array
//              lpszArgv - Array of strings. The first string is the name of
//              the service and subsequent strings are passed by the process
//              that called the StartService function to start the service.
// Return value:None.
//
VOID WINAPI SvcMain(DWORD dwArgc,LPTSTR *lpszArgv)
{
  _set_se_translator(SeTranslator);

  // Register the handler function for the service
  g_svcStatusHandle = RegisterServiceCtrlHandler(g_svcname,SvcCtrlHandler);
  if(!g_svcStatusHandle)
  {
    SvcReportErrorEvent(0,false,_T(__FUNCTION__),_T("Cannot register a service handler for this service")); 
    return; 
  } 

  // These SERVICE_STATUS members remain as set here
  g_svcStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS; 
  g_svcStatus.dwServiceSpecificExitCode = 0;    

  // Tell it before we start
  SvcReportSuccessEvent(XString(PRODUCT_NAME) + _T(" Server starting..."));
  // Report initial status to the SCM
  ReportSvcStatus(SERVICE_START_PENDING,NO_ERROR,SVC_DEFAULT_SERVICE_START_PENDING);

  // Perform service-specific initialization and work.
  SvcInit(dwArgc,lpszArgv);
}

// Purpose:      The service code
//
// Parameters:   dwArgc - Number of arguments in the lpszArgv array
//               lpszArgv - Array of strings. The first string is the name of
//               the service and subsequent strings are passed by the process
//               that called the StartService function to start the service.
// Return value: None
//
VOID SvcInit(DWORD /*dwArgc*/,LPTSTR* /*lpszArgv*/)
{
  // TO_DO: Declare and set any required variables.
  //   Be sure to periodically call ReportSvcStatus() with 
  //   SERVICE_START_PENDING. If initialization fails, call
  //   ReportSvcStatus with SERVICE_STOPPED.

  // Register event source for the WMI
  XString eventlog = XString(PRODUCT_NAME) + _T("\\") + g_svcname;

  g_eventSource = RegisterEventSource(NULL,eventlog);
  DeregisterEventSource(g_eventSource);
  g_eventSource = NULL;

  // Create an event. The control handler function, SvcCtrlHandler,
  // signals this event when it receives the stop control code.

  g_svcStopEvent = CreateEvent(NULL,    // default security attributes
                               TRUE,    // manual reset event
                               FALSE,   // not signaled
                               NULL);   // no name

  if(g_svcStopEvent == NULL)
  {
    ReportSvcStatus(SERVICE_STOPPED,NO_ERROR,0);
    return;
  }

  // START THE SERVICE THREADS
  if(s_theServer->Startup())
  {
    // Tell we have started
    SvcReportSuccessEvent(XString(PRODUCT_NAME) + _T(" server started."));
    // Report running status when initialization is complete.
    ReportSvcStatus(SERVICE_RUNNING,NO_ERROR,0);

    while(1)
    {
      // Check whether to stop the service.
      WaitForSingleObject(g_svcStopEvent, INFINITE);

      SvcReportSuccessEvent(XString(PRODUCT_NAME) + _T(" server about to stop."));
      ReportSvcStatus(SERVICE_STOP_PENDING,NO_ERROR,0);

      // Stop the server
      // STOP THE SERVICE THREADS
      s_theServer->ShutDown();

      SvcReportSuccessEvent(XString(PRODUCT_NAME) + _T(" server is stopped."));
      ReportSvcStatus(SERVICE_STOPPED,NO_ERROR,0);
      return;
    }
  }
}

// Number of times we report the service status (starting at 1)
static DWORD dwCheckPoint = 1;

// Purpose:     Sets the current service status and reports it to the SCM.
//
// Parameters:  dwCurrentState  - The current state (see SERVICE_STATUS)
//              dwWin32ExitCode - The system error code
//              dwWaitHint      - Estimated time for pending operation, 
//                                in milliseconds
// Return value:None
//
VOID ReportSvcStatus(DWORD dwCurrentState
                    ,DWORD dwWin32ExitCode
                    ,DWORD dwWaitHint)
{

  // Fill in the SERVICE_STATUS structure.

  g_svcStatus.dwCurrentState   = dwCurrentState;
  g_svcStatus.dwWin32ExitCode  = dwWin32ExitCode;
  g_svcStatus.dwWaitHint       = dwWaitHint;

  if (dwCurrentState == SERVICE_START_PENDING)
  {
    g_svcStatus.dwControlsAccepted = 0;
  }
  else 
  {
    g_svcStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
  }
  if((dwCurrentState == SERVICE_RUNNING) ||
     (dwCurrentState == SERVICE_STOPPED) )
  {
    g_svcStatus.dwCheckPoint = 0;
  }
  else 
  {
    g_svcStatus.dwCheckPoint = dwCheckPoint++;
  }
  // Report the status of the service to the SCM.
  SetServiceStatus(g_svcStatusHandle,&g_svcStatus);
}

// Purpose:      Called by SCM whenever a control code is sent to the service
//               using the ControlService function.
// Parameters:   dwCtrl - control code
// Return value: None
//
VOID WINAPI SvcCtrlHandler(DWORD dwCtrl)
{
  // Handle the requested control code. 

  switch(dwCtrl) 
  {  
    case SERVICE_CONTROL_STOP: 
      ReportSvcStatus(SERVICE_STOP_PENDING, NO_ERROR, 0);
      // Signal the service to stop.
      SetEvent(g_svcStopEvent);
      return;

    case SERVICE_CONTROL_INTERROGATE: 
      // Fall through to send current status.
      break; 

    default: 
      break;
  } 
  ReportSvcStatus(g_svcStatus.dwCurrentState, NO_ERROR, 0);
}

// Purpose:      Installs a service in the SCM database
// Parameters:   None
// Return value: 0 = OK, 3 = Error
//
int SvcInstall(LPCTSTR username,LPCTSTR password)
{
  SC_HANDLE schSCManager;
  SC_HANDLE schService;
  TCHAR     szPath[MAX_PATH];

  // Tell who we are
  _tprintf(_T("Installing the [%s] service.\n"),g_svcname);

  if( !GetModuleFileName( NULL, szPath, MAX_PATH ) )
  {
    _tprintf(_T("Cannot install service. %s\n"),GetLastErrorAsString().GetString());
    return 3;
  }

  // Get a handle to the SCM database. 
  schSCManager = OpenSCManager(NULL,                    // local computer
                               NULL,                    // ServicesActive database 
                               SC_MANAGER_ALL_ACCESS);  // full access rights 

  if(schSCManager == NULL) 
  {
    _tprintf(_T("Making connection with the service manager has failed\n"));
    _tprintf(_T("Function OpenSCManager failed. %s\n"),GetLastErrorAsString().GetString());
    return 3;
  }

  // Create the service
  schService = CreateService(schSCManager,              // SCM database 
                             g_svcname,                   // name of service 
                             g_svcname,                   // service name to display 
                             SERVICE_ALL_ACCESS,        // desired access 
                             SERVICE_WIN32_OWN_PROCESS, // service type 
                             SERVICE_AUTO_START,        // start type 
                             SERVICE_ERROR_NORMAL,      // error control type 
                             szPath,                    // path to service's binary 
                             NULL,                      // no load ordering group 
                             NULL,                      // no tag identifier 
                             NULL,                      // no dependencies 
                             username,                  // real account
                             password);                 // password 
  if(schService == NULL) 
  {
    _tprintf(_T("Creating the service has failed\n"));
    _tprintf(_T("Function CreateService failed. %s\n"),GetLastErrorAsString().GetString());
    CloseServiceHandle(schSCManager);
    return 3;
  }
  else 
  {
    _tprintf(_T("Service installed successfully!\n")); 
  }
  XString installed;
  installed.Format(_T("%s successfully installed!"),g_svcname);
  SvcReportSuccessEvent(installed);


  // Set the service description
  SERVICE_DESCRIPTION desc;
  desc.lpDescription = const_cast<TCHAR*>(PRODUCT_DISPLAY_NAME);

  if(!ChangeServiceConfig2(schService,SERVICE_CONFIG_DESCRIPTION,reinterpret_cast<void*>(&desc)))
  {
    _tprintf(_T("WARNING: Service description NOT set.\n"));
    _tprintf(_T("ChangeServiceConfig2 failed: %s\n"),GetLastErrorAsString().GetString());
  }

  // Set actions after failure
  SERVICE_FAILURE_ACTIONS actions;
  SC_ACTION restart[6] = 
  {
    { SC_ACTION_RESTART, 3000 }
   ,{ SC_ACTION_RESTART, 6000 }
   ,{ SC_ACTION_RESTART,12000 }
   ,{ SC_ACTION_RESTART,24000 }
   ,{ SC_ACTION_NONE,       0 }
  };
  actions.dwResetPeriod = 600;  // reset count after 10 minutes
  actions.lpRebootMsg   = const_cast<TCHAR*>(PRODUCT_NAME);
  actions.lpCommand     = _T("");
  actions.cActions      = 5;
  actions.lpsaActions   = restart;

  if(!ChangeServiceConfig2(schService,SERVICE_CONFIG_FAILURE_ACTIONS,reinterpret_cast<void*>(&actions)))
  {
    _tprintf(_T("WARNING: Service failure actions NOT set.\n"));
    _tprintf(_T("ChangeServiceConfig2 failed: %s\n"),GetLastErrorAsString().GetString());
  }

  // And register the message dll in the registry
  InstallMessageDLL();

  CloseServiceHandle(schService); 
  CloseServiceHandle(schSCManager);

  return 0;
}

// Install the dll that holds the messages
// for the WIndows WMI logging facility
int
InstallMessageDLL()
{
  XString error;
  
  int result = RegisterMessagesDllForService(g_svcname,PRODUCT_MESSAGES_DLL,error);
  if(!result)
  {
    _tprintf(_T("%s\n"),error.GetString());
  }
  return result;
}

// Open handle to ServiceMangager and the service
// Handle all error situations once
bool
OpenMarlinService(DWORD p_access)
{
  // Get a handle to the SCM database. 
  g_schSCManager = OpenSCManager(NULL,                    // local computer
                                 NULL,                    // ServicesActive database 
                                 SC_MANAGER_ALL_ACCESS);  // full access rights 
  if(g_schSCManager == NULL) 
  {
    _tprintf(_T("Making connection with the service manager has failed\n"));
    _tprintf(_T("OpenSCManager failed: %s\n"),GetLastErrorAsString().GetString());
    return false;
  }
  // Get a handle to the service.
  g_schService = OpenService(g_schSCManager,       // SCM database 
                             g_svcname,            // name of service 
                             p_access);          // need delete access 
  if(g_schService == NULL)
  { 
    XString reason;
    int error = GetLastError();
    switch(error)
    {
      case ERROR_ACCESS_DENIED:           reason = _T("No access to this service");break;
      case ERROR_INVALID_NAME:            reason = _T("Misspelled service name");   break;
      case ERROR_SERVICE_DOES_NOT_EXIST:  reason = _T("Service does not exist");   break;
      default:                            reason = _T("Unknown error code");       break;
    }
    _tprintf(_T("Could not open the service with name [%s]\n"),g_svcname);
    _tprintf(_T("OpenService failed: [%d] %s\n"),error,reason.GetString());
    CloseServiceHandle(g_schSCManager);
    g_schSCManager = NULL;
    return false;
  }
  return true;
}

// And close service- and service manager handles
void
CloseMarlinService()
{
  CloseServiceHandle(g_schService); 
  CloseServiceHandle(g_schSCManager);
  g_schService   = NULL;
  g_schSCManager = NULL;
}

// Get last status (optionally closing the handles)
bool
GetMarlinServiceStatus(bool p_close = true)
{
  DWORD dwBytesNeeded;

  if (!QueryServiceStatusEx(g_schService,                           // handle to service 
                            SC_STATUS_PROCESS_INFO,                 // information level
                            reinterpret_cast<LPBYTE>(&g_sspStatus), // address of structure
                            sizeof(SERVICE_STATUS_PROCESS),         // size of structure
                            &dwBytesNeeded ) )                      // size needed if buffer is too small
  {
    XString reason;
    int error = GetLastError();
    switch(error)
    {
      case ERROR_ACCESS_DENIED:       reason = _T("Access denied");        break;
      case ERROR_INSUFFICIENT_BUFFER: reason = _T("Insufficient buffer");  break;
      case ERROR_INVALID_PARAMETER:   reason = _T("Invalid parameter");    break;
      case ERROR_INVALID_LEVEL:       reason = _T("Invalid info level");   break;
      case ERROR_SHUTDOWN_IN_PROGRESS:reason = _T("Shutdown in progress"); break;
      default:                        reason = _T("Unknown error");        break;
    }
    _tprintf(_T("The service status could not be determined\n"));
    _tprintf(_T("QueryServiceStatusEx failed [%d] %s\n"),error,reason.GetString());
    if(p_close)
    {
      CloseMarlinService();
    }
    return false; 
  }
  return true;
}

// Purpose:     Deletes a service from the SCM database
// Parameters:  None
// Return value:0 = OK, 4 = error
// 
int SvcDelete()
{
  int retval = 0;
  OpenMarlinService(DELETE);

  // Delete the service.
  if(!DeleteService(g_schService)) 
  {
    _tprintf(_T("The service cannot be removed\n"));
    _tprintf(_T("DeleteService failed: %s\n"),GetLastErrorAsString().GetString());
    if(GetLastError() == ERROR_SERVICE_MARKED_FOR_DELETE)
    {
      _tprintf(_T("The service has been marked for delete, but not yet removed\n"));
      _tprintf(_T("Wait until the service is fully removed.\n"));
    }
    retval = 4;
  }
  else 
  {
    _tprintf(_T("Service deleted successfully\n")); 
    XString success;
    success.Format(_T("%s successfully removed from the system!"),g_svcname);
    SvcReportSuccessEvent(success);
  }
  CloseMarlinService();
  DeleteEventLogRegistration();

  return retval;
}

// Remove the message DLL for the WMI windows logging system
// from the registry
void
DeleteEventLogRegistration()
{
  XString error;
  if(!UnRegisterMessagesDllForService(g_svcname, error))
  {
    _tprintf(error);
  }
}

// Purpose:     Starts the service if possible.
// Parameters:  None
// Return value:0 = OK, 1 = error
int SvcStart()
{
  // According to all samples, this needs to be "SERVICE_ALL_ACCESS" 
  // But starting rights + query rights is sufficient
  OpenMarlinService(SERVICE_START|SERVICE_QUERY_STATUS);  
  // Check the status in case the service is not stopped. 
  if(!GetMarlinServiceStatus())
  {
    return 1;
  }

  // Save the tick count and initial checkpoint.
  ULONGLONG dwStartTickCount = GetTickCount64();
  DWORD     dwOldCheckPoint  = g_sspStatus.dwCheckPoint;

  // Check if the service is already running. It would be possible 
  // to stop the service here, but for simplicity this example just returns. 
  if(g_sspStatus.dwCurrentState != SERVICE_STOPPED && g_sspStatus.dwCurrentState != SERVICE_STOP_PENDING)
  {
    _tprintf(_T("Cannot start the service because it is already running\n"));
    CloseMarlinService();
    return 1; 
  }

  // Wait for the service to stop before attempting to start it.
  while(g_sspStatus.dwCurrentState == SERVICE_STOP_PENDING)
  {
    // Do not wait longer than the wait hint. A good interval is 
    // one-tenth of the wait hint but not less than 1 second  
    // and not more than 10 seconds. 
    DWORD dwWaitTime = g_sspStatus.dwWaitHint / 10;

    // Wait for a minimum of 1 second and a maximum of 10 second
    if(dwWaitTime < SVC_MINIMUM_WAIT_HINT)
    {
      dwWaitTime = SVC_MINIMUM_WAIT_HINT;
    }
    else if(dwWaitTime > SVC_MAXIMUM_WAIT_HINT)
    {
      dwWaitTime = SVC_MAXIMUM_WAIT_HINT;
    }
    Sleep(dwWaitTime);

    // Check the status until the service is no longer stop pending. 
    if(!GetMarlinServiceStatus())
    {
      return 1;
    }

    if(g_sspStatus.dwCheckPoint > dwOldCheckPoint)
    {
      // Continue to wait and check.
      dwStartTickCount = GetTickCount64();
      dwOldCheckPoint  = g_sspStatus.dwCheckPoint;
    }
    else
    {
      if((GetTickCount64() - dwStartTickCount) > g_sspStatus.dwWaitHint)
      {
        _tprintf(_T("Timeout waiting for service to stop\n"));
        CloseMarlinService();
        return 1; 
      }
    }
  }

  // Attempt to start the service.
  if (!StartService(g_schService,  // handle to service 
                    0,             // number of arguments 
                    NULL) )        // no arguments 
  {
    _tprintf(_T("StartService failed: %s\n"),GetLastErrorAsString().GetString());
    CloseMarlinService();
    return 1;  
  }
  else _tprintf(_T("Service start pending...\n")); 

  // Check the status until the service is no longer start pending. 
  if(!GetMarlinServiceStatus())
  {
    return 1;
  }

  // Save the tick count and initial checkpoint.
  dwStartTickCount = GetTickCount64();
  dwOldCheckPoint  = g_sspStatus.dwCheckPoint;

  while(g_sspStatus.dwCurrentState == SERVICE_START_PENDING) 
  { 
    // Do not wait longer than the wait hint. A good interval is 
    // one-tenth the wait hint, but no less than 1 second and no 
    // more than 10 seconds. 

    DWORD dwWaitTime = g_sspStatus.dwWaitHint / 10;

    if(dwWaitTime < SVC_MINIMUM_WAIT_HINT)
    {
      dwWaitTime = SVC_MINIMUM_WAIT_HINT;
    }
    else if( dwWaitTime > SVC_MAXIMUM_WAIT_HINT)
    {
      dwWaitTime = SVC_MAXIMUM_WAIT_HINT;
    }
    Sleep(dwWaitTime);

    // Check the status again. 
    if(!GetMarlinServiceStatus(false))
    {
      break;
    }

    if(g_sspStatus.dwCheckPoint > dwOldCheckPoint)
    {
      // Continue to wait and check.

      dwStartTickCount = GetTickCount64();
      dwOldCheckPoint  = g_sspStatus.dwCheckPoint;
    }
    else
    {
      if((GetTickCount64() - dwStartTickCount) > g_sspStatus.dwWaitHint)
      {
        // No progress made within the wait hint.
        break;
      }
    }
  } 

  // Determine whether the service is running.
  int retval = 1;
  if (g_sspStatus.dwCurrentState == SERVICE_RUNNING) 
  {
    _tprintf(_T("Service started successfully.\n")); 
    retval = 0;
  }
  else 
  { 
    _tprintf(_T("Service not started. \n"));
    _tprintf(_T("  Current State : %d\n"), g_sspStatus.dwCurrentState); 
    _tprintf(_T("  Exit Code     : %d\n"), g_sspStatus.dwWin32ExitCode); 
    _tprintf(_T("  Check Point   : %d\n"), g_sspStatus.dwCheckPoint); 
    _tprintf(_T("  Wait Hint     : %d\n"), g_sspStatus.dwWaitHint); 
  } 
  CloseMarlinService();
  return retval;
}

// Purpose:     Stops the service.
// Parameters:  None
// Return value:0 = OK, >= 0 error
// 
int SvcStop()
{
  ULONGLONG dwStartTime = GetTickCount64();
  ULONGLONG dwTimeout   = SVC_MAXIMUM_STOP_TIME; // 30-second time-out

  OpenMarlinService(SERVICE_STOP | 
                    SERVICE_QUERY_STATUS | 
                    SERVICE_ENUMERATE_DEPENDENTS);

  // Make sure the service is not already stopped.
  if(!GetMarlinServiceStatus())
  {
    return 2;
  }
  if(g_sspStatus.dwCurrentState == SERVICE_STOPPED)
  {
    _tprintf(_T("Service is already stopped.\n"));
    goto stop_cleanup;
  }

  // If a stop is pending, wait for it.
  while ( g_sspStatus.dwCurrentState == SERVICE_STOP_PENDING ) 
  {
    _tprintf(_T("Service stop pending...\n"));
    Sleep( g_sspStatus.dwWaitHint );
    if(!GetMarlinServiceStatus())
    {
      return 2;
    }
    if( g_sspStatus.dwCurrentState == SERVICE_STOPPED )
    {
      _tprintf(_T("Service stopped successfully.\n"));
      goto stop_cleanup;
    }

    if ((GetTickCount64() - dwStartTime) > dwTimeout )
    {
      _tprintf(_T("Service stop timed out.\n"));
      goto stop_cleanup;
    }
  }

  _tprintf(_T("Service stop pending...\n"));

  // If the service is running, dependencies must be stopped first.
  StopDependentServices();

  // Send a stop code to the service.
  if(!ControlService(g_schService,SERVICE_CONTROL_STOP,(LPSERVICE_STATUS) &g_sspStatus))
  {
    _tprintf( _T("ControlService failed: %s\n"),GetLastErrorAsString().GetString());
    goto stop_cleanup;
  }

  // Wait for the service to stop.
  while ( g_sspStatus.dwCurrentState != SERVICE_STOPPED ) 
  {
    Sleep( g_sspStatus.dwWaitHint );
    if(!GetMarlinServiceStatus())
    {
      return 2;
    }
    if ( g_sspStatus.dwCurrentState == SERVICE_STOPPED )
    {
      break;
    }
    if((GetTickCount64() - dwStartTime) > dwTimeout )
    {
      _tprintf( _T("Wait timed out\n") );
      goto stop_cleanup;
    }
  }
  _tprintf(_T("Service stopped successfully\n"));

stop_cleanup:
  CloseMarlinService();
  return 0;
}

BOOL StopDependentServices()
{
  DWORD dwBytesNeeded;
  DWORD dwCount;
  BOOL  result = TRUE;

  LPENUM_SERVICE_STATUS   lpDependencies = NULL;
  ENUM_SERVICE_STATUS     ess;
  SERVICE_STATUS_PROCESS  ssp;

  ULONGLONG dwStartTime = GetTickCount64();

  // Pass a zero-length buffer to get the required buffer size.
  if(EnumDependentServices(g_schService
                          ,SERVICE_ACTIVE
                          ,lpDependencies
                          ,0
                          ,&dwBytesNeeded
                          ,&dwCount)) 
  {
    // If the Enum call succeeds, then there are no dependent
    // services, so do nothing.
    return TRUE;
  } 
  else 
  {
    if(GetLastError() != ERROR_MORE_DATA)
    {
      return FALSE; // Unexpected error
    }
    // Allocate a buffer for the dependencies.
    lpDependencies = (LPENUM_SERVICE_STATUS) HeapAlloc(GetProcessHeap()
                                                      ,HEAP_ZERO_MEMORY
                                                      ,dwBytesNeeded);
    if ( !lpDependencies )
    {
      return FALSE;
    }
    __try 
    {
      // Enumerate the dependencies.
      if (!EnumDependentServices(g_schService
                                ,SERVICE_ACTIVE
                                ,lpDependencies
                                ,dwBytesNeeded
                                ,&dwBytesNeeded
                                ,&dwCount))
      {
        result = FALSE;
        __leave;
      }
      for(DWORD index = 0; index < dwCount;++index) 
      {
        ess = *(lpDependencies + index);
        // Open the service.
        SC_HANDLE hDepService = OpenService(g_schSCManager, 
                                            ess.lpServiceName, 
                                            SERVICE_STOP | SERVICE_QUERY_STATUS );
        if ( !hDepService )
        {
          result = FALSE;
          __leave;
        }

        __try 
        {
          // Send a stop code.
          if(!ControlService( hDepService, 
                              SERVICE_CONTROL_STOP,
                              (LPSERVICE_STATUS) &ssp ) )
          {
            result = FALSE;
            __leave;
          }
          // Wait for the service to stop.
          while(ssp.dwCurrentState != SERVICE_STOPPED) 
          {
            Sleep(ssp.dwWaitHint);
            if(!QueryServiceStatusEx(hDepService,
                                     SC_STATUS_PROCESS_INFO,
                                     reinterpret_cast<LPBYTE>(&ssp),
                                     sizeof(SERVICE_STATUS_PROCESS),
                                     &dwBytesNeeded))
            {
              result = FALSE;
              __leave;
            }
            if ( ssp.dwCurrentState == SERVICE_STOPPED )
            {
              break;
            }
            if(GetTickCount64() - dwStartTime > SVC_MAXIMUM_STOP_TIME)
            {
              result = FALSE;
              __leave;
            }
          }
        } 
        __finally 
        {
          // Always release the service handle.
          CloseServiceHandle(hDepService);
        }
      }
    } 
    __finally 
    {
      // Always free the enumeration buffer.
      HeapFree( GetProcessHeap(), 0, lpDependencies );
    }
  } 
  return result;
}

int QueryService()
{
  int result = 0;

  // query rights is sufficient
  OpenMarlinService(SERVICE_QUERY_STATUS);  
  // Check the status in case the service is not stopped. 
  if(GetMarlinServiceStatus())
  {
    result = g_sspStatus.dwCurrentState;
    switch(result)
    {
      case SERVICE_STOPPED:           _tprintf(_T("Service is stopped\n"));             break;
      case SERVICE_START_PENDING:     _tprintf(_T("Service has a pending start\n"));    break;
      case SERVICE_STOP_PENDING:      _tprintf(_T("Service has a pending stop\n"));     break;
      case SERVICE_RUNNING:           _tprintf(_T("Service is running\n"));             break;
      case SERVICE_CONTINUE_PENDING:  _tprintf(_T("Service has a pending continue\n")); break;
      case SERVICE_PAUSE_PENDING:     _tprintf(_T("Service has a pending pause\n"));    break;
      case SERVICE_PAUSED:            _tprintf(_T("Service is paused\n"));              break;
    }
  }
  else
  {
    _tprintf(_T("Cannot get the current service state!\n"));
  }
  CloseMarlinService();

  return result;
}

//////////////////////////////////////////////////////////////////////////
//
// STAND ALONE PART
//
//////////////////////////////////////////////////////////////////////////

// Purpose:     Starts the program if possible.
// Parameters:  None
// Return value:0 = OK, 1 = error
int StandAloneStart()
{
  // For the debugging of the stand-alone solution
  // ::MessageBox(NULL,"Wait for debugger (Visual Studio) before we continue","DEBUGGER",MB_OK);

  int     status = 0;
  int     retval = 1;  // Still not lucky
  int     startResult = 0;
  DWORD   dwWaitTime;
  DWORD   dwWaited = 0;
  XString emptyString;
  XString program(APPLICATION_NAME);
  XString arguments(g_svcname);

  // Do not wait longer than the wait hint. A good interval is 
  // one-tenth of the wait hint but not less than 1 second  
  // and not more than 10 seconds. 
  dwWaitTime = SVC_MINIMUM_WAIT_HINT;
  
  // Check the status in case the service is not stopped. 
  if(!GetMarlinServiceStatusStandAlone(status))
  {
    goto end_of_startalone;
  }

  // Check if the service is already running. It would be possible 
  // to stop the service here, but for simplicity this example just returns. 
  if(status != SERVICE_STOPPED && status != SERVICE_STOP_PENDING)
  {
    _tprintf(_T("Cannot start the service because it is already running\n"));
    goto end_of_startalone;
  }

  // Wait for the service to stop before attempting to start it.
  while (status == SERVICE_STOP_PENDING)
  {
    Sleep(dwWaitTime);
    dwWaited += dwWaitTime;

    // Check the status until the service is no longer stop pending. 
    if(!GetMarlinServiceStatusStandAlone(status))
    {
      goto end_of_startalone;
    }

    if(dwWaited > SVC_MAXIMUM_WAIT_HINT)
    {
      _tprintf(_T("Timeout waiting for service to stop\n"));
      goto end_of_startalone;
    }
  }

  // Attempt to start the service.
  startResult = ExecuteProcess(program,arguments,true,emptyString,SW_HIDE);
  if(startResult)
  {
    _tprintf(_T("StartService failed: %s\n"),GetLastErrorAsString().GetString());
    goto end_of_startalone;
  }
  else 
  {
    _tprintf(_T("Service start pending...\n")); 
  }

  // Wait at least 1 time, otherwise we never get a status back
  Sleep(dwWaitTime);

  // Check the status until the service is no longer start pending. 
  if(!GetMarlinServiceStatusStandAlone(status))
  {
    goto end_of_startalone;
  }

  // Save the tick count and initial checkpoint.
  dwWaited = 0;

  // Do not wait longer than the wait hint. A good interval is 
  // one-tenth the wait hint, but no less than 1 second and no 
  // more than 10 seconds. 
  dwWaitTime = SVC_MINIMUM_WAIT_HINT;
  
  while (status == SERVICE_START_PENDING) 
  { 
    Sleep(dwWaitTime);

    // Check the status again. 
    if(!GetMarlinServiceStatusStandAlone(status))
    {
      break;
    }
    if(dwWaited > SVC_MAXIMUM_WAIT_HINT)
    {
      // No progress made within the wait hint.
      break;
    }
  } 

  // Determine whether the service is running.
  if (status  == SERVICE_RUNNING) 
  {
    _tprintf(_T("Service started successfully.\n"));
    retval = 0;
  }
  else 
  { 
    _tprintf(_T("Service not started. \n"));
    _tprintf(_T("  Current State : %d\n"), status); 
    _tprintf(_T("  Exit Code     : %d\n"), startResult); 
    _tprintf(_T("  Wait Hint     : %d\n"), 10); 
  } 

end_of_startalone:
  // EOT = End of transmission. 
  // Looks very strange, and it IS!
  // But it's the only way to work with a background stream
  _ftprintf(stdout,_T("\004"));
  _ftprintf(stderr,_T("\004"));
  return retval;
}

// Purpose:      The service code
// Return value: None
//
BOOL SvcInitStandAlone()
{
  // For the debugging of the stand alone solution
  // ::MessageBox(NULL,"Wait for debugger (Visual Studio) before we continue","DEBUGGER",MB_OK);

  // Create an event. The control handler function, SvcCtrlHandler,
  // signals this event when it receives the stop control code.

  g_svcStopEvent = CreateEvent(NULL,    // default security attributes
                               FALSE,   // Automatically reset the event
                               FALSE,   // not signaled
                               g_eventNameRunning);

  if(g_svcStopEvent == NULL)
  {
    SvcReportErrorEvent(0,false,_T(__FUNCTION__),_T("Server cannot start: cannot make a service event."));
    ReportSvcStatusStandAlone(SERVICE_STOPPED);
    // Remove mutexes
    return FALSE;
  }

  // Tell that we are about to start
  SvcReportSuccessEvent(XString(PRODUCT_NAME) + _T("Server about to start."));
  ReportSvcStatusStandAlone(SERVICE_START_PENDING);

  // And register the message dll in the registry
  InstallMessageDLL();

  // START THE SERVICE THREADS
  if(s_theServer->Startup())
  {
    // Tell we have started
    SvcReportSuccessEvent(XString(PRODUCT_NAME) + _T("Server started."));
    // Report running status when initialization is complete.
    ReportSvcStatusStandAlone(SERVICE_RUNNING);
    // create running mutex

    while(1)
    {
      // Check whether to stop the service.
      WaitForSingleObject(g_svcStopEvent, INFINITE);

      // Ready with this event
      CloseHandle(g_svcStopEvent);
      g_svcStopEvent = NULL;

      SvcReportSuccessEvent(XString(PRODUCT_NAME) + _T("Server about to stop."));
      ReportSvcStatusStandAlone(SERVICE_STOP_PENDING);

      // Stop the server
      s_theServer->ShutDown();

      SvcReportSuccessEvent(XString(PRODUCT_NAME) + _T("Server stopped."));
      ReportSvcStatusStandAlone(SERVICE_STOPPED);
      break;
    }
    return TRUE;
  }
  return FALSE;
}

void
ReportSvcStatusStandAlone(int p_status)
{
  bool deleteStarting = false;
  bool deleteRunning  = false;
  bool deleteStopping = false;

  // Create requested mutex
  if(p_status == SERVICE_START_PENDING)
  {
    g_mtxStarting = CreateMutex(NULL,FALSE,g_mutexNamePendingStart);
    if(g_mtxStarting == NULL)
    {
      g_mtxStarting = OpenMutex(NULL,FALSE,g_mutexNamePendingStart);
    }
    if(g_mtxStarting != NULL)
    {
      deleteRunning  = true;
      deleteStopping = true;
    }
  }
  else if(p_status == SERVICE_RUNNING)
  {
    g_mtxRunning = CreateMutex(NULL,FALSE,g_mutexNameRunning);
    if(g_mtxRunning == NULL)
    {
      g_mtxRunning = OpenMutex(NULL,FALSE,g_mutexNameRunning);
    }
    if(g_mtxRunning)
    {
      deleteStarting = true;
      deleteStopping = true;
    }
  }
  else if(p_status == SERVICE_STOP_PENDING)
  {
    g_mtxStopping = CreateMutex(NULL,FALSE,g_mutexNamePendingStop);
    if(g_mtxStopping == NULL)
    {
      g_mtxStopping = OpenMutex(NULL,FALSE,g_mutexNamePendingStop);
    }
    if(g_mtxStopping)
    {
      deleteStarting = true;
      deleteRunning  = true;
    }
  }

  // Close all other mutexes
  if(g_mtxStarting && deleteStarting)
  {
    CloseHandle(g_mtxStarting);
    g_mtxStarting = NULL;
  }
  if(g_mtxRunning && deleteRunning)
  {
    CloseHandle(g_mtxRunning);
    g_mtxRunning = NULL;
  }
  if(g_mtxStopping && deleteStopping)
  {
    CloseHandle(g_mtxStopping);
    g_mtxStopping = NULL;
  }
}

// Retrieve status
// 1 = Status retrieved
// 0 = Status NOT retrieved
int
GetMarlinServiceStatusStandAlone(int& p_status)
{
  DWORD _taccess = READ_CONTROL | SYNCHRONIZE;

  // Initially there is no service
  p_status = SERVICE_STOPPED;

  // Pending Start
  HANDLE starting = OpenMutex(_taccess,FALSE,g_mutexNamePendingStart);
  if(starting)
  {
    p_status = SERVICE_START_PENDING;
    CloseHandle(starting);
  }
  else
  {
    // Running
    HANDLE running = OpenMutex(_taccess,FALSE,g_mutexNameRunning);
    if(running)
    {
      p_status = SERVICE_RUNNING;
      CloseHandle(running);
    }
    else
    {
      // Pending stop
      HANDLE stopping = OpenMutex(_taccess,FALSE,g_mutexNamePendingStop);
      if(stopping)
      {
        p_status = SERVICE_STOP_PENDING;
        CloseHandle(stopping);
      }
      else
      {
        return 0;
      }
    }
  }
  // Ready getting service status
  return 1;
}

// Stop Marlin server as a stand-alone program
// return 0 = OK
// return 2 = error
int 
StandAloneStop()
{
  int       status = 0;
  int       retval = 2;   // still not lucky
  ULONGLONG dwStartTime = GetTickCount64();
  DWORD     dwTimeout   = SVC_MAXIMUM_STOP_TIME; // 30-second time-out
  DWORD     dwWaitTime  = SVC_MINIMUM_WAIT_HINT; // 1 second wait resolution
  DWORD     dwWaited    = 0;

  // Make sure the service is not already stopped.
  if(!GetMarlinServiceStatusStandAlone(status))
  {
    goto end_of_stop_standalone;
  }
  if(status == SERVICE_STOPPED)
  {
    retval = 0;
    _tprintf(_T("Service is already stopped.\n"));
    goto end_of_stop_standalone;
  }

  // If a stop is pending, wait for it.
  while(status == SERVICE_STOP_PENDING ) 
  {
    _tprintf(_T("Service stop pending...\n"));
    Sleep(dwWaitTime);
    dwWaited += dwWaitTime;

    if(!GetMarlinServiceStatusStandAlone(status))
    {
      goto end_of_stop_standalone;
    }
    if(status == SERVICE_STOPPED )
    {
      retval = 0;
      _tprintf(_T("Service stopped successfully.\n"));
      goto end_of_stop_standalone;
    }

    if((GetTickCount64() - dwStartTime) > dwTimeout)
    {
      retval = 0;
      _tprintf(_T("Service stop timed out.\n"));
      goto end_of_stop_standalone;
    }
  }

  _tprintf(_T("Service stop pending...\n"));

  g_svcStopEvent = OpenEvent(READ_CONTROL|SYNCHRONIZE|EVENT_MODIFY_STATE
                            ,FALSE,g_eventNameRunning);
  if(g_svcStopEvent == NULL)
  {
    retval = 0;
    _tprintf(_T("Cannot find a running server: %s\n"),GetLastErrorAsString().GetString());
    goto end_of_stop_standalone;
  }
  // Send a stop code to the service.
  if(!SetEvent(g_svcStopEvent))
  {
    retval = 0;
    _tprintf(_T("Sending stop event failed: %s\n"),GetLastErrorAsString().GetString());
    goto end_of_stop_standalone;
  }

  if(!GetMarlinServiceStatusStandAlone(status))
  {
    goto end_of_stop_standalone;
  }

  // Reset wait time
  dwWaited = 0;
  // Wait for the service to stop.
  while(status != SERVICE_STOPPED) 
  {
    Sleep(dwWaitTime);
    dwWaited += dwWaitTime;

    if(!GetMarlinServiceStatusStandAlone(status))
    {
      goto end_of_stop_standalone;
    }
    if(status == SERVICE_STOPPED)
    {
      break;
    }
    if((GetTickCount64() - dwStartTime) > dwTimeout)
    {
      retval = 0;
      _tprintf( _T("Wait timed out\n") );
      goto end_of_stop_standalone;
    }
  }

  // Success
  retval = 0;
  _tprintf(_T("Service stopped successfully\n"));

end_of_stop_standalone:
  // EOT = End of transmission. 
  // Looks very strange, and it IS!!
  // But it's the only way to work with a stream
  _ftprintf(stdout,_T("\004"));
  _ftprintf(stderr,_T("\004"));
  return retval;
}

int QueryServiceStandAlone()
{
  int result = 0;

  // Check the status in case the service is not stopped. 
  if(GetMarlinServiceStatusStandAlone(result))
  {
    switch(result)
    {
      case SERVICE_STOPPED:           _tprintf(_T("Service is stopped\n"));             break;
      case SERVICE_START_PENDING:     _tprintf(_T("Service has a pending start\n"));    break;
      case SERVICE_STOP_PENDING:      _tprintf(_T("Service has a pending stop\n"));     break;
      case SERVICE_RUNNING:           _tprintf(_T("Service is running\n"));             break;
//    case SERVICE_CONTINUE_PENDING:  printf("Service has a pending continue\n"); break;
//    case SERVICE_PAUSE_PENDING:     printf("Service has a pending pause\n");    break;
//    case SERVICE_PAUSED:            printf("Service is paused\n");              break;
    }
  }
  else
  {
    _tprintf(_T("Cannot get the current service state!\n"));
  }
  // EOT = End of transmission. 
  // Looks very strange, and it IS!!
  // But it's the only way to work with a stream
  _ftprintf(stdout,_T("\004"));
  _ftprintf(stderr,_T("\004"));
  return result;
}

//////////////////////////////////////////////////////////////////////////
//
// And yet again, but now for IIS
//
//////////////////////////////////////////////////////////////////////////

XString applicationCommand;

bool
FindApplicationCommand()
{
  // See if we already had the MS-Windows application command
  if(!applicationCommand.IsEmpty())
  {
    return true;
  }

  XString pathname;
  if(pathname.GetEnvironmentVariable(_T("windir")))
  {
    pathname += _T("\\system32\\inetsrv");

    DWORD attrib = GetFileAttributes(pathname);
    if(attrib & FILE_ATTRIBUTE_DIRECTORY)
    {
      pathname += _T("\\appcmd.exe");
      attrib = GetFileAttributes(pathname);
      if(attrib != INVALID_FILE_ATTRIBUTES)
      {
        applicationCommand = pathname;
        return true;
      }
    }
  }
  _tprintf(_T("MS-Windows directory for IIS not found: is the IIS system installed?\n"));
  return false;
}

// Starts the IIS application pool
// Return value: 0 = OK, 1 = error
int StartIISApp()
{
  int result = 1;

  InstallMessageDLL();

  if(FindApplicationCommand())
  {
    XString fout(_T("Cannot run program ") + applicationCommand);
 
    // STARTING THE APPLICATION POOL
    // APPCMD.EXE start APPPOOL <name>
    XString parameter(_T("start APPPOOL "));
    parameter += g_serverName;
    result = ExecuteProcess(applicationCommand,parameter,false,fout,SW_HIDE,true);

    if(result == 0)
    {
      // STARTING THE SITE
      XString site(g_baseURL);
      site.Remove('/');
      parameter = _T("start SITE ") + site;
      result = ExecuteProcess(applicationCommand,parameter,false,fout,SW_HIDE,true);
    }
  }
  return result;
}

// Stops the IIS application pool
// Return value: 0 = OK, 1 = error
int StopIISApp()
{
  int result = 1;

  if(FindApplicationCommand())
  {
    XString fout(_T("Cannot run program ") + applicationCommand);

    // STOP THE SITE
    // APPCMD.EXE start SITE <name>
    XString parameter(_T("stop SITE "));
    XString site(g_baseURL);
    site.Remove('/');
    parameter += site;
    result = ExecuteProcess(applicationCommand,parameter,false,fout,SW_HIDE,true);

    // STOP THE APPLICATION POOL
    // APPCMD.EXE start APPPOOL <name>
    parameter = _T("stop APPPOOL ");
    parameter += g_serverName;
    result    += ExecuteProcess(applicationCommand,parameter,false,fout,SW_HIDE,true);
  }
  return result;
}

// Prints wheter the IIS server is running or not
int QueryIISApp()
{
  int result = SERVICE_STOPPED;

  if(FindApplicationCommand())
  {
    // APPCMD.EXE list APPPOOL <name>
    XString parameter(_T("list APPPOOL "));
    parameter += g_serverName;
    XString output;
    int res = CallProgram_For_String(applicationCommand,parameter,output);
    if(res == 0 && (output.Find(_T("state:Started")) >= 0))
    {
      // APPCMD.EXE list SITE <baseurl>
      parameter = _T("list SITE ");
      XString site(g_baseURL);
      site.Remove('/');
      parameter += site;
      res = CallProgram_For_String(applicationCommand,parameter,output);
      if(res == 0 && (output.Find(_T("state:Started")) >= 0))
      {
        result = SERVICE_RUNNING;
      }
    }
  }

  switch(result)
  {
    case SERVICE_STOPPED: _tprintf(_T("Service is stopped\n")); break;
    case SERVICE_RUNNING: _tprintf(_T("Service is running\n")); break;
  }
  return result;
}
