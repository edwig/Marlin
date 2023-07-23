/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: TestEventDriver.cpp
//
// Marlin Server: Internet server/client
// 
// Copyright (c) 2014-2022 ir. W.E. Huisman
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
#include "TestMarlinServer.h"
#include "HTTPServer.h"
#include "HTTPSite.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define NUM_TEST 20

void EventCallback(void* p_data)
{
  LTEvent* event = reinterpret_cast<LTEvent*>(p_data);
  if (event)
  {
    TestMarlinServer* server = reinterpret_cast<TestMarlinServer*>(event->m_sent);
    if(server)
    {
      server->IncomingEvent(event);
    }
    delete event;
  }
}

void
TestMarlinServer::IncomingEvent(LTEvent* p_event)
{
  XString text;
  XString type;
  switch (p_event->m_type)
  {
    case EvtType::EV_Message: type = "Message"; 
                              break;
    case EvtType::EV_Binary:  type = "Binary";
                              break;
    case EvtType::EV_Error:   type = "Error";
                              break;
    case EvtType::EV_Open:    type = "Open";    
                              m_openSeen++;
                              break;
    case EvtType::EV_Close:   type = "Close";
                              m_closeSeen++;
                              break;
  }
  text.Format("Incoming event [%s] Number [%d] ",type.GetString(),p_event->m_number);
  text += p_event->m_payload;
  qprintf(text);

  PostEventsToDrivers();
}

void
TestMarlinServer::PostEventsToDrivers()
{
  static bool mustsent = true;

  if(mustsent)
  {
    XString payload;
    for(int channel = 1; channel <= 3; ++channel)
    {
      for(int ind = 0; ind < NUM_TEST; ++ind)
      {
        payload.Format("Testing event number [%d] to channel [%d]",ind,channel);
        switch (channel)
        {
          case 1: m_driver.PostEvent(m_channel1, payload); break;
          case 2: m_driver.PostEvent(m_channel2, payload); break;
          case 3: m_driver.PostEvent(m_channel3, payload); break;
        }
      }
    }
  }
  mustsent = false;
}

int
TestMarlinServer::TestEventDriver()
{
  int error = 0;
  XString url("/MarlinTest/Driver/");

  xprintf("TESTING SERVER-EVENT-DRIVER FUNCTIONS\n");
  xprintf("=====================================\n");

  // Create URL channel to listen to "http://+:port/MarlinTest/Driver/"
  HTTPSite* site = m_httpServer->CreateSite(PrefixType::URLPRE_Strong, false, m_inPortNumber, url);
  if (site)
  {
    // SUMMARY OF THE TEST
    // --- "---------------------------------------------- - ------
    qprintf("HTTPSite for ServerEventDriver                : OK : %s\n", site->GetPrefixURL().GetString());
  }
  else
  {
    ++error;
    xerror();
    qprintf("ERROR: Cannot make a HTTP site for: %s\n",url.GetString());
    return error;
  }

  // Register the URL for starting sessions
  m_driver.RegisterSites(m_httpServer,site);

  // In real life we would generate the session names and the cookie values!!
  // these values make the debugging easier in the test application
  m_channel1 = m_driver.RegisterChannel("firstsession_123", "GUID","123-123-123-123");
  m_channel2 = m_driver.RegisterChannel("secondsession_456","GUID","456-456-456-456");
  m_channel3 = m_driver.RegisterChannel("thirdsession_789", "GUID","789-789-789-789");

  // Connect the channel to an application object
  // Here we connect it to the server with the 'this' pointer.
  // In a real application these would be session objects of some kind.
  bool result1 = m_driver.SetChannelPolicy(m_channel1,EVChannelPolicy::DP_SureDelivery,EventCallback,(UINT64)this);  // WebSocket
  bool result2 = m_driver.SetChannelPolicy(m_channel2,EVChannelPolicy::DP_SureDelivery,EventCallback,(UINT64)this);  // SSE
  bool result3 = m_driver.SetChannelPolicy(m_channel3,EVChannelPolicy::DP_SureDelivery,EventCallback,(UINT64)this);  // Long Polling

  // Count our blessings :-)
  if(m_channel1 == 0) ++error;
  if(m_channel2 == 0) ++error;
  if(m_channel3 == 0) ++error;
  if(!result1) ++error;
  if(!result2) ++error;
  if(!result3) ++error;

  if(!m_driver.StartEventDriver())
  {
    ++error;
  }

  // SUMMARY OF THE START
  // --- "---------------------------------------------- - ------
  qprintf("ServerEventDriver registered channels         : %s\n", error ? "ERROR" : "OK");

  return error;
}

int
TestMarlinServer::AfterTestEventDriver()
{
  int size1 = m_driver.GetChannelQueueCount(m_channel1);
  int size2 = m_driver.GetChannelQueueCount(m_channel2);
  int size3 = m_driver.GetChannelQueueCount(m_channel3);

  // All messages must be delivered to the client!
  int errors = size1 + size2 + size3;

  // Delete the channels
  errors += !m_driver.UnRegisterChannel(m_channel1);
  errors += !m_driver.UnRegisterChannel(m_channel2);
  errors += !m_driver.UnRegisterChannel(m_channel3);

  // Stopping the event driver must work
  errors += !m_driver.StopEventDriver();

  // SUMMARY OF THE TEST
  // ---- "---------------------------------------------- - ------
  qprintf("ServerEventDriver all messages delivered [%d]   : %s\n",errors,errors > 0 ? "ERROR" : "OK");
  qprintf("ServerEventDriver all open messages delivered  : %s\n", m_openSeen    < 3 ? "ERROR" : "OK");
  qprintf("ServerEventDriver all close messages delivered : %s\n", m_closeSeen   < 3 ? "ERROR" : "OK");

  return errors;
}
