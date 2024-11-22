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
#include "stdafx.h"
#include "ServerApp.h"
#include "WebConfigIIS.h"
#include "HTTPSite.h"
#include "Version.h"
#include "ServiceReporting.h"
#include <WinFile.h>
#include <assert.h>
#include <string>
#include <set>

#ifdef _AFX
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#endif

#define DETAILLOGV(text,...)    m_httpServer->DetailLogV(_T(__FUNCTION__),LogType::LOG_INFO,text,__VA_ARGS__)
#define WARNINGLOG(text,...)    m_httpServer->DetailLogV(_T(__FUNCTION__),LogType::LOG_WARN,text,__VA_ARGS__)
#define ERRORLOG(code,text)     m_httpServer->ErrorLog  (_T(__FUNCTION__),code,text)

LogAnalysis*  g_analysisLog = nullptr;
ErrorReport*  g_report      = nullptr;

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
ServerApp* _stdcall CreateServerApp(IHttpServer* p_server
                                   ,LPCTSTR      p_webroot
                                   ,LPCTSTR     p_appName)
{
  return appFactory->CreateServerApp(p_server,p_webroot,p_appName);
}

__declspec(dllexport)
bool _stdcall InitServerApp(ServerApp* p_application,IHttpApplication* p_httpapp,XString p_physical)
{
  if(p_application)
  {
    _set_se_translator(SeTranslator);
    try
    {
      // Call the initialization
      p_application->InitInstance();

      // Try loading the sites from IIS in the application
      p_application->LoadSites(p_httpapp,p_physical);

      // Ready, so stop the timer
      p_application->StopCounter();

      // Check if everything went well
      return p_application->CorrectlyStarted();
    }
    catch(StdException& ex)
    {
      SvcReportErrorEvent(0,false,_T(__FUNCTION__),_T("ERROR while initializing the server application: ") + ex.GetErrorMessage());
    }
  }
  return false;
}

__declspec(dllexport)
void _stdcall ExitServerApp(ServerApp* p_application)
{
  if(p_application)
  {
    _set_se_translator(SeTranslator);
    try
    {
      // STOP!!
      p_application->UnloadSites();

      // Let the application stop itself 
      p_application->ExitInstance();
    }
    catch(StdException& ex)
    {
      SvcReportErrorEvent(0,false,_T(__FUNCTION__),_T("ERROR while stopping the server application: ") + ex.GetErrorMessage());
    }
  }
}

__declspec(dllexport)
HTTPSite* _stdcall FindHTTPSite(ServerApp* p_application,int p_port, PCWSTR p_url)
{
  HTTPSite* site = nullptr;
  if(p_application)
  {
    _set_se_translator(SeTranslator);
    try
    {
      p_application->StartCounter();
      site = p_application->GetHTTPServer()->FindHTTPSite(p_port,p_url);
      p_application->StopCounter();
    }
    catch(StdException& ex)
    {
      SvcReportErrorEvent(0,false,_T(__FUNCTION__),_T("ERROR while finding HTTP Site: ") + ex.GetErrorMessage());
      site = nullptr;
    }
  }
  return site;
}

__declspec(dllexport)
int _stdcall GetStreamFromRequest(ServerApp* p_application,IHttpContext* p_context,HTTPSite* p_site,PHTTP_REQUEST p_request)
{
  int gotstream{0};

  if(p_application)
  {
    _set_se_translator(SeTranslator);
    try
    {
      p_application->StartCounter();
      gotstream = p_application->GetHTTPServer()->GetHTTPStreamFromRequest(p_context,p_site,p_request);
      p_application->StopCounter();
    }
    catch(StdException& ex)
    {
      SvcReportErrorEvent(0,false,_T(__FUNCTION__),_T("ERROR while getting new event stream: ") + ex.GetErrorMessage());
      gotstream = 0;
    }
  }
  return gotstream;
}

__declspec(dllexport)
HTTPMessage* _stdcall GetHTTPMessageFromRequest(ServerApp*    p_application
                                               ,IHttpContext* p_context
                                               ,HTTPSite*     p_site
                                               ,PHTTP_REQUEST p_request)
{
  HTTPMessage* msg = nullptr;
  if(p_application)
  {
    _set_se_translator(SeTranslator);
    try
    {
      p_application->StartCounter();
      HTTPServerIIS* server = p_application->GetHTTPServer();
      if(server)
      {
        msg = server->GetHTTPMessageFromRequest(p_context,p_site,p_request);
      }
      p_application->StopCounter();
    }
    catch(StdException& ex)
    {
      SvcReportErrorEvent(0,false,_T(__FUNCTION__),_T("ERROR while getting a new HTTPMessage: ") + ex.GetErrorMessage());
      msg = nullptr;
    }
  }
  return msg;
}

__declspec(dllexport)
bool _stdcall HandleHTTPMessage(ServerApp* p_application,HTTPSite* p_site,HTTPMessage* p_message)
{
  bool handled = false;

  if(p_application && p_message)
  {
    // Install SEH to regular exception translator
    AutoSeTranslator trans(SeTranslator);

    try
    {
      p_application->StartCounter();

      // GO! Let the site handle the message
      p_message->AddReference();
      p_site->HandleHTTPMessage(p_message);

      // If handled (Marlin has reset the request handle)
      if (p_message->GetHasBeenAnswered())
      {
        handled = true;
      }
      p_message->DropReference();
      p_application->StopCounter();
    }
    catch(StdException& ex)
    {
      SvcReportErrorEvent(0,false,_T(__FUNCTION__),_T("ERROR while handeling a HTTPMessage: ") + ex.GetErrorMessage());
      handled = false;
    }
  }
  return handled;
}

__declspec(dllexport)
int __stdcall SitesInApplicationPool(ServerApp* p_application)
{
  return p_application->SitesInThePool();
}

__declspec(dllexport)
bool __stdcall MinMarlinVersion(ServerApp* p_application,int p_version,bool p_unicode)
{
  return p_application->MinMarlinVersion(p_version,p_unicode);
}

}

//////////////////////////////////////////////////////////////////////////
//
// THE GENERAL SERVER APP ROOT CLASS
//
//////////////////////////////////////////////////////////////////////////

// XTOR
ServerApp::ServerApp(IHttpServer* p_iis
                    ,LPCTSTR      p_webroot
                    ,LPCTSTR      p_appName)
          :m_iis(p_iis)
          ,m_webroot(p_webroot)
          ,m_applicationName(p_appName)
{
  // Keep global pointer to the server
  g_iisServer = p_iis;
}

// DTOR
ServerApp::~ServerApp()
{
  // Free all site configuration info
  for(auto& site : m_sites)
  {
    delete site;
  }
}

// Init our server app.
// Overrides should call this method first!
void 
ServerApp::InitInstance()
{
  // Create a marlin HTTPServer object for IIS
  m_httpServer = new HTTPServerIIS(m_applicationName);
  m_httpServer->SetWebroot(m_webroot);

  // Reading our IIS web.config and ApplicationHost.config info
  m_config.ReadConfig(m_applicationName);
  m_httpServer->SetWebConfigIIS(&m_config);

  // Start our own logging file
  StartLogging();
  m_httpServer->SetLogging(m_logfile);
  m_httpServer->SetLogLevel(m_logfile->GetLogLevel());

  // Create our error report
  if(g_report == nullptr)
  {
    g_report      = new ErrorReport();
    m_errorReport = g_report;
    m_ownReport   = true;
  }
  m_httpServer->SetErrorReport(m_errorReport);

  // Now run the marlin server
  m_httpServer->Run();

  // Grab the ThreadPool
  m_threadPool = m_httpServer->GetThreadPool();
}

// Exit our server app
// Overrides should call this method last!
void 
ServerApp::ExitInstance()
{
  // Ready with the HTTPServerIIS
  if(m_httpServer)
  {
    // Stop the server and wait for exit processing
    m_httpServer->StopServer();

    delete m_httpServer;
    m_httpServer = nullptr;
  }

  // Stopping our logfile
  if(m_logfile)
  {
    bool writer = m_logfile->GetBackgroundWriter();
    m_logfile->AnalysisLog(_T(__FUNCTION__),LogType::LOG_INFO,true,_T("%s closed"),m_applicationName.GetString());
    m_logfile->Reset();

    // The server is about to stop. So we wait for the background writer to stop also
    // otherwise the last thread might be prematurely stopped.
    if(writer)
    {
      for(int ind = 0;ind < 100; ++ind)
      {
        if(m_logfile->GetBackgroundWriter() == false)
        {
          break;
        }
        Sleep(100);
      }
    }
    LogAnalysis::DeleteLogfile(m_logfile);
    m_logfile = nullptr;
  }

  // Destroy the general error report
  if(m_errorReport && m_ownReport)
  {
    delete m_errorReport;
  }
  m_errorReport = nullptr;

  // Other objects cannot access these any more
  g_analysisLog = nullptr;
  g_report      = nullptr;
}

// Number of logfiles to keep
void 
ServerApp::SetKeepLogfiles(int p_keep)
{
  m_keepLogFiles = p_keep;

  if(m_keepLogFiles < LOGWRITE_KEEPLOG_MIN) m_keepLogFiles = LOGWRITE_KEEPLOG_MIN;
  if(m_keepLogFiles > LOGWRITE_KEEPLOG_MAX) m_keepLogFiles = LOGWRITE_KEEPLOG_MAX;
}

// The performance counter
void 
ServerApp::StartCounter()
{
  if(m_httpServer)
  {
    m_httpServer->GetCounter()->Start();
  }
}

void 
ServerApp::StopCounter()
{
  if(m_httpServer)
  {
    m_httpServer->GetCounter()->Stop();
  }
}

// Start the logging file for this application
void  
ServerApp::StartLogging()
{
  if(m_logfile == nullptr)
  {
    // Getting the base of the logfile path
    XString logpath = m_config.GetLogfilePath();
    if(logpath.IsEmpty())
    {
      logpath = m_webroot;
    }

    // Create the directory for the logfile
    XString logfile = logpath + _T("\\") + m_applicationName + _T("\\Logfile.txt");
    WinFile ensure(logfile);
    ensure.CreateDirectory();

    // Create the logfile
    m_logfile = LogAnalysis::CreateLogfile(m_applicationName);
    m_logfile->SetLogFilename(logfile);
    m_logfile->SetLogRotation(true);
    m_logfile->SetLogLevel(m_config.GetDoLogging() ? HLL_LOGGING : HLL_NOLOG);
    m_logfile->SetKeepfiles(m_keepLogFiles);

    // Record for test classes
    g_analysisLog = m_logfile;
  }

  // Tell that we started the logfile
  m_logfile->AnalysisLog(_T(__FUNCTION__),LogType::LOG_INFO,true
                        ,_T("Started the application: %s"),m_applicationName.GetString());
}

// Server app was correctly started by MarlinIISModule
bool
ServerApp::CorrectlyStarted()
{
  // MINIMUM REQUIREMENT:
  // If a derived class has been statically declared and version-checked
  // and a IHttpServer and a HTTPServerIIS has been found
  // and a ThreadPool is initialized, we are good to go
  if(m_iis && m_httpServer && m_threadPool && m_logfile && m_versionCheck)
  {
    return true;
  }

  // Log the errors
  // See to it that we did our version check
  if(!m_iis)
  {
    ERRORLOG(ERROR_NOT_FOUND,_T("No connected IIS server found!"));
  }
  else if(!m_threadPool)
  {
    ERRORLOG(ERROR_NOT_FOUND,_T("No connected threadpool found!"));
  }
  else if(!m_logfile)
  {
    ERRORLOG(ERROR_NOT_FOUND,_T("No connected logfile found!"));
  }
  else if(!m_versionCheck)
  {
    ERRORLOG(ERROR_VALIDATE_CONTINUE,_T("MarlinModule version check not done! Did you use a MarlinModule prior to version 7.0.0 ?"));
    return false;
  }
  else
  {
    ERRORLOG(ERROR_NOT_FOUND,_T("No connected MarlinIIS server found!"));
  }
  return false;
}

// Number of IIS sites in this Application Pool
int
ServerApp::SitesInThePool()
{
  return m_numSites;
}

bool
ServerApp::MinMarlinVersion(int p_version,bool p_unicode)
{
  int minVersion =  MARLIN_VERSION_MAJOR      * 10000 +   // Major version main
                    MARLIN_VERSION_MINOR      *   100;
  int maxVersion = (MARLIN_VERSION_MAJOR + 1) * 10000;    // Major version main

  if(p_version < minVersion || maxVersion <= p_version)
  {
    SvcReportErrorEvent(0,true,_T(__FUNCTION__)
                       ,_T("MarlinModule version is out of range: %d.%d.%d\n")
                       ,_T("This application was compiled for: %d.%d.%d")
                       ,p_version / 10000,(p_version % 10000)/100,p_version % 100
                       ,MARLIN_VERSION_MAJOR,MARLIN_VERSION_MINOR,MARLIN_VERSION_SP);
    return false;
  }
#ifdef _UNICODE
  if(!p_unicode)
  {
    SvcReportErrorEvent(0,false,_T(__FUNCTION__),_T("An ANSI mode MarlinModule is calling an UNICODE application DLL\n")
                                                 _T("This is an unsupported scenario and will cause errors!"));
    return false;
  }
#else
  if(p_unicode)
  {
    SvcReportErrorEvent(0,false,_T(__FUNCTION__),_T("An UNICODE mode MarlinModule is calling an ANSI application DLL\n")
                                                 _T("This is an unsupported scenario and will cause errors!"));
    return false;
  }
#endif
  // We have done our version check
  m_versionCheck = true;
  return true;
}

// Default implementation. Use the Marlin error report
ErrorReport*
ServerApp::GetErrorReport()
{
  return m_errorReport;
}

// Setting the logging level
void 
ServerApp::SetLogLevel(int p_logLevel)
{
  m_logLevel = p_logLevel;
  if(m_httpServer)
  {
    m_httpServer->SetLogLevel(p_logLevel);
  }
  if(m_logfile)
  {
    m_logfile->SetLogLevel(p_logLevel);
  }
}

// LoadSites will call LoadSite or its overloaded methods
// Do not forget to call this one, or at least increment the number of sites
bool
ServerApp::LoadSite(IISSiteConfig& /*p_config*/)
{
  ++m_numSites;
  return true;
}

// When stopping the sites: calls the overloaded methods
// Do not forget to call this one, or at least decrement the number of sites
bool
ServerApp::UnloadSite(IISSiteConfig* /*p_config*/)
{
  --m_numSites;
  return true;
}

IISSiteConfig* 
ServerApp::GetSiteConfig(int ind)
{
  if(ind >= 0 && ind < (int)m_sites.size())
  {
    return m_sites[ind];
  }
  return nullptr;
}

// Start our sites from the IIS configuration
void 
ServerApp::LoadSites(IHttpApplication* p_app,XString p_physicalPath)
{
  XString config(p_app->GetAppConfigPath());
  int pos = config.ReverseFind('/');
  XString configSite = config.Mid(pos + 1);

  CComBSTR siteCollection = L"system.applicationHost/sites";
  CComBSTR configPath = StringToWString(config).c_str();

  // Reading all global modules of the IIS installation
  ReadModules(configPath);

  IAppHostElement*      element = nullptr;
  IAppHostAdminManager* manager = g_iisServer->GetAdminManager();
  if(manager->GetAdminSection(siteCollection,configPath,&element) == S_OK)
  {
    IAppHostElementCollection* sites;
    element->get_Collection(&sites);
    if(sites)
    {
      DWORD count = 0;
      sites->get_Count(&count);
      for(int i = 0; i < (int)count; ++i)
      {
        IISSiteConfig* iisConfig = new IISSiteConfig();
        iisConfig->m_id       = 0;
        iisConfig->m_physical = p_physicalPath;
        
        // Now read in the rest of the configuration
        if(ReadSite(sites,configSite,i,*iisConfig))
        {
          // Get Handlers from global site config
          ReadHandlers(configPath,*iisConfig);

          if(LoadSite(*iisConfig))
          {
            DETAILLOGV(_T("Loaded IIS Site: %s"),config.GetString());
            // Save the site config
            m_sites.push_back(iisConfig);
            return;
          }
        }
        delete iisConfig;
      }
    }
  }
  XString text(_T("ERROR Loading IIS Site: "));
  text += config;
  ERRORLOG(ERROR_NO_SITENAME,text);
}

// Stopping all of our sites in the IIS configuration
void 
ServerApp::UnloadSites()
{
  for(auto& site : m_sites)
  {
    UnloadSite(site);
  }
}

void
ServerApp::ReadModules(CComBSTR& /*p_configPath*/)
{
//   USES_CONVERSION;
//   IAppHostAdminManager* manager = g_iisServer->GetAdminManager();
// 
//   // Reading all names of the modules of our DLL
//   IAppHostElement* modulesElement = nullptr;
//   if (manager->GetAdminSection(CComBSTR(L"system.webServer/globalModules"),p_configPath,&modulesElement) == S_OK)
//   {
//     IAppHostElementCollection* modulesCollection = nullptr;
//     modulesElement->get_Collection(&modulesCollection);
//     if (modulesCollection)
//     {
//       DWORD dwElementCount = 0;
//       modulesCollection->get_Count(&dwElementCount);
// 
//       for (USHORT dwElement = 0; dwElement < dwElementCount; ++dwElement)
//       {
//         VARIANT vtItemIndex;
//         vtItemIndex.vt = VT_I2;
//         vtItemIndex.iVal = dwElement;
// 
//         IAppHostElement* childElement = nullptr;
//         if (modulesCollection->get_Item(vtItemIndex, &childElement) == S_OK)
//         {
//           IAppHostProperty* prop = nullptr;
//           VARIANT vvar;
//           vvar.bstrVal = 0;
// 
//           if (childElement->GetPropertyByName(CComBSTR(L"image"), &prop) == S_OK && prop->get_Value(&vvar) == S_OK && vvar.vt == VT_BSTR)
//           {
//             XString image = W2A(vvar.bstrVal);
//             if (image.CompareNoCase(GetDLLName()) == 0)
//             {
//               if (childElement->GetPropertyByName(CComBSTR(L"name"), &prop) == S_OK && prop->get_Value(&vvar) == S_OK && vvar.vt == VT_BSTR)
//               {
//                 m_modules.insert(XString(W2A(vvar.bstrVal)).MakeLower());
//               }
//             }
//           }
//         }
//       }
//     }
//   }
}

void  
ServerApp::ReadHandlers(CComBSTR& p_configPath,IISSiteConfig& p_config)
{
  IAppHostAdminManager* manager = g_iisServer->GetAdminManager();

  // Finding all HTTP Handlers in the configuration
  IAppHostElement* handlersElement = nullptr;
  if (manager->GetAdminSection(CComBSTR(L"system.webServer/handlers"), p_configPath, &handlersElement) == S_OK)
  {
    IAppHostElementCollection* handlersCollection = nullptr;
    handlersElement->get_Collection(&handlersCollection);
    if (handlersCollection)
    {
      DWORD dwElementCount = 0;
      handlersCollection->get_Count(&dwElementCount);
      for (USHORT dwElement = 0; dwElement < dwElementCount; ++dwElement)
      {
        VARIANT vtItemIndex;
        vtItemIndex.vt = VT_I2;
        vtItemIndex.iVal = dwElement;

        IAppHostElement* childElement = nullptr;
        if (handlersCollection->get_Item(vtItemIndex, &childElement) == S_OK)
        {
          IAppHostProperty* prop = nullptr;
          VARIANT vvar;
          vvar.bstrVal = 0;

          XString name;
          XString modules;
          IISHandler handler;

          if (childElement->GetPropertyByName(CComBSTR(L"modules"), &prop) == S_OK && prop->get_Value(&vvar) == S_OK && vvar.vt == VT_BSTR)
          {
            modules = WStringToString(vvar.bstrVal);
          }
          if (m_modules.find(modules.MakeLower()) == m_modules.end())
          {
            continue;
          }
          if (childElement->GetPropertyByName(CComBSTR(L"name"), &prop) == S_OK && prop->get_Value(&vvar) == S_OK && vvar.vt == VT_BSTR)
          {
            name = WStringToString(vvar.bstrVal);
          }
          if (childElement->GetPropertyByName(CComBSTR(L"path"), &prop) == S_OK && prop->get_Value(&vvar) == S_OK && vvar.vt == VT_BSTR)
          {
            handler.m_path = WStringToString(vvar.bstrVal);
          }
          if (childElement->GetPropertyByName(CComBSTR(L"verb"), &prop) == S_OK && prop->get_Value(&vvar) == S_OK && vvar.vt == VT_BSTR)
          {
            handler.m_verb = WStringToString(vvar.bstrVal);
          }
          if (childElement->GetPropertyByName(CComBSTR(L"resourceType"), &prop) == S_OK && prop->get_Value(&vvar) == S_OK && vvar.vt == VT_I4)
          {
            handler.m_resourceType.Format(_T("%d"),vvar.intVal);
          }
          if (childElement->GetPropertyByName(CComBSTR(L"preCondition"), &prop) == S_OK && prop->get_Value(&vvar) == S_OK && vvar.vt == VT_BSTR)
          {
            handler.m_precondition = WStringToString(vvar.bstrVal);
          }
          // Add handler mapping to site
          p_config.m_handlers.insert(std::make_pair(name,handler));
        }
      }
    }
  }
}

// Read the site's configuration from the IIS internal structures
bool  
ServerApp::ReadSite(IAppHostElementCollection* p_sites,XString p_siteName,int p_num,IISSiteConfig& p_config)
{
  IAppHostElement* site;
  VARIANT v;
  v.vt     = VT_INT;
  v.intVal = p_num;
  if(p_sites->get_Item(v,&site) != S_OK)
  {
    return false;
  }

  // Find our site
  XString name = GetProperty(site,_T("name"));
  if(p_siteName.CompareNoCase(name) != 0)
  {
    return false;
  }

  // Remember our site name
  p_config.m_name = name;
  // Record IIS ID of the site
  p_config.m_id = _ttoi(GetProperty(site,_T("id")));

  // Load Application
  IAppHostElement* application = nullptr;
  CComBSTR applic = L"application";
  if(site->GetElementByName(applic,&application) == S_OK)
  {
    p_config.m_pool = GetProperty(application,_T("applicationPool"));
  }

  // Load Directories
  IAppHostElement* virtualDir = nullptr;
  CComBSTR virtDir = L"virtualDirectory";
  if(site->GetElementByName(virtDir,&virtualDir) == S_OK)
  {
    p_config.m_base_url = GetProperty(virtualDir,_T("path"));
    p_config.m_physical = GetProperty(virtualDir,_T("physicalPath"));
  }

  // Load Bindings
  IAppHostElement* bindings = nullptr;
  CComBSTR bind = L"bindings";
  if(site->GetElementByName(bind,&bindings) == S_OK)
  {
    IAppHostElementCollection* collection;
    bindings->get_Collection(&collection);
    if(collection)
    {
      DWORD count;
      collection->get_Count(&count);
      for(int i = 0; i < (int)count; ++i)
      {
        IISBinding binding;
        if(ReadBinding(collection,i,binding))
        {
          p_config.m_bindings.push_back(binding);
        }
      }
    }
  }
  if(p_config.m_bindings.empty())
  {
    ERRORLOG(ERROR_NOT_FOUND,_T("Site bindings not found for: ") + p_siteName);
    return false;
  }
  return true;
}

bool
ServerApp::ReadBinding(IAppHostElementCollection* p_bindings,int p_item,IISBinding& p_binding)
{
  // Getting the binding
  IAppHostElement* binding;
  VARIANT v;
  v.vt     = VT_INT;
  v.intVal = p_item;
  if(p_bindings->get_Item(v,&binding) != S_OK)
  {
    return false;
  }
  // Finding the protocol
  XString protocol = GetProperty(binding,_T("protocol"));
  p_binding.m_secure = protocol.CompareNoCase(_T("https")) == 0 ? true : false;

  // Binding information
  XString info = GetProperty(binding,_T("bindingInformation"));
  switch(info.GetAt(0))
  {
    case _T('*'): p_binding.m_prefix = PrefixType::URLPRE_Weak;    break;
    case _T('+'): p_binding.m_prefix = PrefixType::URLPRE_Strong;  break;
    default:  p_binding.m_prefix = PrefixType::URLPRE_Address; break;
  }

  // Port number
  p_binding.m_port = INTERNET_DEFAULT_HTTP_PORT;
  int pos = info.Find(':');
  if(pos >= 0)
  { 
    p_binding.m_port = _ttoi(info.Mid(pos + 1));
  }

  // Client certificate flags
  p_binding.m_flags = _ttoi(GetProperty(binding,_T("sslFlags")));

  return true;
}

XString
ServerApp::GetProperty(IAppHostElement* p_elem,XString p_property)
{
  IAppHostProperty* prop = nullptr;
  CComBSTR proper = StringToWString(p_property).c_str();
  if(p_elem->GetPropertyByName(proper,&prop) == S_OK)
  {
    BSTR strValue;
    prop->get_StringValue(&strValue);

    return XString(strValue);
  }
  return _T("");
}

//////////////////////////////////////////////////////////////////////////
//
// SERVER APP FACTORY
// Override to create your own "ServerApp derived class object"
//
//////////////////////////////////////////////////////////////////////////

ServerAppFactory::ServerAppFactory()
{
  if(appFactory)
  {
    OutputDebugString(_T("You can only have ONE singleton ServerAppFactory in your program logic"));
    assert(FALSE);
  }
  else
  {
    appFactory = this;
  }
}

ServerApp* 
ServerAppFactory::CreateServerApp(IHttpServer*  p_iis
                                 ,const TCHAR*   p_webroot
                                 ,const TCHAR*   p_appName)
{
  return new ServerApp(p_iis,p_webroot,p_appName);
}

ServerAppFactory* appFactory = nullptr;
