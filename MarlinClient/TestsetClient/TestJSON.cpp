/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: TestJSON.cpp
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
#include "JSONPointer.h"
#include "JSONPath.h"
#include "HTTPClient.h"
#include "MultiPartBuffer.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

int DoSend(HTTPClient* p_client,JSONMessage* p_msg)
{
  int errors = 1;

  if(p_client->Send(p_msg))
  {
    XString msg = p_msg->GetJsonMessage();
    JSONvalue& val = p_msg->GetValue();
    if(val.GetDataType() == JsonType::JDT_object &&
       val.GetObject().size() > 0)
    {
      if(val.GetObject()[0].m_name == _T("one"))
      {
        if(msg == _T("{\"one\":[1,2,3,4,5]}"))
        {
          --errors;
        }
      }
      if(val.GetObject()[0].m_name == _T("two"))
      {
        if(msg == _T("{\"two\":[201,202,203,204.5,205.6789],\"three\":[301,302,303,304.5,305.6789]}"))
        {
          --errors;
        }
      }
    }
  }
  else
  {
    BYTE* response = NULL;
    unsigned length = 0;
    p_client->GetResponse(response,length);
    _tprintf(_T("Message not sent!\n"));
    _tprintf(_T("Service answer: %s\n"),reinterpret_cast<TCHAR*>(response));
    // Raw HTTP error
    _tprintf(_T("HTTP Client error: %s\n"),p_client->GetStatusText().GetString());
  }

  // --- "---------------------------------------------- - ------
  _tprintf(_T("Send: Json message send and received           : %s\n"), errors ? _T("ERROR") : _T("OK"));

  return errors;
}

int TestJsonData(HTTPClient* p_client)
{
  int errors = 0;
  XString url;
  url.Format(_T("http://%s:%d/MarlinTest/Data?test=2&size=medium%%20large"),MARLIN_HOST,TESTING_HTTP_PORT);

  // Test standard JSON
  xprintf(_T("TESTING STANDARD JSON MESSAGE TO /MarlinTest/Data/\n"));
  xprintf(_T("==================================================\n"));

  JSONMessage msg1(_T("\"Test1\""),url);
  msg1.AddHeader(_T("GUID"),_T("888-777-666"));
  errors += DoSend(p_client,&msg1);

  JSONMessage msg2(_T("\"Test2\""),url);
  msg2.AddHeader(_T("GUID"),_T("888-777-666"));
  errors += DoSend(p_client,&msg2);

  // Now Send again in UTF-16 Unicode
  xprintf(_T("TESTING UNICODE JOSN MESSAGE TO /MarlinTest/Data/\n"));
  xprintf(_T("=================================================\n"));


  p_client->SetTimeoutSend(100000);
  p_client->SetTimeoutReceive(100000);

  JSONMessage msg3(_T("\"Test1\""),url);
  msg3.SetSendUnicode(true);
  msg3.AddHeader(_T("GUID"),_T("888-777-666"));
  errors += DoSend(p_client,&msg3);

  JSONMessage msg4(_T("\"Test2\""),url);
  msg4.SetSendUnicode(true);
  msg4.AddHeader(_T("GUID"),_T("888-777-666"));
  errors += DoSend(p_client,&msg4);

  return errors;
}
