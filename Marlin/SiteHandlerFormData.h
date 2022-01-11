/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: SiteHandlerFormData.h
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
#include "SiteHandlerPost.h"
#include "MultiPartBuffer.h"
#include "CrackURL.h"

class SiteHandlerFormData: public SiteHandlerPost
{
protected:
  // Handlers: Override and return 'true' if handling is ready
  virtual bool Handle(HTTPMessage* p_message) override;

  // This is the work of the posted action
  // Pre- / Post- HandleBuffer get called only one (before/after the parts)
  // For each MultiPart in the MultiPartBuffer of the form-data message
  // one (1) handler will be called, on the fact that the "filename" attribute
  // has been given in the "Content-Disposition" part header!!
  // RETURN: NUMBER OF ERRORS!
  virtual int PreHandleBuffer (HTTPMessage* p_message,MultiPartBuffer* p_buffer);
  virtual int HandleData      (HTTPMessage* p_message,MultiPart*       p_part);
  virtual int HandleFile      (HTTPMessage* p_message,MultiPart*       p_part);
  virtual int PostHandleBuffer(HTTPMessage* p_message,MultiPartBuffer* p_buffer);
};
