/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: TestEvents.cpp
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
#include "EventSource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//////////////////////////////////////////////////////////////////////////
//
// PUSH EVENTS TESTING
//
//////////////////////////////////////////////////////////////////////////

// Channel stopped
bool isStopped     = false;
// Messages seen?
int  onOpenSeen    = 0;
int  onMessageSeen = 0;
int  onOtherSeen   = 0;
int  onErrorSeen   = 0;
UINT maxID         = 0;

void OnOpen(ServerEvent* p_event,void* p_data)
{
  UNREFERENCED_PARAMETER(p_data);

  xprintf(_T("ONE TIME OCCURENCE: Open event listener channel\n"));
  fflush(stdout);

  // Now dispose of the event!!
  delete p_event;

  ++onOpenSeen;

  // SUMMARY OF THE TEST
  // --- "---------------------------------------------- - ------
  _tprintf(_T("EVENT channel OnOpen received                  : OK\n"));
}

void OnClose(ServerEvent* p_event,void* p_data)
{
  UNREFERENCED_PARAMETER(p_data);

  xprintf(_T("ONE TIME OCCURENCE: Closed event listener channel\n"));
  fflush(stdout);

  // Now dispose of the event!!
  delete p_event;

  // Normal closure of the channel
  isStopped = true;

  // SUMMARY OF THE TEST
  // --- "---------------------------------------------- - ------
  _tprintf(_T("EVENT channel OnClose received                 : OK\n"));
}

void OnMessage(ServerEvent* p_event,void* p_data)
{
  UNREFERENCED_PARAMETER(p_data);

  xprintf(_T("OnMessage delivered from server.\n")
          _T("Message ID: %d\n%s\n")
         ,p_event->m_id
         ,p_event->m_data.GetString());
  fflush(stdout);

  ++onMessageSeen;
  if(p_event->m_id > maxID)
  {
    maxID = p_event->m_id;
  }
  xprintf(_T("MaxID is now: %u\n"),maxID);

  // SUMMARY OF THE TEST
  // --- "---------------------------------------------- - ------
  _tprintf(_T("EVENT channel OnMessage received               : OK\n"));
  xprintf(_T("%d %s\n"),p_event->m_id,p_event->m_data.GetString());

  // Now dispose of the event!!
  delete p_event;
}

void OnOther(ServerEvent* p_event,void* p_data)
{
  UNREFERENCED_PARAMETER(p_data);

  xprintf(_T("OTHER event delivered from server.\n")
          _T("Message ID: %d\n%s\n")
          ,p_event->m_id
          ,p_event->m_data.GetString());
  fflush(stdout);

  ++onOtherSeen;

  // SUMMARY OF THE TEST
  // --- "---------------------------------------------- - ------
  _tprintf(_T("EVENT channel OnOther received                 : OK\n"));
  xprintf(_T("%d %s\n"),p_event->m_id,p_event->m_data.GetString());

  // Now dispose of the event!!
  delete p_event;
}

void OnError(ServerEvent* p_event,void* p_data)
{
  UNREFERENCED_PARAMETER(p_data);

  xprintf(_T("WHOAAA. ERROR received from server\n")
          _T("Message ID: %d\n%s\n")
          ,p_event->m_id
          ,p_event->m_data.GetString());
  fflush(stdout);

  ++onErrorSeen;

  // SUMMARY OF THE TEST
  // --- "---------------------------------------------- - ------
  _tprintf(_T("EVENT channel OnError received                 : OK\n"));
  xprintf(_T("%d %s\n"), p_event->m_id,p_event->m_data.GetString());

  // Now dispose of the event!!
  delete p_event;
}

int
TestEvents(HTTPClient* p_client)
{
  // Change if an error occurred in this module
  doDetails = false;

  // This is what we are going to test
  xprintf(_T("TESTING EVENTSOURCE SIDE OF SSE (SERVER-SENT-EVENTS) PUSH INTERFACE\n"));
  xprintf(_T("===================================================================\n"));

  XString url;
  url.Format(_T("http://%s:%d/MarlinTest/Events/"),MARLIN_HOST,TESTING_HTTP_PORT);

  HTTPClient client;
  client.SetLogging(p_client->GetLogging());
  client.SetLogLevel(p_client->GetLogLevel());

  EventSource* source = client.CreateEventSource(url);
  source->m_onopen    = OnOpen;
  source->m_onmessage = OnMessage;
  source->m_onerror   = OnError;
  source->m_onclose   = OnClose;
  source->AddEventListener(_T("Other"), OnOther);
  source->SetReconnectionTime(1000);

  // Turning the client into a listener
  // Should by convention be done by the event source, and not by the HTTP CLIENT!!
  source->EventSourceInit(false);

  // Sleep in 1/5 second intervals, until all messages received and channel closed
  // int maxWait = 50; // With a maximum of 10 seconds

  // Longer wait time for testing of event streams
  int maxWait = 10 * 60 * 5;  // Wait for 10 minutes

  do 
  {
    Sleep(200);
    if(maxWait-- == 0) break;
  } 
  while(isStopped == false);

  // Stopping the client
  if(client.GetIsRunning())
  {
    xprintf(_T("Stopping the client\n"));
    client.StopClient();
  }
  xprintf(_T("The client is %s\n"), client.GetIsRunning() ? _T("still running!\n") : _T("stopped.\n"));

  // Total condition for the test to succeed: 
  // All event types seen and a minimum of 3 OnMessage events
  bool result = (onOpenSeen && onMessageSeen && onOtherSeen && onErrorSeen && maxID == 3) ? true : false;
  // SUMMARY OF THE TEST
  // --- "---------------------------------------------- - ------
  _tprintf(_T("Send: Testing the SSE event channel complete   : %s\n"), result ? _T("OK") : _T("ERROR"));

  if(!result)
  {
    _tprintf(_T("ERROR STATE IN THE EVENT TEST:\n"));
    _tprintf(_T("- Open seen    : %d\n"),onOpenSeen);
    _tprintf(_T("- Message seen : %d\n"),onMessageSeen);
    _tprintf(_T("- Other   seen : %d\n"),onOtherSeen);
    _tprintf(_T("- Error   seen : %d\n"),onErrorSeen);
    _tprintf(_T("- Max ID  seen : %d\n"),maxID);
  }

  return result ? 0 : 1;
}
