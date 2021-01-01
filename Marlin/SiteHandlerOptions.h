/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: SiteHandlerOptions.h
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
#include "SiteHandler.h"
#include <vector>

// Most likely the ONLY trace handler you will ever want
// Nothing more to be done. See RFC 2616 to confirm this.
//
// NO NEED TO OVERRIDE THIS CLASS!!
//
class SiteHandlerOptions: public SiteHandler
{
protected:
  // Handlers: Override and return 'true' if handling is ready
  virtual bool  PreHandle(HTTPMessage* p_message) override;
  virtual bool     Handle(HTTPMessage* p_message) override;
  virtual void PostHandle(HTTPMessage* p_message) override;
private:
  // Do the CORS Pre-Flight checking for an OPTIONS call
  bool CheckCrossOriginSettings(HTTPMessage* p_message,CString p_origin,CString p_method,CString p_headers);

  bool CheckCORSOrigin (HTTPMessage* p_message,CString p_origin);
  bool CheckCORSMethod (HTTPMessage* p_message,CString p_method);
  bool CheckCORSHeaders(HTTPMessage* p_message,CString p_headers);

  void SplitHeaders(CString p_headers,std::vector<CString>& p_map);
};
