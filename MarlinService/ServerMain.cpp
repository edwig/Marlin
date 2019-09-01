// BriefServer
// Controle code voor de NT-Service van het product
// Service kan gestart/gestopt geinstalleerd/ge-de-installeerd worden
//
#include "stdafx.h"
#include "WideMessageBox.h"
#include "ServerMain.h"
#include "ServiceReporting.h"
#include "BriefServer.h"
#include "Utilities.h"
#include "BriefConfig.h"
#include "EventLogRegistration.h"
#include <strsafe.h>
#include "GeefExePad.h"
#include "versie.h"
#include "DocumentMessages.h"
#include "GetLastErrorAsString.h"
#include "AutoCritical.h"
#include <winsvc.h>
#include <RunRedirect.h>
#include <StartProgramma.h>
#include "winerror.h"
#include <io.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// Het was in het verleden soms nodig een versie van de briefserver te bouwen
// met als enig doel om een service te verwijderen. Let erop dat dit kan
// met het command-line tool "sc" met het commando
// sc <servernaam> delete <servicenaam>
// Voorbeeld op de pc "nb-gebruiker":
//
// sc \\nb-gebruiker delete Key2Brief_BRIEFServer_1_v7.0.1
//
// LET OP: Wel in local-administrator mode draaien.

// Service naam is versie afhankelijk zodat er van iedere versie een server
// naast elkaar op een service machine kan draaien!!
#define SVCNAME               "Wocas_DocumentServer"
#define DISPLAY               "Server voor \"" PRODUCT_NAME "\": Correspondentie kwaliteit documenten."
#define SERVICE_NAME_LENGTH   50

// LET OP: g_svcname bestaat OOK in BriefServerApp.cpp in de IIS DLL configuratie
char                    g_svcname[SERVICE_NAME_LENGTH];
// Global status. Must be global to report to the SCM
SERVICE_STATUS          g_svcStatus; 
SERVICE_STATUS_HANDLE   g_svcStatusHandle; 
HANDLE                  g_svcStopEvent = NULL;
SERVICE_STATUS_PROCESS  g_sspStatus; 
int                     g_runAsService = RUNAS_NTSERVICE;
CString                 g_serverName;
CString                 g_baseURL;

// Handle to servicemanager and service
SC_HANDLE g_schSCManager = NULL;
SC_HANDLE g_schService   = NULL;

// Stand alone program mutex names for start/stop handling
char  g_eventNameRunning     [SERVICE_NAME_LENGTH];
char  g_mutexNamePendingStart[SERVICE_NAME_LENGTH];
char  g_mutexNameRunning     [SERVICE_NAME_LENGTH];
char  g_mutexNamePendingStop [SERVICE_NAME_LENGTH];

HANDLE g_mtxStarting = NULL;
HANDLE g_mtxRunning  = NULL;
HANDLE g_mtxStopping = NULL;
HANDLE g_eventSource = NULL;

using namespace std;

int _tmain(int argc, TCHAR* argv[], TCHAR* /*envp[]*/)
{
  HMODULE hModule = ::GetModuleHandle(NULL);
  bool standAloneStart = false;

  if(hModule == NULL)
  {
    _tprintf(_T("Fatal error: Windows-OS GetModuleHandle failed\n"));
    return -1;
  }

  // initialize MFC and print and error on failure
  if (!AfxWinInit(hModule, NULL, ::GetCommandLine(), 0))
  {
    _tprintf(_T("Initialisatie van MFC is mislukt\n"));
    return -1;
  }

  // Start the event buffer system
  SvcStartEventBuffer();

  // Zet Nederlandse taal in de SQL componenten voor Date/Time/Timestamp verwerking
  InitSQLComponents(SQLComponents::LN_DUTCH);

  // Check de runtimer
  CheckExePad(APPLICATIE_NAAM);

  // Read the products config file
  ReadConfig();

  // If command-line parameter command is given like "install"
  // handle that command instead of starting the service
  // Otherwise, the service is probably being started by the SCM.
  if(argc >= 2)
  {
    if(lstrcmp(argv[1],"/?") == 0 || lstrcmpi(argv[1],"help") == 0)
    {
      PrintCopyright();
      PrintHelp();
      return 5;
    }
    else if(lstrcmpi(argv[1],"install") == 0)
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
      printf("Can only 'install' for an NT-Service configuration!\n");
      return -1;
    }
    else if(lstrcmpi(argv[1],"uninstall") == 0)
    {
      PrintCopyright();
      if(g_runAsService == RUNAS_NTSERVICE)
      {
        return SvcDelete();
      }
      DeleteEventLogRegistration();
      printf("Can only 'uninstall' for an NT-Service configuration!\n");
      printf("But un-installed the WMI event-log registration.\n");
      return -1;
    }
    else if(lstrcmpi(argv[1],"start") == 0)
    {
      PrintCopyright();
      switch(g_runAsService)
      {
        case RUNAS_STANDALONE: return StandAloneStart();
        case RUNAS_NTSERVICE:  return SvcStart();
        case RUNAS_IISAPPPOOL: return StartIISApp();
      }
    }
    else if(lstrcmpi(argv[1],"stop") == 0)
    {
      PrintCopyright();
      switch(g_runAsService)
      {
        case RUNAS_STANDALONE: return StandAloneStop();
        case RUNAS_NTSERVICE:  return SvcStop();
        case RUNAS_IISAPPPOOL: return StopIISApp();
      }
    }
    else if(lstrcmpi(argv[1],"query") == 0)
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
    else if(lstrcmpi(argv[1],"restart") == 0)
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
      // Errorcode from SvcStop
      return SERVICE_STOPPED;
    }
    else if(lstrcmpi(argv[1],g_svcname) == 0)
    {
      // Start argument is service name
      // Used for stand-alone service start
      standAloneStart = true;
    }
	  else if (lstrcmpi(argv[1], "debug") == 0)
	  {
		  standAloneStart = true;
		  g_runAsService  = RUNAS_STANDALONE;
	  }
  }

  // STARTED WITHOUT ARGUMENTS OR AS A STANDALONE SERVICE
  BOOL started = FALSE;

  if(g_runAsService == RUNAS_NTSERVICE)
  {
    // This proces has one (1) service
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

  // Indien fout: laat zien wat we weten
  if(started == FALSE)
  { 
    CString reden;
    CString fout = "De start van de " PRODUCT_NAME " service is mislukt.";
    int errorCode = GetLastError();
    switch(errorCode)
    {
      case ERROR_INVALID_DATA: 
            reden = "Intern: functie SvcMain niet gevonden"; 
            break;
      case ERROR_SERVICE_ALREADY_RUNNING:
            reden = "De service was al opgestart";
            break;
      case ERROR_FAILED_SERVICE_CONTROLLER_CONNECT:
            reden = "Geen connectie met de service-control-manager";
            break;
      case ERROR_INVALID_PARAMETER:
            reden = "Geen (bekende) servicenaam opgegeven bij start van de server";
            break;
      default:
            reden = "Onbekende fout";
            break;
    }
    // Vertel het aan het WMI logboek
    SvcReportErrorEvent(false,"main",fout); 
    // Vertel het aan de gebruiker
    printf("%s\n%s\n\n",DISPLAY,RES_COPYRIGHT);
    printf("FOUT : %s\n"
           "REDEN: %s.\n"
          ,fout,reden);
  }
  return 0;
}

void ReadConfig()
{
  BriefConfig config;
  config.ReadConfig();

  int instance = config.GetInstance();
  sprintf_s(g_svcname,SERVICE_NAME_LENGTH,"%s_%d_v%s",SVCNAME,instance,SHM_VERSION);

  // Run as a service or as a stand-alone program?
  g_runAsService = config.GetRunAsService();

  // Globale server naam (IIS application pool)
  g_serverName = config.GetName();
  g_baseURL    = config.GetBaseURL();

  // Eventueel namen maken
  if(g_runAsService == RUNAS_STANDALONE)
  {
    // Controling event of the service
    strncpy_s(g_eventNameRunning,g_svcname,     SERVICE_NAME_LENGTH);
    // Status mutexes
    strncpy_s(g_mutexNameRunning,g_svcname,     SERVICE_NAME_LENGTH);
    strcat_s (g_mutexNameRunning,               SERVICE_NAME_LENGTH,"_Running");
    strncpy_s(g_mutexNamePendingStart,g_svcname,SERVICE_NAME_LENGTH);
    strcat_s (g_mutexNamePendingStart,          SERVICE_NAME_LENGTH,"_PendingStart");
    strncpy_s(g_mutexNamePendingStop, g_svcname,SERVICE_NAME_LENGTH);
    strcat_s (g_mutexNamePendingStop,           SERVICE_NAME_LENGTH,"_PendingStop");
  }
}

void PrintCopyright()
{
  printf("%s\n%s\n\n",DISPLAY,RES_COPYRIGHT);
  fflush(stdout);
  CheckPlatform();
}

void PrintHelp()
{
  printf("DocumentServer [command [[username password]]\n");
  printf("Without a command the server will start in the SCM\n");
  printf("Commands are:\n");
  printf("- help or /?    This help page\n");
  printf("- install       Install the " PRODUCT_NAME " Service (needs username password)\n");
  printf("- uninstall     Remove  the " PRODUCT_NAME " Service from the system\n");
  printf("- query         Query   the " PRODUCT_NAME " Service status\n");
  printf("- start         Start   the " PRODUCT_NAME " Service\n");
  printf("- stop          Stop    the " PRODUCT_NAME " Service\n");
  printf("- restart       Bounce  the " PRODUCT_NAME " Service\n");
}

#pragma warning (disable: 4996)

// Check platform wordt altijd minimaal aangelopen voordat de server de "install" kan doen
// Hierdoor krijgt men deze boodschap altijd te zien.
void
CheckPlatform()
{
  OSVERSIONINFO v_info;
  v_info.dwOSVersionInfoSize = sizeof(v_info);
  if (GetVersionEx(&v_info))
  {
    if(v_info.dwPlatformId != VER_PLATFORM_WIN32_NT || v_info.dwMajorVersion < 6)
    {
      // Windows95, Windows98, WindowsME (VER_PLATFORM_WIN32_NT)
      // Windows NT 3 (dwMajorVersion == 3)
      // Windows NT 4 (dwMajorVersion == 4)
      // Windows 2000 (dwMajorVersion == 5 && dwMinorVersion == 0) // DisconnectEx voor sockets !!!!
      CString msg("U draait op een operating systeem met een niet ondersteund TCP/IP, printer queue en/of printer spooler mechanisme.\n"
                  PRODUCT_NAME " werkt alleen onder Windows Vista, Windows-7,Windows 2008 Server R2, Windows-8 en Windows 2012 Server.\n"
                  "Sorry voor het ongemak. Neem contact op met de supportdesk van Hercules Social Housing B.V.\n");
      printf(msg);
      WideMessageBox(NULL,msg,PRODUCT_NAME,MB_OK|MB_ICONERROR);
      _exit(-3);
    }
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
  // Register the handler function for the service
  g_svcStatusHandle = RegisterServiceCtrlHandler(g_svcname,SvcCtrlHandler);
  if(!g_svcStatusHandle)
  {
    SvcReportErrorEvent(false,__FUNCTION__,"Kan geen service handler registreren voor deze service"); 
    return; 
  } 

  // These SERVICE_STATUS members remain as set here
  g_svcStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS; 
  g_svcStatus.dwServiceSpecificExitCode = 0;    

  // Vertel dat we gaan starten
  SvcReportSuccessEvent("DocumentServer gaat starten.");
  // Report initial status to the SCM
  ReportSvcStatus(SERVICE_START_PENDING, NO_ERROR, 3000);

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

  // Event source voor WMI registreren
  CString eventlog = CString(PRODUCT_NAME) + "\\" + g_svcname;

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

  theServer = new BriefServer();

  // START THE SERVICE THREADS
  if(theServer->Startup())
  {
    // Vertel dat we gestart zijn
    SvcReportSuccessEvent("DocumentServer opgestart.");
    // Report running status when initialization is complete.
    ReportSvcStatus(SERVICE_RUNNING,NO_ERROR,0);

    while(1)
    {
      // Check whether to stop the service.
      WaitForSingleObject(g_svcStopEvent, INFINITE);

      SvcReportSuccessEvent("DocumentServer gaat stoppen.");
      ReportSvcStatus(SERVICE_STOP_PENDING,NO_ERROR,0);

      // Stop the server
      // STOP THE SERVICE THREADS
      theServer->ShutDown();

      // And destroy it again
      delete theServer;
      theServer = nullptr;

      SvcReportSuccessEvent("DocumentServer is gestopt.");
      ReportSvcStatus(SERVICE_STOPPED,NO_ERROR,0);

      // Deallocate the logging buffer of the server
      SvcFreeEventBuffer();

      return;
    }
  }
}

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
  static DWORD dwCheckPoint = 1;

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
  SetServiceStatus( g_svcStatusHandle, &g_svcStatus );
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
int SvcInstall(char* username,char* password)
{
  SC_HANDLE schSCManager;
  SC_HANDLE schService;
  TCHAR     szPath[MAX_PATH];

  // Tell who we are
  printf("Installing the [%s] service.\n",g_svcname);

  if( !GetModuleFileName( NULL, szPath, MAX_PATH ) )
  {
    printf("Cannot install service. %s\n", GetLastErrorAsString());
    return 3;
  }

  // Get a handle to the SCM database. 
  schSCManager = OpenSCManager(NULL,                    // local computer
                               NULL,                    // ServicesActive database 
                               SC_MANAGER_ALL_ACCESS);  // full access rights 

  if(schSCManager == NULL) 
  {
    printf  ("Verbinding maken met de service manager is mislukt\n");
    printf("OpenSCManager failed. %s\n", GetLastErrorAsString());
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
    printf("Het aanmaken van de service is mislukt\n");
    printf("CreateService failed. %s\n", GetLastErrorAsString()); 
    CloseServiceHandle(schSCManager);
    return 3;
  }
  else 
  {
    printf("Service installed successfully!\n"); 
  }
  CString installed;
  installed.Format("%s succesfully installed!",g_svcname);
  SvcReportSuccessEvent(installed);


  // Set de service description
  SERVICE_DESCRIPTION desc;
  desc.lpDescription = DISPLAY;

  if(!ChangeServiceConfig2(schService,SERVICE_CONFIG_DESCRIPTION,(void*)&desc))
  {
    printf("WARNING: Service description NOT set.\n");
    printf("ChangeServiceConfig2 failed: %s\n", GetLastErrorAsString()); 
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
  actions.lpRebootMsg   = SVCNAME;
  actions.lpCommand     = "";
  actions.cActions      = 5;
  actions.lpsaActions   = restart;

  if(!ChangeServiceConfig2(schService,SERVICE_CONFIG_FAILURE_ACTIONS,(void*)&actions))
  {
    printf("WARNING: Service failure actions NOT set.\n");
    printf("ChangeServiceConfig2 failed: %s\n", GetLastErrorAsString()); 
  }

  // And register the message dll in the registry
  InstallMessageDLL();

  CloseServiceHandle(schService); 
  CloseServiceHandle(schSCManager);

  return 0;
}

// Install the dll that holds the messages
// for the WIndows WMI logboek facility
int
InstallMessageDLL()
{
  CString melding;
  int res = RegisterMessagesDllForService(g_svcname,"DocumentMessages.dll",melding);
  if(res != 1 || !melding.IsEmpty())
  {
    printf(melding);
  }
  return res;
}

// Open handle toe ServiceMangager and the service
// Handle all error situations once
bool
OpenBriefService(DWORD p_access)
{
  // Get a handle to the SCM database. 
  g_schSCManager = OpenSCManager(NULL,                    // local computer
                                 NULL,                    // ServicesActive database 
                                 SC_MANAGER_ALL_ACCESS);  // full access rights 
  if(g_schSCManager == NULL) 
  {
    printf("Making connection with the service manager has failed\n");
    printf("OpenSCManager failed: %s\n",GetLastErrorAsString());
    return false;
  }
  // Get a handle to the service.
  g_schService = OpenService(g_schSCManager,       // SCM database 
                             g_svcname,            // name of service 
                             p_access);          // need delete access 
  if(g_schService == NULL)
  { 
    CString reden;
    int error = GetLastError();
    switch(error)
    {
      case ERROR_ACCESS_DENIED:           reden = "No access to this service";break;
      case ERROR_INVALID_NAME:            reden = "Misspelled servicename";   break;
      case ERROR_SERVICE_DOES_NOT_EXIST:  reden = "Service does not exist";   break;
      default:                            reden = "Unknown error code";       break;
    }
    printf("Could not open the service with name [%s]\n",g_svcname);
    printf("OpenService failed: [%d] %s\n",error,reden);
    CloseServiceHandle(g_schSCManager);
    g_schSCManager = NULL;
    return false;
  }
  return true;
}

// And close service- and servicemanagerhandles
void
CloseBriefService()
{
  CloseServiceHandle(g_schService); 
  CloseServiceHandle(g_schSCManager);
  g_schService   = NULL;
  g_schSCManager = NULL;
}

// Get last status (optionally closing the handles)
bool
GetBriefServiceStatus(bool p_close = true)
{
  DWORD dwBytesNeeded;

  if (!QueryServiceStatusEx(g_schService,                     // handle to service 
                            SC_STATUS_PROCESS_INFO,         // information level
                            (LPBYTE) &g_sspStatus,             // address of structure
                            sizeof(SERVICE_STATUS_PROCESS), // size of structure
                            &dwBytesNeeded ) )              // size needed if buffer is too small
  {
    CString reason;
    int error = GetLastError();
    switch(error)
    {
      case ERROR_ACCESS_DENIED:       reason = "Access denied";        break;
      case ERROR_INSUFFICIENT_BUFFER: reason = "Insufficient buffer";  break;
      case ERROR_INVALID_PARAMETER:   reason = "Invalid parameter";    break;
      case ERROR_INVALID_LEVEL:       reason = "Invalid info level";   break;
      case ERROR_SHUTDOWN_IN_PROGRESS:reason = "Shutdown in progress"; break;
      default:                        reason = "Unknown error";        break;
    }
    printf("The service status could not be determined\n");
    printf("QueryServiceStatusEx failed [%d] %s\n",error,reason);
    if(p_close)
    {
      CloseBriefService();
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
  OpenBriefService(DELETE);

  // Delete the service.
  if(!DeleteService(g_schService)) 
  {
    printf("The service cannot be removed\n");
    printf("DeleteService failed: %s\n", GetLastErrorAsString()); 
    if(GetLastError() == ERROR_SERVICE_MARKED_FOR_DELETE)
    {
      printf("The service has been marked for delete, but not yet removed\n");
      printf("Wait until the service is fully removed.\n");
    }
    retval = 4;
  }
  else 
  {
    printf("Service deleted successfully\n"); 
    CString success;
    success.Format("%s successfully removed from the system!",g_svcname);
    SvcReportSuccessEvent(success);
  }
  CloseBriefService();
  DeleteEventLogRegistration();

  return retval;
}

// Remove the message DLL for the WMI windows logging system
// from the registry
void
DeleteEventLogRegistration()
{
  CString melding;
  if(UnRegisterMessagesDllForService(g_svcname,melding) == false)
  {
    printf(melding);
  }
}

// Purpose:     Starts the service if possible.
// Parameters:  None
// Return value:0 = OK, 1 = error
int SvcStart()
{
  DWORD dwOldCheckPoint; 
  DWORD dwStartTickCount;
  DWORD dwWaitTime;
  
  // Volgens samples zou dit "SERVICE_ALL_ACCESS" moeten zijn, maar 
  // startrechten + queryrechten is voldoende
  OpenBriefService(SERVICE_START|SERVICE_QUERY_STATUS);  
  // Check the status in case the service is not stopped. 
  if(!GetBriefServiceStatus())
  {
    return 1;
  }

  // Check if the service is already running. It would be possible 
  // to stop the service here, but for simplicity this example just returns. 
  if(g_sspStatus.dwCurrentState != SERVICE_STOPPED && g_sspStatus.dwCurrentState != SERVICE_STOP_PENDING)
  {
    printf("Cannot start the service because it is already running\n");
    CloseBriefService();
    return 1; 
  }

  // Wait for the service to stop before attempting to start it.
  while (g_sspStatus.dwCurrentState == SERVICE_STOP_PENDING)
  {
    // Save the tick count and initial checkpoint.
    dwStartTickCount = GetTickCount();
    dwOldCheckPoint  = g_sspStatus.dwCheckPoint;

    // Do not wait longer than the wait hint. A good interval is 
    // one-tenth of the wait hint but not less than 1 second  
    // and not more than 10 seconds. 
    dwWaitTime = g_sspStatus.dwWaitHint / 10;

    // Wait for a minimum of 1 second and a maximum of 10 second
    if( dwWaitTime < 1000 )
    {
      dwWaitTime = 1000;
    }
    else if ( dwWaitTime > 10000 )
    {
      dwWaitTime = 10000;
    }
    Sleep(dwWaitTime);

    // Check the status until the service is no longer stop pending. 
    if(!GetBriefServiceStatus())
    {
      return 1;
    }

    if(g_sspStatus.dwCheckPoint > dwOldCheckPoint)
    {
      // Continue to wait and check.
      dwStartTickCount = GetTickCount();
      dwOldCheckPoint = g_sspStatus.dwCheckPoint;
    }
    else
    {
      if(GetTickCount()-dwStartTickCount > g_sspStatus.dwWaitHint)
      {
        printf("Timeout waiting for service to stop\n");
        CloseBriefService();
        return 1; 
      }
    }
  }

  // Attempt to start the service.
  if (!StartService(g_schService,  // handle to service 
                    0,             // number of arguments 
                    NULL) )        // no arguments 
  {
    printf("StartService failed: %s\n",GetLastErrorAsString());
    CloseBriefService();
    return 1;  
  }
  else printf("Service start pending...\n"); 

  // Check the status until the service is no longer start pending. 
  if(!GetBriefServiceStatus())
  {
    return 1;
  }
  // Save the tick count and initial checkpoint.
  dwStartTickCount = GetTickCount();
  dwOldCheckPoint = g_sspStatus.dwCheckPoint;

  while (g_sspStatus.dwCurrentState == SERVICE_START_PENDING) 
  { 
    // Do not wait longer than the wait hint. A good interval is 
    // one-tenth the wait hint, but no less than 1 second and no 
    // more than 10 seconds. 

    dwWaitTime = g_sspStatus.dwWaitHint / 10;

    if( dwWaitTime < 1000 )
    {
      dwWaitTime = 1000;
    }
    else if ( dwWaitTime > 10000 )
    {
      dwWaitTime = 10000;
    }
    Sleep( dwWaitTime );

    // Check the status again. 
    if(!GetBriefServiceStatus(false))
    {
      break;
    }

    if ( g_sspStatus.dwCheckPoint > dwOldCheckPoint )
    {
      // Continue to wait and check.

      dwStartTickCount = GetTickCount();
      dwOldCheckPoint = g_sspStatus.dwCheckPoint;
    }
    else
    {
      if(GetTickCount()-dwStartTickCount > g_sspStatus.dwWaitHint)
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
    printf("Service started successfully.\n"); 
    retval = 0;
  }
  else 
  { 
    printf("Service not started. \n");
    printf("  Current State : %d\n", g_sspStatus.dwCurrentState); 
    printf("  Exit Code     : %d\n", g_sspStatus.dwWin32ExitCode); 
    printf("  Check Point   : %d\n", g_sspStatus.dwCheckPoint); 
    printf("  Wait Hint     : %d\n", g_sspStatus.dwWaitHint); 
  } 
  CloseBriefService();
  return retval;
}

// Purpose:     Stops the service.
// Parameters:  None
// Return value:0 = OK, >= 0 error
// 
int SvcStop()
{
  DWORD  dwStartTime = GetTickCount();
  DWORD  dwTimeout = 30000; // 30-second time-out

  OpenBriefService(SERVICE_STOP | 
                   SERVICE_QUERY_STATUS | 
                   SERVICE_ENUMERATE_DEPENDENTS);

  // Make sure the service is not already stopped.
  if(!GetBriefServiceStatus())
  {
    return 2;
  }
  if(g_sspStatus.dwCurrentState == SERVICE_STOPPED)
  {
    printf("Service is already stopped.\n");
    goto stop_cleanup;
  }

  // If a stop is pending, wait for it.
  while ( g_sspStatus.dwCurrentState == SERVICE_STOP_PENDING ) 
  {
    printf("Service stop pending...\n");
    Sleep( g_sspStatus.dwWaitHint );
    if(!GetBriefServiceStatus())
    {
      return 2;
    }
    if( g_sspStatus.dwCurrentState == SERVICE_STOPPED )
    {
      printf("Service stopped successfully.\n");
      goto stop_cleanup;
    }

    if ( GetTickCount() - dwStartTime > dwTimeout )
    {
      printf("Service stop timed out.\n");
      goto stop_cleanup;
    }
  }

  printf("Service stop pending...\n");

  // If the service is running, dependencies must be stopped first.
  StopDependentServices();

  // Send a stop code to the service.
  if(!ControlService(g_schService,SERVICE_CONTROL_STOP,(LPSERVICE_STATUS) &g_sspStatus))
  {
    printf( "ControlService failed: %s\n", GetLastErrorAsString());
    goto stop_cleanup;
  }

  // Wait for the service to stop.
  while ( g_sspStatus.dwCurrentState != SERVICE_STOPPED ) 
  {
    Sleep( g_sspStatus.dwWaitHint );
    if(!GetBriefServiceStatus())
    {
      return 2;
    }
    if ( g_sspStatus.dwCurrentState == SERVICE_STOPPED )
    {
      break;
    }
    if ( GetTickCount() - dwStartTime > dwTimeout )
    {
      printf( "Wait timed out\n" );
      goto stop_cleanup;
    }
  }
  printf("Service stopped successfully\n");

stop_cleanup:
  CloseBriefService();
  return 0;
}

BOOL StopDependentServices()
{
  DWORD i;
  DWORD dwBytesNeeded;
  DWORD dwCount;

  LPENUM_SERVICE_STATUS   lpDependencies = NULL;
  ENUM_SERVICE_STATUS     ess;
  SC_HANDLE               hDepService;
  SERVICE_STATUS_PROCESS  ssp;

  DWORD dwStartTime = GetTickCount();
  DWORD dwTimeout = 30000; // 30-second time-out

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
        return FALSE;
      }
      for ( i = 0; i < dwCount; i++ ) 
      {
        ess = *(lpDependencies + i);
        // Open the service.
        hDepService = OpenService(g_schSCManager, 
                                  ess.lpServiceName, 
                                  SERVICE_STOP | SERVICE_QUERY_STATUS );
        if ( !hDepService )
        {
          return FALSE;
        }

        __try 
        {
          // Send a stop code.
          if(!ControlService( hDepService, 
                              SERVICE_CONTROL_STOP,
                              (LPSERVICE_STATUS) &ssp ) )
          {
            return FALSE;
          }
          // Wait for the service to stop.
          while ( ssp.dwCurrentState != SERVICE_STOPPED ) 
          {
            Sleep( ssp.dwWaitHint );
            if( !QueryServiceStatusEx(hDepService, 
                                      SC_STATUS_PROCESS_INFO,
                                      (LPBYTE)&ssp, 
                                      sizeof(SERVICE_STATUS_PROCESS),
                                      &dwBytesNeeded ) )
            {
              return FALSE;
            }
            if ( ssp.dwCurrentState == SERVICE_STOPPED )
            {
              break;
            }
            if ( GetTickCount() - dwStartTime > dwTimeout )
            {
              return FALSE;
            }
          }
        } 
        __finally 
        {
          // Always release the service handle.
          CloseServiceHandle( hDepService );
        }
      }
    } 
    __finally 
    {
      // Always free the enumeration buffer.
      HeapFree( GetProcessHeap(), 0, lpDependencies );
    }
  } 
  return TRUE;
}

int QueryService()
{
  int result = 0;

  // queryrechten is voldoende
  OpenBriefService(SERVICE_QUERY_STATUS);  
  // Check the status in case the service is not stopped. 
  if(GetBriefServiceStatus())
  {
    result = g_sspStatus.dwCurrentState;
    switch(result)
    {
      case SERVICE_STOPPED:           printf("Service is stopped\n");             break;
      case SERVICE_START_PENDING:     printf("Service has a pending start\n");    break;
      case SERVICE_STOP_PENDING:      printf("Service has a pending stop\n");     break;
      case SERVICE_RUNNING:           printf("Service is running\n");             break;
      case SERVICE_CONTINUE_PENDING:  printf("Service has a pending continue\n"); break;
      case SERVICE_PAUSE_PENDING:     printf("Service has a pending pause\n");    break;
      case SERVICE_PAUSED:            printf("Service is paused\n");              break;
    }
  }
  else
  {
    printf("Cannot get the current service state!\n");
  }
  CloseBriefService();

  return result;
}

//////////////////////////////////////////////////////////////////////////
//
// STAND ALONE PROGRAMMA GEDEELTE
//
//////////////////////////////////////////////////////////////////////////

// Purpose:     Starts the program if possible.
// Parameters:  None
// Return value:0 = OK, 1 = error
int StandAloneStart()
{
  // Voor het debuggen van de stand-alone oplossing
  // ::MessageBox(NULL,"Wachten op debugger (Visual Studio) voordat we verder gaan","DEBUGGER",MB_OK);

  int   status = 0;
  int   retval = 1;  // Still not lucky
  // Save the tick count and initial checkpoint.
  DWORD dwStartTickCount = GetTickCount();
  DWORD dwWaitTime;
  DWORD dwWaited = 0;
  CString leeg;
  CString programma(BRIEF_MANAGER_NAAM ".exe");
  CString arguments(g_svcname);

  // Do not wait longer than the wait hint. A good interval is 
  // one-tenth of the wait hint but not less than 1 second  
  // and not more than 10 seconds. 
  dwWaitTime = 1000;
  
  // Check the status in case the service is not stopped. 
  if(!GetBriefServiceStatusStandAlone(status))
  {
    goto end_of_startalone;
  }

  // Check if the service is already running. It would be possible 
  // to stop the service here, but for simplicity this example just returns. 
  if(status != SERVICE_STOPPED && status != SERVICE_STOP_PENDING)
  {
    printf("Cannot start the service because it is already running\n");
    goto end_of_startalone;
  }

  // Wait for the service to stop before attempting to start it.
  while (status == SERVICE_STOP_PENDING)
  {
    Sleep(dwWaitTime);
    dwWaited += dwWaitTime;

    // Check the status until the service is no longer stop pending. 
    if(!GetBriefServiceStatusStandAlone(status))
    {
      goto end_of_startalone;
    }

    if(dwWaited > 10000)
    {
      printf("Timeout waiting for service to stop\n");
      goto end_of_startalone;
    }
  }

  // Attempt to start the service.
  int startResult = StartProgram(programma,arguments,true,leeg,nullptr,SW_HIDE);
  if(startResult)
  {
    printf("StartService failed: %s\n",GetLastErrorAsString());
    goto end_of_startalone;
  }
  else 
  {
    printf("Service start pending...\n"); 
  }

  // MInimaal 1 keer wachten, anders krijgen we nooit een status terug
  Sleep(dwWaitTime);

  // Check the status until the service is no longer start pending. 
  if(!GetBriefServiceStatusStandAlone(status))
  {
    goto end_of_startalone;
  }

  // Save the tick count and initial checkpoint.
  dwStartTickCount = GetTickCount();
  dwWaited = 0;

  // Do not wait longer than the wait hint. A good interval is 
  // one-tenth the wait hint, but no less than 1 second and no 
  // more than 10 seconds. 
  dwWaitTime = 1000;
  
  while (status == SERVICE_START_PENDING) 
  { 
    Sleep(dwWaitTime);

    // Check the status again. 
    if(!GetBriefServiceStatusStandAlone(status))
    {
      break;
    }
    if(dwWaited > 10000)
    {
      // No progress made within the wait hint.
      break;
    }
  } 

  // Determine whether the service is running.
  if (status  == SERVICE_RUNNING) 
  {
    printf("Service started successfully.\n");
    retval = 0;
  }
  else 
  { 
    printf("Service not started. \n");
    printf("  Current State : %d\n", status); 
    printf("  Exit Code     : %d\n", startResult); 
    printf("  Wait Hint     : %d\n", 10); 
  } 

end_of_startalone:
  // EOT = End of transmission. 
  // Dit ziet er vreemd uit en dat is het ook
  // maar het is de enige manier om de Serverapplet correct te laten werken
  fprintf(stdout,"\004");
  fprintf(stderr,"\004");
  return retval;
}

// Purpose:      The service code
// Return value: None
//
BOOL SvcInitStandAlone()
{
  // Voor het debuggen van de stand-alone oplossing
  // ::MessageBox(NULL,"Wachten op debugger (Visual Studio) voordat we verder gaan","DEBUGGER",MB_OK);

  // Create an event. The control handler function, SvcCtrlHandler,
  // signals this event when it receives the stop control code.

  g_svcStopEvent = CreateEvent(NULL,    // default security attributes
                               FALSE,   // Automatically reset the event
                               FALSE,   // not signaled
                               g_eventNameRunning);

  if(g_svcStopEvent == NULL)
  {
    SvcReportErrorEvent(false,__FUNCTION__,"DocumentServer kan niet starten: Kan geen service event maken.");
    ReportSvcStatusStandAlone(SERVICE_STOPPED);
    // Remove mutexes
    return FALSE;
  }

  // Vertel dat we gaan starten
  SvcReportSuccessEvent("DocumentServer gaat starten.");
  ReportSvcStatusStandAlone(SERVICE_START_PENDING);

  // And register the message dll in the registry
  InstallMessageDLL();

  theServer = new BriefServer();

  // START THE SERVICE THREADS
  if(theServer->Startup())
  {
    // Vertel dat we gestart zijn
    SvcReportSuccessEvent("DocumentServer opgestart.");
    // Report running status when initialization is complete.
    ReportSvcStatusStandAlone(SERVICE_RUNNING);
    // maak running mutex

    while(1)
    {
      // Check whether to stop the service.
      WaitForSingleObject(g_svcStopEvent, INFINITE);

      // Ready with this event
      CloseHandle(g_svcStopEvent);
      g_svcStopEvent = NULL;

      SvcReportSuccessEvent("DocumentServer gaat stoppen.");
      ReportSvcStatusStandAlone(SERVICE_STOP_PENDING);

      // Stop the server
      theServer->ShutDown();

      // And destroy again
      delete theServer;
      theServer = nullptr;

      SvcReportSuccessEvent("DocumentServer is gestopt.");
      ReportSvcStatusStandAlone(SERVICE_STOPPED);

      // Fre the event buffering system
      SvcFreeEventBuffer();

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

  // Maak aangevraagde mutex aan
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

  // Sluit alle andere mutexen
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

// Haal status op
// 1 = Status opgehaald
// 0 = Status NIET opgehaald
int
GetBriefServiceStatusStandAlone(int& p_status)
{
  DWORD access = READ_CONTROL | SYNCHRONIZE;

  // Initeel is er geen service
  p_status = SERVICE_STOPPED;

  // Pending Start
  HANDLE starting = OpenMutex(access,FALSE,g_mutexNamePendingStart);
  if(starting)
  {
    p_status = SERVICE_START_PENDING;
    CloseHandle(starting);
  }
  else
  {
    // Running
    HANDLE running = OpenMutex(access,FALSE,g_mutexNameRunning);
    if(running)
    {
      p_status = SERVICE_RUNNING;
      CloseHandle(running);
    }
    else
    {
      // Pending stop
      HANDLE stopping = OpenMutex(access,FALSE,g_mutexNamePendingStop);
      if(stopping)
      {
        p_status = SERVICE_STOP_PENDING;
        CloseHandle(stopping);
      }
    }
  }
  // Ready getting service status
  return 1;
}

// Stop DocumentServer as a stand-alone program
// return 0 = OK
// return 2 = error
int 
StandAloneStop()
{
  int    status = 0;
  int    retval = 2;   // still not lucky
  DWORD  dwStartTime = GetTickCount();
  DWORD  dwTimeout   = 30000; // 30-second time-out
  DWORD  dwWaitTime  = 1000;  // 1 second wait resolution
  DWORD  dwWaited    = 0;

  // Make sure the service is not already stopped.
  if(!GetBriefServiceStatusStandAlone(status))
  {
    goto end_of_stop_standalone;
  }
  if(status == SERVICE_STOPPED)
  {
    retval = 0;
    printf("Service is already stopped.\n");
    goto end_of_stop_standalone;
  }

  // If a stop is pending, wait for it.
  while(status == SERVICE_STOP_PENDING ) 
  {
    printf("Service stop pending...\n");
    Sleep(dwWaitTime);
    dwWaited += dwWaitTime;

    if(!GetBriefServiceStatusStandAlone(status))
    {
      goto end_of_stop_standalone;
    }
    if(status == SERVICE_STOPPED )
    {
      retval = 0;
      printf("Service stopped successfully.\n");
      goto end_of_stop_standalone;
    }

    if(GetTickCount() - dwStartTime > dwTimeout)
    {
      retval = 0;
      printf("Service stop timed out.\n");
      goto end_of_stop_standalone;
    }
  }

  printf("Service stop pending...\n");

  g_svcStopEvent = OpenEvent(READ_CONTROL|SYNCHRONIZE|EVENT_MODIFY_STATE
                            ,FALSE,g_eventNameRunning);
  if(g_svcStopEvent == NULL)
  {
    retval = 0;
    printf("Cannot find a running server: %s\n",GetLastErrorAsString());
    goto end_of_stop_standalone;
  }
  // Send a stop code to the service.
  if(!SetEvent(g_svcStopEvent))
  {
    retval = 0;
    printf("Sending stop event failed: %s\n", GetLastErrorAsString());
    goto end_of_stop_standalone;
  }

  if(!GetBriefServiceStatusStandAlone(status))
  {
    goto end_of_stop_standalone;
  }

  // Reset waittime
  dwWaited = 0;
  // Wait for the service to stop.
  while(status != SERVICE_STOPPED) 
  {
    Sleep(dwWaitTime);
    dwWaited += dwWaitTime;

    if(!GetBriefServiceStatusStandAlone(status))
    {
      goto end_of_stop_standalone;
    }
    if(status == SERVICE_STOPPED)
    {
      break;
    }
    if(GetTickCount() - dwStartTime > dwTimeout)
    {
      retval = 0;
      printf( "Wait timed out\n" );
      goto end_of_stop_standalone;
    }
  }

  // Success
  retval = 0;
  printf("Service stopped successfully\n");

end_of_stop_standalone:

  // Remove the message DLL for the WMI windows logging system
  DeleteEventLogRegistration();

  // EOT = End of transmission. 
  // Dit ziet er vreemd uit en dat is het ook
  // maar het is de enige manier om de Serverapplet correct te laten werken
  fprintf(stdout,"\004");
  fprintf(stderr,"\004");
  return retval;
}

int QueryServiceStandAlone()
{
  int result = 0;

  // Check the status in case the service is not stopped. 
  if(GetBriefServiceStatusStandAlone(result))
  {
    switch(result)
    {
      case SERVICE_STOPPED:           printf("Service is stopped\n");             break;
      case SERVICE_START_PENDING:     printf("Service has a pending start\n");    break;
      case SERVICE_STOP_PENDING:      printf("Service has a pending stop\n");     break;
      case SERVICE_RUNNING:           printf("Service is running\n");             break;
//    case SERVICE_CONTINUE_PENDING:  printf("Service has a pending continue\n"); break;
//    case SERVICE_PAUSE_PENDING:     printf("Service has a pending pause\n");    break;
//    case SERVICE_PAUSED:            printf("Service is paused\n");              break;
    }
  }
  else
  {
    printf("Cannot get the current service state!\n");
  }
  // EOT = End of transmission. 
  // Dit ziet er vreemd uit en dat is het ook
  // maar het is de enige manier om de Serverapplet correct te laten werken
  fprintf(stdout,"\004");
  fprintf(stderr,"\004");
  return result;
}


//////////////////////////////////////////////////////////////////////////
//
// En nog een keer, maar nu voor IIS
//
//////////////////////////////////////////////////////////////////////////

CString applicationCommand;

bool
FindApplicationCommand()
{
  // Kijk of we hem al een keer hadden opgezocht
  if(!applicationCommand.IsEmpty())
  {
    return true;
  }

  CString padnaam;
  padnaam.GetEnvironmentVariable("windir");
  padnaam += "\\system32\\inetsrv";

  DWORD attrib = GetFileAttributes(padnaam);
  if(attrib & FILE_ATTRIBUTE_DIRECTORY)
  {
    padnaam += "\\appcmd.exe";
    attrib = GetFileAttributes(padnaam);
    if(attrib != INVALID_FILE_ATTRIBUTES)
    {
      applicationCommand = padnaam;
      return true;
    }
  }
  printf("MS-Windows directory for IIS not found: is the IIS system installed?\n");
  return false;
}

// Starts the IIS application pool
// Return value: 0 = OK, 1 = error
int StartIISApp()
{
  int result = 1;

  if(FindApplicationCommand())
  {
    CString fout("Cannot run program " + applicationCommand);
 
    // STARTING THE APPLICATION POOL
    // appcmd start APPPOOL <name>
    CString parameter("start APPPOOL ");
    parameter += g_serverName;
    result = StartProgram(applicationCommand,parameter,false,fout,nullptr,SW_HIDE,true);

    if(result == 0)
    {
      // STARTING THE SITE
      CString site(g_baseURL);
      site.Remove('/');
      parameter = "start SITE " + site;
      result = StartProgram(applicationCommand,parameter,false,fout,nullptr,SW_HIDE,true);
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
    CString fout("Cannot run program " + applicationCommand);

    // STOP THE SITE
    // appcmd start SITE <name>
    CString parameter("stop SITE ");
    CString site(g_baseURL);
    site.Remove('/');
    parameter += site;
    result = StartProgram(applicationCommand,parameter,false,fout,nullptr,SW_HIDE,true);

    // STOP THE APPLICATION POOL
    // appcmd start APPPOOL <name>
    parameter = "stop APPPOOL ";
    parameter += g_serverName;
    result = StartProgram(applicationCommand,parameter,false,fout,nullptr,SW_HIDE,true);
  }
  return result;
}

int QueryIISApp()
{
  int result = SERVICE_STOPPED;

  if(FindApplicationCommand())
  {
    // appcmd list APPPOOL <name>
    CString parameter("list APPPOOL ");
    parameter += g_serverName;
    CString output;
    int res = CallProgram_For_String(applicationCommand,parameter,output);
    if(res == 0 && (output.Find("state:Started") >= 0))
    {
      // appcmd list SITE <baseurl>
      parameter = "list SITE ";
      CString site(g_baseURL);
      site.Remove('/');
      parameter += site;
      res = CallProgram_For_String(applicationCommand,parameter,output);
      if(res == 0 && (output.Find("state:Started") >= 0))
      {
        result = SERVICE_RUNNING;
      }
    }
  }

  switch(result)
  {
    case SERVICE_STOPPED:           printf("Service is stopped\n");             break;
    case SERVICE_RUNNING:           printf("Service is running\n");             break;
  }
  return result;
}
