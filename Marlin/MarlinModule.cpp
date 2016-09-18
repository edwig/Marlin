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
#include "ThreadPool.h"
#include "ErrorReport.h"
#include "AutoCritical.h"
#include "WebConfig.h"
#include "MarlinModule.h"
#include "ServerApp.h"
#ifdef _DEBUG
#include "IISDebug.h"
#endif

#define MODULE_NAME "MarlinIISModule"

// Globals
IHttpServer*      g_iisServer   = nullptr;
LogAnalysis*      g_analysisLog = nullptr;
HTTPServerIIS*    g_marlin      = nullptr;
// Global threadpool object
ThreadPool        g_pool;
// Global error report object
ErrorReport       g_report;

void
StartLog(DWORD p_version)
{
  if(g_analysisLog == nullptr)
  {
    CString logfile = WebConfig::GetExePath() + "Logfile_Marlin.txt";

    g_analysisLog = new LogAnalysis(MODULE_NAME);
    g_analysisLog->SetDoLogging(true);
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

// Logging macro for this file only
#define DETAILLOG(s1,s2)    g_analysisLog->AnalysisLog(__FUNCTION__,LogType::LOG_INFO, true,"%s %s",(s1),(s2))
#define ERRORLOG(s1,s2)     g_analysisLog->AnalysisLog(__FUNCTION__,LogType::LOG_ERROR,true,"%s %s",(s1),(s2))

//////////////////////////////////////////////////////////////////////////
//
// MODULE LEVEL: DONE FOR EACH WEB REQUEST
//
//////////////////////////////////////////////////////////////////////////

MarlinModule::MarlinModule()
{
}

MarlinModule::~MarlinModule()
{
  DETAILLOG("MarlinModule: ","Request ready");
}

//Implement NotificationMethods
REQUEST_NOTIFICATION_STATUS
MarlinModule::OnBeginRequest(IN IHttpContext*       p_context,
                             IN IHttpEventProvider* p_provider)
{
  USES_CONVERSION;

  UNREFERENCED_PARAMETER(p_provider);
  DWORD size       = 512;
  PCSTR serverName = (PCSTR) p_context->AllocateRequestMemory(size);
  PCSTR referer    = (PCSTR) p_context->AllocateRequestMemory(size);

  DETAILLOG("OnBeginRequest","");
    
  if(serverName == NULL || referer == NULL)
  {
    return RQ_NOTIFICATION_CONTINUE;
  }
  IHttpRequest*  request  = p_context->GetRequest();
  IHttpResponse* response = p_context->GetResponse();
  if(request == nullptr || response == nullptr)
  {
    DETAILLOG("Continue: ", "No request or response objects");
    return RQ_NOTIFICATION_CONTINUE;
  }

  HRESULT hr = p_context->GetServerVariable("SERVER_NAME", &serverName, &size);
  if(hr != S_OK)
  {
    DETAILLOG("Continue: ","No server_name");
    return RQ_NOTIFICATION_CONTINUE;
  }

  hr = p_context->GetServerVariable("HTTP_REFERER", &referer, &size);
  if(hr == S_OK)
  {
    DETAILLOG("SERVER_NAME : ",serverName);
    DETAILLOG("HTTP_REFERER: ",referer);

    if(strstr(referer,serverName) == 0)
    {
      DETAILLOG("Finish: ","No referer! XSS Detected!!");
      response->SetStatus(HTTP_STATUS_BAD_REQUEST,"XSS Detected");
      return RQ_NOTIFICATION_FINISH_REQUEST;
    }
  }
  else
  {
    DETAILLOG("Continue","No referer found");
  }

  // Finding the raw HTT_REQUEST from the HTTPServer API 2.0
  const PHTTP_REQUEST rawRequest = request->GetRawHttpRequest();
  if(request == nullptr)
  {
    ERRORLOG("Abort:","IIS did not provide a raw request object!");
    return RQ_NOTIFICATION_CONTINUE;
  }

  // Getting the HTTPSite through the server port/absPath combination
  int  serverPort = GetServerPort(p_context);
  HTTPSite* site = g_marlin->FindHTTPSite(serverPort,rawRequest->CookedUrl.pAbsPath);

  // ONLY IF WE ARE HANDLING THIS SITE AND THIS MESSAGE!!!
  if(site == nullptr)
  {
    // Not our request: Other app running on this machine!
    // This is why it is wastefull to use IIS for our internetserver!
    DETAILLOG("Rejected",rawRequest->pRawUrl);
    return RQ_NOTIFICATION_CONTINUE;
  }

  // Starting the performance counter
  g_marlin->GetCounter()->Start();

  HTTPMessage* msg = g_marlin->GetHTTPMessageFromRequest(site,rawRequest);
  if(msg)
  {
    // In case of authentication done: get the authentication token
    HANDLE token = NULL;
    IHttpUser* user = p_context->GetUser();
    if(user)
    {
      token = user->GetImpersonationToken();
    }

    // Store the context with the message, so we can handle all derived messages
    msg->SetRequestHandle((HTTP_REQUEST_ID)p_context);
    msg->SetAccessToken(token);

    // Let the site handle the message
    site->HandleHTTPMessage(msg);

    // Ready for IIS!
    p_context->SetRequestHandled();
  }
  else
  {
    ERRORLOG("Cannot handle the request:","IIS did not provide enough info for a HTTPMessage.");
  }

  // Stopping the performance counter
  g_marlin->GetCounter()->Stop();

  // Now completly ready. We did everything!
  return RQ_NOTIFICATION_FINISH_REQUEST;
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

//////////////////////////////////////////////////////////////////////////
//
// MODULE FACTORY
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
    DETAILLOG("Failed:","APP pool cannot find the requested Marlin module!");
    return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
  }
  // DETAILLOG("Created","RequestModule");
  // Return a pointer to the module
  *p_module = requestModule;
  return S_OK;
}

void 
MarlinModuleFactory::Terminate()
{
  // Remove the class from memory
  // DETAILLOG("Terminate:","ModuleFactory");
  delete this;

  // Last function to stop. So stop the log here
  StopLog();
};

//////////////////////////////////////////////////////////////////////////
//
// GLOBAL LEVEL
//
//////////////////////////////////////////////////////////////////////////

GLOBAL_NOTIFICATION_STATUS
MarlinGlobalFactory::OnGlobalApplicationStart(_In_ IHttpApplicationStartProvider* p_provider)
{
  USES_CONVERSION;

  IHttpApplication* app = p_provider->GetApplication();
  CString appName    = W2A(app->GetApplicationId());
  CString configPath = W2A(app->GetAppConfigPath());
  CString physical   = W2A(app->GetApplicationPhysicalPath());

  DETAILLOG("Starting:","MarlinIISModule");
  DETAILLOG("IIS ApplicationID/name:",appName);
  DETAILLOG("IIS Configuration path:",configPath);
  DETAILLOG("IIS Pyhsical path     :",physical);

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
  g_marlin->SetWebroot(physical);
  // Now run the marlin server
  g_marlin->Run();
  // Create a global ServerApp object
  if(g_server)
  {
    g_server->ConnectServerApp(g_iisServer,g_marlin,&g_pool,g_analysisLog,&g_report);
  }
  else
  {
    ERRORLOG("ServerApp","No global pointer to a 'ServerApp' derived object found!");
  }
  // Flush the results of starting the server to the logfile
  g_analysisLog->ForceFlush();
  // Ready, so stop the timer
  g_marlin->GetCounter()->Stop();

  return GL_NOTIFICATION_HANDLED;
};

GLOBAL_NOTIFICATION_STATUS
MarlinGlobalFactory::OnGlobalApplicationStop(_In_ IHttpApplicationStartProvider* p_provider)
{
  UNREFERENCED_PARAMETER(p_provider);
  DETAILLOG("Stopping:","MarlinISSModule");

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
  return GL_NOTIFICATION_HANDLED;
};

void 
MarlinGlobalFactory::Terminate()
{
  DETAILLOG("Terminate:","GlobalFactory");
  delete this;
};

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
  HRESULT hr = S_OK;

  DWORD globalEvents = GL_APPLICATION_START | // Starting application pool
                       GL_APPLICATION_STOP;   // Stopping application pool
  DWORD moduleEvents = RQ_BEGIN_REQUEST;      // Begin request

  // Preserving the server in a global pointer
  if(g_iisServer == nullptr && p_server != nullptr)
  {
    g_iisServer = p_server;
  }

  // Start/Restart the logfile
  StartLog(p_version);

  // Global notifications to process
  hr = p_moduleInfo->SetGlobalNotifications(new MarlinGlobalFactory(),globalEvents);
  if(hr == S_OK)
  {
    DETAILLOG("Register GlobalFactory for: ","start/stop");
    hr = p_moduleInfo->SetPriorityForGlobalNotification(globalEvents,PRIORITY_ALIAS_FIRST);
  }

  // Module notifications to process
  if(hr == S_OK)
  {
    hr = p_moduleInfo->SetRequestNotifications(new MarlinModuleFactory(),moduleEvents,0);
    if(hr == S_OK )
    {
      DETAILLOG("Register ModuleFactory for: ","begin/execute request");
      hr = p_moduleInfo->SetPriorityForRequestNotification(moduleEvents,PRIORITY_ALIAS_FIRST);
    }
  }

  // Any errors at all?
  if(hr != S_OK)
  {
    ERRORLOG("MarlinIISModule:","FAILED");
  }
  return hr;
}
