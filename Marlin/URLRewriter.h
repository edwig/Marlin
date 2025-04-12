/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: URLRewriter.h
//
// Marlin Server: Internet server/client
// 
// Copyright (c) 2014-2025 ir. W.E. Huisman
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
#include "MarlinConfig.h"
#include <map>
#include <XString.h>
#include <CrackURL.h>
#include <Routing.h>
#include <vector>

class HTTPMessage;

using URLMap  = std::map<XString,XString>;
using PortMap = std::map<int,int>;
using DelRMap = std::vector<int>;
using RWRoute = std::map<int,std::map<XString,XString>>;

class URLRewriter
{
public:
  URLRewriter();
 ~URLRewriter();

  // Main processor of the URL
  virtual bool ProcessHTTPMessage(HTTPMessage* p_message);

  // Rewrite the URL of the message (true = rewritten)
  virtual bool ReWriteURL(CrackedURL& p_url,Routing& p_routing);

  // Initialize the rewriter from a config file
  virtual void InitRewriter(MarlinConfig& p_config);

  // Add an extra URL rewriter to the rewriter chain
  virtual void AddRewriter(URLRewriter* p_rewriter);

  // Setters
  bool    AddProtocolMapping (XString p_from,XString p_to);
  bool    AddServerMapping   (XString p_from,XString p_to);
  bool    AddPortMapping     (int p_from,int p_to);
  bool    AddPathMapping     (XString p_from,XString p_to);
  bool    AddExtensionMapping(XString p_from,XString p_to);
  bool    AddRouteMapping    (int p_route,XString p_from,XString p_to);
  bool    AddDelRoute        (int p_route);
  bool    AddFromRoute       (int p_route);

  // Getters
  URLMap  GetProtocolMap() const  { return m_protocolMap;   }
  URLMap  GetServerMap()   const  { return m_serverMap;     }
  PortMap GetPortMap()     const  { return m_portMap;       }
  URLMap  GetPathMap()     const  { return m_pathMap;       }
  URLMap  GetExtensionMap()const  { return m_extensionMap;  }
  DelRMap GetDelRoute()    const  { return m_delRoute;      }
  RWRoute GetRouteMap()    const  { return m_routeMap;      }
  int     GetFromRoute()   const  { return m_fromRoute;     }

protected:
  virtual int RewriteProtocol (CrackedURL& p_url);
  virtual int RewriteServer   (CrackedURL& p_url);
  virtual int RewritePort     (CrackedURL& p_url);
  virtual int RewritePath     (CrackedURL& p_url);
  virtual int RewriteExtension(CrackedURL& p_url);
  virtual int RewriteFromRoute(CrackedURL& p_url,Routing& p_routing);
  virtual int RewriteRoute    (CrackedURL& p_url,Routing& p_routing);
  virtual int RewriteDelRoute (CrackedURL& p_url,Routing& p_routing);

  URLMap        m_protocolMap;
  URLMap        m_serverMap;
  PortMap       m_portMap;
  URLMap        m_pathMap;
  URLMap        m_extensionMap;
  DelRMap       m_delRoute;
  RWRoute       m_routeMap;
  int           m_fromRoute    { 0 };
  URLRewriter*  m_nextRewriter { nullptr };
};


