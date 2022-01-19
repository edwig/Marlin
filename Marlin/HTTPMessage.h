/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: HTTPMessage.h
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
#include "Headers.h"
#include "FileBuffer.h"
#include "CrackURL.h"
#include "Cookie.h"
#include "Routing.h"
#include <http.h>
#include <winhttp.h>
#include <map>

// 10k threshold
constexpr auto HTTP_BUFFER_THRESHOLD = (10 * 1024); 

// HTTP Header commands
// Retain numeric values for indexing!!
// See: const char* headers[] in the implementation
enum class HTTPCommand
{
  http_no_command = -1
  // Response
 ,http_response   = 0
  // Outgoing standard commands RFC 2616
 ,http_post       = 1
 ,http_get
 ,http_put
 ,http_delete
 ,http_head
 ,http_trace
 ,http_connect
 ,http_options
  // WebDAV commands RFC 2518
 ,http_move
 ,http_copy
 ,http_proppfind
 ,http_proppatch
 ,http_mkcol
 ,http_lock
 ,http_unlock
 ,http_search  // draft-reschke-webdav-search
  // For REST interfaces RFC 3253
 ,http_merge
 ,http_patch   // draft-dusseault-http-patch 
 ,http_versioncontrol
 ,http_report
 ,http_checkout
 ,http_checkin
 ,http_uncheckout
 ,http_mkworkspace
 ,http_update
 ,http_label
 ,http_baselinecontrol
 ,http_mkactivity
  // RFC 3648
 ,http_orderpatch
  // RFC 3744
 ,http_acl
  // Nothing after this
 ,http_last_command = http_acl
};

// All HTTP header commands
extern const char* headers[];

// Incoming and responding header names
extern const char* header_fields[];
extern const char* header_response[];

using ushort    = unsigned short;

// Forward declarations
class   SOAPMessage;
class   JSONMessage;
class   FileBuffer;
class   HTTPServer;
class   HTTPSite;
class   MultiPartBuffer;

class HTTPMessage
{
public:
  // General XTOR
  HTTPMessage();
  // XTOR for HTTP command (get,post etc)
  HTTPMessage(HTTPCommand p_command,int p_code = HTTP_STATUS_OK);
  // XTOR for HTTP command from a HTTPServer
  HTTPMessage(HTTPCommand p_command,HTTPSite* p_site);
  // XTOR for HTTP command and a URL
  HTTPMessage(HTTPCommand p_command,CString p_url);
  // XTOR from another HTTPMessage
  HTTPMessage(HTTPMessage* p_msg,bool p_deep = false);
  // XTOR from a SOAPMessage
  HTTPMessage(HTTPCommand p_command,SOAPMessage* p_msg);
  // XTOR from a JSONMessage
  HTTPMessage(HTTPCommand p_command,JSONMessage* p_msg);
  // DTOR
 ~HTTPMessage();

  // Recycle the object for usage in a return message
  void Reset();

  // SETTERS
  void SetBody(CString     p_body,CString p_encoding = "");
  void SetBody(const char* p_body,CString p_encoding = "");
  void SetBody(void* p_body,unsigned p_length);
  void SetURL(CString& p_url);
  bool SetVerb(CString p_verb);
  void SetAcceptEncoding(CString p_encoding);
  void SetCommand(HTTPCommand p_command)        { m_command            = p_command;   };
  void SetReferrer(CString p_referrer)          { m_referrer           = p_referrer;  };
  void SetStatus(unsigned p_status)             { m_status             = p_status;    };
  void SetUser(CString p_user)                  { m_user               = p_user;      };
  void SetPassword(CString p_password)          { m_password           = p_password;  };
  void SetSecure(bool p_secure)                 { m_cracked.m_secure   = p_secure;    ReparseURL();  };
  void SetServer(CString& p_server)             { m_cracked.m_host     = p_server;    ReparseURL();  };
  void SetPort(unsigned p_port)                 { m_cracked.m_port     = p_port;      ReparseURL();  };
  void SetAbsolutePath(CString& p_path)         { m_cracked.SetPath(p_path);          ReparseURL();  };
  void SetRequestHandle(HTTP_OPAQUE_ID p_req)   { m_request            = p_req;       };
  void SetAccessToken(HANDLE p_token)           { m_token              = p_token;     };
  void SetRemoteDesktop(UINT p_desktop)         { m_desktop            = p_desktop;   };
  void SetContentType(CString p_type)           { m_contentType        = p_type;      };
  void SetContentLength(size_t p_length)        { m_contentLength      = p_length;    };
  void SetUseIfModified(bool p_ifmodified)      { m_ifmodified         = p_ifmodified;};
  void SetSendBOM(bool p_bom)                   { m_sendBOM            = p_bom;       };
  void SetVerbTunneling(bool p_tunnel)          { m_verbTunnel         = p_tunnel;    };
  void SetConnectionID(HTTP_CONNECTION_ID p_id) { m_connectID          = p_id;        };
  void SetSystemTime(SYSTEMTIME p_time)         { m_systemtime         = p_time;      };
  void SetHasBeenAnswered()                     { m_request            = NULL;        };
  void SetReadBuffer(bool p_read,size_t p_length = 0);
  void SetSender  (PSOCKADDR_IN6 p_address);
  void SetReceiver(PSOCKADDR_IN6 p_address);
  void SetFile(CString& p_fileName);
  void SetAuthorization(CString& p_authorization);
  void SetAllHeaders    (PHTTP_REQUEST_HEADERS p_headers);
  void SetUnknownHeaders(PHTTP_REQUEST_HEADERS p_headers);
  void SetCookie(Cookie& p_cookie);
  void SetCookie(CString p_fromHttp);
  void SetCookie(CString p_name,CString p_value,CString p_metadata = "",bool p_secure = false,bool p_httpOnly = false);
  void SetCookiePairs(CString p_cookies);     // From "Cookie:" only
  void SetCookies(Cookies& p_cookies);
  bool SetHTTPSite(HTTPSite* p_site);

  // GETTERS
  HTTPCommand         GetCommand()              { return m_command;                   };
  CString             GetURL()                  { return m_url;                       };
  CString             GetReferrer()             { return m_referrer;                  };
  CrackedURL&         GetCrackedURL()           { return m_cracked;                   };
  unsigned            GetStatus()               { return m_status;                    };
  CString             GetUser()                 { return m_user;                      };
  CString             GetPassword()             { return m_password;                  };
  bool                GetSecure()               { return m_cracked.m_secure;          };
  CString             GetServer()               { return m_cracked.m_host;            };
  unsigned            GetPort()                 { return m_cracked.m_port;            };
  CString             GetAbsolutePath()         { return m_cracked.AbsolutePath();    };
  CString             GetAbsoluteResource()     { return m_cracked.AbsoluteResource();};
  HTTP_OPAQUE_ID      GetRequestHandle()        { return m_request;                   };
  HTTPSite*           GetHTTPSite()             { return m_site;                      };
  bool                GetReadBuffer()           { return m_readBuffer;                };
  size_t              GetContentLength()        { return m_contentLength;             };
  FileBuffer*         GetFileBuffer()           { return &m_buffer;                   };
  CString             GetContentType()          { return m_contentType;               };
  HANDLE              GetAccessToken()          { return m_token;                     };
  UINT                GetRemoteDesktop()        { return m_desktop;                   };
  PSOCKADDR_IN6       GetSender()               { return &m_sender;                   };
  PSOCKADDR_IN6       GetReceiver()             { return &m_receiver;                 };
  bool                GetUseIfModified()        { return m_ifmodified;                };
  PSYSTEMTIME         GetSystemTime()           { return &m_systemtime;               };
  HeaderMap*          GetHeaderMap()            { return &m_headers;                  };
  bool                GetSendBOM()              { return m_sendBOM;                   };
  bool                GetVerbTunneling()        { return m_verbTunnel;                };
  HTTP_CONNECTION_ID  GetConnectionID()         { return m_connectID;                 };
  CString             GetAcceptEncoding()       { return m_acceptEncoding;            };
  Cookies&            GetCookies()              { return m_cookies;                   };
  Routing&            GetRouting()              { return m_routing;                   };
  CString             GetBody();
  size_t              GetBodyLength();
  CString             GetVerb();
  CString             GetHeader(CString p_name);
  void                GetRawBody(uchar** p_body,size_t& p_length);
  Cookie*             GetCookie(unsigned p_ind);
  CString             GetCookieValue(unsigned p_ind = 0,CString p_metadata = "");
  CString             GetCookieValue(CString p_name,    CString p_metadata = "");
  CString             GetRoute(int p_index);
  bool                GetHasBeenAnswered();

  // EXTRA FUNCTIONS

  // Reset all cookies
  void    ResetCookies();
  // Add a body from a text field
  void    AddBody(const char* p_body);
  // Add a body from a binary BLOB
  void    AddBody(void* p_body,unsigned p_length);
  // Add a header-name / header-value pair
  void    AddHeader(CString p_name,CString p_value);
  // Add a header by known header-id
  void    AddHeader(HTTP_HEADER_ID p_id,CString p_value);
  // Delete a header by name
  void    DelHeader(CString p_name);
  // Add a routing part
  void    AddRoute(CString p_route);
  // Convert system time to HTTP time string
  CString HTTPTimeFormat(PSYSTEMTIME p_systime = NULL);
  // Convert HTTP time string to system time
  bool    SetHTTPTime(CString p_timestring);
  // Change POST method to PUT/MERGE/PATCH/DELETE (Incoming!)
  bool    FindVerbTunneling();
  // Use POST method for PUT/MERGE/PATCH/DELETE (Outgoing!)
  bool    UseVerbTunneling();
  // Accept a MultiPartBuffer in this message
  bool    SetMultiPartFormData(MultiPartBuffer* p_buffer);
  // Reference system for storing the message elsewhere
  void    AddReference();
  void    DropReference();

private:
  // Parse raw URL to cracked URL data
  bool    ParseURL(CString p_url);
  // Check for minimal sending requirements
  void    CheckServer();
  // Re-parse URL after setting a part of the URL
  void    ReparseURL();
  // Fill message with FormData buffer
  bool    SetMultiPartBuffer (MultiPartBuffer* p_buffer);
  // Fill message with FormData URL encoding
  bool    SetMultiPartURL    (MultiPartBuffer* p_buffer);
  bool    SetMultiPartURLGet (MultiPartBuffer* p_buffer);
  bool    SetMultiPartURLPost(MultiPartBuffer* p_buffer);

  HTTPCommand         m_command       { HTTPCommand::http_response};  // HTTP Command code 'get','post' etc
  unsigned            m_status        { HTTP_STATUS_OK };             // HTTP status return code
  HTTP_OPAQUE_ID      m_request       { NULL    };                    // Request handle for answering
  HTTP_CONNECTION_ID  m_connectID     { NULL    };                    // HTTP Connection ID
  HTTPSite*           m_site          { nullptr };                    // Site for which message is received
  CString             m_contentType;                                  // Carrying this content
  CString             m_acceptEncoding;                               // Accepted compression encoding
  CString             m_user;                                         // Found user name for authentication
  CString             m_password;                                     // Found password for authentication
  bool                m_verbTunnel    { false   };                    // HTTP-VERB Tunneling used
  bool                m_sendBOM       { false   };                    // BOM discovered in content block
  bool                m_readBuffer    { false   };                    // HTTP content still to be read
  size_t              m_contentLength { 0       };                    // Total content to read for the message
  FileBuffer          m_buffer;                                       // Body or file buffer
  Cookies             m_cookies;                                      // Cookies
  CString             m_url;                                          // Full URL to service
  CString             m_referrer;                                     // Referrer of this call
  HANDLE              m_token         { NULL    };                    // Access token
  SOCKADDR_IN6        m_sender;                                       // Senders  address end
  SOCKADDR_IN6        m_receiver;                                     // Receiver address end
  UINT                m_desktop       { 0       };                    // Remote desktop number
  CrackedURL          m_cracked;                                      // Cracked down URL
  HeaderMap           m_headers;                                      // All/Known headers
  Routing             m_routing;                                      // Routing information within a site
  bool                m_ifmodified    { false   };                    // Use "if-modified-since"
  SYSTEMTIME          m_systemtime;                                   // System time for m_modified
  long                m_references    { 1       };                    // Referencing system
};

inline void 
HTTPMessage::SetBody(void* p_body,unsigned p_length)
{
  m_buffer.SetBuffer((uchar*)p_body,p_length);
}

inline void
HTTPMessage::SetURL(CString& p_url)
{
  m_url = p_url;
  ParseURL(p_url);
}

inline size_t
HTTPMessage::GetBodyLength()
{
  return m_buffer.GetLength();
}

inline void
HTTPMessage::AddBody(const char* p_buffer)
{
  m_buffer.AddBuffer((uchar*)p_buffer,strlen(p_buffer));
}

inline void 
HTTPMessage::AddBody(void* p_body,unsigned p_length)
{
  m_buffer.AddBuffer((uchar*)p_body,p_length);
}

inline void 
HTTPMessage::SetCookie(Cookie& p_cookie)
{
  m_cookies.AddCookie(p_cookie);
}

inline void 
HTTPMessage::SetCookies(Cookies& p_cookies)
{
  m_cookies = p_cookies;
}

inline void
HTTPMessage::ResetCookies()
{
  m_cookies.Clear();
}

inline bool
HTTPMessage::GetHasBeenAnswered()
{
  return m_request == NULL;
}

inline void
HTTPMessage::AddRoute(CString p_route)
{
  m_routing.push_back(p_route);
}
