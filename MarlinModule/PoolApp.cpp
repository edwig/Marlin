/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: PoolApp.cpp
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
#include "PoolApp.h"
#include "ServiceReporting.h"
#include "..\Marlin\Version.h"
#include <map>
#include <io.h>

#ifdef _AFX
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#endif

#define MODULE_NAME  _T("Application")
#define MODULE_PATH  _T("Directory")
#define MODULE_EMAIL _T("AdministratorEmail")

// Logging macro for this file only
#define DETAILLOG(text)    SvcReportInfoEvent(false,text);
#define ERRORLOG(text)     SvcReportErrorEvent(0,false,_T(__FUNCTION__),text);

// General error function
void Unhealthy(XString p_error, HRESULT p_code);

PoolApp::~PoolApp()
{
  // Only delete application if last site was deallocated
  // If ServerApp::LoadSite() overloads forget to call base method
  // the reference count can drop below zero
  if (m_application && (*m_sitesInAppPool)(m_application) <= 0)
  {
    delete m_application;
  }
}

bool
PoolApp::LoadPoolApp(IHttpApplication* p_httpapp
                    ,XString p_configPath
                    ,XString p_webroot
                    ,XString p_physical
                    ,XString p_application)
{
  // Read Web config from "physical-application-path" + "web.config"
  XString baseWebConfig = p_physical + _T("web.config");
  baseWebConfig.MakeLower();

  WebConfigSettings(p_configPath);

  // Load the **real** application DLL from the settings of web.config
  if(m_dllLocation.IsEmpty())
  {
    Unhealthy(_T("MarlinModule could **NOT** locate the 'Application' in web.config: ") + baseWebConfig,ERROR_NOT_FOUND);
    return false;
  }

  if(m_dllPath.IsEmpty())
  {
    Unhealthy(_T("MarlinModule could **NOT** locate the 'Directory' in web.config: ") + baseWebConfig,ERROR_NOT_FOUND);
    return false;
  }
  if(m_dllPath.Left(2).Compare(_T("..")) == 0)
  {
    Unhealthy(_T("MarlinModule can **NOT** use a relative path in the 'Directory' in web.config: ") + baseWebConfig,ERROR_NOT_FOUND);
    return false;
  } 

  // Find our default administrator email to send a Error report to
  if(m_adminEmail.IsEmpty())
  {
    Unhealthy(_T("MarlinModule could **NOT** locate the 'AdministratorEmail' in the web.config: ") + baseWebConfig,ERROR_NOT_FOUND);
    return false;
  }
  else
  {
    extern TCHAR g_adminEmail[];
    _tcsncpy_s(g_adminEmail,MAX_PATH - 1,m_adminEmail.GetString(),MAX_PATH - 1);
  }

  // Tell MS-Windows where to look while loading our DLL
  m_dllPath = ConstructDLLLocation(p_physical,m_dllPath);
  if(SetDllDirectory(m_dllPath) == FALSE)
  {
    Unhealthy(_T("MarlinModule could **NOT** append DLL-loading search path with: ") + m_dllPath,ERROR_NOT_FOUND);
    return false;
  }

  // Ultimately check that the directory exists and that we have read rights on the application's DLL
  if(!CheckApplicationPresent(m_dllPath, m_dllLocation))
  {
    Unhealthy(_T("MarlinModule could not access the application module DLL: ") + m_dllLocation,ERROR_ACCESS_DENIED);
    return false;
  }

  // See if we must load the DLL application
  if(AlreadyLoaded(m_dllLocation))
  {
    DETAILLOG(_T("MarlinModule already loaded DLL [") + m_dllLocation + _T("] for application: ") + p_application);
  }
  else
  {
    // Try to load the DLL
    m_module = LoadLibrary(m_dllLocation);
    if (m_module)
    {
      m_marlinDLL = m_dllLocation;
      DETAILLOG(_T("MarlinModule loaded DLL from: ") + m_dllLocation);
    }
    else
    {
      HRESULT code = GetLastError();
      XString error(_T("MarlinModule could **NOT** load DLL from: ") + m_dllLocation);
      Unhealthy(error,code);
      return false;
    }

    // Getting the start address of the application factory
    // Beware: GetProcAddress is one of the only MS-Windows API's that does *NOT* have 'A' and 'W' variants
    m_createServerApp = (CreateServerAppFunc)GetProcAddress(m_module,"CreateServerApp");
    m_initServerApp   = (InitServerAppFunc)  GetProcAddress(m_module,"InitServerApp");
    m_exitServerApp   = (ExitServerAppFunc)  GetProcAddress(m_module,"ExitServerApp");
    m_findSite        = (FindHTTPSiteFunc)   GetProcAddress(m_module,"FindHTTPSite");
    m_getHttpStream   = (GetHTTPStreamFunc)  GetProcAddress(m_module,"GetStreamFromRequest");
    m_getHttpMessage  = (GetHTTPMessageFunc) GetProcAddress(m_module,"GetHTTPMessageFromRequest");
    m_handleMessage   = (HandleMessageFunc)  GetProcAddress(m_module,"HandleHTTPMessage");
    m_sitesInAppPool  = (SitesInApplicPool)  GetProcAddress(m_module,"SitesInApplicationPool");
    m_minVersion      = (MinVersionFunc)     GetProcAddress(m_module,"MinMarlinVersion");

    if(m_createServerApp == nullptr ||
       m_initServerApp   == nullptr ||
       m_exitServerApp   == nullptr ||
       m_findSite        == nullptr ||
       m_getHttpStream   == nullptr ||
       m_getHttpMessage  == nullptr ||
       m_handleMessage   == nullptr ||
       m_sitesInAppPool  == nullptr ||
       m_minVersion      == nullptr )
    {
      XString error(_T("MarlinModule loaded ***INCORRECT*** DLL. Missing 'CreateServerApp', 'InitServerApp', 'ExitServerApp', 'FindHTTPSite', 'GetStreamFromRequest', ")
                    _T("'GetHTTPMessageFromRequest', 'HandleHTTPMessage', 'SitesInApplicPool' or 'MinMarlinVersion'"));
      Unhealthy(error,ERROR_NOT_FOUND);
      return false;
    }
  }

  // Let the server app factory create a new one for us
  // And store it in our representation of the active application pool
  m_application = (*m_createServerApp)(g_iisServer                 // Microsoft IIS server object
                                      ,p_webroot.GetString()       // The IIS registered webroot
                                      ,p_application.GetString()); // The application's name
  if(m_application == nullptr)
  {
    XString error(_T("NO APPLICATION CREATED IN THE APP-FACTORY!"));
    Unhealthy(error,ERROR_SERVICE_NOT_ACTIVE);
    return false;
  }

  // Check if application can work together with the current MarlinModule
  int version = MARLIN_VERSION_MAJOR * 10000 +   // Major version main
                MARLIN_VERSION_MINOR *   100 +   // Minor version number
                MARLIN_VERSION_SP;               // Service pack
  // And also check if our ANSI/UNICODE configurations match!
#ifdef _UNICODE
  bool unicode = true;
#else
  bool unicode = false;
#endif

  if(!(*m_minVersion)(m_application,version,unicode))
  {
    XString error(_T("ERROR WRONG VERSION Application: ") + p_application);
    Unhealthy(error,ERROR_SERVICE_NOT_ACTIVE);
    return false;
  }

  // Call the initialization of the application
  if(!(*m_initServerApp)(m_application,p_httpapp,p_physical))
  {
    XString error(_T("ERROR STARTING Application: ") + p_application);
    Unhealthy(error,ERROR_SERVICE_NOT_ACTIVE);
    return false;
  }
  return true;
}

// If the given DLL begins with a '@' it is an absolute pathname
// Otherwise it is relative to the directory the 'web.config' is in
XString
PoolApp::ConstructDLLLocation(XString p_rootpath, XString p_dllPath)
{
#ifdef _DEBUG
  // ONLY FOR DEVELOPMENT TEAMS RUNNING OUTSIDE THE WEBROOT
  if(p_dllPath.GetAt(0) == '@')
  {
    return p_dllPath.Mid(1);
  }
#endif
  // Default implementation
  XString pathname = p_rootpath;
  if(pathname.Right(1) != _T("\\"))
  {
    pathname += _T("\\");
  }
  pathname += p_dllPath;
  if(pathname.Right(1) != _T("\\"))
  {
    pathname += _T("\\");
  }
  return pathname;
}

// Checking for the presence of the application DLL
bool
PoolApp::CheckApplicationPresent(XString& p_dllPath,XString& p_dllName)
{
  // Check if the directory exists
  if(_taccess(p_dllPath, 0) == -1)
  {
    ERRORLOG(_T("The directory does not exist: ") + p_dllPath);
    return false;
  }

  // Check that dllName does not have a directory
  int pos = p_dllName.Find('\\');
  if (pos >= 0)
  {
    ERRORLOG(_T("The variable 'Application' must only be the name of the application DLL: ") + p_dllName);
    return false;
  }

  // Construct the complete path and check for presence
  p_dllName = p_dllPath + p_dllName;
  if(_taccess(p_dllPath, 4) == -1)
  {
    ERRORLOG(_T("The application DLL cannot be read: ") + p_dllName);
    return false;
  }
  return true;
}

// See if we already did load the module
// For some reason reference counting does not work from within IIS.
bool
PoolApp::AlreadyLoaded(XString p_path_to_dll)
{
  // All applications in the application pool
  extern std::map<int,PoolApp*> g_IISApplicationPool;

  for(auto& app : g_IISApplicationPool)
  {
    if(p_path_to_dll.Compare(app.second->m_marlinDLL) == 0)
    {
      // Application module
      m_module          = app.second->m_module;
      m_createServerApp = app.second->m_createServerApp;
      m_initServerApp   = app.second->m_initServerApp;
      m_exitServerApp   = app.second->m_exitServerApp;
      // Function pointers
      m_findSite        = app.second->m_findSite;
      m_getHttpStream   = app.second->m_getHttpStream;
      m_getHttpMessage  = app.second->m_getHttpMessage;
      m_handleMessage   = app.second->m_handleMessage;
      m_sitesInAppPool  = app.second->m_sitesInAppPool;
      m_minVersion      = app.second->m_minVersion;
      return true;
    }
  }
  return false;
}

bool
PoolApp::WebConfigSettings(XString p_configPath)
{
  IAppHostAdminManager* manager = g_iisServer->GetAdminManager();

  // Finding all application settings
  IAppHostElement* handlersElement = nullptr;
  if(manager->GetAdminSection(CComBSTR(L"appSettings"),CComBSTR(p_configPath),&handlersElement) == S_OK)
  {
    IAppHostElementCollection* handlersCollection = nullptr;
    handlersElement->get_Collection(&handlersCollection);
    if(handlersCollection)
    {
      DWORD dwElementCount = 0;
      handlersCollection->get_Count(&dwElementCount);
      for(USHORT dwElement = 0; dwElement < dwElementCount; ++dwElement)
      {
        VARIANT vtItemIndex;
        vtItemIndex.vt   = VT_I2;
        vtItemIndex.iVal = dwElement;

        IAppHostElement* childElement = nullptr;
        if(handlersCollection->get_Item(vtItemIndex,&childElement) == S_OK)
        {
          // Read key/value pairs
          // <add key="Directory" value="@C:\Develop\Marlin\BinDebug_x64\" />
          XString key;
          XString value;

          IAppHostProperty* propKey = nullptr;
          VARIANT vkey;
          vkey.bstrVal = 0;
          IAppHostProperty* propValue = nullptr;
          VARIANT vvalue;
          vvalue.bstrVal = 0;

          if(childElement->GetPropertyByName(CComBSTR(L"key"),&propKey) == S_OK && propKey->get_Value(&vkey) == S_OK && vkey.vt == VT_BSTR)
          {
            key = vkey.bstrVal;
          }
          if(childElement->GetPropertyByName(CComBSTR(L"value"),&propValue) == S_OK && propValue->get_Value(&vvalue) == S_OK && vvalue.vt == VT_BSTR)
          {
            value = vvalue.bstrVal;
          }

          if(key.CompareNoCase(_T("Directory")) == 0)
          {
            m_dllPath = value;
          }
          if(key.CompareNoCase(_T("Application")) == 0)
          {
            m_dllLocation = value;
          }
          if(key.CompareNoCase(_T("AdministratorEmail")) == 0)
          {
            m_adminEmail = value;
          }
          if(key.CompareNoCase(_T("XSSBlocking")) == 0)
          {
            m_xssBlocking = (value.CompareNoCase(_T("true")) == 0);
          }
        }
      }
    }
  }
  return !m_dllPath.IsEmpty() && !m_dllLocation.IsEmpty() && !m_adminEmail.IsEmpty();
}
