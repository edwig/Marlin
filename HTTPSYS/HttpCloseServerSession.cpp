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
#include "OpaqueHandles.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

ULONG WINAPI
HttpCloseServerSession(IN HTTP_SERVER_SESSION_ID ServerSessionId)
{
  ServerSession* session = g_handles.GetSessionFromOpaqueHandle(ServerSessionId);
  if(session == nullptr)
  {
    return ERROR_INVALID_PARAMETER;
  }
  // Check if it was OUR session
  if(session == g_session)
  {
    if(g_handles.RemoveOpaqueHandle((HANDLE)ServerSessionId))
    {
      delete session;
      g_session = nullptr;
      return NO_ERROR;
    }
  }
  return ERROR_INVALID_PARAMETER;
}
