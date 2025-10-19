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
#include "ServerSession.h"
#include "UrlGroup.h"
#include "OpaqueHandles.h"

ULONG WINAPI
HttpCreateUrlGroup(IN  HTTP_SERVER_SESSION_ID ServerSessionId
                  ,OUT PHTTP_URL_GROUP_ID     pUrlGroupId
                  ,_Reserved_ IN ULONG        Reserved)
{
  // Getting the server session
  ServerSession* session = g_handles.GetSessionFromOpaqueHandle(ServerSessionId);
  if(session == nullptr)
  {
    return ERROR_INVALID_PARAMETER;
  }
  // See if it is **OUR** server session
  if(session != g_session)
  {
    return ERROR_INVALID_PARAMETER;
  }

  // Should not happen
  if(Reserved || pUrlGroupId == nullptr)
  {
    return ERROR_INVALID_PARAMETER;
  }

  // Create and keep the group
  UrlGroup* group = alloc_new UrlGroup(session);
  session->AddUrlGroup(group);

  // Reflect the group ID back
  HANDLE handle = g_handles.CreateOpaqueHandle(HTTPHandleType::HTTP_UrlGroup,group);
  *pUrlGroupId = (HTTP_URL_GROUP_ID)handle;

  return NO_ERROR;
}
