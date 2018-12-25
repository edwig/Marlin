//////////////////////////////////////////////////////////////////////////
//
// USER-SPACE IMPLEMENTTION OF HTTP.SYS
//
// 2018 (c) ir. W.E. Huisman
// License: MIT
//
//////////////////////////////////////////////////////////////////////////

#pragma once
#define SECURITY_WIN32
#include <sspi.h>

// For header lines
#define MESSAGE_BUFFER_LENGTH (16*1024)
// Minimum timeout for HTTP body receiving in seconds
#define HTTP_MINIMUM_TIMEOUT 10 

typedef enum _rq_status
{
  RQ_CREATED    // Object created and initialized
 ,RQ_RECEIVED   // Fully received + headers. Waiting to be serviced
 ,RQ_READING    // Server is busy reading the request
 ,RQ_ANSWERING  // Server is busy answering with a response
 ,RQ_SERVICED   // Server is ready with the request
}
RQ_Status;

class RequestQueue;
class Listener;
class SocketStream;

class Request
{
public:
  Request(RequestQueue* p_queue
         ,Listener*     p_listener
         ,SOCKET        p_socket
         ,HANDLE        p_stopEvent);
 ~Request();

  // SETTERS
  void              SetStatus(RQ_Status p_status)             { m_status = p_status; };
  void              SetURLContext(HTTP_URL_CONTEXT p_context) { m_request.UrlContext    = p_context; };
  void              SetBytesRead(ULONG p_bytes)               { m_request.BytesReceived = p_bytes;   };

  // GETTERS
  RQ_Status         GetStatus()         { return m_status;              };
  SocketStream*     GetSocket()         { return m_socket;              };
  ULONG             GetBytes()          { return m_bytesRead;           };
  PHTTP_REQUEST_V2  GetV2Request()      { return &m_request;            };
  HTTP_URL_CONTEXT  GetURLContext()     { return m_request.UrlContext;  };
  PCSTR             GetURL()            { return m_request.pRawUrl;     };
  ULONGLONG         GetContentLength()  { return m_contentLength;       };
  Listener*         GetListener()       { return m_listener;            };
  bool              GetResponseComplete();

  // FUNCTIONS
  void              ReceiveRequest();

  void              FindUrlContext();
  int               ReceiveBuffer(PVOID p_buffer,ULONG p_size,PULONG p_bytes,bool p_all);
  void              ReceiveChunk(PVOID p_buffer, ULONG p_size);
  int               SendResponse(PHTTP_RESPONSE p_response,PULONG p_bytes);
  int               SendEntityChunks(PHTTP_DATA_CHUNK p_chunks,int p_count,PULONG p_bytes);
  void              CloseRequest();
  // Reply with an error
  void              ReplyClientError();
  void              ReplyClientError(int p_error,CString p_errorText);
  void              ReplyServerError();
  void              ReplyServerError(int p_error, CString p_errorText);

private:
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
  CString           ReadTextLine();
  void              ReceiveHTTPLine(CString p_line);
  void              ProcessHeader(CString p_line);
  void              FindContentLength();
  void              CorrectFullURL();
  int               CopyInitialBuffer(PVOID p_buffer,ULONG p_size,PULONG p_bytes);
  void              FreeInitialBuffer();


  // Cooking the URL
  void              FindVerb(char* p_verb);
  void              FindURL (char* p_url);
  void              FindProtocol(char* p_protocol);
  // Finding the known header names
  int               FindKnownHeader(CString p_header);
  void              FindKeepAlive();
  // Authentication of the request
  bool              CheckAuthentication();
  bool              CheckBasicAuthentication   (PHTTP_REQUEST_AUTH_INFO p_info,CString p_payload);
  bool              CheckAuthenticationProvider(PHTTP_REQUEST_AUTH_INFO p_info,CString p_payload,CString p_provider);
  PHTTP_REQUEST_AUTH_INFO GetAuthenticationInfoRecord();
  void              DrainRequest();
  // Sending the response
  CString           HTTPSystemTime();
  void              AddResponseLine  (CString& p_buffer,PHTTP_RESPONSE p_response);
  void              AddAllKnownResponseHeaders  (CString& p_buffer,PHTTP_KNOWN_HEADER   p_headers);
  void              AddAllUnknownResponseHeaders(CString& p_buffer,PHTTP_UNKNOWN_HEADER p_headers,int p_count);
  int               SendEntityChunk              (PHTTP_DATA_CHUNK p_chunk,PULONG p_bytes);
  int               SendEntityChunkFromMemory    (PHTTP_DATA_CHUNK p_chunk,PULONG p_bytes);
  int               SendEntityChunkFromFile      (PHTTP_DATA_CHUNK p_chunk,PULONG p_bytes);
  int               SendEntityChunkFromFragment  (PHTTP_DATA_CHUNK p_chunk,PULONG p_bytes);
  int               SendEntityChunkFromFragmentEx(PHTTP_DATA_CHUNK p_chunk,PULONG p_bytes);
  // File sending sub functions
  int               SendFileByTransmitFunction(PHTTP_DATA_CHUNK p_chunk,PULONG p_bytes);
  int               SendFileByMemoryBlocks    (PHTTP_DATA_CHUNK p_chunk,PULONG p_bytes);

  // Reading and writing
  int               ReadBuffer (PVOID p_buffer,ULONG p_size,PULONG p_bytes);
  int               WriteBuffer(PVOID p_buffer,ULONG p_size);

  // Private data
  RequestQueue*     m_queue;          // Main request queue where we reside
  Listener*         m_listener;       // Listener that accepted our call
  // Request data
  HTTP_REQUEST_V2   m_request;        // Standard HTTP driver structure
  RQ_Status         m_status;         // Current status of the request (by our driver)
  bool              m_secure;         // HTTPS (secure) or not (HTTP)
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
  // Initial buffer (Header and optional first body part) are cached here
  char*             m_initialBuffer { nullptr };
  ULONG             m_initialLength { 0 };
  ULONG             m_bufferPosition{ 0 };
};
