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

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// This is a API 1.0 call and should not be used

ULONG WINAPI
HttpCreateHttpHandle(OUT PHANDLE RequestQueueHandle, _Reserved_ ULONG Reserved)
{
  return ERROR_INVALID_PARAMETER;
}
