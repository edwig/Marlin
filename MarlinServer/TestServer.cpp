/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: TestServer.cpp
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
// TestServer.cpp : Defines the entry point for the console application.
//
#include "stdafx.h"
#include "TestServer.h"
#include "CrackURL.h"
#include "CreateURLPrefix.h"
#include "ThreadPool.h"
#include "HTTPServer.h"
#include "HTTPThreadPool.h"
#include "SOAPMessage.h"
#include "GetLastErrorAsString.h"
#include "PrintToken.h"
#include "Analysis.h"

#define  NOGDI
#define  NOMINMAX
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <io.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <objbase.h>
#include <wtypes.h>
#include <http.h>

#include <winerror.h>
#include <conio.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// Global objects of this test program
int  totalErrors = 0;
bool doDetails   = false;

// eXtended Printf: print only if doDetails is true
void xprintf(const char* p_format, ...)
{
  if (doDetails)
  {
    va_list vl;
    va_start(vl, p_format);
    vprintf(p_format, vl);
    va_end(vl);
  }
}

// Record an error from on of the test functions
void xerror()
{
  ++totalErrors;
}

// Wait for key to occur
// so the messages can be send and debugged :-)
void
WaitForKey()
{
  char buffer[256];
  size_t readIn = 0;
  _cgets_s(buffer, 256, &readIn);
}

// Setting the filename of the logfile
#ifdef _DEBUG
#ifdef _M_X64
CString g_baseDir = "..\\BinDebug_x64\\";
#else
CString g_baseDir = "..\\BinDebug_x32\\";
#endif // _M_X64
#else // _DEBUG
#ifdef _M_X64
CString g_baseDir = "..\\BinRelease_x64\\";
#else
CString g_baseDir = "..\\BinRelease_x32\\";
#endif // _M_X64
#endif // _DEBUG

ErrorReport g_errorReport;

// Starting our server
//
bool StartServer(HTTPServer*&     p_server
                ,HTTPThreadPool*& p_pool
                ,LogAnalysis*&    p_logfile)
{
  // This is the name of our test server
  CString naam("MarlinServer");

  // Start the base objects
  p_pool    = new HTTPThreadPool(4,10);
  p_server  = new HTTPServer(naam);
  p_logfile = new LogAnalysis(naam);

  // Set the logfile
  CString logfileName = g_baseDir + "ServerLog.txt";

  // Put a logfile on the server
  p_logfile->SetLogFilename(logfileName,false);
  p_logfile->SetDoLogging(true);
  p_logfile->SetDoEvents(false);
  p_logfile->SetDoTiming(true);
  p_logfile->SetCache(500);
  p_logfile->SetLogRotation(true);
  p_server->SetDetailedLogging(true);
  // connect
  p_server->SetLogging(p_logfile);

  // SETTING THE MANDATORY MEMBERS

  p_server->SetThreadPool(p_pool);               // Threadpool
  p_server->SetWebroot("C:\\WWW\\Marlin\\");     // WebConfig overrides
  p_server->SetErrorReport(&g_errorReport);      // Error reporting

  bool result = false;
  if(p_server->GetLastError() == NO_ERROR)
  {
    // Alternate method: Test running in this thread
    // server->RunHTTPServer();

    // Preferred method: Test in separate thread
    p_server->Run();

    // Wait max 3 seconds for the server to enter the main loop
    // After this the initialization of the server has been done
    for(int ind = 0; ind < 30; ++ind)
    {
      if(p_server->GetIsRunning())
      {
        result = true;
        break;
      }
      Sleep(100);
    }
  }
  return result;
}

// Stopping the server
void 
CleanupServer(HTTPServer*&     p_server
             ,HTTPThreadPool*& p_pool
             ,LogAnalysis*&    p_logfile)
{
  if(p_server)
  {
    delete p_server;
  }
  if(p_pool)
  {
    delete p_pool;
  }
  if(p_logfile)
  {
    delete p_logfile;
  }
}

// Our main test program
int 
_tmain(int argc,TCHAR* argv[], TCHAR* /*envp[]*/)
{
  int nRetCode = 0;

  HMODULE hModule = ::GetModuleHandle(NULL);

  if(hModule == NULL)
  {
    _tprintf(_T("Fatal Error: GetModuleHandle failed\n"));
    nRetCode = 1;
  }
  else
  {
    // initialize MFC and print and error on failure
    if (!AfxWinInit(hModule, NULL, ::GetCommandLine(), 0))
    {
      _tprintf(_T("Fatal Error: MFC initialization failed\n"));
      nRetCode = 1;
    }
    else
    {
      printf("TESTPROGAM: MARLIN SERVER\n");
      printf("=========================\n");
      printf("\n");
      printf("Version string: %s\n",MARLIN_SERVER_VERSION);
      printf("----------------------------------\n");
      printf("\n");

      // See if we must do the standalone WebServiceServer test
      // Or that we should do the flat HTTPServer tests
      if(argc >= 2)
      {
        if(_stricmp(argv[1],"/ws") == 0)
        {
          CString contract = "http://interface.marlin.org/testing/";
          printf("WebServiceServer test for \"%s\"\n",contract.GetString());
          printf("----------------------------------------------------------------\n");
          printf("\n");

          // Test the Interface
          nRetCode = TestWebServiceServer(NULL,contract);
        }
      }
      else
      {
        HTTPServer*     server  = nullptr;
        HTTPThreadPool* pool    = nullptr;
        LogAnalysis*    logfile = nullptr;

        if(StartServer(server,pool,logfile))
        {
          // Fire up all of our test sites
          int errors = 0;

          // Individual tests
          errors += Test_CrackURL();
          errors += Test_HTTPTime();

          errors += TestThreadPool(pool);

          // Push events interface (SSE)
          errors += TestPushEvents(server);

          // Cookies test for HTTP protocol (HTTP PUT)
          errors += TestCookies(server);

          // Test 'normal' SOAP messages without further protocol
          errors += TestInsecure(server);

          // Test 'async' SOAP Messages
          errors += TestAsynchrone(server);

          // Test 'body signing' capabilities of the SOAP message
          errors += TestBodySigning(server);

          // Test 'body encryption' capabilities
          errors += TestBodyEncryption(server);

          // Testing 'whole message encryption' capabilities
          errors += TestMessageEncryption(server);

          // Testing 'WS-ReliableMessaging protocol' of the site
          errors += TestReliable(server);

          // Testing the token functionality
          errors += TestToken(server);

          // Testing a base site for file gets
          errors += TestBaseSite(server);

          // Testing the sub-site functions
          // Call **AFTER** registering /MarlinTest/TestToken/
          errors += TestSubSites(server);

          // Testing the JSON functionality in this data site
          errors += TestJsonData(server);

          // Testing the Site Filtering capabilities
          errors += TestFilter(server);

          // Testing the HTTP VERB Tunneling
          errors += TestPatch(server);

          // Testing Form-data MultiPart-Buffering
          errors += TestFormData(server);

          // Testing the client-certificate request
          errors += TestClientCertificate(server);

          // Testing the GZIP compression function
          errors += TestCompression(server);

          // Test the WebServiceServer program generation
          CString contract = "http://interface.marlin.org/testing/";
          errors += TestJsonServer(server,contract);
          errors += TestWebServiceServer(server,contract);

          // See if we should wait for testing to occur
          if(errors)
          {
            printf("\n"
                   "SERVER (OR PROGRAMMING) IN ERROR STATE!!\n"
                   "%d sites not correctly started\n"
                   "\n",errors);
          }
          else
          {
            printf("\n"
                   "Server running....\n"
                   "Waiting to be called by test clients...\n"
                   "\n");
            // Wait for key to occur
            WaitForKey();
          }

          // Try to stop the subsites
          errors += StopSubsites(server);

          // Testing the errorlog function
          server->ErrorLog(__FUNCTION__,5,"Not a real error message, but a test to see if it works :-)");

          printf("Stopping the server\n");
          server->StopServer();

          // See if the server is indeed in stopped state
          printf("The server is %s\n",server->GetIsRunning() ? "still running!\n" : "stopped.\n");

          // Remember for a cmd shell
          nRetCode = errors;
        }
        else
        {
          totalErrors = 1;
          printf("HTTPServer in error state in : Error %lu: %s\n"
                 ,server->GetLastError()
                 ,(LPCTSTR)GetLastErrorAsString(GetLastError()));
        }
        CleanupServer(server,pool,logfile);
      }
    }
    printf("\n");
    printf("SUMMARY OF ALL SERVER TESTS\n");
    printf("===========================\n");
    if(totalErrors)
    {
      printf("ERRORS: %d\n",nRetCode += totalErrors);
    }
    else
    {
      printf("ALL OK !!!! YIPEEEE!!!!\n");
    }
    WaitForKey();
    WaitForKey();
  }
  return nRetCode;
}

