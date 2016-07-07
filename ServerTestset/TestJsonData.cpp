/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: TestJsonData.cpp
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
#include "HTTPSite.h"
#include "SiteHandlerJson.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static int totalChecks = 4;

class SiteHandlerJsonData: public SiteHandlerJson
{
public:
  bool Handle(JSONMessage* p_message);
};

bool
SiteHandlerJsonData::Handle(JSONMessage* p_message)
{
  bool result = false;
  JSONvalue& val = p_message->GetValue();

  if(val.GetDataType() == JsonType::JDT_string)
  {
    CString command = val.GetString();
    p_message->Reset();

    if(command == "Test1")
    {
      p_message->ParseMessage("{ \"one\" : [ 1, 2, 3, 4, 5] }");
      result = true;
    }
    else if(command == "Test2")
    {
      p_message->ParseMessage("{ \"two\"  : [ 201, 202, 203, 204.5, 205.6789 ] \n"
                              " ,\"three\": [ 301, 302, 303, 304.5, 305.6789 ] }\n");
      result = true;
    }
    else
    {
      qprintf("JOSN Unknown command. Check your test client\n");
    }

    // Test parameters in JSON Message
    CString test = p_message->GetCrackedURL().GetParameter("test");
    CString size = p_message->GetCrackedURL().GetParameter("size");
    if(test != "2" || size != "medium large")
    {
      result = false;
    }
    // Test headers in JSON message
    if(p_message->GetHeader("GUID") != "888-777-666")
    {
      result = false;
    }
  }
  else
  {
    qprintf("JSON Unknown command\n");
    p_message->Reset();
  }
  // SUMMARY OF THE TEST
  // --- "---------------------------------------------- - ------
  qprintf("JSON singular object received                  : %s\n", result ? "OK" : "ERROR");
  if(!result) xerror();

  // Check done
  --totalChecks;

  return true;
}

int
TestJsonData(HTTPServer* p_server)
{
  int error = 0;

  // If errors, change detail level
  doDetails = false;

  CString url("/MarlinTest/Data/");

  xprintf("TESTING STANDARD JSON RECEIVER FUNCTIONS OF THE HTTP SERVER\n");
  xprintf("===========================================================\n");

  // Create URL channel to listen to "http://+:port/MarlinTest/Data/"
  // Callback function is no longer required!
  HTTPSite* site = p_server->CreateSite(PrefixType::URLPRE_Strong,false,TESTING_HTTP_PORT,url);
  if(site)
  {
    // SUMMARY OF THE TEST
    // --- "--------------------------- - ------\n"
    qprintf("HTTPSite for JSON OData     : OK : %s\n",site->GetPrefixURL().GetString());
  }
  else
  {
    ++error;
    xerror();
    qprintf("ERROR: Cannot make a HTTP site for: %s\n",(LPCTSTR)url);
    return error;
  }

  // Setting the POST handler for this site
  site->SetHandler(HTTPCommand::http_post,new SiteHandlerJsonData());

  // Modify the standard settings for this site
  site->AddContentType("json","application/json");

  // Start the site explicitly
  if(site->StartSite())
  {
    xprintf("Site started correctly: %s\n",url);
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
AfterTestJsonData()
{
  if(totalChecks > 0)
  {
    // SUMMARY OF THE TEST
    // --- "---------------------------------------------- - ------
    qprintf("Not all JSON Data tests received               : ERROR\n");
  }
  return totalChecks > 0;
}
