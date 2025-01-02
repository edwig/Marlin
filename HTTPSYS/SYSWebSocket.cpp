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
// 2018 - 2024 (c) ir. W.E. Huisman
// License: MIT
//
//////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "http_private.h"
#include "SYSWebSocket.h"
#include "SocketStream.h"
#include "Logging.h"
#include <AutoCritical.h>
#include <ConvertWideString.h>
#include <time.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

constexpr auto WS_MAX_HEADER = 14;  // Maximum header size in octets

SYSWebSocket::SYSWebSocket(Request* p_request)
{
  // Last moment we were active
  m_lastAction = _time64(nullptr);

  ReConnectSocket(p_request);
  InitializeCriticalSection(&m_lock);
}

SYSWebSocket::~SYSWebSocket()
{
  Close();
  Reset();

  DeleteCriticalSection(&m_lock);
}

void SYSWebSocket::ReConnectSocket(Request* p_request)
{
  m_request = p_request;
  m_socket  = p_request->GetSocket();
}

void
SYSWebSocket::Close()
{
  AutoCritSec lock(&m_lock);

  if(m_socket)
  {
    if(m_socket->Close() == false)
    {
      // Log the error
      int error = m_socket->GetLastError();
      LogError(_T("Error shutdown websocket: %s Error: %d"),m_serverkey,error);
    }
    m_socket->DropReference();
    m_socket = nullptr;
  }

}

void
SYSWebSocket::Reset()
{
  // Cleaning th reading buffer
  if(m_read_buffer)
  {
    delete [] m_read_buffer;
    m_read_buffer = nullptr;
  }
  // clean the sending buffer
  if(m_send_buffer)
  {
    delete[] m_send_buffer;
    m_send_buffer = nullptr;
  }
}

// HTTP server application has created this handle for the header handshaking already
void
SYSWebSocket::SetTranslationHandle(WEB_SOCKET_HANDLE p_handle)
{
  m_handle = p_handle;
}

void
SYSWebSocket::AssociateThreadPool(HANDLE p_threadPoolIOCP)
{
  m_socket->AssociateThreadPool(p_threadPoolIOCP);
}

void
SYSWebSocket::SetPingPongInterval(ULONG p_interval)
{
  if(p_interval > WEBSOCKET_PINGPONG_MINTIME && p_interval < WEBSOCKET_PINGPONG_MAXTIME)
  {
    m_pingpongTimeout = p_interval;
  }
}

//////////////////////////////////////////////////////////////////////////
//
// VIRTUAL INTERFACE
//
//////////////////////////////////////////////////////////////////////////


// Plain/Secure socket did wake up and send us some results
//
void
WebSocketWritingPartialOverlapped(LPOVERLAPPED p_overlapped)
{
  __try
  {
    SYSWebSocket* socket = reinterpret_cast<SYSWebSocket*>(p_overlapped->Pointer);
    if(socket)
    {
      // Returning from a-sync socket writing
      // Pass the word on to our application
      socket->WritingFragment(p_overlapped);
    }
  }
  __finally
  {
    // Do nothing
  }
}

void
SYSWebSocket::WritingFragment(LPOVERLAPPED p_overlapped)
{
  DWORD error            = (DWORD) p_overlapped->Internal;
  DWORD bytesTransferred = (DWORD) p_overlapped->InternalHigh;

  // Last moment we were active
  m_lastAction = _time64(nullptr);

  // In case of an overlapped I/O action, call back our application context
  if(m_wswr.Pointer && m_send_Completion)
  {
    (*m_send_Completion)(error
                        ,m_read_CompletionContext
                        ,bytesTransferred
                        ,m_send_utf8
                        ,m_send_final
                        ,m_send_close);
  }

}

HRESULT SYSWebSocket::WriteFragment(_In_    VOID*  pData,
                                    _Inout_ DWORD* pcbSent,
                                    _In_    BOOL   fAsync,
                                    _In_    BOOL   fUTF8Encoded,
                                    _In_    BOOL   fFinalFragment,
                                    _In_    PFN_WEBSOCKET_COMPLETION pfnCompletion /*= NULL*/,
                                    _In_    VOID*  pvCompletionContext             /*= NULL*/,
                                    _Out_   BOOL*  pfCompletionExpected            /*= NULL*/)
{
    // Number of bytes read
  int written = 0;
  ZeroMemory(&m_wswr,sizeof(OVERLAPPED));

  // FASE 1: COMPLETENESS CHECKS

  // Check that we have a receiving buffer
  if(pData == nullptr || pcbSent == nullptr)
  {
    return ERROR_INVALID_PARAMETER;
  }
  // Check that we have the completion for the asynchronous operation
  if(fAsync && pfnCompletion == nullptr)
  {
    return ERROR_INVALID_PARAMETER;
  }
  // Check that we do not send too much
  if(*pcbSent > (DWORD)m_bufferSizeSend)
  {
    return ERROR_INVALID_PARAMETER;
  }

  { // Locking scope
    AutoCritSec lock(&m_lock);

    // Preset expectations for results
    if(pfCompletionExpected)
    {
      *pfCompletionExpected = FALSE;
    }

    // FASE 2: REMEMBER OUR CONTEXT
    m_send_Data = pData;
    m_send_Size = pcbSent;
    m_send_Completion = pfnCompletion;
    m_send_CompletionContext = pvCompletionContext;
    m_send_utf8 = fUTF8Encoded;
    m_send_final = fFinalFragment;
    m_send_close = FALSE;
    // Read at the moment of incoming call
    m_send_AccomodatedSize = *pcbSent;

    // FASE 3: START THE WRITE OPERATION

    if(fAsync)
    {
      // Make sure we have a send buffer
      if(!m_send_buffer)
      {
        m_send_buffer = new BYTE[m_bufferSizeSend + WS_MAX_HEADER];
      }
      // Set up the WebSocket translation
      DWORD extra = EncodingFragment();
      if(extra == 0)
      {
        return S_FALSE;
      }
      strncpy_s((char*)&m_send_buffer[extra],(int)m_bufferSizeSend,(char*)pData,*m_send_Size);
      m_send_buffer[extra + *m_send_Size] = 0;

      // Set up for overlapped I/O
      m_wswr.Pointer = this;
      m_wswr.hEvent = (HANDLE)WebSocketWritingPartialOverlapped;
      written = m_socket->SendPartialOverlapped(m_send_buffer,extra + *m_send_Size,&m_wswr);
      // Should return IO_PENDING
      if(written != NO_ERROR)
      {
        return S_FALSE;
      }
    }
    else
    {
      // SYNC Receive
      written = m_socket->SendPartial(m_send_Data,*m_send_Size);
      if(written == SOCKET_ERROR)
      {
        return S_FALSE;
      }
    }
  } // End of locking scope

  // PHASE 4: After the sending

  // We expected to have read 'something' or we will be receiving it in the future
  if(pfCompletionExpected)
  {
    *pfCompletionExpected = (written > 0) ? FALSE : TRUE;
  }
  // Synchronous receive, even from overlapped I/O
  if(written > 0 && pvCompletionContext)
  {
    // Synchronous receive: past it on directly
    OVERLAPPED over;
    memset(&over,0,sizeof(OVERLAPPED));
    over.Internal     = m_socket->GetLastError();
    over.InternalHigh = written;
    WritingFragment(&over);
  }
  // Last moment we were active
  m_lastAction = _time64(nullptr);

  return S_OK;
}

// We now are waked by the I/O completion and have a filled m_read_buffer
DWORD
SYSWebSocket::EncodingFragment(bool p_closing /*= false*/)
{
  HRESULT hr = S_OK;
  ULONG   bufferCount      = 2;
  DWORD   bytesTransferred = 0;

  WEB_SOCKET_BUFFER       sendBuffer;
  WEB_SOCKET_BUFFER       sendBuffers[2] { 0 };
  WEB_SOCKET_BUFFER_TYPE  sendBufferType { WEB_SOCKET_UTF8_MESSAGE_BUFFER_TYPE };
  WEB_SOCKET_ACTION       sendAction     { WEB_SOCKET_NO_ACTION };

  // Reset the action context
  m_actionSendContext = nullptr;

  // Determine the type of buffer we are sending
  if(p_closing)
  {
    sendBufferType = WEB_SOCKET_CLOSE_BUFFER_TYPE;
  }
  else if(m_send_utf8 && m_send_final)
  {
    sendBufferType = WEB_SOCKET_UTF8_MESSAGE_BUFFER_TYPE;
  }
  else if(m_send_utf8 && !m_send_final)
  {
    sendBufferType = WEB_SOCKET_UTF8_FRAGMENT_BUFFER_TYPE;
  }
  else if(!m_send_utf8 && m_send_final)
  {
    sendBufferType = WEB_SOCKET_BINARY_MESSAGE_BUFFER_TYPE;
  }
  else if(!m_send_utf8 && !m_send_final)
  {
    sendBufferType = WEB_SOCKET_BINARY_FRAGMENT_BUFFER_TYPE;
  }

  // Fill the data buffer
  if(p_closing)
  {
    sendBuffer.CloseStatus.pbReason      = (PBYTE)m_send_buffer;
    sendBuffer.CloseStatus.ulReasonLength = *m_send_Size;
    sendBuffer.CloseStatus.usStatus       = m_closeStatus;
  }
  else
  {
    sendBuffer.Data.pbBuffer       = m_send_buffer;
    sendBuffer.Data.ulBufferLength = *m_send_Size;
  }

  AutoCritSec lock(&m_lock);

  // Prepare for a SEND action
  hr = WebSocketSend(m_handle,sendBufferType,&sendBuffer,NULL);
  if(FAILED(hr) || bufferCount == 0)
  {
    return hr;
  }

  // FASE 1: TRANSLATE OUTGOING BUFFER
  do
  {
    // Initialize variables that change with every loop revolution.
    bufferCount = ARRAYSIZE(sendBuffers);

    // Get an action to process.
    hr = WebSocketGetAction(m_handle,
                            WEB_SOCKET_SEND_ACTION_QUEUE,
                            m_sendBuffers,
                            &bufferCount,
                            &sendAction,
                            &sendBufferType,
                            nullptr,
                            &m_actionSendContext);
    if(FAILED(hr))
    {
      // If we cannot get an action, abort the handle but continue processing until all operations are completed.
      return hr;
    }
    switch(sendAction)
    {
      case  WEB_SOCKET_NO_ACTION:
            // No action to perform - just exit the loop.
            break;
      case  WEB_SOCKET_SEND_TO_NETWORK_ACTION:
            memcpy_s(m_send_buffer,*m_read_Size + WS_MAX_HEADER,m_sendBuffers[0].Data.pbBuffer,m_sendBuffers[0].Data.ulBufferLength);
            bytesTransferred = m_sendBuffers[0].Data.ulBufferLength;
            break;
      case  WEB_SOCKET_INDICATE_SEND_COMPLETE_ACTION:
            break;
    default:// This should never happen.
            hr = E_FAIL;
            return hr;
    }
    // Complete the action. If application performs asynchronous operation, the action has to be
    // completed after the async operation has finished. The 'actionContext' then has to be preserved
    // so the operation can complete properly.
    WebSocketCompleteAction(m_handle,m_actionSendContext,bytesTransferred);
  } 
  while(sendAction != WEB_SOCKET_NO_ACTION);

  return bytesTransferred;
}

BOOL
SYSWebSocket::SendPingPong(BOOL p_ping /*= TRUE*/)
{
  // SEE IF WE MUST SPRING TO ACTION
  
  // Check that we are beyond the timeout
  if((m_lastAction + m_pingpongTimeout) > (UINT64)_time64(nullptr))
  {
    // Nothing to be done, keep the socket alive
    return true;
  }
  // Check if we are closing
  // in which case we must not send a ping/pong
  if(m_closeStatus || m_closeReason[0])
  {
    // Nothing to be done, socket is going away anyhow
    return false;
  }

  // Prepare for the ping/pong buffer
  HRESULT hr = S_OK;
  ULONG   bufferCount = 2;
  DWORD   bytesTransferred = 0;
  WEB_SOCKET_BUFFER       sendBuffers[2] { 0 };
  WEB_SOCKET_BUFFER_TYPE  sendBufferType { WEB_SOCKET_PING_PONG_BUFFER_TYPE };
  WEB_SOCKET_ACTION       sendAction     { WEB_SOCKET_NO_ACTION };
  // Reset the action context
  m_actionSendContext = nullptr;

  // Lock on the handle
  AutoCritSec lock(&m_lock);

  // Prepare for a SEND action
  hr = WebSocketSend(m_handle,WEB_SOCKET_PING_PONG_BUFFER_TYPE,nullptr,nullptr);
  if(FAILED(hr))
  {
    // Handle is dead. Stop the socket
    return false;
  }

  // FASE 1: CREATE PING/PONG BUFFER
  do
  {
    // Initialize variables that change with every loop revolution.
    bufferCount = ARRAYSIZE(sendBuffers);
    // Get an action to process.
    hr = WebSocketGetAction(m_handle,
                            WEB_SOCKET_SEND_ACTION_QUEUE,
                            m_sendBuffers,
                            &bufferCount,
                            &sendAction,
                            &sendBufferType,
                            nullptr,
                            &m_actionSendContext);
    if(FAILED(hr) || bufferCount == 0)
    {
      // If we cannot get an action, abort the socket
      return false;
    }
    switch(sendAction)
    {
      case WEB_SOCKET_NO_ACTION:                      // No action to perform - just exit the loop.
                                                      break;
      case WEB_SOCKET_SEND_TO_NETWORK_ACTION:         [[fallthrough]];
      case WEB_SOCKET_INDICATE_SEND_COMPLETE_ACTION:  memcpy_s(m_pingpong,WEBSOCKET_PINGPONG_SIZE,m_sendBuffers[0].Data.pbBuffer,m_sendBuffers[0].Data.ulBufferLength);
                                                      bytesTransferred = m_sendBuffers[0].Data.ulBufferLength;
                                                      break;
      default:                                        // This should never happen.
                                                      return false;
    }
    // Complete the action. If application performs asynchronous operation, the action has to be
    // completed after the async operation has finished. The 'actionContext' then has to be preserved
    // so the operation can complete properly.
    WebSocketCompleteAction(m_handle,m_actionSendContext,bytesTransferred);
  }
  while(sendBufferType != WEB_SOCKET_NO_ACTION);

  // Change the 'PING' to a 'PONG'
  if(p_ping == FALSE && m_pingpong[3] == 'p')
  {
    m_pingpong[4] = 'o';
  }

  // Set up for overlapped I/O
  memset(&m_wsping,0,sizeof(OVERLAPPED));
  // m_wsping.Internal     = 0;
  // m_wsping.InternalHigh = 0;
  // m_wsping.Pointer = nullptr; // DO **NOT** USE THIS POINTER, so the completion routine will not be called
  m_wsping.hEvent  = (HANDLE)WebSocketWritingPartialOverlapped;
  int written = m_socket->SendPartialOverlapped(m_pingpong,bytesTransferred,&m_wsping);

  // Should return IO_PENDING
  if(written != NO_ERROR)
  {
    // Socket most probably dead. Close the websocket
    LogError("Error sending ping/pong: %s",m_serverkey.GetString());
    return FALSE;
  }

  // Last moment we were active
  m_lastAction = _time64(nullptr);

  return TRUE;
}

//////////////////////////////////////////////////////////////////////////
//
// RECEIVING FRAGMENTS
//
//////////////////////////////////////////////////////////////////////////

// Plain/Secure socket did wake up and send us some results
//
void WINAPI
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
  if(*pcbData < (DWORD)m_bufferSizeReceive)
  {
    return ERROR_INVALID_PARAMETER;
  }
  // We cannot get more than the agreed upon buffer size
  if(*pcbData < m_bufferSizeReceive)
  {
    return ERROR_INVALID_PARAMETER;
  }

  { // Locking scope
    AutoCritSec lock(&m_lock);

    // Preset expectations for results
    if(pfCompletionExpected)
    {
      *pfCompletionExpected = FALSE;
    }

    // FASE 2: REMEMBER OUR CONTEXT
    m_read_Data = pData;
    m_read_Size = pcbData;
    m_read_Completion = pfnCompletion;
    m_read_CompletionContext = pvCompletionContext;
    // Read at the moment of incoming call
    m_read_AccomodatedSize = *pcbData;

    // FASE 3: START THE READ

    // Make sure we have a read buffer
    if(!m_read_buffer)
    {
      m_read_buffer = new BYTE[m_bufferSizeReceive];
    }

    if(fAsync)
    {
      // Set up the WebSocket translation
      DWORD receiveSize = SetupForReceive();
      if(receiveSize == SOCKET_ERROR)
      {
        // Cannot setup websocket driver for reading
        return S_FALSE;
      }
      if(receiveSize == 0)
      {
        // Already processed call
        return S_OK;
      }
      // WebSocket translation library requests fewer bytes
      if(receiveSize < m_read_AccomodatedSize)
      {
        m_read_AccomodatedSize = receiveSize;
      }
      if(m_read_AccomodatedSize > *pcbData)
      {
        m_read_AccomodatedSize = *pcbData;
      }

      TRACE("SYSWEBSOCKET Register ReadFragment\n");

      // Set up for overlapped I/O
      memset(&m_wsrd,0,sizeof(OVERLAPPED));
      m_wsrd.Pointer = this;
      m_wsrd.hEvent = (HANDLE)WebSocketReceivePartialOverlapped;
      received = m_socket->RecvPartialOverlapped(m_read_buffer,m_read_AccomodatedSize,&m_wsrd);
      // Should return IO_PENDING
      if(!(received == NO_ERROR && (m_socket->GetLastError() == 0 || m_socket->GetLastError() == WSA_IO_PENDING)))
      {
        return S_FALSE;
      }
    }
    else
    {
      // SYNC Receive
      received = m_socket->RecvPartial(m_read_buffer,m_read_AccomodatedSize);
      if(received == SOCKET_ERROR)
      {
        return S_FALSE;
      }
      else
      {
        // Return how much we did exactly receive
        *pcbData = received;
      }
    }
  } // End of locking scope

  // PHASE 4: After the read

  // We expected to have read 'something' or we will be receiving it in the future
  if(pfCompletionExpected)
  {
    *pfCompletionExpected = (received > 0) ? FALSE : TRUE;
  }
  // Synchronous receive, even from overlapped I/O
  if(received > 0 && pvCompletionContext)
  {
    // Synchronous receive: past it on directly
    OVERLAPPED over;
    memset(&over,0,sizeof(OVERLAPPED));
    over.Internal     = m_socket->GetLastError();
    over.InternalHigh = received;
    ReceiveFragment(&over);
  }
  // Last moment we were active
  m_lastAction = _time64(nullptr);

  return S_OK;
}

int
SYSWebSocket::SetupForReceive()
{
  ULONG bufferCount = ARRAYSIZE(m_recvBuffers);

  HRESULT hr = WebSocketReceive(m_handle,nullptr,nullptr);
  if(SUCCEEDED(hr))
  {

    // Get an action to process.
    hr = WebSocketGetAction(m_handle,
                            WEB_SOCKET_RECEIVE_ACTION_QUEUE,
                            m_recvBuffers,
                            &bufferCount,
                            &m_recvAction,
                            &m_recvBufferType,
                            nullptr,
                            &m_actionReadContext);
    if(SUCCEEDED(hr))
    {
      TRACE("Buffersize for receive: %d\n",m_recvBuffers[0].Data.ulBufferLength);
      if(m_recvAction == WEB_SOCKET_RECEIVE_FROM_NETWORK_ACTION)
      {
        return m_recvBuffers[0].Data.ulBufferLength;
      }
      // Something left in the context of the websocket driver
      if(m_recvAction == WEB_SOCKET_INDICATE_RECEIVE_COMPLETE_ACTION)
      {
        OVERLAPPED over;
        memset(&over,0,sizeof(OVERLAPPED));
        over.Internal     = S_OK;
        over.InternalHigh = m_recvBuffers[0].Data.ulBufferLength;
        ReceiveFragment(&over);
        return 0L;
      }
    }
  } 
  TRACE("Failed to setup for receive: %d\n",hr);
  return SOCKET_ERROR;
}

// We now are waked by the I/O completion and have a filled m_read_buffer
void 
SYSWebSocket::ReceiveFragment(LPOVERLAPPED p_overlapped)
{
  ULONG   error            = (ULONG)p_overlapped->Internal;
  ULONG   bytesTransferred = (ULONG)p_overlapped->InternalHigh;
  ULONG   bytesTranslated  = 0;
  HRESULT hr = S_OK;
  ULONG   bufferCount;
  bool    getNextAction   = false;
  // Status BOOLS
  BOOL    connectionClose = FALSE;
  BOOL    utf8Encoded     = TRUE;
  BOOL    finalFragment   = FALSE;
  BOOL    sendPingPong    = FALSE;

  // Last moment we were active
  m_lastAction = _time64(nullptr);

  // In case of an error: pass it on to the application
  // we cannot continue to translate any buffered data
  if(error)
  {
    // See if other side is closing the WebSocket
    connectionClose = (error == ERROR_NO_MORE_ITEMS);

    if(m_read_Completion)
    {
      (*m_read_Completion)(error
                          ,m_read_CompletionContext
                          ,bytesTranslated
                          ,TRUE
                          ,TRUE
                          ,connectionClose);
    }
    return;
  }

  // Most likely an TCP/IP keep alive frame
  // Restart the read operation for the next TCP/IP frame
  if(bytesTransferred <= TCPIP_KEEPALIVE_FRAMESIZE)
  {
    m_socket->RecvPartialOverlapped(m_read_buffer,m_read_AccomodatedSize,&m_wsrd);
    return;
  }

  TRACE("SYSWEBSOCKET ReceiveFragment: %d bytes\n",bytesTransferred);

  // Locking scope
  {
    AutoCritSec lock(&m_lock);

    // FASE 1: TRANSLATE INCOMING BUFFER
    do
    {
      // Initialize variables that change with every loop revolution.
      // SHOULD NEVER BY SMALLER THAN 2!!
      bufferCount = ARRAYSIZE(m_recvBuffers);

      // Get an action to process.
      if(getNextAction)
      {
        hr = WebSocketGetAction(m_handle,
                                WEB_SOCKET_RECEIVE_ACTION_QUEUE,
                                m_recvBuffers,
                                &bufferCount,
                                &m_recvAction,
                                &m_recvBufferType,
                                nullptr,
                                &m_actionReadContext);
        if(FAILED(hr) || bufferCount == 0)
        {
          // If we cannot get an action, abort the handle but continue processing until all operations are completed.
          goto quit;
        }
      }
      ULONG processed = 0;

      switch(m_recvAction)
      {
        case WEB_SOCKET_NO_ACTION:
             // Ready with this receiving action. Must set up a new one!
             break;

        case WEB_SOCKET_RECEIVE_FROM_NETWORK_ACTION:
             memcpy_s(m_recvBuffers[0].Data.pbBuffer,m_recvBuffers[0].Data.ulBufferLength,m_read_buffer,bytesTransferred);
             processed = bytesTransferred;
             break;
        case WEB_SOCKET_INDICATE_RECEIVE_COMPLETE_ACTION:
             // Copy out the data to the application
             memcpy_s(m_read_Data,*m_read_Size,m_recvBuffers[0].Data.pbBuffer,m_recvBuffers[0].Data.ulBufferLength);

             // TRACING
//              m_recvBuffers[0].Data.pbBuffer[m_recvBuffers[0].Data.ulBufferLength] = 0;
//              OutputDebugString((char*)m_recvBuffers[0].Data.pbBuffer);
//              OutputDebugString("\n");

             switch(m_recvBufferType)
             {
                case WEB_SOCKET_UTF8_MESSAGE_BUFFER_TYPE:     utf8Encoded   = TRUE;
                                                              finalFragment = TRUE;
                                                              break;
                case WEB_SOCKET_UTF8_FRAGMENT_BUFFER_TYPE:    utf8Encoded   = TRUE;
                                                              finalFragment = FALSE;
                                                              break;
                case WEB_SOCKET_BINARY_MESSAGE_BUFFER_TYPE:   utf8Encoded   = FALSE;
                                                              finalFragment = TRUE;
                                                              break;
                case WEB_SOCKET_BINARY_FRAGMENT_BUFFER_TYPE:  utf8Encoded   = FALSE;
                                                              finalFragment = FALSE;
                                                              break;
                case WEB_SOCKET_CLOSE_BUFFER_TYPE:            memcpy_s(m_closeReason,WEB_SOCKET_MAX_CLOSE_REASON_LENGTH,m_recvBuffers[0].CloseStatus.pbReason,m_recvBuffers[0].CloseStatus.ulReasonLength);
                                                              m_closeReason[m_recvBuffers[0].CloseStatus.ulReasonLength] = 0;
                                                              m_closeStatus       = m_recvBuffers[0].CloseStatus.usStatus;
                                                              m_closeReasonLength = m_recvBuffers[0].CloseStatus.ulReasonLength;
                                                              connectionClose = TRUE;
                                                              break;
                case WEB_SOCKET_PING_PONG_BUFFER_TYPE:        // Possibly send back a PONG
                                                              sendPingPong = TRUE;
                                                              // Restart the read action
                                                              [[fallthrough]];
                case WEB_SOCKET_UNSOLICITED_PONG_BUFFER_TYPE: // That's quite OK. Do not take any action
                                                              // Restart the read action
                                                              m_recvBuffers[0].Data.ulBufferLength = 0;
                                                              finalFragment = TRUE;
                                                              break;
             }
             bytesTranslated += m_recvBuffers[0].Data.ulBufferLength;
             break;
      default:
             // This should never happen.
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
      WebSocketCompleteAction(m_handle,m_actionReadContext,processed);

      getNextAction = true;
    } 
    while(m_recvAction != WEB_SOCKET_NO_ACTION);

  } // End of locking scope
quit:

  // Ready with this receiving action. Must set up a new one!
  if(finalFragment)
  {
    // Reset the action context
    m_recvBuffers[0].Data.pbBuffer = nullptr;
    m_recvBuffers[0].Data.ulBufferLength = 0;
    m_recvBuffers[1].Data.pbBuffer = nullptr;
    m_recvBuffers[1].Data.ulBufferLength = 0;
    m_actionReadContext = nullptr;
  }

  // We received a PING, trye to send back a PONG
  if(sendPingPong)
  {
    SendPingPong(FALSE);
  }

  // FASE 2: STORE CLOSING REASON AS AN UTF-16 STRING
  if(m_closeReasonLength)
  {
#ifdef _UNICODE
    CString reason;
    bool foundBom(false);
    TryConvertNarrowString((const BYTE*)m_closeReason,m_closeReasonLength,_T(""),reason,foundBom);
    wcscpy_s((wchar_t*)m_closeReason,WEB_SOCKET_MAX_CLOSE_REASON_LENGTH,(wchar_t*)reason.GetString());
#else
    CString reason((char*)m_closeReason);
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
  *m_read_Size = bytesTranslated;

  // In case of an overlapped I/O action, call back our application context
  if(m_read_Completion)
  {
    (*m_read_Completion)(hr
                        ,m_read_CompletionContext
                        ,bytesTranslated
                        ,utf8Encoded
                        ,finalFragment
                        ,connectionClose);
  }
  else
  {
    // SYNC completion (utf8/final/close)
  }
}

HRESULT
SYSWebSocket::SendConnectionClose(_In_    BOOL                     fAsync,
                                  _In_    USHORT                   uStatusCode,
                                  _In_    LPCWSTR                  pszReason            /*= NULL*/,
                                  _In_    PFN_WEBSOCKET_COMPLETION pfnCompletion        /*= NULL*/,
                                  _In_    VOID* pvCompletionContext  /*= NULL*/,
                                  _Out_   BOOL* pfCompletionExpected /*= NULL*/)
{
  int written = 0;

  // Check status code
  if(uStatusCode < WEB_SOCKET_SUCCESS_CLOSE_STATUS || uStatusCode > WEB_SOCKET_SECURE_HANDSHAKE_ERROR_CLOSE_STATUS)
  {
    return ERROR_INVALID_PARAMETER;
  }

  // Getting the reason
  if(wcslen(pszReason) > WEB_SOCKET_MAX_CLOSE_REASON_LENGTH)
  {
    return ERROR_INVALID_PARAMETER;
  }

  // Lock early, in race conditions the close can happen twice
  {
    AutoCritSec lock(&m_lock);

    wcscpy_s(m_closeReason,WEB_SOCKET_MAX_CLOSE_REASON_LENGTH,pszReason);
    m_closeReasonLength = (ULONG)wcslen(pszReason);

    // Remember new closed status
    m_closeStatus = uStatusCode;

    CStringW reason(pszReason);
    CStringA reasonA(reason);
    DWORD length = reasonA.GetLength();

    m_send_Data = (void*)reasonA.GetString();
    m_send_Size = &length;
    m_send_Completion = pfnCompletion;
    m_send_CompletionContext = pvCompletionContext;
    m_send_utf8 = TRUE;
    m_send_final = TRUE;
    m_send_close = TRUE;

    if(fAsync)
    {
      // Make sure we have a send buffer
      if(!m_send_buffer)
      {
        m_send_buffer = new BYTE[m_bufferSizeSend + WS_MAX_HEADER];
      }
      strncpy_s((char*)m_send_buffer,(int)m_bufferSizeSend,(char*)m_send_Data,*m_send_Size);

      // Set up the WebSocket translation
      DWORD extra = EncodingFragment(true);
      if(extra == 0)
      {
        return S_FALSE;
      }
      strncpy_s((char*)&m_send_buffer[extra],(int)m_bufferSizeSend,(char*)m_send_Data,*m_send_Size);
      m_send_buffer[extra + *m_send_Size] = 0;

      // Set up for overlapped I/O
      m_wswr.Pointer = this;
      m_wswr.hEvent = (HANDLE)WebSocketWritingPartialOverlapped;
      written = m_socket->SendPartialOverlapped(m_send_buffer,extra + *m_send_Size,&m_wswr);
      // Should return IO_PENDING
      if(written != NO_ERROR)
      {
        return S_FALSE;
      }
    }
    else
    {
      // SYNC Receive
      written = m_socket->SendPartial(m_send_Data,*m_send_Size);
      if(written == SOCKET_ERROR)
      {
        return S_FALSE;
      }
    }
  } // End of locking scope

  // PHASE 4: After the sending

  // We expected to have read 'something' or we will be receiving it in the future
  if(pfCompletionExpected)
  {
    *pfCompletionExpected = (written > 0) ? FALSE : TRUE;
  }

  // Synchronous receive, even from overlapped I/O
  if(written > 0 && pvCompletionContext)
  {
    // Synchronous receive: past it on directly
    OVERLAPPED over;
    memset(&over,0,sizeof(OVERLAPPED));
    over.Internal     = m_socket->GetLastError();
    over.InternalHigh = written;
    WritingFragment(&over);
  }
  // Last moment we were active
  m_lastAction = _time64(nullptr);

  return S_OK;
}

// Getting the closing status code and reason string
// Was already stored in UTF-16 format for the MS-Windows system
HRESULT 
SYSWebSocket::GetCloseStatus(_Out_   USHORT*  pStatusCode,
                             _Out_   LPCWSTR* ppszReason  /*= NULL*/,
                             _Out_   USHORT*  pcchReason  /*= NULL*/)
{
  AutoCritSec lock(&m_lock);

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
  // Last moment we were active
  m_lastAction = _time64(nullptr);
}

VOID
SYSWebSocket::CancelOutstandingIO(VOID)
{
  if(m_socket)
  {
    m_socket->Disconnect();
  }
  // Last moment we were active
  m_lastAction = _time64(nullptr);
}

UINT64 
SYSWebSocket::GetSocketIdentifier(VOID)
{
  return m_ident;
}
