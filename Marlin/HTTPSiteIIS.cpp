/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: HTTPSiteIIS.cpp
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
#include <IPTypes.h>//MAX_DOMAIN_NAME_LEN
#include "HTTPSiteIIS.h"
#include "HTTPServerIIS.h"
#include "HTTPURLGroup.h"
#include "WebConfigIIS.h"
#include "GetUserAccount.h"
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

HTTPSiteIIS::HTTPSiteIIS(HTTPServerIIS* p_server
                        ,int            p_port
                        ,XString        p_site
                        ,XString        p_prefix
                        ,HTTPSite*      p_mainSite /*=nullptr*/
                        ,LPFN_CALLBACK  p_callback /*=nullptr*/)
            :HTTPSite(p_server,p_port,p_site,p_prefix,p_mainSite,p_callback)
{
}

void
HTTPSiteIIS::InitSite()
{
  WebConfigIIS* config = reinterpret_cast<HTTPServerIIS*>(m_server)->GetWebConfigIIS();

  // Read for this site
  config->SetApplication(m_site);

  // Getting the port settings from IIS
  g_streaming_limit = config->GetStreamingLimit();
  m_port            = config->GetSitePort     (m_site,m_port);
  m_ntlmCache       = config->GetSiteNTLMCache(m_site,m_ntlmCache);
  m_realm           = config->GetSiteRealm    (m_site,m_realm);
  m_domain          = config->GetSiteDomain   (m_site,m_domain);
  m_authScheme      = config->GetSiteScheme   (m_site,m_authScheme);

  // Authentication scheme
  m_scheme.Empty();
  if(m_authScheme & HTTP_AUTH_ENABLE_BASIC)     m_scheme += _T("Basic/");
  if(m_authScheme & HTTP_AUTH_ENABLE_DIGEST)    m_scheme += _T("Digest/");
  if(m_authScheme & HTTP_AUTH_ENABLE_NTLM)      m_scheme += _T("NTLM/");
  if(m_authScheme & HTTP_AUTH_ENABLE_NEGOTIATE) m_scheme += _T("Negotiate/");
  if(m_authScheme & HTTP_AUTH_ENABLE_KERBEROS)  m_scheme += _T("Kerberos/");
  if(m_authScheme == 0)                         m_scheme += _T("Anonymous/");
  m_scheme = m_scheme.TrimRight('/');

  // Call our main class InitSite
  HTTPSite::InitSite(m_server->GetWebConfig());
}

bool
HTTPSiteIIS::StartSite()
{
  DETAILLOGS(_T("Starting website. URL: "),m_site);

  // Getting the global settings
  InitSite();
 
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

  // Register the site with an URL group for the stand-alone server
  DETAILLOGS(_T("Site started for the IIS server: "),m_site);
  // Return the fact that we started successfully or not
  m_isStarted = true;
  return true;
}

bool
HTTPSiteIIS::SetWebroot(XString p_webroot)
{
  UNREFERENCED_PARAMETER(p_webroot);

  // Getting the IIS server root
  XString root = m_server->GetWebroot();

  // IIS now expects you to add the site name
  m_webroot = root + GetIISSiteDir();

  // Make sure the directory is there
  WinFile ensure(m_webroot);
  if(!ensure.CreateDirectory())
  {
    ERRORLOG(ensure.GetLastError(),_T("Website's root directory does not exist and cannot create it"));
    return false;
  }
  return true;
}

// Getting the sites directory within the IIS rootdir
// /Site/         -> "\\Site\\"
// /Site/Subsite/ -> "\\Site\\"
XString 
HTTPSiteIIS::GetIISSiteDir()
{
  // Follow the main site chain to the site
  HTTPSite* mainsite = this;
  while(mainsite->GetIsSubsite())
  {
    mainsite = mainsite->GetMainSite();
  }

  // This is the mainsite
  XString dir = mainsite->GetSite();

  // Transpose site URL to directory name
  int pos1 = dir.Find(_T('/'),1);
  if(pos1 < dir.GetLength() - 1)
  {
    dir = dir.Left(pos1 + 1);
  }
  dir.Replace(_T("/"),_T("\\"));
  return dir;
}

// IIS servers always have a token
bool
HTTPSiteIIS::GetHasAnonymousAuthentication(HANDLE p_token)
{
  // If no token: always anonymous
  if(p_token == NULL)
  {
    return true;
  }

  // Getting the size of the TOKEN_OWNER block
  DWORD size = 0;
  GetTokenInformation(p_token,TokenOwner,NULL,0,&size);
  if(!size)
  {
    XString text;
    text.Format(_T("Error getting token owner: error code0x%lx\n"), GetLastError());
    ERRORLOG(ENOMEM,text);
    return false;
  }

  // Get owner information
  TOKEN_OWNER* owner = reinterpret_cast<TOKEN_OWNER *>(new uchar[size]);
  GetTokenInformation(p_token,TokenOwner,owner,size,&size);
  if(owner == nullptr)
  {
    ERRORLOG(EACCES,_T("Error getting token information of logged user"));
    return false;
  }

  // Get a copy of the SID
  size = GetLengthSid(owner->Owner);
  SID* sid = reinterpret_cast<SID *>(new uchar[size]);
  CopySid(size,sid,owner->Owner);

  TCHAR userName  [MAX_USER_NAME];
  TCHAR domainName[MAX_DOMAIN_NAME_LEN];

  DWORD userSize   = (sizeof userName   / sizeof *userName) - 1;
  DWORD domainSize = (sizeof domainName / sizeof *domainName) - 1;
  SID_NAME_USE  sidType = SidTypeUser;

  LookupAccountSid(NULL,sid,userName,&userSize,domainName,&domainSize,&sidType);
  delete[] sid;

  // Test for "NT AUTHORITY\IUSR" code of IIS
  if((_tcsncicmp(domainName,_T("NT AUTHORITY"), MAX_DOMAIN_NAME_LEN) == 0) &&
     (_tcsncicmp(userName,  _T("IUSR"),         MAX_USER_NAME)       == 0))
  {
    return true;
  }
  return false;
}

