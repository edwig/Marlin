//////////////////////////////////////////////////////////////////////////
//
// USER-SPACE IMPLEMENTTION OF HTTP.SYS
//
// 2018 (c) ir. W.E. Huisman
// License: MIT
//
//////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "http_private.h"
#include "RequestQueue.h"
#include "PlainSocket.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

HTTPAPI_LINKAGE
ULONG WINAPI
HttpWaitForDisconnectEx(IN HANDLE             RequestQueueHandle
                       ,IN HTTP_CONNECTION_ID ConnectionId
                       ,_Reserved_ IN ULONG   Reserved   OPTIONAL
                       ,IN LPOVERLAPPED       Overlapped OPTIONAL)
{
  UNREFERENCED_PARAMETER(Reserved);
  return HttpWaitForDisconnect(RequestQueueHandle,ConnectionId,Overlapped);
}

HTTPAPI_LINKAGE
ULONG WINAPI
HttpWaitForDisconnect(IN HANDLE             RequestQueueHandle
                     ,IN HTTP_CONNECTION_ID ConnectionId
                     ,IN LPOVERLAPPED       Overlapped OPTIONAL)
{
  // Not supported
  if(Overlapped)
  {
    return ERROR_INVALID_PARAMETER;
  }

  // Check that we have these parameters
  if(RequestQueueHandle == NULL|| ConnectionId == NULL)
  {
    return ERROR_INVALID_PARAMETER;
  }

  // WHY?
  RequestQueue* queue = reinterpret_cast<RequestQueue*>(RequestQueueHandle);

  // Grab our socket
  PlainSocket* socket = reinterpret_cast<PlainSocket*>(ConnectionId);
  if(socket)
  {
    return socket->RegisterForDisconnect();
  }
  return ERROR_INVALID_PARAMETER;
}
