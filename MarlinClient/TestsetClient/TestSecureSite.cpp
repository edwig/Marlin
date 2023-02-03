#include "stdafx.h"
#include "TestClient.h"
#include "HTTPMessage.h"
#include "HTTPClient.h"
#include <io.h>


int TestSecureSite(HTTPClient* p_client)
{
  CString url;
  bool result = false;
  url.Format("https://%s:%d/SecureTest/codes.html",MARLIN_HOST,TESTING_HTTPS_PORT);

  HTTPMessage msg(HTTPCommand::http_get,url);
  CString filename("C:\\TEMP\\codes.html");
  msg.SetContentType("text/html");

  // Remove the file !!
  if(_access(filename,6) == 0)
  {
    if(DeleteFile(filename) == false)
    {
      printf("Filename [%s] not removed\n",filename.GetString());
    }
  }

  xprintf("TESTING GET FROM SECURE SITE /SecureTest/\n");
  xprintf("=========================================\n");

  // Send our message
  bool sendResult = p_client->Send(&msg);

  // If OK and file does exists now!
  if(sendResult)
  {
    msg.GetFileBuffer()->SetFileName(filename);
    msg.GetFileBuffer()->WriteFile();
    if((_access(filename,0) == 0) && (msg.GetFileBuffer()->GetLength() > 0))
    {
      result = true;
    }
  }
  else
  {
    xprintf("ERROR Client received status: %d\n",p_client->GetStatus());
    xprintf("ERROR %s\n",(LPCTSTR)p_client->GetStatusText());

    BYTE*  response = nullptr;
    unsigned length = 0;
    p_client->GetResponse(response,length);
    if(response && length)
    {
      response[length] = 0;
      printf((char*) response);
    }
  }
  // SUMMARY OF THE TEST
  // --- "---------------------------------------------- - ------
  printf("HTML page gotten from secure HTTPS site        : %s\n",result ? "OK" : "ERROR");

  return result ? 0 : 1;
}