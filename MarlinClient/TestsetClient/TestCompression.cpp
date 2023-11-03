/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: TestCompression.cpp
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

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

int TestChunkedTransfer(HTTPClient* p_client)
{
  int errors = 0;

  XString url;
  // url = _T("http://anglesharp.azurewebsites.net/Chunked");
  // url = _T("https://www.httpwatch.com/httpgallery/chunked/chunkedimage.aspx?0.1693287678408546");
  // url = _T("https://jigsaw.w3.org/HTTP/ChunkedScript");
  url.Format(_T("http://%s:%d/MarlinTest/Chunking/Releasenotes.txt"),MARLIN_HOST,TESTING_HTTP_PORT);
  HTTPMessage msg(HTTPCommand::http_get,url);

  bool result = p_client->Send(&msg);

  if(result)
  {
    xprintf(_T("Chunked result OK"));
    // Saving a double copy of our latest releasenotes as a test.
    XString filename(_T("C:\\TMP\\CHRelNotes.txt"));
    msg.GetFileBuffer()->SetFileName(filename);
    msg.GetFileBuffer()->WriteFile();
  }
  else
  {
    ++errors; // test failed
  }
  // SUMMARY OF THE TEST
  // --- "---------------------------------------------- - ------
  _tprintf(_T("Testing to receive chunked input in HTTPClient : %s\n"), errors ? _T("ERROR") : _T("OK"));

  return errors;
}

int TestCompression(HTTPClient* p_client)
{
  XString url;
  url.Format(_T("http://%s:%d/MarlinTest/Compression/Releasenotes_v1.txt"),MARLIN_HOST,TESTING_HTTP_PORT);
  HTTPMessage msg(HTTPCommand::http_get,url);
  msg.SetContentType(_T("text/plain"));

  xprintf(_T("TESTING HTTP COMPRESSION GZIP FUNCTION TO /MarlinTest/Compression/\n"));
  xprintf(_T("==================================================================\n"));

  // Set the compression mode
  p_client->SetHTTPCompression(true);

  // Send our message
  bool result = p_client->Send(&msg);

  if(!result)
  {
    // --- "---------------------------------------------- - ------
    xprintf(_T("ERROR Client received status: %d\n"),p_client->GetStatus());
    xprintf(_T("ERROR %s\n"),p_client->GetStatusText().GetString());
  }
  else
  {
    XString filename(_T("CanBeDeleted.txt"));
    msg.GetFileBuffer()->SetFileName(filename);
    msg.GetFileBuffer()->WriteFile();
  }
  // SUMMARY OF THE TEST
  // --- "---------------------------------------------- - ------
  _tprintf(_T("Send: File gotten with gzip compression        : %s\n"),result ? _T("OK") : _T("ERROR"));

  // Reset the compression mode
  p_client->SetHTTPCompression(false);

  return result ? 0 : 1;
}
