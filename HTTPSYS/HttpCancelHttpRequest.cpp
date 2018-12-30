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
#include "URL.h"
#include "RequestQueue.h"
#include "Request.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

ULONG WINAPI
HttpCancelHttpRequest(IN HANDLE           RequestQueueHandle
                     ,IN HTTP_REQUEST_ID  RequestId
                     ,IN LPOVERLAPPED     Overlapped OPTIONAL)
{
  // Currently overlapping I/O is unsupported
  if(Overlapped)
  {
    return ERROR_INVALID_PARAMETER;
  }

  // Finding the elementary object
  RequestQueue* queue = GetRequestQueueFromHandle(RequestQueueHandle);
  Request*    request = GetRequestFromHandle(RequestId);
  if (queue == nullptr || request == nullptr)
  {
    return ERROR_INVALID_PARAMETER;
  }

  // Handle our request
  if(queue->RequestStillInService(request))
  {
//     // Try restart on a keep-alive
//     if(request->RestartConnection() == false)
//     {
      // If not: Remove request from servicing queue
      queue->RemoveRequest(request);
//    }
  }
  return NO_ERROR;
}
