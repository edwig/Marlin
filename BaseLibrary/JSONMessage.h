/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: JSONMessage.h
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
#include "Cookie.h"
#include "HTTPMessage.h"
#include "XMLMessage.h"
#include "Routing.h"
#include "http.h"
#include <vector>
#include <xstring>

// Forward declaration
class HTTPSite;
class JSONvalue;
class JSONpair;
class JSONParser;
class JSONParserSOAP;
class JSONPointer;

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

using JSONarray  = std::vector<JSONvalue>;
using JSONobject = std::vector<JSONpair>;

// The general JSON value
//
class JSONvalue
{
public:
  JSONvalue();
  explicit JSONvalue(const JSONvalue* p_other);
  explicit JSONvalue(const JsonType   p_type);
  explicit JSONvalue(const JsonConst  p_value);
  explicit JSONvalue(const XString    p_value);
  explicit JSONvalue(const int        p_value);
  explicit JSONvalue(const bcd&       p_value);
  explicit JSONvalue(const bool       p_value);
 ~JSONvalue();

  // SETTERS
  void        SetDatatype(JsonType p_type);
  void        SetValue(XString     p_value);
  void        SetValue(LPCTSTR     p_value);
  void        SetValue(JsonConst   p_value);
  void        SetValue(JSONobject  p_value);
  void        SetValue(JSONarray   p_value);
  void        SetValue(int         p_value);
  void        SetValue(const bcd&  p_value);
  void        SetMark (bool        p_mark);

  // GETTERS
  JsonType    GetDataType()  const { return m_type;     }
  XString     GetString()    const { return m_string;   }
  int         GetNumberInt() const { return m_intNumber;}
  bcd         GetNumberBcd() const { return m_bcdNumber;}
  JsonConst   GetConstant()  const { return m_constant; }
  bool        GetMark()      const { return m_mark;     }
  JSONarray&  GetArray()           { return m_array;    }
  JSONobject& GetObject()          { return m_object;   }
  XString     GetAsJsonString(bool p_white,Encoding p_encoding,unsigned p_level = 0);

  // FUNCTIONS
  void        JsonReplace(XString p_namePattern,XString p_tofind,XString p_replace,int& p_number,bool p_caseSensitive = true);

  // Specials for the empty/null state
  void        Empty();
  bool        IsEmpty();
  // Specials for construction: add to an array/object
  void        Add(JSONvalue& p_value);
  void        Add(JSONpair&  p_value);

  // OPERATORS

  // Assignment of another value
  JSONvalue&  operator=(const JSONvalue&  p_other);
  JSONvalue&  operator=(const XString&    p_other);
  JSONvalue&  operator=(      LPCTSTR p_other);
  JSONvalue&  operator=(const int&        p_other);
  JSONvalue&  operator=(const bcd&        p_other);
  JSONvalue&  operator=(JsonConst&        p_other);
  JSONvalue&  operator=(const bool&       p_other);
  // Getting the value from an JSONarray
  JSONvalue&  operator[](int p_index);
  // Getting the value from an JSONobject
  JSONvalue&  operator[](XString p_name);

  // JSONvalue's can be stored elsewhere. Use the reference mechanism to add/drop references
  // With the drop of the last reference, the object WILL destroy itself
  void        AddReference();
  void        DropReference();

private:
  void        JsonReplaceObject(XString p_namePattern,XString p_tofind,XString p_replace,int& p_number,bool p_caseSensitive);
  void        JsonReplaceArray (XString p_namePattern,XString p_tofind,XString p_replace,int& p_number,bool p_caseSensitive);

  // JSONPointer may have access to the objects
  friend     JSONPointer;

  // What's in there: the data type
  JsonType   m_type       { JsonType::JDT_const };
  // Depending on m_type: one of these
  XString    m_string;
  int        m_intNumber  { 0 };
  bcd        m_bcdNumber;
  JSONarray  m_array;
  JSONobject m_object;
  JsonConst  m_constant   { JsonConst::JSON_NONE };
  // Externally referenced
  long       m_references { 0 };   
  bool       m_mark       { false };
};

// Objects are made of pairs
//
class JSONpair
{
public:
  JSONpair() = default;
  explicit JSONpair(XString p_name);
  explicit JSONpair(XString p_name,JsonType    p_type);
  explicit JSONpair(XString p_name,JSONvalue&  p_value);
  explicit JSONpair(XString p_name,XString     p_value);
  explicit JSONpair(XString p_name,LPCTSTR     p_value);
  explicit JSONpair(XString p_name,int         p_value);
  explicit JSONpair(XString p_name,const bcd&  p_value);
  explicit JSONpair(XString p_name,bool        p_value);
  explicit JSONpair(XString p_name,JsonConst   p_value);

  // Currently public. Will be moved to private in a future version
  XString   m_name;
  JSONvalue m_value;

  void        SetName(XString p_name)             { m_name = p_name;             }
  void        SetDatatype(JsonType       p_type)  { m_value.SetDatatype(p_type); }
  void        SetValue(XString           p_value) { m_value.SetValue(p_value);   }
  void        SetValue(LPCTSTR           p_value) { m_value.SetValue(p_value);   }
  void        SetValue(JsonConst         p_value) { m_value.SetValue(p_value);   }
  void        SetValue(const JSONobject& p_value) { m_value.SetValue(p_value);   }
  void        SetValue(const JSONarray&  p_value) { m_value.SetValue(p_value);   }
  void        SetValue(int               p_value) { m_value.SetValue(p_value);   }
  void        SetValue(const bcd&        p_value) { m_value.SetValue(p_value);   }
  void        SetValue(bool              p_value) { m_value.SetValue(p_value);   }

  JsonType    GetDataType()  const { return m_value.GetDataType();  }
  XString     GetString()    const { return m_value.GetString();    }
  int         GetNumberInt() const { return m_value.GetNumberInt(); }
  bcd         GetNumberBcd() const { return m_value.GetNumberBcd(); }
  JsonConst   GetConstant()  const { return m_value.GetConstant();  }
  JSONarray&  GetArray()           { return m_value.GetArray();     }
  JSONobject& GetObject()          { return m_value.GetObject();    }

  // Specials for construction: add to an array/object
  void        Add(JSONvalue& p_value) { m_value.Add(p_value); }
  void        Add(JSONpair& p_value)  { m_value.Add(p_value); }

  JSONpair&   operator=(const JSONpair&);
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
  explicit JSONMessage(XString p_message);
  // XTOR: Internal construction from MBCS string
  explicit JSONMessage(XString p_message,bool p_whitespace,Encoding p_encoding);
  // XTOR: Outgoing to a URL
  explicit JSONMessage(XString p_message,XString p_url);
  // XTOR: From another XXXmessage
  explicit JSONMessage(JSONMessage* p_other);
  explicit JSONMessage(HTTPMessage* p_message);
  explicit JSONMessage(SOAPMessage* p_message);
  // General DTOR
 ~JSONMessage();

  // Resetting the message for answering
  void Reset(bool p_resetURL = true);
  // Create from message stream
  bool ParseMessage(XString p_message,Encoding p_encoding = Encoding::Default);
  // Load from file
  bool LoadFile(const XString& p_fileName);
  // Save to file
  bool SaveFile(const XString& p_fileName);

  // Finding value nodes within the JSON structure
  JSONvalue*      FindValue (XString    p_name,               bool p_recurse = true,bool p_object = false,JsonType* p_type = nullptr);
  JSONvalue*      FindValue (JSONvalue* p_from,XString p_name,bool p_recurse = true,bool p_object = false,JsonType* p_type = nullptr);
  JSONpair*       FindPair  (XString    p_name,bool p_recursief = true);
  JSONpair*       FindPair  (JSONvalue* p_value,XString p_name,bool p_recursief = true);
  JSONvalue*      GetArrayElement (JSONvalue* p_array, int p_index);
  JSONpair*       GetObjectElement(JSONvalue* p_object,int p_index);
  XString         GetValueString  (XString p_name);
  long            GetValueInteger (XString p_name);
  bcd             GetValueNumber  (XString p_name);
  JsonConst       GetValueConstant(XString p_name);
  bool            AddNamedObject(XString p_name,const JSONobject& p_object,bool p_forceArray = false);
  int             JsonReplace(XString p_namePattern,XString p_tofind,XString p_replace,bool p_caseSensitive = true);

  // GETTERS
  XString         GetJsonMessage       (Encoding p_encoding = Encoding::Default) const;
  XString         GetJsonMessageWithBOM(Encoding p_encoding = Encoding::UTF8)   const;
  JSONvalue&      GetValue() const         { return *m_value;                }
  XString         GetURL() const           { return m_url;                   }
  const CrackedURL& GetCrackedURL() const  { return m_cracked;               }
  unsigned        GetStatus() const        { return m_status;                }
  HTTP_OPAQUE_ID  GetRequestHandle() const { return m_request;               }
  HTTPSite*       GetHTTPSite() const      { return m_site;                  }
  const HeaderMap* GetHeaderMap() const    { return &m_headers;              }
  XString         GetUser() const          { return m_user;                  }
  XString         GetPassword() const      { return m_password;              }
  bool            GetSecure() const        { return m_cracked.m_secure;      }
  XString         GetServer() const        { return m_cracked.m_host;        }
  int             GetPort() const          { return m_cracked.m_port;        }
  XString         GetAbsolutePath() const  { return m_cracked.AbsolutePath();}
  const Cookies&  GetCookies() const       { return m_cookies;               }
  HANDLE          GetAccessToken() const   { return m_token;                 }
  PSOCKADDR_IN6   GetSender()              { return &m_sender;               }
  UINT            GetDesktop() const       { return m_desktop;               }
  bool            GetErrorState() const    { return m_errorstate;            }
  XString         GetLastError() const     { return m_lastError;             }
  bool            GetWhitespace() const    { return m_whitespace;            }
  Encoding        GetEncoding() const      { return m_encoding;              }
  XString         GetAcceptEncoding() const{ return m_acceptEncoding;        }
  bool            GetSendBOM() const       { return m_sendBOM;               }
  bool            GetSendUnicode() const   { return m_sendUnicode;           }
  bool            GetVerbTunneling() const { return m_verbTunnel;            }
  bool            GetIncoming() const      { return m_incoming;              }
  bool            GetHasBeenAnswered()const{ return m_request == NULL;       }
  Encoding        GetEncoding()            { return m_encoding;              }
  XString         GetReferrer() const      { return m_referrer;              }
  const Routing&  GetRouting() const       { return m_routing;               }
  XString         GetExtension() const     { return m_cracked.GetExtension();}
  XString         GetHeader(XString p_name);
  XString         GetRoute(int p_index);
  XString         GetContentType() const;
  XString         GetVerb();

  // SETTERS
  void            SetURL(const XString& p_url);
  void            SetIncoming(bool p_incoming)            { m_incoming           = p_incoming; }
  void            SetErrorstate(bool p_state)             { m_errorstate         = p_state;    }
  void            SetLastError(XString p_error)           { m_lastError          = p_error;    }
  void            SetUser(XString p_user)                 { m_user               = p_user;     }
  void            SetPassword(XString p_password)         { m_password           = p_password; }
  void            SetSecure(bool p_secure)                { m_cracked.m_secure   = p_secure;   ReparseURL(); }
  void            SetServer(XString p_server)             { m_cracked.m_host     = p_server;   ReparseURL(); }
  void            SetPort(int p_port)                     { m_cracked.m_port     = p_port;     ReparseURL(); }
  void            SetAbsolutePath(XString p_path)         { m_cracked.SetPath(p_path);         ReparseURL(); }
  void            SetExtension(XString p_ext)             { m_cracked.SetExtension(p_ext);     ReparseURL(); }
  void            SetStatus(unsigned p_status)            { m_status             = p_status;   }
  void            SetDesktop(UINT p_desktop)              { m_desktop            = p_desktop;  }
  void            SetRequestHandle(HTTP_OPAQUE_ID p_id)   { m_request            = p_id;       }
  void            SetVerb(XString p_verb)                 { m_verb               = p_verb;     }
  void            SetCookies(const Cookies& p_cookies)    { m_cookies            = p_cookies;  }
  void            SetContentType(XString p_type)          { m_contentType        = p_type;     }
  void            SetWhitespace(bool p_white)             { m_whitespace         = p_white;    }
  void            SetSendBOM(bool p_bom)                  { m_sendBOM            = p_bom;      }
  void            SetVerbTunneling(bool p_tunnel)         { m_verbTunnel         = p_tunnel;   }
  void            SetHasBeenAnswered()                    { m_request            = NULL;       }
  void            SetReferrer(XString p_referrer)         { m_referrer           = p_referrer; }
  void            SetAcceptEncoding(XString p_encoding);
  void            AddHeader(XString p_name,XString p_value);
  void            DelHeader(XString p_name);
  void            SetEncoding(Encoding p_encoding);
  void            SetSendUnicode(bool p_unicode);

  // Use POST method for PUT/MERGE/PATCH/DELETE
  bool            UseVerbTunneling();

  // JSONMessages can be stored elsewhere. Use the reference mechanism to add/drop references
  // With the drop of the last reference, the object WILL destroy itself
  void            AddReference();
  void            DropReference();

private:
  // TO BE CALLED FROM THE XTOR!!
  XString ConstructFromRawBuffer(uchar* p_buffer,unsigned p_length,XString p_charset);
  // Parse the URL, true if legal
  bool    ParseURL(XString p_url);
  // Re-parse URL after setting a part of the URL
  void    ReparseURL();

  // The message is contained in a JSON value
  JSONvalue*      m_value;

  // Parser for the XML texts
  friend          JSONParser;
  friend          JSONParserSOAP;
  // Message handling from here
  bool            m_whitespace  { false };                      // Whitespace preservation
  bool            m_incoming    { false };                      // Incoming JSON message
  bool            m_errorstate  { false };                      // Internal for parsing errors
  XString         m_lastError;                                  // Error as text
  Encoding        m_encoding    { Encoding::UTF8 };             // Encoding details
  bool            m_sendUnicode { false };                      // Send message in UTF-16 Unicode
  bool            m_sendBOM     { false };                      // Prepend message with UTF-8 or UTF-16 Byte-Order-Mark
  bool            m_verbTunnel  { false };                      // HTTP-VERB Tunneling used
  // DESTINATION
  XString         m_url;                                        // Full URL of the JSON service
  CrackedURL      m_cracked;                                    // Cracked down URL (all parts)
  XString         m_verb;                                       // HTTP verb, default = POST
  unsigned        m_status      { HTTP_STATUS_OK };             // HTTP status return code
  HTTP_OPAQUE_ID  m_request     { NULL };                       // Request it must answer
  HTTPSite*       m_site        { nullptr };                    // Site for which message is received
  HeaderMap       m_headers;                                    // Extra HTTP headers (incoming / outgoing)
  // Authentication
  XString         m_user;                                       // Found username for authentication
  XString         m_password;                                   // Found password for authentication
  // Message details
  XString         m_referrer;                                   // Referrer of the message
  XString         m_contentType;                                // Content type of JSON message
  XString         m_acceptEncoding;                             // Accepted HTTP compression encoding
  Cookies         m_cookies;                                    // Cookies
  HANDLE          m_token       { NULL };                       // Security access token
  SOCKADDR_IN6    m_sender;                                     // Senders address
  UINT            m_desktop     { 0 };                          // Senders remote desktop
  long            m_references  { 0 };                          // Externally referenced
  Routing         m_routing;                                    // Routing information from HTTP
  XString         m_extension;                                  // Extension of the resource (derived from URL)
};
