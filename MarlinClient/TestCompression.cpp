/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: TestCompression.cpp
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
#include "HTTPClient.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

int TestCompression(HTTPClient* p_client)
{
  CString url = "http://" MARLIN_HOST ":1200/MarlinTest/Compression/Releasenotes_v1.txt";
  HTTPMessage msg(HTTPCommand::http_get,url);
  msg.SetContentType("text/plain");

  printf("TESTING HTTP COMPRESSION GZIP FUNCTION TO /MarlinTest/Compression/\n");
  printf("==================================================================\n");

  // Set the compression mode
  p_client->SetHTTPCompression(true);
  p_client->SetReadAllHeaders(true);

  // Send our message
  bool result = p_client->Send(&msg);

  if(!result)
  {
    printf("ERROR Client received status: %d\n",p_client->GetStatus());
    printf("ERROR %s\n",(LPCTSTR)p_client->GetStatusText());
  }
  else
  {
    CString filename("CanBeDeleted.txt");
    msg.GetFileBuffer()->SetFileName(filename);
    msg.GetFileBuffer()->WriteFile();
  }
  // SUMMARY OF THE TEST
  // --- "--------------------------- - ------\n"
  printf("FILE GOTTEN WITH COMPRESSION: %s\n",result ? "OK" : "ERROR");

  // Reset the compression mode
  p_client->SetHTTPCompression(false);

  return result ? 0 : 1;
}
