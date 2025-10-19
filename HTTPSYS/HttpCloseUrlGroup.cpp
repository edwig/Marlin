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
HttpCloseUrlGroup(IN HTTP_URL_GROUP_ID UrlGroupId)
{
  // Find the URL group
  UrlGroup* group = g_handles.GetUrGroupFromOpaqueHandle(UrlGroupId);
  if(group == nullptr)
  {
    // Group was already removed 
    // e.q. by HttpRemoveUrlFromUrlGroup
    return NO_ERROR;
  }

  ServerSession* session = group->GetServerSession();
  if(session->RemoveUrlGroup(UrlGroupId,group))
  {
    return NO_ERROR;
  }
  return ERROR_INVALID_PARAMETER;
}
