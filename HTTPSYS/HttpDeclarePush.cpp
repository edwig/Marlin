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

ULONG WINAPI
HttpDeclarePush(_In_ HANDLE           RequestQueueHandle
               ,_In_ HTTP_REQUEST_ID  RequestId
               ,_In_ HTTP_VERB        Verb
               ,_In_ PCWSTR           Path
               ,_In_opt_ PCSTR        Query
               ,_In_opt_ PHTTP_REQUEST_HEADERS Headers)
{
  // Not implemented: HTTP 2.0 Server push
  return ERROR_IMPLEMENTATION_LIMIT;
}
