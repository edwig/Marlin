/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: TestClientCertificate.cpp
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
#include "TestClient.h"
#include "HTTPMessage.h"
#include "HTTPClient.h"

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
  bool result = client.SetClientCertificateThumbprint("MY","db 34 40 64 f2 fa c2 13 18 dd 90 f5 07 fe 78 e8 1b 03 16 00");

  // SUMMARY OF THE TEST
  // --- "--------------------------- - ------\n"
  printf("CAN FIND CERTIFICATE        : %s\n",result ? "OK" : "ERROR");

  return result ? 0 : 1;
}

int TestClientCertificate(HTTPClient* p_client)
{
  CString url = "https://" MARLIN_HOST "/MarlinTest/SecureClient/Programming1.vsd";
  HTTPMessage msg(HTTPCommand::http_put,url);
  CString filename("..\\Documentation\\Programming1.vsd");
  msg.GetFileBuffer()->SetFileName(filename);
  msg.SetContentType("application/octet-stream");

  printf("TESTING CLIENT CERTIFICATE FUNCTION TO /MarlinTest/SecureClient/\n");
  printf("================================================================\n");

  // Set the detailed request tracing here
  // p_client->SetTraceRequest(true);

  // Setting to true:  No roundtrips, certificate always sent upfront
  // Setting to false: Certificate will be requested by a roundtrip
  p_client->SetClientCertificatePreset(true);

  // The client certificate comes from MY:MarlinClient
  // p_client->SetClientCertificateStore("MY");
  // p_client->SetClientCertificateName("MarlinClient");

  // Set both store and certificate and test if it exists!
  bool result = p_client->SetClientCertificateThumbprint("MY","db344064f2fac21318dd90f507fe78e81b031600");
  if(result)
  {
    printf("Client certificate correctly set\n");
  }
  else
  {
    printf("ERROR Client certificate not found in the store!\n");
  }

  // Send our message
  result = p_client->Send(&msg);

  if(!result)
  {
    printf("ERROR Client received status: %d\n",p_client->GetStatus());
    printf("ERROR %s\n",(LPCTSTR)p_client->GetStatusText());
  }

  // SUMMARY OF THE TEST
  // --- "--------------------------- - ------\n"
  printf("PUT FILE THROUGH CERTIFICATE: %s\n",result ? "OK" : "ERROR");

  // Empty the certificate details
  p_client->SetClientCertificatePreset(false);
  p_client->SetClientCertificateStore("");
  p_client->SetClientCertificateName("");

  // Reset the tracing option
  p_client->SetTraceRequest(false);

  return result ? 0 : 1;
}
