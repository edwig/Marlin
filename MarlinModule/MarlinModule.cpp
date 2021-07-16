/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: MarlinModule.cpp
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
// MarlinIISModule: Connecting Marlin to IIS
//
//////////////////////////////////////////////////////////////////////////

#include "Stdafx.h"
#include "AutoCritical.h"
#include "MarlinModule.h"
#include "WebConfigIIS.h"
#include "RunRedirect.h"
#include "ServiceReporting.h"
#include "IISDebug.h"
#include <io.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define MODULE_XSS  "XSSBlocking"

// GLOBALS Needed for the module
       AppPool       g_IISApplicationPool;   // All applications in the application pool
static WebConfigIIS* g_config  { nullptr };  // The ApplicationHost.config information only!
static wchar_t       g_moduleName[SERVERNAME_BUFFERSIZE + 1] = L"";
static bool          g_debugMode = false;

// Logging macro for this file only
#define DETAILLOG(text)    SvcReportInfoEvent(false,text);
#define ERRORLOG(text)     SvcReportErrorEvent(0,false,__FUNCTION__,text);

//////////////////////////////////////////////////////////////////////////
//
// IIS IS CALLING THIS FIRST WHEN LOADING THE MODULE DLL
//
//////////////////////////////////////////////////////////////////////////

// IIS calls as a C program (no function decoration in object and linker)
extern "C"
{
// RegisterModule must be exported from the DLL in one of three ways
// 1) Through a *.DEF export file with a LIBRARY definition
//    (but the module name can change!)
// 2) Through the extra linker option "/EXPORT:RegisterModule" 
//    (but can be forgotten if you build a different configuration)
// 3) Through the __declspec(dllexport) modifier in the source
//    This is the most stable way of exporting from a DLL
//
__declspec(dllexport) 
HRESULT _stdcall 
RegisterModule(DWORD                        p_version
              ,IHttpModuleRegistrationInfo* p_moduleInfo
              ,IHttpServer*                 p_server)
{
  TRACE("REGISTER MODULE\n");

  // Global name for the WMI Service event registration
  PRODUCT_NAME = "Marlin_for_IIS";
  strcpy_s(g_svcname,SERVICE_NAME_LENGTH,PRODUCT_NAME);

  // Declaration of the start log function
  void ApplicationConfigStart(DWORD p_version);

  // What we want to have from IIS
  DWORD globalEvents = GL_APPLICATION_START |       // Starting application pool
                       GL_APPLICATION_STOP;         // Stopping application pool
  DWORD moduleEvents = RQ_RESOLVE_REQUEST_CACHE |   // Request is found in the cache or from internet
                       RQ_EXECUTE_REQUEST_HANDLER;  // Request is authenticated, ready for processing
  // Add RQ_BEGIN_REQUEST only for debugging purposes!!
  if(g_debugMode)
  {
    // First point to intercept the IIS integrated pipeline
    moduleEvents |= RQ_BEGIN_REQUEST;
  }

  // First moment IIS is calling us.
  // Read the ApplicationConfig file of IIS
  // Start/Restart the logging in the WMI
  ApplicationConfigStart(p_version);

  // Preserving the server in a global pointer
  if(g_iisServer == nullptr)
  {
    g_iisServer = p_server;
  }
  else
  {
    // Already registered
    ERRORLOG("MarlinModule registered more than once in the global IIS module registry!");
    return S_OK;
  }

  // Register name of our module
  wcsncpy_s(g_moduleName,SERVERNAME_BUFFERSIZE,p_moduleInfo->GetName(),SERVERNAME_BUFFERSIZE);

  // Register global notifications to process
  HRESULT hr = p_moduleInfo->SetGlobalNotifications(new MarlinGlobalFactory(),globalEvents);
  if(hr == S_OK)
  {
    hr = p_moduleInfo->SetPriorityForGlobalNotification(globalEvents,PRIORITY_ALIAS_FIRST);
    if(hr != S_OK)
    {
      ERRORLOG("Notification priority setting FAILED. Continue with lower priority!!");
    }
  }
  else
  {
    ERRORLOG("Setting global notifications for a MarlinFactory FAILED");
    return hr;
  }

  // Register module notifications to process
  hr = p_moduleInfo->SetRequestNotifications(new MarlinModuleFactory(),moduleEvents,0);
  if(hr == S_OK )
  {
    hr = p_moduleInfo->SetPriorityForRequestNotification(moduleEvents,PRIORITY_ALIAS_FIRST);
    if(hr != S_OK)
    {
      ERRORLOG("Request priority setting FAILED. Continue with lower priority!!");
    }
  }
  else
  {
    ERRORLOG("Setting request notifications for a MarlinModule FAILED");
    return hr;
  }
  // Registration complete. Possibly NOT with highest priority.
  DETAILLOG("Marlin native module registered for IIS notifications.");
  return S_OK;
}

//////////////////////////////////////////////////////////////////////////
//
// Starting/Stopping the ApplicationHost.Config of IIS
// Tell it to the WMI logging
//
//////////////////////////////////////////////////////////////////////////

void
ApplicationConfigStart(DWORD p_version)
{
  bool read = false;

  // See if we have a debug.txt in the "%windir%\system32\inetsrv\" directory
  CString debugPath;
  if(!debugPath.GetEnvironmentVariable("windir"))
  {
    debugPath = "C:\\Windows";
  }
  debugPath += "\\system32\\inetsrv\\debug.txt";
  if(_access(debugPath.GetString(),0) == 0)
  {
    g_debugMode = true;
  }

  // Depending on the application pool settings, we are sometimes
  // called more than once, so see if the log is not already started
  if(g_config == nullptr)
  {
    g_config = new WebConfigIIS();
    // Only reads system wide "ApplicationHost.Config"
    read = g_config->ReadConfig();
  }

  // Tell that we started the module.
  SvcReportInfoEvent(true
                    ,"Marlin native module called by IIS version %d.%d. ApplicationHost.Config read: %s"
                    ,p_version / 0x10000
                    ,p_version % 0x10000
                    ,read ? "OK" : "ERROR");
}

// Stopping the ApplicationHost.Config
void
ApplicationConfigStop()
{
  // Removing the application config registration
  if(g_config)
  {
    delete g_config;
    g_config = nullptr;
  }
  SvcReportInfoEvent(false,"ApplicationHost.Config unloaded. Marlin native module stopped.");
}

// End of Extern "C" calls
}

// IIS cannot continue with this application pool for some reason
// and must stop the complete pool (w3wp.exe) process
void Unhealthy(CString p_error, HRESULT p_code)
{
  USES_CONVERSION;

  // Print to the MS-Windows WMI
  ERRORLOG(p_error);
  CComBSTR werr = A2W(p_error);
  // Report to IIS to kill the application with **this** reason
  g_iisServer->ReportUnhealthy(werr, p_code);

  // Stopping the application pool
  CString appPoolName = W2A(g_iisServer->GetAppPoolName());
  CString command;
  CString parameters;
  if(!command.GetEnvironmentVariable("WINDIR"))
  {
    command = "C:\\windows";
  }
  command += "\\system32\\inetsrv\\appcmd.exe";
  parameters.Format("stop apppool \"%s\"",appPoolName.GetString());

  // Stopping the application pool will be called like this
  CString logging;
  logging.Format("W3WP.EXE Stopped by: %s %s",command.GetString(),parameters.GetString());
  DETAILLOG(logging);

  // STOP!
  CString result;
  CallProgram_For_String(command, parameters, result);

  // Program is dead after this point. 
  // AFTER CALING THIS FUNCTION, YOU SHOULD END THE MODULE WITH:
  // return GL_NOTIFICATION_CONTINUE;
}

//////////////////////////////////////////////////////////////////////////
//
// GLOBAL LEVEL
//
//////////////////////////////////////////////////////////////////////////

MarlinGlobalFactory::MarlinGlobalFactory()
{
  InitializeCriticalSection(&m_lock);
}

MarlinGlobalFactory::~MarlinGlobalFactory()
{
  DeleteCriticalSection(&m_lock);
}

// Global start of the application.
// Called once for the start of every website
GLOBAL_NOTIFICATION_STATUS
MarlinGlobalFactory::OnGlobalApplicationStart(_In_ IHttpApplicationStartProvider* p_provider)
{
  // One application start at the time
  AutoCritSec lock(&m_lock);
  USES_CONVERSION;

  IHttpApplication* httpapp = p_provider->GetApplication();
  CString appName    = W2A(httpapp->GetApplicationId());
  CString configPath = W2A(httpapp->GetAppConfigPath());
  CString physical   = W2A(httpapp->GetApplicationPhysicalPath());
  CString webroot    = ExtractWebroot(configPath,physical);
  CString appSite    = ExtractAppSite(configPath);

  // IIS Guarantees that every app site is on an unique port
  int applicationPort = g_config->GetSitePort(appSite,INTERNET_DEFAULT_HTTPS_PORT);
  CString application = g_config->GetSiteName(appSite);
  
  if(!ModuleInHandlers(configPath))
  {
    // Website does NOT use the Marlin Module. Continue silently!
    return GL_NOTIFICATION_CONTINUE;
  }

  // Starting the following application
  CString message("Starting IIS Application\n");
  message += "IIS ApplicationID/name: " + appName;
  message += "IIS Configuration path: " + configPath;
  message += "IIS Physical path     : " + physical;
  message += "IIS Extracted webroot : " + webroot;
  message += "IIS Application       : " + application;
  DETAILLOG(message.GetString());

  // Check if already application started for this port number
  AppPool::iterator it = g_IISApplicationPool.find(applicationPort);
  if(it != g_IISApplicationPool.end())
  {
    // Already started this application
    // Keep fingers crossed that another application implements this port!!!
    CString error;
    error.Format("Application [%s] on port [%d] already started. ANOTHER application must implement this port!!"
                 ,application.GetString(),applicationPort);
    Unhealthy(error,ERROR_DUP_NAME);
    return GL_NOTIFICATION_CONTINUE;
  }

  // Create a new pool application
  PoolApp* poolapp = new PoolApp();
  if(poolapp->LoadPoolApp(httpapp,webroot,physical,application))
  {
    // Keep application in our IIS application pool
    g_IISApplicationPool.insert(std::make_pair(applicationPort,poolapp));
  }
  else
  {
    // NOPE: Didn't work. Delete it again
    delete poolapp;
  }

  // Restore the original DLL search order
  SetDllDirectory(nullptr);

  return GL_NOTIFICATION_HANDLED;
};

// Global stop the application.
// Called once for every website that is stopped
GLOBAL_NOTIFICATION_STATUS
MarlinGlobalFactory::OnGlobalApplicationStop(_In_ IHttpApplicationStartProvider* p_provider)
{
  AutoCritSec lock(&m_lock);
  USES_CONVERSION;

  IHttpApplication* httpapp = p_provider->GetApplication();
  CString configPath = W2A(httpapp->GetAppConfigPath());
  CString appSite    = ExtractAppSite(configPath);
 
  // IIS Guarantees that every app site is on an unique port
  int applicationPort = g_config->GetSitePort(appSite,INTERNET_DEFAULT_HTTPS_PORT);
  CString application = g_config->GetSiteName(appSite);

  // Find our application in the application pool
  AppPool::iterator it = g_IISApplicationPool.find(applicationPort);
  if(it == g_IISApplicationPool.end())
  {
    // Not our application to stop. Continue silently!
    return GL_NOTIFICATION_CONTINUE;
  }
  PoolApp* poolapp = it->second;
  ServerApp* app = poolapp->m_application;

  // Tell that we are stopping
  CString stopping("Marlin stopping application: ");
  stopping += application;
  DETAILLOG(stopping);

  if(CountAppPoolApplications(app) == 1)
  {
    // STOP!!
    app->UnloadSites();

    // Let the application stop itself 
    app->ExitInstance();
  }
  // Destroy the application and the IIS pool app
  // And possibly unload the application DLL
  HMODULE hmodule = poolapp->m_module;
  delete poolapp;

  // Remove from our application pool
  g_IISApplicationPool.erase(it);

  if (hmodule != NULL && !StillUsed(hmodule))
  {
    FreeLibrary(hmodule);
    DETAILLOG("Unloaded the application DLL");
  }

  return GL_NOTIFICATION_HANDLED;
};

int
MarlinGlobalFactory::CountAppPoolApplications(ServerApp* p_application)
{
  int count = 0;
  for(auto& app : g_IISApplicationPool)
  {
    if(app.second->m_application == p_application)
    {
      ++count;
    }
  }
  return count;
}

// RE-Construct the webroot from these two settings
// Configuration path : MACHINE/WEBROOT/APPHOST/SECURETEST
// Physical path      : C:\inetpub\wwwroot\SecureTest\
// WEBROOT will be    : C:\inetpub\wwwroot
//
CString
MarlinGlobalFactory::ExtractWebroot(CString p_configPath,CString p_physicalPath)
{
  CString config(p_configPath);
  CString physic(p_physicalPath);
  // Make same case
  config.MakeLower();
  physic.MakeLower();
  // Remove trailing directory separator (if any)
  physic.TrimRight('\\');

  // Find last marker pos
  int lastPosConf = config.ReverseFind('/');
  int lastPosPhys = physic.ReverseFind('\\');

  if(lastPosConf > 0 && lastPosPhys > 0)
  {
    config = config.Mid(lastPosConf + 1);
    physic = physic.Mid(lastPosPhys + 1);
    if(config.Compare(physic) == 0)
    {
      CString webroot = p_physicalPath.Left(lastPosPhys);
      webroot += "\\";
      return webroot;
    }
  }
  // Not found. return original physical path
  // Use this physical path as webroot for this application
  return p_physicalPath;
}

// Extract site from the config combination
// E.g: "MACHINE/WEBROOT/APPHOST/MARLINTEST" -> "MARLINTEST"
CString
MarlinGlobalFactory::ExtractAppSite(CString p_configPath)
{
  int pos = p_configPath.ReverseFind('/');
  if (pos >= 0)
  {
    return p_configPath.Mid(pos + 1);
  }
  return "";
}

// See if the module is still used by one of the websites in the application pool
// (if so, the module should not be freed by "FreeLibrary")
bool    
MarlinGlobalFactory::StillUsed(const HMODULE& p_module)
{
  for(auto& app : g_IISApplicationPool)
  {
    if(app.second->m_module == p_module)
    {
      return true;
    }
  }
  return false;
}

// Find a handler for the MarlinModule. Works even if MarlinModule has been renamed on disk
// This enables the possibility to have multiple MarlinModules (debug / release) in the
// %SYSTEM32%\inetsrv directory and live in IIS at the same time
bool
MarlinGlobalFactory::ModuleInHandlers(const CString& p_configPath)
{
  IAppHostAdminManager* manager = g_iisServer->GetAdminManager();

  // Finding all HTTP Handlers in the configuration
  IAppHostElement* handlersElement = nullptr;
  if(manager->GetAdminSection(CComBSTR(L"system.webServer/handlers"),CComBSTR(p_configPath),&handlersElement) == S_OK)
  {
    IAppHostElementCollection* handlersCollection = nullptr;
    handlersElement->get_Collection(&handlersCollection);
    if(handlersCollection)
    {
      DWORD dwElementCount = 0;
      handlersCollection->get_Count(&dwElementCount);
      for(USHORT dwElement = 0; dwElement < dwElementCount; ++dwElement)
      {
        VARIANT vtItemIndex;
        vtItemIndex.vt = VT_I2;
        vtItemIndex.iVal = dwElement;

        IAppHostElement* childElement = nullptr;
        if(handlersCollection->get_Item(vtItemIndex,&childElement) == S_OK)
        {
          IAppHostProperty* prop = nullptr;
          VARIANT vvar;
          vvar.bstrVal = 0;

          if(childElement->GetPropertyByName(CComBSTR(L"modules"),&prop) == S_OK && prop->get_Value(&vvar) == S_OK && vvar.vt == VT_BSTR)
          {
            if(wcsncmp(vvar.bstrVal,g_moduleName,SERVERNAME_BUFFERSIZE) == 0)
            {
              // At least one of the handlers of the website refers to this module and wants to use it.
              return true;
            }
          }
        }
      }
    }
  }
  return false;
}

// Stopping the global factory
void 
MarlinGlobalFactory::Terminate()
{
  AutoCritSec lock(&m_lock);

  if(!g_IISApplicationPool.empty())
  {
    CString error;
    error.Format("Not all applications where stopped. Still running: %d",(int) g_IISApplicationPool.size());
    ERRORLOG(error);
  }
  DETAILLOG("GlobalFactory terminated");
  // Strange but true: See MSDN documentation. 
  // Last possible moment to delete current object
  delete this;
};

//////////////////////////////////////////////////////////////////////////
//
// MODULE FACTORY: Create a module handler
//
//////////////////////////////////////////////////////////////////////////

MarlinModuleFactory::MarlinModuleFactory()
{
// #ifdef _DEBUG
//   DETAILLOG("Starting new module factory");
// #endif
}

MarlinModuleFactory::~MarlinModuleFactory()
{
// #ifdef _DEBUG
//   DETAILLOG("Stopping module factory");
// #endif
}
  
HRESULT 
MarlinModuleFactory::GetHttpModule(OUT CHttpModule**     p_module
                                  ,IN  IModuleAllocator* p_allocator)
{
  UNREFERENCED_PARAMETER(p_allocator);
  // Create new module
  MarlinModule* requestModule = new MarlinModule();
  //Test for an error
  if(!requestModule)
  {
    DETAILLOG("Failed: APP pool cannot find the requested Marlin module!");
    return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
  }
  // Return a pointer to the module
  *p_module = requestModule;
  return S_OK;
}

// Last module factory stops AFTER the global factory
// Very strange that global factory terminate is not the last
void 
MarlinModuleFactory::Terminate()
{
  // Last function to be called from within IIS
  // So stop the log here
  ApplicationConfigStop();

  // Strange but true: See MSDN documentation!!
  delete this;
};

//////////////////////////////////////////////////////////////////////////
//
// MODULE LEVEL: DONE FOR EACH WEB REQUEST
//
//////////////////////////////////////////////////////////////////////////

MarlinModule::MarlinModule()
{
// #ifdef _DEBUG
//   DETAILLOG("Start Request");
// #endif
}

MarlinModule::~MarlinModule()
{
// #ifdef _DEBUG
//   DETAILLOG("Request ready");
// #endif
}

// On each request: Note the fact that we got it. 
// Regardless whether we will process it.
// Even non-authenticated requests will be processed
//
// BEWARE: ONLY CALLED IF WE CONFIGURE IT 
//         BY DEFINING "debug.txt" IN THE IIS DIRECTORY
//
REQUEST_NOTIFICATION_STATUS 
MarlinModule::OnBeginRequest(IN IHttpContext*       p_context,
                             IN IHttpEventProvider* p_provider)
{
  UNREFERENCED_PARAMETER(p_provider);
  IHttpRequest* request = p_context->GetRequest();

  if(request)
  {
    // Finding the raw HTT_REQUEST from the HTTPServer API 2.0
    const PHTTP_REQUEST rawRequest = request->GetRawHttpRequest();
    // This is the call we are getting
    CString logging("Request for: ");
    logging += rawRequest->pRawUrl;
    DETAILLOG(logging);

    // Here we can debug IIS variables, before other functionality is called!
    IISDebugAllVariables(p_context);
  }
  // Just continue processing
  return RQ_NOTIFICATION_CONTINUE;
}

// Handle a IIS request after authentication
//
REQUEST_NOTIFICATION_STATUS
MarlinModule::OnResolveRequestCache(IN IHttpContext*       p_context,
                                    IN IHttpEventProvider* p_provider)
{
  UNREFERENCED_PARAMETER(p_provider);
  REQUEST_NOTIFICATION_STATUS status = RQ_NOTIFICATION_CONTINUE;
  char  buffer1[SERVERNAME_BUFFERSIZE + 1];
  char  buffer2[SERVERNAME_BUFFERSIZE + 1];
  DWORD size  = SERVERNAME_BUFFERSIZE;
  PCSTR serverName = buffer1;
  PCSTR referer    = buffer2;

  // Getting the HTTPSite through the server port/absPath combination
  int  serverPort = GetServerPort(p_context);

  // Find if it is for one of our applications
  AppPool::iterator it = g_IISApplicationPool.find(serverPort);
  if(it == g_IISApplicationPool.end())
  {
    return status;
  }

  // Find our app
  PoolApp* app = it->second;
  ServerApp* serverapp = app->m_application;

  // Getting the request/response objects
  IHttpRequest*  request  = p_context->GetRequest();
  IHttpResponse* response = p_context->GetResponse();
  if(request == nullptr || response == nullptr)
  {
    ERRORLOG("No request or response objects");
    return status;
  }

  // Detect Cross Site Scripting (XSS) and block if detected
  if(app->m_config.GetSetting(MODULE_XSS).CompareNoCase("true") == 0)
  {
    // Detect Cross Site Scripting (XSS)
    HRESULT hr = p_context->GetServerVariable("SERVER_NAME", &serverName, &size);
    if (hr == S_OK)
    {
      hr = p_context->GetServerVariable("HTTP_REFERER", &referer, &size);
      if (hr == S_OK)
      {
        if (strstr(referer, serverName) == 0)
        {
          CString message("XSS Detected!! Referrer not our server!");
          message += CString("SERVER_NAME : ") + serverName;
          message += CString("HTTP_REFERER: ") + referer;
          ERRORLOG(message);
          response->SetStatus(HTTP_STATUS_BAD_REQUEST,"XSS Detected");
          return RQ_NOTIFICATION_FINISH_REQUEST;
        }
      }
    }
  }
  // Finding the raw HTT_REQUEST from the HTTPServer API 2.0
  const PHTTP_REQUEST rawRequest = request->GetRawHttpRequest();
  if(rawRequest == nullptr)
  {
    ERRORLOG("Abort: IIS did not provide a raw request object!");
    return status;
  }

  // This is the call we are getting
  if(g_debugMode)
  {
    PCSTR url = rawRequest->pRawUrl;
    DETAILLOG(url);
  }
  // Find our marlin representation of the site
  // Use HTTPServer()->FindHTTPSite of the application in the loaded DLL
  HTTPSite* site =  (*app->m_findSite)(serverapp,serverPort,rawRequest->CookedUrl.pAbsPath);

  // ONLY IF WE ARE HANDLING THIS SITE AND THIS MESSAGE!!!
  if(site == nullptr)
  {
    // Not our request: Other app running on this machine!
    // This is why it is wasteful to use IIS for our internet server!
    if(g_debugMode)
    {
      CString message("Rejected HTTP call: ");
      message += rawRequest->pRawUrl;
      ERRORLOG(message);
    }
    // Let someone else handle this call (if any :-( )
    return RQ_NOTIFICATION_CONTINUE;
  }

  // Grab an event stream, if it was present, otherwise continue to the next handler
  // Use the HTTPServer() method GetHTTPStreamFromRequest to see if we must turn it into a stream
  bool stream = (*app->m_getHttpStream)(serverapp,p_context,site,rawRequest);
  if(stream)
  {
    // If we turn into a stream, more notifications are pending
    // This means the context of this request will **NOT** end
    status = RQ_NOTIFICATION_PENDING;
  }
  return status;
}

REQUEST_NOTIFICATION_STATUS 
MarlinModule::OnExecuteRequestHandler(IN IHttpContext*       p_context,
                                      IN IHttpEventProvider* p_provider)
{
  UNREFERENCED_PARAMETER(p_provider);
  REQUEST_NOTIFICATION_STATUS status = RQ_NOTIFICATION_CONTINUE;
  char  buffer1[SERVERNAME_BUFFERSIZE + 1];
  char  buffer2[SERVERNAME_BUFFERSIZE + 1];
  DWORD size = SERVERNAME_BUFFERSIZE;
  PCSTR serverName = buffer1;
  PCSTR referer    = buffer2;

  // Getting the HTTPSite through the server port/absPath combination
  int  serverPort = GetServerPort(p_context);

  // Find if it is for one of our applications
  AppPool::iterator it = g_IISApplicationPool.find(serverPort);
  if (it == g_IISApplicationPool.end())
  {
    return status;
  }

  // Find our app
  PoolApp* app = it->second;
  ServerApp* serverapp = app->m_application;

  // Getting the request/response objects
  IHttpRequest*  request  = p_context->GetRequest();
  IHttpResponse* response = p_context->GetResponse();
  if (request == nullptr || response == nullptr)
  {
    ERRORLOG("No request or response objects");
    return status;
  }

  // Detect Cross Site Scripting (XSS) and block if detected
  if(app->m_config.GetSetting(MODULE_XSS).CompareNoCase("true") == 0)
  {
    HRESULT hr = p_context->GetServerVariable("SERVER_NAME", &serverName, &size);
    if (hr == S_OK)
    {
      hr = p_context->GetServerVariable("HTTP_REFERER", &referer, &size);
      if((hr == S_OK) && (strstr(referer, serverName) == 0))
      {
        CString message("XSS Detected!! Referrer not our server!");
        message += CString("SERVER_NAME : ") + serverName;
        message += CString("HTTP_REFERER: ") + referer;
        ERRORLOG(message);
        response->SetStatus(HTTP_STATUS_BAD_REQUEST,"XSS Detected");
        return RQ_NOTIFICATION_FINISH_REQUEST;
      }
    }
  }

  // Finding the raw HTT_REQUEST from the HTTPServer API 2.0
  const PHTTP_REQUEST rawRequest = request->GetRawHttpRequest();
  if(rawRequest == nullptr)
  {
    ERRORLOG("Abort: IIS did not provide a raw request object!");
    return status;
  }

  // This is the call we are getting
  if(g_debugMode)
  {
    PCSTR url = rawRequest->pRawUrl;
    DETAILLOG(url);
  }

  // Getting the HTTPSite through the server port/absPath combination
  // Use the HTTPServer() method FindHTTPSite to grab the site
  HTTPSite* site = (*app->m_findSite)(serverapp,serverPort,rawRequest->CookedUrl.pAbsPath);

  // ONLY IF WE ARE HANDLING THIS SITE AND THIS MESSAGE!!!
  if (site == nullptr)
  {
    // Not our request: Other app running on this machine!
    // This is why it is wasteful to use IIS for our internet server!
    if(g_debugMode)
    {
      CString message("Rejected HTTP call: ");
      message += rawRequest->pRawUrl;
      ERRORLOG(message);
    }
    // Let someone else handle this call (if any :-( )
    return RQ_NOTIFICATION_CONTINUE;
  }

  // Grab a message from the raw request
  // Uste the HTTPServer() method GetHTTPMessageFromRequest to construct the message
  HTTPMessage* msg = (*app->m_getHttpMessage)(serverapp,p_context,site,rawRequest);
  if(msg)
  {
    // Call HTTPSite->HandleMessage
    bool handled = (*app->m_handleMessage)(serverapp,site,msg);
    if(handled)
    {
       // Ready for IIS!
       p_context->SetRequestHandled();

      // This request is now completely handled
      status = GetCompletionStatus(response);
    }
  }
  else
  {
    ERRORLOG("Cannot handle the request: IIS did not provide enough info for a HTTPMessage.");
    status = RQ_NOTIFICATION_CONTINUE;
  }
  return status;
}

REQUEST_NOTIFICATION_STATUS
MarlinModule::GetCompletionStatus(IHttpResponse* p_response)
{
  USHORT status = 0;
  p_response->GetStatus(&status);
  if(status == HTTP_STATUS_SWITCH_PROTOCOLS)
  {
    return RQ_NOTIFICATION_PENDING;
  }
  if(status == HTTP_STATUS_CONTINUE)
  {
    return RQ_NOTIFICATION_CONTINUE;
  }
  return RQ_NOTIFICATION_FINISH_REQUEST;
}

int 
MarlinModule::GetServerPort(IHttpContext* p_context)
{
  // Getting the server port number
  char  portNumber[20];
  int   serverPort = INTERNET_DEFAULT_HTTP_PORT;
  DWORD size = 20;
  PCSTR port = (PCSTR) &portNumber[0];
  HRESULT hr = p_context->GetServerVariable("SERVER_PORT",&port,&size);
  if(SUCCEEDED(hr))
  {
    serverPort = atoi(port);
  }
  return serverPort;
}

// We keep coming here now-and-then when working with a-typical 
// condition status-es and flushes. So we handle this asynchronous completion
// otherwise we get asserts from the main class in IIS.
REQUEST_NOTIFICATION_STATUS
MarlinModule::OnAsyncCompletion(IN IHttpContext*        pHttpContext,
                                IN DWORD                dwNotification,
                                IN BOOL                 fPostNotification,
                                IN IHttpEventProvider*  pProvider,
                                IN IHttpCompletionInfo* pCompletionInfo)
{
  UNREFERENCED_PARAMETER(pHttpContext);
  UNREFERENCED_PARAMETER(dwNotification);
  UNREFERENCED_PARAMETER(fPostNotification);
  UNREFERENCED_PARAMETER(pProvider);
  UNREFERENCED_PARAMETER(pCompletionInfo);
  return RQ_NOTIFICATION_CONTINUE;
}
