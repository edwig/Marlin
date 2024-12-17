//////////////////////////////////////////////////////////////////////////
//
// USER-SPACE IMPLEMENTTION OF HTTP.SYS
//
// 2018 - 2024 (c) ir. W.E. Huisman
// License: MIT
//
//////////////////////////////////////////////////////////////////////////

#pragma once
#define SECURITY_WIN32
#include <sspi.h>

// Test to see if it is still a request object
#define HTTP_REQUEST_IDENT 0x00EDED0000EDED00

// For header lines
#define MESSAGE_BUFFER_LENGTH (16*1024)
// For files, the buffer should be arbitrarily shorter than the maximum TCP/IP frame
// To accommodate the header blocks of the TCP/IP stack ( a few hundred bytes)
#define FILE_BUFFER_LENGTH    (16*1000)

// Minimum timeout for HTTP body receiving in seconds
#define HTTP_MINIMUM_TIMEOUT 10 
// Default space for a SSPI authentication provider buffer
#define SSPI_BUFFER_DEFAULT (8*1024)

typedef enum _rq_status
{
  RQ_CREATED    // Object created and initialized
 ,RQ_RECEIVED   // Fully received + headers. Waiting to be serviced
 ,RQ_READING    // Server is busy reading the request body
 ,RQ_ANSWERING  // Server is busy answering with a response header
 ,RQ_WRITING    // Server is busy writing response body
 ,RQ_OPAQUE     // Request is in 'opaque' mode
 ,RQ_SERVICED   // Server is ready with the request
}
RQ_Status;

class RequestQueue;
class Listener;
class SocketStream;
class SYSWebSocket;

class Request
{
public:
  Request(RequestQueue* p_queue
         ,Listener*     p_listener
         ,SOCKET        p_socket
         ,HANDLE        p_stopEvent);
 ~Request();

  // SETTERS
  void              SetStatus(RQ_Status p_status)             { m_status                = p_status;  }
  void              SetURLContext(HTTP_URL_CONTEXT p_context) { m_request.UrlContext    = p_context; }
  void              SetBytesRead(ULONG p_bytes)               { m_request.BytesReceived = p_bytes;   }

  // GETTERS
  ULONGLONG         GetIdent()          { return m_ident;               }
  RQ_Status         GetStatus()         { return m_status;              }
  SocketStream*     GetSocket()         { return m_socket;              }
  ULONG             GetBytes()          { return m_bytesRead;           }
  PHTTP_REQUEST_V2  GetV2Request()      { return &m_request;            }
  HTTP_URL_CONTEXT  GetURLContext()     { return m_request.UrlContext;  }
  PCSTR             GetURL()            { return m_request.pRawUrl;     }
  ULONGLONG         GetContentLength()  { return m_contentLength;       }
  Listener*         GetListener()       { return m_listener;            }
  HANDLE            GetAccessToken()    { return m_token;               }
  SYSWebSocket*     GetWebSocket()      { return m_websocket;           }
  bool              GetResponseComplete();
  XString           GetHostName();

  // FUNCTIONS
  void              ReceiveRequest();

  void              FindUrlContext();
  int               ReceiveBuffer(PVOID p_buffer,ULONG p_size,PULONG p_bytes,bool p_all);
  void              ReceiveChunk(PVOID p_buffer, ULONG p_size);
  int               SendResponse(PHTTP_RESPONSE p_response,ULONG p_flags,PULONG p_bytes);
  int               SendEntityChunks(PHTTP_DATA_CHUNK p_chunks,int p_count,PULONG p_bytes);
  bool              RestartConnection();
  void              CloseRequest();
  // Reply with an error
  void              ReplyClientError();
  void              ReplyClientError(int p_error,CString p_errorText);
  void              ReplyServerError();
  void              ReplyServerError(int p_error, CString p_errorText);

private:

  //////////////////////////////////////////////////////////////////////////
  // PRIVATE
  //////////////////////////////////////////////////////////////////////////

  void              Reset();
  void              ResetRequestV1();
  void              ResetRequestV2();
  void              SetSocket(Listener* p_listener,SOCKET p_socket,HANDLE p_stop);
  void              SetAddresses(SOCKET p_socket);
  void              SetTimings();
  void              InitiateSSL();
  // Header lines
  void              ReceiveHeaders();
  void              ReadInitialMessage();
  LPSTR             ReadTextLine();
  void              ReceiveHTTPLine(LPSTR p_line);
  void              ProcessHeader(LPSTR p_line);
  void              FindContentLength();
  void              CorrectFullURL();
  int               CopyInitialBuffer(PVOID p_buffer,ULONG p_size,PULONG p_bytes);
  void              FreeInitialBuffer();

  // Cooking the URL
  void              FindVerb(LPSTR p_verb);
  void              FindURL (LPSTR p_url);
  void              FindProtocol(LPCSTR p_protocol);
  // Finding the known header names
  int               FindKnownHeader(LPSTR p_header);
  void              FindKeepAlive();
  // Authentication of the request
  bool              CheckAuthentication();
  bool              AlreadyAuthenticated       (PHTTP_REQUEST_AUTH_INFO p_info);
  bool              CheckBasicAuthentication   (PHTTP_REQUEST_AUTH_INFO p_info,CString p_payload);
  bool              CheckAuthenticationProvider(PHTTP_REQUEST_AUTH_INFO p_info,CString p_payload,CString p_provider);
  PHTTP_REQUEST_AUTH_INFO GetAuthenticationInfoRecord();
  DWORD             GetProviderMaxTokenLength(CString p_provider);
  // Sending the response
  void              DrainRequest();
  CString           HTTPSystemTime();
  void              CreateServerHeader(PHTTP_RESPONSE p_response);
  void              AddResponseLine  (CString& p_buffer,PHTTP_RESPONSE p_response);
  void              AddAllKnownResponseHeaders  (CString& p_buffer,PHTTP_KNOWN_HEADER   p_headers);
  void              AddAllUnknownResponseHeaders(CString& p_buffer,PHTTP_UNKNOWN_HEADER p_headers,int p_count);
  int               SendEntityChunk              (PHTTP_DATA_CHUNK p_chunk,PULONG p_bytes);
  int               SendEntityChunkFromMemory    (PHTTP_DATA_CHUNK p_chunk,PULONG p_bytes);
  int               SendEntityChunkFromFile      (PHTTP_DATA_CHUNK p_chunk,PULONG p_bytes);
  int               SendEntityChunkFromFragment  (PHTTP_DATA_CHUNK p_chunk,PULONG p_bytes);
  int               SendEntityChunkFromFragmentEx(PHTTP_DATA_CHUNK p_chunk,PULONG p_bytes);
  // File sending sub functions
//int               SendFileByTransmitFunction(PHTTP_DATA_CHUNK p_chunk,PULONG p_bytes);
//int               SendFileByMemoryBlocks    (PHTTP_DATA_CHUNK p_chunk,PULONG p_bytes);

  // Reading and writing
  int               ReadBuffer (PVOID p_buffer,ULONG p_size,PULONG p_bytes);
  int               WriteBuffer(PVOID p_buffer,ULONG p_size,PULONG p_bytes);

  // WebSockets
  void              CreateWebSocket();

  // Identification of the request 
  ULONGLONG         m_ident{ HTTP_REQUEST_IDENT };
  // Private data
  RequestQueue*     m_queue;          // Main request queue where we reside
  Listener*         m_listener;       // Listener that accepted our call
  // Request data
  HTTP_REQUEST_V2   m_request;        // Standard HTTP driver structure
  RQ_Status         m_status;         // Current status of the request (by our driver)
  bool              m_secure;         // HTTPS (secure) or not (HTTP)
  bool              m_handshakeDone;  // HTTPS initial handshake done for socket
  ULONG             m_bytesRead;      // Total number of bytes read so far
  ULONG             m_bytesWritten;   // Total number of bytes written so far
  SocketStream*     m_socket;         // Socket used to communicate with the client
  USHORT            m_port;           // Port the request came from
  ULONGLONG         m_contentLength;  // Content length to be read or write
  bool              m_keepAlive;      // Keep connection alive
  URL*              m_url;            // URL with longest matching absolute path
  // SSPI authentication handlers
  CString           m_challenge;      // Authentication challenge
  CtxtHandle        m_context;        // Authentication context
  clock_t           m_timestamp;      // Time of the authentication
  HANDLE            m_token;          // Primary authentication token
  // Initial buffer (Header and optional first body part) are cached here
  BYTE*             m_initialBuffer { 0 };
  ULONG             m_initialLength { 0 };
  ULONG             m_bufferPosition{ 0 };
  // WebSocket
  bool              m_websocketPrepare;
  CString           m_websocketKey;
  SYSWebSocket*     m_websocket;
};
