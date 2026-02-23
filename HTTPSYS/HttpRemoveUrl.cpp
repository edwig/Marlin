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

// HTTPServer API 1.0 function. SHould not be called on this implementation!

ULONG WINAPI
HttpRemoveUrl(IN HANDLE /*RequestQueueHandle*/
             ,IN PCWSTR /*FullyQualifiedUrl*/)
{
  return ERROR_INVALID_PARAMETER;
}
