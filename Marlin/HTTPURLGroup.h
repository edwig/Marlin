/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: HTTPURLGroup.h
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
//////////////////////////////////////////////////////////////////////////
//
// Een URLGroup wordt gekenmerkt door dezelfde authenticatie en settings
// binnen een (1) en dezelfde request queue van de HTTP Server
//
//////////////////////////////////////////////////////////////////////////

#pragma once
#include "HTTPServerMarlin.h"
#include <map>

class HTTPSite;

using UrlSiteMap = std::map<CString,HTTPSite*>;

class HTTPURLGroup
{
public:
  HTTPURLGroup(HTTPServerMarlin* p_server
              ,CString p_authName
              ,ULONG   p_authScheme
              ,bool    p_cache
              ,CString p_realm
              ,CString p_domain);
 ~HTTPURLGroup();

  // Starting a new URL Group
  bool  StartGroup();
  // Stopping a URL group
  void  StopGroup();
  // Registering a site in this group
  void  RegisterSite  (HTTPSite* p_site);
  // Unregistering a site from the group
  void  UnRegisterSite(HTTPSite* p_site);

  // GETTERS

  HTTPServerMarlin* GetHTTPServer()              { return m_server;     };
  bool              GetIsStarted()               { return m_isStarted;  };
  HTTP_URL_GROUP_ID GetUrlGroupID()              { return m_group;      };
  CString           GetAuthenticationName()      { return m_authName;   };
  ULONG             GetAuthenticationScheme()    { return m_authScheme; };
  bool              GetAuthenticationNtlmCache() { return m_ntlmCache;  };
  CString           GetAuthenticationRealm()     { return m_realm;      };
  CString           GetAuthenticationDomain()    { return m_domain;     };
  unsigned          GetNumberOfSites();

private:
  HTTPServerMarlin* m_server      { nullptr };        // Server of the URL group
  HTTP_URL_GROUP_ID m_group       { NULL    };        // URL Group within Windows OS
  bool              m_isStarted   { false   };        // Group is started
  // Authentication
  CString           m_authName;                       // Authentication scheme name
  ULONG             m_authScheme  { 0 };              // Authentication scheme
  bool              m_ntlmCache   { true };           // Authentication NTLM Cache 
  CString           m_realm;                          // Authentication realm
  CString           m_domain;                         // Authentication domain
  // All sites
  UrlSiteMap        m_sites;                          // All live sites in the URL-Group
};

inline unsigned
HTTPURLGroup::GetNumberOfSites()
{
  return (unsigned) m_sites.size();
}
