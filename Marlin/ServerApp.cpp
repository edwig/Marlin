/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: ServerApp.h
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
#include "ServerApp.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// Pointer to the one and only server object
// Meant to be run in IIS and not as a stand alone HTTP server!!
ServerApp* g_server = nullptr;

//XTOR
ServerApp::ServerApp()
{
  if(g_server)
  {
    // Cannot run more than one ServerApp
    m_correctInit = false;     
  }
  else
  {
    // First registration of the ServerApp derived class
    m_correctInit = true;
    g_server = this;
  }
}

// DTOR
ServerApp::~ServerApp()
{
}

// Called by the MarlinModule at startup of the application pool.
// Will be called **BEFORE** the "InitInstance" of the ServerApp.
void 
ServerApp::ConnectServerApp(IHttpServer*   p_iis
                           ,HTTPServerIIS* p_server
                           ,ThreadPool*    p_pool
                           ,LogAnalysis*   p_logfile
                           ,ErrorReport*   p_report)
{
  // Simply remember our registration
  m_iis        = p_iis;
  m_appServer  = p_server;
  m_appPool    = p_pool;
  m_appLogfile = p_logfile;
  m_appReport  = p_report;
}

// Server app was correctly started by MarlinIISModule
bool
ServerApp::CorrectlyStarted()
{
  // MINIMUM REQUIREMENT:
  // If a derived class has been staticly declared
  // and a IHttpServer and a HTTPServerIIS has been found
  // and a Threadpool is initialized, we are good to go
  if(m_correctInit && m_iis && m_appServer && m_appPool)
  {
    return true;
  }
  return false;
}

