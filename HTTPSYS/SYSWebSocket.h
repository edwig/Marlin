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

#define TCPIP_KEEPALIVE_FRAMESIZE      6
#define WEBSOCKET_PINGPONG_SIZE       10
// Ping/Pong timing
#define WEBSOCKET_PINGPONG_TIMEOUT   120    // Default ping/pong timeout
#define WEBSOCKET_PINGPONG_MINTIME    15    // Minimal ping/pong timeout
#define WEBSOCKET_PINGPONG_MAXTIME   160    // Maximal ping/pong timeout (Do not extend beyond 180 seconds!)

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

  virtual BOOL SendPingPong(BOOL p_ping = TRUE);

  virtual VOID CloseTcpConnection(VOID);

  virtual VOID CancelOutstandingIO(VOID);

  virtual UINT64 GetSocketIdentifier(VOID);

protected:
  void Close();
  void Reset();

  UINT64        m_ident   { SYSWEBSOCKET_IDENT }; // Identification of the object
  Request*      m_request { nullptr };     // HTTP Request that started last socket connection
  SocketStream* m_socket  { nullptr };     // Communication low-level socket
  CString       m_serverkey;               // Sec-WebSocket-Key of the Server-Handshake

  OVERLAPPED    m_wsrd;                    // Receiving callback
  OVERLAPPED    m_wswr;                    // Sending   callback
  OVERLAPPED    m_wsping;                  // Ping/Pong callback

  ULONG         m_bufferSizeReceive { DEF_BUFF_SIZE };
  ULONG         m_bufferSizeSend    { DEF_BUFF_SIZE };
  BOOL          m_utf8Verification  { TRUE  };
  BOOL          m_disableMasking    { FALSE };

public:
  SYSWebSocket(Request* p_request);
 ~SYSWebSocket();

  // GETTERS
  Request*       GetRequest()               { return m_request;       }
  CString        GetServerKey()             { return m_serverkey;     }
  SocketStream*  GetRealSocket()            { return m_socket;        }

  // SETTERS
  void           SetReceiveBufferSize(ULONG p_size)       { m_bufferSizeReceive = p_size;   }
  void           SetSendBufferSize(ULONG p_size)          { m_bufferSizeSend    = p_size;   }
  void           SetUTF8Verification(BOOL p_utf8)         { m_utf8Verification  = p_utf8;   }
  void           SetDisableClientMasking(BOOL p_masking)  { m_disableMasking    = p_masking;}
  void           SetPingPongInterval(ULONG p_interval);

  // FUNCTIONS

  // Connect to the threadpool of the server
  void           AssociateThreadPool(HANDLE p_threadPoolIOCP);
  // Reconnecting to an already previously opened socket
  void           ReConnectSocket(Request* p_request);
  // HTTP server application has created this handle for the header handshaking already
  void           SetTranslationHandle(WEB_SOCKET_HANDLE p_handle);
  // Public, but intended for internal use
  void           ReceiveFragment(LPOVERLAPPED p_overlapped);
  void           WritingFragment(LPOVERLAPPED p_overlapped);
  // Before writing, encode the frame buffer
  DWORD          EncodingFragment(bool p_closing = false);

private:
  int            SetupForReceive();

  // Read call lives here
  VOID*  m_read_Data                { nullptr };
  DWORD* m_read_Size                { nullptr };
  VOID*  m_read_CompletionContext   { nullptr };
  BYTE*  m_read_buffer              { nullptr };
  DWORD  m_read_AccomodatedSize     { 0L      };
  // Write call lives here
  VOID*  m_send_Data                { nullptr };
  DWORD* m_send_Size                { nullptr };
  VOID*  m_send_CompletionContext   { nullptr };
  BYTE*  m_send_buffer              { nullptr };
  DWORD  m_send_AccomodatedSize     { 0L      };
  BOOL   m_send_utf8                { TRUE    };
  BOOL   m_send_final               { FALSE   };
  BOOL   m_send_close               { FALSE   };
  // Context for sending and receiving
  PVOID  m_actionSendContext        { nullptr };
  PVOID  m_actionReadContext        { nullptr };
  // Closing
  WCHAR  m_closeReason[WEB_SOCKET_MAX_CLOSE_REASON_LENGTH + 1] = { 0 };
  ULONG  m_closeReasonLength{ 0 };
  USHORT m_closeStatus      { 0 };
  // Ping/Pong buffer
  CHAR   m_pingpong[WEBSOCKET_PINGPONG_SIZE] = { 0 };
  UINT64 m_pingpongTimeout  { WEBSOCKET_PINGPONG_TIMEOUT };
  UINT64 m_lastAction       { 0 };
  // Completion functions
  PFN_WEBSOCKET_COMPLETION m_read_Completion { nullptr };
  PFN_WEBSOCKET_COMPLETION m_send_Completion { nullptr };
  // The buffer translation process
  WEB_SOCKET_HANDLE       m_handle           { NULL };
  WEB_SOCKET_BUFFER       m_recvBuffers[2]   { 0 };
  WEB_SOCKET_BUFFER       m_sendBuffers[2]   { 0 };
  WEB_SOCKET_BUFFER_TYPE  m_recvBufferType   { WEB_SOCKET_UTF8_MESSAGE_BUFFER_TYPE };
  WEB_SOCKET_ACTION       m_recvAction       { WEB_SOCKET_NO_ACTION };
  // Lock on the WEB_SOCKET_HANDLE
  CRITICAL_SECTION        m_lock;
};
