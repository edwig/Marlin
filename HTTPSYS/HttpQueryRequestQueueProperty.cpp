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

HTTPAPI_LINKAGE
ULONG WINAPI
HttpQueryRequestQueueProperty(_In_ HANDLE                 RequestQueueHandle
                             ,_In_ HTTP_SERVER_PROPERTY   Property
                             ,_Out_writes_bytes_to_opt_(PropertyInformationLength, *ReturnLength) PVOID PropertyInformation
                             ,_In_ ULONG                  PropertyInformationLength
                             ,_Reserved_ _In_ ULONG       Reserved1
                             ,_Out_opt_ PULONG            ReturnLength OPTIONAL
                             ,_Reserved_ _In_ PVOID       Reserved2)
{
  // Must always be zero
  if(Reserved1 || Reserved2)
  {
    return ERROR_INVALID_PARAMETER;
  }

  // Parameters must be given
  if(PropertyInformationLength == 0)
  {
    return ERROR_INVALID_PARAMETER;
  }

  // Finding our request queue
  RequestQueue* queue = g_handles.GetReQueueFromOpaqueHandle(RequestQueueHandle);
  if (queue == nullptr)
  {
    return ERROR_INVALID_PARAMETER;
  }

  if(Property == HttpServer503VerbosityProperty)
  {
    if(PropertyInformationLength >= sizeof(HTTP_503_RESPONSE_VERBOSITY))
    {
      *((PHTTP_503_RESPONSE_VERBOSITY)PropertyInformation) = queue->GetVerbosity();
      return NO_ERROR;
    }
  }
  else if(Property == HttpServerQueueLengthProperty)
  {
    if(PropertyInformationLength == sizeof(ULONG))
    {
      *((PULONG)PropertyInformation) = queue->GetQueueLength();
      return NO_ERROR;
    }
  }
  else if(Property == HttpServerStateProperty)
  {
    if(PropertyInformationLength >= sizeof(HTTP_STATE_INFO))
    {
      PHTTP_STATE_INFO info = (PHTTP_STATE_INFO)PropertyInformation;
      info->Flags.Present = 1;
      info->State = queue->GetEnabledState();
      return NO_ERROR;
    }
  }
  return ERROR_INVALID_PARAMETER;
}
