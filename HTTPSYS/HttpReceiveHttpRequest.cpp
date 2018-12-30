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

ULONG WINAPI
HttpReceiveHttpRequest(IN HANDLE          RequestQueueHandle
                      ,IN HTTP_REQUEST_ID RequestId
                      ,IN ULONG           Flags
                      ,_Out_writes_bytes_to_(RequestBufferLength, *BytesReturned) PHTTP_REQUEST RequestBuffer
                      ,IN ULONG           RequestBufferLength
                      ,_Out_opt_ PULONG   BytesReturned
                      ,IN LPOVERLAPPED    Overlapped OPTIONAL)
{
  // VARIOUS CHECKS

  // Both cannot be supplied (sync I/O or Async I/O must be performed)
  if(BytesReturned && Overlapped)
  {
    return ERROR_INVALID_PARAMETER;
  }

  // Flags must be 0 or one of the body commands
  if(Flags && Flags != HTTP_RECEIVE_REQUEST_FLAG_COPY_BODY && Flags != HTTP_RECEIVE_REQUEST_FLAG_FLUSH_BODY)
  {
    return ERROR_INVALID_PARAMETER;
  }

  // RequestBuffer must be at least as big as the HTTP_REQUEST
  if(RequestBufferLength < sizeof(HTTP_REQUEST_V2) + 16)
  {
    return ERROR_INVALID_PARAMETER;
  }

  // If we re-issue a read call, it must be for the same request
  if(RequestId && RequestId != RequestBuffer->RequestId)
  {
    return ERROR_INVALID_PARAMETER;
  }

  // SORRY: Currently not supported by this driver
  // Must create overlapped I/O implementation on a later date
  if(Overlapped)
  {
    return ERROR_IMPLEMENTATION_LIMIT;
  }

  // Find the request queue
  RequestQueue* queue = GetRequestQueueFromHandle(RequestQueueHandle);
  if(queue == nullptr)
  {
    return ERROR_INVALID_PARAMETER;
  }

  // Get the next request from the queue
  ULONG bytes  = 0;
  ULONG result = queue->GetNextRequest(RequestId,Flags,RequestBuffer,RequestBufferLength,&bytes);

  // Reflect number of bytes read
  if(BytesReturned)
  {
    *BytesReturned = bytes;
  }

  return result;
}
