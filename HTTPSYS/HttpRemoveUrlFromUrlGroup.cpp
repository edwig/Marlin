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
#include "UrlGroup.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

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
  UrlGroup* group = GetUrlGroupFromHandle(UrlGroupId);
  if(group == nullptr)
  {
    return ERROR_INVALID_PARAMETER;
  }

  // Find the prefix of the URL registration
  USES_CONVERSION;
  CString prefix(W2A(pFullyQualifiedUrl));

  // And remove from the group
  return group->DelUrlPrefix(prefix,Flags);
}
