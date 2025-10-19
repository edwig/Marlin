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
#include "Request.h"
#include "WebSocket.h"
#include "HTTPSYS_Websocket.h"
#include "SYSWebSocket.h"
#include "OpaqueHandles.h"

HTTPAPI_LINKAGE
ULONG WINAPI
HttpCloseWebSocket(IN HANDLE                RequestQueueHandle
                  ,IN HTTP_REQUEST_ID       RequestId)
{
  // Getting the elementary objects
  RequestQueue* queue = g_handles.GetReQueueFromOpaqueHandle(RequestQueueHandle);
  Request*    request = g_handles.GetRequestFromOpaqueHandle(RequestId);

  // Check for correct RequestQUeue and current request
  if(queue == nullptr || request == nullptr)
  {
    return ERROR_INVALID_PARAMETER;
  }

  // Returns the WebSocket or nullptr
  SYSWebSocket* websocket = request->GetWebSocket();
  if(websocket == nullptr)
  {
    return ERROR_INVALID_PARAMETER;
  }

  // Close the socket (if not already done)
  websocket->CloseTcpConnection();

  // Clean out the request
  queue->RemoveRequest(request);

  return NO_ERROR;
}