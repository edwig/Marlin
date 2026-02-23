/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: TestMarlinServer.cpp
//
// Marlin Server: Internet server/client
// 
// Copyright (c) 2014-2024 ir. W.E. Huisman
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
#include "pch.h"
#include "TestMarlinServer.h"
#include <ServerApp.h>
#include <StdException.h>
#include <ServiceReporting.h>
#include <AppConfig.h>
#include <HTTPServerSync.h>
#include <HTTPServerMarlin.h>
#include <ErrorReport.h>
#include <HTTPLoglevel.h>
#include <AutoCritical.h>
#include <Alert.h>
#include <Version.h>
#include <WinFile.h>
#include <GetLastErrorAsString.h>

// Load product and application constants
void LoadConstants(LPCTSTR /*p_app_name*/)
{
  // These constants reside in the "ServerMain"
  APPLICATION_NAME      = _T("MarlinServer.exe");                     // Name of the application EXE file!!
  PRODUCT_NAME          = _T(MARLIN_PRODUCT_NAME);                    // Short name of the product (one word only)
  PRODUCT_DISPLAY_NAME  = _T("Service for MarlinServer tester");      // "Service for PRODUCT_NAME: <description of the service>"
  PRODUCT_COPYRIGHT     = _T("Copyright (c) 2025 ir. W.E. Huisman");  // Copyright line of the product (c) <year> etc.
  PRODUCT_VERSION       = _T(MARLIN_VERSION_NUMBER);                  // Short version string (e.g.: "3.2.0") Release.major.minor ONLY!
  PRODUCT_MESSAGES_DLL  = _T("MarlinServerMessages.dll");             // Filename of the WMI Messages dll.
  PRODUCT_SITE          = _T("/MarlinTest/");                         // Standard base URL absolute path e.g. "/MarlinServer/"
}

// In case we do **NOT** use IIS and the application factory, This is the one and only server
// This macro is defined in the project files. **NOT** in the *.h files
#ifndef MARLIN_IIS
TestMarlinServer theServer;
#include "..\Marlin\ServerMain.cpp"
#endif

#define DETAILLOG1(text)          if(MUSTLOG(HLL_LOGGING) && m_log) { DetailLog (_T(__FUNCTION__),LogType::LOG_INFO,text); }
#define SERVERSTATUSOK            _T("Server status: Running & OK")

TestMarlinServer::TestMarlinServer()
                 :WebServiceServer(_T(MARLIN_PRODUCT_NAME),_T(""),_T(""),PrefixType::URLPRE_Strong,_T(""),8)
                 ,MarlinServer()
{
  InitializeCriticalSection(&m_std_stream);
}

TestMarlinServer::~TestMarlinServer()
{
  Reset();
  DeleteCriticalSection(&m_std_stream);
  if(m_ownReport)
  {
    delete m_errorReport;
    m_errorReport = nullptr;
  }
}

bool
TestMarlinServer::Startup()
{
  bool result = false;

  // First starting point of the standalone server
  // We can wait here for a debugger to attach
  // Sleep(20 * CLOCKS_PER_SEC);

  try
  {
    // Start the clock
    m_counter.Start();

    // Start of program. Start COM+ in multi-threaded mode
    CoInitializeEx(0,COINIT_APARTMENTTHREADED);

    // Read our "*.config" file
    ReadConfig();

    // Set Error Reporting
    StartErrorReporting();

    // Configure our alert logs
    StartAlerts();

    // Translate the configuration to the server + create URL
    ConfigToServer();

    // Start server log (if any)
    StartServerLog();

    // Log is running: We are starting
    SvcReportInfoEvent(false,_T(__FUNCTION__),(XString(_T("Starting ")) + XString(PRODUCT_NAME)).GetString());

    // Starting the WSDL caching
    StartWsdl();

    // Start our services
    StartWebServices();

    // Register all site handlers (besides SOAP handlers)
    RegisterSiteHandlers();

    // From this point on we can now process incoming calls
    GetHTTPServer()->SetIsProcessing(true);

    // Ok, server is running, so log that
    SvcReportInfoEvent(false,_T(__FUNCTION__),SERVERSTATUSOK);
    m_httpServer->DetailLog(_T(__FUNCTION__),LogType::LOG_INFO,_T("\n") SERVERSTATUSOK _T("\n"));

    // Back to ServerMain to wait for the ending event
    result = true;
  }
  catch(StdException& er)
  {
    SvcReportErrorEvent(0,true,_T(__FUNCTION__),_T("Server initialization failed: %s"),er.GetErrorMessage().GetString());
  }
  catch(...)
  {
    XString error = GetLastErrorAsString();
    SvcReportErrorEvent(0,true,_T(__FUNCTION__),_T("Server initialization failed: %s"),error.GetString());
  }
  // Stop theClock
  m_counter.Stop();

  return result;
}

void
TestMarlinServer::ShutDown()
{
  // Stopping subsites in special order for testing
  StopSubsites();
  // Stopping test websocket (if any)
  StopWebSocket();

  // Get the results of our tests
  AfterTests();

  // Reset all sites and services and the logfiles
  Reset();
}


// Register the objects from the ServerApp out of the IIS configuration
void
TestMarlinServer::ConfigIISServer(const XString& p_applicationName
                                 ,HTTPServer*    p_server
                                 ,ThreadPool*    p_pool
                                 ,LogAnalysis*   p_log)
{
  m_serverName = p_applicationName;
  m_httpServer = p_server;
  m_pool       = p_pool;
  m_log        = p_log;

  m_serverOwner = false;
  m_poolOwner   = false;
  m_logOwner    = false;
}

//////////////////////////////////////////////////////////////////////////
//
// PRIVATE
//
//////////////////////////////////////////////////////////////////////////

void
TestMarlinServer::Server_xerror()
{
  ++m_errors;
}

// eXtended printf: print only if doDetails is true
void 
TestMarlinServer::Server_xprintf(LPCTSTR p_format, ...)
{
  if(m_doDetails)
  {
    AutoCritSec lock(&m_std_stream);
    XString info;

    va_list vl;
    va_start(vl,p_format);
    info.FormatV(p_format, vl);
    va_end(vl);

    info.TrimRight(_T("\n"));
    m_log->AnalysisLog(_T(__FUNCTION__),LogType::LOG_INFO,false,info.GetString());
  }
}

void 
TestMarlinServer::Server_qprintf(LPCTSTR p_format, ...)
{
  XString string;
  static XString stringRegister;

  va_list vl;
  va_start(vl, p_format);
  string.FormatV(p_format, vl);
  va_end(vl);


  // See if we must just register the string
  if (string.Right(3) == _T("<+>"))
  {
    stringRegister += string.Left(string.GetLength() - 3);
    return;
  }
  else
  {
    string.TrimRight(_T("\n"));
  }

  // Print the result to the logfile as INFO
  string = stringRegister + string;
  stringRegister.Empty();

  AutoCritSec lock(&m_std_stream);
  m_log->AnalysisLog(_T(__FUNCTION__),LogType::LOG_INFO,false,string.GetString());
}

void
TestMarlinServer::StartErrorReporting()
{
  if(m_errorReport == nullptr && m_runAsService != RUNAS_IISAPPPOOL)
  {
    WebServiceServer::m_errorReport = alloc_new ErrorReport();
    m_ownReport = true;
  }
}

// Reading our *.config file
void  
TestMarlinServer::ReadConfig()
{
  AppConfig   config;
  bool readOK = config.ReadConfig();
  XString section(SECTION_APPLICATION);

  // Read the configuration
  m_baseURL         = config.GetParameterString (section,_T("BaseURL"),_T("/"));
  m_runAsService    = config.GetParameterInteger(section,_T("RunAsService"),RUNAS_NTSERVICE);
  m_inPortNumber    = config.GetParameterInteger(section,_T("ServerPort"),INTERNET_DEFAULT_HTTPS_PORT);
  m_serverSecure    = config.GetParameterBoolean(section,_T("Secure"),true);
  m_logLevel        = config.GetParameterInteger(section,_T("ServerLogging"),HLL_ERRORS);
  m_serverLogfile   = config.GetParameterString (section,_T("ServerLog"),_T("Logfile.txt"));
  m_webroot         = config.GetParameterString (section,_T("WebRoot"),_T(""));
  m_instance        = config.GetParameterInteger(section,_T("Instance"),1);
  m_serverName      = config.GetParameterString (section,_T("Name"),PRODUCT_NAME);

  // Check instance number
  if(m_instance)
  {
    // Maximum of 100 servers on any one machine
    if(m_instance <   0) m_instance = 0;
    if(m_instance > 100) m_instance = 100;
  }
  // Registered name for the WMI logging
  _stprintf_s(g_svcname,SERVICE_NAME_LENGTH,_T("%s_%d_v%s"),PRODUCT_NAME,m_instance,PRODUCT_VERSION);

  // Port numbers cannot be under the IANA border value of 1024
  // unless... they are the default 80/443 ports
  if((m_inPortNumber != INTERNET_DEFAULT_HTTP_PORT ) &&
     (m_inPortNumber != INTERNET_DEFAULT_HTTPS_PORT) &&
     (m_inPortNumber  < 1025))
  {
    XString error;
    error.Format(_T("%s Server port [%d] does not conform to IANA rules (80,443 or greater than 1024)"),PRODUCT_NAME,m_inPortNumber);
    SvcReportErrorEvent(0,false,_T(__FUNCTION__),error);
    throw StdException(error);
  }

  // Checking the Base-URL. Minimum base URL is a one (1) char site "/x/"
  if(m_baseURL.IsEmpty() || m_baseURL.GetLength() < 3 || m_baseURL == _T("/") || m_baseURL.Left(1) != _T("/") || m_baseURL.Right(1) != _T("/"))
  {
    XString error(PRODUCT_NAME);
    error += _T(" Cannot start on an empty or illegal Base-URL");
    SvcReportErrorEvent(0,false,_T(__FUNCTION__),error);
    throw StdException(error);
  }

  // If not ok: Write to the WMI log. Our own logfile is not yet airborne
  if(readOK == false)
  {
    SvcReportErrorEvent(0,true,_T(__FUNCTION__),_T("Error reading '%s.config' file. Falling back to defaults"),PRODUCT_NAME);
  }
}

// Start the alert log functionality
void  
TestMarlinServer::StartAlerts()
{
  XString path = m_serverLogfile;
  int pos = path.ReverseFind('\\');
  if(pos > 0)
  {
    path = path.Left(pos + 1);
    path += _T("Alerts\\");
    WinFile ensure(path);
    if(ensure.CreateDirectory())
    {
      // Server registers the first module
      // Should return the 'module = 0' value
      m_alertModule = ConfigureApplicationAlerts(path);
      if(m_alertModule >= 0)
      {
        SvcReportInfoEvent(true,_T("Configured the 'Alerts' directory [%d] for the product [%s] in [%s]"),m_alertModule,PRODUCT_NAME,path.GetString());
        return;
      }
    }
  }
  SvcReportErrorEvent(0,true,_T(__FUNCTION__),_T("Cannot configure the 'Alerts' directory for the product [%s] in [%s]"),PRODUCT_NAME,path.GetString());
}

// Translate the configuration to the server + create URL
void
TestMarlinServer::ConfigToServer()
{
  XString hostname = GetHostName(HOSTNAME_FULL);

  // CONFIGURE URL / CHANNEL / NAMESPACE
  m_url.Format(_T("http%s://%s:%u%s")
               ,m_serverSecure ? _T("s") : _T("")
               ,hostname.GetString()
               ,m_inPortNumber
               ,m_baseURL.GetString());
  m_channelType = PrefixType::URLPRE_Strong;
  m_targetNamespace = DEFAULT_NAMESPACE;

  // DEPENDENT ON THE RUN MODE: CONFIGURE ONE OF THREE SERVER CONFIGURATIONS
  if(m_runAsService != RUNAS_IISAPPPOOL)
  {
    // RUNAS_STANDALONE / RUNAS_NTSERVICE
  
    // Create a threadpool
    m_pool        = alloc_new ThreadPool();
    m_poolOwner   = true;  // Do DTOR ourselves!!

    // Create a logfile
    if(m_log == nullptr)
    {
      m_log         = LogAnalysis::CreateLogfile(PRODUCT_NAME);
      m_logOwner    = true;  // Do DTOR later!
    }
    // Create a sync server or a a-synchronous server
    // Un-Comment the other if you want to test it.
    // m_httpServer  = alloc_new HTTPServerSync(m_serverName);
    m_httpServer  = alloc_new HTTPServerMarlin(m_serverName);
    m_serverOwner = true; // Do DTOR later!
  }

  // Set webroot at earliest convenient moment
  m_httpServer->SetWebroot(m_webroot);

  // Connect the error reporting
  if(!m_errorReport)
  {
    WebServiceServer::SetErrorReport(g_report);
  }
}

// Start server log (if any)
void
TestMarlinServer::StartServerLog()
{
  if(m_logLevel > HLL_NOLOG &&
     !m_serverLogfile.IsEmpty())
  {
    if(m_logLevel > HLL_HIGHEST)
    {
      m_logLevel = HLL_HIGHEST;
    }
    m_log->SetCache(100);
    m_log->SetLogRotation(true);
    m_log->SetLogLevel(m_logLevel);
    if(m_runAsService != RUNAS_IISAPPPOOL)
    {
      m_log->SetLogFilename(m_serverLogfile);
    }
    // Only open it, if not yet opened by another session
    m_log->AnalysisLog(_T(__FUNCTION__),LogType::LOG_INFO, false,_T("*** MarlinServer logfile ****"));
    m_httpServer->SetLogging(m_log);
    m_log->ForceFlush();
  }
  else if (m_logLevel <= 0)
  {
    StopServerLog();
  }
}

void
TestMarlinServer::StopServerLog()
{
  m_log->Reset();
  m_logLevel = HLL_NOLOG;
}

void
TestMarlinServer::StartWsdl()
{
  // From TestContract.cpp
  AddOperations(DEFAULT_NAMESPACE);
}

// Start our services
void  
TestMarlinServer::StartWebServices()
{
  // Make config setting
  SetJsonSoapTranslation(true);


  // Try running the service
  if(RunService())
  {
    qprintf(_T("WebServiceServer [%s] is now running OK\n"), m_serverName.GetString());
    qprintf(_T("Running contract      : %s\n"), DEFAULT_NAMESPACE);
    qprintf(_T("For WSDL download use : %s%s.wsdl\n"),m_baseURL.GetString(),m_serverName.GetString());
    qprintf(_T("For interface page use: %s%s%s\n"),   m_baseURL.GetString(),m_serverName.GetString(),GetServicePostfix().GetString());
    qprintf(_T("\n"));
  }
  else
  {
    xerror();
    qprintf(_T("ERROR Starting WebServiceServer for: %s\n"),m_serverName.GetString());
    qprintf(_T("ERROR Reported by the server: %s\n"),GetErrorMessage().GetString());
  }
}

// Register all site handlers (besides SOAP handlers)
void
TestMarlinServer::RegisterSiteHandlers()
{
  // All handlers from the test set
  TestAsynchrone();
  TestBaseSite();
  TestBodyEncryption();
  TestBodySigning();
  TestCookies();
  TestCrackURL();
  TestPushEvents();
  TestEventDriver();
  TestFilter();
  TestFormData();
  // Sites
  TestInsecure();
  TestSecureSite       (m_runAsService != RUNAS_IISAPPPOOL);
  TestClientCertificate(m_runAsService != RUNAS_IISAPPPOOL);
  TestJsonData();
  TestPatch();
  TestChunking();
  TestCompression();
  TestMessageEncryption();
  TestReliable();
  TestReliableBA();
  TestThreadPool(m_pool);
  TestHTTPTime();
  TestToken();
  TestSubSites();
  TestWebSocket();
  TestWebSocketSecure();
}

// Perform all after test reporting
void
TestMarlinServer::AfterTests()
{
  AfterTestEventDriver();
  AfterTestAsynchrone();
  AfterTestBaseSite();
  AfterTestBodyEncryption();
  AfterTestBodySigning();
  AfterTestContract();
  AfterTestCookies();
  AfterTestCrackURL();
  AfterTestEvents();
  AfterTestFilter();
  AfterTestFormData();
  AfterTestInsecure();
  AfterTestSecureSite();
  AfterTestClientCert();
  AfterTestJsonData();
  AfterTestPatch();
  AfterTestChunking();
  AfterTestCompression();
  AfterTestMessageEncryption();
  AfterTestReliable();
  AfterTestThreadpool();
  AfterTestHTTPTime();
  AfterTestToken();
  AfterTestSubSites();
  AfterTestWebSocket();
  AfterTestWebSocketSecure();
}
