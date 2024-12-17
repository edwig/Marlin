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
#include "OpaqueHandles.h"

ServerSession* g_session = nullptr;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

ULONG WINAPI
HttpCreateServerSession(IN  HTTPAPI_VERSION         Version
                       ,OUT PHTTP_SERVER_SESSION_ID ServerSessionId
                       ,_Reserved_ IN ULONG         Reserved)
{
  // Check if we did not already have a server session
  if(g_session)
  {
    return ERROR_CAN_NOT_COMPLETE;
  }

  // We just support version 2.0 of HTTPSYS
  if(Version.HttpApiMajorVersion != 2 || Version.HttpApiMinorVersion != 0)
  {
    return ERROR_REVISION_MISMATCH;
  }

  // Create session and reflect back
  g_session = new ServerSession();
  HANDLE handle = g_handles.CreateOpaqueHandle(HTTPHandleType::HTTP_Session,g_session);
  *ServerSessionId = (HTTP_SERVER_SESSION_ID) handle;

  return NO_ERROR;
}

