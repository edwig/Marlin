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

ULONG WINAPI
HttpAddUrlToUrlGroup(IN HTTP_URL_GROUP_ID UrlGroupId
                    ,IN PCWSTR            pFullyQualifiedUrl
                    ,IN HTTP_URL_CONTEXT  UrlContext OPTIONAL
                    ,_Reserved_ IN ULONG  Reserved)
{
  // Test if reserved is zero
  if(Reserved)
  {
    return ERROR_INVALID_PARAMETER;
  }

  // Find the URL group
  UrlGroup* group = GetUrlGroupFromHandle(UrlGroupId);
  if (group == nullptr)
  {
    return ERROR_INVALID_PARAMETER;
  }

  USES_CONVERSION;
  CString prefix = W2A(pFullyQualifiedUrl);

  return group->AddUrlPrefix(pFullyQualifiedUrl,UrlContext);
}
