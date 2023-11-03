/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: WebSocketContext.cpp
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
#include "StdAfx.h"
#include "WebSocketContext.h"
#include <assert.h>
#include "ServiceReporting.h"
#include <io.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

WebSocketContext::WebSocketContext()
{
}

WebSocketContext::~WebSocketContext()
{
  Reset();
}

bool
WebSocketContext::PerformHandshake(_In_  WEB_SOCKET_HTTP_HEADER*  p_clientHeaders,
                                   _In_  ULONG                    p_clientHeadersCount,
                                   _Out_ WEB_SOCKET_HTTP_HEADER** p_serverHeaders,
                                   _Out_ ULONG*                   p_serverHeadersCount,
                                   _In_  PCSTR                    /*p_subProtocol*/)
{
  // Init
  *p_serverHeaders = nullptr;
  *p_serverHeadersCount = 0;

  CreateServerHandle();

  if(m_handle)
  {
    HRESULT hr = WebSocketBeginServerHandshake(m_handle,
                                               nullptr,
                                               nullptr, 0,
                                               p_clientHeaders,p_clientHeadersCount,
                                               p_serverHeaders,p_serverHeadersCount);
    if(SUCCEEDED(hr))
    {
      // Keep the pointers for reclamation
      return true;
    }
  }
  Reset();
  return false;
}

bool
WebSocketContext::CompleteHandshake()
{
  if(m_handle)
  {
    HRESULT hr = WebSocketEndServerHandshake(m_handle);
    return SUCCEEDED(hr);
  }
  return false;
}

void 
WebSocketContext::SetRecieveBufferSize(ULONG p_size)
{
  if(p_size < WEBSOCKET_BUFFER_MINIMUM)
  {
    p_size = WEBSOCKET_BUFFER_MINIMUM;
  }
  m_ws_recv_buffersize = p_size;
}

void 
WebSocketContext::SetSendBufferSize(ULONG p_size)
{
  if(p_size < WEBSOCKET_BUFFER_MINIMUM)
  {
    p_size = WEBSOCKET_BUFFER_MINIMUM;
  }
  m_ws_send_buffersize = p_size;
}

void 
WebSocketContext::SetDisableClientMasking(BOOL p_disable)
{
  m_ws_disable_masking = p_disable;
}

void 
WebSocketContext::SetDisableUTF8Checking(BOOL p_disable)
{
  m_ws_disable_utf8_verify = p_disable;
}

void 
WebSocketContext::SetKeepAliveInterval(ULONG p_interval)
{
  if(p_interval < WEBSOCKET_KEEPALIVE_MIN)
  {
    p_interval = WEBSOCKET_KEEPALIVE_MIN;
  }
  m_ws_keepalive_interval = p_interval;
}

HRESULT 
WebSocketContext::ReadFragment(_Out_   VOID*  pData,
                               _Inout_ DWORD* pcbData,
                               _In_    BOOL   /*fAsync*/,
                               _Out_   BOOL*  pfUTF8Encoded,
                               _Out_   BOOL*  pfFinalFragment,
                               _Out_   BOOL*  pfConnectionClose,
                               _In_    PFN_WEBSOCKET_COMPLETION /*pfnCompletion*/ /*= NULL*/,
                               _In_    VOID*  /*pvCompletionContext*/  /*= NULL*/,
                               _Out_   BOOL*  pfCompletionExpected /*= NULL*/)
{
  HRESULT hr = S_OK;
  WEB_SOCKET_ACTION action = WEB_SOCKET_NO_ACTION;
  WEB_SOCKET_BUFFER buffers[2] = { 0 };
  WEB_SOCKET_BUFFER_TYPE bufferType = (WEB_SOCKET_BUFFER_TYPE)0;
  ULONG bufferCount = 0;
  ULONG bytesTransferred = 0;

  // Keeping the buffer
  buffers[0].Data.pbBuffer = reinterpret_cast<PBYTE>(pData);
  buffers[0].Data.ulBufferLength = *pcbData;

  // Setting reasonable default answers
  *pfUTF8Encoded        = FALSE;
  *pfFinalFragment      = FALSE;
  *pfConnectionClose    = FALSE;
  if(pfCompletionExpected)
  {
    *pfCompletionExpected = FALSE;
  }
  // Reset our action context and initiate the receive action
  m_actionContext = nullptr;
  hr = WebSocketReceive(m_handle,nullptr,nullptr);
  if(FAILED(hr))
  {
    goto quit;
  }

  // RUNLOOP
  do
  {
    // Initialize variables that change with every loop revolution.
    bufferCount = ARRAYSIZE(buffers);
    bytesTransferred = 0;

    // Get an action to process.
    hr = WebSocketGetAction(m_handle,
                            WEB_SOCKET_RECEIVE_ACTION_QUEUE,
                            buffers,
                            &bufferCount,
                            &action,
                            &bufferType,
                            NULL,
                            &m_actionContext);
    if(FAILED(hr))
    {
      // If we cannot get an action, abort the handle but continue processing until all operations are completed.
      WebSocketAbortHandle(m_handle);
    }

    switch(action)
    {
      case WEB_SOCKET_NO_ACTION: 
        // No action to perform - just exit the loop.
        break;

      case WEB_SOCKET_RECEIVE_FROM_NETWORK_ACTION:

        assert(bufferCount == 1);
        // Read data from a transport (in production application this may be a socket).
        if(false) //(fAsync)
        {
          // ReadFileEx(m_handle,pData,*pcbData,iocompletion,routine);
        }
        else
        {
          // NOT READY IMPLEMENTING ON MarlinServer !!!


          if(!ReadFile(m_handle,buffers[0].Data.pbBuffer,buffers[0].Data.ulBufferLength,&bytesTransferred,nullptr))
          {
            int result = GetLastError();
            SvcReportErrorEvent(0,true,_T(__FUNCTION__),_T("Socket error: %d"),result);
            return -result;
          }

//           WSABUF buf;
//           buf.buf = (CHAR*) buffers[0].Data.pbBuffer;
//           buf.len = (ULONG) buffers[0].Data.ulBufferLength;
// 
//           int receiveResult = WSARecv((SOCKET)m_handle,&buf,1,&bytesTransferred,0,nullptr,nullptr);
//           if(bytesTransferred <= 0 || receiveResult == SOCKET_ERROR)
//           {
//             int result = WSAGetLastError();
//             SvcReportErrorEvent(0,true,__FUNCTION__,"Socket error: %d",result);
//             return -result;
//           }

        }
        // Less read than buffer size -> This is the final fragment
        *pfFinalFragment = (buffers[0].Data.ulBufferLength > bytesTransferred) ? TRUE : FALSE;

        break;

      case WEB_SOCKET_INDICATE_RECEIVE_COMPLETE_ACTION:

        if (bufferCount != 1)
        {
          assert(!"This should never happen.");
          hr = E_FAIL;
          goto quit;
        }
        *pcbData = buffers[0].Data.ulBufferLength;
        *pfUTF8Encoded = (bufferType == WEB_SOCKET_UTF8_MESSAGE_BUFFER_TYPE) ? TRUE : FALSE;

        break;
      default:
        // This should never happen.
        assert(!"Invalid switch");
        hr = E_FAIL;
        goto quit;
    }

    if (FAILED(hr))
    {
      // If we failed at some point processing actions, abort the handle but continue processing
      // until all operations are completed.
      WebSocketAbortHandle(m_handle);
    }

    // Complete the action. If application performs asynchronous operation, the action has to be
    // completed after the async operation has finished. The 'actionContext' then has to be preserved
    // so the operation can complete properly.
    WebSocketCompleteAction(m_handle,m_actionContext,bytesTransferred);
  }
  while(action != WEB_SOCKET_NO_ACTION);

quit:
  return hr;
}

// NOT IMPLEMENTED !!
#pragma warning (disable: 6101)
#pragma warning (disable: 6054)
HRESULT 
WebSocketContext::GetCloseStatus(_Out_ USHORT*  /*pStatusCode*/,
                                 _Out_ LPCWSTR* /*ppszReason*/ /*= NULL*/,
                                 _Out_ USHORT*  /*pcchReason*/ /*= NULL*/)
{
  return S_FALSE;
}


//////////////////////////////////////////////////////////////////////////
//
// PRIVATE
//
//////////////////////////////////////////////////////////////////////////

void
WebSocketContext::Reset()
{
  // Simply close the socket handle and ignore the errors
  WebSocketDeleteHandle(m_handle);
  m_handle = NULL;

  if(m_ws_buffer)
  {
    delete[] m_ws_buffer;
    m_ws_buffer = nullptr;
  }
}

bool
WebSocketContext::CreateServerHandle()
{
  // Make sure we have enough buffering space for the socket
  if(!m_ws_buffer)
  {
    m_ws_buffer = new BYTE[(size_t)m_ws_recv_buffersize + (size_t)m_ws_send_buffersize + WEBSOCKET_BUFFER_OVERHEAD];
  }

  WEB_SOCKET_PROPERTY properties[5] = 
  {
    { WEB_SOCKET_RECEIVE_BUFFER_SIZE_PROPERTY_TYPE,      &m_ws_recv_buffersize,     sizeof(ULONG) }
   ,{ WEB_SOCKET_SEND_BUFFER_SIZE_PROPERTY_TYPE,         &m_ws_send_buffersize,     sizeof(ULONG) }
   ,{ WEB_SOCKET_DISABLE_MASKING_PROPERTY_TYPE,          &m_ws_disable_masking,     sizeof(BOOL)  }
   ,{ WEB_SOCKET_DISABLE_UTF8_VERIFICATION_PROPERTY_TYPE,&m_ws_disable_utf8_verify, sizeof(BOOL)  }
   ,{ WEB_SOCKET_ALLOCATED_BUFFER_PROPERTY_TYPE,         &m_ws_buffer,              sizeof(DWORD_PTR) }
//    Interval not accepted by the API.
//    Interval can be set by registry key: HKLM:\SOFTWARE\Microsoft\WebSocket\KeepaliveInterval
// ,{ WEB_SOCKET_KEEPALIVE_INTERVAL_PROPERTY_TYPE,       &m_ws_keepalive_interval,  sizeof(ULONG) }
  };

  // Create our websocket handle
//HRESULT hr = WebSocketCreateServerHandle(properties,5,&m_handle);
  HRESULT hr = WebSocketCreateServerHandle(nullptr,0,&m_handle);
  return SUCCEEDED(hr);
}
