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

ULONG WINAPI
HttpFlushResponseCache(IN HANDLE        RequestQueueHandle
                      ,IN PCWSTR        UrlPrefix
                      ,IN ULONG         Flags
                      ,IN LPOVERLAPPED  Overlapped OPTIONAL)
{
  // Overlapped I/O is yet unsupported
  if(Overlapped)
  {
    return ERROR_IMPLEMENTATION_LIMIT;
  }

  // Mandatory parameters
  if(RequestQueueHandle == nullptr || UrlPrefix == nullptr)
  {
    return ERROR_INVALID_PARAMETER;
  }

  // Only supported flags in HTTP.SYS
  if(Flags != 0 && Flags != HTTP_FLUSH_RESPONSE_FLAG_RECURSIVE)
  {
    return ERROR_INVALID_PARAMETER;
  }

  // Finding our request queue
  RequestQueue* queue = g_handles.GetReQueueFromOpaqueHandle(RequestQueueHandle);
  if(queue == nullptr)
  {
    return ERROR_INVALID_PARAMETER;
  }

  // Prefix name of the object to flush
  USES_CONVERSION;
  XString prefix(W2A(UrlPrefix));

  // Flush the fragment cache
  return queue->FlushFragment(prefix,Flags);
}
