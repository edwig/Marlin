/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: TestSecureSite.cpp
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

int TestSecureSite(HTTPClient* p_client)
{
  XString url;
  bool result = false;
  url.Format(_T("https://%s:%d/SecureTest/codes.html"),MARLIN_HOST,TESTING_HTTPS_PORT);

  HTTPMessage msg(HTTPCommand::http_get,url);
  XString filename(_T("C:\\TEMP\\codes.html"));
  msg.SetContentType(_T("text/html"));

  // Remove the file !!
  if(_taccess(filename,6) == 0)
  {
    if(DeleteFile(filename) == false)
    {
      _tprintf(_T("Filename [%s] not removed\n"),filename.GetString());
    }
  }

  xprintf(_T("TESTING GET FROM SECURE SITE /SecureTest/\n"));
  xprintf(_T("=========================================\n"));

  // Send our message
  bool sendResult = p_client->Send(&msg);

  // If OK and file does exists now!
  if(sendResult)
  {
    msg.GetFileBuffer()->SetFileName(filename);
    msg.GetFileBuffer()->WriteFile();
    if((_taccess(filename,0) == 0) && (msg.GetFileBuffer()->GetLength() > 0))
    {
      result = true;
    }
  }
  else
  {
    xprintf(_T("ERROR Client received status: %d\n"),p_client->GetStatus());
    xprintf(_T("ERROR %s\n"),p_client->GetStatusText().GetString());

    BYTE*  response = nullptr;
    unsigned length = 0;
    p_client->GetResponse(response,length);
    if(response && length)
    {
      response[length] = 0;
      _tprintf(reinterpret_cast<TCHAR*>(response));
    }
  }
  // SUMMARY OF THE TEST
  // --- "---------------------------------------------- - ------
  _tprintf(_T("HTML page gotten from secure HTTPS site        : %s\n"),result ? _T("OK") : _T("ERROR"));

  return result ? 0 : 1;
}