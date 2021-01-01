/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: MarlinModule.h
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
#pragma once
// To prevent bug report from the Windows 8.1 SDK
#pragma warning (disable:4091)
#include <httpserv.h>
#pragma warning (error:4091)

#include "ServerApp.h"
#include <map>

#define SERVERNAME_BUFFERSIZE 256

// Forward reference
class ServerApp;
class LogAnalysis;
class HTTPServerIIS;
class ErrorReport;

// Helper class with a web application
// Also contains the function pointers into our application DLL
class APP
{
public:
  ~APP()
  {
    // Only delete application if last site was deallocated
    // If ServerApp::LoadSite() overloads forget to call base method
    // the reference count can drop below zero
    if(m_application && (*m_sitesInAppPool)(m_application) <= 0)
    {
      delete m_application;
    }
  }
  WebConfigIIS        m_config;
  CString             m_marlinDLL;
  ServerApp*          m_application     { nullptr };
  LogAnalysis*        m_analysisLog     { nullptr };
  HMODULE             m_module          { NULL    };
  // DLL Loaded functions
  CreateServerAppFunc m_createServerApp { nullptr };
  FindHTTPSiteFunc    m_findSite        { nullptr };
  GetHTTPStreamFunc   m_getHttpStream   { nullptr };
  GetHTTPMessageFunc  m_getHttpMessage  { nullptr };
  HandleMessageFunc   m_handleMessage   { nullptr };
  SitesInApplicPool   m_sitesInAppPool  { nullptr };
};

// All applications in the application pool
using AppPool = std::map<int,APP*>;

// Global objects: The one and only IIS Server application pool
extern AppPool g_IISApplicationPool;

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
                                                                 IN IHttpEventProvider*  p_provider) override;
  // First point where we can intercept the IIS integrated pipeline
  // after the authenticate and the authorize request are handled
  virtual REQUEST_NOTIFICATION_STATUS OnResolveRequestCache     (IN IHttpContext*        p_httpContext,
                                                                 IN IHttpEventProvider*  p_provider) override;
  virtual REQUEST_NOTIFICATION_STATUS OnExecuteRequestHandler(   IN IHttpContext*        p_httpContext,
                                                                 IN IHttpEventProvider*  p_provider) override;
  virtual REQUEST_NOTIFICATION_STATUS OnAsyncCompletion         (IN IHttpContext*        pHttpContext,
                                                                 IN DWORD                dwNotification,
                                                                 IN BOOL                 fPostNotification,
                                                                 IN IHttpEventProvider*  pProvider,
                                                                 IN IHttpCompletionInfo* pCompletionInfo) override;
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
  virtual HRESULT GetHttpModule(OUT CHttpModule** p_module,IN IModuleAllocator* p_allocator) override;
  // Stopping the module factory
  virtual void    Terminate() override;
};

// Global calls
class MarlinGlobalFactory : public CGlobalModule
{
public:
  MarlinGlobalFactory();
 ~MarlinGlobalFactory();

  // Registered Start/Stop handlers
  virtual GLOBAL_NOTIFICATION_STATUS OnGlobalApplicationStart(_In_ IHttpApplicationStartProvider* p_provider) override;
  virtual GLOBAL_NOTIFICATION_STATUS OnGlobalApplicationStop (_In_ IHttpApplicationStartProvider* p_provider) override;

  // Extract webroot from config/physical combination
  CString ExtractWebroot(CString p_configPath,CString p_physicalPath);
  // Extract site from the config combination
  CString ExtractAppSite(CString p_configPath);

  // Stopping the global factory
  virtual void Terminate() override;
private:
  bool    ModuleInHandlers(const CString& p_configPath);
  CString ConstructDLLLocation(CString p_rootpath,CString p_dllPath);
  bool    CheckApplicationPresent(CString& p_dllPath,CString& p_dllName);
  bool    AlreadyLoaded(APP* p_app, CString p_path_to_dll);
  bool    StillUsed(const HMODULE& p_module);
  GLOBAL_NOTIFICATION_STATUS Unhealthy(CString p_error,HRESULT p_code);

  CRITICAL_SECTION m_lock;
};
