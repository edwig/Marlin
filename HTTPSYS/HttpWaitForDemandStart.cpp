//////////////////////////////////////////////////////////////////////////
//
// USER-SPACE IMPLEMENTTION OF HTTP.SYS
//
// 2018 - 2024 (c) ir. W.E. Huisman
// License: MIT
//
//////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "http_private.h"
#include "RequestQueue.h"
#include "OpaqueHandles.h"

HTTPAPI_LINKAGE
ULONG WINAPI
HttpWaitForDemandStart(IN HANDLE        RequestQueueHandle
                      ,IN LPOVERLAPPED  Overlapped OPTIONAL)
{
  // Overlapping I/O is not supported
  if(Overlapped)
  {
    return ERROR_INVALID_PARAMETER;
  }

  // Finding our request queue
  RequestQueue* queue = g_handles.GetReQueueFromOpaqueHandle(RequestQueueHandle);
  if (queue == nullptr)
  {
    return ERROR_INVALID_PARAMETER;
  }

  // Try to wait on a first request in the queue
  return queue->RegisterDemandStart();
}
