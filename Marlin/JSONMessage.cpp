/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: JSONMessage.cpp
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
#include "stdafx.h"
#include "JSONMessage.h"
#include "JSONParser.h"
#include "XMLParser.h"
#include "HTTPMessage.h"
#include "HTTPSite.h"
#include "DefuseBOM.h"
#include "ConvertWideString.h"
#include <iterator>
#include <algorithm>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

JSONvalue::JSONvalue()
{
}

JSONvalue::JSONvalue(JSONvalue* p_other)
{
  *this = *p_other;
}

JSONvalue::~JSONvalue()
{
  m_array .clear();
  m_object.clear();
}

JSONvalue&
JSONvalue::operator=(JSONvalue& p_other)
{
  m_type     = p_other.m_type;
  m_string   = p_other.m_string;
  m_constant = p_other.m_constant;
  m_number   = p_other.m_number;
  // objects
  m_array.clear();
  m_object.clear();
  std::copy(p_other.m_object.begin(),p_other.m_object.end(),back_inserter(m_object));
  std::copy(p_other.m_array .begin(),p_other.m_array .end(),back_inserter(m_array));

  return *this;
}

// Only set the type, clearing the rest
void
JSONvalue::SetDatatype(JsonType p_type)
{
  // Clear the values
  m_object.clear();
  m_array .clear();
  m_string.Empty();
  m_number.m_intNumber = 0;
  m_constant = JsonConst::JSON_NONE;
  // Remember our type
  m_type = p_type;
}

void
JSONvalue::SetValue(CString p_value)
{
  m_string = p_value;
  m_type   = JsonType::JDT_string;
  // Clear the rest
  m_array .clear();
  m_object.clear();
  m_number.m_intNumber = 0;
  m_constant = JsonConst::JSON_NONE;
}

void
JSONvalue::SetValue(JsonConst p_value)
{
  m_type = JsonType::JDT_const;
  m_constant = p_value;
  // Clear the rest
  m_array.clear();
  m_object.clear();
  m_number.m_intNumber = 0;
  m_string.Empty();
}

void        
JSONvalue::SetValue(JSONobject p_value)
{
  m_type = JsonType::JDT_object;
  m_object.clear();
  std::copy(p_value.begin(),p_value.end(),back_inserter(m_object));
  // Clear the rest
  m_array.clear();
  m_string.Empty();
  m_number.m_dblNumber = 0;
  m_constant = JsonConst::JSON_NONE;
}

void
JSONvalue::SetValue(JSONarray p_value)
{
  m_type = JsonType::JDT_array;
  m_array.clear();
  std::copy(p_value.begin(),p_value.end(),back_inserter(m_array));
  // Clear the rest
  m_object.clear();
  m_string.Empty();
  m_number.m_intNumber = 0;
  m_constant = JsonConst::JSON_NONE;
}

void
JSONvalue::SetValue(int p_value)
{
  m_type = JsonType::JDT_number_int;
  m_number.m_intNumber = p_value;
  // Clear the rest
  m_object.clear();
  m_array.clear();
  m_string.Empty();
  m_constant = JsonConst::JSON_NONE;
}

void
JSONvalue::SetValue(double p_value)
{
  m_type = JsonType::JDT_number_dbl;
  m_number.m_dblNumber = p_value;
  // Clear the rest
  m_object.clear();
  m_array.clear();
  m_string.Empty();
  m_constant = JsonConst::JSON_NONE;
}

CString
JSONvalue::GetAsJsonString(bool p_white,bool p_utf8,unsigned p_level /*=0*/)
{
  CString result;
  CString separ;
  CString newln = p_white ? "\n" : "";

  if(p_white)
  {
    for(unsigned ind = 0;ind < p_level; ++ind)
    {
      separ += "  ";
    }
  }

  switch(m_type)
  {
    case JsonType::JDT_const:       switch(m_constant)
                                    {
                                      case JsonConst::JSON_NONE:  return "";
                                      case JsonConst::JSON_NULL:  return "null";
                                      case JsonConst::JSON_FALSE: return "false";
                                      case JsonConst::JSON_TRUE:  return "true";
                                    }
                                    break;
    case JsonType::JDT_string:      return FormatAsJsonString(m_string,p_utf8);
    case JsonType::JDT_number_int:  result.Format("%d",m_number.m_intNumber);
                                    break;
    case JsonType::JDT_number_dbl:  result.Format("%.15g",m_number.m_dblNumber);
                                    break;
    case JsonType::JDT_array:       result = newln + "[" + separ;
                                    for(unsigned ind = 0;ind < m_array.size();++ind)
                                    {
                                      result += m_array[ind].GetAsJsonString(p_white,p_utf8,p_level+1);
                                      if(ind < m_array.size() - 1)
                                      {
                                        result += newln + separ + ",";
                                      }
                                    }
                                    result += newln + "]";
                                    break;
    case JsonType::JDT_object:      result = "{" + newln + separ + (p_white ? " " : "");
                                    for(unsigned ind = 0; ind < m_object.size(); ++ind)
                                    {
                                      // Check for empty object
                                      if(m_object.size() == 1 && m_object[0].m_name.IsEmpty() &&
                                         m_object[0].m_value.GetDataType() == JsonType::JDT_const &&
                                         m_object[0].m_value.GetConstant() == JsonConst::JSON_NONE)
                                      {
                                        break;
                                      }
                                      result += FormatAsJsonString(m_object[ind].m_name,p_utf8);
                                      result += ":";
                                      result += m_object[ind].m_value.GetAsJsonString(p_white,p_utf8,p_level+1);
                                      if(ind < m_object.size() - 1)
                                      {
                                        result += newln + separ + ",";
                                      }
                                    }
                                    if(p_white && result.Right(1) != "\n")
                                    {
                                      result += newln;
                                    }
                                    result += separ + "}" + newln;
                                    break;
  }
  return result;
}

CString
JSONvalue::FormatAsJsonString(CString p_string,bool p_utf8 /*=false*/)
{
  CString result("\"");
  char ch = 0;
  unsigned char buffer[3];
  buffer[2] = 0;

  for(int ind = 0; ind < p_string.GetLength(); ++ind)
  {
    ch = p_string.GetAt(ind);

    if(ch < 0x80)
    {
      switch(ch = p_string.GetAt(ind))
      {
        case '\"': result += "\\\"";   break;
        case '\\': result += "\\\\";   break;
        case '/':  result += "\\/";    break;
        case '\b': result += "\\b";    break;
        case '\f': result += "\\f";    break;
        case '\n': result += "\\n";    break;
        case '\r': result += "\\r";    break;
        case '\t': result += "\\t";    break;
        default:   result += ch;       break;
      }
    }
    else
    {
      // Plainly add the character
      // Windows-1252 encoding or UTF-8 encoding
      result += ch;
    }
  }
  // Closing
  result += "\"";

  if(p_utf8)
  {
    // Convert to UTF-8
    result = EncodeUTF8String(result);
  }

  return result;
}

//////////////////////////////////////////////////////////////////////////
//
// JSONMessage object
//
//////////////////////////////////////////////////////////////////////////

JSONMessage::JSONMessage()
{
  // Set sender to null
  memset(&m_sender,0,sizeof(SOCKADDR_IN6));
}

// XTOR: From a string. 
// Incoming string , no whitespace preservation, expect it to be UTF-8
JSONMessage::JSONMessage(CString p_message)
{
  // Overrides from the declaration
  m_incoming = true; // INCOMING!!

  // Set sender to null
  memset(&m_sender,0,sizeof(SOCKADDR_IN6));

  // Use the parameter
  ParseMessage(p_message);
}

// XTOR: From an internal string with explicit space and encoding
JSONMessage::JSONMessage(CString p_message,bool p_whitespace,JsonEncoding p_encoding)
{
  // Preserve whitespace setting
  m_whitespace = p_whitespace;

  // Set sender to null
  memset(&m_sender,0,sizeof(SOCKADDR_IN6));

  // Use the parameter
  ParseMessage(p_message,p_encoding);
}

// XTOR: Outgoing message + url
JSONMessage::JSONMessage(CString p_message,CString p_url)
{
  // Set sender to null
  memset(&m_sender,0,sizeof(SOCKADDR_IN6));

  // Set sender to null
  memset(&m_sender,0,sizeof(SOCKADDR_IN6));

  // To this URL
  SetURL(p_url);

  // Use the parameter
  ParseMessage(p_message,JsonEncoding::JENC_Plain);
}

// XTOR: From another message
JSONMessage::JSONMessage(JSONMessage* p_other)
{
  // Copy the primary message value
  m_value      = p_other->m_value;
  m_whitespace = p_other->m_whitespace;

  // Copy all other data members
  m_incoming    = p_other->m_incoming;
  m_errorstate  = p_other->m_errorstate;
  m_lastError   = p_other->m_lastError;
  m_url         = p_other->m_url;
  m_cracked     = p_other->m_cracked;
  m_request     = p_other->m_request;
  m_site        = p_other->m_site;
  m_desktop     = p_other->m_desktop;
  m_encoding    = p_other->m_encoding;
  m_sendUnicode = p_other->m_sendUnicode;
  m_sendBOM     = p_other->m_sendBOM;
  m_verbTunnel  = p_other->m_verbTunnel;
  m_acceptEncoding = p_other->m_acceptEncoding;
  // Duplicate all cookies
  m_cookies = p_other->GetCookies();

  // Copy all headers from the HTTPmessage
  HeaderMap* map = p_other->GetHeaderMap();
  for(HeaderMap::iterator it = map->begin(); it != map->end(); ++it)
  {
    m_headers[it->first] = it->second;
  }

  // Get sender (if any) from the HTTP message
  memcpy(&m_sender,p_other->GetSender(),sizeof(SOCKADDR_IN6));

  // Duplicate the HTTP token for ourselves
  if(DuplicateTokenEx(p_other->m_token
                     ,TOKEN_DUPLICATE|TOKEN_IMPERSONATE|TOKEN_ALL_ACCESS|TOKEN_READ|TOKEN_WRITE
                     ,NULL
                     ,SecurityImpersonation
                     ,TokenImpersonation
                     ,&m_token) == FALSE)
  {
    m_token = NULL;
  }
}

JSONMessage::JSONMessage(HTTPMessage* p_message)
{
  // Copy all parts
  m_url            = p_message->GetURL();
  m_request        = p_message->GetRequestHandle();
  m_site           = p_message->GetHTTPSite();
  m_cracked        = p_message->GetCrackedURL();
  m_desktop        = p_message->GetRemoteDesktop();
  m_contentType    = p_message->GetContentType();
  m_acceptEncoding = p_message->GetAcceptEncoding();
  m_verbTunnel     = p_message->GetVerbTunneling();
  m_incoming       = (p_message->GetCommand() != HTTPCommand::http_response);

  // Duplicate all cookies
  m_cookies = p_message->GetCookies();

  // Copy all headers from the HTTPmessage
  HeaderMap* map = p_message->GetHeaderMap();
  for(HeaderMap::iterator it = map->begin(); it != map->end(); ++it)
  {
    m_headers[it->first] = it->second;
  }

  // Get sender (if any) from the HTTP message
  memcpy(&m_sender,p_message->GetSender(),sizeof(SOCKADDR_IN6));
  m_desktop = p_message->GetRemoteDesktop();

  // Duplicate the HTTP token for ourselves
  if(DuplicateTokenEx(p_message->GetAccessToken()
                     ,TOKEN_DUPLICATE|TOKEN_IMPERSONATE|TOKEN_ALL_ACCESS|TOKEN_READ|TOKEN_WRITE
                     ,NULL
                     ,SecurityImpersonation
                     ,TokenImpersonation
                     ,&m_token) == FALSE)
  {
    m_token = NULL;
  }

  // The message itself
  CString message;
  JsonEncoding encoding = JsonEncoding::JENC_UTF8;
  CString charset = FindCharsetInContentType(m_contentType);
  if(charset.Left(6).CompareNoCase("utf-16") == 0)
  {
    // Works for "UTF-16", "UTF-16LE" and "UTF-16BE" as of RFC 2781
    uchar* buffer = nullptr;
    size_t length = 0;
    p_message->GetRawBody(&buffer,length);

    int uni = IS_TEXT_UNICODE_UNICODE_MASK;  // Intel/AMD processors + BOM
    if(IsTextUnicode(buffer,(int)length,&uni))
    {
      if(TryConvertWideString(buffer,(int)length,"",message,m_sendBOM))
      {
        // Current encoding is now plain current codepage
        encoding = JsonEncoding::JENC_Plain;
        // Will answer as 16 bits Unicode
        m_sendUnicode = true;
      }
      else
      {
        // We are now officially in error state
        m_errorstate = true;
        m_lastError  = "Cannot convert UTF-16 buffer";
      }
    }
    else
    {
      // Probably already processed in HandleTextContext of the server
      message = p_message->GetBody();
    }
    delete [] buffer;
  }
  else
  {
    message = p_message->GetBody();

    // Other special cases of the charset
    if(charset.Left(12).CompareNoCase("windows-1252") == 0)
    {
      encoding = JsonEncoding::JENC_Plain;
    }
    else if(charset.Left(10).CompareNoCase("iso-8859-1") == 0)
    {
      encoding = JsonEncoding::JENC_ISO88591;
    }
  }
  ParseMessage(message,encoding);
}

JSONMessage::JSONMessage(SOAPMessage* p_message)
{
  // Copy all parts
  m_url             = p_message->GetURL();
  ParseURL(m_url);
  m_request         = p_message->GetRequestHandle();
  m_site            = p_message->GetHTTPSite();
  m_desktop         = p_message->GetRemoteDesktop();
  m_contentType     = p_message->GetContentType();
  m_verbTunnel      = false; //p_message->GetVerbTunneling();
  m_incoming        = p_message->GetIncoming();
  m_whitespace     = !p_message->GetCondensed();
  m_acceptEncoding  = p_message->GetAcceptEncoding();

  // Duplicate all cookies
  m_cookies = p_message->GetCookies();

  // Copy all headers from the HTTPmessage
  HeaderMap* map = p_message->GetHeaderMap();
  for(HeaderMap::iterator it = map->begin(); it != map->end(); ++it)
  {
    m_headers[it->first] = it->second;
  }

  // Get sender (if any) from the HTTP message
  memcpy(&m_sender,p_message->GetSender(),sizeof(SOCKADDR_IN6));
  m_desktop = p_message->GetRemoteDesktop();

  // Duplicate the HTTP token for ourselves
  if(DuplicateTokenEx(p_message->GetAccessToken()
                     ,TOKEN_DUPLICATE|TOKEN_IMPERSONATE|TOKEN_ALL_ACCESS|TOKEN_READ|TOKEN_WRITE
                     ,NULL
                     ,SecurityImpersonation
                     ,TokenImpersonation
                     ,&m_token) == FALSE)
  {
    m_token = NULL;
  }

  // The message itself
  JSONParserSOAP(this,p_message);
}

JSONMessage::~JSONMessage()
{
  // Free the authentication token
  if(m_token)
  {
    CloseHandle(m_token);
    m_token = NULL;
  }
}

void
JSONMessage::Reset()
{
  // Clear the message
  m_value.SetValue(JsonConst::JSON_NULL);
  m_incoming = false;
  // Reset error
  m_errorstate = false;
  m_lastError.Empty();
  // leave the rest for the destination
}

// Parse the URL, true if legal
bool
JSONMessage::ParseURL(CString p_url)
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
JSONMessage::ReparseURL()
{
  m_url = m_cracked.URL();
}

CString
JSONMessage::GetContentType()
{
  if(m_contentType.IsEmpty())
  {
    return "application/json";
  }
  return m_contentType;
}

// JSON messages mostly get's send as a POST
// But you can override it for e.g. OData 
CString
JSONMessage::GetVerb()
{
  if(m_verb.IsEmpty())
  {
    return "POST";
  }
  return m_verb;
}

void
JSONMessage::SetAcceptEncoding(CString p_encoding)
{
  m_acceptEncoding = p_encoding;
  m_acceptEncoding.MakeLower();
}

void 
JSONMessage::AddHeader(CString p_name,CString p_value)
{
  p_name.MakeLower();
  m_headers.insert(std::make_pair(p_name,p_value));
}

// Finding a header by lowercase name
CString
JSONMessage::GetHeader(CString p_name)
{
  p_name.MakeLower();

  HeaderMap::iterator it = m_headers.find(p_name);
  if(it != m_headers.end())
  {
    return it->second;
  }
  return "";
}

// Addressing the message's has three levels
// 1) The complete url containing both server and port number
// 2) Setting server/port/absolutepath separately
// 3) By remembering the requestID of the caller
void
JSONMessage::SetURL(CString& p_url)
{
  m_url = p_url;

  CrackedURL url;
  if(url.CrackURL(p_url))
  {
    m_cracked = url;
  }
}

void
JSONMessage::SetEncoding(JsonEncoding p_encoding)
{
  m_encoding    = p_encoding;
  m_sendUnicode = (p_encoding == JsonEncoding::JENC_UTF16);
}

void
JSONMessage::SetSendUnicode(bool p_unicode)
{
  m_sendUnicode = p_unicode;
  if(p_unicode)
  {
    m_encoding = JsonEncoding::JENC_UTF16;
  }
  else if(m_encoding == JsonEncoding::JENC_UTF16)
  {
    // Reset to UTF8, if not sending in Unicode16
    m_encoding = JsonEncoding::JENC_UTF8;
  }
}

// Go from JSON string to this message
// The p_encoding gives the encoding the incoming string is in!
bool
JSONMessage::ParseMessage(CString p_message,JsonEncoding p_encoding /*=JsonEncoding::JENC_UTF8*/)
{
  JSONParser parser(this);

  // Starting the parser, preserving it's whitespace state
  parser.ParseMessage(p_message,m_whitespace,p_encoding);

  return (m_errorstate == false);
}

// Reconstruct JSON string from this message
CString 
JSONMessage::GetJsonMessage(JsonEncoding p_encoding /*=JsonEncoding::JENC_UTF8*/)
{
  return m_value.GetAsJsonString(m_whitespace,(p_encoding == JsonEncoding::JENC_UTF8));
}

CString 
JSONMessage::GetJsonMessageWithBOM(JsonEncoding p_encoding /*=JsonEncoding::JENC_UTF8*/)
{
  if(m_sendBOM && p_encoding == JsonEncoding::JENC_UTF8)
  {
    CString msg = ConstructBOM();
    msg += GetJsonMessage(p_encoding);
    return msg;
  }
  return GetJsonMessage(p_encoding);
}

// Use POST method for PUT/MERGE/PATCH/DELETE
// Also known as VERB-Tunneling
bool
JSONMessage::UseVerbTunneling()
{
  if(m_verbTunnel)
  {
    if(m_verb == "PUT"    || 
       m_verb == "MERGE"  || 
       m_verb == "DELETE" || 
       m_verb == "PATCH"   ) 
    {
      // Change of identity!
      AddHeader("X-HTTP-Method-Override",m_verb);
      m_verb = "POST";
      return true;
    }
  }
  return false;
}
