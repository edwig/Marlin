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

  // Check if HttpInitialize was called
  if(!g_httpsys_initialized)
  {
    return ERROR_DLL_INIT_FAILED;
  }

  // Create a name for the request queue
  XString orgName(Name);
  if(orgName.IsEmpty())
  {
    orgName = "HTTPServer";
  }

  // Check if name is unique
  HandleMap map;
  g_handles.GetAllQueueHandles(map);
  for(auto& handle : map)
  {
    RequestQueue* other = g_handles.GetReQueueFromOpaqueHandle(handle.first);
    if(other)
    {
      if(other->GetName().CompareNoCase(orgName) == 0)
      {
        return ERROR_ALREADY_EXISTS;
      }
    }
  }

  // Create the request queue
  RequestQueue* queue = alloc_new RequestQueue(orgName);
  HANDLE handle = queue->CreateHandle();
  
  bool stored = false;
  do 
  {
    stored = g_handles.StoreOpaqueHandle(HTTPHandleType::HTTP_Queue,handle,queue);
    if(!stored)
    {
      // Try a new one
      handle = queue->CreateHandle();
    }
  } 
  while(!stored);
  
  // Tell it our application
  *RequestQueueHandle = handle;
  return NO_ERROR;
}
