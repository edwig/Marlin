/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: HTTPURLGroup.cpp
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
#include "HTTPURLGroup.h"
#include "HTTPSite.h"
#include "LogAnalysis.h"

#ifdef _AFX
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#endif

// Logging via the server
#define DETAILLOG(text,...)       if(m_server->GetLogfile()) m_server->GetLogfile()->AnalysisLog(_T(__FUNCTION__),LogType::LOG_INFO,true,text,__VA_ARGS__)
#define ERRORLOG(code,text)       if(m_server->GetLogfile())\
                                  {\
                                    m_server->GetLogfile()->AnalysisLog(_T(__FUNCTION__),LogType::LOG_ERROR,false,text);\
                                    m_server->GetLogfile()->AnalysisLog(_T(__FUNCTION__),LogType::LOG_ERROR,true, _T("Error code: %d"),code);\
                                  }

HTTPURLGroup::HTTPURLGroup(HTTPServerMarlin* p_server
                          ,XString p_authName
                          ,ULONG   p_authScheme
                          ,bool    p_cache
                          ,XString p_realm
                          ,XString p_domain)
             :m_server(p_server)
             ,m_authName(p_authName)
             ,m_authScheme(p_authScheme)
             ,m_ntlmCache(p_cache)
             ,m_realm(p_realm)
             ,m_domain(p_domain)
{
}

HTTPURLGroup::~HTTPURLGroup()
{
  StopGroup();
}

bool
HTTPURLGroup::StartGroup()
{
  // Already started
  if(m_isStarted)
  {
    return true;
  }

  // STEP 4: CREATE URL GROUP / REQUEST QUEUE AND SETTINGS
  // Create our URL group with "HttpCreateUrlGroup"
  ULONG retCode = HttpCreateUrlGroup(m_server->GetServerSessionID(),&m_group,0);
  if(retCode != NO_ERROR)
  {
    ERRORLOG(retCode,_T("CreateUrlGroup"));
    return false;
  }
  DETAILLOG(_T("URL-Group ID created: %I64X"),m_group);


  // Bind URLGROUP to request queue
  HTTP_BINDING_INFO binding;
  binding.Flags.Present      = 1;
  binding.RequestQueueHandle = m_server->GetRequestQueue();

  // Using: HttpSetUrlGroupProperty
  retCode = HttpSetUrlGroupProperty(m_group
                                   ,HttpServerBindingProperty
                                   ,&binding
                                   ,sizeof(HTTP_BINDING_INFO));
  if(retCode != NO_ERROR)
  {
    ERRORLOG(retCode,_T("Bind UrlGroup to request queue"));
    return false;
  }
  DETAILLOG(_T("Bound request queue to URL-Group"));

  // Get realm and domain
  wstring wrealm  = StringToWString(m_realm);
  wstring wdomain = StringToWString(m_domain);

  HTTP_SERVER_AUTHENTICATION_INFO authInfo;
  memset(&authInfo,0,sizeof(authInfo));
  authInfo.Flags.Present        = 1;
  authInfo.ExFlags              = (m_authScheme & HTTP_AUTH_ENABLE_KERBEROS) ? HTTP_AUTH_EX_FLAG_ENABLE_KERBEROS_CREDENTIAL_CACHING : 0;
  authInfo.AuthSchemes          = m_authScheme;
  authInfo.ReceiveMutualAuth    = true;
  authInfo.ReceiveContextHandle = true;
  authInfo.DisableNTLMCredentialCaching = !m_ntlmCache;
  if(m_authScheme & HTTP_AUTH_ENABLE_DIGEST)
  {
    authInfo.DigestParams.DomainName       = reinterpret_cast<PWSTR>(const_cast<wchar_t*>(wdomain.c_str()));
    authInfo.DigestParams.DomainNameLength = static_cast<USHORT>(wdomain.size() * 2);
    authInfo.DigestParams.Realm            = reinterpret_cast<PWSTR>(const_cast<wchar_t*>(wrealm .c_str()));
    authInfo.DigestParams.RealmLength      = static_cast<USHORT>(wrealm .size() * 2);
  }
  if(m_authScheme & HTTP_AUTH_ENABLE_BASIC)
  {
    authInfo.BasicParams.Realm       = reinterpret_cast<PWSTR>(const_cast<wchar_t*>(wrealm.c_str()));
    authInfo.BasicParams.RealmLength = static_cast<USHORT>(wrealm.size() * 2);
  }

  // See which we must use
  HTTP_SERVER_PROPERTY authenticate = HttpServerAuthenticationProperty;
  if(m_authScheme & HTTP_AUTH_ENABLE_KERBEROS)
  {
    authenticate = HttpServerExtendedAuthenticationProperty;
  }

  // Now go set the authentication scheme: using HttpSetUrlGroupProperty
  retCode = HttpSetUrlGroupProperty(m_group,authenticate,&authInfo,sizeof(authInfo));
  if(retCode != NO_ERROR)
  {
    ERRORLOG(retCode,_T("Cannot set the authentication properties"));
    return false;
  }
  DETAILLOG(_T("Authentication scheme set to: %s"),m_authName.GetString());
  if(m_authScheme & HTTP_AUTH_ENABLE_NTLM)
  {
    DETAILLOG(_T("Authentication NTLM cache is: %s"),m_ntlmCache ? _T("on") : _T("off"));
  }
  if(m_authScheme && (HTTP_AUTH_ENABLE_DIGEST | HTTP_AUTH_ENABLE_BASIC))
  {
    DETAILLOG(_T("Authentication realm is     : %s"),m_realm.GetString());
  }
  if(m_authScheme && HTTP_AUTH_ENABLE_DIGEST)
  {
    DETAILLOG(_T("Authentication domain is    : %s"),m_domain.GetString());
  }

  // Reached the end.
  return (m_isStarted = true);
}

void
HTTPURLGroup::StopGroup()
{
  // Close URL group
  if(m_group)
  {
    // If not all sites stopped, stop them now
    // as the group can no longer service the URL's
    while(!m_sites.empty())
    {
      UrlSiteMap::iterator it = m_sites.begin();
      HTTPSite* site = it->second;
      site->StopSite(true);
    }
    // Remove from server registration and driver
    m_server->RemoveURLGroup(this);

    // Reset the group
    m_group     = NULL;
    m_isStarted = false;
  }
}

void
HTTPURLGroup::RegisterSite(HTTPSite* p_site)
{
  XString site = p_site->GetPrefixURL();
  UrlSiteMap::iterator it = m_sites.find(site);
  if(it != m_sites.end())
  {
    // Already registered
    ERRORLOG(ERROR_ALREADY_ASSIGNED,_T("Cannot register site in URL-Group: ") + p_site->GetSite());
    return;
  }
  // Register this site
  m_sites[site] = p_site;
}

void
HTTPURLGroup::UnRegisterSite(HTTPSite* p_site)
{
  XString site = p_site->GetPrefixURL();
  UrlSiteMap::iterator it = m_sites.find(site);
  if(it != m_sites.end())
  {
    m_sites.erase(it);
  }

  // After all sites are removed, clean up the whole group
//   if(m_sites.empty() && m_group)
//   {
//     // Remove from server registration and driver
//     m_server->RemoveURLGroup(this);
//     // Deleting the group will call StopGroup()
//     delete this;
//   }
}
