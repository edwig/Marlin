/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: MarlinModule.cpp
//
// Marlin Server: Internet server/client
// 
// Copyright (c) 2016 ir. W.E. Huisman
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
#include "HTTPThreadPool.h"
#include "ErrorReport.h"
#include "AutoCritical.h"
#include "WebConfig.h"
#include "MarlinModule.h"
#include "ServerApp.h"
#include "EnsureFile.h"
#include "AutoCritical.h"
#include "WebConfigIIS.h"
#ifdef _DEBUG
#include "IISDebug.h"
#endif

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define MODULE_NAME "MarlinIISModule"

// GLOBALS Needed for the module
IHttpServer*      g_iisServer   = nullptr;    // Pointer to the IIS Server
LogAnalysis*      g_analysisLog = nullptr;    // Pointer to our logfile
HTTPServerIIS*    g_marlin      = nullptr;    // Pointer to Marlin Server for IIS
HTTPThreadPool    g_pool;                     // Threadpool for events and tasks
ErrorReport       g_report;                   // Error reporting for Marlin
WebConfigIIS      g_config;                   // Global ApplicationHost.config

// Logging macro for this file only
#define DETAILLOG(text)    g_analysisLog->AnalysisLog(__FUNCTION__,LogType::LOG_INFO, false,(text))
#define ERRORLOG(text)     g_analysisLog->AnalysisLog(__FUNCTION__,LogType::LOG_ERROR,false,(text))

//////////////////////////////////////////////////////////////////////////
//
// IIS IS CALLING THIS FIRST
//
//////////////////////////////////////////////////////////////////////////

// RegisterModule must be exported through the linker option
// /EXPORT:RegisterModule
//
HRESULT _stdcall 
RegisterModule(DWORD                        p_version
              ,IHttpModuleRegistrationInfo* p_moduleInfo
              ,IHttpServer*                 p_server)
{
  void StartLog(DWORD p_version);
  DWORD globalEvents = GL_APPLICATION_START |     // Starting application pool
                       GL_APPLICATION_STOP;       // Stopping application pool
  DWORD moduleEvents = RQ_BEGIN_REQUEST |         // First point to intercept the IIS integrated pipeline
                       RQ_RESOLVE_REQUEST_CACHE;  // Request is authenticated, ready for processing

  // Start/Restart the logfile.
  // First moment IIS is calling us. So start logging first!
  StartLog(p_version);

  // Preserving the server in a global pointer
  if(g_iisServer == nullptr && p_server != nullptr)
  {
    if(g_iisServer)
    {
      ERRORLOG("MarlinIISModule registered more than once in the global IIS module registry!");
    }
    g_iisServer = p_server;
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
  // Registration complete. Possibly not with highest priority.
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
  if(g_analysisLog == nullptr)
  {
    // Read in ApplicationHost.Config
    g_config.ReadConfig();

    CString logfile = g_config.GetLogfilePath() + "\\Marlin\\Logfile.txt";
    EnsureFile ensure(logfile);
    ensure.CheckCreateDirectory();

    g_analysisLog = new LogAnalysis(MODULE_NAME);
    g_analysisLog->SetDoLogging(g_config.GetDoLogging());
    g_analysisLog->SetLogFilename(logfile);
    g_analysisLog->SetLogRotation(true);
  }
  g_analysisLog->AnalysisLog(__FUNCTION__,LogType::LOG_INFO,true
                            ,"%s called by IIS version %d.%d"
                            ,MODULE_NAME
                            ,p_version / 0x10000
                            ,p_version % 0x10000);
}

void
StopLog()
{
  if(g_analysisLog)
  {
    g_analysisLog->AnalysisLog(__FUNCTION__,LogType::LOG_INFO,true,"%s closed",MODULE_NAME);
    delete g_analysisLog;
    g_analysisLog = nullptr;
  }
}

//////////////////////////////////////////////////////////////////////////
//
// GLOBAL LEVEL
//
//////////////////////////////////////////////////////////////////////////

MarlinGlobalFactory::MarlinGlobalFactory()
                    :m_applications(0)
{
  InitializeCriticalSection(&m_lock);
  if(g_analysisLog)
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
  AutoCritSec lock(&m_lock);
  USES_CONVERSION;

  IHttpApplication* app = p_provider->GetApplication();
  CString appName    = W2A(app->GetApplicationId());
  CString configPath = W2A(app->GetAppConfigPath());
  CString physical   = W2A(app->GetApplicationPhysicalPath());
  CString webroot    = ExtractWebroot(configPath,physical);
  CString showroot;

  if(m_applications == 0)
  {
    // First application starts all
    appName    = "IIS ApplicationID/name: " + appName;
    configPath = "IIS Configuration path: " + configPath;
    physical   = "IIS Physical path     : " + physical;
    showroot   = "IIS Extracted webroot : " + webroot;

    DETAILLOG("Starting: MarlinIISModule");
    DETAILLOG(appName);
    DETAILLOG(configPath);
    DETAILLOG(physical);
    DETAILLOG(showroot);

    // Create a marlin HTTPServer object for IIS
    g_marlin = new HTTPServerIIS(appName);
    // Connect the logging file
    g_marlin->SetLogging(g_analysisLog);
    g_marlin->SetDetailedLogging(true);
    // Provide a minimal threadpool for general tasks
    g_marlin->SetThreadPool(&g_pool);
    // Provide an error reporting object
    g_marlin->SetErrorReport(&g_report);
    // Setting the base webroot
    g_marlin->SetWebroot(webroot);
    // Now run the marlin server
    g_marlin->Run();
    // Create a global ServerApp object
    if(g_server)
    {
      // Connect all these to the global object
      g_server->ConnectServerApp(g_iisServer,g_marlin,&g_pool,g_analysisLog,&g_report);
      // And then INIT the server application
      g_server->InitInstance();
    }
    else
    {
      ERRORLOG("No global pointer to a 'ServerApp' derived object found! Implement a ServerApp!");
    }
  }
  else
  {
    // Adding to existing application
    configPath = "IIS Configuration path: " + configPath;
    DETAILLOG("Adding to: MarlinIISModule");
    DETAILLOG(configPath);

    if(g_marlin->GetWebroot().CompareNoCase(webroot))
    {
      ERRORLOG("SITE CONFIGURED INCORRECTLY: " + configPath);
      ERRORLOG("Configured site with different webroot. Proceed with fingers crossed!!");
      ERRORLOG("Original webroot: " + g_marlin->GetWebroot());
      ERRORLOG("New Site webroot: " + webroot);
    }
  }
  // Increment the number of applications running
  ++m_applications;
  // Flush the results of starting the server to the logfile
  g_analysisLog->ForceFlush();
  // Ready, so stop the timer
  g_marlin->GetCounter()->Stop();

  return GL_NOTIFICATION_HANDLED;
};

GLOBAL_NOTIFICATION_STATUS
MarlinGlobalFactory::OnGlobalApplicationStop(_In_ IHttpApplicationStartProvider* p_provider)
{
  AutoCritSec lock(&m_lock);
  USES_CONVERSION;

  IHttpApplication* app = p_provider->GetApplication();
  CString config = W2A(app->GetAppConfigPath());
  CString stopping("MarlinISSModule stopping application: ");
  stopping += config;
  DETAILLOG(stopping);

  // Decrement our application counter
  --m_applications;

  // Only stopping if last application is stopping
  if(m_applications <= 0)
  {
    DETAILLOG("Stopping last application. Stopping Marlin ServerApp.");
    // Stopping the ServerApp
    if(g_server)
    {
      g_server->ExitInstance();
    }

    // Stopping the marlin server
    if(g_marlin)
    {
      g_marlin->StopServer();
      delete g_marlin;
      g_marlin = nullptr;
    }
  }
  return GL_NOTIFICATION_HANDLED;
};

// RE-Construct the webroot from thse two settings
// Configuration path : MACHINE/WEBROOT/APPHOST/SECURETEST
// Physical path      : C:\inetpub\wwwroot\SecureTest\
// WEBROOT wil be     : C:\inetpub\wwwroot
//
CString
MarlinGlobalFactory::ExtractWebroot(CString p_configPath,CString p_physicalPath)
{
  CString config(p_configPath);
  CString physic(p_physicalPath);
  // Make same case
  config.MakeLower();
  physic.MakeLower();
  // Remove trailing directory seperator (if any)
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
  // Not found. return original
  ERRORLOG("CANNOT FIND THE SERVER WEBROOT. PROCEED WITH FINGERS CROSSED!!");
  return p_physicalPath;
}

void 
MarlinGlobalFactory::Terminate()
{
  AutoCritSec lock(&m_lock);

  // Only log if log still there!
  if(g_analysisLog)
  {
    if(m_applications)
    {
      CString error;
      error.Format("Not all applications where stopped. Stil running: %d",m_applications);
      ERRORLOG(error);
    }
    DETAILLOG("GlobalFactory terminated");
  }
  delete this;
};

//////////////////////////////////////////////////////////////////////////
//
// MODULE FACTORY: Create a module handler
//
//////////////////////////////////////////////////////////////////////////

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

void 
MarlinModuleFactory::Terminate()
{
  // Last function to be called from within IIS
  // So stop the log here
  StopLog();

  // Strange but true: See MSDN documentation
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
//     IISDebugAllVariables(p_context,g_analysisLog);
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
  UNREFERENCED_PARAMETER(p_provider);
  REQUEST_NOTIFICATION_STATUS status = RQ_NOTIFICATION_CONTINUE;
  char  buffer1[SERVERNAME_BUFFERSIZE + 1];
  char  buffer2[SERVERNAME_BUFFERSIZE + 1];
  DWORD size  = SERVERNAME_BUFFERSIZE;
  PCSTR serverName = buffer1;
  PCSTR referrer   = buffer2;

  // Starting the performance counter
  g_marlin->GetCounter()->Start();

  // Getting the request/response objects
  IHttpRequest*  request  = p_context->GetRequest();
  IHttpResponse* response = p_context->GetResponse();
  if(request == nullptr || response == nullptr)
  {
    DETAILLOG("No request or response objects");
    g_marlin->GetCounter()->Stop();
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
  if(request == nullptr)
  {
    ERRORLOG("Abort: IIS did not provide a raw request object!");
    g_marlin->GetCounter()->Stop();
    return status;
  }

  // This is the call we are getting
  PCSTR url = rawRequest->pRawUrl;
  DETAILLOG(url);

  // Getting the HTTPSite through the server port/absPath combination
  int  serverPort = GetServerPort(p_context);
  HTTPSite* site = g_marlin->FindHTTPSite(serverPort,rawRequest->CookedUrl.pAbsPath);

  // ONLY IF WE ARE HANDLING THIS SITE AND THIS MESSAGE!!!
  if(site == nullptr)
  {
    // Not our request: Other app running on this machine!
    // This is why it is wastefull to use IIS for our internetserver!
    CString message("Rejected HTTP call: ");
    message += rawRequest->pRawUrl;
    DETAILLOG(message);
    g_marlin->GetCounter()->Stop();
    // Let someone else handle this call (if any :-( )
    return RQ_NOTIFICATION_CONTINUE;
  }

  EventStream* stream = nullptr;
  HTTPMessage* msg = g_marlin->GetHTTPMessageFromRequest(p_context,site,rawRequest,stream);
  if(msg)
  {
    // In case of authentication done: get the authentication token
    HANDLE token = NULL;
    IHttpUser* user = p_context->GetUser();
    if(user)
    {
      // Make duplicate of the token, otherwise IIS will crash!
      if(DuplicateTokenEx(user->GetImpersonationToken()
                         ,TOKEN_DUPLICATE | TOKEN_IMPERSONATE | TOKEN_ALL_ACCESS | TOKEN_READ | TOKEN_WRITE
                         ,NULL
                         ,SecurityImpersonation
                         ,TokenImpersonation
                         ,&token) == FALSE)
      {
        token = NULL;
      }
    }

    // Store the context with the message, so we can handle all derived messages
    msg->SetRequestHandle((HTTP_REQUEST_ID)p_context);
    msg->SetAccessToken(token);

    // GO! Let the site handle the message
    site->HandleHTTPMessage(msg);

    // Ready for IIS!
    p_context->SetRequestHandled();

    // Stopping the performance counter
    g_marlin->GetCounter()->Stop();

    // This request is now compeletly handled
    status = RQ_NOTIFICATION_FINISH_REQUEST;
  }
  else if(stream)
  {
    // If we turn into a stream, more notifications are pending
    status = RQ_NOTIFICATION_PENDING;
  }
  else
  {
    ERRORLOG("Cannot handle the request: IIS did not provide enough info for a HTTPMessage or an event stream.");
    status = RQ_NOTIFICATION_CONTINUE;
  }
  // Now completly ready. We did everything!
  g_marlin->GetCounter()->Stop();
  return status;
}

int 
MarlinModule::GetServerPort(IHttpContext* p_context)
{
  // Getting the server port
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

