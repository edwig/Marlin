/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: HTTPSiteMarlin.cpp
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
#include "AutoCritical.h"
#include "HTTPSiteMarlin.h"
#include "HTTPServerMarlin.h"
#include "HTTPURLGroup.h"
#include <WinFile.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// Logging via the server
#define DETAILLOG1(text)        m_server->DetailLog (_T(__FUNCTION__),LogType::LOG_INFO,text)
#define DETAILLOGS(text,extra)  m_server->DetailLogS(_T(__FUNCTION__),LogType::LOG_INFO,text,extra)
#define DETAILLOGV(text,...)    m_server->DetailLogV(_T(__FUNCTION__),LogType::LOG_INFO,text,__VA_ARGS__)
#define WARNINGLOG(text,...)    m_server->DetailLogV(_T(__FUNCTION__),LogType::LOG_WARN,text,__VA_ARGS__)
#define ERRORLOG(code,text)     m_server->ErrorLog  (_T(__FUNCTION__),code,text)
#define CRASHLOG(code,text)     if(m_server->GetLogfile())\
                                {\
                                  m_server->GetLogfile()->AnalysisLog(_T(__FUNCTION__),LogType::LOG_ERROR,false,text); \
                                  m_server->GetLogfile()->AnalysisLog(_T(__FUNCTION__),LogType::LOG_ERROR,true,_T("Error code: %d"),code); \
                                }

HTTPSiteMarlin::HTTPSiteMarlin(HTTPServerMarlin* p_server
                              ,int               p_port
                              ,XString           p_site
                              ,XString           p_prefix
                              ,HTTPSite*         p_mainSite /*=nullptr*/
                              ,LPFN_CALLBACK     p_callback /*=nullptr*/)
  :HTTPSite(p_server,p_port,p_site,p_prefix,p_mainSite,p_callback)
{
}

bool
HTTPSiteMarlin::StartSite()
{
  bool result = false;
  DETAILLOGS(_T("Starting website. URL: "),m_site);

  // Getting the global settings
  InitSite(m_server->GetWebConfig());

  // If we have a site Marlin.config file: read it
  // Overrides the programmatical settings between HTTPServer::CreateSite and HTTPSite::StartSite
  XString siteConfigFile = MarlinConfig::GetSiteConfig(m_prefixURL);
  if(!siteConfigFile.IsEmpty())
  {
    MarlinConfig config(siteConfigFile);
    if(config.IsFilled())
    {
      InitSite(config);
    }
  }

  // Now log the settings, once we read all Marlin.config files
  LogSettings();

  // See if we have a reliable messaging WITH authentication
  CheckReliable();

  // Checking our webroot
  if(!SetWebroot(m_webroot))
  {
    ERRORLOG(ERROR_INVALID_NAME,_T("Cannot start site: invalid webroot"));
    return false;
  }

  // Call all filters 'OnStartSite' methods
  for(auto& filter : m_filters)
  {
    filter.second->OnStartSite();
  }

  // Call all site handlers 'OnStartSite' methods
  for(auto& handler : m_handlers)
  {
    SiteHandler* shand = handler.second.m_handler;
    while(shand)
    {
      shand->OnStartSite();
      shand = shand->GetNextHandler();
    }
  }

  // Lock for the initialization of the site
  AutoCritSec lock(m_server->AcquireSitesLockObject());

  // Find our server type
  HTTPServerMarlin* server = reinterpret_cast<HTTPServerMarlin*>(m_server);

  // Start a group for our site, and record the group
  m_group = server->FindUrlGroup(m_scheme,m_authScheme,m_ntlmCache,m_realm,m_domain);
  if(m_group)
  {
    // Register site in the group
    m_group->RegisterSite(this);

    // The registration
    if(m_mainSite)
    {
      DETAILLOGV(_T("URL not registered to URL-Group. Site [%s] is subsite of [%s]"),m_site.GetString(),m_mainSite->GetSite().GetString());
      result = true;
    }
    else
    {
      // Add URL to our URL-Group: using HttpAddUrlToUrlGroup
      wstring uniURL = StringToWString(m_prefixURL);
      HTTP_URL_GROUP_ID group = m_group->GetUrlGroupID();
      ULONG retCode = HttpAddUrlToUrlGroup(group,uniURL.c_str(),(HTTP_URL_CONTEXT)this,0);
      if(retCode != NO_ERROR && retCode != ERROR_ALREADY_EXISTS)
      {
        XString error;
        error.Format(_T("Cannot add URL to the URL-Group: %s"),m_prefixURL.GetString());
        ERRORLOG(retCode,error);
      }
      else
      {
        DETAILLOGS(_T("URL added to URL-Group: "),m_prefixURL);
        result = true;
      }
    }
  }
  else
  {
    ERRORLOG(ERROR_INVALID_PARAMETER,_T("No URL group created/found for authentication scheme: ") + m_scheme);
  }
  // Return the fact that we started successfully or not
  return (m_isStarted = result);
}

bool
HTTPSiteMarlin::SetWebroot(XString p_webroot)
{
  // Cleaning the webroot is simple
  if(p_webroot.IsEmpty())
  {
    m_webroot = m_server->GetWebroot();
    return true;
  }

  // Check the directory
  XString siteWebroot;
  if(m_virtualDirectory)
  {
    // Check already done by setting virtual directory
    return true;
  }
  else if(p_webroot.Left(10).CompareNoCase(_T("virtual://")) == 0)
  {
    // It's a HTTP virtual directory outside of the server's webroot
    siteWebroot = p_webroot.Mid(10);
    m_virtualDirectory = true;
  }
  else
  {
    siteWebroot = p_webroot;

    // Checking the webroot of the site against the webroot of the server
    XString serverWebroot = m_server->GetWebroot();
    serverWebroot.MakeLower();
    siteWebroot  .MakeLower();
    serverWebroot.Replace(_T("/"),_T("\\"));
    siteWebroot  .Replace(_T("/"),_T("\\"));

    if(siteWebroot.Find(serverWebroot) != 0)
    {
      XString error;
      ERRORLOG(ERROR_INVALID_PARAMETER,error);
      return false;
    }
  }

  // Make sure the directory is there
  WinFile ensure(siteWebroot);
  if(!ensure.CreateDirectory())
  {
    ERRORLOG(ensure.GetLastError(),_T("Website's root directory does not exist and cannot create it"));
  }

  // Acceptable webroot of a site
  m_webroot = siteWebroot;
  return true;
}

void
HTTPSiteMarlin::InitSite(MarlinConfig& p_config)
{
  // Call our main class InitSite
  HTTPSite::InitSite(p_config);

  // AUTHENTICATION
  // Setup the authentication of the URL group
  XString scheme    = p_config.GetParameterString (_T("Authentication"),_T("Scheme"),   m_scheme);
  bool    ntlmCache = p_config.GetParameterBoolean(_T("Authentication"),_T("NTLMCache"),m_ntlmCache);
  XString realm     = p_config.GetParameterString (_T("Authentication"),_T("Realm"),    m_realm);
  XString domain    = p_config.GetParameterString (_T("Authentication"),_T("Domain"),   m_domain);

  // CHECK AND FIND the Authentication scheme
  ULONG authScheme = 0;
  if(!scheme.IsEmpty())
  {
    // Detect authentication scheme
         if(scheme == _T("Anonymous")) authScheme = 0;
    else if(scheme == _T("Basic"))     authScheme = HTTP_AUTH_ENABLE_BASIC;
    else if(scheme == _T("Digest"))    authScheme = HTTP_AUTH_ENABLE_DIGEST;
    else if(scheme == _T("NTLM"))      authScheme = HTTP_AUTH_ENABLE_NTLM;
    else if(scheme == _T("Negotiate")) authScheme = HTTP_AUTH_ENABLE_NEGOTIATE;
    else if(scheme == _T("Kerberos"))  authScheme = HTTP_AUTH_ENABLE_KERBEROS | HTTP_AUTH_ENABLE_NEGOTIATE;
    else if(scheme != _T(""))
    {
      ERRORLOG(ERROR_INVALID_PARAMETER,_T("Invalid parameter for authentication scheme"));
    }
  }

  // Remember the authentication scheme
  m_authScheme = authScheme;
  m_scheme     = scheme;
  m_realm      = realm;
  m_domain     = domain;
  m_ntlmCache  = ntlmCache;
}
