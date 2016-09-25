/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: TestEvents.cpp
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
#include "stdafx.h"
#include "TestServer.h"
#include "HTTPServer.h"
#include "HTTPSite.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static int totalChecks = 1;
static int EventTests  = 3;

//////////////////////////////////////////////////////////////////////////
//
// TESTING PUSH EVENTS
//
//////////////////////////////////////////////////////////////////////////

static EventStream* testStream = nullptr;

class SiteHandlerStream: public SiteHandler
{
public:
  void HandleStream(EventStream* p_stream);
};

void 
SiteHandlerStream::HandleStream(EventStream* p_stream)
{
  bool result = false;

  // Use the event stream
  testStream = p_stream;
  HTTPServer* server = p_stream->m_site->GetHTTPServer();

  // Report it
  xprintf("NEW EVENT STREAM : %p\n", (void*)testStream);

  for(int x = 1; x <= EventTests; ++x)
  {
    ServerEvent* eventx = new ServerEvent("message");
    eventx->m_id = x;
    eventx->m_data.Format("This is message number: %u\n",x);

    result = server->SendEvent(p_stream,eventx);

    // --- "---------------------------------------------- - ------
    qprintf("Event stream OnMessage %d sent                  : %s\n", x, result ? "OK" : "ERROR");
    if(!result) xerror();
    // Wait 1/10 of a second
    Sleep(100);
  }

  xprintf("Sending other messages\n");
  ServerEvent* ander = new ServerEvent("other");
  ander->m_id   = 1;
  ander->m_data = "This is a complete different message in another set of stories.";
  result = server->SendEvent(p_stream,ander);
  // --- "---------------------------------------------- - ------
  qprintf("Event stream 'other' message sent              : %s\n", result ? "OK" : "ERROR");
  if(!result) xerror();


  xprintf("Sending an error message\n");
  ServerEvent* err = new ServerEvent("error");
  err->m_id = 0;
  err->m_data = "This is a very serious bug report from your server! Heed attention to it!";
  result = server->SendEvent(p_stream,err);
  // --- "---------------------------------------------- - ------
  qprintf("Event stream 'OnError' message sent            : %s\n", result ? "OK" : "ERROR");
  if(!result) xerror();

  // Implicitly sending an OnClose
  xprintf("Closing event stream\n");
  server->CloseEventStream(p_stream);

  // Check for closed stream
  result = !server->HasEventStream(p_stream);
  // --- "---------------------------------------------- - ------
  qprintf("Event stream closed by server (OnClose sent)   : %s\n", result ? "OK" : "ERROR");
  if(!result) xerror();

  // Checks done
  --totalChecks;
}

int
TestPushEvents(HTTPServer* p_server)
{
  int error = 0;

  xprintf("TESTING SSE (Server-Sent-Events) CHANNEL FUNCTIONS OF THE HTTP-SERVER\n");
  xprintf("=====================================================================\n");
  CString url("/MarlinTest/Events/");
  // Create URL site to listen to events "http://+:port/MarlinTest/Events/"
  HTTPSite* site = p_server->CreateSite(PrefixType::URLPRE_Strong,false,TESTING_HTTP_PORT,url);
  if (site)
  {
    // SUMMARY OF THE TEST
    // --- "--------------------------- - ------\n"
    qprintf("HTTPSite for event streams  : OK : %s\n",(LPCTSTR) site->GetPrefixURL());
  }
  else
  {
    ++error;
    xerror();
    qprintf("Cannot create a HTTP site for: %s\n",(LPCTSTR) url);
    return error;
  }

  site->SetHandler(HTTPCommand::http_get,new SiteHandlerStream());

  // HERE IS THE MAGIC. MAKE IT INTO AN EVENT STREAM HANDLER!!!
  // Modify standard settings for this site
  site->AddContentType("txt","text/event-stream");
  site->SetIsEventStream(true);

  // Start the site explicitly
  if(site->StartSite())
  {
    xprintf("Site started correctly\n");
  }
  else
  {
    ++error;
    xerror();
    qprintf("ERROR STARTING SITE: %s\n",(LPCTSTR)url);
  }
  return error;
}

int 
AfterTestEvents()
{
  if(totalChecks > 0)
  {
    // SUMMARY OF THE TEST
    // --- "---------------------------------------------- - ------
    qprintf("Testing of the event streams not done          : ERROR\n");
  }
  return totalChecks > 0;
}
