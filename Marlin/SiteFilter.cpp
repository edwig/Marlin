/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: SiteFilter.cpp
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
#include "stdafx.h"
#include "SiteFilter.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

SiteFilter::SiteFilter(unsigned p_priority,CString p_name)
           :m_priority(p_priority)
           ,m_name(p_name)
{
}

SiteFilter::~SiteFilter()
{
}

// Default nothing to do
// Implement your own for initialisation reasons!
void
SiteFilter::OnStartSite()
{
}

// Default nothing to do
// Implement your own for initialisation reasons!
void
SiteFilter::OnStopSite()
{
}

// TO IMPLEMENT: Override this method to write your own filter
// MIND YOU: The intention is to READ the HTTPMessage only
// and to not send an answer to the client (although you can!)
// Let the SiteHandler take the task of sending an answer
// But you can do all kinds of interesting things with it
bool
SiteFilter::Handle(HTTPMessage* p_message)
{
  UNREFERENCED_PARAMETER(p_message);
  // Nothing done
  TRACE("BEWARE: Incorrect implemented HTTP SiteFilter. Handle method not overriden!\n");

  return true;
}
