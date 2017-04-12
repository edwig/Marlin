/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: ServerApp.h
//
// Marlin Server: Internet server/client
// 
// Copyright (c) 2017 ir. W.E. Huisman
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
#include <string>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define DETAILLOGV(text,...)    m_appServer->DetailLogV(__FUNCTION__,LogType::LOG_INFO,text,__VA_ARGS__)
#define WARNINGLOG(text,...)    m_appServer->DetailLogV(__FUNCTION__,LogType::LOG_WARN,text,__VA_ARGS__)
#define ERRORLOG(code,text)     m_appServer->ErrorLog  (__FUNCTION__,code,text)


// Pointer to the one and only server object
// Meant to be run in IIS and not as a stand alone HTTP server!!
ServerApp* g_server = nullptr;

//XTOR
ServerApp::ServerApp()
{
  if(g_server)
  {
    // Cannot run more than one ServerApp
    m_correctInit = false;     
  }
  else
  {
    // First registration of the ServerApp derived class
    m_correctInit = true;
    g_server = this;
  }
}

// DTOR
ServerApp::~ServerApp()
{
}

// Called by the MarlinModule at startup of the application pool.
// Will be called **BEFORE** the "InitInstance" of the ServerApp.
void 
ServerApp::ConnectServerApp(IHttpServer*   p_iis
                           ,HTTPServerIIS* p_server
                           ,ThreadPool*    p_pool
                           ,LogAnalysis*   p_logfile
                           ,ErrorReport*   p_report)
{
  // Simply remember our registration
  m_iis        = p_iis;
  m_appServer  = p_server;
  m_appPool    = p_pool;
  m_appLogfile = p_logfile;
  m_appReport  = p_report;
}

// Server app was correctly started by MarlinIISModule
bool
ServerApp::CorrectlyStarted()
{
  // MINIMUM REQUIREMENT:
  // If a derived class has been staticly declared
  // and a IHttpServer and a HTTPServerIIS has been found
  // and a Threadpool is initialized, we are good to go
  if(m_correctInit && m_iis && m_appServer && m_appPool)
  {
    return true;
  }
  return false;
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
            DETAILLOGV("Loaded IIS Site: %s",config);
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

  // Portnumber
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
