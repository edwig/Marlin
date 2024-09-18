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
    queue->RemoveRequest(request);
  }

  if(Overlapped)
  {
    // Post a completion to the applications completion port for the request queue
    PostQueuedCompletionStatus(queue->GetIOCompletionPort(),0,queue->GetIOCompletionKey(),Overlapped);
  }
  return NO_ERROR;
}
