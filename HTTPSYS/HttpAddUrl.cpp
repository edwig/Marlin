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

// BEWARE: This is a HTTP Server API 1.0 function
//         it is not supported by this driver

ULONG WINAPI
HttpAddUrl(IN HANDLE RequestQueueHandle
          ,IN PCWSTR FullyQualifiedUrl
          ,_Reserved_ PVOID Reserved)
{
  return ERROR_INVALID_PARAMETER;
}
