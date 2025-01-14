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
#include "OpaqueHandles.h"
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
  RequestQueue* queue = g_handles.GetReQueueFromOpaqueHandle(RequestQueueHandle);
  if (queue == nullptr)
  {
    return ERROR_INVALID_PARAMETER;
  }

  // Find this queue in our global queues registration
  if(g_handles.RemoveOpaqueHandle(RequestQueueHandle))
  {
    delete queue;
    return NO_ERROR;
  }

  // Queue not found
  return ERROR_INVALID_PARAMETER;
}
