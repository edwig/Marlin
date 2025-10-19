/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: WebConfigIIS.h
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
#include "XMLMessage.h"
#include "FileBuffer.h"
#include <map>

#define APPHOST_CONFIG_RETRIES  10

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
  int         m_port       { 0     };
  bool        m_secure     { false };
  XString     m_path;
  ULONG       m_authScheme { 0     };
  XString     m_realm;
  XString     m_domain;
  bool        m_ntlmCache  { false };
  bool        m_preload    { false };
  IISError    m_error      { IISER_NoError };
  IISHandlers m_handlers;
}
IISSite;

// What we want to remember about an IIS Application Pool
typedef struct _appPool
{
  XString   m_name;
  XString   m_startMode;
  XString   m_periodicRestart;
  XString   m_idleTimeout;
  bool      m_autoStart  { false };
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
  explicit WebConfigIIS(const XString& p_application = _T(""));
 ~WebConfigIIS();

  // Read total config
  bool ReadConfig();
  bool ReadConfig(const XString& p_application,const XString& p_extraWebConfig = _T(""));
  // Set a different application before re-reading the config
  void SetApplication(const XString& p_app);

  // GETTERS
  
  // Getting information of the server
  XString     GetLogfilePath()    const { return m_logpath;         };
  bool        GetDoLogging()      const { return m_logging;         };
  ULONG       GetStreamingLimit() const { return m_streamingLimit;  };

  // Getting information of a site
  const IISSite* GetSite      (const XString& p_site) const;
  XString     GetSiteName     (const XString& p_site) const;
  XString     GetSetting      (const XString& p_key)  const;
  XString     GetSiteAppPool  (const XString& p_site) const;
  XString     GetSiteBinding  (const XString& p_site,const XString& p_default) const;
  XString     GetSiteProtocol (const XString& p_site,const XString& p_default) const;
  int         GetSitePort     (const XString& p_site,const int      p_default) const;
  bool        GetSiteSecure   (const XString& p_site,const bool     p_default) const;
  XString     GetSiteWebroot  (const XString& p_site,const XString& p_default) const;
  XString     GetSitePathname (const XString& p_site,const XString& p_default) const;
  ULONG       GetSiteScheme   (const XString& p_site,const ULONG    p_default) const;
  XString     GetSiteRealm    (const XString& p_site,const XString& p_default) const;
  XString     GetSiteDomain   (const XString& p_site,const XString& p_default) const;
  bool        GetSiteNTLMCache(const XString& p_site,const bool     p_default) const;
  bool        GetSitePreload  (const XString& p_site) const;
  IISError    GetSiteError    (const XString& p_site) const;
  XString     GetWebConfig();

  IISHandlers* GetAllHandlers (const XString& p_site) const;
  IISHandler*  GetHandler     (const XString& p_site,const XString& p_handler) const;
  IISHandlers* GetWebConfigHandlers();

  // Getting information of a application pool
  XString     GetPoolStartMode      (const XString& p_pool) const;
  XString     GetPoolPeriodicRestart(const XString& p_pool) const;
  XString     GetPoolIdleTimeout    (const XString& p_pool) const;
  bool        GetPoolAutostart      (const XString& p_pool) const;
  XString     GetPoolPipelineMode   (const XString& p_pool) const;

  // Read one config file
  bool        ReadConfig(const XString& p_configFile,IISSite* p_site);
  void        ReadWebConfigHandlers(XMLMessage& p_msg);

private:
  // Replace environment variables in a string
  static bool ReplaceEnvironVars(XString& p_string);
  // Pool registration
  const IISAppPool* GetPool(const XString& p_pool) const;
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

  IISHandlers m_webConfigHandlers;
  AppSettings m_settings;
  IISPools    m_pools;
  IISSites    m_sites;
  XString     m_logpath;
  bool        m_logging        { false };
  ULONG       m_streamingLimit { STREAMING_LIMIT };
};
