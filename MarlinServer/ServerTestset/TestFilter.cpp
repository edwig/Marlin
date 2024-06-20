/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: TestFilter.cpp
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
  SiteFilterTester1(unsigned p_priority,XString p_name);
  virtual bool Handle(HTTPMessage* p_message);
};

SiteFilterTester1::SiteFilterTester1(unsigned p_priority,XString p_name)
                  :SiteFilter(p_priority,p_name)
{
}

bool
SiteFilterTester1::Handle(HTTPMessage* p_message)
{
  xprintf(_T("FILTER TESTER NR 1: %s FROM %s\n"),p_message->GetURL().GetString(),SocketToServer(p_message->GetSender()).GetString());
  xprintf(_T("%s\n"),p_message->GetBody().GetString());

  // SUMMARY OF THE TEST
  // --- "---------------------------------------------- - ------
  qprintf(_T("Filter handler with priority 1                 : OK\n"));

  --totalChecks;

  return true;
}

class SiteFilterTester23 : public SiteFilter
{
public:
  SiteFilterTester23(unsigned p_priority,XString p_name);
  virtual bool Handle(HTTPMessage* p_message);
};

SiteFilterTester23::SiteFilterTester23(unsigned p_priority,XString p_name)
                   :SiteFilter(p_priority,p_name)
{
}

// Not much useful things here, but hey: it's a test!
bool
SiteFilterTester23::Handle(HTTPMessage* p_message)
{
  HTTPMessage* msg = const_cast<HTTPMessage*>(p_message);
  xprintf(_T("FILTER TESTER NR 23: %s FROM %s\n"), msg->GetURL().GetString(),SocketToServer(msg->GetSender()).GetString());
  xprintf(_T("Registering the body length: %lu\n"),static_cast<unsigned long>(msg->GetBodyLength()));

  // SUMMARY OF THE TEST
  // --- "---------------------------------------------- - ------
  qprintf(_T("Filter handler with priority 23                : OK\n"));

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
  xprintf(_T("Incoming message in XML:\n%s\n"), p_message->GetSoapMessage().GetString());

  XString param = p_message->GetParameter(_T("Price"));
  p_message->Reset(ResponseType::RESP_ACTION_NAME);
  double price = _ttof(param);
  price *= 1.21; // VAT percentage in the Netherlands
  p_message->SetParameter(_T("PriceInclusive"), price);

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

  XString url(_T("/MarlinTest/Filter/"));

  xprintf(_T("TESTING SITE FILTERING FUNCTIONS OF THE HTTP SERVER\n"));
  xprintf(_T("===================================================\n"));

  // Create URL channel to listen to "http://+:port/MarlinTest/Filter/"
  // Callback function is no longer required!
  HTTPSite* site = m_httpServer->CreateSite(PrefixType::URLPRE_Strong,false,m_inPortNumber,url);
  if (site)
  {
    // SUMMARY OF THE TEST
    // --- "--------------------------- - ------\n"
    qprintf(_T("HTTPSite for filter testing : OK : %s\n"), site->GetPrefixURL().GetString());
  }
  else
  {
    ++error;
    xerror();
    qprintf(_T("ERROR: Cannot make a HTTP site for: %s\n"),url.GetString());
    return error;
  }

  // Setting the POST handler for this site
  site->SetHandler(HTTPCommand::http_post, new SiteHandlerSoapFiltering());

  // Add filters to this site by priority of handling
  site->SetFilter( 1, new SiteFilterTester1 ( 1,_T("Tester1")));
  site->SetFilter(23, new SiteFilterTester23(23,_T("Tester23")));

  // Modify the standard settings for this site
  site->AddContentType(true,_T("pos"),_T("text/xml"));
  site->AddContentType(true,_T("xml"),_T("application/soap+xml"));

  // Start the site explicitly
  if (site->StartSite())
  {
    xprintf(_T("Site started correctly: %s\n"), url.GetString());
  }
  else
  {
    ++error;
    xerror();
    qprintf(_T("ERROR STARTING SITE: %s\n"),url.GetString());
  }
  return error;
}

// At least both filters must show up in the results
int
TestMarlinServer::AfterTestFilter()
{
  // SUMMARY OF THE TEST
  // ---- "---------------------------------------------- - ------
  qprintf(_T("Firing of the filter handlers                  : %s\n"),totalChecks > 0 ? _T("ERROR") : _T("OK"));
  return totalChecks > 0;
}

