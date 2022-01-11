/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: WebConfigIIS.h
//
// Marlin Server: Internet server/client
// 
// Copyright (c) 2014-2022 ir. W.E. Huisman
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
#include "XMLMessage.h"
#include "FileBuffer.h"
#include <map>

typedef enum _iisConfigError
{
  IISER_NoError = 0
 ,IISER_AuthenticationConflict
}
IISError;

typedef struct _iisHandler
{
  CString   m_path;
  CString   m_verb;
  CString   m_modules;
  CString   m_resourceType;
  CString   m_precondition;
}
IISHandler;

using IISHandlers = std::map<CString,IISHandler>;

// What we want to remember about an IIS HTTP site
typedef struct _iisSite
{
  CString     m_name;
  CString     m_appPool;
  CString     m_binding;
  CString     m_protocol;
  int         m_port;
  bool        m_secure;
  CString     m_path;
  ULONG       m_authScheme;
  CString     m_realm;
  CString     m_domain;
  bool        m_ntlmCache;
  bool        m_preload;
  IISError    m_error;
  IISHandlers m_handlers;
}
IISSite;

// Waht we want to remember about an IIS Application Pool
typedef struct _appPool
{
  CString   m_name;
  CString   m_startMode;
  CString   m_periodicRestart;
  CString   m_idleTimeout;
  bool      m_autoStart;
  CString   m_pipelineMode;
}
IISAppPool;

using IISSites    = std::map<CString,IISSite>;
using IISPools    = std::map<CString,IISAppPool>;
using WCFiles     = std::map<CString,int>;
using AppSettings = std::map<CString,CString>;

class WebConfigIIS
{
public:
  WebConfigIIS(CString p_application = "");
 ~WebConfigIIS();

  // Read total config
  bool ReadConfig();
  bool ReadConfig(CString p_application,CString p_extraWebConfig = "");
  // Set a different application before re-reading the config
  void SetApplication(CString p_app);

  // GETTERS
  
  // Getting information of the server
  CString     GetLogfilePath()    { return m_logpath;         };
  bool        GetDoLogging()      { return m_logging;         };
  ULONG       GetStreamingLimit() { return m_streamingLimit;  };

  // Getting information of a site
  IISSite*    GetSite         (CString p_site);
  CString     GetSiteName     (CString p_site);
  CString     GetSetting      (CString p_key);
  CString     GetSiteAppPool  (CString p_site);
  CString     GetSiteBinding  (CString p_site,CString p_default);
  CString     GetSiteProtocol (CString p_site,CString p_default);
  int         GetSitePort     (CString p_site,int     p_default);
  bool        GetSiteSecure   (CString p_site,bool    p_default);
  CString     GetSiteWebroot  (CString p_site,CString p_default);
  CString     GetSitePathname (CString p_site,CString p_default);
  ULONG       GetSiteScheme   (CString p_site,ULONG   p_default);
  CString     GetSiteRealm    (CString p_site,CString p_default);
  CString     GetSiteDomain   (CString p_site,CString p_default);
  bool        GetSiteNTLMCache(CString p_site,bool    p_default);
  bool        GetSitePreload  (CString p_site);
  IISError    GetSiteError    (CString p_site);

  IISHandlers* GetAllHandlers (CString p_site);
  IISHandler*  GetHandler     (CString p_site,CString p_handler);

  // Getting information of a application pool
  CString     GetPoolStartMode      (CString p_pool);
  CString     GetPoolPeriodicRestart(CString p_pool);
  CString     GetPoolIdleTimeout    (CString p_pool);
  bool        GetPoolAutostart      (CString p_pool);
  CString     GetPoolPipelineMode   (CString p_pool);

  // Read one config file
  bool        ReadConfig(CString p_configFile,IISSite* p_site);

private:
  // Replace environment variables in a string
  static bool ReplaceEnvironVars(CString& p_string);
  // Pool registration
  IISAppPool* GetPool(CString p_pool);
  // Reading of the internal structures of a config file
  void        ReadLogPath (XMLMessage& p_msg);
  void        ReadAppPools(XMLMessage& p_msg);
  void        ReadSites   (XMLMessage& p_msg);
  void        ReadSettings(XMLMessage& p_msg);
  void        ReadStreamingLimit(XMLMessage& p_msg, XMLElement* p_elem);
  void        ReadAuthentication(IISSite& p_site,XMLMessage& p_msg,XMLElement* p_elem);
  void        ReadAuthentication(IISSite& p_site,XMLMessage& p_msg);
  void        ReadHandlerMapping(IISSite& p_site,XMLMessage& p_msg);
  void        ReadHandlerMapping(IISSite& p_site,XMLMessage& p_msg,XMLElement* p_elem);

  // For specific web application, or just defaults
  CString     m_application;
  // Base web.config file
  CString     m_webconfig;
  // Files already read in
  WCFiles     m_files;

  AppSettings m_settings;
  IISPools    m_pools;
  IISSites    m_sites;
  CString     m_logpath;
  bool        m_logging        { false };
  ULONG       m_streamingLimit { STREAMING_LIMIT };
};
