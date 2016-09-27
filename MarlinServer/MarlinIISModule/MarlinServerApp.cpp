#include "stdafx.h"
#include "MarlinServerApp.h"
#include "MarlinModule.h"
#include "TestServer.h"

// The one and only server object
MarlinServerApp theServer;

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
  if(!CorrectlyStarted() || m_running)
  {
    return;
  }
  // Instance is now running
  m_running = true;

  // Small local test
  Test_CrackURL();
  Test_HTTPTime();
//TestThreadPool(m_appPool);

  // Starting objects and sites
  TestPushEvents(m_appServer);
  TestWebServiceServer(m_appServer,contract);
  TestJsonServer(m_appServer,contract);
  TestSecureSite(m_appServer);
  TestClientCertificate(m_appServer);
  TestCookies(m_appServer);
  TestInsecure(m_appServer);
  TestBaseSite(m_appServer);
  TestBodySigning(m_appServer);
  TestBodyEncryption(m_appServer);
  TestMessageEncryption(m_appServer);
  TestReliable(m_appServer);
  TestToken(m_appServer);
  TestSubSites(m_appServer);
  TestJsonData(m_appServer);
  TestFilter(m_appServer);
  TestPatch(m_appServer);
  TestFormData(m_appServer);
  TestCompression(m_appServer);
  TestAsynchrone(m_appServer);
}

void
MarlinServerApp::ExitInstance()
{
  if(m_running)
  {
    // Testing the errorlog function
    m_appServer->ErrorLog(__FUNCTION__,0,"Not a real error message, but a test to see if the errorlog works :-)");

    // Stopping all subsites
    StopSubsites(m_appServer);

    // Report all tests
    ReportAfterTesting();

    // Stopped running
    m_running = false;
  }
}

bool
MarlinServerApp::CorrectlyStarted()
{
  if(ServerApp::CorrectlyStarted() == false)
  {
    qprintf("ServerApp incorrectly started. Review your program logic");
    return false;
  }
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
  AfterTestCrackURL();
  AfterTestTime();
//AfterTestThreadpool();
  AfterTestSecureSite();
  AfterTestBaseSite();
  AfterTestClientCert();
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

  // SUMMARY OF ALL THE TEST
  // ---- "---------------------------------------------- - ------
  qprintf("TOTAL number of errors after all tests are run : %d",m_errors);
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
  theServer.IncrementError();
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

// Printing to the logfile for testresults
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
