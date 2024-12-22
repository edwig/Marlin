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

typedef HTTP_OPAQUE_ID HTTP_REQUEST_ID;

typedef enum _WEB_SOCKET_PROPERTY_TYPE
{
    WEB_SOCKET_RECEIVE_BUFFER_SIZE_PROPERTY_TYPE        = 0,
    WEB_SOCKET_SEND_BUFFER_SIZE_PROPERTY_TYPE           = 1,
    WEB_SOCKET_DISABLE_MASKING_PROPERTY_TYPE            = 2,
    WEB_SOCKET_ALLOCATED_BUFFER_PROPERTY_TYPE           = 3,
    WEB_SOCKET_DISABLE_UTF8_VERIFICATION_PROPERTY_TYPE  = 4,
    WEB_SOCKET_KEEPALIVE_INTERVAL_PROPERTY_TYPE         = 5,
    WEB_SOCKET_SUPPORTED_VERSIONS_PROPERTY_TYPE         = 6,
} WEB_SOCKET_PROPERTY_TYPE;


DECLARE_HANDLE(WEB_SOCKET_HANDLE);

typedef struct _WEB_SOCKET_PROPERTY
{
  WEB_SOCKET_PROPERTY_TYPE Type;
  PVOID pvValue;
  ULONG ulValueSize;
} 
WEB_SOCKET_PROPERTY, *PWEB_SOCKET_PROPERTY;

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
