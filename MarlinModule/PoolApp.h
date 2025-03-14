/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: PoolApp.h
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
#include <httpserv.h>

typedef void ServerApp;
typedef void HTTPSite;
typedef void HTTPMessage;

// Exported functions that can be called from the MarlinModule
typedef ServerApp*    (CALLBACK* CreateServerAppFunc)(IHttpServer*,PCWSTR,PCWSTR);
typedef bool          (CALLBACK* InitServerAppFunc)  (ServerApp*,IHttpApplication* p_hhtpapp,PCWSTR p_physial);
typedef void          (CALLBACK* ExitServerAppFunc)  (ServerApp*);
typedef HTTPSite*     (CALLBACK* FindHTTPSiteFunc)   (ServerApp*,int port,PCWSTR p_url);
typedef int           (CALLBACK* GetHTTPStreamFunc)  (ServerApp*,IHttpContext*,HTTPSite*,PHTTP_REQUEST);
typedef HTTPMessage*  (CALLBACK* GetHTTPMessageFunc) (ServerApp*,IHttpContext*,HTTPSite*,PHTTP_REQUEST);
typedef bool          (CALLBACK* HandleMessageFunc)  (ServerApp*,HTTPSite* p_site,HTTPMessage*);
typedef int           (CALLBACK* SitesInApplicPool)  (ServerApp*);
typedef bool          (CALLBACK* MinVersionFunc)     (ServerApp*,int version);

// Our IIS Server
extern IHttpServer* g_IISServer;

// Helper class with a web application
// Also contains the function pointers into our application DLL
class PoolApp
{
public:
  bool LoadPoolApp(IHttpApplication* p_httpapp
                  ,XString p_configPath
                  ,XString p_webroot
                  ,XString p_physical
                  ,XString p_application
                  ,XString p_appSite);

  // DATA
  XString             m_appSite;
  XString             m_marlinDLL;
  ServerApp*          m_application     { nullptr };
  HMODULE             m_module          { NULL    };
  bool                m_xssBlocking     { false   };
  // DLL Loaded functions
  CreateServerAppFunc m_createServerApp { nullptr };
  InitServerAppFunc   m_initServerApp   { nullptr };
  ExitServerAppFunc   m_exitServerApp   { nullptr };
  FindHTTPSiteFunc    m_findSite        { nullptr };
  GetHTTPStreamFunc   m_getHttpStream   { nullptr };
  GetHTTPMessageFunc  m_getHttpMessage  { nullptr };
  HandleMessageFunc   m_handleMessage   { nullptr };
  SitesInApplicPool   m_sitesInAppPool  { nullptr };
  MinVersionFunc      m_minVersion      { nullptr };

private:
  bool    WebConfigSettings(XString p_configPath);
  XString ConstructDLLLocation(XString p_rootpath, XString p_dllPath);
  bool    CheckApplicationPresent(XString& p_dllPath, XString& p_dllName);
  bool    AlreadyLoaded(XString p_path_to_dll);

  // Application settings
  XString m_dllPath;                // mostly "<webroot>\<application>\bin"
  XString m_dllLocation;            // Name of the DLL
  XString m_adminEmail;             // Who to notify of an error
};

