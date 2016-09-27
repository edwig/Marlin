/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: WebConfigIIS.h
//
// Marlin Server: Internet server/client
// 
// Copyright (c) 2016 ir. W.E. Huisman
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
#include <map>

// What we want to remember about an IIS HTTP site
typedef struct _iisSite
{
  CString m_name;
  CString m_binding;
  CString m_protocol;
  int     m_port;
  bool    m_secure;
  CString m_path;
  ULONG   m_authScheme;
  CString m_realm;
  CString m_domain;
  bool    m_ntlmCache;
}
IISSite;

using IISSites = std::map<CString,IISSite>;

class WebConfigIIS
{
public:
  WebConfigIIS(CString p_application = "");
 ~WebConfigIIS();

  // Read total config
  bool ReadConfig();
  // Set a different application before re-reading the config
  void SetApplication(CString p_app);

  // GETTERS
  
  // Getting information of the server
  CString     GetLogfilePath()    { return m_logpath;         };
  bool        GetDoLogging()      { return m_logging;         };
  ULONG       GetStreamingLimit() { return m_streamingLimit;  };

  // Getting information of a site
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

private:
  // Read one config file
  bool        ReadConfig(CString p_configFile,IISSite* p_site = nullptr);
  // Replace environment variables in a string
  static bool ReplaceEnvironVars(CString& p_string);
  // Site registration
  IISSite*    GetSite(CString p_site);
  // Reading of the internal structures of a config file
  void        ReadLogPath(XMLMessage& p_msg);
  void        ReadSites  (XMLMessage& p_msg);
  void        ReadStreamingLimit(IISSite& p_site,XMLMessage& p_msg,XMLElement* p_elem);
  void        ReadAuthentication(IISSite& p_site,XMLMessage& p_msg,XMLElement* p_elem);
  void        ReadAuthentication(IISSite& p_site,XMLMessage& p_msg);

  // For specific web application, or juist defaults
  CString   m_application;

  IISSites  m_sites;
  CString   m_logpath;
  bool      m_logging { false };
  ULONG     m_streamingLimit;
};
