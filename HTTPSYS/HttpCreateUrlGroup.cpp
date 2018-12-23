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
HttpCreateUrlGroup(IN  HTTP_SERVER_SESSION_ID ServerSessionId
                  ,OUT PHTTP_URL_GROUP_ID     pUrlGroupId
                  ,_Reserved_ IN ULONG        Reserved)
{
  // See if it is our server session
  ServerSession* session = (ServerSession*)ServerSessionId;
  if(session != g_session)
  {
    return ERROR_INVALID_PARAMETER;
  }

  // Should not happen
  if(Reserved)
  {
    return ERROR_INVALID_PARAMETER;
  }

  // Create and keep the group
  UrlGroup* group = new UrlGroup(session);
  session->AddUrlGroup(group);

  // Reflect the group ID back
  *pUrlGroupId = (HTTP_URL_GROUP_ID)group;

  return NO_ERROR;
}
