/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: HostedWebCore.cpp
//
// Marlin Server: Internet server/client
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

#include "pch.h"
#include "HostedWebCore.h"
#include "ServerApp.h"
#include "LogAnalysis.h"
#include "Version.h"
#include <winsvc.h>
#include <conio.h>
#include <string>
#include <io.h>

using std::wstring;

//////////////////////////////////////////////////////////////////////////
//
// GLOBALS
//
//////////////////////////////////////////////////////////////////////////

// Handle on the 'hwebcore.dll' module
HMODULE g_webcore = nullptr;    // Web core module
DWORD   g_hwcShutdownMode = 0;  // Shutdown mode 0=gracefull, 1=immediate
XString g_poolName;

// The Hosted Web Core API functions
PFN_WEB_CORE_ACTIVATE               HWC_Activate    = nullptr;
PFN_WEB_CORE_SET_METADATA_DLL_ENTRY HWC_SetMetadata = nullptr;
PFN_WEB_CORE_SHUTDOWN               HWC_Shutdown    = nullptr;

// Names of the application config files
XString g_applicationhost;      // ApplicationHost.config file to use
XString g_webconfig;            // Web.config file to use

// Server callbacks
PFN_SERVERSTATUS g_ServerStatus = nullptr;
PFN_SETMETADATA  g_SetMetaData  = nullptr;

//////////////////////////////////////////////////////////////////////////
//
// HEADER

void 
PrintHeader()
{
  _tprintf(_T("HOSTED WEB CORE development IIS replacement.\n"));
  _tprintf(_T("============================================\n"));
  _tprintf(_T("\n"));
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
    _tprintf(_T("Making connection with the service manager has failed\n"));
    _tprintf(_T("Cannot find the status of the IIS service!\n"));
    return false;
  }
  // Get a handle to the service. IIS is actually called W3SVC
  SC_HANDLE service = OpenService(manager,_T("W3SVC"),SERVICE_QUERY_STATUS);
  if(service)
  {
    DWORD bytesNeeded;
    SERVICE_STATUS_PROCESS  status; 
    if(QueryServiceStatusEx(service,                            // handle to service 
                            SC_STATUS_PROCESS_INFO,             // information level
                            reinterpret_cast<LPBYTE>(&status),  // address of structure
                            sizeof(SERVICE_STATUS_PROCESS),     // size of structure
                            &bytesNeeded))                      // size needed if buffer is too small
    {
      // Process ID is filled if service is really running
      if(status.dwProcessId)
      {
        result = true;
      }
    }
    CloseServiceHandle(service);
  }
  else
  {
    _tprintf(_T("Cannot find the IIS service! Have you installed it as a MS-Windows feature?\n"));
  }
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
  g_webcore = ::LoadLibrary(_T("inetsrv\\hwebcore.dll"));
  if(g_webcore)
  {
    // Load all functions
    HWC_Activate    = (PFN_WEB_CORE_ACTIVATE)               GetProcAddress(g_webcore,"WebCoreActivate");
    HWC_SetMetadata = (PFN_WEB_CORE_SET_METADATA_DLL_ENTRY) GetProcAddress(g_webcore,"WebCoreSetMetadata");
    HWC_Shutdown    = (PFN_WEB_CORE_SHUTDOWN)               GetProcAddress(g_webcore,"WebCoreShutdown");

    if(HWC_Activate && HWC_SetMetadata && HWC_Shutdown)
    {
      _tprintf(_T("IIS Hosted Web Core is loaded.\n"));
      return true;
    }
    _tprintf(_T("ERROR: IIS Hosted Web Core cannot be located. Aborting!\n"));
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
    _tprintf(_T("IIS Hosted Web Core unloaded.\n"));
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
  bool    result    = false;
  wstring apphost   = StringToWString(g_applicationhost);
  wstring webconfig = StringToWString(g_webconfig);

  if(HWC_Activate)
  {
    // Check for valid application host file in readable mode (4)
    if(_waccess(apphost.c_str(),4) != 0)
    {
      _tprintf(_T("ERROR: Cannot access the ApplicationHost.config file!\n"));
      return result;
    }

    // Check for valid web.config file (if any) in readable mode
    if(!webconfig.empty())
    {
      if(_waccess(webconfig.c_str(),4) != 0)
      {
        _tprintf(_T("ERROR: Cannot access the web.config file!\n"));
        return result;
      }
    }

    // GO STARTING WEB CORE
    wstring poolname = L"POOLNAAM";
    HRESULT hres = (*HWC_Activate)(apphost.c_str(),webconfig.c_str(),poolname.c_str());

    // Handle result of the startup
    switch(HRESULT_CODE(hres))
    {
      case S_OK:                          _tprintf(_T("IIS Hosted Web Core is started.\n"));
                                          result = true; 
                                          break;
      case ERROR_SERVICE_ALREADY_RUNNING: _tprintf(_T("ERROR: Hosted Web Core already running!\n"));
                                          break;
      case ERROR_INVALID_DATA:            _tprintf(_T("ERROR: Invalid data in Hosted Web Core config file(s)!\n"));
                                          break;
      case ERROR_ALREADY_EXISTS:          _tprintf(_T("ERROR: The application pool is already running in IIS!\n"));
                                          break;
      case ERROR_PROC_NOT_FOUND:          _tprintf(_T("ERROR: The 'RegisterModule' procedure could not be found!\n"));
                                          break;
      case ERROR_ACCESS_DENIED:           _tprintf(_T("ERROR: Access denied. Are you running as local administrator?\n"));
                                          break;
      case ERROR_MOD_NOT_FOUND:           _tprintf(_T("ERROR: The specified module in the config files could not be found!\n"));
                                          break;
      default:                            _tprintf(_T("ERROR: Cannot start the Hosted Web Core. Error number: %X\n"),HRESULT_CODE(hres));
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
    _tprintf(_T("Initiating Web Core shutdown...\n"));

    // Try to shutdown the Hosted Web Core
    HRESULT hres = (*HWC_Shutdown)(g_hwcShutdownMode);

    // Show any errors or warnings
    switch(HRESULT_CODE(hres))
    {
      case S_OK:                          _tprintf(_T("IIS Hosted Web Core shutdown.\n"));
                                          break;
      case ERROR_SERVICE_NOT_ACTIVE:      _tprintf(_T("ERROR: Hosted Web Core was not running!\n"));
                                          break;
      case ERROR_INVALID_SERVICE_CONTROL: _tprintf(_T("ERROR: Hosted Web Core shutdown already in progress!\n"));
                                          break;
      case ERROR_SERVICE_REQUEST_TIMEOUT: _tprintf(_T("ERROR: Hosted Web Core graceful shutdown not possible!\n")
                                                   _T("See IIS Admin for further shutdown actions and options!\n"));
                                          break;
      default:                            _tprintf(_T("ERROR: Cannot shutdown the Hosted Web Core. Error number: %X\n"),HRESULT_CODE(hres));
                                          break;
    }
  }
}

bool 
SetMetaData(XString p_datatype,XString p_value)
{
  bool    result = false;
  wstring type   = StringToWString(p_datatype);
  wstring value  = StringToWString(p_value);

  if(HWC_SetMetadata)
  {
    // Try set the metadata
    HRESULT hres = (*HWC_SetMetadata)(type.c_str(),value.c_str());

    // Show result from setting
    switch(HRESULT_CODE(hres))
    {
      case S_OK:                _tprintf(_T("METADATA [%s:%s] is set.\n"),p_datatype.GetString(),p_value.GetString());
                                result = true;
                                break;
      case ERROR_NOT_SUPPORTED: _tprintf(_T("ERROR: Metadata not supported by Hosted Web Core [%s:%s]\n"),p_datatype.GetString(),p_value.GetString());
                                break;
      case ERROR_INVALID_DATA:  _tprintf(_T("ERROR: Metadata for Hosted Web Core [%s:%s] contains invalid data!\n"),p_datatype.GetString(),p_value.GetString());
                                break;
      default:                  _tprintf(_T("ERROR: Metadata NOT set. Error number: %X\n"),HRESULT_CODE(hres));
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

static TCHAR  g_staticAddress;

XString 
GetExeName()
{
  TCHAR buffer[_MAX_PATH + 1];

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
  XString application(buffer);

  int position = application.ReverseFind('\\');
  if(position == 0)
  {
    return _T("");
  }
  return application.Mid(position + 1);
}

void
ParseCommandLine(int argc,const char* argv[])
{
  // Setting the application pool name
  XString exeName = GetExeName();
  g_poolName = exeName.IsEmpty() ? XString(MARLIN_PRODUCT_NAME) : exeName;

  for(int ind = 1; ind < argc && argv[ind]; ++ind)
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
    _tprintf(_T("USAGE: <application> <ApplictionHost.config> <web.config> <applicationpool>\n"));
  }
}

void 
TrySetMetadata()
{
  TCHAR buffer[80];
  XString variable,value;

  // Trying to get metadata to set
  _tprintf(_T("ENTER METADATA\n"));
  _tprintf(_T("Variable: "));
  if(_getts_s(buffer,80) == NULL)
  {
    _tprintf(_T("Cannot read variable!\n"));
    return;
  }
  variable = buffer;

  // Getting the value
  _tprintf(_T("Value   : "));
  if(_getts_s(buffer,80) == NULL)
  {
    _tprintf(_T("Cannot read value!\n"));
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
  XString line;
  for(int ind = 0; ind < 51; ++ind) line += _T("-");

  _tprintf(_T("+%s+\n"),line.GetString());
  _tprintf(_T("| %-50s|\n"),_T("MENU HOSTED WEBCORE"));
  _tprintf(_T("+%s+\n"),line.GetString());
  _tprintf(_T("| %-50s|\n"),_T(""));
  _tprintf(_T("| %-50s|\n"),_T("A) Server status"));
  _tprintf(_T("| %-50s|\n"),_T("B) Server set metadata"));
  _tprintf(_T("| %-50s|\n"),_T("C) Flush serverlog"));
  _tprintf(_T("| %-50s|\n"),_T("D) Enter Metadata directly"));
  _tprintf(_T("| %-50s|\n"),_T(""));
  _tprintf(_T("| %-50s|\n"),_T("S) Stop"));
  _tprintf(_T("| %-50s|\n"),_T(""));
  _tprintf(_T("+%s+\n"),line.GetString());
  _tprintf(_T("\n"));
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
    _tprintf(_T("Choice made: %c\n"),ch);

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
                  _tprintf(_T("Serverlog flushed!\n"));
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
HWC_main(int argc,const char *argv[])
{
  int retval = 1;

  PrintHeader();
  if(FindIISRunning())
  {
    _tprintf(_T("Cannot use the Hosted Web Core: IIS still running!\n"));
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
  _tprintf(_T("Press enter: "));
  int ch = _getch();
  _putch(ch);
  return retval;
}
