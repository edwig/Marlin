/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: HostedWebCore.cpp
//
// Marlin Server: Internet server/client
// 
// Copyright (c) 2014-2021 ir. W.E. Huisman
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

//////////////////////////////////////////////////////////////////////////
//
// HOSTED WEB CORE
//
// The HostedWebCore is meant as a main executable stub to run the 
// IIS Marlin ServerApp from a console application. This is done to make
// it easier to debug IIS applications without first stopping and starting
// an IIS application pool and attaching the debugger to the W3WP.exe process
//
//////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "HostedWebCore.h"
#include "ServerApp.h"
#include "Analysis.h"
#include "Version.h"
#include <winsvc.h>
#include <conio.h>
#include <string>
#include <io.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

using std::wstring;

//////////////////////////////////////////////////////////////////////////
//
// GLOBALS
//
//////////////////////////////////////////////////////////////////////////

// Handle on the 'hwebcore.dll' module
HMODULE g_webcore = nullptr;    // Web core module
DWORD   g_hwcShutdownMode = 0;  // Shutdown mode 0=gracefull, 1=immediate
CString g_poolName;

// The Hosted Web Core API functions
PFN_WEB_CORE_ACTIVATE               HWC_Activate    = nullptr;
PFN_WEB_CORE_SET_METADATA_DLL_ENTRY HWC_SetMetadata = nullptr;
PFN_WEB_CORE_SHUTDOWN               HWC_Shutdown    = nullptr;

// Names of the application config files
CString g_applicationhost;      // ApplicationHost.config file to use
CString g_webconfig;            // Web.config file to use

// Server callbacks
PFN_SERVERSTATUS g_ServerStatus = nullptr;
PFN_SETMETADATA  g_SetMetaData  = nullptr;

//////////////////////////////////////////////////////////////////////////
//
// HEADER

void 
PrintHeader()
{
  printf("HOSTED WEB CORE development IIS replacement.\n");
  printf("============================================\n");
  printf("\n");
}


//////////////////////////////////////////////////////////////////////////
//
// CHECKING IIS
//
//////////////////////////////////////////////////////////////////////////

bool
FindIISRunning()
{
  bool result = false;
  // Get a handle to the SCM database. 
  SC_HANDLE manager = OpenSCManager(NULL,NULL,SC_MANAGER_ALL_ACCESS);
  if(manager == NULL)
  {
    printf("Making connection with the service manager has failed\n");
    printf("Cannot find the status of the IIS service!\n");
    return false;
  }
  // Get a handle to the service. IIS is actually called W3SVC
  SC_HANDLE service = OpenService(manager,"W3SVC",SERVICE_QUERY_STATUS);
  if(service)
  {
    DWORD bytesNeeded;
    SERVICE_STATUS_PROCESS  status; 
    if(QueryServiceStatusEx(service,                        // handle to service 
                            SC_STATUS_PROCESS_INFO,         // information level
                            (LPBYTE)&status,                // address of structure
                            sizeof(SERVICE_STATUS_PROCESS), // size of structure
                            &bytesNeeded))                  // size needed if buffer is too small
    {
      // Process ID is filled if service is really running
      if(status.dwProcessId)
      {
        result = true;
      }
    }
  }
  else
  {
    printf("Cannot find the IIS service! Have you installed it as a MS-Windows feature?\n");
  }
  CloseServiceHandle(service);
  CloseServiceHandle(manager);
  return result;
}

//////////////////////////////////////////////////////////////////////////
//
// Loading and unloading the webcore
//
//////////////////////////////////////////////////////////////////////////

bool 
LoadWebCore()
{
  g_webcore = ::LoadLibrary("inetsrv\\hwebcore.dll");
  if(g_webcore)
  {
    // Load all functions
    HWC_Activate    = (PFN_WEB_CORE_ACTIVATE)               GetProcAddress(g_webcore,"WebCoreActivate");
    HWC_SetMetadata = (PFN_WEB_CORE_SET_METADATA_DLL_ENTRY) GetProcAddress(g_webcore,"WebCoreSetMetadata");
    HWC_Shutdown    = (PFN_WEB_CORE_SHUTDOWN)               GetProcAddress(g_webcore,"WebCoreShutdown");

    if(HWC_Activate && HWC_SetMetadata && HWC_Shutdown)
    {
      printf("IIS Hosted Web Core is loaded.\n");
      return true;
    }
    printf("ERROR: IIS Hosted Web Core cannot be located. Aborting!\n");
  }
  return false;
}

void 
UnloadWebCore()
{
  if(g_webcore)
  {
    ::FreeLibrary(g_webcore);
    g_webcore = NULL;
    printf("IIS Hosted Web Core unloaded.\n");
  }
}

//////////////////////////////////////////////////////////////////////////
//
// USING THE WEBCORE
//
//////////////////////////////////////////////////////////////////////////

bool 
ActivateWebCore()
{
  USES_CONVERSION;
  bool    result    = false;
  wstring apphost   = A2CW(g_applicationhost);
  wstring webconfig = A2CW(g_webconfig);
  wstring poolname  = L"POOLNAAM";

  if(HWC_Activate)
  {
    // Check for valid application host file in readable mode (4)
    if(_waccess(apphost.c_str(),4) != 0)
    {
      printf("ERROR: Cannot access the ApplicationHost.config file!\n");
      return result;
    }

    // Check for valid web.config file (if any) in readable mode
    if(!webconfig.empty())
    {
      if(_waccess(webconfig.c_str(),4) != 0)
      {
        printf("ERROR: Cannot access the web.config file!\n");
        return result;
      }
    }

    // GO STARTING WEB CORE
    HRESULT hres = (*HWC_Activate)(apphost.c_str(),webconfig.c_str(),poolname.c_str());

    // Handle result of the startup
    switch(HRESULT_CODE(hres))
    {
      case S_OK: 
        printf("IIS Hosted Web Core is started.\n");
        result = true; 
        break;
      case ERROR_SERVICE_ALREADY_RUNNING:
        printf("ERROR: Hosted Web Core already running!\n");
        break;
      case ERROR_INVALID_DATA:
        printf("ERROR: Invalid data in Hosted Web Core config file(s)!\n");
        break;
      case ERROR_ALREADY_EXISTS:
        printf("ERROR: The application pool is already running in IIS!\n");
        break;
      case ERROR_PROC_NOT_FOUND:
        printf("ERROR: The 'RegisterModule' procedure could not be found!\n");
        break;
      case ERROR_ACCESS_DENIED:
        printf("ERROR: Access denied. Are you running as local admin?\n");
        break;
      case ERROR_MOD_NOT_FOUND:
        printf("ERROR: The specified module in the config files could not be found!\n");
        break;
      default:
        printf("ERROR: Cannot start the Hosted Web Core. Error number: %X\n",HRESULT_CODE(hres));
        break;
    }
  }
  return result;
}

void 
ShutdownWebCore()
{
  if(HWC_Shutdown)
  {
    // Make sure shutdown mode is only 0 or 1
    if(g_hwcShutdownMode != 0)
    {
      g_hwcShutdownMode = 1;
    }

    // Tell that we will shutdown
    printf("Initiating Web Core shutdown...\n");

    // Try to shutdown the Hosted Web Core
    HRESULT hres = (*HWC_Shutdown)(g_hwcShutdownMode);

    // Show any errors or warnings
    switch(HRESULT_CODE(hres))
    {
      case S_OK: 
        printf("IIS Hosted Web Core shutdown.\n");
        break;
      case ERROR_SERVICE_NOT_ACTIVE:
        printf("ERROR: Hosted Web Core was not running!\n");
        break;
      case ERROR_INVALID_SERVICE_CONTROL:
        printf("ERROR: Hosted Web Core shutdown already in progress!\n");
        break;
      case ERROR_SERVICE_REQUEST_TIMEOUT:
        printf("ERROR: Hosted Web Core gracefull shutdown not possible!\n"
               "See IIS Admin for further shutdown actions and optoins!\n");
        break;
      default:
        printf("ERROR: Cannot shutdown the Hosted Web Core. Error number: %X\n",HRESULT_CODE(hres));
        break;
    }
  }
}

bool 
SetMetaData(CString p_datatype,CString p_value)
{
  USES_CONVERSION;
  bool    result = false;
  wstring type   = A2CW(p_datatype);
  wstring value  = A2CW(p_value);

  if(HWC_SetMetadata)
  {
    // Try set the metadata
    HRESULT hres = (*HWC_SetMetadata)(type.c_str(),value.c_str());

    // Show result from setting
    switch(HRESULT_CODE(hres))
    {
      case S_OK: 
        printf("METADATA [%s:%s] is set.\n",p_datatype.GetString(),p_value.GetString());
        result = true;
        break;
      case ERROR_NOT_SUPPORTED:
        printf("ERROR: Metadata not supported by Hosted Web Core [%s:%s]\n",p_datatype.GetString(),p_value.GetString());
        break;
      case ERROR_INVALID_DATA:
        printf("ERROR: Metadata for Hosted Web Core [%s:%s] contains invalid data!\n",p_datatype.GetString(),p_value.GetString());
        break;
      default:
        printf("ERROR: Metadata NOT set. Error number: %X\n",HRESULT_CODE(hres));
        break;
    }
  }
  return result;
}

//////////////////////////////////////////////////////////////////////////
//
// PARSE THE COMMAND LINE
// "path-to-exe" apphost [webconfig [poolname]]
//
//////////////////////////////////////////////////////////////////////////

static char g_staticAddress;

CString 
GetExeName()
{
  char buffer[_MAX_PATH + 1];

  // Getting the module handle, if any
  // If it fails, the process names will be retrieved
  // Thus we get the *.DLL handle in IIS instead of a
  // %systemdrive\system32\inetsrv\w3wp.exe path
  HMODULE module = NULL;
  GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                    GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT
                    ,static_cast<LPCTSTR>(&g_staticAddress)
                    ,&module);

  // Retrieve the path
  GetModuleFileName(module,buffer,_MAX_PATH);
  CString application(buffer);

  int position = application.ReverseFind('\\');
  if(position == 0)
  {
    return "";
  }
  return application.Mid(position + 1);
}

void
ParseCommandLine(int argc,char* argv[])
{
  // Setting the application pool name
  CString exeName = GetExeName();
  g_poolName = exeName.IsEmpty() ? MARLIN_PRODUCT_NAME : exeName;

  for(int ind = 1; ind < argc, argv[ind]; ++ind)
  {
    switch(ind)
    {
      case 1: g_applicationhost = argv[1]; break;
      case 2: g_webconfig       = argv[2]; break;
      case 3: g_poolName        = argv[3]; break;
    }
  }
  // Can be set by command line or by the application
  // through the external declarations of these global parameters
  if(g_applicationhost.IsEmpty() || g_poolName.IsEmpty())
  {
    printf("USAGE: <application> <ApplictionHost.config> <web.config> <applicationpool>\n");
  }
}

void 
TrySetMetadata()
{
  char buffer[80];
  CString variable,value;

  // Trying to get metadata to set
  printf("ENTER METADATA\n");
  printf("Variable: ");
  if(gets_s(buffer,80) == NULL)
  {
    printf("Cannot read variable!\n");
    return;
  }
  variable = buffer;

  // Getting the value
  printf("Value   : ");
  if(gets_s(buffer,80) == NULL)
  {
    printf("Cannot read value!\n");
    return;
  }
  value = buffer;

  // Go try to set it
  SetMetaData(variable,value);
}

// Printing of the console menu
void 
PrintMenu()
{
  CString line;
  for(int ind = 0; ind < 51; ++ind) line += "-";

  printf("+%s+\n",line.GetString());
  printf("| %-50s|\n","MENU HOSTED WEBCORE");
  printf("+%s+\n",line.GetString());
  printf("| %-50s|\n","");
  printf("| %-50s|\n","A) Server status");
  printf("| %-50s|\n","B) Server set metadata");
  printf("| %-50s|\n","C) Flush serverlog");
  printf("| %-50s|\n","D) Enter Metadata directly");
  printf("| %-50s|\n","");
  printf("| %-50s|\n","S) Stop");
  printf("| %-50s|\n","");
  printf("+%s+\n",line.GetString());
  printf("\n");
}

// Running the console menu for the hosted webcore
void 
RunHostedMenu()
{
  int ch = 0;

  do
  {
    PrintMenu();
    ch = toupper(_getch());
    printf("Choice made: %c\n",ch);

    switch(ch)
    {
      case 'A': if(g_ServerStatus)
                {
                  (*g_ServerStatus)();
                }
                break;
      case 'B': if(g_SetMetaData)
                {
                  (*g_SetMetaData)();
                }
                break;
      case 'C': if(g_analysisLog)
                {
                  g_analysisLog->ForceFlush();
                  printf("Serverlog flushed!\n");
                }
                break;
      case 'D': TrySetMetadata();
      default:  break;
    }
  } 
  while(ch != 'S');
}

//////////////////////////////////////////////////////////////////////////
//
// THE MAIN PROGRAM DRIVER
//
//////////////////////////////////////////////////////////////////////////

int 
HWC_main(int argc,char *argv[])
{
  int retval = 1;

  PrintHeader();
  if(FindIISRunning())
  {
    printf("Cannot use the Hosted Web Core: IIS still running!\n");
  }
  else
  {
    if(LoadWebCore())
    {
      ParseCommandLine(argc,argv);

      if(ActivateWebCore())
      {
        RunHostedMenu();
        ShutdownWebCore();
        retval = 0;
      }
      UnloadWebCore();
    }
  }
  printf("Press enter: ");
  _getch();
  return retval;
}
