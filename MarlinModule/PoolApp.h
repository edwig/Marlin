/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: PoolApp.h
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
#pragma once
#include <ServerApp.h>

// Forward reference
class IHttpApplication;

// Helper class with a web application
// Also contains the function pointers into our application DLL
class PoolApp
{
public:
  // Conditional destructor!!
  ~PoolApp();

  bool LoadPoolApp(IHttpApplication* p_httpapp,XString p_webroot,XString p_physical,XString p_application);

  // DATA
  WebConfigIIS        m_config;
  XString             m_marlinDLL;
  ServerApp*          m_application     { nullptr };
  HMODULE             m_module          { NULL    };
  // DLL Loaded functions
  CreateServerAppFunc m_createServerApp { nullptr };
  FindHTTPSiteFunc    m_findSite        { nullptr };
  GetHTTPStreamFunc   m_getHttpStream   { nullptr };
  GetHTTPMessageFunc  m_getHttpMessage  { nullptr };
  HandleMessageFunc   m_handleMessage   { nullptr };
  SitesInApplicPool   m_sitesInAppPool  { nullptr };
  MinVersionFunc      m_minVersion      { nullptr };

private:
  XString ConstructDLLLocation(XString p_rootpath, XString p_dllPath);
  bool    CheckApplicationPresent(XString& p_dllPath, XString& p_dllName);
  bool    AlreadyLoaded(XString p_path_to_dll);
};

