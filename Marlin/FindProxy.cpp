/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: FindProxy.cpp
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
#include "FindProxy.h"
#include <ConvertWideString.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

FindProxy::FindProxy()
{
  m_perDest = true;
  m_info    = nullptr;
}

FindProxy::~FindProxy()
{
  if(m_info)
  {
    delete m_info;
    m_info = nullptr;
  }
}

XString
FindProxy::Find(const XString& p_url,bool p_secure)
{
  // If already initialized
  if(!m_lastUsedDst.IsEmpty() && (m_lastUsedDst == p_url))
  {
    //and no need to init again
    return m_proxy;
  }
  //get proxy configuration for current user using WinHttpGetIEProxyConfigForCurrentUser [ + WinHttpGetProxyForUrl ]
  m_proxy.Empty();
  m_ignored = "";
  m_perDest = false;
  XString proxy;

  ProxyConfig proxyCfg = { 0 };

  WINHTTP_CURRENT_USER_IE_PROXY_CONFIG &cfg = proxyCfg.cfg;
  if(::WinHttpGetIEProxyConfigForCurrentUser(&cfg))
  {
    if(cfg.lpszProxy)
    {
      proxy = WStringToString(cfg.lpszProxy);
      if(!!cfg.lpszProxyBypass)
      {
        m_ignored = WStringToString(cfg.lpszProxyBypass);
      }
    }
    LPWSTR autoCfgUrl = cfg.lpszAutoConfigUrl;
    if(proxy.IsEmpty() && (cfg.fAutoDetect || !!autoCfgUrl))
    {
      //check for autoproxy settings
      WINHTTP_AUTOPROXY_OPTIONS autoOpts = { 0, };
      autoOpts.fAutoLogonIfChallenged = TRUE;
      if(cfg.fAutoDetect)
      {
        autoOpts.dwAutoDetectFlags = WINHTTP_AUTO_DETECT_TYPE_DHCP | WINHTTP_AUTO_DETECT_TYPE_DNS_A;
        autoOpts.dwFlags = WINHTTP_AUTOPROXY_AUTO_DETECT;
      }
      if(autoCfgUrl)
      {
        autoOpts.lpszAutoConfigUrl = autoCfgUrl;
        autoOpts.dwFlags |= WINHTTP_AUTOPROXY_CONFIG_URL;
      }
      Internet internet   = { ::WinHttpOpen(L"", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0) };
      if(m_info)
      {
        delete m_info;
      }
      m_info = new ProxyInfo();

      WINHTTP_PROXY_INFO &autoCfg = m_info->cfg;
      if(internet.hInter && ::WinHttpGetProxyForUrl(internet.hInter, StringToWString(p_url).c_str(),&autoOpts,&autoCfg))
      {
        if(autoCfg.lpszProxy && autoCfg.dwAccessType != WINHTTP_ACCESS_TYPE_NO_PROXY)
        {
          m_perDest = true;
          proxy = WStringToString(autoCfg.lpszProxy);
          if(!!autoCfg.lpszProxyBypass)
          {
            m_ignored = WStringToString(autoCfg.lpszProxyBypass);
          }
        }
      }
    }
    if(!proxy.IsEmpty())
    {
      FindUniqueProxy(proxy,p_secure);
      if(!m_ignored.IsEmpty())
      {
        // Converting WinHttp bypass list to ignore list requires "a bit" more then replacing ';' with ','
        // but in lots cases this works, so it is better then simply ignoring the ignore list (or is it?!)
        m_ignored.Replace(';',',');
      }
    }
    m_lastUsedDst = p_url;
  }
  SetInfo(m_proxy,m_ignored);

  return m_proxy;
}

void
FindProxy::SetInfo(XString p_proxy,XString p_bypass)
{
  if(!m_info)
  {
    m_info = new ProxyInfo();
  }
  m_proxy   = p_proxy;
  m_ignored = p_bypass;

  m_wProxy   = StringToWString(m_proxy);
  m_wIgnored = StringToWString(m_ignored);

  m_info->cfg.dwAccessType    = WINHTTP_ACCESS_TYPE_NAMED_PROXY;
  m_info->cfg.lpszProxy       = (LPWSTR) m_wProxy.c_str();
  m_info->cfg.lpszProxyBypass = (LPWSTR) m_wIgnored.c_str();
}

void
FindProxy::FindUniqueProxy(XString p_proxyList,bool p_secure)
{
  while(p_proxyList.GetLength())
  {
    XString part;
    int pos = p_proxyList.Find(';');
    if(pos > 0)
    {
      part = p_proxyList.Left(pos);
      p_proxyList = p_proxyList.Mid(pos + 1);
    }
    else
    {
      part = p_proxyList;
      p_proxyList.Empty();
    }
    // Secure proxy goes before insecure proxy
    if(p_secure)
    {
      if(part.Find("https=") == 0)
      {
        part.Replace("=","://");
        m_proxy = part;
        return;
      }
    }
    else
    {
      if(part.Find("http=") == 0)
      {
        part.Replace("=","://");
        m_proxy = part;
        return;
      }
    }
  }
  // Protocol not found
}

