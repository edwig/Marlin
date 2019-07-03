/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: MarlinModule.cpp
//
// Marlin Server: Internet server/client
// 
// Copyright (c) 2015-2018 ir. W.E. Huisman
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
#include "Analysis.h"
#include "HTTPServerIIS.h"
#include "HTTPSite.h"
#include "ThreadPool.h"
#include "AutoCritical.h"
#include "WebConfig.h"
#include "MarlinModule.h"
#include "ServerApp.h"
#include "EnsureFile.h"
#include "AutoCritical.h"
#include "WebConfigIIS.h"
#include "StdException.h"
#ifdef _DEBUG
#include "IISDebug.h"
#endif

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define MODULE_NAME "MarlinModule"
#define MODULE_PATH "DllDirectory"

// GLOBALS Needed for the module
AppPool       g_IISApplicationPool;   // All applications in the application pool
WebConfigIIS* g_config  { nullptr };  // The ApplicationHost.config information only!
LogAnalysis*  g_logfile { nullptr };  // Logfile for the MarlinModule only

// Logging macro for this file only
#define DETAILLOG(text)    if(g_logfile) { g_logfile->AnalysisLog(__FUNCTION__,LogType::LOG_INFO, false,(text)); }
#define ERRORLOG(text)     if(g_logfile) { g_logfile->AnalysisLog(__FUNCTION__,LogType::LOG_ERROR,false,(text)); }

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
  // Install SEH to regular exception translator
  AutoSeTranslator trans(SeTranslator);

  // FIRST MOMENT OF DEBUG
  // WAIT HERE FOR IIS
  // Sleep(20000);

  TRACE("REGISTER MODULE\n");

  // Declaration of the start log function
  void StartLog(DWORD p_version);

  // What we want to have from IIS
  DWORD globalEvents = GL_APPLICATION_START |     // Starting application pool
                       GL_APPLICATION_STOP;       // Stopping application pool
  DWORD moduleEvents = RQ_BEGIN_REQUEST |         // First point to intercept the IIS integrated pipeline
                       RQ_RESOLVE_REQUEST_CACHE |
                       RQ_EXECUTE_REQUEST_HANDLER;  // Request is authenticated, ready for processing

  // Start/Restart the logfile.
  // First moment IIS is calling us. So start logging first!
  StartLog(p_version);

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

  // Register global notifications to process
  HRESULT hr = p_moduleInfo->SetGlobalNotifications(new MarlinGlobalFactory(),globalEvents);
  if(hr == S_OK)
  {
    DETAILLOG("Register GlobalFactory for: start/stop");
    hr = p_moduleInfo->SetPriorityForGlobalNotification(globalEvents,PRIORITY_ALIAS_FIRST);
    if(hr == S_OK)
    {
      DETAILLOG("Setting global notification priority to: FIRST");
    }
    else
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
    DETAILLOG("Register ModuleFactory for requests");
    hr = p_moduleInfo->SetPriorityForRequestNotification(moduleEvents,PRIORITY_ALIAS_FIRST);
    if(hr == S_OK)
    {
      DETAILLOG("Setting requests priority to: FIRST");
    }
    else
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
  return S_OK;
}

//////////////////////////////////////////////////////////////////////////
//
// Starting/Stopping logging for the module application
//
//////////////////////////////////////////////////////////////////////////

void
StartLog(DWORD p_version)
{
  // Depending on the application pool settings, we are sometimes
  // called more than once, so see if the log is not already started
  if(g_logfile == nullptr)
  {
    if(g_config == nullptr)
    {
      g_config = new WebConfigIIS();
      // Only reads system wide "ApplicationHost.Config"
      g_config->ReadConfig();
    }
    // Create the directory for the logfile
    CString logfile = g_config->GetLogfilePath() + "\\Marlin\\Logfile.txt";
    EnsureFile ensure(logfile);
    ensure.CheckCreateDirectory();

    // Create the logfile
    g_logfile = new LogAnalysis(MODULE_NAME);
    g_logfile->SetLogFilename(logfile);
    g_logfile->SetLogRotation(true);
    g_logfile->SetLogLevel(g_config->GetDoLogging() ? HLL_LOGGING : HLL_NOLOG);
  }
  // Tell that we started the logfile
  g_logfile->AnalysisLog(__FUNCTION__,LogType::LOG_INFO,true
                            ,"%s called by IIS version %d.%d"
                            ,MODULE_NAME
                            ,p_version / 0x10000
                            ,p_version % 0x10000);
}

// Stopping the logging
void
StopLog()
{
  if(g_config)
  {
    delete g_config;
    g_config = nullptr;
  }

  if(g_logfile)
  {
    g_logfile->AnalysisLog(__FUNCTION__,LogType::LOG_INFO,true,"%s closed",MODULE_NAME);
    delete g_logfile;
    g_logfile = nullptr;
  }
}

// End of Extern "C" calls
}

//////////////////////////////////////////////////////////////////////////
//
// GLOBAL LEVEL
//
//////////////////////////////////////////////////////////////////////////

MarlinGlobalFactory::MarlinGlobalFactory()
{
  InitializeCriticalSection(&m_lock);
  if(g_logfile)
  {
    DETAILLOG("GlobalFactory started");
  }
}

MarlinGlobalFactory::~MarlinGlobalFactory()
{
  DeleteCriticalSection(&m_lock);
}

GLOBAL_NOTIFICATION_STATUS
MarlinGlobalFactory::OnGlobalApplicationStart(_In_ IHttpApplicationStartProvider* p_provider)
{
  // One application start at the time
  AutoCritSec lock(&m_lock);
  // Install SEH to regular exception translator
  AutoSeTranslator trans(SeTranslator);
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
  
  // Be sure the logfile is there!
  StartLog(10 * 0x10000);

  // Starting the following application
  DETAILLOG("Starting IIS Application");;
  DETAILLOG("IIS ApplicationID/name: " + appName);
  DETAILLOG("IIS Configuration path: " + configPath);
  DETAILLOG("IIS Physical path     : " + physical);
  DETAILLOG("IIS Extracted webroot : " + webroot);
  DETAILLOG("IIS Application       : " + application);

  // Check if already application started for this port number
  AppPool::iterator it = g_IISApplicationPool.find(applicationPort);
  if(it != g_IISApplicationPool.end())
  {
    // Already started this application
    // Keep fingers crossed that another application implements this port!!!
    CString logging;
    logging.Format("Application [%s] on port [%d] already started.",application,applicationPort);
    DETAILLOG(logging);
    DETAILLOG("ANOTHER application must implement this port!!");
    return GL_NOTIFICATION_HANDLED;
  }

  // Create a new pool application
  APP* poolapp = new APP();
  poolapp->m_analysisLog = g_logfile;

  // Read Web config from "physical-application-path" + "web.config"
  CString baseWebConfig = physical + "web.config";
  baseWebConfig.MakeLower();
  poolapp->m_config.ReadConfig(baseWebConfig);

  // Load the **real** application DLL from the settings of web.config
  CString dllLocation = poolapp->m_config.GetSetting(MODULE_NAME);
  if(dllLocation.IsEmpty())
  {
    delete poolapp;
    CString error("MarlinModule could **NOT** locate MarlinModule in web.config: " + baseWebConfig);
    return Unhealthy(error,ERROR_NOT_FOUND);
  }
  dllLocation = ConstructDLLLocation(physical,dllLocation);

  CString dllPath = poolapp->m_config.GetSetting(MODULE_PATH);
  if(!dllPath.IsEmpty())
  {
    dllPath = ConstructDLLLocation(physical,dllPath);
    if(SetDllDirectory(dllPath))
    {
      delete poolapp;
      CString error("MarlinModule could **NOT** set the DLL directory path to: " + dllPath);
      return Unhealthy(error,ERROR_NOT_FOUND);
    }
  }

  // See if we must load the DLL application
  if(AlreadyLoaded(poolapp,dllLocation) == false)
  {
    // Try to load the DLL
    poolapp->m_module = LoadLibrary(dllLocation);
    if (poolapp->m_module)
    {
      poolapp->m_ownModule = true;
      poolapp->m_marlinDLL = dllLocation;
      DETAILLOG("MarlinModule loaded DLL from: " + dllLocation);
    }
    else
    {
      HRESULT code = GetLastError();
      CString error("MarlinModule could **NOT** load DLL from: " + dllLocation);
      SetDllDirectory(NULL);
      delete poolapp;
      return Unhealthy(error,code);
    }

    // Getting the start address of the application factory
    poolapp->m_createServer = (CreateServerAppFunc)GetProcAddress(poolapp->m_module,"CreateServerApp");
    if (poolapp->m_createServer == nullptr)
    {
      HRESULT code = GetLastError();
      CString error("MarlinModule loaded ***INCORRECT*** DLL. Missing 'CreateServerApp'");
      delete poolapp;
      return Unhealthy(error,code);
    }
  }

  // Let the server app factory create a new one for us
  // And store it in our representation of the active application pool
  ServerApp* app = (*poolapp->m_createServer)(g_iisServer             // Microsoft IIS server object
                                             ,webroot.GetString()     // The IIS registered webroot
                                             ,application.GetString() // The application's name
                                             ,g_analysisLog           // LogAnalysis to be reused
                                             ,g_report);              // ErrorReport to be reused
  if(app == nullptr)
  {
    delete poolapp;
    CString error("NO APPLICATION CREATED IN THE APP-FACTORY!");
    return Unhealthy(error,ERROR_SERVICE_NOT_ACTIVE);
  }

  // Keep our application
  poolapp->m_application = app;

  // Keep application in our IIS application pool
  g_IISApplicationPool.insert(std::make_pair(applicationPort,poolapp));

  // Call the initialization
  app->InitInstance();

  // Try loading the sites from IIS in the application
  app->LoadSites(httpapp,physical);

  // Check if everything went well
  if(app->CorrectlyStarted() == false)
  {
    delete poolapp;
    CString error("ERROR STARTING Application: " + application);
    return Unhealthy(error,ERROR_SERVICE_NOT_ACTIVE);
  }

  // First made logfile will be reused later by all others
  if(g_analysisLog == nullptr)
  {
    g_analysisLog = app->GetLogfile();
  }
  // First made error report will be reused later by all others
  if(g_report == nullptr)
  {
    g_report = app->GetErrorReport();
  }

  // Restore the original DLL search order
  if(!dllPath.IsEmpty())
  {
    SetDllDirectory(NULL);
  }

  // Flush the results of starting the server to the logfile
  g_analysisLog->ForceFlush();
  g_logfile->ForceFlush();
  
  // Ready, so stop the timer
  app->StopCounter();

  return GL_NOTIFICATION_HANDLED;
};

GLOBAL_NOTIFICATION_STATUS
MarlinGlobalFactory::OnGlobalApplicationStop(_In_ IHttpApplicationStartProvider* p_provider)
{
  // Install SEH to regular exception translator
  AutoSeTranslator trans(SeTranslator);

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
    CString error("GLOBAL APPLICATION STOP: Not found: Could not stop application: " + application);
    return Unhealthy(error,ERROR_NOT_FOUND);
  }
  APP* poolapp = it->second;
  ServerApp* app = poolapp->m_application;

  // Tell that we are stopping
  CString stopping(MODULE_NAME " stopping application: ");
  stopping += application;
  DETAILLOG(stopping);

  // STOP!!
  // Let the application stop itself 
  app->ExitInstance();

  // Destroy the application and the IIS pool app
  // And possibly unload the application DLL
  delete poolapp;

  // Remove from our application pool
  g_IISApplicationPool.erase(it);

  if(g_IISApplicationPool.empty())
  {
    // So stop the log here
    StopLog();
  }

  return GL_NOTIFICATION_HANDLED;
};

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

bool
MarlinGlobalFactory::AlreadyLoaded(APP* p_app,CString p_path_to_dll)
{
  for(auto& app : g_IISApplicationPool)
  {
    if(p_path_to_dll.Compare(app.second->m_marlinDLL) == 0)
    {
      p_app->m_module       = app.second->m_module;
      p_app->m_createServer = app.second->m_createServer;
      return true;
    }
  }
  return false;
}

GLOBAL_NOTIFICATION_STATUS 
MarlinGlobalFactory::Unhealthy(CString p_error,HRESULT p_code)
{
  USES_CONVERSION;

  // Print to our logfile
  ERRORLOG(p_error);
  CComBSTR werr = A2W(p_error);
  // Report to IIS to kill the application with **this** reason
  g_iisServer->ReportUnhealthy(werr,p_code);
  return GL_NOTIFICATION_HANDLED;
}

// If the given DLL begins with a '@' it is an absolute pathname
// Othwerwise it is relative to the directory the 'web.config' is in
CString 
MarlinGlobalFactory::ConstructDLLLocation(CString p_rootpath,CString p_dllPath)
{
  CString pathname;

  if(p_dllPath.GetAt(0) == '@')
  {
    pathname = p_dllPath.Mid(1);
  }
  else
  {
    pathname = p_rootpath;
    if(pathname.Right(1) != "\\")
    {
      pathname += "\\";
    }
    pathname += p_dllPath;
  }
  return pathname;
}

// Stopping the global factory
void 
MarlinGlobalFactory::Terminate()
{
  AutoCritSec lock(&m_lock);

  // Only log if log still there!
  if(g_logfile)
  {
    if(!g_IISApplicationPool.empty())
    {
      CString error;
      error.Format("Not all applications where stopped. Still running: %d",(int) g_IISApplicationPool.size());
      ERRORLOG(error);
    }
    DETAILLOG("GlobalFactory terminated");
  }
  // Strange but true: See MSDN documentation
  delete this;
};

//////////////////////////////////////////////////////////////////////////
//
// MODULE FACTORY: Create a module handler
//
//////////////////////////////////////////////////////////////////////////

MarlinModuleFactory::MarlinModuleFactory()
{
  DETAILLOG("Starting new module factory");
}

MarlinModuleFactory::~MarlinModuleFactory()
{
  DETAILLOG("Stopping module factory");
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
  StopLog();

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
  DETAILLOG("Start Request");
}

MarlinModule::~MarlinModule()
{
  DETAILLOG("Request ready");
}

// On each request: Note the fact that we got it. 
// Regardless whether we will process it.
// Even non-authenticated requests will be processed
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
// #ifdef _DEBUG
//     IISDebugAllVariables(p_context,g_logfile);
// #endif
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
  // Install SEH to regular exception translator
  AutoSeTranslator trans(SeTranslator);

  UNREFERENCED_PARAMETER(p_provider);
  REQUEST_NOTIFICATION_STATUS status = RQ_NOTIFICATION_CONTINUE;
  char  buffer1[SERVERNAME_BUFFERSIZE + 1];
  char  buffer2[SERVERNAME_BUFFERSIZE + 1];
  DWORD size  = SERVERNAME_BUFFERSIZE;
  PCSTR serverName = buffer1;
  PCSTR referrer   = buffer2;

  // Getting the HTTPSite through the server port/absPath combination
  int  serverPort = GetServerPort(p_context);

  // Find if it is for one of our applications
  AppPool::iterator it = g_IISApplicationPool.find(serverPort);
  if(it == g_IISApplicationPool.end())
  {
    return status;
  }

  // Find our app
  ServerApp* app = it->second->m_application;

  // Starting the performance counter
  app->StartCounter();

  // Getting the request/response objects
  IHttpRequest*  request  = p_context->GetRequest();
  IHttpResponse* response = p_context->GetResponse();
  if(request == nullptr || response == nullptr)
  {
    DETAILLOG("No request or response objects");
    app->StopCounter();
    return status;
  }

  // Detect Cross Site Scripting (XSS)
  HRESULT hr = p_context->GetServerVariable("SERVER_NAME",&serverName, &size);
  if(hr == S_OK)
  {
    hr = p_context->GetServerVariable("HTTP_REFERER", &referrer, &size);
    if(hr == S_OK)
    {
      if(strstr(referrer,serverName) == 0)
      {
        DETAILLOG("XSS Detected!! Deferrer not our server!");
        response->SetStatus(HTTP_STATUS_BAD_REQUEST,"XSS Detected");
      }
    }
  }

  // Finding the raw HTT_REQUEST from the HTTPServer API 2.0
  const PHTTP_REQUEST rawRequest = request->GetRawHttpRequest();
  if(rawRequest == nullptr)
  {
    ERRORLOG("Abort: IIS did not provide a raw request object!");
    app->StopCounter();
    return status;
  }

  // This is the call we are getting
  PCSTR url = rawRequest->pRawUrl;
  DETAILLOG(url);

  // Find our marlin representation of the site
  HTTPSite* site =  app->GetHTTPServer()->FindHTTPSite(serverPort,rawRequest->CookedUrl.pAbsPath);

  // ONLY IF WE ARE HANDLING THIS SITE AND THIS MESSAGE!!!
  if(site == nullptr)
  {
    // Not our request: Other app running on this machine!
    // This is why it is wasteful to use IIS for our internet server!
    CString message("Rejected HTTP call: ");
    message += rawRequest->pRawUrl;
    DETAILLOG(message);
    app->StopCounter();
    // Let someone else handle this call (if any :-( )
    return RQ_NOTIFICATION_CONTINUE;
  }

  // Grab an event stream, if it was present, otherwise continue to the next handler
  EventStream* stream = app->GetHTTPServer()->GetHTTPStreamFromRequest(p_context,site,rawRequest);
  if(stream != nullptr)
  {
    // If we turn into a stream, more notifications are pending
    // This means the context of this request will **NOT** end
    status = RQ_NOTIFICATION_PENDING;
  }

  // Now completely ready. We did everything!
  app->StopCounter();

  return status;
}

REQUEST_NOTIFICATION_STATUS 
MarlinModule::OnExecuteRequestHandler(IN IHttpContext*       p_context,
                                      IN IHttpEventProvider* p_provider)
{
  // Install SEH to regular exception translator
  AutoSeTranslator trans(SeTranslator);

  UNREFERENCED_PARAMETER(p_provider);
  REQUEST_NOTIFICATION_STATUS status = RQ_NOTIFICATION_CONTINUE;
  char  buffer1[SERVERNAME_BUFFERSIZE + 1];
  char  buffer2[SERVERNAME_BUFFERSIZE + 1];
  DWORD size = SERVERNAME_BUFFERSIZE;
  PCSTR serverName = buffer1;
  PCSTR referrer   = buffer2;

  // Getting the HTTPSite through the server port/absPath combination
  int  serverPort = GetServerPort(p_context);

  // Find if it is for one of our applications
  AppPool::iterator it = g_IISApplicationPool.find(serverPort);
  if (it == g_IISApplicationPool.end())
  {
    return status;
  }

  // Find our app
  ServerApp* app = it->second->m_application;

  // Starting the performance counter
  app->StartCounter();

  // Getting the request/response objects
  IHttpRequest*  request  = p_context->GetRequest();
  IHttpResponse* response = p_context->GetResponse();
  if (request == nullptr || response == nullptr)
  {
    DETAILLOG("No request or response objects");
    app->StopCounter();
    return status;
  }

  // Detect Cross Site Scripting (XSS)
  HRESULT hr = p_context->GetServerVariable("SERVER_NAME", &serverName, &size);
  if (hr == S_OK)
  {
    hr = p_context->GetServerVariable("HTTP_REFERER", &referrer, &size);
    if((hr == S_OK) && (strstr(referrer, serverName) == 0))
      {
        DETAILLOG("XSS Detected!! Deferrer not our server!");
        response->SetStatus(HTTP_STATUS_BAD_REQUEST, "XSS Detected");
      }
    }

  // Finding the raw HTT_REQUEST from the HTTPServer API 2.0
  const PHTTP_REQUEST rawRequest = request->GetRawHttpRequest();
  if(rawRequest == nullptr)
  {
    ERRORLOG("Abort: IIS did not provide a raw request object!");
    app->StopCounter();
    return status;
  }

  // This is the call we are getting
  PCSTR url = rawRequest->pRawUrl;
  DETAILLOG(url);

  // Getting the HTTPSite through the server port/absPath combination
  HTTPSite* site = app->GetHTTPServer()->FindHTTPSite(serverPort, rawRequest->CookedUrl.pAbsPath);

  // ONLY IF WE ARE HANDLING THIS SITE AND THIS MESSAGE!!!
  if (site == nullptr)
  {
    // Not our request: Other app running on this machine!
    // This is why it is wasteful to use IIS for our internet server!
    CString message("Rejected HTTP call: ");
    message += rawRequest->pRawUrl;
    DETAILLOG(message);
    app->StopCounter();
    // Let someone else handle this call (if any :-( )
    return RQ_NOTIFICATION_CONTINUE;
  }

  // Grab a message from the raw request
  HTTPMessage* msg = app->GetHTTPServer()->GetHTTPMessageFromRequest(p_context, site, rawRequest);
  if(msg)
  {
    // GO! Let the site handle the message
    msg->AddReference();
    site->HandleHTTPMessage(msg);
  
    // If handled (Marlin has reset the request handle)
    if(msg->GetHasBeenAnswered())
    {
       // Ready for IIS!
       p_context->SetRequestHandled();

      // This request is now completely handled
      status = GetCompletionStatus(response);
    }
    msg->DropReference();
  }
  else
  {
    ERRORLOG("Cannot handle the request: IIS did not provide enough info for a HTTPMessage.");
    status = RQ_NOTIFICATION_CONTINUE;
  }
  // Now completely ready. We did everything!
  app->StopCounter();

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
