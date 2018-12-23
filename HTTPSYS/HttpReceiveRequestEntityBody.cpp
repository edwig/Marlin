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
#include "Request.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

ULONG WINAPI
HttpReceiveRequestEntityBody(IN HANDLE          RequestQueueHandle
                            ,IN HTTP_REQUEST_ID RequestId
                            ,IN ULONG           Flags
                            ,_Out_writes_bytes_to_(EntityBufferLength, *BytesReturned) PVOID EntityBuffer
                            ,IN ULONG           EntityBufferLength
                            ,_Out_opt_ PULONG   BytesReturned
                            ,IN LPOVERLAPPED    Overlapped OPTIONAL)
{
  // Check possible values of the Flags parameter
  if(Flags != 0 && Flags != HTTP_RECEIVE_REQUEST_ENTITY_BODY_FLAG_FILL_BUFFER)
  {
    return ERROR_INVALID_PARAMETER;
  }

  // We do not (yet) support async I/O.
  if(Overlapped)
  {
    return ERROR_INVALID_PARAMETER;
  }

  // We should have a buffer of at least 1 byte
  if(EntityBuffer == nullptr || EntityBufferLength == 0)
  {
    return ERROR_INVALID_PARAMETER;
  }

  // Getting the elementary objects
  RequestQueue* queue = reinterpret_cast<RequestQueue*>(RequestQueueHandle);
  Request*    request = reinterpret_cast<Request*>(RequestId);

  if(queue->RequestStillInService(request))
  {
    return request->ReceiveBuffer(EntityBuffer, EntityBufferLength, BytesReturned, Flags == HTTP_RECEIVE_REQUEST_ENTITY_BODY_FLAG_FILL_BUFFER);
  }
  return ERROR_HANDLE_EOF;
}
