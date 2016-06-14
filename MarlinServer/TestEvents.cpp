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

  for(unsigned x = 1; x <= 3; ++x)
  {
    ServerEvent* eventx = new ServerEvent("message");
    eventx->m_id = x;
    eventx->m_data.Format("This is message number: %u\n",x);

    result = server->SendEvent(p_stream,eventx);

    // --- "--------------------------- - ------\n"
    printf("TEST EVENT MESSAGE [%d]      : %s\n", x, result ? "OK" : "ERROR");
    if(!result) xerror();
  }

  xprintf("Sending other messages\n");
  ServerEvent* ander = new ServerEvent("other");
  ander->m_id   = 1;
  ander->m_data = "This is a complete different message in another set of stories.";
  result = server->SendEvent(p_stream,ander);
  // --- "--------------------------- - ------\n"
  printf("TEST EVENT MESSAGE [OTHER]  : %s\n", result ? "OK" : "ERROR");
  if(!result) xerror();


  xprintf("Sending an error message\n");
  ServerEvent* err = new ServerEvent("error");
  err->m_id = 0;
  err->m_data = "This is a very serious bug report from your server! Heed attention to it!";
  result = server->SendEvent(p_stream,err);
  // --- "--------------------------- - ------\n"
  printf("TEST EVENT MESSAGE [ERROR]  : %s\n", result ? "OK" : "ERROR");
  if(!result) xerror();

  // Implicitly sending an OnClose
  xprintf("Closing event stream\n");
  server->CloseEventStream(p_stream);

  // Check for closed stream
  result = !server->HasEventStream(p_stream);
  // --- "--------------------------- - ------\n"
  printf("TEST EVENT STREAM CLOSED    : %s\n", result ? "OK" : "ERROR");
  if(!result) xerror();
}

int
TestPushEvents(HTTPServer* p_server)
{
  int error = 0;

  xprintf("TESTING SSE (Server-Sent-Events) CHANNEL FUNCTIONS OF THE HTTP-SERVER\n");
  xprintf("=====================================================================\n");
  CString url("/MarlinTest/Events/");
  // Create URL site to listen to events "http://+:1200/MarlinTest/Events/"
  HTTPSite* site = p_server->CreateSite(PrefixType::URLPRE_Strong,false,TESTING_HTTP_PORT,url);
  if (site)
  {
    // SUMMARY OF THE TEST
    // --- "--------------------------- - ------\n"
    printf("HTTPSite for event streams  : OK : %s\n",(LPCTSTR) site->GetPrefixURL());
  }
  else
  {
    ++error;
    xerror();
    printf("Cannot create a HTTP site for: %s\n",(LPCTSTR) url);
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
    printf("ERROR STARTING SITE: %s\n",(LPCTSTR)url);
  }
  return error;
}
