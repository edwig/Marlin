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
#include "UrlGroup.h"
#include "OpaqueHandles.h"

HTTPAPI_LINKAGE
ULONG WINAPI
HttpRemoveUrlFromUrlGroup(IN HTTP_URL_GROUP_ID  UrlGroupId
                         ,IN PCWSTR             pFullyQualifiedUrl
                         ,IN ULONG              Flags)
{
  // Check mandatory parameters
  if(UrlGroupId == NULL || pFullyQualifiedUrl == nullptr)
  {
    return ERROR_INVALID_PARAMETER;
  }

  // Check values for flags
  if(Flags != 0 && Flags != HTTP_URL_FLAG_REMOVE_ALL)
  {
    return ERROR_INVALID_PARAMETER;
  }

  // Find the URL group
  UrlGroup* group = g_handles.GetUrGroupFromOpaqueHandle(UrlGroupId);
  if(group == nullptr)
  {
    return ERROR_INVALID_PARAMETER;
  }

  // Decrease the number of endpoints
  g_session->RemoveEndpoint();

  // Find the prefix of the URL registration
  XString fullUrl(pFullyQualifiedUrl);

  // And remove from the group
  return group->DelUrlPrefix(UrlGroupId,fullUrl,Flags);
}
