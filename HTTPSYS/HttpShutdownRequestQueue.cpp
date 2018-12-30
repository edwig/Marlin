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
  RequestQueues::iterator it = std::find(g_requestQueues.begin(),g_requestQueues.end(),queue);
  if(it != g_requestQueues.end())
  {
    (*it)->ClearIncomingWaiters();
    return NO_ERROR;
  }

  return ERROR_INVALID_PARAMETER;
}
