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

// The system is/was initialized by calling HttpInitialize
bool    g_httpsys_initialized = false;

// Initialization follows here
HTTPAPI_LINKAGE
ULONG WINAPI
HttpInitialize(IN HTTPAPI_VERSION Version, IN ULONG Flags,_Reserved_ IN OUT PVOID /*pReserved*/)
{
  // See if doubly called
  if(g_httpsys_initialized)
  {
    return ERROR_CAN_NOT_COMPLETE;
  }

  // For now we just support version 2.0 of HTTP.SYS
  if(Version.HttpApiMajorVersion != 2 || Version.HttpApiMinorVersion != 0)
  {
    return ERROR_REVISION_MISMATCH;
  }

  // For now we just support in-process server initialization
  // Service configuration not yet supported
  if (Flags != HTTP_INITIALIZE_SERVER)
  {
    return ERROR_INVALID_PARAMETER;
  }

  // Startup WinSockets
  // Recent versions of MS-Windows use WinSock 2.2 library
  WSADATA wsaData;
  WORD wVersionRequested = MAKEWORD(2,2);
  if (WSAStartup(wVersionRequested,&wsaData))
  {
    // Could not start WinSockets?
    return ERROR_INVALID_PARAMETER;
  }

  // Ready to roll
  g_httpsys_initialized = true;

  // All OK
  return NO_ERROR;
}
