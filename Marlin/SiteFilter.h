/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: SiteFilter.h
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
#pragma once
#include "HTTPSite.h"

// The site filter
// Create your own derived class with it's own "Handle" method
//
class SiteFilter
{
public:
  SiteFilter(unsigned p_priority,CString p_name);
  virtual ~SiteFilter();

  // When starting the site
  virtual void OnStartSite();
  // When stopping the site
  virtual void OnStopSite();

  // Handle the filter
  virtual bool Handle(HTTPMessage* p_message);
  // Getters and setters
  virtual void SetSite(HTTPSite* p_site) { m_site = p_site;   };
  HTTPSite*    GetSite()                 { return m_site;     };
  CString      GetName()                 { return m_name;     };
  unsigned     GetPriority()             { return m_priority; };

protected:
  HTTPSite* m_site      { nullptr };
  unsigned  m_priority  { 0       };
  CString   m_name;
};


