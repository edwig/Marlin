/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: TestMarlinServer.h : The general test server for all instrumentations
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
#pragma once
#include "WebServiceServer.h"
#include "ServerEventDriver.h"
#include "MarlinServer.h"
#include "AppConfig.h"

#define xprintf ((TestMarlinServer*)s_theServer)->Server_xprintf
#define qprintf ((TestMarlinServer*)s_theServer)->Server_qprintf
#define xerror  ((TestMarlinServer*)s_theServer)->Server_xerror

class TestMarlinServer : public WebServiceServer, MarlinServer
{
public:
  TestMarlinServer();
  virtual ~TestMarlinServer();

  virtual bool Startup()  override;   // Called from ServerMain / ServerApp
  virtual void ShutDown() override;   // Called from ServerMain / ServerApp

  // Register the objects from the ServerApp out of the IIS configuration
  void    ConfigIISServer(CString p_appName,HTTPServer* p_server,ThreadPool* p_pool,LogAnalysis* p_log);

  // Registering an extra error while running: so we can report total errors
  void  Server_xerror();
  // Printing to the log for our test set only
  void  Server_xprintf(const char* p_format, ...);
  void  Server_qprintf(const char* p_format, ...);

  CString m_socket;

  // Testing the ServerEventDriver
  void IncomingEvent(LTEvent* p_event);

protected:
  void  StartErrorReporting();
  // Reading our *.config file
  void  ReadConfig();
  // Start the alert log functionality
  void  StartAlerts();
  // Translate the configuration to the server + create URL
  void  ConfigToServer();
  // Start server log (if any)
  void  StartServerLog();
  // Starting our WSDL for our services
  void  StartWsdl();
  // Start our services
  void  StartWebServices();
  // Register all site handlers (besides SOAP handlers)
  void  RegisterSiteHandlers();
  // Stopping the server logfile
  void  StopServerLog();
  // Gathering all the test results
  void  AfterTests();

  WEBSERVICE_MAP; // Using a WEBSERVICE mapping

  // Declare all our webservice call names
  // which will translate in the On.... methods
  WEBSERVICE_DECLARE(OnMarlinFirst)
  WEBSERVICE_DECLARE(OnMarlinSecond)
  WEBSERVICE_DECLARE(OnMarlinThird)
  WEBSERVICE_DECLARE(OnMarlinFourth)
  WEBSERVICE_DECLARE(OnMarlinFifth)

  // Add service operations for WSDL and service handlers
  void AddOperations(CString p_contract);

  // Testing ServerEventDriver
  void PostEventsToDrivers();

private:

  // TESTSETS Installation
  int TestAsynchrone();
  int TestBaseSite();
  int TestBodyEncryption();
  int TestBodySigning();
  int TestCompression();
  int TestCookies();
  int TestCrackURL();
  int TestPushEvents();
  int TestFilter();
  int TestFormData();
  int TestInsecure();
  int TestJsonData();
  int TestMessageEncryption();
  int TestPatch();
  int TestReliable();
  int TestReliableBA();
  int TestSecureSite(bool p_standalone);
  int TestClientCertificate(bool p_standalone);
  int TestSubSites();
  int TestThreadPool(ThreadPool* p_pool);
  int TestHTTPTime();
  int TestToken();
  int TestWebSocket();
  int TestEventDriver();

  // AFTER THE TEST
  int  StopSubsites();
  void StopWebSocket();

  // REPORTING TEST RESULTS
  int AfterTestAsynchrone();
  int AfterTestBaseSite();
  int AfterTestBodyEncryption();
  int AfterTestBodySigning();
  int AfterTestClientCert();
  int AfterTestCompression();
  int AfterTestContract();
  int AfterTestCookies();
  int AfterTestCrackURL();
  int AfterTestEvents();
  int AfterTestEventDriver();
  int AfterTestFilter();
  int AfterTestFormData();
  int AfterTestInsecure();
  int AfterTestJsonData();
  int AfterTestMessageEncryption();
  int AfterTestPatch();
  int AfterTestReliable();
  int AfterTestSecureSite();
  int AfterTestSubSites();
  int AfterTestThreadpool();
  int AfterTestHTTPTime();
  int AfterTestToken();
  int AfterTestWebSocket();

  // SERVICE TESTING OnMarlin.....
  CString Translation(CString p_language, CString p_translation, CString p_word);
  // Set input/output languages
  CString m_language;
  CString m_translation;

  // Server
  CString         m_serverName;                         // Name of the server
  CString         m_baseURL;
  int             m_runAsService{ RUNAS_IISAPPPOOL };
  bool            m_serverSecure{ false };
  CString         m_agent;                              // Agent's name (spoofing!!)
  int             m_instance    { 1  };                 // Instance number on this machine
  ushort          m_inPortNumber{ 80 };                 // Port number of incoming port
  int             m_alertModule { -1 };                 // Server alert module
  // Others
  HPFCounter      m_counter;
  CString         m_serverLogfile;
  int             m_errors      { 0  };                 // Total number of errors
  bool            m_doDetails   { false };              // Do not log any detailed information
  bool            m_ownReport   { false };

  // Testing the event drivers
  ServerEventDriver m_driver;
  int m_channel1 { 0 };
  int m_channel2 { 0 };
  int m_channel3 { 0 };
  int m_openSeen { 0 };
  int m_closeSeen{ 0 };

  CRITICAL_SECTION  m_std_stream;
};

extern TestMarlinServer theServer;