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

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

ULONG WINAPI
HttpReadFragmentFromCache(IN HANDLE           RequestQueueHandle
                         ,IN PCWSTR           UrlPrefix
                         ,IN PHTTP_BYTE_RANGE ByteRange OPTIONAL
                         ,_Out_writes_bytes_to_(BufferLength, *BytesRead) PVOID Buffer
                         ,IN ULONG            BufferLength
                         ,_Out_opt_ PULONG    BytesRead
                         ,IN LPOVERLAPPED     Overlapped OPTIONAL)
{
  // Overlapped I/O not yet supported
  if(Overlapped)
  {
    return ERROR_IMPLEMENTATION_LIMIT;
  }

  // Mandatory parameters must be supplied
  if(RequestQueueHandle == NULL || UrlPrefix == nullptr || Buffer == nullptr || BytesRead == nullptr || BufferLength == 0)
  {
    return ERROR_INVALID_PARAMETER;
  }

  // Finding our request queue
  RequestQueue* queue = g_handles.GetReQueueFromOpaqueHandle(RequestQueueHandle);
  if(queue == nullptr)
  {
    return ERROR_INVALID_PARAMETER;
  }

  USES_CONVERSION;
  XString prefix(W2A(UrlPrefix));

  PHTTP_DATA_CHUNK chunk = queue->FindFragment(prefix);
  if(chunk)
  {
    ULONG size = chunk->FromMemory.BufferLength;
    *BytesRead = size;
    if(BufferLength < size)
    {
      // Reporting the number of bytes needed in 'BytesRead'
      return ERROR_MORE_DATA;
    }
    memcpy(Buffer,chunk->FromMemory.pBuffer,size);
    return NO_ERROR;
  }
  return ERROR_NOT_FOUND;
}
