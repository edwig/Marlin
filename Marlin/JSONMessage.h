/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: JSONMessage.h
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
#pragma once
#include "Cookie.h"
#include "HTTPMessage.h"
#include "XMLMessage.h"
#include "Routing.h"
#include "http.h"
#include "bcd.h"
#include <vector>
#include <xstring>

// Forward declaration
class HTTPSite;
class JSONvalue;
class JSONpair;
class JSONParser;
class JSONParserSOAP;

// The JSON constants
//
enum class JsonConst
{
  JSON_NONE  = -1
 ,JSON_NULL  = 0
 ,JSON_FALSE = 1
 ,JSON_TRUE  = 2
};

// The JSON data types
//
enum class JsonType
{
  JDT_string      = 1
 ,JDT_number_int  = 2
 ,JDT_number_bcd  = 3 
 ,JDT_array       = 4
 ,JDT_object      = 5
 ,JDT_const       = 6
};

// JSON coding of the message
enum class JsonEncoding
{
   JENC_Plain    = 0  // No action taken, use GetACP(): windows-1252 in "The Netherlands"
  ,JENC_UTF8     = 1  // Default in XML and HTML5
  ,JENC_UTF16    = 2  // Default ECMA-404 The JSON Data Interchange Standard
  ,JENC_ISO88591 = 3  // Default (X)HTML 4.01 standard
};


// Numbers are integers or exponentials
typedef struct _jsonNumber
{
  long m_intNumber;
  bcd  m_bcdNumber;
}
JSONnumber;

using JSONarray  = std::vector<JSONvalue>;
using JSONobject = std::vector<JSONpair>;

// The general JSON value
//
class JSONvalue
{
public:
  JSONvalue();
  JSONvalue(JSONvalue* p_other);
 ~JSONvalue();

  // SETTERS
  void        SetDatatype(JsonType p_type);
  void        SetValue(CString    p_value);
  void        SetValue(JsonConst  p_value);
  void        SetValue(JSONobject p_value);
  void        SetValue(JSONarray  p_value);
  void        SetValue(int        p_value);
  void        SetValue(bcd        p_value);

  // GETTERS
  JsonType    GetDataType()  { return m_type;     };
  CString     GetString()    { return m_string;   };
  int         GetNumberInt() { return m_number.m_intNumber;  };
  bcd         GetNumberBcd() { return m_number.m_bcdNumber;  };
  JSONarray&  GetArray()     { return m_array;    };
  JSONobject& GetObject()    { return m_object;   };
  JsonConst   GetConstant()  { return m_constant; };
  CString     GetAsJsonString(bool p_white,bool p_utf8,unsigned p_level = 0);

  // OPERATORS
  JSONvalue&  operator=(JSONvalue& p_other);

  // JSONvalue's can be stored elsewhere. Use the reference mechanism to add/drop references
  // With the drop of the last reference, the object WILL destroy itself
  void        AddReference();
  void        DropReference();
private:
  // What's in there: the data type
  JsonType   m_type       { JsonType::JDT_const };
  // Depending on m_type: one of these
  CString    m_string;
  JSONnumber m_number     { 0 };
  JSONarray  m_array;
  JSONobject m_object;
  JsonConst  m_constant   { JsonConst::JSON_NONE };
  long       m_references { 0 };   // Externally referenced
};

// Objects are made of pairs
//
class JSONpair
{
public:
  CString   m_name;
  JSONvalue m_value;
};

//////////////////////////////////////////////////////////////////////////
//
// This is the JSON message
//
class JSONMessage
{
public:
  JSONMessage();
  // XTOR: For incoming UTF-8 String
  JSONMessage(CString p_message);
  // XTOR: Internal construction from MBCS string
  JSONMessage(CString p_message,bool p_whitespace,JsonEncoding p_encoding);
  // XTOR: Outgoing to a URL
  JSONMessage(CString p_message,CString p_url);
  // XTOR: From another XXXmessage
  JSONMessage(JSONMessage* p_other);
  JSONMessage(HTTPMessage* p_message);
  JSONMessage(SOAPMessage* p_message);
  // General DTOR
 ~JSONMessage();

  // Resetting the message for answering
  void Reset();
  // Create from message stream
  bool ParseMessage(CString p_message,JsonEncoding p_encoding = JsonEncoding::JENC_Plain);
  // Load from file
  bool LoadFile(const CString& p_fileName);
  // Save to file
  bool SaveFile(const CString& p_fileName, bool p_withBom = false);

  // Finding value nodes within the JSON structure
  JSONvalue*      FindValue (CString    p_name,               bool p_object = false);
  JSONvalue*      FindValue (JSONvalue* p_from,CString p_name,bool p_object = false);
  JSONpair*       FindPair  (CString    p_name);
  JSONpair*       FindPair  (JSONvalue* p_value, CString p_name);
  JSONvalue*      GetArrayElement (JSONvalue* p_array, int p_index);
  JSONpair*       GetObjectElement(JSONvalue* p_object,int p_index);
  CString         GetValueString  (CString p_name);
  long            GetValueInteger (CString p_name);
  bcd             GetValueNumber  (CString p_name);
  JsonConst       GetValueConstant(CString p_name);

  // GETTERS
  CString         GetJsonMessage       (JsonEncoding p_encoding = JsonEncoding::JENC_Plain);
  CString         GetJsonMessageWithBOM(JsonEncoding p_encoding = JsonEncoding::JENC_UTF8);
  JSONvalue&      GetValue()          { return *m_value;                };
  CString         GetURL()            { return m_url;                   };
  CrackedURL&     GetCrackedURL()     { return m_cracked;               };
  unsigned        GetStatus()         { return m_status;                };
  HTTP_OPAQUE_ID  GetRequestHandle()  { return m_request;               };
  HTTPSite*       GetHTTPSite()       { return m_site;                  };
  HeaderMap*      GetHeaderMap()      { return &m_headers;              };
  bool            GetSecure()         { return m_cracked.m_secure;      };
  CString         GetUser()           { return m_cracked.m_userName;    };
  CString         GetPassword()       { return m_cracked.m_password;    };
  CString         GetServer()         { return m_cracked.m_host;        };
  int             GetPort()           { return m_cracked.m_port;        };
  CString         GetAbsolutePath()   { return m_cracked.AbsolutePath();};
  Cookies&        GetCookies()        { return m_cookies;               };
  HANDLE          GetAccessToken()    { return m_token;                 };
  PSOCKADDR_IN6   GetSender()         { return &m_sender;               };
  UINT            GetDesktop()        { return m_desktop;               };
  bool            GetErrorState()     { return m_errorstate;            };
  CString         GetLastError()      { return m_lastError;             };
  bool            GetWhitespace()     { return m_whitespace;            };
  JsonEncoding    GetEncoding()       { return m_encoding;              };
  CString         GetAcceptEncoding() { return m_acceptEncoding;        };
  bool            GetSendBOM()        { return m_sendBOM;               };
  bool            GetSendUnicode()    { return m_sendUnicode;           };
  bool            GetVerbTunneling()  { return m_verbTunnel;            };
  bool            GetIncoming()       { return m_incoming;              };
  bool            GetHasBeenAnswered(){ return m_request == NULL;       };
  CString         GetReferrer()       { return m_referrer;              };
  Routing&        GetRouting()        { return m_routing;               };
  CString         GetHeader(CString p_name);
  CString         GetRoute(int p_index);
  CString         GetContentType();
  CString         GetVerb();

  // SETTERS
  void            SetURL(CString& p_url);
  void            SetIncoming(bool p_incoming)            { m_incoming           = p_incoming; };
  void            SetErrorstate(bool p_state)             { m_errorstate         = p_state;    };
  void            SetLastError(CString p_error)           { m_lastError          = p_error;    };
  void            SetSecure(bool p_secure)                { m_cracked.m_secure   = p_secure;   ReparseURL(); };
  void            SetUser(CString p_user)                 { m_cracked.m_userName = p_user;     ReparseURL(); };
  void            SetPassword(CString p_password)         { m_cracked.m_password = p_password; ReparseURL(); };
  void            SetServer(CString p_server)             { m_cracked.m_host     = p_server;   ReparseURL(); };
  void            SetPort(int p_port)                     { m_cracked.m_port     = p_port;     ReparseURL(); };
  void            SetAbsolutePath(CString p_path)         { m_cracked.m_path     = p_path;     ReparseURL(); };
  void            SetStatus(unsigned p_status)            { m_status             = p_status;   };
  void            SetDesktop(UINT p_desktop)              { m_desktop            = p_desktop;  };
  void            SetRequestHandle(HTTP_OPAQUE_ID p_id)   { m_request            = p_id;       };
  void            SetVerb(CString p_verb)                 { m_verb               = p_verb;     };
  void            SetCookies(Cookies& p_cookies)          { m_cookies            = p_cookies;  };
  void            SetContentType(CString p_type)          { m_contentType        = p_type;     };
  void            SetWhitespace(bool p_white)             { m_whitespace         = p_white;    };
  void            SetSendBOM(bool p_bom)                  { m_sendBOM            = p_bom;      };
  void            SetVerbTunneling(bool p_tunnel)         { m_verbTunnel         = p_tunnel;   };
  void            SetHasBeenAnswered()                    { m_request            = NULL;       };
  void            SetAcceptEncoding(CString p_encoding);
  void            AddHeader(CString p_name,CString p_value);
  void            SetEncoding(JsonEncoding p_encoding);
  void            SetSendUnicode(bool p_unicode);

  // Use POST method for PUT/MERGE/PATCH/DELETE
  bool            UseVerbTunneling();

  // JSONMessages can be stored elsewhere. Use the reference mechanism to add/drop references
  // With the drop of the last reference, the object WILL destroy itself
  void            AddReference();
  void            DropReference();

private:
  // Parse the URL, true if legal
  bool ParseURL(CString p_url);
  // Re-parse URL after setting a part of the URL
  void ReparseURL();

  // The message is contained in a JSON value
  JSONvalue*      m_value;

  // Parser for the XML texts
  friend          JSONParser;
  friend          JSONParserSOAP;
  // Message handling from here
  bool            m_whitespace  { false };                      // Whitespace preservation
  bool            m_incoming    { false };                      // Incoming JSON message
  bool            m_errorstate  { false };                      // Internal for parsing errors
  CString         m_lastError;                                  // Error as text
  JsonEncoding    m_encoding    { JsonEncoding::JENC_UTF8 };    // Encoding details
  bool            m_sendUnicode { false };                      // Send message in UTF-16 Unicode
  bool            m_sendBOM     { false };                      // Pre-pend message with UTF-8 or UTF-16 Byte-Order-Mark
  bool            m_verbTunnel  { false };                      // HTTP-VERB Tunneling used
  // DESTINATION
  CString         m_url;                                        // Full URL of the JSON service
  unsigned        m_status      { HTTP_STATUS_OK };             // HTTP status return code
  CrackedURL      m_cracked;                                    // Cracked down URL (all parts)
  CString         m_verb;                                       // HTTP verb, default = POST
  HTTP_OPAQUE_ID  m_request     { NULL };                       // Request it must answer
  HTTPSite*       m_site        { nullptr };                    // Site for which message is received
  HeaderMap       m_headers;                                    // Extra HTTP headers (incoming / outgoing)
  // Message details
  CString         m_referrer;                                   // Referrer of the message
  CString         m_contentType;                                // Content type of JSON message
  CString         m_acceptEncoding;                             // Accepted HTTP compression encoding
  Cookies         m_cookies;                                    // Cookies
  HANDLE          m_token       { NULL };                       // Security access token
  SOCKADDR_IN6    m_sender;                                     // Senders address
  UINT            m_desktop     { 0 };                          // Senders remote desktop
  long            m_references  { 0 };                          // Externally referenced
  Routing         m_routing;                                    // Routing information from HTTP
};
