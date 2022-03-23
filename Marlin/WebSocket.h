/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: WebSocket.h
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
#include "HTTPClient.h"
#include "LogAnalysis.h"
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
constexpr DWORD WS_OVERHEAD          = 4;

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
  DWORD       m_read    { 0       };    // Last read block by server
};

// Incoming stacks of data
using FragmentStack = std::deque<RawFrame*>;
using WSFrameStack  = std::deque<WSFrame*>;
using SocketParams  = std::map<XString,XString>;

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
  WebSocket(XString p_uri);
  virtual ~WebSocket();

  // FUNCTIONS

  // Reset the socket
  virtual void Reset();
  // Open the socket
  virtual bool OpenSocket() = 0;
  // Close the socket unconditionally
  virtual bool CloseSocket() = 0;
  // Close the socket with a closing frame
  virtual bool SendCloseSocket(USHORT p_code,XString p_reason) = 0;
  // Write fragment to a WebSocket
  virtual bool WriteFragment(BYTE* p_buffer,DWORD p_length,Opcode p_opcode,bool p_last = true) = 0;
  // Register the server request for sending info
  virtual bool RegisterSocket(HTTPMessage* p_message) = 0;

  // Decoded close connection (use in 'OnClose')
  bool GetCloseSocket(USHORT& p_code,XString& p_reason);
  // Socket still open?
  bool IsOpenForReading();
  bool IsOpenForWriting();
  // Close the reading side (status only, does not send a 'close')
  void CloseForReading();
  // Close the writing side (status only, does not send a 'close')
  void CloseForWriting();

  // HIGH LEVEL INTERFACE

  // Write as an UTF-8 string to the WebSocket
  bool WriteString(XString p_string);
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
  void SetProtocols(XString p_protocols);
  // Setting the extensions
  void SetExtensions(XString p_extensions);
  // Setting the logfile
  void SetLogfile(LogAnalysis* p_logfile);
  // Set logging to on or off
  void SetLogLevel(int p_logLevel);
  // Setting the application completion port
  void SetApplication(void* p_application);
  // Setting the application context data
  void SetApplicationData(UINT64 p_data);
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
  XString GetURI()            { return m_uri;          }
  XString GetIdentityKey()    { return m_key;          }
  ULONG   GetKeepalive()      { return m_keepalive;    }
  ULONG   GetFragmentSize()   { return m_fragmentsize; }
  XString GetProtocols()      { return m_protocols;    }
  XString GetExtensions()     { return m_extensions;   }
  USHORT  GetClosingError()   { return m_closingError; }
  XString GetClosingMessage() { return m_closing;      }
  int     GetLogLevel()       { return m_logLevel;     }
  LogAnalysis* GetLogfile()   { return m_logfile;      }
  void*   GetApplication()    { return m_application;  }
  UINT64  GetApplicationData(){ return m_appData;      }
  XString GetClosingErrorAsString();

  // Add a URI parameter
  void    AddParameter(XString p_name,XString p_value);
  // Add a HTTP header
  void    AddHeader(XString p_name,XString p_value);
  // Find a URI parameter
  XString GetParameter(XString p_name);

  // LOGGING
  void    DetailLog (const char* p_function,LogType p_type,const char* p_text);
  void    DetailLogS(const char* p_function,LogType p_type,const char* p_text,const char* p_extra);
  void    DetailLogV(const char* p_function,LogType p_type,const char* p_text,...);
  void    ErrorLog  (const char* p_function,DWORD p_code,XString p_text);

  // Perform the server handshake
  virtual bool ServerHandshake(HTTPMessage* p_message);
  // Generate a server key-answer
  XString   ServerAcceptKey(XString p_clientKey);

protected:
  // Completely close the connection
  void    Close();
  // Store an incoming WSframe 
  void    StoreWSFrame(WSFrame*& p_frame);
  // Get the first frame
  WSFrame* GetWSFrame();
  // Convert the UTF-8 in a frame back to MBCS
  void    ConvertWSFrameToMBCS(WSFrame* p_frame);

  // GENERAL SOCKET DATA
  XString m_uri;                      // ws[s]://resource URI for the socket
  XString m_key;                      // Essential the accept-key that registers the socket
  bool    m_openReading { false };    // WebSocket is opened and alive for reading
  bool    m_openWriting { false };    // WebSocket is opened and alive for writing
  ULONG   m_keepalive;                // Keep alive time of the socket
  ULONG   m_closingTimeout;           // Timeout for answering 'close' message
  ULONG   m_fragmentsize;             // Max fragment size
  XString m_protocols;                // WebSocket main protocols
  XString m_extensions;               // Extensions in 1,2,3 fields
  USHORT  m_closingError{ 0     };    // Error on closing
  XString m_closing;                  // Closing error text
  ULONG   m_pingTimeout { 30000 };    // How long we wait for a pong after a ping
  bool    m_pongSeen    { false };    // Seen a pong for a ping
  int     m_logLevel  {HLL_NOLOG};    // Logging level for the WebSocket
  DWORD   m_messageNumber { 0   };    // Last message number
  // Complex objects
  FragmentStack m_stack;              // Incoming raw fragments (stand-alone)
  WSFrameStack  m_frames;             // Incoming WS  fragments (IIS, WinHTTP)
  LogAnalysis*  m_logfile {nullptr};  // Connected to this logfile
  SocketParams  m_parameters;         // All URL parameters
  SocketParams  m_headers;            // All HTTP headers
  // Application completion port
  void*              m_application  { nullptr };
  UINT64             m_appData      { 0L      };
  // Handlers
  LPFN_SOCKETHANDLER m_onopen       { nullptr }; // OnOpen    handler
  LPFN_SOCKETHANDLER m_onmessage    { nullptr }; // OnMessage handler
  LPFN_SOCKETHANDLER m_onbinary     { nullptr }; // OnBinary  handler
  LPFN_SOCKETHANDLER m_onerror      { nullptr }; // OnError   handler
  LPFN_SOCKETHANDLER m_onclose      { nullptr }; // OnClose   handler
  // Current frame for reading & writing
  WSFrame* m_reading { nullptr };
  // Synchronization for the fragment stack
  CRITICAL_SECTION   m_lock;
  CRITICAL_SECTION   m_disp;
};

inline bool
WebSocket::IsOpenForReading()
{
  return m_openReading;
}

inline bool
WebSocket::IsOpenForWriting()
{
  return m_openWriting;
}

inline void 
WebSocket::CloseForReading()
{
  m_openReading = false;
}

inline void 
WebSocket::CloseForWriting()
{
  m_openWriting = false;
}

inline void 
WebSocket::SetProtocols(XString p_protocols)
{
  m_protocols = p_protocols;
}

inline void 
WebSocket::SetExtensions(XString p_extensions)
{
  m_extensions = p_extensions;
}

inline void 
WebSocket::SetLogfile(LogAnalysis* p_logfile)
{
  m_logfile = p_logfile;
}

inline void
WebSocket::SetLogLevel(int p_logLevel)
{
  m_logLevel = p_logLevel;
}

inline void 
WebSocket::SetApplication(void* p_application)
{
  m_application = p_application;
}

inline void 
WebSocket::SetApplicationData(UINT64 p_data)
{
  m_appData = p_data;
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
