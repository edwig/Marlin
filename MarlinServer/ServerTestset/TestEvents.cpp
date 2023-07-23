/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: TestEvents.cpp
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

static int EventTests  = 3;              // OnMessage
static int totalChecks = EventTests + 3; // OnOther + OnError + OnClose

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
SiteHandlerStream::HandleStream(HTTPMessage* /*p_message*/,EventStream* p_stream)
{
  bool result = false;

  // Use the event stream
  testStream = p_stream;
  HTTPServer* server = p_stream->m_site->GetHTTPServer();

  // Report it
  xprintf("NEW EVENT STREAM : %p\n", reinterpret_cast<void*>(testStream));

  for(int x = 1; x <= EventTests; ++x)
  {
    ServerEvent* eventx = new ServerEvent("message");
    eventx->m_id = x;
    eventx->m_data.Format("This is message number: %u\n",x);

    result = server->SendEvent(p_stream,eventx);

    // --- "---------------------------------------------- - ------
    qprintf("Event stream OnMessage %d sent                  : %s\n", x, result ? "OK" : "ERROR");
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
    line += (char)('A' + y);
    line += (char)('z' - y);
  }
  XString all = line + line + line + line;
  all += "\r\n";
  for(int x = 0; x < 1000; ++x)
  {
    otherData += all;
  }

  xprintf("Sending other message\n");
  ServerEvent* other = new ServerEvent("other");
  other->m_data = "This is a complete different message in another set of stories.\r\n";
  other->m_data += otherData;
  result = server->SendEvent(p_stream,other);
  // --- "---------------------------------------------- - ------
  qprintf("Event stream 'other' message sent              : %s\n", result ? "OK" : "ERROR");
  if(result) 
  {
    --totalChecks;
  }
  else
  {
    xerror();
  }

  xprintf("Sending an error message\n");
  ServerEvent* err = new ServerEvent("error");
  err->m_data = "This is a very serious bug report from your server! Heed attention to it!";
  result = server->SendEvent(p_stream,err);
  // --- "---------------------------------------------- - ------
  qprintf("Event stream 'OnError' message sent            : %s\n", result ? "OK" : "ERROR");
  if(result)
  {
    --totalChecks;
  }
  else
  {
    xerror();
  }

  // Implicitly sending an OnClose
  xprintf("Closing event stream\n");
  server->CloseEventStream(p_stream);

  // Check for closed stream
  result = !server->HasEventStream(p_stream);
  // --- "---------------------------------------------- - ------
  qprintf("Event stream closed by server (OnClose sent)   : %s\n", result ? "OK" : "ERROR");
  if(result)
  {
    --totalChecks;
  }
  else
  {
    xerror(); 
  }
  return true;
}

int
TestMarlinServer::TestPushEvents()
{
  int error = 0;

  xprintf("TESTING SSE (Server-Sent-Events) CHANNEL FUNCTIONS OF THE HTTP-SERVER\n");
  xprintf("=====================================================================\n");
  XString url("/MarlinTest/Events/");
  // Create URL site to listen to events "http://+:port/MarlinTest/Events/"
  HTTPSite* site = m_httpServer->CreateSite(PrefixType::URLPRE_Strong,false,m_inPortNumber,url);
  if (site)
  {
    // SUMMARY OF THE TEST
    // --- "--------------------------- - ------\n"
    qprintf("HTTPSite for event streams  : OK : %s\n",site->GetPrefixURL().GetString());
  }
  else
  {
    ++error;
    xerror();
    qprintf("Cannot create a HTTP site for: %s\n",url.GetString());
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
    qprintf("ERROR STARTING SITE: %s\n",url.GetString());
  }
  return error;
}


int 
TestMarlinServer::AfterTestEvents()
{
  // SUMMARY OF THE TEST
  // ---- "---------------------------------------------- - ------
  qprintf("Event streams On-Message/Error/Other/Close     : %s\n",totalChecks > 0 ? "ERROR" : "OK");
  return totalChecks > 0;
}
