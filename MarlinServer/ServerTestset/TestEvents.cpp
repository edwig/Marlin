/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: TestEvents.cpp
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
#include "TestMarlinServer.h"
#include "TestPorts.h"
#include <MarlinServer.h>
#include <HTTPServer.h>
#include <HTTPSite.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static int EventTests  = 3;              // OnMessage
static int totalChecks = EventTests + 3; // OnOther + OnError + OnClose

void SendSSEMessages(void* p_data);


//////////////////////////////////////////////////////////////////////////
//
// TESTING PUSH EVENTS
//
//////////////////////////////////////////////////////////////////////////

static EventStream* testStream = nullptr;

class SiteHandlerStream: public SiteHandler
{
public:
  bool HandleStream(HTTPMessage* p_message,EventStream* p_stream) override;
};

bool
SiteHandlerStream::HandleStream(HTTPMessage* p_message,EventStream* p_stream)
{
  UNREFERENCED_PARAMETER(p_message);

  // Use the event stream
  testStream = p_stream;
  HTTPServer* server = p_stream->m_site->GetHTTPServer();

  // Report it
  xprintf(_T("NEW EVENT STREAM : %p\n"),reinterpret_cast<void*>(testStream));

  server->GetThreadPool()->SubmitWork(SendSSEMessages,p_stream);
  return true;
}

void SendSSEMessages(void* p_data)
{
  EventStream* stream = reinterpret_cast<EventStream*>(p_data);
  HTTPServer* server = stream->m_site->GetHTTPServer();
  bool result = false;

  // Wait until connection is established
  Sleep(1000);

  for(int x = 1; x <= EventTests; ++x)
  {
    ServerEvent* eventx = new ServerEvent(_T("message"));
    eventx->m_id = x;
    eventx->m_data.Format(_T("This is message number: %u\n"),x);

    result = server->SendEvent(stream,eventx);

    // --- "---------------------------------------------- - ------
    qprintf(_T("Event stream OnMessage %d sent                  : %s\n"), x, result ? _T("OK") : _T("ERROR"));
    if(result) 
    {
      --totalChecks;
    }
    else
    {
      xerror();
    }
    // Waiting long time to see if the flush works and testing event streams
    // with immediately reaction on the client
    // Sleep(20000);

    // Wait 1/10 of a second
    Sleep(100);
  }

  // Create very large event data message
  XString otherData;
  XString line;
  for (int y = 0; y < 26; ++y)
  {
    line += (TCHAR)(_T('A') + y);
    line += (TCHAR)(_T('z') - y);
  }
  XString all = line + line + line + line;
  all += _T("\r\n");
  for(int x = 0; x < 1000; ++x)
  {
    otherData += all;
  }

  xprintf(_T("Sending other message\n"));
  ServerEvent* other = new ServerEvent(_T("other"));
  other->m_data = _T("This is a complete different message in another set of stories.\r\n");
  other->m_data += otherData;
  result = server->SendEvent(stream,other);
  // --- "---------------------------------------------- - ------
  qprintf(_T("Event stream 'other' message sent              : %s\n"), result ? _T("OK") : _T("ERROR"));
  if(result) 
  {
    --totalChecks;
  }
  else
  {
    xerror();
  }

  xprintf(_T("Sending an error message\n"));
  ServerEvent* err = new ServerEvent(_T("error"));
  err->m_data = _T("This is a very serious bug report from your server! Heed attention to it!");
  result = server->SendEvent(stream,err);
  // --- "---------------------------------------------- - ------
  qprintf(_T("Event stream 'OnError' message sent            : %s\n"), result ? _T("OK") : _T("ERROR"));
  if(result)
  {
    --totalChecks;
  }
  else
  {
    xerror();
  }

  // Implicitly sending an OnClose
  xprintf(_T("Closing event stream\n"));
  server->CloseEventStream(stream);

  // Check for closed stream
  result = !server->HasEventStream(stream);
  // --- "---------------------------------------------- - ------
  qprintf(_T("Event stream closed by server (OnClose sent)   : %s\n"), result ? _T("OK") : _T("ERROR"));
  if(result)
  {
    --totalChecks;
  }
  else
  {
    xerror(); 
  }
}

int
TestMarlinServer::TestPushEvents()
{
  int error = 0;

  xprintf(_T("TESTING SSE (Server-Sent-Events) CHANNEL FUNCTIONS OF THE HTTP-SERVER\n"));
  xprintf(_T("=====================================================================\n"));
  XString url(_T("/MarlinTest/Events/"));
  // Create URL site to listen to events "http://+:port/MarlinTest/Events/"
  HTTPSite* site = m_httpServer->CreateSite(PrefixType::URLPRE_Strong,false,TESTING_HTTP_PORT,url,true);
  if (site)
  {
    // SUMMARY OF THE TEST
    // --- "--------------------------- - ------\n"
    qprintf(_T("HTTPSite for event streams  : OK : %s\n"),site->GetPrefixURL().GetString());
  }
  else
  {
    ++error;
    xerror();
    qprintf(_T("Cannot create a HTTP site for: %s\n"),url.GetString());
    return error;
  }

  site->SetHandler(HTTPCommand::http_get,new SiteHandlerStream());

  // HERE IS THE MAGIC. MAKE IT INTO AN EVENT STREAM HANDLER!!!
  // Modify standard settings for this site
  site->AddContentType(true,_T("txt"),_T("text/event-stream"));
  site->SetIsEventStream(true);

  // Start the site explicitly
  if(site->StartSite())
  {
    xprintf(_T("Site started correctly\n"));
  }
  else
  {
    ++error;
    xerror();
    qprintf(_T("ERROR STARTING SITE: %s\n"),url.GetString());
  }
  return error;
}


int 
TestMarlinServer::AfterTestEvents()
{
  // SUMMARY OF THE TEST
  // ---- "---------------------------------------------- - ------
  qprintf(_T("Event streams On-Message/Error/Other/Close     : %s\n"),totalChecks > 0 ? _T("ERROR") : _T("OK"));
  return totalChecks > 0;
}
