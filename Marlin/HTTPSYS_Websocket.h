//////////////////////////////////////////////////////////////////////////
//
// USER-SPACE IMPLEMENTTION OF HTTP.SYS
//
// Created for various reasons:
// - Educational purposes
// - Non-disclosed functions (websockets)
// - Because it can be done :-)
//
// Not implemented features of HTTP.SYS are:
// - HTTP 2.0 Server push
//
// Copyright 2018 (c) ir. W.E. Huisman
// License: MIT
//
//////////////////////////////////////////////////////////////////////////

#pragma once

// WebSockets can only be operational in MS-Windows version 8.0 and higher
#if NTDDI_VERSION >= NTDDI_WIN8

typedef VOID
(WINAPI * PFN_WEBSOCKET_COMPLETION)(HRESULT     hrError,
                                    VOID *      pvCompletionContext,
                                    DWORD       cbIO,
                                    BOOL        fUTF8Encoded,
                                    BOOL        fFinalFragment,
                                    BOOL        fClose);


class HTTPSYS_WebSocket;

//__declspec(dllimport)
HTTPSYS_WebSocket* WINAPI
HttpReceiveWebSocket(IN HANDLE                RequestQueueHandle
                    ,IN HTTP_REQUEST_ID       RequestId
                    ,IN WEB_SOCKET_HANDLE     SocketHandle
                    ,IN WEB_SOCKET_PROPERTY*  SocketProperties = NULL OPTIONAL
                    ,IN DWORD                 PropertyCount    = NULL OPTIONAL);

//////////////////////////////////////////////////////////////////////////
// 
// PURE VIRTUAL CLASS INTERFACE DEFINITION
//
// Can be retrieved with "HttpReceiveWebSocket" in case we connect
// to a WebSocket on the other side
//
//////////////////////////////////////////////////////////////////////////

class HTTPSYS_WebSocket
{
public:

    virtual
    HRESULT
    WriteFragment(
        _In_    VOID *                   pData,
        _Inout_ DWORD *                  pcbSent,
        _In_    BOOL                     fAsync,
        _In_    BOOL                     fUTF8Encoded,
        _In_    BOOL                     fFinalFragment,
        _In_    PFN_WEBSOCKET_COMPLETION pfnCompletion        = NULL,
        _In_    VOID *                   pvCompletionContext  = NULL, 
        _Out_   BOOL *                   pfCompletionExpected = NULL
    ) = 0;

    virtual
    HRESULT
    ReadFragment(
        _Out_   VOID *                   pData,
        _Inout_ DWORD *                  pcbData,
        _In_    BOOL                     fAsync,
        _Out_   BOOL *                   pfUTF8Encoded,
        _Out_   BOOL *                   pfFinalFragment,
        _Out_   BOOL *                   pfConnectionClose,
        _In_    PFN_WEBSOCKET_COMPLETION pfnCompletion        = NULL,
        _In_    VOID *                   pvCompletionContext  = NULL,
        _Out_   BOOL *                   pfCompletionExpected = NULL
    ) = 0;

    virtual
    HRESULT
    SendConnectionClose(
        _In_    BOOL                     fAsync,
        _In_    USHORT                   uStatusCode,
        _In_    LPCWSTR                  pszReason            = NULL,
        _In_    PFN_WEBSOCKET_COMPLETION pfnCompletion        = NULL,
        _In_    VOID *                   pvCompletionContext  = NULL,
        _Out_   BOOL *                   pfCompletionExpected = NULL
    ) = 0;

    virtual
    HRESULT
    GetCloseStatus(
        _Out_   USHORT *              pStatusCode,
        _Out_   LPCWSTR *             ppszReason = NULL,
        _Out_   USHORT *              pcchReason = NULL
    ) = 0;

    virtual VOID CloseTcpConnection(VOID) = 0;

    virtual VOID CancelOutstandingIO(VOID) = 0;
};

#endif /* NTDDI_VERSION >= NTDDI_WIN8 */
