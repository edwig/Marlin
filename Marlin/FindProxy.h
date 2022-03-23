/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: FindProxy.h
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
#pragma once
#include <winhttp.h>
#include <string>

using std::wstring;

// Struct to free the proxy info
typedef struct FreeTheProxyInfo
{
  WINHTTP_PROXY_INFO cfg;
  FreeTheProxyInfo()
  {
    cfg.dwAccessType    = 0;
    cfg.lpszProxy       = nullptr;
    cfg.lpszProxyBypass = nullptr;
  };
}
ProxyInfo;

// struct to GlobalFree any allocated cfg memmbers
typedef struct FreeTheProxyCfg 
{
  WINHTTP_CURRENT_USER_IE_PROXY_CONFIG cfg;
 ~FreeTheProxyCfg()
  {
    if(cfg.lpszProxy)
    {
      ::GlobalFree(cfg.lpszProxy);
    }
    if(cfg.lpszProxyBypass)
    {
      ::GlobalFree(cfg.lpszProxyBypass);
    }
    if(cfg.lpszAutoConfigUrl)
    {
      ::GlobalFree(cfg.lpszAutoConfigUrl);
    }
  }
}
ProxyConfig;

typedef struct FreeTheInternet//struct to close the dummy internet handle when done
{
  HINTERNET hInter;
 ~FreeTheInternet() 
  { 
    if (!!hInter) 
    {
      ::WinHttpCloseHandle(hInter); 
    }
  };
}
Internet;

class FindProxy
{
public:
  FindProxy();
 ~FindProxy();

  XString     Find(const XString& p_url,bool p_secure);
  XString     GetIngoreList();
  ProxyInfo*  GetProxyInfo();
  void        SetInfo(XString p_proxy, XString p_bypass);

private:
  void        FindUniqueProxy(XString p_proxyList,bool p_secure);
  ProxyInfo*  m_info;
  bool        m_perDest;
  XString     m_ignored;
  XString     m_lastUsedDst;
  XString     m_proxy;
  wstring     m_wProxy;
  wstring     m_wIgnored;
};

inline XString 
FindProxy::GetIngoreList()
{
  return m_ignored;
}

inline ProxyInfo*
FindProxy::GetProxyInfo()
{
  return m_info;
}