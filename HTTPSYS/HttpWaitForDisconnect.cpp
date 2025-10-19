//////////////////////////////////////////////////////////////////////////
//
// USER-SPACE IMPLEMENTTION OF HTTP.SYS
//
// 2018 - 2024 (c) ir. W.E. Huisman
// License: MIT
//
//////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "http_private.h"
#include "RequestQueue.h"
#include "PlainSocket.h"
#include "OpaqueHandles.h"

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
  // Finding our request queue
  RequestQueue* queue = g_handles.GetReQueueFromOpaqueHandle(RequestQueueHandle);
  if (queue == nullptr)
  {
    return ERROR_INVALID_PARAMETER;
  }

  // Grab our socket
  PlainSocket* socket = reinterpret_cast<PlainSocket*>(ConnectionId);
  if(socket)
  {
    return socket->RegisterForDisconnect();
  }
  return ERROR_INVALID_PARAMETER;
}
