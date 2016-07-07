#include "stdafx.h"
#include "MarlinServerApp.h"
#include "MarlinModule.h"
#include "TestServer.h"

MarlinServerApp theServer;

bool doDetails = false;
ServerApp* g_serverApp = nullptr;

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

    g_analysisLog->AnalysisLog("MarlinServerApp",LogType::LOG_INFO,false,string);
  }
}

void qprintf(const char* p_format,...)
{
  va_list varargs;
  va_start(varargs,p_format);
  CString string;
  string.FormatV(p_format,varargs);
  va_end(varargs);

  g_analysisLog->AnalysisLog("MarlinServerApp",LogType::LOG_INFO,false,string);
}

//////////////////////////////////////////////////////////////////////////
//
// The test server app
//
//////////////////////////////////////////////////////////////////////////

MarlinServerApp::MarlinServerApp()
{
  g_serverApp = dynamic_cast<ServerApp*>(this);
}

MarlinServerApp::~MarlinServerApp()
{
}

void
MarlinServerApp::InitInstance()
{
  CString contract = "http://interface.marlin.org/testing/";

  // Small local test
//   Test_CrackURL();
//   Test_HTTPTime();
//   TestThreadPool(m_appPool);

  // Starting objects and sites
//   TestWebServiceServer(m_appServer,contract);
//   TestJsonServer(m_appServer,contract);
//   TestBaseSite(m_appServer);
//   TestPushEvents(m_appServer);
  TestInsecure(m_appServer);
//   TestBodySigning(m_appServer);
//   TestBodyEncryption(m_appServer);
//   TestMessageEncryption(m_appServer);
//   TestReliable(m_appServer);
//   TestCookies(m_appServer);
//   TestToken(m_appServer);
//   TestSubSites(m_appServer);
//   TestJsonData(m_appServer);
//   TestFilter(m_appServer);
//   TestPatch(m_appServer);
//   TestFormData(m_appServer);
//   TestClientCertificate(m_appServer);
//   TestCompression(m_appServer);
}

void
MarlinServerApp::ExitInstance()
{
  // Testing the errorlog function
  m_appServer->ErrorLog(__FUNCTION__,5,"Not a real error message, but a test to see if it works :-)");

  StopSubsites(m_appServer);
}