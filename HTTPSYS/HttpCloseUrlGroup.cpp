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
#include "ServerSession.h"
#include "UrlGroup.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

ULONG WINAPI
HttpCloseUrlGroup(IN HTTP_URL_GROUP_ID UrlGroupId)
{
  UrlGroup* group = (UrlGroup*)UrlGroupId;

  ServerSession* session = group->GetServerSession();
  if (session->RemoveUrlGroup(group))
  {
    return NO_ERROR;
  }
  return ERROR_INVALID_PARAMETER;
}
