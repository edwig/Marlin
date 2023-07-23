/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: TestManualEvents.cpp
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
#include "TestMarlinServer.h"
#include "TestPorts.h"
#include "HTTPServer.h"
#include "HTTPSite.h"
#include "HTTPMessage.h"
#include "SiteHandlerFormData.h"
#include "MultiPartBuffer.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

class SiteHandlerManualStream: public SiteHandler
{
public:
  void HandleStream(EventStream* p_stream);
};

void
SiteHandlerManualStream::HandleStream(EventStream* p_stream)
{
  qprintf("STREAM OPENED FOR:");
  qprintf("Stream URL   : %s",p_stream->m_baseURL.GetString());
  qprintf("URL port     : %d",p_stream->m_port);
  qprintf("Absolute Path: %s",p_stream->m_absPath.GetString());
  qprintf("Site found   : %s",p_stream->m_site ? "yes" : "no");
  qprintf("Response     : %X",p_stream->m_response);
  qprintf("Request ID   : %X",p_stream->m_requestID);
  qprintf("Last event ID: %d",p_stream->m_lastID);
  qprintf("User         : %s",p_stream->m_user.GetString());
}

// Manually test event streams from a web page
int
TestStreams(HTTPServer* p_server)
{
  int error = 0;

  xprintf("TESTING SSE (Server-Sent-Events) CHANNEL FUNCTIONS FROM A WEB PAGE\n");
  xprintf("==================================================================\n");
  CString url("/MarlinTest/Streams/");
  // Create URL site to listen to events "http://+:port/MarlinTest/Streams/"
  HTTPSite* site = p_server->CreateSite(PrefixType::URLPRE_Strong,false,TESTING_HTTP_PORT,url);
  if(site)
  {
    // SUMMARY OF THE TEST
    // --- "--------------------------- - ------\n"
    qprintf("Site for manual streams    : OK : %s\n",site->GetPrefixURL().GetString());
  }
  else
  {
    ++error;
    xerror();
    qprintf("Cannot create a HTTP site for: %s\n",url.GetString());
    return error;
  }

  site->SetHandler(HTTPCommand::http_get,new SiteHandlerManualStream());

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

//////////////////////////////////////////////////////////////////////////
//
// FORMS
//
//////////////////////////////////////////////////////////////////////////

class FormsHandler: public SiteHandlerFormData
{
protected:
  int HandleData(HTTPMessage* p_message,MultiPart* p_part);
private:
  int m_id{0};
};

int
FormsHandler::HandleData(HTTPMessage* p_message,MultiPart* p_part)
{
  SITE_DETAILLOGS("Handling form-data data-part: ",p_part->GetName());

  int errors = 0;
  CString data = p_part->GetData();
  CString name = p_part->GetName();
  xprintf("MANUAL FORMS DATA = Name : %s\n",name.GetString());
  xprintf("MANUAL FORMS Content-type: %s\n",p_part->GetContentType().GetString());
  xprintf("MANUAL FORMS \n%s\n",data.GetString());

  JSONMessage json(data);
  JSONvalue val = json.GetValue();

  CString site("/MarlinTest/Streams/42");
  ServerEvent event;
  event.m_data = "server message for programmer 42";
  event.m_id   = ++m_id;
  
  // Try to sent the event
  if(!p_message->GetHTTPSite()->GetHTTPServer()->SendEvent(TESTING_HTTP_PORT,site,&event))
  {
    ++errors;
  }

  // return number of errors!
  return errors;
}


int TestForms(HTTPServer* p_server)
{
  int error = 0;

  // If errors, change detail level
  // doDetails = false;

  CString url("/MarlinTest/Forms/");

  xprintf("TESTING FORM-DATA FOR STREAMS MANUALLY\n");
  xprintf("======================================\n");

  // Create HTTP site to listen to "http://+:port/MarlinTest/Forms/"
  HTTPSite* site = p_server->CreateSite(PrefixType::URLPRE_Strong,false,TESTING_HTTP_PORT,url);
  if(site)
  {
    // SUMMARY OF THE TEST
    //  --- "--------------------------- - ------\n"
    qprintf("HTTPSite Form-data forms    : OK : %s\n",site->GetPrefixURL().GetString());
  }
  else
  {
    ++error;
    xerror();
    qprintf("ERROR: Cannot make a HTTP site for: %s\n",url.GetString());
    return error;
  }

  // Setting the POST handler for this site
  site->SetHandler(HTTPCommand::http_post,new FormsHandler());

  // Modify the standard settings for this site
  site->AddContentType("","multipart/form-data;");

  // Start the site explicitly
  if(site->StartSite())
  {
    xprintf("Site started correctly: %s\n",url.GetString());
  }
  else
  {
    ++error;
    xerror();
    qprintf("ERROR STARTING SITE: %s\n",url.GetString());
  }
  return error;
}

