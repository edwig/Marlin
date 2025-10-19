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

// This is a API 1.0 call and should not be used

ULONG WINAPI
HttpCreateHttpHandle(OUT PHANDLE RequestQueueHandle, _Reserved_ ULONG Reserved)
{
  return ERROR_INVALID_PARAMETER;
}
