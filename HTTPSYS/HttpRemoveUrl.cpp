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

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// HTTPServer API 1.0 function. SHould not be called on this implementation!

ULONG WINAPI
HttpRemoveUrl(IN HANDLE RequestQueueHandle
             ,IN PCWSTR FullyQualifiedUrl)
{
  return ERROR_INVALID_PARAMETER;
}
