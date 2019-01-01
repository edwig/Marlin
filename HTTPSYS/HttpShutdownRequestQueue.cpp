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
#include <vector>
#include <algorithm>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

ULONG WINAPI
HttpShutdownRequestQueue(IN HANDLE RequestQueueHandle)
{
  // Finding our request queue
  RequestQueue* queue = GetRequestQueueFromHandle(RequestQueueHandle);
  if (queue == nullptr)
  {
    return ERROR_INVALID_PARAMETER;
  }

  // Remove from global queue if possible
  queue->ClearIncomingWaiters();
  return NO_ERROR;
}
