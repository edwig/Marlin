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

// Marlin extensions to HTTP_SERVER_PROPERTY enumeration
#define HttpServerIOCPPort 32
#define HttpServerIOCPKey  33


ULONG WINAPI
HttpSetRequestQueueProperty(_In_ HANDLE               RequestQueueHandle
                           ,_In_ HTTP_SERVER_PROPERTY Property
                           ,_In_reads_bytes_(PropertyInformationLength) PVOID PropertyInformation
                           ,_In_ ULONG                PropertyInformationLength
                           ,_Reserved_ _In_ ULONG     Reserved1
                           ,_Reserved_ _In_ PVOID     Reserved2)
{
  // Finding our request queue
  RequestQueue* queue = g_handles.GetReQueueFromOpaqueHandle(RequestQueueHandle);
  if (queue == nullptr)
  {
    return ERROR_INVALID_PARAMETER;
  }

  if(Reserved1 != 0L || Reserved2 != nullptr)
  {
    return ERROR_NOT_SUPPORTED;
  }

  LONGLONG value = 0;
  switch(PropertyInformationLength)
  {
    case 1:   value = (LONGLONG)(*((unsigned char *)  PropertyInformation));
              break;
    case 2:   value = (LONGLONG)(*((short *)          PropertyInformation));
              break;
    case 4:   value = (LONGLONG)(*((int *)            PropertyInformation));
              break;
    case 8:   value = (LONGLONG)(*((__int64 *)        PropertyInformation));
              break;
    default:  return ERROR_INVALID_PARAMETER;
  }

  if(Property == HttpServerLoggingProperty)
  {
    if(PropertyInformationLength == sizeof(HTTP_LOGGING_INFO))
    {
      if(!g_session->SetupForLogging((PHTTP_LOGGING_INFO)PropertyInformation))
      {
        return ERROR_INVALID_PARAMETER;
      }
    }
  }
  else if(Property == HttpServer503VerbosityProperty)
  {
    if(!queue->SetVerbosity((HTTP_503_RESPONSE_VERBOSITY)value))
    {
      return ERROR_INVALID_PARAMETER;
    }
  }
  else if (Property == HttpServerQueueLengthProperty)
  {
    if(!queue->SetQueueLength((ULONG)value))
    {
      return ERROR_INVALID_PARAMETER;
    }
  }
  else if (Property == HttpServerStateProperty)
  {
    if(!queue->SetEnabledState((HTTP_ENABLED_STATE)value))
    {
      return ERROR_INVALID_PARAMETER;
    }
  }
  else if (Property == HttpServerIOCPPort)
  {
    if(!queue->SetIOCompletionPort((HANDLE)value))
    {
      return ERROR_INVALID_PARAMETER;
    }
  }
  else if (Property == HttpServerIOCPKey)
  {
    if(!queue->SetIOCompletionKey((ULONG_PTR)value))
    {
      return ERROR_INVALID_PARAMETER;
    }
  }
  else
  {
    // Wrong property
    return ERROR_INVALID_PARAMETER;
  }
  return NO_ERROR;
}
