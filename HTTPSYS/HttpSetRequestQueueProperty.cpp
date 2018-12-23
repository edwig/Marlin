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

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

ULONG WINAPI
HttpSetRequestQueueProperty(_In_ HANDLE               RequestQueueHandle
                           ,_In_ HTTP_SERVER_PROPERTY Property
                           ,_In_reads_bytes_(PropertyInformationLength) PVOID PropertyInformation
                           ,_In_ ULONG                PropertyInformationLength
                           ,_Reserved_ _In_ ULONG     Reserved1
                           ,_Reserved_ _In_ PVOID     Reserved2)
{
  RequestQueue* queue = (RequestQueue*)RequestQueueHandle;
  if (queue->GetIdent() != HTTP_QUEUE_IDENT)
  {
    return ERROR_INVALID_PARAMETER;
  }

  if(Reserved1 != 0L || Reserved2 != nullptr)
  {
    return ERROR_NOT_SUPPORTED;
  }

  LONG value = 0;
  switch(PropertyInformationLength)
  {
    case 1:   value = (LONG)(*((unsigned char *)  PropertyInformation));
              break;
    case 2:   value = (LONG)(*((short *)          PropertyInformation));
              break;
    case 4:   value = (LONG)(*((int *)            PropertyInformation));
              break;
    case 8:   value = (LONG)(*((__int64 *)        PropertyInformation));
              break;
    default:  return ERROR_INVALID_PARAMETER;
  }

  if(Property == HttpServer503VerbosityProperty)
  {
    if(!queue->SetVerbosity((HTTP_503_RESPONSE_VERBOSITY)value))
    {
      return ERROR_INVALID_PARAMETER;
    }
  }
  else if (Property == HttpServerQueueLengthProperty)
  {
    if(!queue->SetQueueLength(value))
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
  else
  {
    // Wrong property
    return ERROR_INVALID_PARAMETER;
  }
  return NO_ERROR;
}
