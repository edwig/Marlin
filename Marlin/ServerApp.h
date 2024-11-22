/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: ServerApp.h
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
#pragma once
#include "HTTPServerIIS.h"
#include "IISSiteConfig.h"
#include "ThreadPool.h"
#include "HTTPLoglevel.h"
#include "LogAnalysis.h"
#include "ErrorReport.h"
#include <set>

// To prevent bug report from the Windows 8.1 SDK
#pragma warning (disable:4091)
#include <httpserv.h>
#pragma warning (error:4091)

using IISModules = std::set<XString>;

class ServerApp;

// Exported functions that can be called from the MarlinModule
typedef ServerApp*    (CALLBACK* CreateServerAppFunc)(IHttpServer*,LPCTSTR,LPCTSTR);
typedef bool          (CALLBACK* InitServerAppFunc)  (ServerApp*,IHttpApplication* p_hhtpapp,XString p_physial);
typedef void          (CALLBACK* ExitServerAppFunc)  (ServerApp*);
typedef HTTPSite*     (CALLBACK* FindHTTPSiteFunc)   (ServerApp*,int port,PCWSTR p_url);
typedef int           (CALLBACK* GetHTTPStreamFunc)  (ServerApp*,IHttpContext*,HTTPSite*,PHTTP_REQUEST);
typedef HTTPMessage*  (CALLBACK* GetHTTPMessageFunc) (ServerApp*,IHttpContext*,HTTPSite*,PHTTP_REQUEST);
typedef bool          (CALLBACK* HandleMessageFunc)  (ServerApp*,HTTPSite* p_site,HTTPMessage*);
typedef int           (CALLBACK* SitesInApplicPool)  (ServerApp*);
typedef bool          (CALLBACK* MinVersionFunc)     (ServerApp*,int version);

extern IHttpServer*  g_iisServer;
extern LogAnalysis*  g_analysisLog;
extern ErrorReport*  g_report;
extern IHttpServer*  g_iisServer;

//////////////////////////////////////////////////////////////////////////
//
// THE SERVER APP
// For IIS Derive a class from this main ServerApp class
//
class ServerApp
{
public:
  ServerApp(IHttpServer* p_iis
           ,LPCTSTR      p_webroot
           ,LPCTSTR      p_appName);
  virtual ~ServerApp();

  // BEWARE: MarlinModule uses VTABLE in this order
  //         ONLY ADD NEW virtual overrides AT THE END!!

  // Starting and stopping the server
  virtual void InitInstance();
  virtual void ExitInstance();
  virtual bool LoadSite  (IISSiteConfig& p_config);
  virtual bool UnloadSite(IISSiteConfig* p_config);
  virtual ErrorReport* GetErrorReport();

  // The performance counter
  virtual void StartCounter();
  virtual void StopCounter();

  // Setting the logging level
  virtual void SetLogLevel(int p_logLevel);

  // Start our sites from the IIS configuration
  virtual void LoadSites(IHttpApplication* p_app,XString p_physicalPath);
  // Stopping all of our sites in the IIS configuration
  virtual void UnloadSites();

  // Server app was correctly started by MarlinIISModule
  virtual bool CorrectlyStarted();

  // Number of IIS sites in this Application Pool
  virtual int  SitesInThePool();

  // Minimal needed MarlinModule version. Checked after DLL loading in MarlinModule
  virtual bool MinMarlinVersion(int p_version,bool p_unicode);

  // Add new MarlinModule used virtual overrides at this end of the table!
  // END OF THE VTABLE

  // Number of logfiles to keep
  void SetKeepLogfiles(int p_keep);

  // GETTERS
  // Never virtual. Derived classes should use these!!
  HTTPServerIIS* GetHTTPServer()  { return m_httpServer;  };
  ThreadPool*    GetThreadPool()  { return m_threadPool;  };
  LogAnalysis*   GetLogfile()     { return m_logfile;     };
  int            GetLogLevel()    { return m_logLevel;    };
  int            GetKeepLogfiles(){ return m_keepLogFiles;};
  IISSiteConfig* GetSiteConfig(int ind);

protected:
  // Start the logging file for this application
  void  StartLogging();

  // Read the site's configuration from the IIS internal structures
  bool  ReadSite    (IAppHostElementCollection* p_sites,XString p_site,int p_num,IISSiteConfig& p_config);
  bool  ReadBinding (IAppHostElementCollection* p_bindings,int p_item,IISBinding& p_binding);
  void  ReadModules (CComBSTR& p_configPath);
  void  ReadHandlers(CComBSTR& p_configPath,IISSiteConfig& p_config);

  // General way to read a property
  XString GetProperty(IAppHostElement* p_elem,XString p_property);

  // DATA
  XString        m_applicationName;             // Name of our application / IIS Site
  XString        m_webroot;                     // WebRoot of our application
  IISModules     m_modules;                     // Global IIS modules for this application
  IISSiteConfigs m_sites;                       // Configures sites, modules and handlers
  WebConfigIIS   m_config;                      // Our web.config object
  int            m_numSites     { 0       };    // Reference counting of sites in the pool
  IHttpServer*   m_iis          { nullptr };    // Main ISS application
  HTTPServerIIS* m_httpServer   { nullptr };    // Our Marlin HTTPServer for IIS
  ThreadPool*    m_threadPool   { nullptr };    // Pointer to our own ThreadPool
  LogAnalysis*   m_logfile      { nullptr };    // Logfile object
  ErrorReport*   m_errorReport  { nullptr };    // Error reporting object
  bool           m_ownReport    { false   };    // We made the error report
  bool           m_versionCheck { false   };    // Did we do our version check
  int            m_logLevel     { HLL_NOLOG };  // Logging level of server and logfile
  int            m_keepLogFiles { LOGWRITE_KEEPFILES };  // Default number of logfiles to keep
};

// Factory for your application to create a new class derived from the ServerApp
// Implement your own server app factory or use this default one
class ServerAppFactory
{
public:
  ServerAppFactory();

  virtual ServerApp* CreateServerApp(IHttpServer* p_iis
                                    ,LPCTSTR      p_webroot
                                    ,LPCTSTR      p_appName);
};

extern ServerAppFactory* appFactory;
