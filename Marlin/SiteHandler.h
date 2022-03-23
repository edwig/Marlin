/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: SiteHandler.h
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
#include "HTTPSite.h"
#include "HTTPMessage.h"
#include "HTTPServer.h"
#include "LogAnalysis.h"

// Macro's for logging in the site handlers
#define SITE_DETAILLOG1(text)       m_site->GetHTTPServer()->DetailLog (__FUNCTION__,LogType::LOG_INFO,(text))
#define SITE_DETAILLOGS(text,extra) m_site->GetHTTPServer()->DetailLogS(__FUNCTION__,LogType::LOG_INFO,(text),(extra))
#define SITE_DETAILLOGV(text,...)   m_site->GetHTTPServer()->DetailLogV(__FUNCTION__,LogType::LOG_INFO,(text),__VA_ARGS__)
#define SITE_ERRORLOG(code,text)    m_site->GetHTTPServer()->ErrorLog  (__FUNCTION__,(code),(text));

// Main base class for a HTTPSite:SiteHandler

class SiteHandler
{
public:
  SiteHandler();
  virtual ~SiteHandler();

  // Connect to this site / to be called from HTTPSite
  void         SetSite(HTTPSite* p_site)            { m_site = p_site; }
  SiteHandler* GetNextHandler()                     { return m_next;   }
  void         SetNextHandler(SiteHandler* p_next,bool p_owner);

  // When starting the site
  virtual void OnStartSite();

  // Go handle this message
  virtual void HandleMessage(HTTPMessage* p_message);
  virtual void HandleStream (HTTPMessage* p_message,EventStream* p_stream);
  virtual void CleanUp      (HTTPMessage* p_message);

  // When stopping the site
  virtual void OnStopSite();

protected:
  // Handlers: Override and return 'false' if handling is ready
  virtual bool  PreHandle(HTTPMessage* p_message);
  virtual bool     Handle(HTTPMessage* p_message);
  virtual void PostHandle(HTTPMessage* p_message);

  HTTPSite*    m_site { nullptr };  // Parent site of the handler
  SiteHandler* m_next { nullptr };  // Next handler for this HTTP command
  bool         m_nextowner{false};  // Owner of next handler
};

