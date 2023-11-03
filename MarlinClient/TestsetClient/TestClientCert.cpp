/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: TestClientCertificate.cpp
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
#include "TestClient.h"
#include "HTTPMessage.h"
#include "HTTPClient.h"
#include <io.h>

// STORE NAMES
// MY
// Root
// AddressBook
// AuthRoot
// TrustedPeople
// TrustedPublisher

// CA / MY / ROOT / SPC

// Test if we can find the MarlinClient certificate in my personal store
int TestFindClientCertificate()
{
  HTTPClient client;
  bool result = client.SetClientCertificateThumbprint(_T("MY"),_T("8e 02 b7 fe 7d 0e 6a 35 6d 99 66 64 a5 42 89 7f ba e4 d2 7e"));

  // SUMMARY OF THE TEST
  // --- "---------------------------------------------- - ------
  _tprintf(_T("Can find the Marlin client certificate in store: %s\n"),result ? _T("OK") : _T("ERROR"));

  return result ? 0 : 1;
}

int TestClientCertificate(HTTPClient* p_client)
{
  bool result = false;
  XString url;
  url.Format(_T("https://%s:%d/SecureClientCert/codes.html"),MARLIN_HOST,TESTING_CLCERT_PORT);

  HTTPMessage msg(HTTPCommand::http_get,url);
  XString filename(_T("..\\Documentation\\codes.html"));
  msg.SetContentType(_T("text/html"));

  xprintf(_T("TESTING CLIENT CERTIFICATE FUNCTION TO /SecureClientCert/\n"));
  xprintf(_T("=========================================================\n"));

  // Set the detailed request tracing here
  p_client->SetTraceRequest(true);

  // Setting to true:  No round trips, certificate always sent upfront
  // Setting to false: Certificate will be requested by a round trip
  p_client->SetClientCertificatePreset(false);

  // The client certificate comes from MY:MarlinClient
  p_client->SetClientCertificateStore(_T("MY"));
  p_client->SetClientCertificateName(_T("MarlinClient"));

  // Set both store and certificate and test if it exists!
  result = p_client->SetClientCertificateThumbprint(_T("MY"),_T("8e02b7fe7d0e6a356d996664a542897fbae4d27e"));
  if(result)
  {
    xprintf(_T("Client certificate correctly set\n"));
  }
  else
  {
    xprintf(_T("ERROR Client certificate not found in the store!\n"));
  }

  // Remove the file !!
  if(_taccess(filename,6) == 0)
  {
    if(DeleteFile(filename) == false)
    {
      _tprintf(_T("Filename [%s] not removed\n"),filename.GetString());
    }
  }

  // Send our message
  result = p_client->Send(&msg);

  if(result)
  {
    // Getting the file
    msg.GetFileBuffer()->SetFileName(filename);
    msg.GetFileBuffer()->WriteFile();

    // If OK and file does exists now!
    if((_taccess(filename,0) == 0) && (msg.GetFileBuffer()->GetLength() > 0))
    {
      result = true;
    }
  }
  else
  {
    xprintf(_T("ERROR Client received status: %d\n"),p_client->GetStatus());
    xprintf(_T("ERROR %s\n"),p_client->GetStatusText().GetString());
  }

  // SUMMARY OF THE TEST
  // --- "---------------------------------------------- - ------
  _tprintf(_T("Get file through client certificate check      : %s\n"),result ? _T("OK") : _T("ERROR"));

  // Empty the certificate details
  p_client->SetClientCertificatePreset(false);
  p_client->SetClientCertificateStore(_T(""));
  p_client->SetClientCertificateName(_T(""));

  return result ? 0 : 1;
}
