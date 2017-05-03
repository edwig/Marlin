/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: WebSocket.h
//
// Marlin Server: Internet server/client
// 
// Copyright (c) 2017 ir. W.E. Huisman
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
#include "HTTPClient.h"
#include "Analysis.h"
#include <deque>
#include <map>

// Header and fragment sizes
constexpr auto WS_MIN_HEADER        =  2;  // Minimum header size in octets
constexpr auto WS_MAX_HEADER        = 14;  // Maximum header size in octets
constexpr auto WS_FRAGMENT_MINIMUM  = (  1 * 4096) - WS_MAX_HEADER;  //  1 TCP/IP buffer
constexpr auto WS_FRAGMENT_DEFAULT  = (  2 * 4096) - WS_MAX_HEADER;  //  8 KB buffer
constexpr auto WS_FRAGMENT_MAXIMUM  = (256 * 4096) - WS_MAX_HEADER;  //  1 MB buffer
// Default keep alive time for a 'pong'
constexpr auto WS_KEEPALIVE_TIME    =  30000;      // 30 sec = 20000 milliseconds
constexpr auto WS_KEEPALIVE_MINIMUM =  15000;
constexpr auto WS_KEEPALIVE_MAXIMUM = 120000;
// Maximum length of a WebSocket 'close' message
constexpr auto WS_CLOSE_MAXIMUM     = 123;
// Timeout for a 'close' message
constexpr auto WS_CLOSING_TIME      =  10000;
constexpr auto WS_CLOSING_MINIMUM   =   2000;
constexpr auto WS_CLOSING_MAXIMUM   = 120000;
// Buffer overhead, for safety allocation
constexpr auto WS_OVERHEAD          = 4;

// Forward declaration of our class
class WebSocket;
class HTTPServer;
class HTTPMessage;
class IWebSocketContext;

enum class Opcode
{
  SO_CONTINU = 0    // Continuation frame of one of the other types
 ,SO_UTF8    = 1    // Payload is UTF-8 data
 ,SO_BINARY  = 2    // Payload is binary data
 ,SO_EXT3    = 3    // RESERVED for future use: PAYLOAD
 ,SO_EXT4    = 4    // RESERVED for future use: PAYLOAD
 ,SO_EXT5    = 5    // RESERVED for future use: PAYLOAD
 ,SO_EXT6    = 6    // RESERVED for future use: PAYLOAD
 ,SO_EXT7    = 7    // RESERVED for future use: PAYLOAD
 ,SO_CLOSE   = 8    // Close the socket frame
 ,SO_PING    = 9    // Ping the other side
 ,SO_PONG    = 10   // Pong on a ping, or a 'keepalive'
 ,SO_CTRL1   = 11   // RESERVED for future use: CONTROL frame
 ,SO_CTRL2   = 12   // RESERVED for future use: CONTROL frame
 ,SO_CTRL3   = 13   // RESERVED for future use: CONTROL frame
 ,SO_CTRL4   = 14   // RESERVED for future use: CONTROL frame
 ,SO_CTRL5   = 15   // RESERVED for future use: CONTROL frame
};

// Define OnClose status codes
#define WS_CLOSE_NORMAL       1000      // Normal closing of the connection
#define WS_CLOSE_GOINGAWAY    1001      // Going away. Closing webpage etc.
#define WS_CLOSE_BYERROR      1002      // Closing due to protocol error
#define WS_CLOSE_TERMINATE    1003      // Terminating (unacceptable data)
#define WS_CLOSE_RESERVED     1004      // Reserved for future use
#define WS_CLOSE_NOCLOSE      1005      // Internal error (no closing frame received)
#define WS_CLOSE_ABNORMAL     1006      // No closing frame, tcp/ip error?
#define WS_CLOSE_DATA         1007      // Abnormal data (no UTF-8 in UTF-8 frame)
#define WS_CLOSE_POLICY       1008      // Policy error, or extension error
#define WS_CLOSE_TOOBIG       1009      // Message is too big to handle
#define WS_CLOSE_NOEXTENSION  1010      // Not one of the expected extensions
#define WS_CLOSE_CONDITION    1011      // Internal server error
#define WS_CLOSE_SECURE       1015      // TLS handshake error (do not send!)

// OnClose status codes ranges
#define WS_CLOSE_MAX_PROTOCOL 2999      // 1000 - 2999 Reserved for WebSocket protocol
#define WS_CLOSE_MAX_IANA     3999      // 3000 - 3999 Reserved for IANA registration
#define WS_CLOSE_MAX_PRIVATE  4999      // 4000 - 4999 Usable by WebSocket applications

using int64 = __int64;

// RawFrames are used for the stand-alone Marlin server
class RawFrame
{
public:
  RawFrame();
 ~RawFrame();

  bool    m_internal;       // For internal use or for the wire
  bool    m_isRead;         // Each fragment can be read once!
  // HEADER CONTENTS
  bool    m_finalFrame;     // true if final frame
  bool    m_reserved1;      // Extended protocol 1
  bool    m_reserved2;      // Extended protocol 2
  bool    m_reserved3;      // Extended protocol 3
  Opcode  m_opcode;         // Opcode of the frame
  bool    m_masked;         // Payload is masked
  int64   m_payloadLength;  // Length of the payload
  // HEADER END
  int     m_headerLength;   // Length of the header
  BYTE*   m_data;           // Pointer to the raw data (header + payload)
};

// WSFrames are the simplified frames for WinHTTP and IIS
class WSFrame
{
public:
  WSFrame();
 ~WSFrame();

  bool        m_final   { false   };    // True if final frame for message
  bool        m_utf8    { false   };    // UTF-8 Frame or Binary frame
  BYTE*       m_data    { nullptr };    // Data block pointer
  DWORD       m_length  { 0       };    // Length of the data block
};

// Incoming stacks of data
using FragmentStack = std::deque<RawFrame*>;
using WSFrameStack  = std::deque<WSFrame*>;
using SocketParams  = std::map<CString,CString>;

// Prototype for WebSocket callback handler
typedef void(*LPFN_SOCKETHANDLER)(WebSocket* p_socket,WSFrame* p_event);

//////////////////////////////////////////////////////////////////////////
//
// The WebSocket class
//
//////////////////////////////////////////////////////////////////////////

class WebSocket
{
public:
  WebSocket(CString p_uri);
  virtual ~WebSocket();

  // FUNCTIONS

  // Reset the socket
  virtual void Reset();
  // Open the socket
  virtual bool OpenSocket() = 0;
  // Close the socket unconditionally
  virtual bool CloseSocket() = 0;
  // Close the socket with a closing frame
  virtual bool SendCloseSocket(USHORT p_code,CString p_reason) = 0;
  // Write fragment to a WebSocket
  virtual bool WriteFragment(BYTE* p_buffer,DWORD p_length,Opcode p_opcode,bool p_last = true) = 0;
  // Register the server request for sending info
  virtual bool RegisterServerRequest(HTTPServer* p_server,HTTP_REQUEST_ID p_request) = 0;
  // Decoded close connection (use in 'OnClose')
  bool GetCloseSocket(USHORT& p_code,CString& p_reason);
  // Socket still open?
  bool IsOpen();

  // HIGH LEVEL INTERFACE

  // Write as an UTF-8 string to the WebSocket
  bool WriteString(CString p_string);
  // Write as a binary object to the channel
  bool WriteObject(BYTE* p_buffer,int64 p_length);

  // DEFAULT HANDLERS
  void OnOpen();      // Called when WebSocket opens
  void OnMessage();   // Called on an incoming UTF-8 string (on last frame only!)
  void OnBinary();    // Called on an incoming binary frame buffer  (all frames!)
  void OnError();     // Called on error
  void OnClose();     // Called when closing the socket

  // SETTERS

  // Setting the keep-alive interval for sending a 'pong'
  void SetKeepalive(unsigned p_miliseconds);
  // Setting the timeout for a 'close' message
  void SetClosingTimeout(unsigned p_timeout);
  // Max fragmentation size
  void SetFragmentSize(ULONG p_fragment);
  // Setting the protocols
  void SetProtocols(CString p_protocols);
  // Setting the extensions
  void SetExtensions(CString p_extensions);
  // Setting the logfile
  void SetLogfile(LogAnalysis* p_logfile);
  // Set logging to on or off
  void SetLogging(bool p_logging);
  // Set the OnOpen handler
  void SetOnOpen   (LPFN_SOCKETHANDLER p_onOpen);
  // Set the OnMessage handler
  void SetOnMessage(LPFN_SOCKETHANDLER p_onMessage);
  // Set the OnBinary handler
  void SetOnBinary (LPFN_SOCKETHANDLER p_onBinary);
  // Set the OnError handler
  void SetOnError  (LPFN_SOCKETHANDLER p_onError);
  // Set the OnClose handler
  void SetOnClose  (LPFN_SOCKETHANDLER p_onClose);

  // GETTERS

  // Getting the URI
  CString GetURI()            { return m_uri;          };
  ULONG   GetKeepalive()      { return m_keepalive;    };
  ULONG   GetFragmentSize()   { return m_fragmentsize; };
  CString GetProtocols()      { return m_protocols;    };
  CString GetExtensions()     { return m_extensions;   };
  USHORT  GetClosingError()   { return m_closingError; };
  CString GetClosingMessage() { return m_closing;      };
  bool    GetDoLogging()      { return m_doLogging;    };
  LogAnalysis* GetLogfile()   { return m_logfile;      };

  // Add a URI parameter
  void    AddParameter(CString p_name,CString p_value);
  // Find a URI parameter
  CString GetParameter(CString p_name);

  // LOGGING
  void    DetailLog (const char* p_function,LogType p_type,const char* p_text);
  void    DetailLogS(const char* p_function,LogType p_type,const char* p_text,const char* p_extra);
  void    DetailLogV(const char* p_function,LogType p_type,const char* p_text,...);
  void    ErrorLog  (const char* p_function,DWORD p_code,CString p_text);

  // Perform the server handshake
  virtual bool ServerHandshake(HTTPMessage* p_message);

protected:
  // Completely close the connection
  void    Close();
  // Store an incoming WSframe 
  void    StoreWSFrame(WSFrame*& p_frame);
  // Get the first frame
  WSFrame* GetWSFrame();
  // Convert the UTF-8 in a frame back to MBCS
  void    ConvertWSFrameToMBCS(WSFrame* p_frame);
  // Append UTF-8 text to last frame on the stack, or just store it
  void    StoreOrAppendWSFrame(WSFrame*& p_frame);

  // GENERAL SOCKET DATA
  CString m_uri;                      // ws[s]://resource URI for the socket
  bool    m_open        { false };    // WebSocket is opened and alive
  ULONG   m_keepalive;                // Keep alive time of the socket
  ULONG   m_closingTimeout;           // Timeout for answering 'close' message
  ULONG   m_fragmentsize;             // Max fragment size
  CString m_protocols;                // WebSocket main protocols
  CString m_extensions;               // Extensions in 1,2,3 fields
  USHORT  m_closingError{ 0     };    // Error on closing
  CString m_closing;                  // Closing error text
  ULONG   m_pingTimeout { 30000 };    // How long we wait for a pong after a ping
  bool    m_pongSeen    { false };    // Seen a pong for a ping
  bool    m_doLogging   { false };    // Do we do the logging?
  DWORD   m_messageNumber { 0   };    // Last message number
  // Complex objects
  FragmentStack m_stack;              // Incoming raw fragments (stand-alone)
  WSFrameStack  m_frames;             // Incoming WS  fragments (IIS, WinHTTP)
  LogAnalysis*  m_logfile {nullptr};  // Connected to this logfile
  SocketParams  m_parameters;         // All parameters
  // Handlers
  LPFN_SOCKETHANDLER m_onopen    { nullptr }; // OnOpen    handler
  LPFN_SOCKETHANDLER m_onmessage { nullptr }; // OnMessage handler
  LPFN_SOCKETHANDLER m_onbinary  { nullptr }; // OnBinary  handler
  LPFN_SOCKETHANDLER m_onerror   { nullptr }; // OnError   handler
  LPFN_SOCKETHANDLER m_onclose   { nullptr }; // OnClose   handler
  // Current frame for reading & writing
  WSFrame* m_reading { nullptr };
  // Synchronization for the fragment stack
  CRITICAL_SECTION   m_lock;
  CRITICAL_SECTION   m_disp;
};

inline bool
WebSocket::IsOpen()
{
  return m_open;
}

inline void 
WebSocket::SetProtocols(CString p_protocols)
{
  m_protocols = p_protocols;
}

inline void 
WebSocket::SetExtensions(CString p_extensions)
{
  m_extensions = p_extensions;
}

inline void 
WebSocket::SetLogfile(LogAnalysis* p_logfile)
{
  m_logfile = p_logfile;
}

inline void
WebSocket::SetLogging(bool p_logging)
{
  m_doLogging = p_logging;
}

inline void 
WebSocket::SetOnOpen(LPFN_SOCKETHANDLER p_onOpen)
{
  m_onopen = p_onOpen;
}

inline void 
WebSocket::SetOnMessage(LPFN_SOCKETHANDLER p_onMessage)
{
  m_onmessage = p_onMessage;
}

inline void 
WebSocket::SetOnBinary(LPFN_SOCKETHANDLER p_onBinary)
{
  m_onbinary = p_onBinary;
}

inline void 
WebSocket::SetOnError(LPFN_SOCKETHANDLER p_onError)
{
  m_onerror = p_onError;
}

inline void 
WebSocket::SetOnClose(LPFN_SOCKETHANDLER p_onClose)
{
  m_onclose = p_onClose;
}

//////////////////////////////////////////////////////////////////////////
//
// SERVER MARLIN WebSocket
//
//////////////////////////////////////////////////////////////////////////

class WebSocketServer : public WebSocket
{
public:
  WebSocketServer(CString p_uri);
  virtual ~WebSocketServer();

  // Reset the socket
  virtual void Reset();
  // Open the socket
  virtual bool OpenSocket();
  // Close the socket unconditionally
  virtual bool CloseSocket();
  // Close the socket with a closing frame
  virtual bool SendCloseSocket(USHORT p_code,CString p_reason);
  // Write fragment to a WebSocket
  virtual bool WriteFragment(BYTE* p_buffer,DWORD p_length,Opcode p_opcode,bool p_last = true);
  // Register the server request for sending info
  virtual bool RegisterServerRequest(HTTPServer* p_server,HTTP_REQUEST_ID p_request);
  // Read a fragment from a WebSocket
  bool    ReadFragment(BYTE*& p_buffer,int64& p_length,Opcode& p_opcode,bool& p_last);

  // Send a 'ping' to see if other side still there
  bool    SendPing();
  // Send a 'pong' as an answer on a 'ping'
  void    SendPong(RawFrame* p_ping);

  // BOTH HTTPClient and HTTPServer must have access to these functions

  // Perform the server handshake
  virtual bool ServerHandshake(HTTPMessage* p_message);
  // Generate a server key-answer
  CString ServerAcceptKey(CString p_clientKey);
  // Encode raw frame buffer
  bool    EncodeFramebuffer(RawFrame* p_frame,Opcode p_opcode,bool p_mask,BYTE* p_buffer,int64 p_length,bool p_last);
  // Decode raw frame buffer (only m_data is filled)
  bool    DecodeFrameBuffer(RawFrame* p_frame,int64 p_length);
  // Store incoming raw frame buffer
  bool    StoreFrameBuffer(RawFrame* p_frame);
  // Async starting a socket listener on the HTTPServer
  void    StartSocket();

private:
  // Decode the incoming closing fragment before we call 'OnClose'
  void    DecodeCloseFragment(RawFrame* p_frame);

  // Private data for the server variant of the WebSocket
  HTTPServer*     m_server  { nullptr };
  HTTP_REQUEST_ID m_request { NULL    };
};

//////////////////////////////////////////////////////////////////////////
//
// SERVER IIS WebSocket
//
//////////////////////////////////////////////////////////////////////////

class WebSocketServerIIS : public WebSocket
{
public:
  WebSocketServerIIS(CString p_uri);
  virtual ~WebSocketServerIIS();

  // FUNCTIONS

  // Reset the socket
  virtual void Reset();
  // Open the socket
  virtual bool OpenSocket();
  // Close the socket unconditionally
  virtual bool CloseSocket();
  // Close the socket with a closing frame
  virtual bool SendCloseSocket(USHORT p_code,CString p_reason);
  // Write fragment to a WebSocket
  virtual bool WriteFragment(BYTE* p_buffer,DWORD p_length,Opcode p_opcode,bool p_last = true);
  // Register the server request for sending info
  virtual bool RegisterServerRequest(HTTPServer* p_server,HTTP_REQUEST_ID p_request);
  // To be called for ASYNC I/O completion!
  void    SocketReader(HRESULT p_error,DWORD p_bytes,BOOL p_utf8,BOOL p_final,BOOL p_close);
  void    SocketWriter(HRESULT p_error,DWORD p_bytes,BOOL p_utf8,BOOL p_final,BOOL p_close);
  // Socket listener, entered by the HTTPServerIIS only!!
  void    SocketListener();

  // Perform the server handshake
  virtual bool ServerHandshake(HTTPMessage* p_message);

protected:
  // Decode the incoming close socket message
  bool    ReceiveCloseSocket();

  // Private data for the IIS WebSocket variant
  HTTPServer*         m_server    { nullptr };
  IWebSocketContext*  m_iis_socket{ nullptr };
  HTTP_REQUEST_ID     m_request   { NULL    };
  HANDLE              m_listener  { NULL    };
  // Async write buffer
  WSFrameStack        m_writing;
};

//////////////////////////////////////////////////////////////////////////
//
// CLIENT WebSocket
//
//////////////////////////////////////////////////////////////////////////

class WebSocketClient: public WebSocket
{
public:
  WebSocketClient(CString p_uri);
  virtual ~WebSocketClient();

  // FUNCTIONS

  // Reset the socket
  virtual void Reset();
  // Open the socket
  virtual bool OpenSocket();
  // Close the socket
  virtual bool CloseSocket();
  // Close the socket with a closing frame
  virtual bool SendCloseSocket(USHORT p_code,CString p_reason);
  // Register the server request for sending info
  virtual bool RegisterServerRequest(HTTPServer* p_server,HTTP_REQUEST_ID p_request);
  // Write fragment to a WebSocket
  virtual bool WriteFragment(BYTE* p_buffer,DWORD p_length,Opcode p_opcode,bool p_last = true);
  // Generate a handshake key
  CString      GenerateKey();
  // Socket listener, entered by the StartClientListener only!
  void         SocketListener();

  // GETTERS

  // The handshake key the socket was started with
  CString      GetHandshakeKey();

protected:
  // Start listener thread for the client WebSocket
  bool         StartClientListner();
  // Decode the incoming close socket message
  bool         ReceiveCloseSocket();

  // WinHTTP Client version of the WebSocket
  HINTERNET    m_socket   { NULL };   // Our socket handle for WinHTTP
  HANDLE       m_listener { NULL };   // Listener thread
  CString      m_socketKey;           // Given at the start
};

inline CString
WebSocketClient::GetHandshakeKey()
{
  return m_socketKey;
}

// Register the server request for sending info
inline bool 
WebSocketClient::RegisterServerRequest(HTTPServer* /*p_server*/,HTTP_REQUEST_ID /*p_request*/)
{
  // NO-OP for the client side
  return true;
}
