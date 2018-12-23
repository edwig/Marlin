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
HttpCreateRequestQueue(IN  HTTPAPI_VERSION      Version
                      ,IN  PCWSTR               Name                OPTIONAL
                      ,IN  PSECURITY_ATTRIBUTES SecurityAttributes  OPTIONAL
                      ,IN  ULONG                Flags               OPTIONAL
                      ,OUT PHANDLE              RequestQueueHandle)
{
  // We just support version 2.0 of HTTPSYS
  if(Version.HttpApiMajorVersion != 2 || Version.HttpApiMinorVersion != 0)
  {
    return ERROR_REVISION_MISMATCH;
  }

  if(Name == nullptr && Flags & HTTP_CREATE_REQUEST_QUEUE_FLAG_OPEN_EXISTING)
  {
    return ERROR_INVALID_PARAMETER;
  }

  // We do NOT support control queues
  if(Flags & HTTP_CREATE_REQUEST_QUEUE_FLAG_CONTROLLER)
  {
    return ERROR_INVALID_PARAMETER;
  }

  // Names cannot be longer than this
  if(Name && wcslen(Name) > MAX_PATH)
  {
    return ERROR_INVALID_PARAMETER;
  }

  // We do not (yet) support security aspects on out-of-process queues
  if(SecurityAttributes != nullptr)
  {
    return ERROR_INVALID_PARAMETER;
  }

  USES_CONVERSION;
  CString name = W2A(Name);

  // Create the request queue
  RequestQueue* queue = new RequestQueue(Name);
  *RequestQueueHandle = (HANDLE)queue;

  // Remember our queue
  g_requestQueues.push_back(queue);

  return NO_ERROR;
}

