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
  XString   m_path;
  XString   m_verb;
  XString   m_modules;
  XString   m_resourceType;
  XString   m_precondition;
}
IISHandler;

using IISHandlers = std::map<XString,IISHandler>;

// What we want to remember about an IIS HTTP site
typedef struct _iisSite
{
  XString     m_name;
  XString     m_appPool;
  XString     m_binding;
  XString     m_protocol;
  int         m_port;
  bool        m_secure;
  XString     m_path;
  ULONG       m_authScheme;
  XString     m_realm;
  XString     m_domain;
  bool        m_ntlmCache;
  bool        m_preload;
  IISError    m_error;
  IISHandlers m_handlers;
}
IISSite;

// Waht we want to remember about an IIS Application Pool
typedef struct _appPool
{
  XString   m_name;
  XString   m_startMode;
  XString   m_periodicRestart;
  XString   m_idleTimeout;
  bool      m_autoStart;
  XString   m_pipelineMode;
}
IISAppPool;

using IISSites    = std::map<XString,IISSite>;
using IISPools    = std::map<XString,IISAppPool>;
using WCFiles     = std::map<XString,int>;
using AppSettings = std::map<XString,XString>;

class WebConfigIIS
{
public:
  WebConfigIIS(XString p_application = "");
 ~WebConfigIIS();

  // Read total config
  bool ReadConfig();
  bool ReadConfig(XString p_application,XString p_extraWebConfig = "");
  // Set a different application before re-reading the config
  void SetApplication(XString p_app);

  // GETTERS
  
  // Getting information of the server
  XString     GetLogfilePath()    { return m_logpath;         };
  bool        GetDoLogging()      { return m_logging;         };
  ULONG       GetStreamingLimit() { return m_streamingLimit;  };

  // Getting information of a site
  IISSite*    GetSite         (XString p_site);
  XString     GetSiteName     (XString p_site);
  XString     GetSetting      (XString p_key);
  XString     GetSiteAppPool  (XString p_site);
  XString     GetSiteBinding  (XString p_site,XString p_default);
  XString     GetSiteProtocol (XString p_site,XString p_default);
  int         GetSitePort     (XString p_site,int     p_default);
  bool        GetSiteSecure   (XString p_site,bool    p_default);
  XString     GetSiteWebroot  (XString p_site,XString p_default);
  XString     GetSitePathname (XString p_site,XString p_default);
  ULONG       GetSiteScheme   (XString p_site,ULONG   p_default);
  XString     GetSiteRealm    (XString p_site,XString p_default);
  XString     GetSiteDomain   (XString p_site,XString p_default);
  bool        GetSiteNTLMCache(XString p_site,bool    p_default);
  bool        GetSitePreload  (XString p_site);
  IISError    GetSiteError    (XString p_site);

  IISHandlers* GetAllHandlers (XString p_site);
  IISHandler*  GetHandler     (XString p_site,XString p_handler);

  // Getting information of a application pool
  XString     GetPoolStartMode      (XString p_pool);
  XString     GetPoolPeriodicRestart(XString p_pool);
  XString     GetPoolIdleTimeout    (XString p_pool);
  bool        GetPoolAutostart      (XString p_pool);
  XString     GetPoolPipelineMode   (XString p_pool);

  // Read one config file
  bool        ReadConfig(XString p_configFile,IISSite* p_site);

private:
  // Replace environment variables in a string
  static bool ReplaceEnvironVars(XString& p_string);
  // Pool registration
  IISAppPool* GetPool(XString p_pool);
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
  XString     m_application;
  // Base web.config file
  XString     m_webconfig;
  // Files already read in
  WCFiles     m_files;

  AppSettings m_settings;
  IISPools    m_pools;
  IISSites    m_sites;
  XString     m_logpath;
  bool        m_logging        { false };
  ULONG       m_streamingLimit { STREAMING_LIMIT };
};
