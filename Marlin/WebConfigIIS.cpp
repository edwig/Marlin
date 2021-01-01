/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: WebConfigIIS.cpp
//
// Marlin Server: Internet server/client
// 
// Copyright (c) 2014-2021 ir. W.E. Huisman
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

WebConfigIIS::WebConfigIIS(CString p_application /*=""*/)
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
WebConfigIIS::ReadConfig(CString p_baseWebConfig)
{
  // Standard web.config file
  m_webconfig = p_baseWebConfig;

  // Reads the central IIS application host configuration file first
  // this file contains the defaults for IIS.
  bool result = ReadConfig("%windir%\\system32\\inetsrv\\config\\ApplicationHost.Config",nullptr);
  SetApplication(m_application);
  return result;
}

void
WebConfigIIS::SetApplication(CString p_application)
{
  // Try reading applications web.config
  if(p_application.IsEmpty())
  {
    ReadConfig(m_webconfig,nullptr);
  }
  else
  {
    IISSite* site = GetSite(p_application);
    if(site)
    {
      // Get virtual directory: can be outside "inetpub\wwwroot"
      CString config = site->m_path + "\\Web.Config";
      // Read the web.config of the application site, if any requested
      ReadConfig(config,site);
    }
  }
  // Store the application
  if(m_application.IsEmpty())
  {
    m_application = p_application;
  }
}

CString
WebConfigIIS::GetSiteName(CString p_site)
{
  IISSite* site = GetSite(p_site);
  if (site)
  {
    return site->m_name;
  }
  return "";
}

CString
WebConfigIIS::GetSetting(CString p_key)
{
  return m_settings[p_key];
}

CString
WebConfigIIS::GetSiteBinding(CString p_site,CString p_default)
{
  IISSite* site = GetSite(p_site);
  if(site)
  {
    return site->m_binding;
  }
  return p_default;
}

CString
WebConfigIIS::GetSiteProtocol(CString p_site,CString p_default)
{
  IISSite* site = GetSite(p_site);
  if(site)
  {
    return site->m_protocol;
  }
  return p_default;
}

int
WebConfigIIS::GetSitePort(CString p_site,int p_default)
{
  IISSite* site = GetSite(p_site);
  if(site)
  {
    return site->m_port;
  }
  return p_default;
}

bool
WebConfigIIS::GetSiteSecure(CString p_site,bool p_default)
{
  IISSite* site = GetSite(p_site);
  if(site)
  {
    return site->m_secure;
  }
  return p_default;
}

CString
WebConfigIIS::GetSiteWebroot(CString p_site,CString p_default)
{
  IISSite* site = GetSite(p_site);
  if(site)
  {
    return site->m_path;
  }
  return p_default;
}

CString
WebConfigIIS::GetSitePathname(CString p_site,CString p_default)
{
  IISSite* site = GetSite(p_site);
  if(site)
  {
    return site->m_path;
  }
  return p_default;
}

ULONG
WebConfigIIS::GetSiteScheme(CString p_site,ULONG   p_default)
{
  IISSite* site = GetSite(p_site);
  if(site)
  {
    return site->m_authScheme;
  }
  return p_default;
}

CString
WebConfigIIS::GetSiteRealm(CString p_site,CString p_default)
{
  IISSite* site = GetSite(p_site);
  if(site)
  {
    return site->m_realm;
  }
  return p_default;
}

CString
WebConfigIIS::GetSiteDomain(CString p_site,CString p_default)
{
  IISSite* site = GetSite(p_site);
  if(site)
  {
    return site->m_domain;
  }
  return p_default;
}

bool
WebConfigIIS::GetSiteNTLMCache(CString p_site,bool p_default)
{
  IISSite* site = GetSite(p_site);
  if(site)
  {
    return site->m_ntlmCache;
  }
  return p_default;
}

IISError
WebConfigIIS::GetSiteError(CString p_site)
{
  IISSite* site = GetSite(p_site);
  if (site)
  {
    return site->m_error;
  }
  return IISER_NoError;
}

//////////////////////////////////////////////////////////////////////////
//
// PRIVATE METHODS
//
//////////////////////////////////////////////////////////////////////////

// Replace environment variables in a string
bool 
WebConfigIIS::ReplaceEnvironVars(CString& p_string)
{
  bool replaced = false;
  CString local(p_string);

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
    CString var = local.Mid(beginPos + 1, (endPos - beginPos - 1));
    CString value;
    value.GetEnvironmentVariable(var);
    if(!value.IsEmpty())
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
WebConfigIIS::ReadConfig(CString p_configFile,IISSite* p_site /*=nullptr*/)
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
  if(it != m_files.end())
  {
    return true;
  }

  // Parse the incoming file
  XMLMessage msg;
  msg.LoadFile(p_configFile);

  ReadLogPath(msg);
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
      CString enabled   = p_msg.GetAttribute(central,"enabled");
      CString directory = p_msg.GetAttribute(central,"directory");

      // Process the directory
      ReplaceEnvironVars(directory);

      // Remember this
      m_logging = enabled.CompareNoCase("true") == 0;
      m_logpath = directory;
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
      theSite.m_authScheme = 0;
      theSite.m_error      = IISER_NoError;
      CString name = p_msg.GetAttribute(site,"name");
      theSite.m_name = name;

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
      CString key   = p_msg.GetAttribute(add,"key");
      CString value = p_msg.GetAttribute(add,"value");

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
      CString max = p_msg.GetAttribute(limit,"maxAllowedContentLength");
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
    CString path = p_msg.GetAttribute(location,"path");
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
    CString enable = p_msg.GetAttribute(anonymous,"enabled");
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
    CString enable = p_msg.GetAttribute(basic,"enabled");
    if(enable.CompareNoCase("true") == 0)
    {
      p_site.m_authScheme |= HTTP_AUTH_ENABLE_BASIC;

      CString realm = p_msg.GetAttribute(basic,"realm");
      if(!realm.IsEmpty())
      {
        p_site.m_realm = realm;
      }
      CString domain = p_msg.GetAttribute(basic,"defaultLogonDomain");
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
    CString enable = p_msg.GetAttribute(digest,"enabled");
    if(enable.CompareNoCase("true") == 0)
    {
      p_site.m_authScheme |= HTTP_AUTH_ENABLE_DIGEST;

      CString realm = p_msg.GetAttribute(digest,"realm");
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
    CString enable = p_msg.GetAttribute(windows,"enabled");
    if(enable.CompareNoCase("true") == 0)
    {
      // See if we must de-activate NTLM caching
      CString persist = p_msg.GetAttribute(windows,"authPersistSingleRequest");
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
          CString value = p_msg.GetAttribute(add,"value");
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
    CString path = p_msg.GetAttribute(location, "path");
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

    CString name           = p_msg.GetAttribute(add,"name");
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
WebConfigIIS::GetAllHandlers(CString p_site)
{
  IISSite* site = GetSite(p_site);
  if(site)
  {
    return &(site->m_handlers);
  }
  return nullptr;
}

IISHandler*
WebConfigIIS::GetHandler(CString p_site,CString p_handler)
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
WebConfigIIS::GetSite(CString p_site)
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
