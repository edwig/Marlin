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
#include "MarlinModule.h"
#include "WebConfigIIS.h"
#include "EnsureFile.h"
#include <string>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define DETAILLOGV(text,...)    m_httpServer->DetailLogV(__FUNCTION__,LogType::LOG_INFO,text,__VA_ARGS__)
#define WARNINGLOG(text,...)    m_httpServer->DetailLogV(__FUNCTION__,LogType::LOG_WARN,text,__VA_ARGS__)
#define ERRORLOG(code,text)     m_httpServer->ErrorLog  (__FUNCTION__,code,text)

//XTOR
ServerApp::ServerApp(IHttpServer* p_iis,CString p_appName,CString p_webroot)
          :m_iis(p_iis)
          ,m_applicationName(p_appName)
          ,m_webroot(p_webroot)
{
}

// DTOR
ServerApp::~ServerApp()
{
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

  // Start our own logging file
  StartLogging();
  m_httpServer->SetLogging(m_logfile);
  m_httpServer->SetLogLevel(m_logfile->GetLogLevel());

  // Create our error report
  if(g_report == nullptr)
  {
    m_errorReport = new ErrorReport();
    m_ownReport   = true;
    m_httpServer->SetErrorReport(m_errorReport);
  }
  else
  {
    m_errorReport = g_report;
    m_httpServer->SetErrorReport(g_report);
  }

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
    m_logfile = nullptr;
  }

  // Destroy the general error report
  if(m_errorReport && m_ownReport)
  {
    delete m_errorReport;
    m_errorReport = nullptr;
  }
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
  // Create the directory for the logfile
  CString logfile = g_config.GetLogfilePath() + "\\" + m_applicationName + "\\Logfile.txt";
  EnsureFile ensure(logfile);
  ensure.CheckCreateDirectory();

  // Create the logfile
  m_logfile = new LogAnalysis(m_applicationName);
  m_logfile->SetLogFilename(logfile);
  m_logfile->SetLogRotation(true);
  m_logfile->SetLogLevel(g_config.GetDoLogging() ? HLL_LOGGING : HLL_NOLOG);

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
        IISSiteConfig iisConfig;
        iisConfig.m_id       = 0;
        iisConfig.m_physical = p_physicalPath;
        
        // Now read in the rest of the configuration
        if(ReadSite(sites,configSite,i,iisConfig))
        {
          if(LoadSite(iisConfig))
          {
            DETAILLOGV("Loaded IIS Site: %s",config.GetString());
            return;
          }
        }
      }
    }
  }
  CString text("ERROR Loading IIS Site: ");
  text += config;
  ERRORLOG(ERROR_NO_SITENAME,text);
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
ServerAppFactory::CreateServerApp(IHttpServer* p_iis,CString p_appName,CString p_webroot)
{
  return new ServerApp(p_iis,p_appName,p_webroot);
}

ServerAppFactory* appFactory = nullptr;
