/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: HTTPRequest.h
//
// Marlin Server: Internet server/client
// 
// Copyright (c) 2014-2024 ir. W.E. Huisman
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
#include "HTTPServer.h"

class HTTPMessage;
class HTTPSite;
class HTTPRequest;
class WebSocketServer;

// Event action type for asynchronous I/O
typedef enum _ioaction
{
  IO_Nothing     = 0    // Nothing done yet
 ,IO_Request     = 1    // Receiving a new request
 ,IO_Reading     = 2    // Receiving a request body part
 ,IO_Response    = 3    // Did send a response
 ,IO_Writing     = 4    // Did write a response body part
 ,IO_StartStream = 5    // Did start an event stream
 ,IO_WriteStream = 6    // Did write a stream part
 ,IO_Cancel      = 9    // Cancel current request

}
IOAction;

// Overlay for async I/O registration
// The overlay is derived from OVERLAPPED, thusly making it possible to 
// pass a pointer to this object to the I/O function calls
class OutstandingIO : public OVERLAPPED
{
public:
  explicit OutstandingIO(HTTPRequest* p_request);
  HTTPRequest* m_request;
  IOAction     m_action;
};

// Callback from I/O Completion port right out of the threadpool
// USE this function as the completion key after registering the 
// httpRequestQueue in the server.
void HandleAsynchroneousIO(OVERLAPPED* p_overlapped);

// Strings for headers must be tied to the request, otherwise they do not
// survive for the asynchronous I/O commands
// They must always be in ANSI/MBCS format!
using RequestStrings = std::vector<LPCSTR>;

#define HTTPREQUEST_IDENT 0x66FACE66

// Our outstanding request in the server
class HTTPRequest
{
public:
  explicit HTTPRequest(HTTPServer* p_server);
 ~HTTPRequest();
 
  // Start a new request against the server
  void StartRequest();
  // Start the response handling
  void StartResponse(HTTPMessage* p_message);
  // Callback from I/O Completion port
  void HandleAsynchroneousIO(IOAction p_action);
  // Cancel the request at the HTTP driver
  void CancelRequestStream();
  // Start a response stream
  void StartEventStreamResponse();
  // Send as a stream part to an existing stream
  bool SendResponseStream(BYTE*   p_buffer
                         ,size_t  p_length
                         ,bool    p_continue = true);

  // GETTERS

  // Object still has outstanding I/O to handle
  bool              GetIsActive()   { return m_active;    }
  // The HTTPServer that handles this request
  HTTPServer*       GetHTTPServer() { return m_server;    }
  // The connected WebSocket
  WebSocketServer*  GetWebSocket()  { return m_socket;    }
  // OPAQUE Request
  HTTP_OPAQUE_ID    GetRequest()    { return m_requestID; }
  // OPAQUE Response
  PHTTP_RESPONSE    GetResponse()   { return m_response;  }
  // Getting the long term status
  bool              GetLongTerm()   { return m_longTerm;  }

  // SETTERS
  void SetChunkEvent(HANDLE p_event){ m_chunkEvent = p_event; }

  // Identity for callbacks
  ULONG m_ident { HTTPREQUEST_IDENT };
private:
  // Ready with the response
  void Finalize();
  // Clear local memory structures
  void ClearMemory();
  // Startup of other parts of the request
  void StartReceiveRequest();
  void StartSendResponse();
  // Detailed handlers of HandleAsynchroneousIO in phases
  void ReceivedRequest();          // 1) Receive general request headers and such
  void ReceivedBodyPart();         // 2) Receive trailing request body
  void SendResponseBody();         // 3) Start sending body parts
  void SendBodyPart();             // 4) Has send a body part
  void StartedStream();            // 5) Has started event stream
  void SendStreamPart();           // 6) Has send a stream part

  // Sub procedures for the handlers

  // We have read the whole body of a message
  void PostReceive();
  // Add a well known HTTP header to the response structure
  void AddKnownHeader(HTTP_HEADER_ID p_header,LPCTSTR p_value);
  // Add previously unknown HTTP headers
  void AddUnknownHeaders(UKHeaders& p_headers);
  // Fill response structure out of the HTTPMessage
  void FillResponse(int p_status,bool p_responseOnly = false);
  void FillResponseWebSocketHeaders(UKHeaders& p_headers);
  // Reset outstanding OVERLAPPED
  void ResetOutstanding(OutstandingIO& p_outstanding);
  // Add a request string for a header
  void AddRequestString(XString p_string,LPSTR& p_buffer,USHORT& p_size);
  // Create the logging data
  void CreateLogData();

  HTTPServer*       m_server     { nullptr };   // Our server
  bool              m_active     { false   };   // Authentication done: may receive
  HTTP_OPAQUE_ID    m_requestID  { NULL    };   // The request we are processing
  PHTTP_REQUEST     m_request    { nullptr };   // Pointer to the request  object
  PHTTP_RESPONSE    m_response   { nullptr };   // Pointer to the response object
  HTTPSite*         m_site       { nullptr };   // Site from the HTTP context
  HTTPMessage*      m_message    { nullptr };   // The message we are processing in the request
  WebSocketServer*  m_socket     { nullptr };   // WebSocket (if any)
  HTTP_CACHE_POLICY m_policy     { HttpCachePolicyNocache,0 };   // Sending cache policy
  long              m_expect     { 0       };   // Expected content length
  OutstandingIO     m_incoming;                 // Incoming IO request
  OutstandingIO     m_reading;                  // Outstanding reading action
  OutstandingIO     m_writing;                  // Outstanding writing action
  bool              m_responding { false   };   // Response already started
  int               m_logLevel   { HLL_NOLOG }; // Logging level of the server
  BYTE*             m_readBuffer { nullptr };   // Read data buffer
  BYTE*             m_sendBuffer { nullptr };   // Send data buffer
  HTTP_DATA_CHUNK   m_sendChunk;                // Send buffer as a chunked info
  RequestStrings    m_strings;                  // Strings for headers and such
  HANDLE            m_file       { NULL    };   // File handle for sending a file
  HANDLE            m_chunkEvent { NULL    };   // Event for first chunk
  int               m_bufferpart { 0       };   // Buffer part being sent
  PHTTP_UNKNOWN_HEADER m_unknown { nullptr };   // Send unknown headers
  CString           m_originalVerb;             // Verb before the send
  bool              m_longTerm   { false   };   // Long term connection (SSE or WebSocket)
  PHTTP_LOG_DATA    m_logData    { nullptr };   // Data to log for this request (in last send!)
  CRITICAL_SECTION  m_critical;                 // Locking section
};
