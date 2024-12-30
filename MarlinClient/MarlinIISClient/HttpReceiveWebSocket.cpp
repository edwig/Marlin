//////////////////////////////////////////////////////////////////////////
//
// STUB FUNCTION to prevent linkage errors
//
// 2024 (c) ir. W.E. Huisman
// License: MIT
//
//////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include <http.h>
#include <websocket.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

class HTTPSYS_WebSocket
{
public:
  HTTPSYS_WebSocket() = default;
 ~HTTPSYS_WebSocket() = default;
};

//////////////////////////////////////////////////////////////////////////
//
// The actual call
// THIS IS THE EFFECTIVLY MISSING CALL FROM THE HTTPServerAPI in HTTP.SYS
//
//////////////////////////////////////////////////////////////////////////

HTTPSYS_WebSocket* WINAPI
HttpReceiveWebSocket(IN HANDLE                /*RequestQueueHandle*/
                    ,IN HTTP_REQUEST_ID       /*RequestId*/
                    ,IN WEB_SOCKET_HANDLE     /*SocketHandle*/
                    ,IN PWEB_SOCKET_PROPERTY  /*SocketProperties */ OPTIONAL
                    ,IN DWORD                 /*PropertyCount*/     OPTIONAL)
{
  return nullptr;
}

ULONG WINAPI
HttpCloseWebSocket(IN HANDLE          /*RequestQueueHandle*/
                  ,IN HTTP_REQUEST_ID /*RequestId*/)
{
  return 0L;
}
