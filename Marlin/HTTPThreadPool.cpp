/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: HTTPThreadPool.cpp
//
// Marlin Server: Internet server/client
// 
// Copyright (c) 2016 ir. W.E. Huisman
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
#include "HTTPThreadPool.h"
#include "HTTPSite.h"

//////////////////////////////////////////////////////////////////////////
//
// Derived threadpool class, especially for HTTP module
// It's a contract that the callback's p_argument is a HTTPMessage
// So we can call the default handler of the HTTPSite
//
//////////////////////////////////////////////////////////////////////////

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// A means to be free to debug the threadpool in debug mode
#ifdef DEBUG_THREADPOOL
#define TP_TRACE0(sz)        TRACE0(sz)
#else
#define TP_TRACE0(sz)        ;
#endif


HTTPThreadPool::HTTPThreadPool()
{
}

HTTPThreadPool::HTTPThreadPool(int p_minThreads,int p_maxThreads)
               :ThreadPool(p_minThreads,p_maxThreads)
{
}

// This is the real callback. Overloaded from the ThreadPool
void
HTTPThreadPool::DoTheCallback(LPFN_CALLBACK p_callback,void* p_argument)
{
  if(p_callback == (LPFN_CALLBACK) SITE_CALLBACK_HTTP)
  {
    // Most likely a HTTP message
    HTTPMessage* message = dynamic_cast<HTTPMessage*>((HTTPMessage*)p_argument);
    if(message)
    {
      HTTPSite* site = message->GetHTTPSite();
      if(site)
      {
        site->HandleHTTPMessage(message);
        return;
      }
    }
  }
  else if(p_callback == (LPFN_CALLBACK) SITE_CALLBACK_EVENT)
  {
    // Starting a SSE stream?
    EventStream* stream = dynamic_cast<EventStream*>((EventStream*)p_argument);
    if(stream)
    {
      HTTPSite* site = stream->m_site;
      if(site)
      {
        site->HandleEventStream(stream);
        return;
      }
    }
  }
  else if(p_callback)
  {
    // Regular callback. eg. Worker thread
    (*(p_callback))(p_argument);
  }
  else
  {
    // Something is very wrong!!
    TP_TRACE0("INTERNAL ERROR: HTTPThreadpool called without a regular callback, a HTTPMessage or an EventStream!\n");
  }
}

