/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: HTTPMessage.h
//
// BaseLibrary: Indispensable general objects and functions
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
#include "Headers.h"
#include "FileBuffer.h"
#include "CrackURL.h"
#include "Cookie.h"
#include "Routing.h"
#include "ConvertWideString.h"
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
extern LPCTSTR headers[];

// Incoming and responding header names
extern LPCTSTR header_fields[];
extern LPCTSTR header_response[];

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
  explicit HTTPMessage(HTTPCommand p_command,int p_code = HTTP_STATUS_OK);
  // XTOR for HTTP command from a HTTPServer
  explicit HTTPMessage(HTTPCommand p_command,HTTPSite* p_site);
  // XTOR for HTTP command and a URL
  explicit HTTPMessage(HTTPCommand p_command,XString p_url);
  // XTOR from another HTTPMessage
  explicit HTTPMessage(HTTPMessage* p_msg,bool p_deep = false);
  // XTOR from a SOAPMessage
  explicit HTTPMessage(HTTPCommand p_command,const SOAPMessage* p_msg);
  // XTOR from a JSONMessage
  explicit HTTPMessage(HTTPCommand p_command,const JSONMessage* p_msg);
  // DTOR
 ~HTTPMessage();

  // Recycle the object for usage in a return message
  void Reset(bool p_resetURL = false);

  // SETTERS
  void SetBody(XString p_body,XString p_charset = _T(""));
  void SetBody(LPCTSTR p_body,XString p_charset = _T(""));
  void SetBody(void* p_body,unsigned p_length);
  void SetURL(const XString& p_url);
  bool SetVerb(XString p_verb);
  void SetAcceptEncoding(XString p_encoding);
  void SetCommand(HTTPCommand p_command)        { m_command            = p_command;   }
  void SetReferrer(XString p_referrer)          { m_referrer           = p_referrer;  }
  void SetStatus(unsigned p_status)             { m_status             = p_status;    }
  void SetUser(XString p_user)                  { m_user               = p_user;      }
  void SetPassword(XString p_password)          { m_password           = p_password;  }
  void SetSecure(bool p_secure)                 { m_cracked.m_secure   = p_secure;    ReparseURL();  }
  void SetServer(const XString& p_server)       { m_cracked.m_host     = p_server;    ReparseURL();  }
  void SetPort(unsigned p_port)                 { m_cracked.m_port     = p_port;      ReparseURL();  }
  void SetAbsolutePath(const XString& p_path)   { m_cracked.SetPath(p_path);          ReparseURL();  }
  void SetRequestHandle(HTTP_OPAQUE_ID p_req)   { m_request            = p_req;       }
  void SetAccessToken(HANDLE p_token)           { m_token              = p_token;     }
  void SetRemoteDesktop(UINT p_desktop)         { m_desktop            = p_desktop;   }
  void SetContentType(XString p_type)           { m_contentType        = p_type;      }
  void SetContentLength(size_t p_length)        { m_contentLength      = p_length;    }
  void SetUseIfModified(bool p_ifmodified)      { m_ifmodified         = p_ifmodified;}
  void SetEncoding(Encoding p_encoding)         { m_encoding           = p_encoding;  }
  void SetSendBOM(bool p_bom)                   { m_sendBOM            = p_bom;       }
  void SetVerbTunneling(bool p_tunnel)          { m_verbTunnel         = p_tunnel;    }
  void SetConnectionID(HTTP_CONNECTION_ID p_id) { m_connectID          = p_id;        }
  void SetSystemTime(SYSTEMTIME p_time)         { m_systemtime         = p_time;      }
  void SetHasBeenAnswered()                     { m_request            = NULL;        }
  void SetChunkNumber(int p_chunk)              { m_chunkNumber        = p_chunk;     }
  void SetXMLHttpRequest(boolean p_value)       { m_XMLHttpRequest     = p_value;     }
  void SetExtension(XString p_ext,bool p_reparse = true);
  void SetReadBuffer(bool p_read,size_t p_length = 0);
  void SetSender  (PSOCKADDR_IN6 p_address);
  void SetReceiver(PSOCKADDR_IN6 p_address);
  void SetFile(const XString& p_fileName);
  void SetAuthorization(XString& p_authorization);
  void SetAllHeaders    (PHTTP_REQUEST_HEADERS p_headers);
  void SetUnknownHeaders(PHTTP_REQUEST_HEADERS p_headers);
  void SetCookie(Cookie& p_cookie);
  void SetCookie(XString p_fromHttp);
  void SetCookie(XString p_name,XString p_value,XString p_metadata = _T(""),bool p_secure = false,bool p_httpOnly = false);
  void SetCookie(XString        p_name
                ,XString        p_value
                ,XString        p_metadata
                ,XString        p_path
                ,XString        p_domain
                ,bool           p_secure   = false
                ,bool           p_httpOnly = false
                ,CookieSameSite p_samesite = CookieSameSite::NoSameSite
                ,int            p_maxAge   = 0
                ,SYSTEMTIME*    p_expires  = nullptr);
  void SetCookiePairs(XString p_cookies);     // From "Cookie:" only
  void SetCookies(const Cookies& p_cookies);
  bool SetHTTPSite(HTTPSite* p_site);

  // GETTERS
  HTTPCommand         GetCommand()              { return m_command;                   }
  XString             GetURL()                  { return m_url;                       }
  XString             GetReferrer()             { return m_referrer;                  }
  CrackedURL&         GetCrackedURL()           { return m_cracked;                   }
  unsigned            GetStatus()               { return m_status;                    }
  XString             GetUser()                 { return m_user;                      }
  XString             GetPassword()             { return m_password;                  }
  bool                GetSecure()               { return m_cracked.m_secure;          }
  XString             GetServer()               { return m_cracked.m_host;            }
  unsigned            GetPort()                 { return m_cracked.m_port;            }
  XString             GetAbsolutePath()         { return m_cracked.AbsolutePath();    }
  XString             GetAbsoluteResource()     { return m_cracked.AbsoluteResource();}
  XString             GetExtension()            { return m_cracked.GetExtension();    }
  HTTP_OPAQUE_ID      GetRequestHandle()        { return m_request;                   }
  HTTPSite*           GetHTTPSite()             { return m_site;                      }
  bool                GetReadBuffer()           { return m_readBuffer;                }
  size_t              GetContentLength()        { return m_contentLength;             }
  FileBuffer*         GetFileBuffer()           { return &m_buffer;                   }
  XString             GetContentType()          { return m_contentType;               }
  HANDLE              GetAccessToken()          { return m_token;                     }
  UINT                GetRemoteDesktop()        { return m_desktop;                   }
  PSOCKADDR_IN6       GetSender()               { return &m_sender;                   }
  PSOCKADDR_IN6       GetReceiver()             { return &m_receiver;                 }
  bool                GetUseIfModified()        { return m_ifmodified;                }
  PSYSTEMTIME         GetSystemTime()           { return &m_systemtime;               }
  HeaderMap*          GetHeaderMap()            { return &m_headers;                  }
  Encoding            GetEncoding()             { return m_encoding;                  }
  bool                GetSendBOM()              { return m_sendBOM;                   }
  bool                GetVerbTunneling()        { return m_verbTunnel;                }
  HTTP_CONNECTION_ID  GetConnectionID()         { return m_connectID;                 }
  XString             GetAcceptEncoding()       { return m_acceptEncoding;            }
  Cookies&            GetCookies()              { return m_cookies;                   }
  Routing&            GetRouting()              { return m_routing;                   }
  unsigned            GetChunkNumber()          { return m_chunkNumber;               }
  boolean             GetXMLHttpRequest()       { return m_XMLHttpRequest;            }

  XString             GetBody();
  size_t              GetBodyLength();
  XString             GetVerb();
  XString             GetHeader(XString p_name);
  void                GetRawBody(uchar** p_body,size_t& p_length);
  Cookie*             GetCookie(unsigned p_ind);
  XString             GetCookieValue(unsigned p_ind = 0,XString p_metadata = _T(""));
  XString             GetCookieValue(XString p_name,    XString p_metadata = _T(""));
  XString             GetRoute(int p_index);
  bool                GetHasBeenAnswered();

  // EXTRA FUNCTIONS

  // Reset all cookies
  void    ResetCookies();
  // Add a body from a text field
  void    AddBody(XString p_body,XString p_charset = _T("utf-8"));
  // Add a body from a binary BLOB
  void    AddBody(void* p_body,unsigned p_length);
  // Add a header-name / header-value pair
  void    AddHeader(XString p_name,XString p_value);
  // Add a header by known header-id
  void    AddHeader(HTTP_HEADER_ID p_id,XString p_value);
  // Delete a header by name
  void    DelHeader(XString p_name);
  // Add a routing part
  void    AddRoute(XString p_route);
  // Convert system time to HTTP time string
  XString HTTPTimeFormat(PSYSTEMTIME p_systime = nullptr);
  // Convert HTTP time string to system time
  bool    SetHTTPTime(XString p_timestring);
  // Change POST method to PUT/MERGE/PATCH/DELETE (Incoming!)
  bool    FindVerbTunneling();
  // Use POST method for PUT/MERGE/PATCH/DELETE (Outgoing!)
  bool    UseVerbTunneling();
  // Accept a MultiPartBuffer in this message
  bool    SetMultiPartFormData(MultiPartBuffer* p_buffer);
  // Reference system for storing the message elsewhere
  void    AddReference();
  bool    DropReference();

  // Operators
  HTTPMessage& operator=(const JSONMessage& p_message);
  HTTPMessage& operator=(const SOAPMessage& p_message);

private:
  // TO BE CALLED FROM THE XTOR!!
  XString DecodeCharsetAndEncoding(Encoding p_encoding,XString p_contentType,XString p_defaultContentType);
  void    ConstructBodyFromString(XString p_string,XString p_charset,bool p_withBom);
  // Parse raw URL to cracked URL data
  bool    ParseURL(XString p_url);
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
  XString             m_contentType;                                  // Carrying this content
  XString             m_acceptEncoding;                               // Accepted compression encoding
  XString             m_user;                                         // Found user name for authentication
  XString             m_password;                                     // Found password for authentication
  bool                m_verbTunnel    { false   };                    // HTTP-VERB Tunneling used
  Encoding            m_encoding      { Encoding::EN_ACP };           // Buffer encoding (if any known charset)
  bool                m_sendBOM       { false   };                    // BOM discovered in content block
  bool                m_readBuffer    { false   };                    // HTTP content still to be read
  size_t              m_contentLength { 0       };                    // Total content to read for the message
  unsigned            m_chunkNumber   { 0       };                    // Chunk number in case of transfer-encoding: chunked
  FileBuffer          m_buffer;                                       // Body or file buffer
  Cookies             m_cookies;                                      // Cookies
  XString             m_url;                                          // Full URL to service
  CrackedURL          m_cracked;                                      // Cracked down URL
  XString             m_referrer;                                     // Referrer of this call
  HANDLE              m_token         { NULL    };                    // Access token
  SOCKADDR_IN6        m_sender;                                       // Senders  address end
  SOCKADDR_IN6        m_receiver;                                     // Receiver address end
  UINT                m_desktop       { 0       };                    // Remote desktop number
  HeaderMap           m_headers;                                      // All/Known headers
  Routing             m_routing;                                      // Routing information within a site
  bool                m_ifmodified    { false   };                    // Use "if-modified-since"
  SYSTEMTIME          m_systemtime;                                   // System time for m_modified
  long                m_references    { 1       };                    // Referencing system
  boolean             m_XMLHttpRequest{ false   };                    // Ajax Request (Triggers CORS!)
};

inline void 
HTTPMessage::SetBody(void* p_body,unsigned p_length)
{
  m_buffer.SetBuffer(reinterpret_cast<uchar*>(p_body),p_length);
}

inline size_t
HTTPMessage::GetBodyLength()
{
  return m_buffer.GetLength();
}

inline void
HTTPMessage::AddBody(XString p_buffer,XString p_charset /*=_T("utf-8")*/)
{
#ifdef _UNICODE
  if(p_charset.Compare(_T("utf-16")) == 0)
  {
    m_buffer.AddBuffer((uchar*)p_buffer.GetString(),p_buffer.GetLength() * sizeof(TCHAR));
  }
  else
  {
    BYTE* buffer = nullptr;
    int   length = 0;
    if(TryCreateNarrowString(p_buffer,p_charset,false,&buffer,length))
    {
      m_buffer.AddBuffer(buffer,length);
    }
    delete[] buffer;
  }
#else
  XString buffer = EncodeStringForTheWire(p_buffer,p_charset);
  m_buffer.AddBuffer((uchar*)buffer.GetString(),buffer.GetLength());
#endif
}

inline void 
HTTPMessage::AddBody(void* p_body,unsigned p_length)
{
  m_buffer.AddBuffer(reinterpret_cast<uchar*>(p_body),p_length);
}

inline void 
HTTPMessage::SetCookie(Cookie& p_cookie)
{
  m_cookies.AddCookie(p_cookie);
}

inline void 
HTTPMessage::SetCookies(const Cookies& p_cookies)
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
HTTPMessage::AddRoute(XString p_route)
{
  m_routing.push_back(p_route);
}
