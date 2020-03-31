/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: MarlinServerApp.cpp
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
#include "TestMarlinServerApp.h"
#include "WebSocket.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// Total number of errors registered while testing
int g_errors = 0;
WebConfigIIS* g_config = nullptr;

// CString contract = "http://interface.marlin.org/testing/";

// This is the ServerApp of the IIS server variant (running in W3SVC)

//////////////////////////////////////////////////////////////////////////
//
// The test server app
//
//////////////////////////////////////////////////////////////////////////

TestMarlinServerAppPool::TestMarlinServerAppPool(IHttpServer*   p_iis
                                        ,const char*    p_webroot
                                        ,const char*    p_appName)
                    :ServerApp(p_iis,p_webroot,p_appName)
{
}

TestMarlinServerAppPool::~TestMarlinServerAppPool()
{
}

void
TestMarlinServerAppPool::InitInstance()
{
  // AppPool already running?
  // If already started for another app in the application pool
  if(m_running)
  {
    ++m_numSites;
    return;
  }

  // First always call the main class 
  // Must init for the HTTPServer and other objects
  ServerApp::InitInstance();

  // Can only be called once if correctly started
  if(!CorrectlyStarted())
  {
    return;
  }

  // PRE-Configure our server application for use with IIS
  // so it will now it's not running standalone
  m_server.ConfigIISServer(m_applicationName,m_httpServer,m_threadPool,m_logfile);

  // Set our logging level
  SetLogLevel(HLL_TRACEDUMP);  // NOLOG / ERRORS / LOGGING / LOGBODY / TRACE / TRACEDUMP
  m_server.SetLogLevel(HLL_TRACEDUMP);

  // Now starting our main application
  if(m_server.Startup())
  {
    // Instance is now running
    m_running = true;
    ++m_numSites;
  }
}

bool 
TestMarlinServerAppPool::LoadSite(IISSiteConfig& p_config)
{
  // Already done in the InitInstance
  return true;
}

void
TestMarlinServerAppPool::ExitInstance()
{
  // One less site in the application pool?
  if(--m_numSites > 0)
  {
    return;
  }

  if(m_running)
  {
    // Testing the error log function
    m_httpServer->ErrorLog(__FUNCTION__,0,"Not a real error message, but a test to see if the error logging works :-)");

    // Stopped running
    m_running = false;
  }

  // Now shutting down our server
  m_server.ShutDown();

  // Always call the ExitInstance of the main class last
  // Will destroy the HTTPServer and all other objects
  ServerApp::ExitInstance();
}

bool
TestMarlinServerAppPool::CorrectlyStarted()
{
  if(ServerApp::CorrectlyStarted() == false)
  {
    m_logfile->AnalysisLog(__FUNCTION__,LogType::LOG_ERROR,false,"ServerApp incorrectly started. Review your program logic");
    return false;
  }
  return true;
}


