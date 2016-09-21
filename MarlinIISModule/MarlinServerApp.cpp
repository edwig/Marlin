#include "stdafx.h"
#include "MarlinServerApp.h"
#include "MarlinModule.h"
#include "TestServer.h"

// The one and only server object
MarlinServerApp theServer;

bool doDetails = false;

// In TestServer.cpp
void xerror()
{
  theServer.IncrementError();
}

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

void qprintf(const char* p_format,...)
{
  static CString stringRegister;

  va_list varargs;
  va_start(varargs,p_format);
  CString string;
  string.FormatV(p_format,varargs);
  va_end(varargs);

  string.TrimRight("\n");
  if(string.Right(3) == "<+>")
  {
    stringRegister = string.Left(string.GetLength() - 3);
    return;
  }
  string = stringRegister + string;

  g_analysisLog->AnalysisLog("Testing",LogType::LOG_INFO,false,string);
  stringRegister.Empty();
}

//////////////////////////////////////////////////////////////////////////
//
// The test server app
//
//////////////////////////////////////////////////////////////////////////

MarlinServerApp::MarlinServerApp()
{
}

MarlinServerApp::~MarlinServerApp()
{
}

void
MarlinServerApp::InitInstance()
{
  CString contract = "http://interface.marlin.org/testing/";

  // Can only be called once if correctly started
  if(!CorrectStarted() || m_running)
  {
    return;
  }

  // Small local test
  Test_CrackURL();
  Test_HTTPTime();
  TestThreadPool(m_appPool);

  // Starting objects and sites
  //   TestWebServiceServer(m_appServer,contract);
  //   TestJsonServer(m_appServer,contract);
  //   TestPushEvents(m_appServer);
// OK: TestSecureSite(m_appServer);
  TestClientCertificate(m_appServer);
// OK:  TestBaseSite(m_appServer);
// OK:  TestInsecure(m_appServer);
// OK:  TestBodySigning(m_appServer);
// OK:  TestBodyEncryption(m_appServer);
// OK:  TestMessageEncryption(m_appServer);
// OK:  TestReliable(m_appServer);
// OK:  TestCookies(m_appServer);
// OK:  TestToken(m_appServer);
// OK:  TestSubSites(m_appServer);
// OK:  TestJsonData(m_appServer);
// OK:  TestFilter(m_appServer);
// OK:  TestPatch(m_appServer);
// OK:  TestFormData(m_appServer);
// OK:  TestCompression(m_appServer);
// OK:  TestAsynchrone(m_appServer);

  // Instance is now running
  m_running = true;
}

void
MarlinServerApp::ExitInstance()
{
  if(m_running)
  {
    // Testing the errorlog function
    m_appServer->ErrorLog(__FUNCTION__,5,"Not a real error message, but a test to see if the errorlog works :-)");

    // Tell the log how many errors where detected on this testrun
    m_appServer->DetailLogV(__FUNCTION__,LogType::LOG_INFO,"Total server errors: %d",m_errors);

    // Stopping all subsites
    StopSubsites(m_appServer);

    // Report all tests
    ReportAfterTesting();

    // Stopped running
    m_running = false;
  }
}

bool
MarlinServerApp::CorrectStarted()
{
  if(m_correctInit == false)
  {
    qprintf("Server instance incorrectly started. Review your IIS application pool settings!");
    return false;
  }
  return true;
}

void 
MarlinServerApp::ReportAfterTesting()
{
  AfterSecureSite();
}
