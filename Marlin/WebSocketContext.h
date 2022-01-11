/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: WebSocketContext.h
//
// Marlin Server: Internet server/client
// 
// Copyright (c) 2014-2022 ir. W.E. Huisman
// All rights reserved
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
#pragma once
#include <websocket.h>

// Documented by Microsoft on:
// https://docs.microsoft.com/en-us/windows/win32/api/websocket/ne-websocket-web_socket_property_type
#define WEBSOCKET_BUFFER_OVERHEAD  256
#define WEBSOCKET_BUFFER_MINIMUM   256
#define WEBSOCKET_KEEPALIVE_MIN    15000

typedef VOID
(WINAPI* PFN_WEBSOCKET_COMPLETION)(HRESULT     hrError,
                                   VOID*       pvCompletionContext,
                                   DWORD       cbIO,
                                   BOOL        fUTF8Encoded,
                                   BOOL        fFinalFragment,
                                   BOOL        fClose);

class WebSocketContext 
{
public:
  WebSocketContext();
 ~WebSocketContext();

  virtual bool    PerformHandshake(_In_  WEB_SOCKET_HTTP_HEADER*  p_clientHeaders,
                                   _In_  ULONG                    p_clientHeadersCount,
                                   _Out_ WEB_SOCKET_HTTP_HEADER** p_serverHeaders,
                                   _Out_ ULONG*                   p_serverHeadersCount,
                                   _In_  PCSTR                    p_subProtocol);
  virtual bool    CompleteHandshake();

//   virtual HRESULT WriteFragment(_In_    VOID*   pData,
//                                 _Inout_ DWORD*  pcbSent,
//                                 _In_    BOOL    fAsync,
//                                 _In_    BOOL    fUTF8Encoded,
//                                 _In_    BOOL    fFinalFragment,
//                                 _In_    PFN_WEBSOCKET_COMPLETION pfnCompletion = NULL,
//                                 _In_    VOID*   pvCompletionContext  = NULL,
//                                 _Out_   BOOL*   pfCompletionExpected = NULL);
// 
  virtual HRESULT ReadFragment( _Out_   VOID*  pData,
                                _Inout_ DWORD* pcbData,
                                _In_    BOOL   fAsync,
                                _Out_   BOOL*  pfUTF8Encoded,
                                _Out_   BOOL*  pfFinalFragment,
                                _Out_   BOOL*  pfConnectionClose,
                                _In_    PFN_WEBSOCKET_COMPLETION pfnCompletion = NULL,
                                _In_    VOID*  pvCompletionContext = NULL,
                                _Out_   BOOL*  pfCompletionExpected = NULL);

//   virtual HRESULT SendConnectionClose(_In_  BOOL     fAsync,
//                                       _In_  USHORT   uStatusCode,
//                                       _In_  LPCWSTR  pszReason = NULL,
//                                       _In_  PFN_WEBSOCKET_COMPLETION pfnCompletion = NULL,
//                                       _In_  VOID*    pvCompletionContext = NULL,
//                                       _Out_ BOOL*    pfCompletionExpected = NULL);
// 
  virtual HRESULT GetCloseStatus( _Out_ USHORT*  pStatusCode,
                                  _Out_ LPCWSTR* ppszReason = NULL,
                                  _Out_ USHORT*  pcchReason = NULL);
// 
//   virtual void CloseTcpConnection(void);
// 
//   virtual void CancelOutstandingIO(void);

  // SETTERS

  void SetRecieveBufferSize(ULONG p_size);
  void SetSendBufferSize(ULONG p_size);
  void SetDisableClientMasking(BOOL p_disable);
  void SetDisableUTF8Checking(BOOL p_disable);
  void SetKeepAliveInterval(ULONG p_interval);
  
  // GETTERS

  ULONG GetReceiveBufferSize()    { return m_ws_recv_buffersize;      }
  ULONG GetSendBufferSize()       { return m_ws_send_buffersize;      }
  BOOL  GetDisableClientMasking() { return m_ws_disable_masking;      }
  BOOL  GetDisableUTF8Checking()  { return m_ws_disable_utf8_verify;  }
  ULONG GetKeepAliveInterval()    { return m_ws_keepalive_interval;   }

private:
  void Reset();
  bool CreateServerHandle();

  WEB_SOCKET_HANDLE       m_handle { NULL };
  CString                 m_subProtocol;

  // Buffer parameters
  ULONG m_ws_recv_buffersize      { 4096  };
  ULONG m_ws_send_buffersize      { 4096  };
  BOOL  m_ws_disable_masking      { FALSE };
  BOOL  m_ws_disable_utf8_verify  { TRUE  };
  ULONG m_ws_keepalive_interval   { 30000 };
  BYTE* m_ws_buffer               { nullptr };
  // Receiving data
  PVOID m_actionContext           { nullptr };

};
