/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: MarlinSecureServerApp.h
//
// Marlin Server: Internet server/client
// 
// Copyright (c) 2015-2018 ir. W.E. Huisman
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
#include "ServerApp.h"

extern int g_errors;

// This is the ServerApp of the IIS server variant (running in W3SVC)

class MarlinSecureServerApp : public ServerApp
{
public:
  MarlinSecureServerApp(IHttpServer* p_iis,LogAnalysis* p_logfile,CString p_appName, CString p_webroot);
  virtual ~MarlinSecureServerApp();

  virtual void InitInstance();
  virtual void ExitInstance();
  virtual bool LoadSite(IISSiteConfig& p_config);

private:
  bool CorrectlyStarted();
  void ReportAfterTesting();

  bool m_doDetails { false };
  bool m_running   { false };
};
