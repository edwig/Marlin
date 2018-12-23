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
#include "Request.h"
#include "RequestQueue.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

ULONG WINAPI
HttpSendResponseEntityBody(IN HANDLE          RequestQueueHandle
                          ,IN HTTP_REQUEST_ID RequestId
                          ,IN ULONG           Flags
                          ,IN USHORT          EntityChunkCount OPTIONAL
                          ,_In_reads_opt_(EntityChunkCount) PHTTP_DATA_CHUNK EntityChunks
                          ,OUT PULONG         BytesSent   OPTIONAL
                          ,_Reserved_ PVOID   Reserved1   OPTIONAL
                          ,_Reserved_ ULONG   Reserved2   OPTIONAL
                          ,IN LPOVERLAPPED    Overlapped  OPTIONAL
                          ,IN PHTTP_LOG_DATA  LogData     OPTIONAL)
{
  // Check if these are filled
  if(Reserved1 || Reserved2 || Overlapped)
  {
    return ERROR_INVALID_PARAMETER;
  }

  // We need entity bodies to send
  if(EntityChunkCount == 0 || EntityChunks == nullptr)
  {
    return ERROR_INVALID_PARAMETER;
  }

  // Get elementary objects
  RequestQueue* queue = reinterpret_cast<RequestQueue*>(RequestQueueHandle);
  Request*    request = reinterpret_cast<Request*>(RequestId);
  ULONG        result = ERROR_HANDLE_EOF;
  
  if(queue->RequestStillInService(request))
  {
    ULONG bytes;
    result = request->SendEntityChunks(EntityChunks, EntityChunkCount, &bytes);

    // Sometimes propagate the number of bytes sent
    if (result == NO_ERROR && BytesSent)
    {
      *BytesSent = bytes;
    }


    if ((Flags & (HTTP_SEND_RESPONSE_FLAG_MORE_DATA | HTTP_SEND_RESPONSE_FLAG_OPAQUE)) == 0)
    {
      if (request->GetResponseComplete() || (Flags & HTTP_SEND_RESPONSE_FLAG_DISCONNECT))
      {
        // Request/Response now complete
        queue->RemoveRequest(request);
      }
    }
  }

  // Log our response on the closing of the call
  if (LogData)
  {
    // To implement: Logging of the log data
  }

  return result;
}
