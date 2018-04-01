/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: MarlinModule.h
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
#pragma once
// To prevent bug report from the Windows 8.1 SDK
#pragma warning (disable:4091)
#include <httpserv.h>
#pragma warning (error:4091)
#include <map>

#define SERVERNAME_BUFFERSIZE 256

// Forward reference
class ServerApp;
class LogAnalysis;
class HTTPServerIIS;
class WebConfigIIS;
class ErrorReport;

// All applications in the application pool
using AppPool = std::map<int, ServerApp*>;

// Global objects: The one and only IIS Server application pool
extern AppPool        g_IISApplicationPool;
extern LogAnalysis*   g_analysisLog;
extern IHttpServer*   g_iisServer;
extern CString        g_poolName;
extern WebConfigIIS   g_config;
extern ErrorReport*   g_report;

// Get the base name of the module DLL
CString GetDLLBaseName();
CString GetDLLName();

// Create the module class
// Hooking into the 'integrated pipeline' of IIS
class MarlinModule: public CHttpModule
{
public:
  MarlinModule();
  virtual ~MarlinModule();
  // Implement NotificationMethods

  // BeginRequest is needed for the client certificate
  virtual REQUEST_NOTIFICATION_STATUS OnBeginRequest            (IN IHttpContext*        p_httpContext,
                                                                 IN IHttpEventProvider*  p_provider);
  // First point where we can intercept the IIS integrated pipeline
  // after the authenticate and the authorize request are handled
  virtual REQUEST_NOTIFICATION_STATUS OnResolveRequestCache     (IN IHttpContext*        p_httpContext,
                                                                 IN IHttpEventProvider*  p_provider);
  virtual REQUEST_NOTIFICATION_STATUS OnExecuteRequestHandler(   IN IHttpContext*        p_httpContext,
                                                                 IN IHttpEventProvider*  p_provider);
  virtual REQUEST_NOTIFICATION_STATUS OnAsyncCompletion         (IN IHttpContext*        pHttpContext,
                                                                 IN DWORD                dwNotification,
                                                                 IN BOOL                 fPostNotification,
                                                                 IN IHttpEventProvider*  pProvider,
                                                                 IN IHttpCompletionInfo* pCompletionInfo);
private:
  int GetServerPort(IHttpContext* p_context);
  REQUEST_NOTIFICATION_STATUS GetCompletionStatus(IHttpResponse* p_response);
};

// Create the modules' class factory
class MarlinModuleFactory : public IHttpModuleFactory
{
public:
  MarlinModuleFactory();
 ~MarlinModuleFactory();
  // Creating a HTTPmodule
  virtual HRESULT GetHttpModule(OUT CHttpModule** p_module,IN IModuleAllocator* p_allocator);
  // Stopping the module factory
  virtual void    Terminate();
};

// Global calls
class MarlinGlobalFactory : public CGlobalModule
{
public:
  MarlinGlobalFactory();
 ~MarlinGlobalFactory();

  // Registered Start/Stop handlers
  virtual GLOBAL_NOTIFICATION_STATUS OnGlobalApplicationStart(_In_ IHttpApplicationStartProvider* p_provider);
  virtual GLOBAL_NOTIFICATION_STATUS OnGlobalApplicationStop (_In_ IHttpApplicationStartProvider* p_provider);

  // Extract webroot from config/physical combination
  CString ExtractWebroot(CString p_configPath,CString p_physicalPath);
  // Extract site from the config combination
  CString ExtractAppSite(CString p_configPath);

  // Stopping the global factory
  virtual void Terminate();
private:
  CRITICAL_SECTION m_lock;
};
