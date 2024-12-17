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
#include "RequestQueue.h"
#include "Request.h"
#include "WebSocket.h"
#include "HTTPSYS_Websocket.h"
#include "SYSWebSocket.h"
#include "OpaqueHandles.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


//////////////////////////////////////////////////////////////////////////
//
// The actual call
// THIS IS THE EFFECTIVLY MISSING CALL FROM THE HTTPServerAPI in HTTP.SYS
//
//////////////////////////////////////////////////////////////////////////

HTTPAPI_LINKAGE
HTTPSYS_WebSocket* WINAPI
HttpReceiveWebSocket(IN HANDLE                RequestQueueHandle
                    ,IN HTTP_REQUEST_ID       RequestId
                    ,IN WEB_SOCKET_HANDLE     SocketHandle
                    ,IN WEB_SOCKET_PROPERTY*  SocketProperties OPTIONAL
                    ,IN DWORD                 PropertyCount    OPTIONAL)
{
  // Getting the elementary objects
  RequestQueue* queue = g_handles.GetReQueueFromOpaqueHandle(RequestQueueHandle);
  Request*    request = g_handles.GetRequestFromOpaqueHandle(RequestId);

  // Check for correct RequestQUeue and current request
  if(queue == nullptr || request == nullptr)
  {
    return nullptr;
  }
  // Must have a buffer translation handle from the handshaking process
  if(SocketHandle == NULL)
  {
    return nullptr;
  }

  // Returns the WebSocket or nullptr
  SYSWebSocket* websocket = request->GetWebSocket();
  if(websocket == nullptr)
  {
    return nullptr;
  }

  // Remember our buffer translation handle
  websocket->SetTranslationHandle(SocketHandle);

  // Optional socket properties
  __try
  {
    if(SocketProperties && PropertyCount)
    {
      for(int index = 0;index < (int)PropertyCount;++index)
      {
        switch(SocketProperties[index].Type)
        {
          case WEB_SOCKET_RECEIVE_BUFFER_SIZE_PROPERTY_TYPE:        websocket->SetReceiveBufferSize  (*((ULONG*)SocketProperties[index].pvValue));
                                                                    break;
          case WEB_SOCKET_SEND_BUFFER_SIZE_PROPERTY_TYPE:           websocket->SetSendBufferSize     (*((ULONG*)SocketProperties[index].pvValue));
                                                                    break;
          case WEB_SOCKET_DISABLE_MASKING_PROPERTY_TYPE:            websocket->SetDisableClientMasking(*((BOOL*)SocketProperties[index].pvValue));
                                                                    break;
          case WEB_SOCKET_DISABLE_UTF8_VERIFICATION_PROPERTY_TYPE:  websocket->SetUTF8Verification    (*((BOOL*)SocketProperties[index].pvValue));
                                                                    break;
        }
      }
    }
  }
  __finally
  {
    if(AbnormalTermination())
    {
      // Something went wrong, restore the defaults
      websocket->SetSendBufferSize(DEF_BUFF_SIZE);
      websocket->SetReceiveBufferSize(DEF_BUFF_SIZE);
      websocket->SetDisableClientMasking(FALSE);
      websocket->SetUTF8Verification(TRUE);
    }
  }

  // Return the base class to the outside world
  // With the pure virtual interface only
  return (HTTPSYS_WebSocket*)(websocket);
}

