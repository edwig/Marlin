/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: WebConfigIIS.cpp
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
#include "stdafx.h"
#include "WebConfigIIS.h"
#include "XMLMessage.h"
#include "FileBuffer.h"
#include <http.h>
#include <io.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

WebConfigIIS::WebConfigIIS(XString p_application /*=""*/)
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
  return ReadConfig("%windir%\\system32\\inetsrv\\config\\ApplicationHost.Config", nullptr);
}

bool
WebConfigIIS::ReadConfig(XString p_application,XString p_extraWebConfig /*= ""*/)
{
  // Reads the central IIS application host configuration file first
  // this file contains the defaults for IIS.
  bool result = ReadConfig("%windir%\\system32\\inetsrv\\config\\ApplicationHost.Config",nullptr);
  if(!p_application.IsEmpty())
  {
    SetApplication(p_application);
  }
  else if(!p_extraWebConfig.IsEmpty())
  {
    ReadConfig(p_extraWebConfig,nullptr);
  }
  return result;
}

void
WebConfigIIS::SetApplication(XString p_application)
{
  // Try reading applications web.config
  IISSite* site = GetSite(p_application);
  if(site)
  {
    // Get virtual directory: can be outside "inetpub\wwwroot"
    XString config = site->m_path + "\\web.config";
    // Read the web.config of the application site, if any requested
    if(ReadConfig(config,site))
    {
      m_webconfig = config;
    }
  }
  // Store the application
  m_application = p_application;
}

XString
WebConfigIIS::GetSiteName(XString p_site)
{
  IISSite* site = GetSite(p_site);
  if (site)
  {
    return site->m_name;
  }
  return "";
}

XString
WebConfigIIS::GetSetting(XString p_key)
{
  return m_settings[p_key];
}

XString
WebConfigIIS::GetSiteAppPool(XString p_site)
{
  IISSite* site = GetSite(p_site);
  if(site)
  {
    return site->m_appPool;
  }
  return XString();
}

XString
WebConfigIIS::GetSiteBinding(XString p_site,XString p_default)
{
  IISSite* site = GetSite(p_site);
  if(site)
  {
    return site->m_binding;
  }
  return p_default;
}

XString
WebConfigIIS::GetSiteProtocol(XString p_site,XString p_default)
{
  IISSite* site = GetSite(p_site);
  if(site)
  {
    return site->m_protocol;
  }
  return p_default;
}

int
WebConfigIIS::GetSitePort(XString p_site,int p_default)
{
  IISSite* site = GetSite(p_site);
  if(site)
  {
    return site->m_port;
  }
  return p_default;
}

bool
WebConfigIIS::GetSiteSecure(XString p_site,bool p_default)
{
  IISSite* site = GetSite(p_site);
  if(site)
  {
    return site->m_secure;
  }
  return p_default;
}

XString
WebConfigIIS::GetSiteWebroot(XString p_site,XString p_default)
{
  IISSite* site = GetSite(p_site);
  if(site)
  {
    return site->m_path;
  }
  return p_default;
}

XString
WebConfigIIS::GetSitePathname(XString p_site,XString p_default)
{
  IISSite* site = GetSite(p_site);
  if(site)
  {
    return site->m_path;
  }
  return p_default;
}

ULONG
WebConfigIIS::GetSiteScheme(XString p_site,ULONG   p_default)
{
  IISSite* site = GetSite(p_site);
  if(site)
  {
    return site->m_authScheme;
  }
  return p_default;
}

XString
WebConfigIIS::GetSiteRealm(XString p_site,XString p_default)
{
  IISSite* site = GetSite(p_site);
  if(site)
  {
    return site->m_realm;
  }
  return p_default;
}

XString
WebConfigIIS::GetSiteDomain(XString p_site,XString p_default)
{
  IISSite* site = GetSite(p_site);
  if(site)
  {
    return site->m_domain;
  }
  return p_default;
}

bool
WebConfigIIS::GetSiteNTLMCache(XString p_site,bool p_default)
{
  IISSite* site = GetSite(p_site);
  if(site)
  {
    return site->m_ntlmCache;
  }
  return p_default;
}

bool
WebConfigIIS::GetSitePreload(XString p_site)
{
  IISSite* site = GetSite(p_site);
  if(site)
  {
    return site->m_preload;
  }
  return false;
}

IISError
WebConfigIIS::GetSiteError(XString p_site)
{
  IISSite* site = GetSite(p_site);
  if (site)
  {
    return site->m_error;
  }
  return IISER_NoError;
}

XString
WebConfigIIS::GetPoolStartMode(XString p_pool)
{
  IISAppPool* pool = GetPool(p_pool);
  if(pool)
  {
    return pool->m_startMode;
  }
  return XString();
}

XString
WebConfigIIS::GetPoolPeriodicRestart(XString p_pool)
{
  IISAppPool* pool = GetPool(p_pool);
  if(pool)
  {
    return pool->m_periodicRestart;
  }
  return XString();
}

XString
WebConfigIIS::GetPoolIdleTimeout(XString p_pool)
{
  IISAppPool* pool = GetPool(p_pool);
  if(pool)
  {
    return pool->m_idleTimeout;
  }
  return XString();
}

bool
WebConfigIIS::GetPoolAutostart(XString p_pool)
{
  IISAppPool* pool = GetPool(p_pool);
  if(pool)
  {
    return pool->m_autoStart;
  }
  return false;
}

XString
WebConfigIIS::GetPoolPipelineMode(XString p_pool)
{
  IISAppPool* pool = GetPool(p_pool);
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
    int endPos = local.Find('%',beginPos+1);
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
WebConfigIIS::ReadConfig(XString p_configFile,IISSite* p_site /*=nullptr*/)
{
  // Find the file, see if we have read access at least
  ReplaceEnvironVars(p_configFile);
  if(_access(p_configFile,4) != 0)
  {
    return false;
  }

  // See if we did already read this file earlier
  p_configFile.MakeLower();
  WCFiles::iterator it = m_files.find(p_configFile);
  if(it != m_files.end() && !p_site)
  {
    return true;
  }

  // Parse the incoming file
  XMLMessage msg;
  msg.LoadFile(p_configFile);

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
  m_files.insert(std::make_pair(p_configFile,1));

  return true;
}

void
WebConfigIIS::ReadLogPath(XMLMessage& p_msg)
{
  XMLElement* log = p_msg.FindElement("log");
  if(log)
  {
    XMLElement* central = p_msg.FindElement(log,"centralW3CLogFile");
    if(central)
    {
      XString enabled   = p_msg.GetAttribute(central,"enabled");
      XString directory = p_msg.GetAttribute(central,"directory");

      // Process the directory
      ReplaceEnvironVars(directory);

      // Remember this
      m_logging = enabled.CompareNoCase("true") == 0;
      m_logpath = directory;
    }
  }
}

void
WebConfigIIS::ReadAppPools(XMLMessage& p_msg)
{
  XMLElement* appPools = p_msg.FindElement("applicationPools");
  if(!appPools) return;

  XString defaultStartmode; // empty or "AlwaysRunning"
  XString defaultRecycling; // time "days.hours:minutes:seconds"

  // Find defaults for all the pools
  XMLElement* defs = p_msg.FindElement(appPools,"applicationPoolDefaults");
  if(defs)
  {
    defaultStartmode = p_msg.GetAttribute(defs,"startMode");
    XMLElement* restart = p_msg.FindElement(defs,"periodicRestart");
    if(restart)
    {
      defaultRecycling = p_msg.GetAttribute(restart,"time");
    }
  }

  // Find pools and specific settings for the pool
  XMLElement* pools = p_msg.FindElement(appPools,"add");
  while(pools)
  {
    IISAppPool pool;
    pool.m_autoStart       = false;
    pool.m_startMode       = defaultStartmode;
    pool.m_periodicRestart = defaultRecycling;
    
    // Getting pool name
    XString name = p_msg.GetAttribute(pools,"name");
    pool.m_name = name;

    // Getting pool startmode
    XString startMode = p_msg.GetAttribute(pools,"startMode");
    if(!startMode.IsEmpty())
    {
      pool.m_startMode = startMode;
    }

    // Getting pool autostart
    XString autostart = p_msg.GetAttribute(pools,"autoStart");
    if(autostart.CompareNoCase("true") == 0)
    {
      pool.m_autoStart = true;
    }

    // Getting the pipeline mode
    pool.m_pipelineMode = p_msg.GetAttribute(pools,"managedPipelineMode");

    // Getting idle timeout
    XMLElement* process = p_msg.FindElement(pools,"processModel");
    if(process)
    {
      pool.m_idleTimeout = p_msg.GetAttribute(process,"idleTimeout");
    }

    // Retain pool information
    name.MakeLower();
    m_pools.insert(std::make_pair(name,pool));

    // Getting the next pool
    pools = p_msg.GetElementSibling(pools);
    if(pools && pools->GetName().Compare("add"))
    {
      break;
    }
  }
}

void
WebConfigIIS::ReadSites(XMLMessage& p_msg)
{
  XMLElement* sites = p_msg.FindElement("sites");
  if(!sites) return;

  XMLElement* site = p_msg.FindElement(sites,"site");
  while(site)
  {
    if(site->GetName() == "site")
    {
      IISSite theSite;
      theSite.m_ntlmCache  = true;
      theSite.m_secure     = false;
      theSite.m_preload    = false;
      theSite.m_authScheme = 0;
      theSite.m_error      = IISER_NoError;
      theSite.m_name       = p_msg.GetAttribute(site,"name");

      // Application
      XMLElement* applic = p_msg.FindElement(site,"application");
      if(applic)
      {
        theSite.m_appPool = p_msg.GetAttribute(applic,"applicationPool");
        theSite.m_preload = p_msg.GetAttribute(applic,"preloadEnabled").CompareNoCase("true") == 0;
      }

      // Virtual path
      XMLElement* virtdir = p_msg.FindElement(site,"virtualDirectory");
      if(virtdir)
      {
        theSite.m_path = p_msg.GetAttribute(virtdir,"physicalPath");
        ReplaceEnvironVars(theSite.m_path);
      }

      // First binding
      XMLElement* binding = p_msg.FindElement(site,"binding");
      if(binding)
      {
        theSite.m_binding  = p_msg.GetAttribute(binding,"bindingInformation");
        theSite.m_protocol = p_msg.GetAttribute(binding,"protocol");
        theSite.m_secure   = p_msg.GetAttribute(binding,"sslFlags").GetLength() > 0;
      }

      // Port is part of URLACL binding "*:80" or "+:443" or something
      theSite.m_port = atoi(theSite.m_binding.Mid(2));

      // Check for secure site
      if(theSite.m_protocol.CompareNoCase("https") == 0)
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
  XMLElement* settings = p_msg.FindElement("appSettings");
  if(settings)
  {
    XMLElement* add = p_msg.FindElement(settings,"add");
    while(add)
    {
      XString key   = p_msg.GetAttribute(add,"key");
      XString value = p_msg.GetAttribute(add,"value");

      m_settings[key] = value;

      // Next setting
      add = p_msg.GetElementSibling(add);
    }
  }
}

void 
WebConfigIIS::ReadStreamingLimit(XMLMessage& p_msg,XMLElement* p_elem)
{
  XMLElement* webserv = p_msg.FindElement(p_elem,"system.webServer");
  if(webserv)
  {
    XMLElement* limit = p_msg.FindElement(webserv,"requestLimits");
    if(limit)
    {
      XString max = p_msg.GetAttribute(limit,"maxAllowedContentLength");
      long theLimit = atol(max);
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
  XMLElement* location = p_msg.FindElement("location");

  while(location)
  {
    XString path = p_msg.GetAttribute(location,"path");
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

  XMLElement* auth = p_msg.FindElement(p_elem,"authentication");
  if(!auth) return;

  // Check anonymous authentication
  XMLElement* anonymous = p_msg.FindElement(auth,"anonymousAuthentication");
  if(anonymous)
  {
    XString enable = p_msg.GetAttribute(anonymous,"enabled");
    if(enable.CompareNoCase("true") == 0)
    {
      // Anonymous authentication IS enabled
      // No other authentication will EVER BE DONE
      // Do not read all the other authentications
      p_site.m_authScheme = 0;
      anonymousFound = true;
    }
  }

  // Check basic authentication
  XMLElement* basic = p_msg.FindElement(auth,"basicAuthentication");
  if(basic)
  {
    XString enable = p_msg.GetAttribute(basic,"enabled");
    if(enable.CompareNoCase("true") == 0)
    {
      p_site.m_authScheme |= HTTP_AUTH_ENABLE_BASIC;

      XString realm = p_msg.GetAttribute(basic,"realm");
      if(!realm.IsEmpty())
      {
        p_site.m_realm = realm;
      }
      XString domain = p_msg.GetAttribute(basic,"defaultLogonDomain");
      if(!domain.IsEmpty())
      {
        p_site.m_domain = domain;
      }
    }
  }

  // Check Digest authentication
  XMLElement* digest = p_msg.FindElement(auth,"digestAuthentication");
  if(digest)
  {
    XString enable = p_msg.GetAttribute(digest,"enabled");
    if(enable.CompareNoCase("true") == 0)
    {
      p_site.m_authScheme |= HTTP_AUTH_ENABLE_DIGEST;

      XString realm = p_msg.GetAttribute(digest,"realm");
      if(!realm.IsEmpty())
      {
        p_site.m_realm = realm;
      }
    }
  }

  // Check Windows Authentication
  XMLElement* windows = p_msg.FindElement(auth,"windowsAuthentication");
  if(windows)
  {
    XString enable = p_msg.GetAttribute(windows,"enabled");
    if(enable.CompareNoCase("true") == 0)
    {
      // See if we must de-activate NTLM caching
      XString persist = p_msg.GetAttribute(windows,"authPersistSingleRequest");
      if(persist.CompareNoCase("true") == 0)
      {
        p_site.m_ntlmCache = false;
      }

      // Find providers
      XMLElement* providers = p_msg.FindElement(windows,"providers");
      if(providers)
      {
        XMLElement* add = p_msg.FindElement(providers,"add");
        while(add)
        {
          XString value = p_msg.GetAttribute(add,"value");
          if(value.CompareNoCase("NTLM") == 0)
          {
            p_site.m_authScheme |= HTTP_AUTH_ENABLE_NTLM;
          }
          if(value.CompareNoCase("Negotiate") == 0)
          {
            p_site.m_authScheme |= HTTP_AUTH_ENABLE_NEGOTIATE;
          }
          if(value.CompareNoCase("Negotiate:Kerberos") == 0)
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
  XMLElement* location = p_msg.FindElement("location");

  while(location)
  {
    XString path = p_msg.GetAttribute(location, "path");
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
  XMLElement* sws = p_msg.FindElement(p_elem,"system.webServer");
  if (!sws) return;
  XMLElement* handlers = p_msg.FindElement(sws,"handlers");
  if (!handlers) return;
  XMLElement* add = p_msg.FindElement(handlers,"add");
  while (add)
  {
    IISHandler handler;

    XString name           = p_msg.GetAttribute(add,"name");
    handler.m_path         = p_msg.GetAttribute(add,"path");
    handler.m_verb         = p_msg.GetAttribute(add,"verb");
    handler.m_modules      = p_msg.GetAttribute(add,"modules");
    handler.m_resourceType = p_msg.GetAttribute(add,"resourceType");
    handler.m_precondition = p_msg.GetAttribute(add,"preCondition");

    p_site.m_handlers.insert(std::make_pair(name,handler));

    // Next handler mapping
    add = p_msg.GetElementSibling(add);
  }
}

IISHandlers*
WebConfigIIS::GetAllHandlers(XString p_site)
{
  IISSite* site = GetSite(p_site);
  if(site)
  {
    return &(site->m_handlers);
  }
  return nullptr;
}

IISHandler*
WebConfigIIS::GetHandler(XString p_site,XString p_handler)
{
  IISSite* site = GetSite(p_site);
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

// Finding a site registration
// Site/Subsite -> Finds "site"
// Site         -> FInds "site"
IISSite*
WebConfigIIS::GetSite(XString p_site)
{
  // Registration is in lower case
  p_site.MakeLower();
  p_site.Trim('/');

  // Knock off the sub sites
  int pos = p_site.Find('/');
  if(pos > 0)
  {
    p_site = p_site.Left(pos);
  }

  // Find the site structure
  IISSites::iterator it = m_sites.find(p_site);
  if(it != m_sites.end())
  {
    return &it->second;
  }
  return nullptr;
}

// Finding a application pool registration
IISAppPool* 
WebConfigIIS::GetPool(XString p_pool)
{
  p_pool.MakeLower();
  IISPools::iterator it = m_pools.find(p_pool);
  if(it != m_pools.end())
  {
    return &(it->second);
  }
  return nullptr;
}
