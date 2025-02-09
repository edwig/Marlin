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
#include "URL.h"
#include "RequestQueue.h"
#include "OpaqueHandles.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

ULONG WINAPI
HttpAddFragmentToCache(IN HANDLE              RequestQueueHandle
                      ,IN PCWSTR              UrlPrefix
                      ,IN PHTTP_DATA_CHUNK    DataChunk
                      ,IN PHTTP_CACHE_POLICY  CachePolicy
                      ,IN LPOVERLAPPED        Overlapped OPTIONAL)
{
  // We do not yet support Overlapped I/O
  if(Overlapped)
  {
    return ERROR_IMPLEMENTATION_LIMIT;
  }

  // Mandatory needed for this function
  if(UrlPrefix == nullptr || DataChunk == nullptr)
  {
    return ERROR_INVALID_PARAMETER;
  }

  // Only user invalidated fragments are accepted in the fragment cache
  // Time-to-live parameter not yet implemented
  if(CachePolicy && CachePolicy->Policy != HttpCachePolicyUserInvalidates)
  {
    return ERROR_INVALID_PARAMETER;
  }

  // Only memory fragments are accepted into the fragment cache
  if(DataChunk->DataChunkType != HttpDataChunkFromMemory)
  {
    return ERROR_INVALID_PARAMETER;
  }

  // Finding our request queue
  RequestQueue* queue = g_handles.GetReQueueFromOpaqueHandle(RequestQueueHandle);
  if (queue == nullptr)
  {
    return ERROR_INVALID_PARAMETER;
  }

  USES_CONVERSION;
  XString prefix(W2A(UrlPrefix));

  // Add fragment to the fragment cache of the request queue
  return queue->AddFragment(prefix,DataChunk);
}
