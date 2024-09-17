#include "stdafx.h"
#include "TestClient.h"
#include "HTTPMessage.h"
#include "HTTPClient.h"
#include <io.h>

bool 
PreFlight(HTTPClient* p_client,HTTPMessage& p_msg,XString p_method,XString p_headers)
{
  // See if we may do a GET on this site
  p_client->SetCORSPreFlight(p_method,p_headers);

  bool result = p_client->Send(&p_msg);
  if(result == false)
  {
    // HTTP error on CORS pre-flight check
    // --- "---------------------------------------------- - ------
    _tprintf(_T("ERROR CORS pre-flight check returned status    : STATUS %d\n"),p_client->GetStatus());
  }
  return result;
}

int
TestGet(HTTPClient* p_client)
{
  bool result = false;
  XString url;
  url.Format(_T("http://%s:%d/MarlinTest/Site/FileOne.html"),MARLIN_HOST,TESTING_HTTP_PORT);

  HTTPMessage pre(HTTPCommand::http_options,url);
  HTTPMessage msg(HTTPCommand::http_get,url);
  XString filename(_T("C:\\TEMP\\FileOne.html"));
  msg.SetContentType(_T("text/html"));

  // Remove the file !!
  if(_taccess(filename,6) == 0)
  {
    if(DeleteFile(filename) == false)
    {
      _tprintf(_T("Filename [%s] not removed\n"),filename.GetString());
    }
  }

  xprintf(_T("TESTING GET FROM BASE SITE /MarlinTest/Site/\n"));
  xprintf(_T("============================================\n"));

  // Prepare client for CORS checking
  p_client->SetCORSOrigin(_T("http://localhost"));

  // Send our message
  bool sendResult = p_client->Send(&msg);

  // If OK and file does exists now!
  if(sendResult)
  {
    msg.GetFileBuffer()->SetFileName(filename);
    msg.GetFileBuffer()->WriteFile();
    if(_taccess(filename,0) == 0)
    {
      result = true;
    }
  }
  else
  {
    _tprintf(_T("ERROR Client received status: %d\n"),p_client->GetStatus());
    _tprintf(_T("ERROR %s\n"),p_client->GetStatusText().GetString());

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
  _tprintf(_T("HTML page gotten from base HTTP site           : %s\n"),result ? _T("OK") : _T("ERROR"));

  // RESET CORS checking for the client!
  p_client->SetCORSOrigin(_T(""));

  return result ? 0 : 1;
}

int
TestPut(HTTPClient* p_client)
{
  bool result = false;
  XString url;
  url.Format(_T("http://%s:%d/MarlinTest/Site/FileThree.html"),MARLIN_HOST,TESTING_HTTP_PORT);
  HTTPMessage msg(HTTPCommand::http_put,url);

  // This file was downloaded in the last 'get' test
  XString filename(_T("C:\\TEMP\\FileOne.html"));
  msg.GetFileBuffer()->SetFileName(filename);

  // Remove the file !!
  if(_taccess(filename,6) != 0)
  {
    _tprintf(_T("Filename to upload [%s] not found\n"),filename.GetString());
  }

  xprintf(_T("TESTING PUT TO   BASE SITE /MarlinTest/Site/\n"));
  xprintf(_T("============================================\n"));

  // Prepare client for CORS checking
  p_client->SetCORSOrigin(_T("http://localhost"));

  // Send our message
  bool sendResult = p_client->Send(&msg);

  // If OK and file does exists now!
  if(sendResult)
  {
    result = true;
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
  _tprintf(_T("HTML page put to base HTTP site                : %s\n"),result ? _T("OK") : _T("ERROR"));

  // RESET CORS checking for the client!
  p_client->SetCORSOrigin(_T(""));
  p_client->Reset();

  return result ? 0 : 1;
}

int
TestBaseSite(HTTPClient* p_client)
{
  // Testing the HTTP 'GET'
  if(TestGet(p_client) == 0)
  {
    // If the 'GET' succeeds, try the 'PUT'
    // return 0;
    return TestPut(p_client);
  }
  return 1;
}
