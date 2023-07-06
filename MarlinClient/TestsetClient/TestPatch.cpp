/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: TestPatch.cpp
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
#include "HTTPClient.h"
#include "GetLastErrorAsString.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

int
DoSend(HTTPClient* p_client,HTTPMessage* p_msg)
{
  int  errors  = 0;
  bool success = false;

  // sending and testing
  success = p_client->Send(p_msg);
  if(success)
  {
    if(p_msg->GetStatus() == HTTP_STATUS_OK)
    {
      XString body = p_msg->GetBody();
      if(body == "AB-\n")
      {
        xprintf("Succesfully patched\n");
      }
      else ++errors;
    }
    else
    {
      // Patching did NOT WORK!!
      ++errors;
    }
  }
  else
  {
    // Not much here to show the error
    ++errors;
  }

  // SUMMARY OF THE TEST
  // --- "---------------------------------------------- - ------
  printf("Send: HTTP PATCH verb (less known protocol)    : %s\n", errors ? "ERROR" : "OK");

  // Ready with the message
  delete p_msg;
  return errors;
}

int TestPatching(HTTPClient* p_client)
{
  int errors = 0;

  // URL with resource and parameters
  XString url;
  url.Format("http://%s:%d/MarlinTest/Patching/FirstPatchTest?type=ab&rhesus=neg",MARLIN_HOST,TESTING_HTTP_PORT);

  // Test 1: Send through a HTTP-VERB Tunnel
  xprintf("TESTING STANDARD HTTP MESSAGE TO /Key2Test/Patching/\n");
  xprintf("======================================================\n");
  HTTPMessage* msg1 = new HTTPMessage(HTTPCommand::http_patch,url);
  msg1->SetBody("Example one: 56123\n"
                "Example two: 98127\n");
  // Test with or without HTTP-VERB tunneling
  msg1->SetVerbTunneling(true);
  // Set extra headers
  msg1->AddHeader("EdosHeader","16-05-1986");

  errors += DoSend(p_client,msg1);


  // Test 2: Now send as a RAW HTTP PATCH command
  HTTPMessage* msg2 = new HTTPMessage(HTTPCommand::http_patch,url);
  msg2->SetBody("Example one: 56123\n"
                "Example two: 98127\n");
  // Test with or without HTTP-VERB tunneling
  msg2->SetVerbTunneling(false);
  // Set extra headers
  msg2->AddHeader("EdosHeader","16-05-1986");

  errors += DoSend(p_client,msg2);

  return errors;
}

