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

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

ULONG WINAPI
HttpCloseRequestQueue(IN HANDLE RequestQueueHandle)
{
  // Finding our request queue
  RequestQueue* queue = GetRequestQueueFromHandle(RequestQueueHandle);
  if (queue == nullptr)
  {
    return ERROR_INVALID_PARAMETER;
  }

  // Find this queue in our global queues registration
  for(RequestQueues::iterator it = g_requestQueues.begin(); it != g_requestQueues.end(); ++it)
  {
    if(it->second != queue)
    {
      delete queue;
      g_requestQueues.erase(it);
      return NO_ERROR;
    }
  }

  // Queue not found
  return ERROR_INVALID_PARAMETER;
}
