/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: TestMSGraph.cpp
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
#include "TestClient.h"
#include "JSONMessage.h"
#include "SOAPMessage.h"
#include "HTTPClient.h"
#include "OAuth2Cache.h"
#include <iostream>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// MEMO
// In order to be able to use MS-Graph you need to register your application in 
// Microsoft Azure portal as a Azure-App. This will get you the following
// 1) An application client-id (it's a GUID assigned to your app)
// 2) An application client-key (it's a base64 string from a password or certificate)
// 3) You need to know your Azure tenant-ID (it's a GUID assigned to the Azure lease)
// 4) You need to use your Microsoft registered email address in the Azure-AD
// All these four values are replaced with a "<DESCRIPTION>" string below
//
// Use the MS-Graph documentation: https://docs.microsoft.com/en-US/graph/api/overview?view=graph-rest-1.0
// 
int TestMSGraph(HTTPClient* p_client)
{
  int errors = 0;
  XString tenant(_T("<MY AZURE TENANT GUID>"));
  XString applicationID(_T("<MY-APPLICATION-ID>"));
  XString applicationKey(_T("<NOT-CHECKED-IN> :-)"));

  OAuth2Cache cache;
  cache.SetAnalysisLog(p_client->GetLogging());

  XString url = cache.CreateTokenURL(token_server_ms,tenant);
  int session = cache.CreateClientCredentialsGrant(url,applicationID,applicationKey,scope_ms_graph);
  if(!session)
  {
    _tprintf(_T("Unexpected: Cannot create an OAuth2 credentials session!\n"));
    return 1;
  }

  // Connect the session to the client
  p_client->SetOAuth2Cache(&cache);
  p_client->SetOAuth2Session(session);

  // Build our MS-Graph query URL
  url  = _T("https://graph.microsoft.com/v1.0/users/");
  url += CrackedURL::EncodeURLChars(_T("<MY-EMAIL-ADDRESS>"),true);
  url += _T("/calendar/events");
  url += _T("?startdatetime=2021-06-04T13:41:56.174Z");
  url += _T("&enddatetime=2021-06-11T13:41:56.174Z");

  // Get my calendar events of the next week.
  HTTPMessage msg(HTTPCommand::http_get,url);
  msg.AddHeader(_T("Prefer"),_T("outlook.timezone=\"Europe/Amsterdam\""));

  // SEND
  if(p_client->Send(&msg))
  {
    // All went well
    JSONMessage json(&msg);
    json.SetWhitespace(true);
    XString result = json.GetJsonMessage();
    _tprintf(_T("All my calendar events of the next week:\n"));
    _tprintf(result.GetString());
  }
  else
  {
    // ERRORS: Print the error
    ++errors;

    _tprintf(_T("HTTP Error: %d\n"), p_client->GetStatus());
    BYTE* response = nullptr;
    unsigned length = 0;
    p_client->GetResponse(response,length);

    JSONMessage json(reinterpret_cast<TCHAR*>(response));
    json.SetWhitespace(true);
    XString jsonmsg = json.GetJsonMessage();
    _tprintf(jsonmsg.GetString());
  }

  // Reset the OAuth2
  cache.EndSession(session);
  p_client->SetOAuth2Cache(nullptr);
  p_client->SetOAuth2Session(0);


  // SUMMARY OF THE TEST
  // --- "---------------------------------------------- - ------
  _tprintf(_T("MS-Graph test with OAuth2 authorization        : %s\n"), errors ? _T("ERROR") : _T("OK"));
  return errors;
}

