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

#include "stdafx.h"
#include "http_private.h"
#include "SYSWebSocket.h"
#include "SocketStream.h"
#include "Logging.h"
#include <ConvertWideString.h>
#include <assert.h>

SYSWebSocket::SYSWebSocket(Request* p_request)
{
  ReConnectSocket(p_request);
}

SYSWebSocket::~SYSWebSocket()
{
  Close();
  Reset();
}

void SYSWebSocket::ReConnectSocket(Request* p_request)
{
  m_request = p_request;
  m_socket  = p_request->GetSocket();
}

void
SYSWebSocket::Close()
{
  if(m_socket)
  {
    if(m_socket->Close() == false)
    {
      // Log the error
      int error = m_socket->GetLastError();
      LogError(_T("Error shutdown websocket: %s Error: %d"),m_serverkey,error);
    }
    delete m_socket;
    m_socket = nullptr;
  }
}

void
SYSWebSocket::Reset()
{
  if(m_read_buffer)
  {
    delete [] m_read_buffer;
    m_read_buffer = nullptr;
  }
}

// HTTP server application has created this handle for the header handshaking already
void
SYSWebSocket::SetTranslationHandle(WEB_SOCKET_HANDLE p_handle)
{
  m_handle = p_handle;
}

//////////////////////////////////////////////////////////////////////////
//
// VIRTUAL INTERFACE
//
//////////////////////////////////////////////////////////////////////////


HRESULT SYSWebSocket::WriteFragment(_In_    VOID*  pData,
                                    _Inout_ DWORD* pcbSent,
                                    _In_    BOOL   fAsync,
                                    _In_    BOOL   fUTF8Encoded,
                                    _In_    BOOL   fFinalFragment,
                                    _In_    PFN_WEBSOCKET_COMPLETION pfnCompletion /*= NULL*/,
                                    _In_    VOID*  pvCompletionContext /*= NULL*/,
                                    _Out_   BOOL*  pfCompletionExpected /*= NULL*/)
{
  return NULL;
}

// Plain/Secure socket did wake up and send us some results
//
void 
WebSocketReceivePartialOverlapped(LPOVERLAPPED p_overlapped)
{
  __try
  {
    SYSWebSocket* socket = reinterpret_cast<SYSWebSocket*>(p_overlapped->Pointer);
    if(socket)
    {
      socket->ReceiveFragment(p_overlapped);
    }
  }
  __finally
  {
    // Do nothing
  }
}

HRESULT SYSWebSocket::ReadFragment(_Out_   VOID*  pData,
                                   _Inout_ DWORD* pcbData,
                                   _In_    BOOL   fAsync,
                                   _Out_   BOOL*  pfUTF8Encoded,
                                   _Out_   BOOL*  pfFinalFragment,
                                   _Out_   BOOL*  pfConnectionClose,
                                   _In_    PFN_WEBSOCKET_COMPLETION pfnCompletion /*= NULL*/,
                                   _In_    VOID*  pvCompletionContext  /*= NULL*/,
                                   _Out_   BOOL*  pfCompletionExpected /*= NULL*/)
{
  // Number of bytes read
  int received = 0;
  ZeroMemory(&m_wsrd,sizeof(OVERLAPPED));

  // FASE 1: COMPLETENESS CHECKS
  
  // Check that we have a receiving buffer
  if(pData == nullptr || pcbData == nullptr)
  {
    return ERROR_INVALID_PARAMETER;
  }
  // Check that we have the mandatory boolean pointers
  if(pfUTF8Encoded == nullptr || pfFinalFragment == nullptr || pfConnectionClose == nullptr)
  {
    return ERROR_INVALID_PARAMETER;
  }
  // Check that we have the completion for the asynchronous operation
  if(fAsync && pfnCompletion == nullptr)
  {
    return ERROR_INVALID_PARAMETER;
  }
  // Check that we do not read too much
  if(*pcbData > (DWORD)m_bufferSizeReceive)
  {
    return ERROR_INVALID_PARAMETER;
  }
  // We cannot get more than the agreed upon buffer size
  if(*pcbData > m_bufferSizeReceive)
  {
    return ERROR_INVALID_PARAMETER;
  }

  // FASE 2: REMEMBER OUR CONTEXT
  m_read_Data               = pData;
  m_read_Size               = pcbData;
  m_read_UTF8Encoded        = pfUTF8Encoded;
  m_read_FinalFragement     = pfFinalFragment;
  m_read_ConnectionClose    = pfConnectionClose;
  m_read_Completion         = pfnCompletion;
  m_read_CompletionContext  = pvCompletionContext;
  m_read_CompletionExpected = pfCompletionExpected;
  // Read at the moment of incoming call
  m_read_AccomodatedSize    = *pcbData;

  // FASE 3: START THE READ

  // Make sure we have a read buffer
  if(!m_read_buffer)
  {
    m_read_buffer = new BYTE[m_bufferSizeReceive];
  }

  if(fAsync)
  {
    // Set up for overlapped I/O
    m_wsrd.Pointer = this;
    m_wsrd.hEvent  = (HANDLE) WebSocketReceivePartialOverlapped;
    received = m_socket->RecvPartialOverlapped(m_read_buffer,m_read_AccomodatedSize,&m_wsrd);
    // Should return IO_PENDING
    if(!(received == SOCKET_ERROR && (m_socket->GetLastError() == WSA_IO_PENDING)))
    {
      return S_FALSE;
    }
  }
  else
  {
    // SYNC Receive
    received = m_socket->RecvPartial(m_read_buffer, m_read_AccomodatedSize);
    if(received == SOCKET_ERROR)
    {
      return S_FALSE;
    }
  }
  // We expected to have read 'something' or we will be receiving it in the future
  if(pfCompletionExpected)
  {
    *pfCompletionExpected = (received > 0) ? FALSE : TRUE;
  }
  // Synchronous receive, even from overlapped I/O
  if(received > 0)
  {
    // Synchronous receive: past it on directly
    OVERLAPPED over;
    over.Internal     = m_socket->GetLastError();
    over.InternalHigh = received;
    ReceiveFragment(&over);
  }
  return S_OK;
}

// We now are waked by the I/O completion and have a filled m_read_buffer
void 
SYSWebSocket::ReceiveFragment(LPOVERLAPPED p_overlapped)
{
  // FASE 1: TRANSLATE INCOMING BUFFER
  ULONG bytesTransferred = (int)p_overlapped->InternalHigh;
  ULONG extraRead = 0;

  HRESULT           hr = S_OK;
  WEB_SOCKET_BUFFER buffers[2] = { 0 };
  ULONG             bufferCount;
  WEB_SOCKET_BUFFER_TYPE bufferType;
  WEB_SOCKET_ACTION action;
  PVOID             actionContext;

  hr = WebSocketReceive(m_handle,NULL,NULL);
  if(FAILED(hr))
  {
    goto quit;
  }

  do
  {
    // Initialize variables that change with every loop revolution.
    // SHOULD NEVER BY SMALLER THAN 2!!
    bufferCount = ARRAYSIZE(buffers);

    // Get an action to process.
    hr = WebSocketGetAction(m_handle,
                            WEB_SOCKET_RECEIVE_ACTION_QUEUE,
                            buffers,
                            &bufferCount,
                            &action,
                            &bufferType,
                            NULL,
                            &actionContext);
    if(FAILED(hr))
    {
      // If we cannot get an action, abort the handle but continue processing until all operations are completed.
      goto quit;
    }

    switch(action)
    {
      case WEB_SOCKET_NO_ACTION:
           // No action to perform - just exit the loop.
           break;

      case WEB_SOCKET_RECEIVE_FROM_NETWORK_ACTION:
           assert(bufferCount >= 1);
           for (ULONG i = 0; i < bufferCount; i++)
           {
             if(i > 0 || extraRead)
             {
               // Busy reading, read some extra
               bytesTransferred = m_socket->RecvPartial(m_read_buffer,m_read_AccomodatedSize);
               if(bytesTransferred == SOCKET_ERROR)
               {
                 hr = E_FAIL;
                 goto quit;
               }
             }
             // Initial read from the overlapped I/O call
             memcpy_s(buffers[i].Data.pbBuffer,buffers[i].Data.ulBufferLength,m_read_buffer,bytesTransferred);

             // Exit the loop if there were not enough data to fill this buffer.
             if(buffers[i].Data.ulBufferLength > bytesTransferred)
             {
               break;
             }
           }
           break;
      case WEB_SOCKET_INDICATE_RECEIVE_COMPLETE_ACTION:
           // Copy out the data to the application
           memcpy_s(m_read_Data,*m_read_Size,buffers[0].Data.pbBuffer,buffers[0].Data.ulBufferLength);

           switch(bufferType)
           {
              case WEB_SOCKET_UTF8_MESSAGE_BUFFER_TYPE:     *(m_read_UTF8Encoded)    = TRUE;
                                                            *(m_read_FinalFragement) = TRUE;
                                                            break;
              case WEB_SOCKET_UTF8_FRAGMENT_BUFFER_TYPE:    *(m_read_UTF8Encoded)    = TRUE;
                                                            *(m_read_FinalFragement) = FALSE;
                                                            break;
              case WEB_SOCKET_BINARY_MESSAGE_BUFFER_TYPE:   *(m_read_UTF8Encoded)    = FALSE;
                                                            *(m_read_FinalFragement) = TRUE;
                                                            break;
              case WEB_SOCKET_BINARY_FRAGMENT_BUFFER_TYPE:  *(m_read_UTF8Encoded)    = FALSE;
                                                            *(m_read_FinalFragement) = FALSE;
                                                            break;
              case WEB_SOCKET_CLOSE_BUFFER_TYPE:            memcpy_s(m_closeReason,WEB_SOCKET_MAX_CLOSE_REASON_LENGTH,buffers[0].CloseStatus.pbReason,buffers[0].CloseStatus.ulReasonLength);
                                                            m_closeReason[buffers[0].CloseStatus.ulReasonLength] = 0;
                                                            m_closeStatus = buffers[0].CloseStatus.usStatus;
                                                            m_closeReasonLength = buffers[0].CloseStatus.ulReasonLength;
                                                            *(m_read_ConnectionClose) = TRUE;
                                                            break;
              case WEB_SOCKET_PING_PONG_BUFFER_TYPE:        // Send a PONG back [ TO BE IMPLEMENTED ]
                                                            break;
              case WEB_SOCKET_UNSOLICITED_PONG_BUFFER_TYPE: // That's quite OK. Do not take any action
                                                            break;
           }
           extraRead += bytesTransferred;
           break;

    default:
           // This should never happen.
           assert(!"Invalid switch");
           hr = E_FAIL;
           goto quit;
    }
    if(FAILED(hr))
    {
      // If we failed at some point processing actions, abort the handle but continue processing
      // until all operations are completed.
      WebSocketAbortHandle(m_handle);
    }

    // Complete the action. If application performs asynchronous operation, the action has to be
    // completed after the async operation has finished. The 'actionContext' then has to be preserved
    // so the operation can complete properly.
    WebSocketCompleteAction(m_handle,actionContext,bytesTransferred);
  } 
  while(action != WEB_SOCKET_NO_ACTION);

quit:

  // FASE 2: STORE CLOSING REASON AS AN UTF-16 STRING
  if(m_closeReasonLength)
  {
#ifdef _UNICODE
    CString reason;
    bool foundBom(false);
    TryConvertNarrowString((const BYTE*)m_closeReason,m_closeReasonLength,_T(""),reason,foundBom);
    wcscpy_s((wchar_t*)m_closeReason,WEB_SOCKET_MAX_CLOSE_REASON_LENGTH,(wchar_t*)reason.GetString());
#else
    CString reason(m_closeReason);
    BYTE* buffer = nullptr;
    int length   = 0;
    TryCreateWideString(reason,_T(""),false,&buffer,length);
    wcscpy_s((wchar_t*)m_closeReason,WEB_SOCKET_MAX_CLOSE_REASON_LENGTH,(wchar_t*)buffer);
    m_closeReasonLength = length / 2;
    delete [] buffer;
#endif
  }

  // FASE 3: PASS ON TO THE APPLICATION

  // Amount of bytes effectively read
  *m_read_Size = extraRead;

  // In case of an overlapped I/O action, call back our application context
  if(m_wsrd.Pointer && m_read_Completion)
  {
    (*m_read_Completion)(hr
                        ,m_read_CompletionContext
                        ,extraRead // bytesTransferred
                        ,*(m_read_UTF8Encoded)
                        ,*(m_read_FinalFragement)
                        ,*(m_read_ConnectionClose));
  }
}


HRESULT SYSWebSocket::SendConnectionClose(_In_    BOOL                     fAsync,
                                          _In_    USHORT                   uStatusCode,
                                          _In_    LPCWSTR                  pszReason            /*= NULL*/,
                                          _In_    PFN_WEBSOCKET_COMPLETION pfnCompletion        /*= NULL*/,
                                          _In_    VOID*                    pvCompletionContext  /*= NULL*/,
                                          _Out_   BOOL*                    pfCompletionExpected /*= NULL*/)
{
  return NULL;
}

// Getting the closing status code and reason string
// Was already stored in UTF-16 format for the MS-Windows system
HRESULT 
SYSWebSocket::GetCloseStatus(_Out_   USHORT*  pStatusCode,
                             _Out_   LPCWSTR* ppszReason  /*= NULL*/,
                             _Out_   USHORT*  pcchReason  /*= NULL*/)
{
  if(pStatusCode == nullptr)
  {
    return E_FAIL;
  }
  *pStatusCode = m_closeStatus;

  if(ppszReason && pcchReason)
  {
    *ppszReason = m_closeReason;
    *pcchReason = (USHORT) m_closeReasonLength;
  }
  return S_OK;
}

VOID
SYSWebSocket::CloseTcpConnection(VOID)
{
  if(m_socket)
  {
    m_socket->Close();
  }
}

VOID
SYSWebSocket::CancelOutstandingIO(VOID)
{
  if(m_socket)
  {
    m_socket->Disconnect();
  }
}

