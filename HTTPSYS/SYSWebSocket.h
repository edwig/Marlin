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

#include "HTTPSYS_Websocket.h"
#include "Request.h"
#include <websocket.h>

// Documented: https://learn.microsoft.com/en-us/windows/win32/api/websocket/ne-websocket-web_socket_property_type
#define DEF_BUFF_SIZE  4096

class  SYSWebSocket : public HTTPSYS_WebSocket
{
public:
  // Implementation of the abstract class HTTPSYS_WebSocket
  virtual HRESULT WriteFragment(_In_    VOID *                   pData,
                                _Inout_ DWORD *                  pcbSent,
                                _In_    BOOL                     fAsync,
                                _In_    BOOL                     fUTF8Encoded,
                                _In_    BOOL                     fFinalFragment,
                                _In_    PFN_WEBSOCKET_COMPLETION pfnCompletion        = NULL,
                                _In_    VOID *                   pvCompletionContext  = NULL, 
                                _Out_   BOOL *                   pfCompletionExpected = NULL);

  virtual HRESULT ReadFragment( _Out_   VOID *                   pData,
                                _Inout_ DWORD *                  pcbData,
                                _In_    BOOL                     fAsync,
                                _Out_   BOOL *                   pfUTF8Encoded,
                                _Out_   BOOL *                   pfFinalFragment,
                                _Out_   BOOL *                   pfConnectionClose,
                                _In_    PFN_WEBSOCKET_COMPLETION pfnCompletion        = NULL,
                                _In_    VOID *                   pvCompletionContext  = NULL,
                                _Out_   BOOL *                   pfCompletionExpected = NULL);

  virtual HRESULT SendConnectionClose(_In_    BOOL                     fAsync,
                                      _In_    USHORT                   uStatusCode,
                                      _In_    LPCWSTR                  pszReason            = NULL,
                                      _In_    PFN_WEBSOCKET_COMPLETION pfnCompletion        = NULL,
                                      _In_    VOID *                   pvCompletionContext  = NULL,
                                      _Out_   BOOL *                   pfCompletionExpected = NULL);

  virtual HRESULT GetCloseStatus(_Out_   USHORT *              pStatusCode,
                                 _Out_   LPCWSTR *             ppszReason = NULL,
                                 _Out_   USHORT *              pcchReason = NULL);

  virtual VOID CloseTcpConnection(VOID);

  virtual VOID CancelOutstandingIO(VOID);

protected:
  void Close();
  void Reset();

  Request*      m_request { nullptr };     // HTTP Request that started last socket connection
  SocketStream* m_socket  { nullptr };     // Communication low-level socket
  CString       m_serverkey;               // Sec-WebSocket-Key of the Server-Handshake

  OVERLAPPED    m_wsrd;                    // Receiving callback
  OVERLAPPED    m_wswr;                    // Sending   callback

  ULONG         m_bufferSizeReceive { DEF_BUFF_SIZE };
  ULONG         m_bufferSizeSend    { DEF_BUFF_SIZE };
  BOOL          m_utf8Verification  { TRUE  };
  BOOL          m_disableMasking    { FALSE };

public:
  SYSWebSocket(Request* p_request);
 ~SYSWebSocket();

  // GETTERS
  Request*       GetRequest()    { return m_request;   }
  CString        GetServerKey()  { return m_serverkey; }
  SocketStream*  GetRealSocket() { return m_socket;    }

  // SETTERS
  void           SetReceiveBufferSize(ULONG p_size)       { m_bufferSizeReceive = p_size;   }
  void           SetSendBufferSize(ULONG p_size)          { m_bufferSizeSend    = p_size;   }
  void           SetUTF8Verification(BOOL p_utf8)         { m_utf8Verification  = p_utf8;   }
  void           SetDisableClientMasking(BOOL p_masking)  { m_disableMasking    = p_masking;}

  // FUNCTIONS

  // Reconnecting to an already previously opened socket
  void           ReConnectSocket(Request* p_request);
  // HTTP server application has created this handle for the header handshaking already
  void           SetTranslationHandle(WEB_SOCKET_HANDLE p_handle);
  // Public, but intended for internal use
  void           ReceiveFragment(LPOVERLAPPED p_overlapped);

private:
  // Read call lives here
  VOID*  m_read_Data                { nullptr };
  DWORD* m_read_Size                { nullptr };
  BOOL*  m_read_UTF8Encoded         { nullptr };
  BOOL*  m_read_FinalFragement      { nullptr };
  BOOL*  m_read_ConnectionClose     { nullptr };
  VOID*  m_read_CompletionContext   { nullptr };
  BOOL*  m_read_CompletionExpected  { nullptr };
  BYTE*  m_read_buffer              { nullptr };
  DWORD  m_read_AccomodatedSize     { 0L      };

  WCHAR  m_closeReason[WEB_SOCKET_MAX_CLOSE_REASON_LENGTH + 1] = { 0 };
  ULONG  m_closeReasonLength{ 0 };
  USHORT m_closeStatus      { 0 };

  PFN_WEBSOCKET_COMPLETION m_read_Completion { nullptr };

  // The buffer translation process
  WEB_SOCKET_HANDLE m_handle        { NULL    };
};
