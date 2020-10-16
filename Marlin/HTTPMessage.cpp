/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: HTTPMessage.cpp
//
// Marlin Server: Internet server/client
// 
// Copyright (c) 2015-2018 ir. W.E. Huisman
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
#include "HTTPMessage.h"
#include "HTTPSite.h"
#include "SOAPMessage.h"
#include "JSONMessage.h"
#include "CrackURL.h"
#include "Base64.h"
#include "Crypto.h"
#include "HTTPTime.h"
#include "ConvertWideString.h"
#include "MultiPartBuffer.h"
#include <xutility>
#include <string>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

using std::wstring;

// All headers. Must be in sequence with HTTPCommand
// See HTTPCommand for sequencing
const char* headers[] = 
{
   "HTTP"             // HTTP Response header
  ,"POST"             // Posting a resource
  ,"GET"              // Getting a resource
  ,"PUT"              // Putting a resource
  ,"DELETE"           // Deleting a resource
  ,"HEAD"             // Header for forwarding calls
  ,"TRACE"            // Tracing the server
  ,"CONNECT"          // Connect tunneling and forwarding
  ,"OPTIONS"          // Connection abilities of the server

  // EXTENSION ON THE STANDARD HTTP PROTOCOL

  ,"MOVE"             // Moving a resource
  ,"COPY"             // Copying a resource
  ,"PROPFIND"         // Finding a property of a resource
  ,"PROPPATCH"        // Patching a property of a resource
  ,"MKCOL"            // Make a resource collection
  ,"LOCK"             // Locking a resource
  ,"UNLOCK"           // Unlocking a resource
  ,"SEARCH"           // Searching for a resource
  ,"MERGE"            // Merge resources
  ,"PATCH"            // Patching a resource
  ,"VERSION-CONTROL"  // VERSION-CONTROL of a resource
  ,"REPORT"           // REPORT-ing of a resource
  ,"CHECKOUT"         // CHECKOUT of a resource
  ,"CHECKIN"          // CHECKIN of a resource
  ,"UNCHECKOUT"       // UNDO a CHECKOUT of a resource
  ,"MKWORKSPACE"      // Making a WORKSPACE for resources
  ,"UDPATE"           // UPDATE of a resource
  ,"LABEL"            // LABELing of a resource
  ,"BASELINE-CONTROL" // BASLINES of resources
  ,"MKACTIVITY"       // ACTIVITY creation for a resource
  ,"ORDERPATCH"       // PATCHING of orders
  ,"ACL"              // Account Controll List
};

// General XTOR
HTTPMessage::HTTPMessage()
{
  m_status = HTTP_STATUS_FIRST;
  memset(&m_sender,    0,sizeof(SOCKADDR_IN6));
  memset(&m_systemtime,0,sizeof(SYSTEMTIME));
}

// XTOR for a specific message (get, post etc)
HTTPMessage::HTTPMessage(HTTPCommand p_command, int p_code /*=HTTP_STATUS_OK*/)
            :m_command(p_command)
            ,m_status(p_code)
{
  // Init these members
  memset(&m_sender,    0,sizeof(SOCKADDR_IN6));
  memset(&m_systemtime,0,sizeof(SYSTEMTIME));
}

// XTOR for HTTP  command from a HTTPServer
HTTPMessage::HTTPMessage(HTTPCommand p_command,HTTPSite* p_site)
            :m_command(p_command)
            ,m_site(p_site)
{
  // Init these members
  memset(&m_sender,    0,sizeof(SOCKADDR_IN6));
  memset(&m_systemtime,0,sizeof(SYSTEMTIME));
}

// XTOR for HTTP command and a URL
HTTPMessage::HTTPMessage(HTTPCommand p_command,CString p_url)
            :m_command(p_command)
{
  // Init these members
  memset(&m_sender,    0,sizeof(SOCKADDR_IN6));
  memset(&m_systemtime,0,sizeof(SYSTEMTIME));

  // Now crackdown the URL
  m_cracked.CrackURL(p_url);
}

// XTOR from another HTTPMessage
// DEFAULT is a 'shallow' copy, without the filebuffer payload!
HTTPMessage::HTTPMessage(HTTPMessage* p_msg,bool p_deep /*=false*/)
            :m_buffer(p_msg->m_buffer)
{
  // Member copies
  m_command           = p_msg->m_command;
  m_status            = p_msg->m_status;
  m_request           = p_msg->m_request;
  m_contentType       = p_msg->m_contentType;
  m_acceptEncoding    = p_msg->m_acceptEncoding;
  m_url               = p_msg->m_url;
  m_site              = p_msg->m_site;
  m_desktop           = p_msg->m_desktop;
  m_ifmodified        = p_msg->m_ifmodified;
  m_sendBOM           = p_msg->m_sendBOM;

  // Taking a duplicate token
  if(DuplicateTokenEx(p_msg->m_token
                     ,TOKEN_DUPLICATE|TOKEN_IMPERSONATE|TOKEN_ALL_ACCESS |TOKEN_READ|TOKEN_WRITE
                     ,NULL
                     ,SecurityImpersonation
                     ,TokenImpersonation
                     ,&m_token) == FALSE)
  {
    m_token = NULL;
  }

  // Sender address 
  memcpy(&m_sender,p_msg->GetSender(),sizeof(SOCKADDR_IN6));
  // Copy system time for if-modified-since
  memcpy(&m_systemtime,&p_msg->m_systemtime,sizeof(SYSTEMTIME));

  // Copy all headers
  m_headers = p_msg->m_headers;
  // Copy all cookies
  m_cookies = p_msg->m_cookies;
  // Copy the cracked URL
  m_cracked = p_msg->m_cracked;

  // Copy routing information
  m_routing = p_msg->GetRouting();


  // Copy the file buffer in case of a 'deep' copy
  // A shallow copy does not create a full buffer copy
  if(p_deep)
  {
    m_buffer = p_msg->m_buffer;
  }
}

// XTOR from a SOAPServerMessage
HTTPMessage::HTTPMessage(HTTPCommand p_command,SOAPMessage* p_msg)
            :m_command(p_command)
{
  m_request       = p_msg->GetRequestHandle();
  m_cookies       = p_msg->GetCookies();
  m_contentType   = p_msg->GetContentType();
  m_desktop       = p_msg->GetRemoteDesktop();
  m_site          = p_msg->GetHTTPSite();
  m_sendBOM       = p_msg->GetSendBOM();
  m_acceptEncoding= p_msg->GetAcceptEncoding();
  m_status        = p_msg->GetStatus();
  memset(&m_systemtime,0,sizeof(SYSTEMTIME));

  // Getting the URL of all parts
  m_url = p_msg->GetURL();
  ParseURL(m_url);

  // Relay ownership of the token
  if(DuplicateTokenEx(p_msg->GetAccessToken()
                     ,TOKEN_DUPLICATE|TOKEN_IMPERSONATE|TOKEN_ALL_ACCESS|TOKEN_READ|TOKEN_WRITE
                     ,NULL
                     ,SecurityImpersonation
                     ,TokenImpersonation
                     ,&m_token) == FALSE)
  {
    m_token = NULL;
  }

  // Get sender (if any) from the soap message
  memcpy(&m_sender,p_msg->GetSender(),sizeof(SOCKADDR_IN6));

  // Copy all headers from the SOAPmessage
  HeaderMap* map = p_msg->GetHeaderMap();
  for(HeaderMap::iterator it = map->begin(); it != map->end(); ++it)
  {
    m_headers[it->first] = it->second;
  }

  // Copy routing information
  m_routing = p_msg->GetRouting();

  // Take care of character encoding
  XMLEncoding encoding = p_msg->GetEncoding();
  CString charset = FindCharsetInContentType(m_contentType);

  if(charset.Left(6).CompareNoCase("utf-16") == 0 || 
     p_msg->GetSendUnicode() || (m_site && m_site->GetSendUnicode()))
  {
    encoding = XMLEncoding::ENC_UTF16;
  }

  int acp = -1;
  switch(encoding)
  {
    case XMLEncoding::ENC_Plain:   acp =    -1; break; // Find Active Code Page
    case XMLEncoding::ENC_UTF8:    acp = 65001; break; // See ConvertWideString.cpp
    case XMLEncoding::ENC_UTF16:   acp =  1200; break; // See ConvertWideString.cpp
    case XMLEncoding::ENC_ISO88591:acp = 28591; break; // See ConvertWideString.cpp
    default:          break;
  }

  // Reconstruct the content type header
  m_contentType = FindMimeTypeInContentType(m_contentType);
  m_contentType.AppendFormat("; charset=%s", CodepageToCharset(acp).GetString());

  // Propagate the BOM settings of the site to this message
  if(m_site && m_site->GetSendSoapBOM())
  {
    p_msg->SetSendBOM(true);
  }

  // Set body and contentLength
  if(acp == 1200)
  {
    p_msg->SetSendUnicode(true);
    CString soap = p_msg->GetSoapMessage();
    uchar* buffer = nullptr;
    int    length = 0;
    if(TryCreateWideString(soap,"",p_msg->GetSendBOM(),&buffer,length))
    {
      SetBody(buffer,length + 2);
      delete [] buffer;
    }
    else
    {
      // We are now officially in error state
      // So produce a status 400 (incoming = client error)
      // or produce a status 500 (outgoing = server error)
      m_status = (m_command == HTTPCommand::http_response) ? HTTP_STATUS_SERVER_ERROR : HTTP_STATUS_BAD_REQUEST;
    }
  }
  else
  {
    // Simply record as our body
    SetBody(p_msg->GetSoapMessageWithBOM());
  }
  // Make sure we have a server name for host headers
  CheckServer();
}

// XTOR from a JSONMessage
HTTPMessage::HTTPMessage(HTTPCommand p_command,JSONMessage* p_msg)
            :m_command(p_command)
{
  m_request        = p_msg->GetRequestHandle();
  m_cookies        = p_msg->GetCookies();
  m_contentType    = p_msg->GetContentType();
  m_acceptEncoding = p_msg->GetAcceptEncoding();
  m_desktop        = p_msg->GetDesktop();
  m_sendBOM        = p_msg->GetSendBOM();
  m_site           = p_msg->GetHTTPSite();
  m_verbTunnel     = p_msg->GetVerbTunneling();
  m_status         = p_msg->GetStatus();
  memset(&m_systemtime,0,sizeof(SYSTEMTIME));

  // Getting the URL of all parts
  m_url = p_msg->GetURL();
  ParseURL(m_url);

  // Relay ownership of the token
  if(DuplicateTokenEx(p_msg->GetAccessToken()
                     ,TOKEN_DUPLICATE|TOKEN_IMPERSONATE|TOKEN_ALL_ACCESS|TOKEN_READ|TOKEN_WRITE
                     ,NULL
                     ,SecurityImpersonation
                     ,TokenImpersonation
                     ,&m_token) == FALSE)
  {
    m_token = NULL;
  }

  // Get sender (if any) from the soap message
  memcpy(&m_sender,p_msg->GetSender(),sizeof(SOCKADDR_IN6));

  // Copy all headers from the SOAPmessage
  HeaderMap* map = p_msg->GetHeaderMap();
  for(HeaderMap::iterator it = map->begin(); it != map->end(); ++it)
  {
    m_headers[it->first] = it->second;
  }

  // Copy all routing
  m_routing = p_msg->GetRouting();

  // Setting the payload of the message
  SetBody(p_msg->GetJsonMessage());

  // Take care of character encoding
  CString charset = FindCharsetInContentType(m_contentType);
  JsonEncoding encoding = p_msg->GetEncoding();

  if(charset.Left(6).CompareNoCase("utf-16") == 0 || 
     p_msg->GetSendUnicode() || (m_site && m_site->GetSendUnicode()))
  {
    encoding = JsonEncoding::JENC_UTF16;
  }

  int acp = -1;
  switch(encoding)
  {
    case JsonEncoding::JENC_Plain:   acp =    -1; break; // Find Active Code Page
    case JsonEncoding::JENC_UTF8:    acp = 65001; break; // See ConvertWideString.cpp
    case JsonEncoding::JENC_UTF16:   acp =  1200; break; // See ConvertWideString.cpp
    case JsonEncoding::JENC_ISO88591:acp = 28591; break; // See ConvertWideString.cpp
    default:                                      break;
  }

  // Reconstruct the content type header
  m_contentType = FindMimeTypeInContentType(m_contentType);
  m_contentType.AppendFormat("; charset=%s", CodepageToCharset(acp).GetString());

  // Propagate the BOM settings of the site to this message
  if(m_site->GetSendJsonBOM())
  {
    p_msg->SetSendBOM(true);
  }

  // Set body and contentLength
  if(acp == 1200)
  {
    p_msg->SetSendUnicode(true);
    CString soap = p_msg->GetJsonMessage(JsonEncoding::JENC_Plain);
    uchar* buffer = nullptr;
    int    length = 0;
    if(TryCreateWideString(soap,"",p_msg->GetSendBOM(),&buffer,length))
    {
      SetBody(buffer,length + 2);
      delete [] buffer;
    }
    else
    {
      // We are now officially in error state
      // So produce a status 400 (incoming = client error)
      // or produce a status 500 (outgoing = server error)
      m_status = (m_command == HTTPCommand::http_response) ? HTTP_STATUS_SERVER_ERROR : HTTP_STATUS_BAD_REQUEST;
    }
  }
  else
  {
    // Simply record as our body
    SetBody(p_msg->GetJsonMessageWithBOM(encoding));
  }
  // Make sure we have a server name for host headers
  CheckServer();
}

// General DTOR
HTTPMessage::~HTTPMessage()
{
  if(m_token)
  {
    CloseHandle(m_token);
    m_token = NULL;
  }
}

// Recycle the object for usage in a return message
void
HTTPMessage::Reset()
{
  m_command       = HTTPCommand::http_response;
  m_status        = HTTP_STATUS_OK;
  m_ifmodified    = false;
  memset(&m_systemtime,0,sizeof(SYSTEMTIME));

  // Reset resulting cracked URL;
  m_cracked.m_scheme = "http";
  m_cracked.m_secure = false;
  m_cracked.m_userName.Empty();
  m_cracked.m_password.Empty();
  m_cracked.m_host.Empty();
  m_cracked.m_port = INTERNET_DEFAULT_HTTP_PORT;
  m_cracked.m_path.Empty();
  m_cracked.m_parameters.clear();
  m_cracked.m_anchor.Empty();

  m_url.Empty();
  m_buffer.Reset();
  m_headers.clear();
  m_routing.clear();

  // Leave access token untouched!
  // Leave cookie cache untouched!
}

// REALLY? Do you want to do this?
// Be sure that the message is now handled by a different site!
bool
HTTPMessage::SetHTTPSite(HTTPSite* p_site)
{
  // Only if it's a real site, and it's controlled
  // by the same HTTPServer object
  if(p_site != nullptr && 
     m_site != nullptr &&
     p_site->GetHTTPServer() == m_site->GetHTTPServer())
  {
    m_site = p_site;
    return true;
  }
  // Otherwise only if we set it the first time
  if(m_site == nullptr && p_site != nullptr)
  {
    m_site = p_site;
    return true;
  }
  return false;
}

// Setting the verb 
bool
HTTPMessage::SetVerb(CString p_verb)
{
  p_verb.MakeUpper();
  for(unsigned ind = 0; ind < sizeof(headers) / sizeof(char*); ++ind)
  {
    if(p_verb.Compare(headers[ind]) == 0)
    {
      m_command = static_cast<HTTPCommand>(ind);
      return true;
    }
  }
  return false;
}

// Getting the verb (HTTPCommand to string)
CString
HTTPMessage::GetVerb()
{
  if(m_command >= HTTPCommand::http_response &&
     m_command <= HTTPCommand::http_last_command)
  {
    return headers[static_cast<unsigned>(m_command)];
  }
  return "";
}

// Getting the whole body as one string of chars
// No embedded '0' are possible!!
CString
HTTPMessage::GetBody()
{
  CString answer;
  uchar*  buffer = NULL;
  size_t  length = 0;

  if(m_buffer.GetHasBufferParts())
  {
    int part = 0;
    while(m_buffer.GetBufferPart(part++,buffer,length))
    {
      if(buffer)
      {
        answer.Append((const char*)buffer,(int)length);
      }
    }
  }
  else
  {
    m_buffer.GetBuffer(buffer,length);
    if(buffer)
    {
      answer.Append((const char*)buffer,(int)length);
    }
  }
  return answer;
}

// Get a RAW copy of the HTTP buffer
// Size it big enough for Unicode translations
void
HTTPMessage::GetRawBody(uchar** p_body,size_t& p_length)
{
  p_length = m_buffer.GetLength();

  // No body in this buffer (FILE?)
  if(p_length == 0)
  {
    *p_body = NULL;
    return;
  }
  // Allocate a buffer, big enough for Unicode conversion
  *p_body  = new uchar[p_length + 2];

  if(m_buffer.GetHasBufferParts())
  {
    int    part = 0;
    uchar* buf  = nullptr;
    size_t len  = 0;
    uchar* dest = *p_body;

    while(m_buffer.GetBufferPart(part++,buf,len))
    {
      memcpy(dest,buf,len);
      dest += len;
    }
  }
  else
  {
    uchar* buf = nullptr;
    size_t len = 0;
    
    m_buffer.GetBuffer(buf,len);
    memcpy(*p_body,buf,len);
  }
  // Prepare for character (Unicode) conversion
  (*p_body)[p_length    ] = 0;
  (*p_body)[p_length + 1] = 0;
}

void 
HTTPMessage::SetAcceptEncoding(CString p_encoding)
{
  m_acceptEncoding = p_encoding;
  m_acceptEncoding.MakeLower();
}

Cookie*
HTTPMessage::GetCookie(unsigned p_ind)
{
  return m_cookies.GetCookie(p_ind);
}

CString
HTTPMessage::GetCookieValue(unsigned p_ind /*=0*/,CString p_metadata /*=""*/)
{
  Cookie* cookie = m_cookies.GetCookie(p_ind);
  if(cookie)
  {
    return cookie->GetValue(p_metadata);
  }
  return "";
}

CString
HTTPMessage::GetCookieValue(CString p_name /*=""*/,CString p_metadata /*=""*/)
{
  Cookie* cookie = m_cookies.GetCookie(p_name);
  if(cookie)
  {
    return cookie->GetValue(p_metadata);
  }
  return "";
}

void
HTTPMessage::SetReadBuffer(bool p_read,size_t p_length)
{
  m_readBuffer    = p_read;
  m_contentLength = p_length;
};

void
HTTPMessage::SetSender(PSOCKADDR_IN6 p_address)
{
  memcpy(&m_sender,p_address,sizeof(SOCKADDR_IN6));
}

void
HTTPMessage::SetReceiver(PSOCKADDR_IN6 p_address)
{
  memcpy(&m_receiver,p_address,sizeof(SOCKADDR_IN6));
}

void 
HTTPMessage::SetFile(CString& p_fileName)
{
  m_buffer.Reset();
  m_buffer.SetFileName(p_fileName);
}

CString
HTTPMessage::GetRoute(int p_index)
{
  if(p_index >= 0 && p_index < (int)m_routing.size())
  {
    return m_routing[p_index];
  }
  return "";
}

void 
HTTPMessage::SetCookie(CString p_fromHttp)
{
  Cookie monster(p_fromHttp);
  m_cookies.AddCookie(monster);
}

void 
HTTPMessage::SetCookie(CString p_name
                      ,CString p_value
                      ,CString p_metadata /*= ""*/
                      ,bool    p_secure   /*= false*/
                      ,bool    p_httpOnly /*= false*/)
{
  Cookie monster;
  monster.SetCookie(p_name,p_value,p_metadata,p_secure,p_httpOnly);
  m_cookies.AddCookie(monster);
}

// From "Cookie:" only.
// Can have multiple cookies, but NO attributes
void 
HTTPMessage::SetCookiePairs(CString p_cookies)
{
  while(p_cookies.GetLength())
  {
    int pos = p_cookies.Find(';');
    if(pos >= 0)
    {
      CString cookie = p_cookies.Left(pos);
      p_cookies = p_cookies.Mid(pos+1);
      SetCookie(cookie);
    }
    else
    {
      SetCookie(p_cookies);
      return;
    }
  }
}

// Parse the URL, true if legal
bool
HTTPMessage::ParseURL(CString p_url)
{
  if(m_cracked.CrackURL(p_url) == false)
  {
    m_cracked.Reset();
    return false;
  }
  return true;
}

// Reparse URL after setting a part of the URL
void
HTTPMessage::ReparseURL()
{
  m_url = m_cracked.URL();
}

// Check that we have a server string (for host headers)
void
HTTPMessage::CheckServer()
{
  if(m_cracked.m_host.IsEmpty())
  {
    if(!m_url.IsEmpty())
    {
      ParseURL(m_url);
    }
  }
}

// Getting the user/password from the base authorization
// for use in this and other programs for autorization purposes
void
HTTPMessage::SetAuthorization(CString& p_authorization)
{
  if(p_authorization.Left(6).CompareNoCase("basic ") == 0)
  {
    p_authorization.Delete(0, 6);
    Base64 base;
    CString decrypted;
    decrypted.GetBufferSetLength((int)base.Ascii_length(p_authorization.GetLength()) + 1);
    base.Decrypt((const unsigned char*) p_authorization.GetString()
                ,p_authorization.GetLength()
                ,(unsigned char*)decrypted.GetString());
    int from = decrypted.Find(':');
    if(from > 0)
    {
      SetUser(decrypted.Left(from));
      SetPassword(decrypted.Mid(from + 1));
    }
  }
}

// The names of all 'known headers' of the HTTP 1.1. protocol
// are in the order of the "enum_HTTP_HEADER_ID" of the Marlin-Server library!!
// DO NOT CHANGE THE ORDER, because the array/header information will be
// delivered in this order from the MS-Windows kernel
//
char* header_fields[HttpHeaderMaximum] =
{
  /*  0 */   "cache-control"
  /*  1 */  ,"connection"
  /*  2 */  ,"date"
  /*  3 */  ,"keep-alive"
  /*  4 */  ,"trailer"
  /*  5 */  ,"pragma"
  /*  6 */  ,"transfer-encoding"
  /*  7 */  ,"upgrade"
  /*  8 */  ,"via" 
  /*  9 */  ,"warning"
  /* 10 */  ,"allow"
  /* 11 */  ,"content-length"
  /* 12 */  ,"content-type"
  /* 13 */  ,"content-encoding"
  /* 14 */  ,"content-language"
  /* 15 */  ,"content-location"
  /* 16 */  ,"content-md5"
  /* 17 */  ,"content-range"
  /* 18 */  ,"expires"
  /* 19 */  ,"last-modified"
  /* 20 */  ,"accept"
  /* 21 */  ,"accept-charset"
  /* 22 */  ,"accept-encoding"
  /* 23 */  ,"accept-language"
  /* 24 */  ,"authorization"
  /* 25 */  ,"cookie"
  /* 26 */  ,"expect"
  /* 27 */  ,"from"
  /* 28 */  ,"host"
  /* 29 */  ,"if-match"
  /* 30 */  ,"if-modified-since"
  /* 31 */  ,"if-none-match"
  /* 32 */  ,"if-range"
  /* 33 */  ,"if-unmodified-since"
  /* 34 */  ,"max-forwards"
  /* 35 */  ,"proxy-authorization"
  /* 36 */  ,"referer"
  /* 37 */  ,"range"
  /* 38 */  ,"te"
  /* 39 */  ,"translate"
  /* 40 */  ,"user-agent"
};

char* header_response[10]
{
  /* 20 */   "accept-ranges"
  /* 21 */  ,"age"
  /* 22 */  ,"etag"
  /* 23 */  ,"location"
  /* 24 */  ,"proxy-authenticate"
  /* 25 */  ,"retry-after"
  /* 26 */  ,"server"
  /* 27 */  ,"set-cookie"
  /* 28 */  ,"vary"
  /* 29 */  ,"www-authenticate"
};

void 
HTTPMessage::SetAllHeaders(PHTTP_REQUEST_HEADERS p_headers)
{
  // Add all unknown headers
  SetUnknownHeaders(p_headers);

  // Add all known headers
  for(int ind = 0; ind < HttpHeaderMaximum; ++ind)
  {
    if(p_headers->KnownHeaders[ind].RawValueLength)
    {
      CString value(p_headers->KnownHeaders[ind].pRawValue);
      CString name(header_fields[ind]);
      AddHeader(name,value);
    }
  }
}

void
HTTPMessage::SetUnknownHeaders(PHTTP_REQUEST_HEADERS p_headers)
{
  // Cycle through all unknown headers
  for(int ind = 0; ind < p_headers->UnknownHeaderCount; ++ind)
  {
    CString name (p_headers->pUnknownHeaders[ind].pName);
    CString value(p_headers->pUnknownHeaders[ind].pRawValue);
    AddHeader(name,value);
  }
}

// Add a header by known header-id
void    
HTTPMessage::AddHeader(HTTP_HEADER_ID p_id,CString p_value)
{
  int maximum = HttpHeaderMaximum;

  if(p_id >= 0 && p_id < maximum)
  {
    CString name = p_id < HttpHeaderAcceptRanges ? header_fields[p_id] : header_response[p_id - HttpHeaderAcceptRanges];
    AddHeader(name,p_value);
  }
}

// Add header outside protocol
void
HTTPMessage::AddHeader(CString p_name,CString p_value,bool p_lower /*=true*/)
{
  if(p_lower)
  {
    p_name.MakeLower();
  }
  m_headers.insert(std::make_pair(p_name,p_value));
}

// Finding a header by lowercase name
CString
HTTPMessage::GetHeader(CString p_name)
{
  p_name.MakeLower();

  HeaderMap::iterator it = m_headers.find(p_name);
  if(it != m_headers.end())
  {
    return it->second;
  }
  return "";
}

// Delete a header by name (case (in)sensitive)
void    
HTTPMessage::DelHeader(CString p_name)
{
  HeaderMap::iterator it = m_headers.find(p_name);
  if(it != m_headers.end())
  {
    m_headers.erase(it);
    return;
  }
  p_name.MakeLower();
  it = m_headers.find(p_name);
  if(it != m_headers.end())
  {
    m_headers.erase(it);
  }
}

// Convert system time to HTTP time string
// leaves the m_systemtime member untouched!!
CString
HTTPMessage::HTTPTimeFormat(PSYSTEMTIME p_systime /*=NULL*/)
{
  CString    result;
  SYSTEMTIME sTime;

  // Getting the current time if not givven
  // WINDOWS API does also this if p_systime is empty
  if(p_systime == nullptr)
  {
    p_systime = &sTime;
    ::GetSystemTime(&sTime);
  }
  // Convert the current time to HTTP format.
  if(!HTTPTimeFromSystemTime(p_systime,result))
  {
    // Empty result
    result.Empty();
  }
  return result;
}

// Convert HTTP time string to system time
bool    
HTTPMessage::SetHTTPTime(CString p_timestring)
{
  if(!HTTPTimeToSystemTime(p_timestring,&m_systemtime))
  {
    memset(&m_systemtime,0,sizeof(SYSTEMTIME));
    return false;
  }
  return true;
}

// Change POST method to PUT/MERGE/PATCH/DELETE
// Used for incoming traffic through VERB-Tunneling
bool
HTTPMessage::FindVerbTunneling()
{
  HTTPCommand oldCommand = m_command;
  CString method1 = GetHeader("X-HTTP-Method");           // The 'Microsoft/IIS' way
  CString method2 = GetHeader("X-HTTP-Method-Override");  // The 'Google/Gdata' way
  CString method3 = GetHeader("X-METHOD-OVERRIDE");       // The 'IBM/Domino' way
  if(method1.IsEmpty() && method2.IsEmpty() && method3.IsEmpty())
  {
    return false;
  }
  // Take first non-empty override / Last has precedence!
  CString method = method3.IsEmpty() ? (method2.IsEmpty() ? method1 : method2) : method3;
  if(!method.IsEmpty())
  {
    method.Trim();
    unsigned max = static_cast<unsigned>(HTTPCommand::http_last_command);
    for(unsigned ind = 1; ind <= max; ++ind)
    {
      if(method.CompareNoCase(headers[ind]) == 0)
      {
        m_command = static_cast<HTTPCommand>(ind);
      }
    }
  }
  // Did we change our identity?
  return m_verbTunnel = (oldCommand != m_command);
}

// Use POST method for PUT/MERGE/PATCH/DELETE
// Also known as VERB-Tunneling
bool
HTTPMessage::UseVerbTunneling()
{
  if(m_verbTunnel)
  {
    if(m_command == HTTPCommand::http_put   ||
       m_command == HTTPCommand::http_merge ||
       m_command == HTTPCommand::http_patch ||
       m_command == HTTPCommand::http_delete )
    {
      // Change of identity!
      CString method = headers[static_cast<unsigned>(m_command)];
      AddHeader("X-HTTP-Method-Override",method);
      m_command = HTTPCommand::http_post;
      return true;
    }
  }
  return false;
}

// Accept a MultiPartBuffer in this message
bool    
HTTPMessage::SetMultiPartFormData(MultiPartBuffer* p_buffer)
{
  // Check that we have a multi-part buffer and that it is filled!
  if(!p_buffer || p_buffer->GetParts() == 0)
  {
    return false;
  }

  // Create accept header
  CString accept = p_buffer->CalculateAcceptHeader();
  AddHeader("Accept",accept);

  // Get type of formdata buffer
  FormDataType type = p_buffer->GetFormDataType();
  switch(type)
  {
    case FD_URLENCODED:  return SetMultiPartURL(p_buffer);
    case FD_MULTIPART:   return SetMultiPartBuffer(p_buffer);
    case FD_UNKNOWN:     [[fallthrough]];
    default:             return false;
  }
  return false;
}

// RFC 7578: Fill url-encoding
bool
HTTPMessage::SetMultiPartURL(MultiPartBuffer* p_buffer)
{
  SetContentType(p_buffer->GetContentType());
  if(m_command == HTTPCommand::http_post)
  {
    return SetMultiPartURLPost(p_buffer);
  }
  // else put/get/patch etc.
  return SetMultiPartURLGet(p_buffer);
}

bool
HTTPMessage::SetMultiPartURLGet(MultiPartBuffer* p_buffer)
{
  unsigned ind = 0;
  MultiPart* part = p_buffer->GetPart(ind);
  do
  {
    m_cracked.SetParameter(part->GetName(),part->GetData());
    // Getting the next part
    part = p_buffer->GetPart(++ind);
  }
  while(part);

  return true;
}

bool
HTTPMessage::SetMultiPartURLPost(MultiPartBuffer* p_buffer)
{
  CrackedURL url;
  unsigned ind = 0;
  MultiPart* part = p_buffer->GetPart(ind);
  do
  {
    url.SetParameter(part->GetName(),part->GetData());
    // Getting the next part
    part = p_buffer->GetPart(++ind);
  }
  while(part);

  // Set parameters together as the body
  CString parameters = url.AbsolutePath();
  parameters.TrimLeft('?');
  SetBody(parameters);

  return true;
}

bool
HTTPMessage::SetMultiPartBuffer(MultiPartBuffer* p_buffer)
{
  bool result = false;

  // Create the correct content type
  CString boundary = p_buffer->CalculateBoundary();
  CString contentType(p_buffer->GetContentType());
  contentType += "; boundary=" + boundary;
  SetContentType(contentType);

  unsigned ind = 0;
  MultiPart* part = p_buffer->GetPart(ind);
  do
  {
    CString header = part->CreateHeader(boundary,p_buffer->GetFileExtensions());
    m_buffer.AddBuffer((uchar*)header.GetString(),(size_t)header.GetLength());
    if(part->GetFileName().IsEmpty())
    {
      // Add data
      m_buffer.AddBufferCRLF((uchar*)part->GetData().GetString()
                            ,(size_t)part->GetData().GetLength());
    }
    else
    {
      // Add file buffer, by buffer COPY!!
      uchar* buf  = nullptr;
      size_t size = 0L;
      FileBuffer* partBuffer = part->GetBuffer();
      partBuffer->GetBuffer(buf,size);
      m_buffer.AddBufferCRLF(buf,size);
    }
    // At least one part added
    result = true;
    // Getting the next part
    part = p_buffer->GetPart(++ind);
  }
  while(part);

  // The ending boundary after the last part
  // Beware. Does NOT end with CR-LF but two hypens!
  CString lastBoundary = "--" + boundary + "--";
  m_buffer.AddBuffer((uchar*)lastBoundary.GetString()
                    ,(size_t)lastBoundary.GetLength());

  return result;
}


// HTTPMessages can be stored elsewhere. Use the reference mechanism to add/drop references
// With the drop of the last reference, the object WILL destroy itself

void
HTTPMessage::AddReference()
{
  InterlockedIncrement(&m_references);
}

void
HTTPMessage::DropReference()
{
  if(InterlockedDecrement(&m_references) <= 0)
  {
    delete this;
  }
}

