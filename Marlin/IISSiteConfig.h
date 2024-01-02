/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: IISSiteConfig.h
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
#include "CreateURLPrefix.h"
#include "WebConfigIIS.h"
#include <vector>

typedef struct _iis_binding
{
  bool        m_secure { false };   // Protocol http or https
  PrefixType  m_prefix { PrefixType::URLPRE_Weak}; // Prefix type of binding
  int         m_port   {    80 };   // Default port
  int         m_flags  {     0 };   // SSL flags 0=ssl, 1=accept client cert, 2=require client cert
}
IISBinding;

using BindMap = std::vector<IISBinding>;

// Config of an application
typedef struct _iis_site_config
{
  //                      What it is                  applicationHost.config
  // -------------------- -------------------------   -----------------------------------------------------
  XString     m_name;     // Name of the site         sites/site/name=
  unsigned    m_id { 0 }; // IIS ID of the site       sites/site/id
  XString     m_pool;     // Application pool name    sites/site/application/applicationPool=
  XString     m_base_url; // URL base path            sites/site/application/virtualDirectory/path=
  XString     m_physical; // Physical working dir     sites/site/application/virtualDirectory/physicalPath=

  BindMap     m_bindings; // Bindings to the internet
  IISHandlers m_handlers; // Handlers
}
IISSiteConfig;

using IISSiteConfigs = std::vector<IISSiteConfig*>;
