/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: TestClient.cpp
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
#include "TestClient.h"
#include "HTTPClient.h"
#include "LogAnalysis.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// The one and only application object
CWinApp theApp;

using namespace std;

bool doDetails = false;
int  MARLIN_SERVER_PORT = 80;

void xprintf(const char* p_format,...)
{
  if(doDetails)
  {
    va_list vl;
    va_start(vl,p_format);
    vprintf(p_format,vl);
    va_end(vl);
  }
}

void
WaitForKey()
{
  // Wait for key to occur
  // so the messages can be send and debugged :-)
  char buffer[256];
  buffer[0] = 0;
  gets_s(buffer,255);
}

CString hostname("localhost");
static LogAnalysis* g_log = nullptr;

HTTPClient*
StartClient(int p_loglevel)
{
  CString logfileName = MarlinConfig::GetExePath() + "ClientLog.txt";

  // Create log file and turn logging 'on'
  g_log = new LogAnalysis("TestHTTPClient");
  g_log->SetLogFilename(logfileName,true);
  g_log->SetLogLevel(p_loglevel);
  g_log->SetDoTiming(true);
  g_log->SetDoEvents(false);
  g_log->SetCache(1000);

  // Create client and connect logging object
  HTTPClient* client = new HTTPClient();
  client->SetLogging(g_log);
  client->SetLogLevel(p_loglevel);

  return client;
}

void
StopClient(HTTPClient* p_client)
{
  // Stop the client
  p_client->StopClient();
  delete p_client;

  // Stop the logfile
  g_log->Reset();
  delete g_log;
  g_log = nullptr;
}

int main(int argc, TCHAR* argv[], TCHAR* /*envp[]*/)
{
  int nRetCode = 0;
  int errors   = 0;
  int loglevel = HLL_TRACEDUMP; // HLL_ERRORS / HLL_LOGGING / HLL_LOGBODY / HLL_TRACE / HLL_TRACEDUMP

  HMODULE hModule = ::GetModuleHandle(NULL);
  if(hModule == NULL)
  {
    printf("Fatal Error: GetModuleHandle failed\n");
    nRetCode = 1;
    return nRetCode;
  }
  _set_se_translator(SeTranslator);

  // initialize MFC and print an error on failure
  if (!AfxWinInit(hModule, NULL, ::GetCommandLine(), 0))
  {
    printf("Fatal Error: MFC initialization failed\n");
    nRetCode = 1;
  }
  else
  {
    printf("TESTING THE MARLIN CLIENT %s\n",CLIENT_CONFIG);
    printf("====================================\n");
    printf("\n");

    HTTPClient* client = StartClient(loglevel);

    if(argc >= 2 && _stricmp(argv[1],"/ws") == 0)
    {
      // Testing WebServiceClient standalone
      errors += TestContract(nullptr,false,false);
    }
    else
    {
      // Do Unit testing of the client without connecting to the internet
      errors += TestUnicode();
      errors += TestURLChars();
      errors += TestXML();
      errors += TestCryptography();
      errors += TestReader();
      errors += TestConvert();
      errors += TestNamespaces();
      errors += TestHeaders();
      errors += TestJSON();
      errors += TestFindClientCertificate();
      errors += TestCookiesOverwrite();
      errors += TestDecryptCookie();
//    errors += TestMSGraph(client);
      errors += TestBaseSite(client);
      errors += TestSecureSite(client);
      errors += TestClientCertificate(client);
      errors += TestChunkedTransfer(client);
#ifdef TEST_WEBSOCKETS
      errors += TestWebSocketAccept();
      errors += TestWebSocket(g_log);
      errors += TestCloseWebSocket();
#endif
      errors += TestEvents(client);
      errors += TestEventDriver(g_log);
      errors += TestCookies(*client);
      errors += TestFormData(client);
      errors += TestJsonData(client);
      errors += TestContract(client,true, false);  // JSON No authentication
      errors += TestContract(client,false,false);  // WS   No authentication
      errors += TestContract(client,false,true);   // WS   WS-Secure token-profile
      errors += TestPatching(client);
      errors += TestCompression(client);
      errors += TestWebservices(*client);
    }

    printf("\n");
    printf("SUMMARY OF ALL CLIENT TESTS\n");
    printf("===========================\n");
    if(errors)
    {
      printf("ERRORS: %d\n",nRetCode = errors);
    }
    else
    {
      printf("ALL OK !!!! YIPEEEE!!!!\n");
    }

    printf("\nRead everything? ");
    WaitForKey();

    // And stop the client
    StopClient(client);
  }
  // Ready with MFC
  AfxWinTerm();

  return nRetCode;
}
