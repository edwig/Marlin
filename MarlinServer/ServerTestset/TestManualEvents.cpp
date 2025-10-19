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

class SiteHandlerManualStream: public SiteHandler
{
public:
  void HandleStream(EventStream* p_stream);
};

void
SiteHandlerManualStream::HandleStream(EventStream* p_stream)
{
  qprintf(_T("STREAM OPENED FOR:"));
  qprintf(_T("Stream URL   : %s"),p_stream->m_baseURL.GetString());
  qprintf(_T("URL port     : %d"),p_stream->m_port);
  qprintf(_T("Absolute Path: %s"),p_stream->m_absPath.GetString());
  qprintf(_T("Site found   : %s"),p_stream->m_site ? _T("yes") : _T("no"));
  qprintf(_T("Response     : %X"),p_stream->m_response);
  qprintf(_T("Request ID   : %X"),p_stream->m_requestID);
  qprintf(_T("Last event ID: %d"),p_stream->m_lastID);
  qprintf(_T("User         : %s"),p_stream->m_user.GetString());
}

// Manually test event streams from a web page
int
TestStreams(HTTPServer* p_server)
{
  int error = 0;

  xprintf(_T("TESTING SSE (Server-Sent-Events) CHANNEL FUNCTIONS FROM A WEB PAGE\n"));
  xprintf(_T("==================================================================\n"));
  XString url(_T("/MarlinTest/Streams/"));
  // Create URL site to listen to events "http://+:port/MarlinTest/Streams/"
  HTTPSite* site = p_server->CreateSite(PrefixType::URLPRE_Strong,false,TESTING_HTTP_PORT,url,true);
  if(site)
  {
    // SUMMARY OF THE TEST
    // --- "--------------------------- - ------\n"
    qprintf(_T("Site for manual streams    : OK : %s\n"),site->GetPrefixURL().GetString());
  }
  else
  {
    ++error;
    xerror();
    qprintf(_T("Cannot create a HTTP site for: %s\n"),url.GetString());
    return error;
  }

  site->SetHandler(HTTPCommand::http_get,alloc_new SiteHandlerManualStream());

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
  SITE_DETAILLOGS(_T("Handling form-data data-part: "),p_part->GetName());

  int errors = 0;
  XString data = p_part->GetData();
  XString name = p_part->GetName();
  xprintf(_T("MANUAL FORMS DATA = Name : %s\n"),name.GetString());
  xprintf(_T("MANUAL FORMS Content-type: %s\n"),p_part->GetContentType().GetString());
  xprintf(_T("MANUAL FORMS \n%s\n"),data.GetString());

  JSONMessage json(data);
  JSONvalue val = json.GetValue();

  XString site(_T("/MarlinTest/Streams/42"));
  ServerEvent event;
  event.m_data = _T("server message for programmer 42");
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

  XString url(_T("/MarlinTest/Forms/"));

  xprintf(_T("TESTING FORM-DATA FOR STREAMS MANUALLY\n"));
  xprintf(_T("======================================\n"));

  // Create HTTP site to listen to "http://+:port/MarlinTest/Forms/"
  HTTPSite* site = p_server->CreateSite(PrefixType::URLPRE_Strong,false,TESTING_HTTP_PORT,url,true);
  if(site)
  {
    // SUMMARY OF THE TEST
    //  --- "--------------------------- - ------\n"
    qprintf(_T("HTTPSite Form-data forms    : OK : %s\n"),site->GetPrefixURL().GetString());
  }
  else
  {
    ++error;
    xerror();
    qprintf(_T("ERROR: Cannot make a HTTP site for: %s\n"),url.GetString());
    return error;
  }

  // Setting the POST handler for this site
  site->SetHandler(HTTPCommand::http_post,alloc_new FormsHandler());

  // Modify the standard settings for this site
  site->AddContentType(false,_T(""),_T("multipart/form-data;"));

  // Start the site explicitly
  if(site->StartSite())
  {
    xprintf(_T("Site started correctly: %s\n"),url.GetString());
  }
  else
  {
    ++error;
    xerror();
    qprintf(_T("ERROR STARTING SITE: %s\n"),url.GetString());
  }
  return error;
}

