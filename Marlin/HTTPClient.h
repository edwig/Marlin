/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: HTTPClient.h
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

//////////////////////////////////////////////////////////////////////////
//
//  HTTPClient
//
//////////////////////////////////////////////////////////////////////////

#pragma  once
#include "MarlinConfig.h"
#include "Headers.h"
#include "HPFCounter.h"
#include "SOAPTypes.h"
#include "Cookie.h"
#include "FindProxy.h"
#include "HTTPMessage.h"
#include "HTTPLoglevel.h"
#include <winhttp.h>
#include <vector>
#include <string>
#include <list>

// Using for conversion and calling API's
using std::wstring;

// Maximum number of sending retries
constexpr auto CLIENT_MAX_RETRIES = 3;
// Initial 32 KB temporary buffer. 
// Do not go under 8K otherwise network traffic efficiency will decrease
// Must be the same buffer size as the HTTPServer (in essence)
constexpr auto INT_BUFFERSIZE     =  32 * 1024;
// The amount of time the queue thread will linger (10 seconds)
constexpr auto QUEUE_WAITTIME     = 10000;
// The amount of interval time on closing polling
constexpr auto CLOSING_WAITTIME   = 100;
// The amount of polling intervals we wait on closing
constexpr auto CLOSING_INTERVALS  = 300;

// Partly copied from <wininet.h>
#ifndef INTERNET_ERROR_BASE
#define INTERNET_ERROR_BASE         12000
#define ERROR_INTERNET_FORCE_RETRY  (INTERNET_ERROR_BASE + 32)
#endif

// All secure protocols the HTTPClient accepts.
// Differs from WINHTTP_FLAG_SECURE_PROTOCOL_ALL in <winhttp>
// as that it does not allow the older insecure SSL2 protocol
#define WINHTTP_FLAG_SECURE_PROTOCOL_MARLIN  (WINHTTP_FLAG_SECURE_PROTOCOL_SSL3   |\
                                              WINHTTP_FLAG_SECURE_PROTOCOL_TLS1   |\
                                              WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_1 |\
                                              WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2 )
// Default timeout values
constexpr auto DEF_TIMEOUT_RESOLVE  =  5000;
constexpr auto DEF_TIMEOUT_CONNECT  =  6000;
constexpr auto DEF_TIMEOUT_SEND     = 20000;
constexpr auto DEF_TIMEOUT_RECEIVE  = 20000;
// Minimum timeout values
constexpr auto MIN_TIMEOUT_RESOLVE  =   500;
constexpr auto MIN_TIMEOUT_CONNECT  =   500;
constexpr auto MIN_TIMEOUT_SEND     =  3000;
constexpr auto MIN_TIMEOUT_RECEIVE  =  5000;

// Forward declarations
class SOAPMessage;
class FileBuffer;
class LogAnalysis;
class EventSource;
class ThreadPool;
class OAuth2Cache;
class HTTPClientTracing;

// Types of proxies supported
enum class ProxyType
{
  PROXY_IEPROXY   = 1   // Use IE autoproxy settings in the connect
 ,PROXY_AUTOPROXY       // Use IE autoproxy if connection fails (handle default)
 ,PROXY_MYPROXY         // Use MY proxy settings from Marlin.config
 ,PROXY_NOPROXY         // Never use any proxy
};

// Enumeration of three messages in the client queue
enum class MsgType
{
   HTPC_HTTP  = 1 // HTTPMessage
  ,HTPC_SOAP      // SOAPMessage
  ,HTPC_JSON      // JSONMessage
};

// Message buffer for the queue
// can contain any of these three messages
class MsgBuf
{
public:
  MsgType m_type;
  union _message
  {
    HTTPMessage* m_httpMessage;
    SOAPMessage* m_soapMessage;
    JSONMessage* m_jsonMessage;
  }
  m_message;
};

// Used mappings in this class
using HttpQueue   = std::list<MsgBuf>;

class HTTPClient
{
public:
  HTTPClient();
 ~HTTPClient();
  // Reset the client to 'sane' values
  void Reset();
  // Pre-Initialize the client
  bool Initialize();

  // Our primary function: send something
  bool Send();
  // Send HTTP to an URL
  bool Send(const XString& p_url);
  // Send HTTP + body to an URL
  bool Send(XString& p_url,XString& p_body);
  // Send HTTP + body-buffer to an URL
  bool Send(const XString& p_url,const void* p_buffer,unsigned p_length);
  // Send HTTPMessage
  bool Send(HTTPMessage* p_msg);
  // Send HTTP + SOAP Message
  bool Send(SOAPMessage* p_msg);
  // Send HTTP + JSON Message
  bool Send(JSONMessage* p_msg);
  // Send to an URL to GET a file in a buffer
  bool Send(const XString&    p_url
           ,      FileBuffer* p_buffer
           ,const XString*    p_contentType = nullptr
           ,const Cookies*    p_cookies     = nullptr
           ,const HeaderMap*  p_headers     = nullptr);
  // Translate SOAP to JSON, send/receive and translate back
  bool SendAsJSON(SOAPMessage* p_msg);
  // Send and redirect
  bool SendAndRedirect();

  // Add an HTTP message to the async queue
  void AddToQueue(HTTPMessage* p_entry);
  // Add a SOAP message to the async queue
  void AddToQueue(SOAPMessage* p_message);
  // Add a JSON message to the async queue
  void AddToQueue(JSONMessage* p_message);
  // Stopping the client queue
  void StopClient();
  // See if it is still running
  bool GetIsRunning();

  // FUNCTIONS
  // Add extra header for the call
  bool AddHeader(XString p_header);
  // Add extra header by name and value pair
  void AddHeader(XString p_name,XString p_value);
  // Delete a header
  bool DelHeader(XString p_name);
  // Add extra cookie for the call
  bool AddCookie(XString p_cookie);
  // Disconnect from server
  void Disconnect();
  // Lock the client (for MT use)
  void LockClient();
  // Unlock the client (for MT use)
  void UnlockClient();
  // Find a result header
  XString FindHeader(XString p_header);
  
  // SETTERS
  bool SetURL(XString p_url);
  bool SetBody(const XString& p_body,const XString p_charset = _T("utf-8"));
  void SetBody(const void* p_body,unsigned p_length);
  void SetLogging(LogAnalysis* p_log,bool p_transferOwnership = false);
  void SetSecure(bool p_secure)                         { m_secure            = p_secure;   }; // URL Part secure
  void SetUser(const XString& p_user)                   { m_user              = p_user;     }; // URL Part user
  void SetPassword(const XString& p_password)           { m_password          = p_password; }; // URL Part password
  void SetServer(const XString& p_server)               { m_server            = p_server;   }; // URL Part server
  void SetPort(int p_port)                              { m_port              = p_port;     }; // URL Part port
  void SetRetries(int p_retries)                        { m_retries           = p_retries;  };
  void SetAgent(const XString& p_agent)                 { m_agent             = p_agent;    };
  void SetUseProxy(ProxyType p_type)                    { m_useProxy          = p_type;     };
  void SetProxy(const XString& p_proxy)                 { m_proxy             = p_proxy;    };
  void SetProxyBypass(const XString& p_bypass)          { m_proxyBypass       = p_bypass;   };
  void SetProxyUser(const XString& p_user)              { m_proxyUser         = p_user;     };
  void SetProxyPassword(const XString& p_pass)          { m_proxyPassword     = p_pass;     };
  void SetSendUnicode(bool p_unicode)                   { m_sendUnicode       = p_unicode;  };
  void SetVerb(XString p_verb)                          { m_verb              = p_verb;     };
  void SetContentType(const XString& p_type)            { m_contentType       = p_type;     };
  void SetHTTPCompression(bool p_compress)              { m_httpCompression   = p_compress; };
  void SetSoapAction(const XString& p_action)           { m_soapAction        = p_action;   };
  void SetTimeoutResolve(int p_timeout)                 { m_timeoutResolve    = p_timeout;  };
  void SetTimeoutConnect(int p_timeout)                 { m_timeoutConnect    = p_timeout;  };
  void SetTimeoutSend   (int p_timeout)                 { m_timeoutSend       = p_timeout;  };
  void SetTimeoutReceive(int p_timeout)                 { m_timeoutReceive    = p_timeout;  };
  void SetQueueRetention(int p_wait)                    { m_queueRetention    = p_wait;     };
  void SetRelaxOptions(DWORD p_options)                 { m_relax             = p_options;  };
  void SetPreEmptiveAuthorization(DWORD p_pre)          { m_preemtive         = p_pre;      };
  void SetTerminalServices(bool p_term)                 { m_terminalServices  = p_term;     };
  void SetEncryptionLevel(XMLEncryption p_level)        { m_securityLevel     = p_level;    };
  void SetEncryptionPassword(const XString& p_word)     { m_enc_password      = p_word;     };
  void SetSingleSignOn(bool p_sso)                      { m_sso               = p_sso;      };
  void SetSoapCompress(bool p_compress)                 { m_soapCompress      = p_compress; };
  void SetSslTlsSettings(unsigned p_ssltls)             { m_ssltls            = p_ssltls;   }; // WINHTTP_FLAG_SECURE_PROTOCOL_ALL
  void SetSendBOM(bool p_bom)                           { m_sendBOM           = p_bom;      };
  void SetVerbTunneling(bool p_tunnel)                  { m_verbTunneling     = p_tunnel;   };
  void SetClientCertificatePreset(bool p_preset)        { m_certPreset        = p_preset;   };
  void SetClientCertificateName(const XString& p_name)  { m_certName          = p_name;     };
  void SetClientCertificateStore(const XString& p_store){ m_certStore         = p_store;    };
  void SetWebsocketHandshake(bool p_socket)             { m_websocket         = p_socket;   };
  void SetOAuth2Cache(OAuth2Cache* p_cache)             { m_oauthCache        = p_cache;    };
  void SetOAuth2Session(int p_session)                  { m_oauthSession      = p_session;  };
  bool SetClientCertificateThumbprint(const XString& p_store,const XString& p_thumbprint);
  void SetCORSOrigin(const XString& p_origin);
  bool SetCORSPreFlight(const XString& p_method,const XString& p_headers);
  void SetLogLevel(int p_logLevel);
  void SetDetailLogging(bool p_detail);
  void SetTraceData(bool p_trace);
  void SetTraceRequest(bool p_trace);

  // GETTERS
  XString       GetURL()                    { return m_url;               }; // Full URL
  bool          GetSecure()                 { return m_secure;            }; // URL Part secure
  XString       GetUser()                   { return m_user;              }; // URL Part user
  XString       GetPassword()               { return m_password;          }; // URL Part password
  XString       GetServer()                 { return m_server;            }; // URL Part server
  int           GetPort()                   { return m_port;              }; // URL Part port
  BYTE*         GetBody()                   { return m_requestBody;       };
  bool          GetSendUnicode()            { return m_sendUnicode;       };
  XString       GetAgent()                  { return m_agent;             };
  ProxyType     GetUseProxy()               { return m_useProxy;          };
  XString       GetProxy()                  { return m_proxy;             };
  XString       GetProxyBypass()            { return m_proxyBypass;       };
  XString       GetProxyUser()              { return m_proxyUser;         };
  XString       GetProxyPassword()          { return m_proxyPassword;     };
  XString       GetVerb()                   { return m_verb;              };
  XString       GetContentType()            { return m_contentType;       };
  XString       GetSoapAction()             { return m_soapAction;        };
  int           GetTimeoutResolve()         { return m_timeoutResolve;    };
  int           GetTimeoutConnect()         { return m_timeoutConnect;    };
  int           GetTimeoutSend()            { return m_timeoutSend;       };
  int           GetTimeoutReceive()         { return m_timeoutReceive;    };
  int           GetStatus()                 { return m_status;            };
  unsigned      GetQueueRetention()         { return m_queueRetention;    };
  bool          GetQueueIsRunning()         { return m_queueThread!=NULL; };
  DWORD         GetRelaxOptions()           { return m_relax;             };
  DWORD         GetPreEmptiveAuthorization(){ return m_preemtive;         };
  bool          GetTerminalServices()       { return m_terminalServices;  };
  XMLEncryption GetEncryptionLevel()        { return m_securityLevel;     };
  XString       GetEncryptionPassword()     { return m_enc_password;      };
  bool          GetSingleSignOn()           { return m_sso;               };
  bool          GetSoapCompress()           { return m_soapCompress;      };
  HPFCounter*   GetCounter()                { return &m_counter;          };
  LogAnalysis*  GetLogging()                { return m_log;               };
  unsigned      GetSslTlsSettings()         { return m_ssltls;            }; 
  bool          GetSendBOM()                { return m_sendBOM;           };
  bool          GetVerbTunneling()          { return m_verbTunneling;     };
  bool          GetClientCertificatePreset(){ return m_certPreset;        };
  XString       GetClientCertificateName()  { return m_certName;          };
  XString       GetClientCertificateStore() { return m_certStore;         };
  bool          GetHTTPCompression()        { return m_httpCompression;   };
  XString       GetCORSOrigin()             { return m_corsOrigin;        };
  int           GetLogLevel()               { return m_logLevel;          };
  bool          GetIsInitialized()          { return m_initialized;       };
  unsigned      GetRetries()                { return m_retries;           };
  FindProxy&    GetProxyFinder()            { return m_proxyFinder;       };
  FileBuffer*   GetFileBuffer()             { return m_buffer;            };
  Cookies&      GetCookies()                { return m_cookies;           };
  Cookies&      GetResultCookies()          { return m_resultCookies;     };
  HeaderMap&    GetResponseHeaders()        { return m_responseHeaders;   };
  bool          GetPushEvents()             { return m_pushEvents;        };
  bool          GetOnCloseSeen()            { return m_onCloseSeen;       };
  int           GetQueueSize()              { return (int)m_queue.size(); };
  OAuth2Cache*  GetOAuth2Cace()             { return m_oauthCache;        };
  int           GetOAuth2Session()          { return m_oauthSession;      };
  XString       GetLastBearerToken()        { return m_lastBearerToken;   };
  int           GetError(XString* p_message = NULL);
  XString       GetStatusText();
  void          GetBody(void*& p_body,unsigned& p_length);
  void          GetResponse(BYTE*& p_response,unsigned& p_length);
  HINTERNET     GetWebsocketHandle();
  bool          GetDetailLogging();
  bool          GetTraceData();
  bool          GetTraceRequest();

  // Service routines. Not normally called by other objects
  // Central queue running function
  void          QueueRunning();

  // EVENT STREAM INTERFACE

  // Start a new event stream object
  EventSource*  CreateEventSource(XString p_url);
  // Main loop of the event-interface (public: starting a thread)
  void          EventThreadRunning();

private:
  // Handling of push-events
  friend   EventSource;

  // Error handling for logging
  int      ErrorLog(LPCTSTR p_function,LPCTSTR p_logText);
  // Initializing the client 
  void     ResetBody();
  void     InitializeSingleSignOn();
  void     CleanQueue();
  void     TestReconnect();
  void     InitSettings();
  void     InitLogging();
  void     InitSecurity();
  void     ReplaceSetting(XString* m_setting,XString p_potential);
  bool     StartEventStream(const XString& p_url); // Called from EventStream
  // To be done inside a 'Send'
  void     AddProxyInfo();
  void     AddHostHeader();
  void     AddLengthHeader();
  void     AddSecurityOptions();
  void     AddCORSHeaders();
  void     AddExtraHeaders();
  void     AddCookieHeaders();
  void     AddWebSocketUpgrade();
  void     AddProxyAuthorization();
  void     AddPreEmptiveAuthorization();
  bool     AddOAuth2authorization();
  void     FlushAllHeaders();
  void     AddMessageHeaders(HTTPMessage* p_message);
  void     AddMessageHeaders(SOAPMessage* p_message);
  void     AddMessageHeaders(JSONMessage* p_message);
  void     LogTheSend(wstring& p_server,int p_port);
  void     TraceTheSend();
  void     TraceTheAnswer();
  void     ProcessResultCookies();
  XString  GetResultHeader(DWORD p_header,DWORD p_index);
  DWORD    ChooseAuthScheme(DWORD p_dwSupportedSchemes);
  bool     AddAuthentication(bool p_ntlm3Step);
  bool     AddProxyAuthentication();
  void     SendBodyData();
  void     ReceiveResponseData();
  void     ReceiveResponseDataFile();
  void     ReceiveResponseDataBuffer();
  void     ReceivePushEvents();
  XString  ReadHeaderField(int p_header);
  void     ReadAllResponseHeaders();
  bool     CheckCORSAnswer();
  void     ResetOAuth2Session();
  bool     DoRedirectionAfterSend();
  void     ProcessChunkedEncoding();
  uchar*   GetChunkSize(uchar* p_reading,unsigned& p_size);
  void     ReCreateAsSOAPFault(SOAPMessage* p_msg,SoapVersion p_version,XString p_response);
  // Methods for WS-Security
  void     CheckAnswerSecurity (SOAPMessage* p_msg,XString p_answer,XMLEncryption p_security,XString p_password);
  void     CheckBodySigning    (XString p_password,SOAPMessage* p_msg);
  void     DecodeBodyEncryption(XString p_password,SOAPMessage* p_msg,XString p_answer);
  void     DecodeMesgEncryption(XString p_password,SOAPMessage* p_msg,XString p_answer);
  // Get next message from the queue
  bool     GetFromQueue(HTTPMessage** p_entry1,SOAPMessage** p_entry2,JSONMessage** p_entry3);
  void     CreateQueueEvent();
  void     StartQueue();
  // Start a thread for the streaming server-push event interface
  bool     StartEventStreamingThread();
  void     OnCloseSeen();
  // Processing after a send
  void     ProcessJSONResult(JSONMessage* p_msg,bool& p_result);
  // Setting a client certificate on the request handle
  bool     SetClientCertificate(HINTERNET p_request);
  // Running the queue
  void     ProcessQueueMessage(HTTPMessage* p_message);
  void     ProcessQueueMessage(SOAPMessage* p_message);
  void     ProcessQueueMessage(JSONMessage* p_message);

  // PRIVATE DATA

  // Connection specials
  bool          m_initialized     { false   };                    // Initialisation done?
  bool          m_initializedLog  { false   };                    // Initialisation logfile done?
  unsigned      m_retries         { 0       };                    // Number of sending retries
  XString       m_agent           { "HTTPClient/8.1" };           // User agents name (spoofing!!)
  ProxyType     m_useProxy        { ProxyType::PROXY_IEPROXY };   // Which proxy to use
  XString       m_proxy;                                          // Use proxy
  XString       m_proxyBypass;                                    // Do not use these proxies
  XString       m_proxyUser;                                      // Proxy authentication user
  XString       m_proxyPassword;                                  // Proxy authentication password
  XString       m_verb            { "GET"   };                    // Sending VERB
  bool          m_soapCompress    { false   };                    // Compress SOAP webservices
  FindProxy     m_proxyFinder;                                    // For finding our proxy
  bool          m_verbTunneling   { false   };                    // Use HTTP VERB-Tunneling
  bool          m_httpCompression { false   };                    // Accepts HTTP compression (gzip!)
  // URL
  XString       m_url;                                            // Full URL
  XString       m_scheme          { "http"  };                    // URL part: protocol scheme
  bool          m_secure          { false   };                    // URL part: Secure connection (HTTPS)
  XString       m_user;                                           // URL part: Authentication user
  XString       m_password;                                       // URL part: Authentication password
  XString       m_server;                                         // URL part: Server name part
  int           m_port            { INTERNET_DEFAULT_HTTP_PORT }; // URL part: port number
  // Connection variables
  DWORD         m_relax           { 0       };                    // Relax certificate security for test reasons
  bool          m_terminalServices{ false   };                    // Support TerminalServices desktops (Citrix)
  XMLEncryption m_securityLevel   { XMLEncryption::XENC_NoInit }; // Encryption level for SOAP messages
  XString       m_enc_password;                                   // Encryption password for SOAP messages
  bool          m_sso             { false   };                    // Use Windows Single-Sign-On
  unsigned      m_ssltls { WINHTTP_FLAG_SECURE_PROTOCOL_MARLIN }; // SSL / TLS settings of secure HTTPS
  DWORD         m_preemtive       { 0       };                    // Pre-emptive authentication schema
  // Client Certificate
  bool          m_certPreset      { false   };                    // Pre-set client certificate prior to request
  XString       m_certStore       { "MY"    };                    // Certificate store of the current user
  XString       m_certName;                                       // Client certificate name to search for
  // The content to send/receive
  BYTE*         m_requestBody     { nullptr };                    // Body to send
  unsigned      m_bodyLength      { 0       };                    // Length of body to send
  FileBuffer*   m_buffer          { nullptr };                    // File buffer to send
  XString       m_contentType;                                    // Content MIME type to send
  XString       m_soapAction;                                     // Soap action if a SOAPMessage WebService
  bool          m_sendUnicode     { false   };                    // Content is in Unicode-16
  bool          m_sniffCharset    { true    };                    // Sniff content charset on receive
  bool          m_sendBOM         { false   };                    // Prepend BOM to all messages
  // Result of the call
  DWORD         m_lastError       { 0       };                    // Last error if any at all
  unsigned      m_status          { 0       };                    // Total status result of the call
  Cookies       m_resultCookies;                                  // Resulting cookies optimized here
  BYTE*         m_response        { nullptr };                    // Result body
  unsigned      m_responseLength  { 0       };                    // Length of result body
  // Connection handles
  HINTERNET     m_session         { nullptr };                    // HTTP session
  HINTERNET     m_connect         { nullptr };                    // Current connection to URL
  HINTERNET     m_request         { nullptr };                    // Current request (send/receive)
  // Timeouts
  unsigned      m_timeoutResolve  { DEF_TIMEOUT_RESOLVE };        // Timeout resolving URL
  unsigned      m_timeoutConnect  { DEF_TIMEOUT_CONNECT };        // Timeout in connecting to URL
  unsigned      m_timeoutSend     { DEF_TIMEOUT_SEND    };        // Timeout in sending 
  unsigned      m_timeoutReceive  { DEF_TIMEOUT_RECEIVE };        // Timeout in receiving
  // Extra headers / Cookie
  HeaderMap     m_requestHeaders;                                 // All request headers to the call
  HeaderMap     m_responseHeaders;                                // All response headers from the call
  Cookies       m_cookies;                                        // All cookies to send
  // CORS Cross Origin Resource Sharing
  XString       m_corsOrigin;                                     // Use CORS header methods (sending "Origin:")
  XString       m_corsMethod;                                     // Pre-flight request method  in OPTIONS call
  XString       m_corsHeaders;                                    // Pre-flight request headers in OPTIONS call
  // Server-push-event stream
  bool          m_pushEvents      { false   };                    // Handles server-push-events
  EventSource*  m_eventSource     { nullptr };                    // Parsing of incoming stream
  bool          m_onCloseSeen     { false   };                    // OnClose has passed the interface, try new stream
  // Re-connect optimization
  XString       m_lastServer;                                     // Last connected server.
  int           m_lastPort        { 0       };                    // Last connected port
  bool          m_lastSecure      { false   };                    // Last connected secure status
  // Queuing of multiple messages
  HttpQueue     m_queue;                                          // Queue of multiple HTTP/SOAP calls
  HANDLE        m_queueEvent      { NULL    };                    // Event for entering messages in the queue
  HANDLE        m_queueThread     { NULL    };                    // Event for waking the queue thread
  unsigned      m_queueRetention  { QUEUE_WAITTIME };             // Seconds before queue thread dies
  bool          m_running         { false   };                    // Queue is running
  // Logging & settings
  MarlinConfig     m_marlinConfig;                                   // Current Marlin.config file
  LogAnalysis*  m_log             { nullptr };                    // Current logging
  bool          m_logOwner        { false   };                    // Owner of the current logging
  int           m_logLevel        { HLL_NOLOG };                  // Logging level of the client
  HPFCounter    m_counter;                                        // High Performance counter
  HTTPClientTracing* m_trace      { nullptr };                    // The tracing object
  // WebSocket
  bool          m_websocket       { false   };                    // Try WebSocket handshake
  // OAuth2
  OAuth2Cache*  m_oauthCache      { nullptr };                    // OAuth tokens
  int           m_oauthSession    { 0       };                    // Session in the OAuth2 Cache
  XString       m_lastBearerToken;                                // Last used OAuth2 bearer token
  // For syncing threads
  CRITICAL_SECTION m_queueSection;  // Synchronizing queue adding/sending
  CRITICAL_SECTION m_sendSection;   // Synchronizing sending for multiple threads
};
