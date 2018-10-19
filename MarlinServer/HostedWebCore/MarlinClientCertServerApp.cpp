/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: MarlinClientCertServerApp.cpp
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
#include "MarlinClientCertServerApp.h"
#include "MarlinServerApp.h"
#include "MarlinModule.h"
#include "TestServer.h"
#include "WebSocket.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// This is a ServerApp of the IIS server variant (running in W3SVC)

//////////////////////////////////////////////////////////////////////////
//
// The test server app
//
//////////////////////////////////////////////////////////////////////////

MarlinClientCertServerApp::MarlinClientCertServerApp(IHttpServer* p_iis,LogAnalysis* p_logfile,CString p_appName,CString p_webroot)
                          :ServerApp(p_iis,p_logfile,p_appName,p_webroot)
{
}

MarlinClientCertServerApp::~MarlinClientCertServerApp()
{
}

void
MarlinClientCertServerApp::InitInstance()
{
  // First always call the main class 
  // Must init for the HTTPServer and other objects
  ServerApp::InitInstance();

  CString contract = "http://interface.marlin.org/testing/";

  // Can only be called once if correctly started
  if (!CorrectlyStarted() || m_running)
  {
    return;
  }
  // Instance is now running
  m_running = true;

  // Set our logging level
  SetLogLevel(HLL_TRACEDUMP);  // NOLOG / ERRORS / LOGGING / LOGBODY / TRACE / TRACEDUMP

  // Starting objects and sites
  TestClientCertificate(m_httpServer,false);
}

bool
MarlinClientCertServerApp::LoadSite(IISSiteConfig& /*p_config*/)
{
  // Already done in the InitInstance
  return true;
}

void
MarlinClientCertServerApp::ExitInstance()
{
  if (m_running)
  {
    // Testing the error log function
    m_httpServer->ErrorLog(__FUNCTION__, 0, "Not a real error message, but a test to see if the error logging works :-)");

    // Stopping all sub-sites
    StopSubsites(m_httpServer);

    // Report all tests
    ReportAfterTesting();

    // Stopped running
    m_running = false;
  }

  // Always call the ExitInstance of the main class last
  // Will destroy the HTTPServer and all other objects
  ServerApp::ExitInstance();
}

bool
MarlinClientCertServerApp::CorrectlyStarted()
{
  if (ServerApp::CorrectlyStarted() == false)
  {
    qprintf("ServerApp incorrectly started. Review your program logic");
    return false;
  }
  return true;
}

void
MarlinClientCertServerApp::ReportAfterTesting()
{
  AfterTestClientCert();

  // SUMMARY OF ALL THE TEST
  // ---- "---------------------------------------------- - ------
  qprintf("TOTAL number of errors after all tests are run : %d", g_errors);
}

