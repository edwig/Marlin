/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: WebConfigIIS.cpp
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
#include "pch.h"
#include "WebConfigIIS.h"
#include "XMLMessage.h"
#include "FileBuffer.h"
#include <ServiceReporting.h>
#include <http.h>
#include <io.h>

WebConfigIIS::WebConfigIIS(const XString& p_application /*=""*/)
             :m_application(p_application)
{
}

WebConfigIIS::~WebConfigIIS()
{
}

bool
WebConfigIIS::ReadConfig()
{
  // Reads the central IIS application host configuration file first
  // this file contains the defaults for IIS.
  // It can be locked for short periods of time by the WAS service or the IIS service
  for(int tries = 0;tries < APPHOST_CONFIG_RETRIES;++tries)
  {
    if(ReadConfig(_T("%windir%\\system32\\inetsrv\\config\\ApplicationHost.Config"),nullptr))
    {
      return true;
    }
    Sleep(100);
  }
  SvcReportErrorEvent(0,false,_T(__FUNCTION__),_T("Cannot read the standard IIS 'ApplicationHost.Config'!"));
  return false;
}

bool
WebConfigIIS::ReadConfig(const XString& p_application,const XString& p_extraWebConfig /*= ""*/)
{
  // Reads the central IIS application host configuration file first
  if(!ReadConfig())
  {
    return false;
  }
  if(!p_application.IsEmpty())
  {
    SetApplication(p_application);
  }
  else if(!p_extraWebConfig.IsEmpty())
  {
    if(ReadConfig(p_extraWebConfig,nullptr) == false)
    {
      SvcReportErrorEvent(0,false,_T(__FUNCTION__),XString(_T("Cannot read the extra Web.Config file: ")) + p_extraWebConfig);
      return false;
    }
  }
  return true;
}

void
WebConfigIIS::SetApplication(const XString& p_application)
{
  // Try reading applications web.config
  IISSite* site = const_cast<IISSite*>(GetSite(p_application));
  if(site)
  {
    // Get virtual directory: can be outside "inetpub\wwwroot"
    XString config = site->m_path + _T("\\web.config");
    // Read the web.config of the application site, if any requested
    if(ReadConfig(config,site))
    {
      m_webconfig = config;
    }
    else
    {
      SvcReportErrorEvent(0,false,_T(__FUNCTION__),_T("Cannot read the standard Web.Config file!"));
    }
  }
  // Store the application
  m_application = p_application;
}

XString
WebConfigIIS::GetSiteName(const XString& p_site) const
{
  IISSite* site = const_cast<IISSite*>(GetSite(p_site));
  if (site)
  {
    return site->m_name;
  }
  return _T("");
}

XString
WebConfigIIS::GetSetting(const XString& p_key) const
{
  AppSettings::const_iterator it = m_settings.find(p_key);
  if(it != m_settings.end())
  {
    return it->second;
  }
  return _T("");
}

XString
WebConfigIIS::GetSiteAppPool(const XString& p_site) const
{
  IISSite* site = const_cast<IISSite*>(GetSite(p_site));
  if(site)
  {
    return site->m_appPool;
  }
  return XString();
}

XString
WebConfigIIS::GetSiteBinding(const XString& p_site,const XString& p_default) const
{
  IISSite* site = const_cast<IISSite*>(GetSite(p_site));
  if(site)
  {
    return site->m_binding;
  }
  return p_default;
}

XString
WebConfigIIS::GetSiteProtocol(const XString& p_site,const XString& p_default) const
{
  IISSite* site = const_cast<IISSite*>(GetSite(p_site));
  if(site)
  {
    return site->m_protocol;
  }
  return p_default;
}

int
WebConfigIIS::GetSitePort(const XString& p_site,int p_default) const
{
  IISSite* site = const_cast<IISSite*>(GetSite(p_site));
  if(site)
  {
    return site->m_port;
  }
  return p_default;
}

bool
WebConfigIIS::GetSiteSecure(const XString& p_site,bool p_default) const
{
  IISSite* site = const_cast<IISSite*>(GetSite(p_site));
  if(site)
  {
    return site->m_secure;
  }
  return p_default;
}

XString
WebConfigIIS::GetSiteWebroot(const XString& p_site,const XString& p_default) const
{
  IISSite* site = const_cast<IISSite*>(GetSite(p_site));
  if(site)
  {
    return site->m_path;
  }
  return p_default;
}

XString
WebConfigIIS::GetSitePathname(const XString& p_site,const XString& p_default) const
{
  IISSite* site = const_cast<IISSite*>(GetSite(p_site));
  if(site)
  {
    return site->m_path;
  }
  return p_default;
}

ULONG
WebConfigIIS::GetSiteScheme(const XString& p_site,const ULONG p_default) const
{
  IISSite* site = const_cast<IISSite*>(GetSite(p_site));
  if(site)
  {
    return site->m_authScheme;
  }
  return p_default;
}

XString
WebConfigIIS::GetSiteRealm(const XString& p_site,const XString& p_default) const
{
  IISSite* site = const_cast<IISSite*>(GetSite(p_site));
  if(site)
  {
    return site->m_realm;
  }
  return p_default;
}

XString
WebConfigIIS::GetSiteDomain(const XString& p_site,const XString& p_default) const
{
  IISSite* site = const_cast<IISSite*>(GetSite(p_site));
  if(site)
  {
    return site->m_domain;
  }
  return p_default;
}

bool
WebConfigIIS::GetSiteNTLMCache(const XString& p_site,const bool p_default) const
{
  IISSite* site = const_cast<IISSite*>(GetSite(p_site));
  if(site)
  {
    return site->m_ntlmCache;
  }
  return p_default;
}

bool
WebConfigIIS::GetSitePreload(const XString& p_site) const
{
  IISSite* site = const_cast<IISSite*>(GetSite(p_site));
  if(site)
  {
    return site->m_preload;
  }
  return false;
}

IISError
WebConfigIIS::GetSiteError(const XString& p_site) const
{
  IISSite* site = const_cast<IISSite*>(GetSite(p_site));
  if (site)
  {
    return site->m_error;
  }
  return IISER_NoError;
}

XString
WebConfigIIS::GetWebConfig()
{
  return m_webconfig;
}

XString
WebConfigIIS::GetPoolStartMode(const XString& p_pool) const
{
  IISAppPool* pool = const_cast<IISAppPool*>(GetPool(p_pool));
  if(pool)
  {
    return pool->m_startMode;
  }
  return XString();
}

XString
WebConfigIIS::GetPoolPeriodicRestart(const XString& p_pool) const
{
  IISAppPool* pool = const_cast<IISAppPool*>(GetPool(p_pool));
  if(pool)
  {
    return pool->m_periodicRestart;
  }
  return XString();
}

XString
WebConfigIIS::GetPoolIdleTimeout(const XString& p_pool) const
{
  IISAppPool* pool = const_cast<IISAppPool*>(GetPool(p_pool));
  if(pool)
  {
    return pool->m_idleTimeout;
  }
  return XString();
}

bool
WebConfigIIS::GetPoolAutostart(const XString& p_pool) const
{
  IISAppPool* pool = const_cast<IISAppPool*>(GetPool(p_pool));
  if(pool)
  {
    return pool->m_autoStart;
  }
  return false;
}

XString
WebConfigIIS::GetPoolPipelineMode(const XString& p_pool) const
{
  IISAppPool* pool = const_cast<IISAppPool*>(GetPool(p_pool));
  if(pool)
  {
    return pool->m_pipelineMode;
  }
  return XString();
}

//////////////////////////////////////////////////////////////////////////
//
// PRIVATE METHODS
//
//////////////////////////////////////////////////////////////////////////

// Replace environment variables in a string
bool 
WebConfigIIS::ReplaceEnvironVars(XString& p_string)
{
  bool replaced = false;
  XString local(p_string);

  while(true)
  {
    // Find first and second '%' marker
    int beginPos = local.Find('%');
    if(beginPos < 0)
    {
      return replaced;
    }
    int endPos = local.Find(_T('%'),beginPos+1);
    if(endPos <= beginPos)
    {
      break;
    }

    // Two markers found
    XString var = local.Mid(beginPos + 1, (endPos - beginPos - 1));
    XString value;
    if(value.GetEnvironmentVariable(var) && !value.IsEmpty())
    {
      local = local.Left(beginPos) + value + local.Mid(endPos + 1);
      p_string = local;
      replaced = true;
    }
    else
    {
      break;
    }
  }
  return false;
}

bool
WebConfigIIS::ReadConfig(const XString& p_configFile,IISSite* p_site /*=nullptr*/)
{
  XString configFile(p_configFile);
  // Find the file, see if we have read access at least
  ReplaceEnvironVars(configFile);
  if(_taccess(configFile,4) != 0)
  {
    return false;
  }

  // See if we did already read this file earlier
  configFile.MakeLower();
  WCFiles::iterator it = m_files.find(configFile);
  if(it != m_files.end() && !p_site)
  {
    return true;
  }

  // Parse the incoming file
  XMLMessage msg;
  if(!msg.LoadFile(configFile))
  {
    SvcReportErrorEvent(0,false,_T(__FUNCTION__),XString(_T("Cannot read the application host file: ")) + configFile);
    return false;
  }
  if(msg.GetInternalError() != XmlError::XE_NoError)
  {
    SvcReportErrorEvent(0,false,_T(__FUNCTION__),XString(_T("XML Error in the application host file: ")) + configFile);
    return false;
  }
  ReadLogPath(msg);
  ReadAppPools(msg);
  ReadSites(msg);
  ReadSettings(msg);

  // Read web.config overrides for application
  if(p_site)
  {
    ReadStreamingLimit(msg,nullptr);
    ReadAuthentication(*p_site,msg);
    ReadHandlerMapping(*p_site,msg);
  }

  // Remember we did read this file
  m_files.insert(std::make_pair(configFile,1));

  return true;
}

void
WebConfigIIS::ReadLogPath(XMLMessage& p_msg)
{
  XMLElement* log = p_msg.FindElement(_T("log"));
  if(log)
  {
    XMLElement* central = p_msg.FindElement(log,_T("centralW3CLogFile"));
    if(central)
    {
      XString enabled   = p_msg.GetAttribute(central,_T("enabled"));
      XString directory = p_msg.GetAttribute(central,_T("directory"));

      // Process the directory
      ReplaceEnvironVars(directory);

      // Remember this
      m_logging = enabled.CompareNoCase(_T("true")) == 0;
      m_logpath = directory;
    }
  }
}

void
WebConfigIIS::ReadAppPools(XMLMessage& p_msg)
{
  XMLElement* appPools = p_msg.FindElement(_T("applicationPools"));
  if(!appPools) return;

  XString defaultStartmode; // empty or "AlwaysRunning"
  XString defaultRecycling; // time "days.hours:minutes:seconds"

  // Find defaults for all the pools
  XMLElement* defs = p_msg.FindElement(appPools,_T("applicationPoolDefaults"));
  if(defs)
  {
    defaultStartmode = p_msg.GetAttribute(defs,_T("startMode"));
    XMLElement* restart = p_msg.FindElement(defs,_T("periodicRestart"));
    if(restart)
    {
      defaultRecycling = p_msg.GetAttribute(restart,_T("time"));
    }
  }

  // Find pools and specific settings for the pool
  XMLElement* pools = p_msg.FindElement(appPools,_T("add"));
  while(pools)
  {
    IISAppPool pool;
    pool.m_autoStart       = false;
    pool.m_startMode       = defaultStartmode;
    pool.m_periodicRestart = defaultRecycling;
    
    // Getting pool name
    XString name = p_msg.GetAttribute(pools,_T("name"));
    pool.m_name = name;

    // Getting pool startmode
    XString startMode = p_msg.GetAttribute(pools,_T("startMode"));
    if(!startMode.IsEmpty())
    {
      pool.m_startMode = startMode;
    }

    // Getting pool autostart
    XString autostart = p_msg.GetAttribute(pools,_T("autoStart"));
    if(autostart.CompareNoCase(_T("true")) == 0)
    {
      pool.m_autoStart = true;
    }

    // Getting the pipeline mode
    pool.m_pipelineMode = p_msg.GetAttribute(pools,_T("managedPipelineMode"));

    // Getting idle timeout
    XMLElement* process = p_msg.FindElement(pools,_T("processModel"));
    if(process)
    {
      pool.m_idleTimeout = p_msg.GetAttribute(process,_T("idleTimeout"));
    }

    // Retain pool information
    name.MakeLower();
    m_pools.insert(std::make_pair(name,pool));

    // Getting the next pool
    pools = p_msg.GetElementSibling(pools);
    if(pools && pools->GetName().Compare(_T("add")))
    {
      break;
    }
  }
}

void
WebConfigIIS::ReadSites(XMLMessage& p_msg)
{
  XMLElement* sites = p_msg.FindElement(_T("sites"));
  if(!sites) return;

  XMLElement* site = p_msg.FindElement(sites,_T("site"));
  while(site)
  {
    if(site->GetName() == _T("site"))
    {
      IISSite theSite;
      theSite.m_ntlmCache  = true;
      theSite.m_secure     = false;
      theSite.m_preload    = false;
      theSite.m_authScheme = 0;
      theSite.m_error      = IISER_NoError;
      theSite.m_name       = p_msg.GetAttribute(site,_T("name"));

      // Application
      XMLElement* applic = p_msg.FindElement(site,_T("application"));
      if(applic)
      {
        theSite.m_appPool = p_msg.GetAttribute(applic,_T("applicationPool"));
        theSite.m_preload = p_msg.GetAttribute(applic,_T("preloadEnabled")).CompareNoCase(_T("true")) == 0;
      }

      // Virtual path
      XMLElement* virtdir = p_msg.FindElement(site,_T("virtualDirectory"));
      if(virtdir)
      {
        theSite.m_path = p_msg.GetAttribute(virtdir,_T("physicalPath"));
        ReplaceEnvironVars(theSite.m_path);
      }

      // First binding
      XMLElement* binding = p_msg.FindElement(site,_T("binding"));
      if(binding)
      {
        theSite.m_binding  = p_msg.GetAttribute(binding,_T("bindingInformation"));
        theSite.m_protocol = p_msg.GetAttribute(binding,_T("protocol"));
        theSite.m_secure   = p_msg.GetAttribute(binding,_T("sslFlags")).GetLength() > 0;
      }

      // Port is part of URLACL binding "*:80" or "+:443" or something
      theSite.m_port = _ttoi(theSite.m_binding.Mid(2));

      // Check for secure site
      if(theSite.m_protocol.CompareNoCase(_T("https")) == 0)
      {
        theSite.m_secure = true;
      }

      ReadStreamingLimit(p_msg,site);
      ReadAuthentication(theSite,p_msg);

      // Add to sites
      XString name = theSite.m_name;
      name.MakeLower();
      m_sites.insert(std::make_pair(name,theSite));
    }
    // Next site
    site = p_msg.GetElementSibling(site);
  }
}

void
WebConfigIIS::ReadSettings(XMLMessage& p_msg)
{
  XMLElement* settings = p_msg.FindElement(_T("appSettings"));
  if(settings)
  {
    XMLElement* add = p_msg.FindElement(settings,_T("add"));
    while(add)
    {
      XString key   = p_msg.GetAttribute(add,_T("key"));
      XString value = p_msg.GetAttribute(add,_T("value"));

      m_settings[key] = value;

      // Next setting
      add = p_msg.GetElementSibling(add);
    }
  }
}

void 
WebConfigIIS::ReadWebConfigHandlers(XMLMessage& p_msg)
{
  XMLElement* handlers = p_msg.FindElement(_T("handlers"));
  if(handlers)
  {
    XMLElement* add = p_msg.FindElement(handlers,_T("add"));
    while(add)
    {
      IISHandler handler;

      XString name           = p_msg.GetAttribute(add,_T("name"));
      handler.m_path         = p_msg.GetAttribute(add,_T("path"));
      handler.m_verb         = p_msg.GetAttribute(add,_T("verb"));
      handler.m_modules      = p_msg.GetAttribute(add,_T("modules"));
      handler.m_resourceType = p_msg.GetAttribute(add,_T("resourceType"));
      handler.m_precondition = p_msg.GetAttribute(add,_T("preCondition"));

      m_webConfigHandlers.insert(std::make_pair(name,handler));

      // Next handler mapping
      add = p_msg.GetElementSibling(add);
    }
  }
}

void 
WebConfigIIS::ReadStreamingLimit(XMLMessage& p_msg,XMLElement* p_elem)
{
  XMLElement* webserv = p_msg.FindElement(p_elem,_T("system.webServer"));
  if(webserv)
  {
    XMLElement* limit = p_msg.FindElement(webserv,_T("requestLimits"));
    if(limit)
    {
      XString max = p_msg.GetAttribute(limit,_T("maxAllowedContentLength"));
      long theLimit = _ttol(max);
      if(theLimit > 0)
      {
        m_streamingLimit = theLimit;
      }
    }
  }
}

void
WebConfigIIS::ReadAuthentication(IISSite& p_site,XMLMessage& p_msg)
{
  XMLElement* location = p_msg.FindElement(_T("location"));

  while(location)
  {
    XString path = p_msg.GetAttribute(location,_T("path"));
    if(path.CompareNoCase(p_site.m_name) == 0)
    {
       ReadAuthentication(p_site,p_msg,location);
       return;
    }
    // Next location
    location = p_msg.GetElementSibling(location);
  }
}

void
WebConfigIIS::ReadAuthentication(IISSite& p_site,XMLMessage& p_msg,XMLElement* p_elem)
{
  bool anonymousFound = false;

  XMLElement* auth = p_msg.FindElement(p_elem,_T("authentication"));
  if(!auth) return;

  // Check anonymous authentication
  XMLElement* anonymous = p_msg.FindElement(auth,_T("anonymousAuthentication"));
  if(anonymous)
  {
    XString enable = p_msg.GetAttribute(anonymous,_T("enabled"));
    if(enable.CompareNoCase(_T("true")) == 0)
    {
      // Anonymous authentication IS enabled
      // No other authentication will EVER BE DONE
      // Do not read all the other authentications
      p_site.m_authScheme = 0;
      anonymousFound = true;
    }
  }

  // Check basic authentication
  XMLElement* basic = p_msg.FindElement(auth,_T("basicAuthentication"));
  if(basic)
  {
    XString enable = p_msg.GetAttribute(basic,_T("enabled"));
    if(enable.CompareNoCase(_T("true")) == 0)
    {
      p_site.m_authScheme |= HTTP_AUTH_ENABLE_BASIC;

      XString realm = p_msg.GetAttribute(basic,_T("realm"));
      if(!realm.IsEmpty())
      {
        p_site.m_realm = realm;
      }
      XString domain = p_msg.GetAttribute(basic,_T("defaultLogonDomain"));
      if(!domain.IsEmpty())
      {
        p_site.m_domain = domain;
      }
    }
  }

  // Check Digest authentication
  XMLElement* digest = p_msg.FindElement(auth,_T("digestAuthentication"));
  if(digest)
  {
    XString enable = p_msg.GetAttribute(digest,_T("enabled"));
    if(enable.CompareNoCase(_T("true")) == 0)
    {
      p_site.m_authScheme |= HTTP_AUTH_ENABLE_DIGEST;

      XString realm = p_msg.GetAttribute(digest,_T("realm"));
      if(!realm.IsEmpty())
      {
        p_site.m_realm = realm;
      }
    }
  }

  // Check Windows Authentication
  XMLElement* windows = p_msg.FindElement(auth,_T("windowsAuthentication"));
  if(windows)
  {
    XString enable = p_msg.GetAttribute(windows,_T("enabled"));
    if(enable.CompareNoCase(_T("true")) == 0)
    {
      // See if we must de-activate NTLM caching
      XString persist = p_msg.GetAttribute(windows,_T("authPersistSingleRequest"));
      if(persist.CompareNoCase(_T("true")) == 0)
      {
        p_site.m_ntlmCache = false;
      }

      // Find providers
      XMLElement* providers = p_msg.FindElement(windows,_T("providers"));
      if(providers)
      {
        XMLElement* add = p_msg.FindElement(providers,_T("add"));
        while(add)
        {
          XString value = p_msg.GetAttribute(add,_T("value"));
          if(value.CompareNoCase(_T("NTLM")) == 0)
          {
            p_site.m_authScheme |= HTTP_AUTH_ENABLE_NTLM;
          }
          if(value.CompareNoCase(_T("Negotiate")) == 0)
          {
            p_site.m_authScheme |= HTTP_AUTH_ENABLE_NEGOTIATE;
          }
          if(value.CompareNoCase(_T("Negotiate:Kerberos")) == 0)
          {
            p_site.m_authScheme |= HTTP_AUTH_ENABLE_NEGOTIATE;
            p_site.m_authScheme |= HTTP_AUTH_ENABLE_KERBEROS;
          }
          // Next provider
          add = p_msg.GetElementSibling(add);
        }
      }
    }
  }

  if(p_site.m_authScheme && anonymousFound)
  {
    // ERROR on this site. Authentication will **NOT** work
    // Both anonymous and some other authentication found!!
    p_site.m_error = IISER_AuthenticationConflict;
  }
}

void
WebConfigIIS::ReadHandlerMapping(IISSite& p_site, XMLMessage& p_msg)
{
  XMLElement* location = p_msg.FindElement(_T("location"));

  while(location)
  {
    XString path = p_msg.GetAttribute(location, _T("path"));
    if (path.CompareNoCase(p_site.m_name) == 0)
    {
      ReadHandlerMapping(p_site,p_msg,location);
      return;
    }
    // Next location
    location = p_msg.GetElementSibling(location);
  }
}

// Getting the handler mappings from IIS
// (through ReadConfig/ReadSites)
//
void
WebConfigIIS::ReadHandlerMapping(IISSite& p_site,XMLMessage& p_msg,XMLElement* p_elem)
{
  XMLElement* sws = p_msg.FindElement(p_elem,_T("system.webServer"));
  if (!sws) return;
  XMLElement* handlers = p_msg.FindElement(sws,_T("handlers"));
  if (!handlers) return;
  XMLElement* add = p_msg.FindElement(handlers,_T("add"));
  while (add)
  {
    IISHandler handler;

    XString name           = p_msg.GetAttribute(add,_T("name"));
    handler.m_path         = p_msg.GetAttribute(add,_T("path"));
    handler.m_verb         = p_msg.GetAttribute(add,_T("verb"));
    handler.m_modules      = p_msg.GetAttribute(add,_T("modules"));
    handler.m_resourceType = p_msg.GetAttribute(add,_T("resourceType"));
    handler.m_precondition = p_msg.GetAttribute(add,_T("preCondition"));

    p_site.m_handlers.insert(std::make_pair(name,handler));

    // Next handler mapping
    add = p_msg.GetElementSibling(add);
  }
}

IISHandlers*
WebConfigIIS::GetAllHandlers(const XString& p_site) const
{
  IISSite* site = const_cast<IISSite*>(GetSite(p_site));
  if(site)
  {
    return &(site->m_handlers);
  }
  return nullptr;
}

IISHandler*
WebConfigIIS::GetHandler(const XString& p_site,const XString& p_handler) const
{
  IISSite* site = const_cast<IISSite*>(GetSite(p_site));
  if(site)
  {
    IISHandlers::iterator it = site->m_handlers.find(p_handler);
    if(it != site->m_handlers.end())
    {
      return &(it->second);
    }
  }
  return nullptr;
}

IISHandlers* 
WebConfigIIS::GetWebConfigHandlers()
{
  return &m_webConfigHandlers;
}

// Finding a site registration
// Site/Subsite -> Finds "site"
// Site         -> Finds "site"
const IISSite*
WebConfigIIS::GetSite(const XString& p_site) const
{
  XString site(p_site);

  // Registration is in lower case
  site.MakeLower();
  site.Trim('/');

  // Knock off the sub sites
  int pos = site.Find('/');
  if(pos > 0)
  {
    site = site.Left(pos);
  }

  // Find the site structure
  IISSites::const_iterator it = m_sites.find(site);
  if(it != m_sites.end())
  {
    return &it->second;
  }
  return nullptr;
}

// Finding a application pool registration
const IISAppPool* 
WebConfigIIS::GetPool(const XString& p_pool) const
{
  XString pool(p_pool);
  pool.MakeLower();
  IISPools::const_iterator it = m_pools.find(pool);
  if(it != m_pools.end())
  {
    return &(it->second);
  }
  return nullptr;
}
