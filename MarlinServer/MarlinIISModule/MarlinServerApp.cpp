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
#include "MarlinServerApp.h"
#include "MarlinModule.h"
#include "TestServer.h"
#include "WebSocket.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// Total number of errors registered while testing
int g_errors = 0;


// This is the ServerApp of the IIS server variant (running in W3SVC)

//////////////////////////////////////////////////////////////////////////
//
// The test server app
//
//////////////////////////////////////////////////////////////////////////

MarlinServerApp::MarlinServerApp(IHttpServer* p_iis, CString p_appName, CString p_webroot)
                :ServerApp(p_iis,p_appName,p_webroot)
{
}

MarlinServerApp::~MarlinServerApp()
{
}

void
MarlinServerApp::InitInstance()
{
  // First always call the main class 
  // Must init for the HTTPServer and other objects
  ServerApp::InitInstance();

  CString contract = "http://interface.marlin.org/testing/";

  // Can only be called once if correctly started
  if(!CorrectlyStarted() || m_running)
  {
    return;
  }
  // Instance is now running
  m_running = true;

  // Set our logging level
  SetLogLevel(HLL_TRACEDUMP);  // NOLOG / ERRORS / LOGGING / LOGBODY / TRACE / TRACEDUMP

  // Small local test
  Test_CrackURL();
  Test_HTTPTime();

  // Starting objects and sites
  TestPushEvents(m_httpServer);
  TestWebServiceServer(m_httpServer,contract,m_logLevel);
  TestJsonServer(m_httpServer,contract,m_logLevel);
  TestCookies(m_httpServer);
  TestInsecure(m_httpServer);
  TestBaseSite(m_httpServer);
  TestBodySigning(m_httpServer);
  TestBodyEncryption(m_httpServer);
  TestMessageEncryption(m_httpServer);
  TestReliable(m_httpServer);
  TestReliableBA(m_httpServer);
  TestToken(m_httpServer);
  TestSubSites(m_httpServer);
  TestJsonData(m_httpServer);
  TestFilter(m_httpServer);
  TestPatch(m_httpServer);
  TestFormData(m_httpServer);
  TestCompression(m_httpServer);
  TestAsynchrone(m_httpServer);
  TestWebSocket(m_httpServer);
}

bool 
MarlinServerApp::LoadSite(IISSiteConfig& p_config)
{
  // Already done in the InitInstance
  return true;
}

void
MarlinServerApp::ExitInstance()
{
  if(m_running)
  {
    // Testing the error log function
    m_httpServer->ErrorLog(__FUNCTION__,0,"Not a real error message, but a test to see if the error logging works :-)");

    // Stopping our WebSocket
    StopWebSocket();

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
MarlinServerApp::CorrectlyStarted()
{
  if(ServerApp::CorrectlyStarted() == false)
  {
    qprintf("ServerApp incorrectly started. Review your program logic");
    return false;
  }
  return true;
}

void 
MarlinServerApp::ReportAfterTesting()
{
  AfterTestCrackURL();
  AfterTestTime();
  AfterTestBaseSite();
  AfterTestFilter();
  AfterTestAsynchrone();
  AfterTestBodyEncryption();
  AfterTestBodySigning();
  AfterTestCompression();
  AfterTestContract();
  AfterTestCookies();
  AfterTestEvents();
  AfterTestFormData();
  AfterTestInsecure();
  AfterTestJsonData();
  AfterTestMessageEncryption();
  AfterTestPatch();
  AfterTestReliable();
  AfterTestSubsites();
  AfterTestToken();
  AfterTestWebSocket();

  // SUMMARY OF ALL THE TEST
  // ---- "---------------------------------------------- - ------
  qprintf("TOTAL number of errors after all tests are run : %d",g_errors);
}

void
MarlinServerApp::StopWebSocket()
{
  WebSocket* socket = m_httpServer->FindWebSocket("/MarlinTest/Socket/socket_123");
  if(socket)
  {
    if(socket->CloseSocket() == false)
    {
      ++g_errors;
    }
  }

}

//////////////////////////////////////////////////////////////////////////
//
// SERVICE ROUTINES FOR TESTING
//
//////////////////////////////////////////////////////////////////////////

bool doDetails = false;

// Increment the total global number of errors while testing
void xerror()
{
  ++g_errors;
}

// Suppressed printing. Only print when doDetails = true
// Any testing module can turn doDetails to 'on' or 'off'
void xprintf(const char* p_format,...)
{
  if(doDetails)
  {
    va_list varargs;
    va_start(varargs,p_format);
    CString string;
    string.FormatV(p_format,varargs);
    va_end(varargs);

    string.TrimRight("\n");

    g_analysisLog->AnalysisLog("Testing details",LogType::LOG_INFO,false,string);
  }
}

// Printing to the logfile for test results
// "String to the logfile"   -> Will be printed to logfile including terminating newline
// "Another string <+>"      -> Will be printed WITHOUT terminating newline
void qprintf(const char* p_format,...)
{
  static CString stringRegister;

  // Print variadic arguments
  va_list varargs;
  va_start(varargs,p_format);
  CString string;
  string.FormatV(p_format,varargs);
  va_end(varargs);

  // See if we must just register the string
  string.TrimRight("\n");
  if(string.Right(3) == "<+>")
  {
    stringRegister += string.Left(string.GetLength() - 3);
    return;
  }

  // Print the result to the logfile as INFO
  string = stringRegister + string;
  g_analysisLog->AnalysisLog("Testing",LogType::LOG_INFO,false,string);
  stringRegister.Empty();
}
