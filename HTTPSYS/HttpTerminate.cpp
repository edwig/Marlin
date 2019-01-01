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
#include "RequestQueue.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

ULONG WINAPI
HttpTerminate(IN ULONG Flags,_Reserved_ IN OUT PVOID pReserved)
{
  // We only support in-process servers
  if(Flags != HTTP_INITIALIZE_SERVER)
  {
    return ERROR_INVALID_PARAMETER;
  }

  // Check that reserved is null
  if(pReserved != nullptr)
  {
    return ERROR_INVALID_PARAMETER;
  }

  // Still outstanding request queues: gracefully shutdown
  if(!g_requestQueues.empty())
  {
    for(auto& queue : g_requestQueues)
    {
      delete queue.second;
    }
    g_requestQueues.clear();
  }

  // If still a session open: close it
  if(g_session)
  {
    HttpCloseServerSession((HTTP_SERVER_SESSION_ID)g_session);
  }

  if(g_httpsys_initialized)
  {
    // Clean up all socket info
    WSACleanup();
    // Back to not-initialized
    g_httpsys_initialized = false;
    return NO_ERROR;
  }

  // Wrongly called, not initialized!
  return ERROR_INVALID_PARAMETER;
}
