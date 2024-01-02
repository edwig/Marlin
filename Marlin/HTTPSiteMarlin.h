/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: HTTPSiteMarlin.h
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
#include "HTTPSite.h"

class HTTPServerMarlin;

class HTTPSiteMarlin : public HTTPSite
{
public:
  HTTPSiteMarlin(HTTPServerMarlin*  p_server
                ,int                p_port
                ,XString            p_site
                ,XString            p_prefix
                ,HTTPSite*          p_mainSite = nullptr
                ,LPFN_CALLBACK      p_callback = nullptr);

  // MANDATORY: Explicitly starting after configuration of the site
  virtual bool StartSite() override;

  // OPTIONAL: Set the webroot of the site
  virtual bool SetWebroot(XString p_webroot) override;

protected:
  // Initialize the site from automatic settings in the config
  void         InitSite(MarlinConfig& p_config);
};
