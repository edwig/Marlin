/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: TestFilter.cpp
//
// Marlin Server: Internet server/client
// 
// Copyright (c) 2014-2021 ir. W.E. Huisman
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
#include "SiteHandlerSoap.h"
#include "SiteFilter.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static int totalChecks = 2;

//////////////////////////////////////////////////////////////////////////
//
// FIRST DEFINE TWO FILTERS WITH PRIORITY 1 and 23
//
//////////////////////////////////////////////////////////////////////////

class SiteFilterTester1 : public SiteFilter
{
public:
  SiteFilterTester1(unsigned p_priority,CString p_name);
  virtual bool Handle(HTTPMessage* p_message);
};

SiteFilterTester1::SiteFilterTester1(unsigned p_priority,CString p_name)
                  :SiteFilter(p_priority,p_name)
{
}

bool
SiteFilterTester1::Handle(HTTPMessage* p_message)
{
  xprintf("FILTER TESTER NR 1: %s FROM %s\n", (LPCTSTR)p_message->GetURL(),(LPCTSTR)SocketToServer(p_message->GetSender()));
  xprintf("%s\n",(LPCTSTR)p_message->GetBody());

  // SUMMARY OF THE TEST
  // --- "---------------------------------------------- - ------
  qprintf("Filter handler with priority 1                 : OK\n");

  --totalChecks;

  return true;
}

class SiteFilterTester23 : public SiteFilter
{
public:
  SiteFilterTester23(unsigned p_priority,CString p_name);
  virtual bool Handle(HTTPMessage* p_message);
};

SiteFilterTester23::SiteFilterTester23(unsigned p_priority,CString p_name)
                   :SiteFilter(p_priority,p_name)
{
}

// Not much useful things here, but hey: it's a test!
bool
SiteFilterTester23::Handle(HTTPMessage* p_message)
{
  HTTPMessage* msg = const_cast<HTTPMessage*>(p_message);
  xprintf("FILTER TESTER NR 23: %s FROM %s\n", (LPCTSTR)msg->GetURL(), (LPCTSTR)SocketToServer(msg->GetSender()));
  xprintf("Registering the body length: %lu\n",(unsigned long)msg->GetBodyLength());

  // SUMMARY OF THE TEST
  // --- "---------------------------------------------- - ------
  qprintf("Filter handler with priority 23                : OK\n");

  --totalChecks;

  return true;
}

//////////////////////////////////////////////////////////////////////////
//
// NOW DEFINE THE "REGULAR" SOAP POST HANDLER
//
//////////////////////////////////////////////////////////////////////////

class SiteHandlerSoapFiltering : public SiteHandlerSoap
{
protected:
  bool  Handle(SOAPMessage* p_message);
};

bool
SiteHandlerSoapFiltering::Handle(SOAPMessage* p_message)
{
  // Display incoming message
  xprintf("Incoming message in XML:\n%s\n", p_message->GetSoapMessage().GetString());

  CString param = p_message->GetParameter("Price");
  p_message->Reset(ResponseType::RESP_ACTION_NAME);
  double price = atof(param);
  price *= 1.21; // VAT percentage in the Netherlands
  p_message->SetParameter("PriceInclusive", price);

  return true;
}

//////////////////////////////////////////////////////////////////////////
//
// CREATE THE SITE
//
//////////////////////////////////////////////////////////////////////////

int
TestMarlinServer::TestFilter()
{
  int error = 0;

  // If errors, change detail level
  m_doDetails = false;

  CString url("/MarlinTest/Filter/");

  xprintf("TESTING SITE FILTERING FUNCTIONS OF THE HTTP SERVER\n");
  xprintf("===================================================\n");

  // Create URL channel to listen to "http://+:port/MarlinTest/Filter/"
  // Callback function is no longer required!
  HTTPSite* site = m_httpServer->CreateSite(PrefixType::URLPRE_Strong,false,m_inPortNumber,url);
  if (site)
  {
    // SUMMARY OF THE TEST
    // --- "--------------------------- - ------\n"
    qprintf("HTTPSite for filter testing : OK : %s\n", site->GetPrefixURL().GetString());
  }
  else
  {
    ++error;
    xerror();
    qprintf("ERROR: Cannot make a HTTP site for: %s\n", (LPCTSTR)url);
    return error;
  }

  // Setting the POST handler for this site
  site->SetHandler(HTTPCommand::http_post, new SiteHandlerSoapFiltering());

  // Add filters to this site by priority of handling
  site->SetFilter( 1, new SiteFilterTester1 ( 1,"Tester1"));
  site->SetFilter(23, new SiteFilterTester23(23,"Tester23"));

  // Modify the standard settings for this site
  site->AddContentType("","text/xml");
  site->AddContentType("xml","application/soap+xml");

  // Start the site explicitly
  if (site->StartSite())
  {
    xprintf("Site started correctly: %s\n", url.GetString());
  }
  else
  {
    ++error;
    xerror();
    qprintf("ERROR STARTING SITE: %s\n", (LPCTSTR)url);
  }
  return error;
}

// At least both filters must show up in the results
int
TestMarlinServer::AfterTestFilter()
{
  // SUMMARY OF THE TEST
  // ---- "---------------------------------------------- - ------
  qprintf("Firing of the filter handlers                  : %s\n",totalChecks > 0 ? "ERROR" : "OK");
  return totalChecks > 0;
}

