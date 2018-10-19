/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: MarlinServerAppFactory.h
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
#include "stdafx.h"
#include "MarlinServerAppFactory.h"
#include "MarlinServerApp.h"
#include "MarlinSecureServerApp.h"
#include "MarlinClientCertServerApp.h"

// The one and only app-factory for creating server apps
MarlinServerAppFactory factory;

// Generally this will only create ONE object
// But for the sake of explanation, we have three here!
// So we can have multiple applications in the same IIS application pool
//
ServerApp* 
MarlinServerAppFactory::CreateServerApp(IHttpServer* p_iis,LogAnalysis* p_logfile,CString p_appName, CString p_webroot)
{
  if(p_appName.CompareNoCase("MarlinTest") == 0)
  {
    return new MarlinServerApp(p_iis,p_logfile,p_appName,p_webroot);
  }

  if(p_appName.CompareNoCase("SecureTest") == 0)
  {
    return new MarlinSecureServerApp(p_iis,p_logfile,p_appName,p_webroot);
  }

  if(p_appName.CompareNoCase("SecureClientCert") == 0)
  {
    return new MarlinClientCertServerApp(p_iis,p_logfile,p_appName,p_webroot);
  }

  return nullptr;
}
