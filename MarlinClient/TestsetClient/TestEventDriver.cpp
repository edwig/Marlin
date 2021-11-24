/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: TestEventDriver.cpp
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
#include "ClientEventDriver.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

ThreadPool         g_pool;
ClientEventDriver* g_driver = nullptr;
int                g_number = 0;

// Set this one to true to see all logging
extern bool doDetails;

void CEDCallback(void* p_object, LTEvent* p_event)
{
  UNREFERENCED_PARAMETER(p_object);

  CString text;
  CString type = LTEvent::EventTypeToString(p_event->m_type);
  text.Format("Server Event: [%d:%s] %s",p_event->m_number,type.GetString(),p_event->m_payload.GetString());
  xprintf("%s\n",text.GetString());

  // Count the number of 'normal' messages for completion of the test
  if(p_event->m_type == EvtType::EV_Message)
  {
    ++g_number;
  }

  // Done with this event
  delete p_event;
}

int
TestEventDriver(LogAnalysis* p_log)
{
  int errors = 0;

  // This is what we are going to test
  xprintf("TESTING ClientEventDriver INTERFACE FOR APPLICATIONS\n");
  xprintf("====================================================\n");

  CString url;
  url.Format("http://%s:%d/MarlinTest/Driver/",MARLIN_HOST,TESTING_HTTP_PORT);

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
  CString session("secondsession_456");
  CString cookie("GUID");
  CString token("456-456-456-456");

  bool result = g_driver->StartEventsForSession(session,cookie,token);
  // SUMMARY OF THE TEST
  // --- "---------------------------------------------- - ------
  printf("Starting the ClientEventDriver + session 2     : %s\n", result ? "OK" : "ERROR");

  // Wait for first 20 messages
  int waiting = 100;
  while(g_number < 20)
  {
    Sleep(100);
    if(--waiting <= 0) break;
  }
  // --- "---------------------------------------------- - ------
  printf("All SSE events are received                    : %s\n", g_number == 20 ? "OK" : "ERROR");
  g_number = 0;

  // change policy BEFORE restarting !!!!
  g_driver->SetChannelPolicy(EVChannelPolicy::DP_Binary);
  g_driver->StopEventsForSession();

  // Next session is for WebSockets
  session = "firstsession_123";
  token   = "123-123-123-123";
  result  = g_driver->StartEventsForSession(session,cookie,token);
  
  // --- "---------------------------------------------- - ------
  printf("Starting the ClientEventDriver + session 1     : %s\n", result ? "OK" : "ERROR");

  // Wait for second set of 20 messages
  waiting = 100; // 
  while(g_number < 20) 
  {
    Sleep(100);
    if(--waiting <= 0) break;
  }
  // --- "---------------------------------------------- - ------
  printf("All WebSocket events are received              : %s\n", g_number == 20 ? "OK" : "ERROR");
  g_number = 0;

  g_driver->SetChannelPolicy(EVChannelPolicy::DP_Disconnected);
  g_driver->StopEventsForSession();

  // Next session is for Polling
  session = "thirdsession_789";
  token   = "789-789-789-789";
  result  = g_driver->StartEventsForSession(session,cookie,token);
  // --- "---------------------------------------------- - ------
  printf("Starting the ClientEventDriver + session 3     : %s\n", result ? "OK" : "ERROR");

  // Wait for third set of 20 messages
  waiting = 100; // 
  while (g_number < 20)
  {
    Sleep(100);
    if (--waiting <= 0) break;
  }
  // --- "---------------------------------------------- - ------
  printf("All LongPolling events are received            : %s\n", g_number == 20 ? "OK" : "ERROR");
  g_number = 0;

  // Delete test driver 
  delete g_driver;
  g_driver = nullptr;

  return errors;
}
