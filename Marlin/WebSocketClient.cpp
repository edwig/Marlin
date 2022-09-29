/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: WebSocketClient.cpp
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
#include "stdafx.h"
#include "WebSocketClient.h"
#include "GetLastErrorAsString.h"
#include "HTTPError.h"
#include "Base64.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define DETAILLOG1(text)          if(MUSTLOG(HLL_LOGGING) && m_logfile) { DetailLog (__FUNCTION__,LogType::LOG_INFO,text); }
#define DETAILLOGS(text,extra)    if(MUSTLOG(HLL_LOGGING) && m_logfile) { DetailLogS(__FUNCTION__,LogType::LOG_INFO,text,extra); }
#define DETAILLOGV(text,...)      if(MUSTLOG(HLL_LOGGING) && m_logfile) { DetailLogV(__FUNCTION__,LogType::LOG_INFO,text,__VA_ARGS__); }
#define ERRORLOG(code,text)       ErrorLog (__FUNCTION__,code,text)

//////////////////////////////////////////////////////////////////////////
//
// CLIENT WEBSOCKET
//
//////////////////////////////////////////////////////////////////////////

WebSocketClient::WebSocketClient(XString p_uri)
                :WebSocket(p_uri)
{
  LoadHTTPLibrary();
}

WebSocketClient::~WebSocketClient()
{
  CloseSocket();
  FreeHTTPLibrary();
}

void
WebSocketClient::Reset()
{
  // Reset the main class part
  WebSocket::Reset();

  // Reset our part
  m_socket   = NULL;
  m_listener = NULL;
  m_socketKey.Empty();
}

// Load function pointers from WinHTTP library
void
WebSocketClient::LoadHTTPLibrary()
{
  m_winhttp = LoadLibrary("WinHTTP.dll");
  if(m_winhttp)
  {
    m_websocket_complete    = (WSOCK_COMPLETE)  GetProcAddress(m_winhttp,"WinHttpWebSocketCompleteUpgrade");
    m_websocket_close       = (WSOCK_CLOSE)     GetProcAddress(m_winhttp,"WinHttpWebSocketClose");
    m_websocket_queryclose  = (WSOCK_QUERYCLOSE)GetProcAddress(m_winhttp,"WinHttpWebSocketQueryCloseStatus");  
    m_websocket_send        = (WSOCK_SEND)      GetProcAddress(m_winhttp,"WinHttpWebSocketSend");
    m_websocket_receive     = (WSOCK_RECEIVE)   GetProcAddress(m_winhttp,"WinHttpWebSocketReceive");
  }

  if(m_websocket_complete   == nullptr ||
     m_websocket_close      == nullptr ||
     m_websocket_queryclose == nullptr ||
     m_websocket_send       == nullptr ||
     m_websocket_receive    == nullptr)
  {
    // Mark for OpenSocket to fail on a websocket
    m_websocket_complete = nullptr;
  }
}

void
WebSocketClient::FreeHTTPLibrary()
{
  if(m_winhttp)
  {
    FreeLibrary(m_winhttp);
    m_winhttp = nullptr;
  }
}

// Setting parameters for the client socket
void
WebSocketClient::AddWebSocketHeaders()
{
  // Principal WebSocket handshake
  if(!::WinHttpSetOption(m_socket,WINHTTP_OPTION_WEB_SOCKET_CLOSE_TIMEOUT,&m_closingTimeout,sizeof(unsigned)))
  {
    ERRORLOG(ERROR_INVALID_FUNCTION,"Cannot set WebSocket closing timeout interval. Error [%d] %s");
  }
  if(!::WinHttpSetOption(m_socket,WINHTTP_OPTION_WEB_SOCKET_KEEPALIVE_INTERVAL,&m_keepalive,sizeof(unsigned)))
  {
    ERRORLOG(ERROR_INVALID_FUNCTION,"Cannot set WebSocket keep-alive interval. Error [%d] %s");
  }
  DETAILLOGV("Prepared for WebSocket upgrade. Timeout: %d Keep-alive: %d",m_closingTimeout,m_keepalive);
}

bool
WebSocketClient::OpenSocket()
{
  // Totally reset of the HTTPClient
  HTTPClient client;

  // Connect the logging
  client.SetLogging(m_logfile);
  client.SetLogLevel(m_logLevel);

  // Check if WINHTTP library was loaded
  if(m_websocket_complete == nullptr)
  {
    ERRORLOG(ERROR_DLL_INIT_FAILED,"Could not load WebSocket functions from WINHTTP.DLL");
    return false;
  }

  // GET this URI (ws[s]://resource) !!
  client.SetVerb("GET");
  client.SetURL(m_uri);

  // Add extra protocol headers
  if(!m_protocols.IsEmpty())
  {
    client.AddHeader("Sec-WebSocket-Protocol",m_protocols);
  }
  if(!m_extensions.IsEmpty())
  {
    client.AddHeader("Sec-WebSocket-Extensions",m_extensions);
  }
  if(!m_headers.empty())
  {
    for(auto& header : m_headers)
    {
      client.AddHeader(header.first,header.second);
    }
  }

  // We will do the WebSocket handshake on this client!
  // This keeps open the request handle for output as well
  client.SetWebsocketHandshake(true);

  // Send a bare HTTP line, just with headers: no body!
  if(client.Send())
  {
    // Wait for the opening of the WebSocket
    if(client.GetStatus() == HTTP_STATUS_SWITCH_PROTOCOLS)
    {
      // Switch the handles from WinHTTP to WinSocket
      HINTERNET handle = client.GetWebsocketHandle();
      m_socket = m_websocket_complete(handle,NULL); // WinHttpWebSocketCompleteUpgrade
      if(m_socket)
      {
        // Remember the identity key
        HeaderMap::iterator key = client.GetResponseHeaders().find("Sec-Websocket-Accept");
        if(key != client.GetResponseHeaders().end())
        {
          m_key = key->second;

          // Set our timeout headers
          AddWebSocketHeaders();

          // Trying to start the listener
          if(StartClientListner())
          {
            // If we come back here the receive thread is running
            DETAILLOGS("WebSocket open for: ",m_uri);
            m_openReading = true;
            m_openWriting = true;
            OnOpen();
          }
        }
        else
        {
          ERRORLOG(ERROR_NOT_FOUND,"Socket upgrade to WinSocket failed. Sec-WebSocket-Accept not found\n");
        }
      }
      else
      {
        DWORD error = GetLastError();
        XString message;
        message.Format("Socket upgrade to WinSocket failed [%d] %s\n",error,GetHTTPErrorText(error).GetString());
        ERRORLOG(error,message);
      }
    }
    else
    {
      // No switching of protocols. Really strange to succeed in the HTTP call.
      ERRORLOG(ERROR_PROTOCOL_UNREACHABLE,"Server cannot switch from HTTP to WebSocket protocol");
    }
  }
  else
  {
    // Error handling. Socket not found on URI
    XString error;
    error.Format("WebSocket protocol not found on URI [%s] HTTP status [%d]",m_uri.GetString(),client.GetStatus());
    ERRORLOG(ERROR_NOT_FOUND,error);
  }
  return m_openReading && m_openWriting;
}

// Close the socket
// Close the underlying TCP/IP connection
bool 
WebSocketClient::CloseSocket()
{
  // Hard close
  if(m_socket)
  {
    DETAILLOGS("Hard TCP/IP close for WebSocket: ",m_uri);
    m_openReading = false;
    m_openWriting = false;

    WinHttpCloseHandle(m_socket);

    DWORD wait = 10;
    while(m_listener && wait <= 6400)
    {
      Sleep(wait);
      wait *= 2;
    }
    m_socket = NULL;

    // Really get rid of the thread!
    // Since waiting on the thread did not work, we must preemptively terminate it.
#pragma warning(disable:6258)
    if(m_listener)
    {
      TerminateThread(m_listener,0);
    }
  }
  return true;
}

// Close the socket with a closing frame
bool 
WebSocketClient::SendCloseSocket(USHORT p_code,XString p_reason)
{
  // Already closed?
  if(!m_socket)
  {
    return true;
  }

  // Check if p_reason is shorter then or equal to 123 bytes 
  DWORD length = p_reason.GetLength();
  if(length > WS_CLOSE_MAXIMUM)
  {
    length = WS_CLOSE_MAXIMUM;
  }
  DETAILLOGV("Send close WebSocket [%d:%s] for: %s",p_code,p_reason.GetString(),m_uri.GetString());
  DWORD error = m_websocket_close(m_socket,p_code,(void*)p_reason.GetString(),length); // WinHttpWebSocketClose
  if(error && (error != ERROR_WINHTTP_OPERATION_CANCELLED))
  {
    // We could be in a tight spot (socket already closed)
    if(error == ERROR_INVALID_OPERATION)
    {
      return true;
    }
    // Failed to send a close socket message.
    XString message = "Failed to send WebSocket 'close' message: " + GetLastErrorAsString(error);
    ERRORLOG(error,message);
    return false;
  }
  else if(!m_closingError)
  {
    // ReceiveCloseSocket();
  }
  // Tell it to the application
  OnClose();

  // The other side acknowledged the fact that they did close also
  // It was an answer on an incoming 'close' message
  // We did our answering, so close completely
  CloseSocket();

  return true;
}

// Decode the incoming close socket message
bool
WebSocketClient::ReceiveCloseSocket()
{
  BYTE  reason[WS_CLOSE_MAXIMUM + 1];
  DWORD received = 0;

  if(m_socket)
  {
    // WinHttpWebSocketQueryCloseStatus
    DWORD error = m_websocket_queryclose(m_socket
                                        ,&m_closingError
                                        ,&reason
                                        ,WS_CLOSE_MAXIMUM + 1
                                        ,&received);
    if(error == ERROR_INVALID_OPERATION)
    {
      // No closing frame received yet
      m_closingError = 0;
      m_closing.Empty();
      return false;
    }
    else
    {
      reason[received] = 0;
      m_closing = reason;
      DETAILLOGV("Closing WebSocket frame received [%d:%s]",m_closingError,m_closing.GetString());
      WinHttpCloseHandle(m_socket);
      m_socket = NULL;
      m_openReading = false;
      m_openWriting = false;
    }
    return true;
  }
  return false;
}

// Writing one fragment out to the WebSocket handle
// Works regarding the type of fragment
bool
WebSocketClient::WriteFragment(BYTE* p_buffer,DWORD p_length,Opcode p_opcode,bool p_last /* = true */)
{
  // See if we can do anything at all
  if(!m_socket)
  {
    return false;
  }

  WINHTTP_WEB_SOCKET_BUFFER_TYPE type = WINHTTP_WEB_SOCKET_UTF8_MESSAGE_BUFFER_TYPE;
  if(p_opcode == Opcode::SO_UTF8)
  {
    type = p_last ? WINHTTP_WEB_SOCKET_UTF8_MESSAGE_BUFFER_TYPE : 
                    WINHTTP_WEB_SOCKET_UTF8_FRAGMENT_BUFFER_TYPE;
  }
  else if(p_opcode == Opcode::SO_BINARY)
  {
    type = p_last ? WINHTTP_WEB_SOCKET_BINARY_MESSAGE_BUFFER_TYPE : 
                    WINHTTP_WEB_SOCKET_BINARY_FRAGMENT_BUFFER_TYPE;
  }
  else if(p_opcode == Opcode::SO_CLOSE)
  {
    type = WINHTTP_WEB_SOCKET_CLOSE_BUFFER_TYPE;
  }

  // WinHttpWebSocketSend
  DWORD error = m_websocket_send(m_socket,type,p_buffer,p_length);
  if(error)
  {
    ERRORLOG(error,"ERROR while sending to WebSocket: " + m_uri);
    switch(error)
    {
      case ERROR_INVALID_OPERATION: // Socket closed
                                    CloseSocket();
                                    return false;
      case ERROR_INVALID_PARAMETER: // Buffer not matched
                                    return false;
    }
  }
  else
  {
    DETAILLOGV("WebSocket sent type: %d Bytes: %d to: %s",type,p_length,m_uri.GetString());
  }
  return (error == ERROR_SUCCESS);
}

void
WebSocketClient::SocketListener()
{
  // Install SEH to regular exception translator
  _set_se_translator(SeTranslator);

  // Wait a short time for the OnOpen event to have been handled
  // Immediately after the OnOpen, the "m_open" goes to 'true'
  for(unsigned wait = 10; wait <= 320; wait *= 2)
  {
    if(m_openReading)
    {
      break;
    }
    Sleep(wait);
  }

  do 
  {
    if(!m_reading)
    {
      m_reading = new WSFrame;
      m_reading->m_data = (BYTE*)malloc((size_t)m_fragmentsize + WS_OVERHEAD);
    }
    // Happens on SocketClose from the server
    if(!m_socket)
    {
      break;
    }

    if(!m_reading->m_data)
    {
      ERRORLOG(ERROR_NOT_ENOUGH_MEMORY,"Reading websocket data!");
      CloseSocket();
      return;
    }

    DWORD bytesRead = 0;
    WINHTTP_WEB_SOCKET_BUFFER_TYPE type;
    // WinHttpWebSocketReceive
    DWORD error = m_websocket_receive(m_socket
                                     ,&m_reading->m_data[m_reading->m_length]
                                     ,m_fragmentsize
                                     ,&bytesRead
                                     ,&type);
    if(error)
    {
      if(error != ERROR_WINHTTP_OPERATION_CANCELLED)
      {
        ERRORLOG(error, "ERROR while receiving from WebSocket: " + m_uri);
        CloseSocket();
      }
      m_openReading = false;
    }
    else
    {
      DETAILLOGV("WebSocket receive type: %d Bytes: %d to: %s",type,bytesRead,m_uri.GetString());

      bool final = (type == WINHTTP_WEB_SOCKET_BINARY_MESSAGE_BUFFER_TYPE) ||
                   (type == WINHTTP_WEB_SOCKET_UTF8_MESSAGE_BUFFER_TYPE)   ||
                   (type == WINHTTP_WEB_SOCKET_CLOSE_BUFFER_TYPE);
      bool utf8  = (type == WINHTTP_WEB_SOCKET_UTF8_MESSAGE_BUFFER_TYPE)   ||
                   (type == WINHTTP_WEB_SOCKET_UTF8_FRAGMENT_BUFFER_TYPE);

      if(final)
      {
        // Fragment is complete
        m_reading->m_final   = final;
        m_reading->m_utf8    = utf8;
        m_reading->m_length += bytesRead;
        m_reading->m_data[m_reading->m_length] = 0;

        StoreWSFrame(m_reading);

        if(type == WINHTTP_WEB_SOCKET_CLOSE_BUFFER_TYPE)
        {
          ReceiveCloseSocket();
          if(m_openWriting)
          {
            SendCloseSocket(WS_CLOSE_NORMAL,"Socket closed!");
          }
          else
          {
            OnClose();
          }
          CloseSocket();
        }
        else if(utf8)
        {
          OnMessage();
        }
        else
        {
          OnBinary();
        }
      }
      else
      {
        // Reading another fragment
        if(utf8)
        {
          // Just append another fragment after the current one
          // And keep reading until we find the final UTF-8 fragment
          DWORD newsize = m_reading->m_length + bytesRead + m_fragmentsize + WS_OVERHEAD;
          m_reading->m_data = (BYTE*)realloc(m_reading->m_data,newsize);
          m_reading->m_length += bytesRead;
        }
        else
        {
          // Partial fragment for a binary buffer 
          // Store the fragment and call the handler
          m_reading->m_final  = false;
          m_reading->m_utf8   = false;
          m_reading->m_length = bytesRead;
          m_reading->m_data[bytesRead] = 0;
          StoreWSFrame(m_reading);
          OnBinary();
        }
      }
    }
  } 
  while (m_openReading);

  OnClose();

  // Ready with this listener
  m_listener = NULL;
}

unsigned int __stdcall StartingClientListenerThread(void* p_context)
{
  WebSocketClient* client = reinterpret_cast<WebSocketClient*>(p_context);
  client->SocketListener();
  return 0;
}

// Starting a listener for the WebSocket
bool
WebSocketClient::StartClientListner()
{
  if(m_listener == NULL)
  {
    // Thread for the client queue
    unsigned int threadID = 0;
    if((m_listener = (HANDLE)_beginthreadex(NULL,0,StartingClientListenerThread,(void *)(this),0,&threadID)) == INVALID_HANDLE_VALUE)
    {
      m_listener = NULL;
      ERRORLOG(GetLastError(),"Cannot start client listener thread for a WebSocket");
    }
    else
    {
      DETAILLOGV("Thread started with threadID [%d] for WebSocket stream.",threadID);
      return true;
    }
  }
  return false;
}

// Client key generator for the WebSocket protocol
// Now using the built-in key generator of WinHTTP
XString
WebSocketClient::GenerateKey()
{
  // crank up the random generator
  clock_t time = clock();
  srand((unsigned int)time);

  // WebSocket key is 16 bytes random data
  BYTE key[16];
  for(int ind = 0; ind < 16; ++ind)
  {
    // Take random value between 0 and 255 inclusive
    key[ind] = (BYTE)(rand() / ((RAND_MAX + 1) / 0x100));
  }

  // Set key in a base64 encoded string
  Base64 base;
  char* buffer = m_socketKey.GetBufferSetLength((int)base.B64_length(16) + 1);
  base.Encrypt(key,16,(unsigned char*)buffer);
  m_socketKey.ReleaseBuffer();

  DETAILLOGS("Generated client WebSocket key: ",m_socketKey);
  return m_socketKey;
}
