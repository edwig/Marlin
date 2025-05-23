/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: URLRewriter.cpp
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
#include "stdafx.h"
#include "URLRewriter.h"
#include "HTTPMessage.h"
#include "HTTPClient.h"
#include <StringUtilities.h>
#include <vector>

/*/////////////////////////////////////////////////////////////////////////

URL Rewrite

Type Pattern                  Rewrite
---- ------------------------ -------------------------------
P    http                     https,ws,wss
P    https                    http,ws,wss
P    ws                       http,https,wss
P    wss                      http,https,ws
S    server:<name>            server:<othername>
I    port:xx                  port:yy
R    Route1:<name>            Route1:<othername>
R    Route2:<name>            Route2:<othername>
R    Route<n>:<name>          Route<n>:<othername>
A    Abspath:<path>           Abspath <other path>
E    extension:ext            extension:zzz
D    DelRoute:n               Remove route element from abspath
F    FromRoute:n              Everything after this route element

URI
Convert to lowercase!

URI
<protocol>://<user>:<password>@<server>:<port>/<route1>/<route2>/<route3>/<filename>.<ext>

Reduce to secure URI
<protocol>://<server>:<port>/<route1>/<route2>/<route3>/.../<route-n>/<filename>.<ext>

Reduce to Abspath
/<route1>/<route2>/<route3>/.../<route-n>/<filename>.<ext>

/////////////////////////////////////////////////////////////////////////*/

URLRewriter::URLRewriter()
{
}

URLRewriter::~URLRewriter()
{
  if(m_nextRewriter)
  {
    delete m_nextRewriter;
    m_nextRewriter = nullptr;
  }
}

// Main processor of the URL
bool 
URLRewriter::ProcessHTTPMessage(HTTPMessage* p_message)
{
  CrackedURL url(p_message->GetCrackedURL());
  Routing    routing(p_message->GetRouting());

  if(!ReWriteURL(url,routing))
  {
    // Nothing rewritten, try recurse to the next rewriter(s)
    if(m_nextRewriter)
    {
      return m_nextRewriter->ProcessHTTPMessage(p_message);
    }
    // Nothing to proxy
    return false;
  }

  // Reconstruct the URL
  HTTPMessage message(p_message);
  message.SetURL(url.URL());

  HTTPClient client;
  if(client.Send(&message))
  {
    p_message->Reset();
    p_message->SetStatus(message.GetStatus());
    p_message->GetFileBuffer()->Reset();
   *p_message->GetFileBuffer() = *message.GetFileBuffer();
   *p_message->GetHeaderMap()  = *message.GetHeaderMap();
  }
  else
  {
    // Send a 503 BAD GATEWAY
    p_message->Reset();
    p_message->SetStatus(HTTP_STATUS_BAD_GATEWAY);
  }
  // Processed
  return true;
}

// Rewrite the URL of the message (true = rewritten)
bool 
URLRewriter::ReWriteURL(CrackedURL& p_url,Routing& p_routing)
{
  int changes = 0;

  changes += RewriteProtocol (p_url);
  changes += RewriteServer   (p_url);
  changes += RewritePort     (p_url);
  changes += RewritePath     (p_url);
  changes += RewriteExtension(p_url);
  changes += RewriteFromRoute(p_url,p_routing);
  changes += RewriteRoute    (p_url,p_routing);
  changes += RewriteDelRoute (p_url,p_routing);

  return (changes > 0);
}

void
URLRewriter::InitRewriter(MarlinConfig& p_config)
{ 
  CString rewriter(_T("Rewriter"));

  CString protocol        = p_config.GetParameterString(rewriter,_T("Protocol"),      _T(""));
  CString server          = p_config.GetParameterString(rewriter,_T("Server"),        _T(""));
  CString port            = p_config.GetParameterString(rewriter,_T("Port"),          _T(""));
  CString abspath         = p_config.GetParameterString(rewriter,_T("Path"),          _T(""));
  CString route0          = p_config.GetParameterString(rewriter,_T("Route0"),        _T(""));
  CString route1          = p_config.GetParameterString(rewriter,_T("Route1"),        _T(""));
  CString route2          = p_config.GetParameterString(rewriter,_T("Route2"),        _T(""));
  CString route3          = p_config.GetParameterString(rewriter,_T("Route3"),        _T(""));
  CString route4          = p_config.GetParameterString(rewriter,_T("Route4"),        _T(""));
  CString removeRoute     = p_config.GetParameterString(rewriter,_T("RemoveRoute"),   _T(""));
  CString startRoute      = p_config.GetParameterString(rewriter,_T("StartRoute"),    _T(""));

  CString targetProtocol  = p_config.GetParameterString(rewriter,_T("TargetProtocol"),_T(""));
  CString targetServer    = p_config.GetParameterString(rewriter,_T("TargetServer"),  _T(""));
  CString targetPort      = p_config.GetParameterString(rewriter,_T("TargetPort"),    _T(""));
  CString targetAbspath   = p_config.GetParameterString(rewriter,_T("TargetPath"),    _T(""));
  CString targetRoute0    = p_config.GetParameterString(rewriter,_T("TargetRoute0"),  _T(""));
  CString targetRoute1    = p_config.GetParameterString(rewriter,_T("TargetRoute1"),  _T(""));
  CString targetRoute2    = p_config.GetParameterString(rewriter,_T("TargetRoute2"),  _T(""));
  CString targetRoute3    = p_config.GetParameterString(rewriter,_T("TargetRoute3"),  _T(""));
  CString targetRoute4    = p_config.GetParameterString(rewriter,_T("TargetRoute4"),  _T(""));

  if(!protocol.IsEmpty())
  {
    AddProtocolMapping(protocol,targetProtocol);
  }
  if(!server.IsEmpty())
  {
    AddServerMapping(server,targetServer);
  }
  if(!port.IsEmpty())
  {
    AddPortMapping(_ttoi(port),_ttoi(targetPort));
  }
  if(!abspath.IsEmpty())
  {
    AddPathMapping(abspath,targetAbspath);
  }
  if(!route0.IsEmpty())
  {
    AddRouteMapping(0,route0,targetRoute0);
  }
  if(!route1.IsEmpty())
  {
    AddRouteMapping(1,route1,targetRoute1);
  }
  if(!route2.IsEmpty())
  {
    AddRouteMapping(2,route2,targetRoute2);
  }
  if(!route3.IsEmpty())
  {
    AddRouteMapping(3,route3,targetRoute3);
  }
  if(!route4.IsEmpty())
  {
    AddRouteMapping(4,route4,targetRoute4);
  }
  if(!removeRoute.IsEmpty())
  {
    std::vector<XString> routes;
    SplitString(removeRoute,routes,_T(','));
    for(int ind = 0; ind < routes.size(); ++ind)
    {
      int route = _ttoi(routes[ind]);
      AddDelRoute(route);
    }
  }
  if(!startRoute.IsEmpty())
  {
    int route = _ttoi(startRoute);
    AddFromRoute(route);
  }
}

bool
URLRewriter::AddProtocolMapping(XString p_from,XString p_to)
{
  m_protocolMap[p_from] = p_to;
  return true;
}

bool
URLRewriter::AddServerMapping(XString p_from,XString p_to)
{
  m_serverMap[p_from] = p_to;
  return true;
}

bool
URLRewriter::AddPortMapping(int p_from,int p_to)
{
  m_portMap[p_from] = p_to;
  return true;
}

bool
URLRewriter::AddPathMapping(XString p_from,XString p_to)
{
  m_pathMap[p_from] = p_to;
  return true;
}

bool
URLRewriter::AddExtensionMapping(XString p_from,XString p_to)
{
  m_extensionMap[p_from] = p_to;
  return true;
}

bool
URLRewriter::AddRouteMapping(int p_route,XString p_from,XString p_to)
{
  m_routeMap[p_route][p_from] = p_to;
  return true;
}

bool
URLRewriter::AddDelRoute(int p_route)
{
  m_delRoute.push_back(p_route);
  return true;
}

bool
URLRewriter::AddFromRoute(int p_route)
{
  m_fromRoute = p_route;
  return true;
}

// Add an extra URL rewriter to the rewriter chain
void 
URLRewriter::AddRewriter(URLRewriter* p_rewriter)
{
  if(m_nextRewriter)
  {
    m_nextRewriter->AddRewriter(p_rewriter);
  }
  else
  {
    m_nextRewriter = p_rewriter;
  }
}

//////////////////////////////////////////////////////////////////////////
//
// REWRITE METHODS
//
//////////////////////////////////////////////////////////////////////////

int 
URLRewriter::RewriteProtocol(CrackedURL& p_url)
{
  URLMap::iterator it = m_protocolMap.find(p_url.m_scheme);
  if(it != m_protocolMap.end())
  {
    p_url.m_scheme = it->second;
    return 1;
  }
  return 0;
}

int
URLRewriter::RewriteServer(CrackedURL& p_url)
{
  URLMap::iterator it = m_serverMap.find(p_url.m_host);
  if(it != m_serverMap.end())
  {
    p_url.m_host = it->second;
    return 1;
  }
  return 0;
}

int
URLRewriter::RewritePort(CrackedURL& p_url)
{
  PortMap::iterator it = m_portMap.find(p_url.m_port);
  if(it != m_portMap.end())
  {
    p_url.m_port = it->second;
    return 1;
  }
  return 0;
}

int 
URLRewriter::RewritePath(CrackedURL& p_url)
{
  URLMap::iterator it = m_pathMap.find(p_url.m_path);
  if(it != m_pathMap.end())
  {
    p_url.m_path = it->second;
    return 1;
  }
  return 0;
}

int
URLRewriter::RewriteExtension(CrackedURL& p_url)
{
  URLMap::iterator it = m_extensionMap.find(p_url.GetExtension());
  if(it != m_extensionMap.end())
  {
    p_url.SetExtension(it->second);
    return 1;
  }
  return 0;
}

int
URLRewriter::RewriteFromRoute(CrackedURL& p_url,Routing& p_routing)
{
  if(m_fromRoute == 0)
  {
    return 0;
  }
  int changes = 0;

  if(p_routing.size() > m_fromRoute)
  {
    XString path(_T("/"));
    for(int ind = m_fromRoute;ind < p_routing.size() ;++ind)
    {
      path += p_routing[ind];
      path += _T("/");
    }
    p_url.m_path = path;
    changes = 1;
  }
  return changes;
}

int
URLRewriter::RewriteRoute(CrackedURL& p_url,Routing& p_routing)
{
  int changes = 0;
  for(int ind = 0; ind < p_routing.size(); ++ind)
  {
    RWRoute::iterator it = m_routeMap.find(ind);
    if(it != m_routeMap.end())
    {
      URLMap::iterator it2 = it->second.find(p_routing[ind]);
      if(it2 != it->second.end())
      {
        p_routing[ind] = it2->second;
        ++changes;
      }
    }
  }
  if(changes)
  {
    XString path(_T("/"));
    for(auto& route : p_routing)
    {
      path += route;
      path += _T("/");
    }
    p_url.m_path = path;
  }
  return changes;
}

int
URLRewriter::RewriteDelRoute(CrackedURL& p_url,Routing& p_routing)
{
  int changes = 0;

  for(auto& delroute : m_delRoute)
  {
    if(delroute >= 0 && delroute < p_routing.size())
    {
      p_routing.erase(p_routing.begin() + delroute);
      ++changes;
    }
  }
  if(changes)
  {
    XString path(_T("/"));
    for(auto& route : p_routing)
    {
      path += route;
      path += _T("/");
    }
    p_url.m_path = path;
  }
  return changes;
}
