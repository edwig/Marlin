/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: MarlinModule.h
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
#pragma once
#pragma warning (disable:4091)
#include <httpserv.h>

// Forward reference
class ServerApp;
class LogAnalysis;
class HTTPServerIIS;

// Global objects: The one and only IIS Server
extern IHttpServer*   g_iisServer;
extern LogAnalysis*   g_analysisLog;
extern HTTPServerIIS* g_marlin;

// The application must supply a ServerApp derived object pointer
extern ServerApp*   g_serverApp;

// Create the module class
// Hooking into the 'integrated pipeline' of IIS
class MarlinModule: public CHttpModule
{
public:
  MarlinModule();
  virtual ~MarlinModule();

  // Implement NotificationMethods
  virtual REQUEST_NOTIFICATION_STATUS OnBeginRequest            (IN IHttpContext*        p_httpContext,
                                                                 IN IHttpEventProvider*  p_provider);
//virtual REQUEST_NOTIFICATION_STATUS OnMapRequestHandler       (IN IHttpContext*        p_httpContext,
//                                                               IN IMapHandlerProvider* p_provider);
//virtual REQUEST_NOTIFICATION_STATUS OnPreExecuteRequestHandler(IN IHttpContext*        p_httpContext,
//                                                               IN IHttpEventProvider* p_provider);
private:
  int GetServerPort(IHttpContext* p_context);
};

// Create the modules' class factory
class MarlinModuleFactory : public IHttpModuleFactory
{
public:
  // Creating a HTTPmodule
  virtual HRESULT GetHttpModule(OUT CHttpModule** p_module,IN IModuleAllocator* p_allocator);
  // Stopping the module factory
  virtual void    Terminate();
};

// Global calls
class MarlinGlobalFactory : public CGlobalModule
{
public:
  virtual GLOBAL_NOTIFICATION_STATUS OnGlobalApplicationStart(_In_ IHttpApplicationStartProvider* p_provider);
  virtual GLOBAL_NOTIFICATION_STATUS OnGlobalApplicationStop (_In_ IHttpApplicationStartProvider* p_provider);
  // Stopping the global factory
  virtual void Terminate();
};
