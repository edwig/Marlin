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
HttpSendHttpResponse(IN HANDLE              RequestQueueHandle
                    ,IN HTTP_REQUEST_ID     RequestId
                    ,IN ULONG               Flags
                    ,IN PHTTP_RESPONSE      HttpResponse
                    ,IN PHTTP_CACHE_POLICY  CachePolicy OPTIONAL
                    ,OUT PULONG             BytesSent   OPTIONAL
                    ,_Reserved_ PVOID       Reserved1
                    ,_Reserved_ ULONG       Reserved2
                    ,IN LPOVERLAPPED        Overlapped  OPTIONAL
                    ,IN PHTTP_LOG_DATA      LogData     OPTIONAL)
{
  // Check if reserved 1 or 2 or Overlapped are set 
  if(Reserved1 || Reserved2 || Overlapped)
  {
    return ERROR_INVALID_PARAMETER;
  }

  // We totally ignore the Cache Policy parameter: No caching!
  UNREFERENCED_PARAMETER(CachePolicy);

  // Check that we have the BytesSent parameter
  if(BytesSent == nullptr)
  {
    return ERROR_INVALID_PARAMETER;
  }

  // Reset bytes sent
  *BytesSent = 0L;

  // Finding the elementary object
  RequestQueue* queue = GetRequestQueueFromHandle(RequestQueueHandle);
  Request*    request = GetRequestFromHandle(RequestId);
  ULONG        result = ERROR_HANDLE_EOF;

  if (queue == nullptr || request == nullptr)
  {
    return ERROR_INVALID_PARAMETER;
  }

  if(queue->RequestStillInService(request))
  {
    // Create and send the response
    result = request->SendResponse(HttpResponse,Flags,BytesSent);

    if((Flags & (HTTP_SEND_RESPONSE_FLAG_MORE_DATA | HTTP_SEND_RESPONSE_FLAG_OPAQUE)) == 0)
    {
      if(request->GetResponseComplete() || (Flags & HTTP_SEND_RESPONSE_FLAG_DISCONNECT))
      {
        if(request->RestartConnection() == false)
        {
          // Request/Response now complete
          queue->RemoveRequest(request);
        }
      }
    }
  }

  // Log our response
  if (LogData)
  {
    // To implement: Logging of the log data
  }

  return result;
}

