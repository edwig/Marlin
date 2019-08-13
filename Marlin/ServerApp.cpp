/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: ServerApp.h
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
#include "stdafx.h"
#include "ServerApp.h"
#include "WebConfigIIS.h"
#include "EnsureFile.h"
#include <string>
#include <set>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define DETAILLOGV(text,...)    m_httpServer->DetailLogV(__FUNCTION__,LogType::LOG_INFO,text,__VA_ARGS__)
#define WARNINGLOG(text,...)    m_httpServer->DetailLogV(__FUNCTION__,LogType::LOG_WARN,text,__VA_ARGS__)
#define ERRORLOG(code,text)     m_httpServer->ErrorLog  (__FUNCTION__,code,text)

IHttpServer*  g_iisServer   = nullptr;
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
                                   ,const char*  p_webroot
                                   ,const char*  p_appName)
{
  return appFactory->CreateServerApp(p_server,p_webroot,p_appName);
}

}

// XTOR
ServerApp::ServerApp(IHttpServer* p_iis
                    ,const char*  p_webroot
                    ,const char*  p_appName)
          :m_iis(p_iis)
{
  // Keep global pointer to the server
  g_iisServer = p_iis;

  // Construct local MFC CStrings from char pointers
  m_webroot         = p_webroot;
  m_applicationName = p_appName;
}

// DTOR
ServerApp::~ServerApp()
{
  // Free all site configuration info
  for(auto& site : m_sites)
  {
    delete site;
  }

  // Just to be sure
  ExitInstance();
}

// Init our server app.
// Overrides should call this method first!
void 
ServerApp::InitInstance()
{
  // Create a marlin HTTPServer object for IIS
  m_httpServer = new HTTPServerIIS(m_applicationName);
  m_httpServer->SetWebroot(m_webroot);

  // Reading our web.config and ApplicationHost.config info
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
    m_logfile->AnalysisLog(__FUNCTION__, LogType::LOG_INFO, true, "%s closed",m_applicationName.GetString());

    delete m_logfile;
    m_logfile     = nullptr;
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
    // Create the directory for the logfile
    CString logfile = m_config.GetLogfilePath() + "\\" + m_applicationName + "\\Logfile.txt";
    EnsureFile ensure(logfile);
    ensure.CheckCreateDirectory();

    // Create the logfile
    m_logfile = new LogAnalysis(m_applicationName);
    m_logfile->SetLogFilename(logfile);
    m_logfile->SetLogRotation(true);
    m_logfile->SetLogLevel(m_config.GetDoLogging() ? HLL_LOGGING : HLL_NOLOG);

    // Record for test classes
    g_analysisLog = m_logfile;
  }

  // Tell that we started the logfile
  m_logfile->AnalysisLog(__FUNCTION__,LogType::LOG_INFO,true
                        ,"Started the application: %s",m_applicationName.GetString());
}

// Server app was correctly started by MarlinIISModule
bool
ServerApp::CorrectlyStarted()
{
  // MINIMUM REQUIREMENT:
  // If a derived class has been statically declared
  // and a IHttpServer and a HTTPServerIIS has been found
  // and a ThreadPool is initialized, we are good to go
  if(m_iis && m_httpServer && m_threadPool && m_logfile)
  {
    return true;
  }

  // Log the errors
  if(!m_iis)
  {
    ERRORLOG(ERROR_NOT_FOUND,"No connected IIS server found!");
  }
  if(!m_httpServer)
  {
    ERRORLOG(ERROR_NOT_FOUND,"No connected MarlinIIS server found!");
  }
  if(!m_threadPool)
  {
    ERRORLOG(ERROR_NOT_FOUND,"No connected threadpool found!");
  }
  return false;
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

bool
ServerApp::LoadSite(IISSiteConfig& /*p_config*/)
{
  // Already done in LoadSites
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
ServerApp::LoadSites(IHttpApplication* p_app,CString p_physicalPath)
{
  USES_CONVERSION;

  CString config(p_app->GetAppConfigPath());
  int pos = config.ReverseFind('/');
  CString configSite = config.Mid(pos + 1);

  CComBSTR siteCollection = L"system.applicationHost/sites";
  CComBSTR configPath = A2CW(config);

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
            DETAILLOGV("Loaded IIS Site: %s",config.GetString());
          }
        }
        // Save the site config
        m_sites.push_back(iisConfig);
      }
    }
  }
  CString text("ERROR Loading IIS Site: ");
  text += config;
  ERRORLOG(ERROR_NO_SITENAME,text);
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
//             CString image = W2A(vvar.bstrVal);
//             if (image.CompareNoCase(GetDLLName()) == 0)
//             {
//               if (childElement->GetPropertyByName(CComBSTR(L"name"), &prop) == S_OK && prop->get_Value(&vvar) == S_OK && vvar.vt == VT_BSTR)
//               {
//                 m_modules.insert(CString(W2A(vvar.bstrVal)).MakeLower());
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
  USES_CONVERSION;
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

          CString name;
          CString modules;
          IISHandler handler;

          if (childElement->GetPropertyByName(CComBSTR(L"modules"), &prop) == S_OK && prop->get_Value(&vvar) == S_OK && vvar.vt == VT_BSTR)
          {
            modules = W2A(vvar.bstrVal);
          }
          if (m_modules.find(modules.MakeLower()) == m_modules.end())
          {
            continue;
          }
          if (childElement->GetPropertyByName(CComBSTR(L"name"), &prop) == S_OK && prop->get_Value(&vvar) == S_OK && vvar.vt == VT_BSTR)
          {
            name = W2A(vvar.bstrVal);
          }
          if (childElement->GetPropertyByName(CComBSTR(L"path"), &prop) == S_OK && prop->get_Value(&vvar) == S_OK && vvar.vt == VT_BSTR)
          {
            handler.m_path = W2A(vvar.bstrVal);
          }
          if (childElement->GetPropertyByName(CComBSTR(L"verb"), &prop) == S_OK && prop->get_Value(&vvar) == S_OK && vvar.vt == VT_BSTR)
          {
            handler.m_verb = W2A(vvar.bstrVal);
          }
          if (childElement->GetPropertyByName(CComBSTR(L"resourceType"), &prop) == S_OK && prop->get_Value(&vvar) == S_OK && vvar.vt == VT_I4)
          {
            handler.m_resourceType.Format("%d",vvar.intVal);
          }
          if (childElement->GetPropertyByName(CComBSTR(L"preCondition"), &prop) == S_OK && prop->get_Value(&vvar) == S_OK && vvar.vt == VT_BSTR)
          {
            handler.m_precondition = W2A(vvar.bstrVal);
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
ServerApp::ReadSite(IAppHostElementCollection* p_sites,CString p_siteName,int p_num,IISSiteConfig& p_config)
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
  CString name = GetProperty(site,"name");
  if(p_siteName.CompareNoCase(name) != 0)
  {
    return false;
  }

  // Remember our site name
  p_config.m_name = name;
  // Record IIS ID of the site
  p_config.m_id = atoi(GetProperty(site,"id"));

  // Load Application
  IAppHostElement* application = nullptr;
  CComBSTR applic = L"application";
  if(site->GetElementByName(applic,&application) == S_OK)
  {
    p_config.m_pool = GetProperty(application,"applicationPool");
  }

  // Load Directories
  IAppHostElement* virtualDir = nullptr;
  CComBSTR virtDir = L"virtualDirectory";
  if(site->GetElementByName(virtDir,&virtualDir) == S_OK)
  {
    p_config.m_base_url = GetProperty(virtualDir,"path");
    p_config.m_physical = GetProperty(virtualDir,"physicalPath");
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
    ERRORLOG(ERROR_NOT_FOUND,"Site bindings not found for: " + p_siteName);
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
  CString protocol = GetProperty(binding,"protocol");
  p_binding.m_secure = protocol.CompareNoCase("https") == 0 ? true : false;

  // Binding information
  CString info = GetProperty(binding,"bindingInformation");
  switch(info.GetAt(0))
  {
    case '*': p_binding.m_prefix = PrefixType::URLPRE_Weak;    break;
    case '+': p_binding.m_prefix = PrefixType::URLPRE_Strong;  break;
    default:  p_binding.m_prefix = PrefixType::URLPRE_Address; break;
  }

  // Port number
  p_binding.m_port = INTERNET_DEFAULT_HTTP_PORT;
  int pos = info.Find(':');
  if(pos >= 0)
  { 
    p_binding.m_port = atoi(info.Mid(pos + 1));
  }

  // Client certificate flags
  p_binding.m_flags = atoi(GetProperty(binding,"sslFlags"));

  return true;
}

CString
ServerApp::GetProperty(IAppHostElement* p_elem,CString p_property)
{
  USES_CONVERSION;

  IAppHostProperty* prop = nullptr;
  if(p_elem->GetPropertyByName(A2W(p_property),&prop) == S_OK)
  {
    BSTR strValue;
    prop->get_StringValue(&strValue);

    return CString(strValue);
  }
  return "";
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
    TRACE("You can only have ONE singleton ServerAppFactory in your program logic");
    ASSERT(FALSE);
  }
  else
  {
    appFactory = this;
  }
}

ServerApp* 
ServerAppFactory::CreateServerApp(IHttpServer*  p_iis
                                 ,const char*   p_webroot
                                 ,const char*   p_appName)
{
  return new ServerApp(p_iis,p_webroot,p_appName);
}

ServerAppFactory* appFactory = nullptr;
