/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: TestEventDriver.cpp
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
#include "stdafx.h"
#include "TestClient.h"
#include "HTTPClient.h"
#include "ClientEventDriver.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

ThreadPool         g_pool;
ClientEventDriver* g_driver     = nullptr;
int                g_msgSeen    = 0;
int                g_openSeen   = 0;
int                g_closeSeen  = 0;
int                g_errorSeen  = 0;
int                g_binarySeen = 0;

// Set this one to true to see all logging
extern bool doDetails;

void CEDCallback(void* p_object, LTEvent* p_event)
{
  UNREFERENCED_PARAMETER(p_object);

  XString text;
  XString type = LTEvent::EventTypeToString(p_event->m_type);
  text.Format(_T("Server Event: [%d:%s] %s"),p_event->m_number,type.GetString(),p_event->m_payload.GetString());
  xprintf(_T("%s\n"),text.GetString());

  // Count the number of messages for completion of the test
  switch(p_event->m_type)
  {
    case EvtType::EV_Open:    g_openSeen++;   break;
    case EvtType::EV_Message: g_msgSeen++;    break;
    case EvtType::EV_Binary:  g_binarySeen++; break;
    case EvtType::EV_Error:   g_errorSeen++;  break;
    case EvtType::EV_Close:   g_closeSeen++;  break;
  }
  // Done with this event
  delete p_event;
}

int
TestEventDriver(LogAnalysis* p_log)
{
  int errors = 0;

  doDetails = false;

  // This is what we are going to test
  xprintf(_T("TESTING ClientEventDriver INTERFACE FOR APPLICATIONS\n"));
  xprintf(_T("====================================================\n"));

  XString url;
  url.Format(_T("http://%s:%d/MarlinTest/Driver/"),MARLIN_HOST,TESTING_HTTP_PORT);

  // Run the threadpool
  g_pool.TrySetMinimum(4);
  g_pool.TrySetMaximum(20);
  g_pool.Run();

  // Set the driver: Normally this would NOT be a static member!!
  // Normally th second parameter would be the owning objects 'this' pointer
  g_driver = new ClientEventDriver();
  g_driver->SetApplicationCallback(CEDCallback,g_driver);
  g_driver->SetThreadPool(&g_pool);
  g_driver->SetLogfile(p_log);

  // Setting server URL and Channel Policy
  g_driver->SetServerURL(url);
  g_driver->SetChannelPolicy(EVChannelPolicy::DP_HighSecurity); // TESTING SSE

  // Starting the driver
  XString session(_T("secondsession_456"));
  XString cookie(_T("GUID"));
  XString token(_T("456-456-456-456"));

  bool result = g_driver->StartEventsForSession(session,cookie,token);
  // SUMMARY OF THE TEST
  // --- "---------------------------------------------- - ------
  _tprintf(_T("Starting the ClientEventDriver + session 2     : %s\n"), result ? _T("OK") : _T("ERROR"));
  errors += !result;

  // Wait for first 20 messages
  int waiting = 100;
  while(g_msgSeen < 20)
  {
    Sleep(100);
    if(--waiting <= 0) break;
  }
  // --- "---------------------------------------------- - ------
  _tprintf(_T("All SSE events are received                    : %s\n"), g_msgSeen == 20 ? _T("OK") : _T("ERROR"));
  errors += g_msgSeen != 20;
  g_msgSeen = 0;

  // change policy BEFORE restarting !!!!
  g_driver->SetChannelPolicy(EVChannelPolicy::DP_Binary);
  g_driver->StopEventsForSession();

  // Next session is for WebSockets
  session = _T("firstsession_123");
  token   = _T("123-123-123-123");
  result  = g_driver->StartEventsForSession(session,cookie,token);
  
  // --- "---------------------------------------------- - ------
  _tprintf(_T("Starting the ClientEventDriver + session 1     : %s\n"), result ? _T("OK") : _T("ERROR"));

  // Wait for second set of 20 messages
  waiting = 100; // 
  while(g_msgSeen < 20) 
  {
    Sleep(100);
    if(--waiting <= 0) break;
  }
  // --- "---------------------------------------------- - ------
  _tprintf(_T("All WebSocket events are received              : %s\n"), g_msgSeen == 20 ? _T("OK") : _T("ERROR"));
  errors += g_msgSeen != 20;
  g_msgSeen = 0;

  g_driver->SetChannelPolicy(EVChannelPolicy::DP_Disconnected);
  g_driver->StopEventsForSession();

  // Next session is for Polling
  session = _T("thirdsession_789");
  token   = _T("789-789-789-789");
  result  = g_driver->StartEventsForSession(session,cookie,token);
  // --- "---------------------------------------------- - ------
  _tprintf(_T("Starting the ClientEventDriver + session 3     : %s\n"), result ? _T("OK") : _T("ERROR"));

  // Wait for third set of 20 messages
  waiting = 100; // 
  while (g_msgSeen < 20)
  {
    Sleep(100);
    if (--waiting <= 0) break;
  }
  // --- "---------------------------------------------- - ------
  _tprintf(_T("All LongPolling events are received            : %s\n"), g_msgSeen == 20 ? _T("OK") : _T("ERROR"));
  errors += (g_msgSeen != 20);
  g_msgSeen = 0;

  _tprintf(_T("All channel 'open' events are received         : %s\n"), g_openSeen == 3 ? _T("OK") : _T("ERROR"));
  errors += (g_openSeen != 3);
  g_openSeen = 0;

  _tprintf(_T("All channel 'close' events are received        : %s\n"), g_closeSeen >= 3 ? _T("OK") : _T("ERROR"));
  errors += (g_closeSeen < 3);
  g_openSeen = 0;

  // Delete test driver 
  delete g_driver;
  g_driver = nullptr;

  doDetails = false;

  return errors;
}
