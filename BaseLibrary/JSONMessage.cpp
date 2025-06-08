/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: JSONMessage.cpp
//
// BaseLibrary: Indispensable general objects and functions
// 
// Copyright (c) 2014-2025 ir. W.E. Huisman
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
#include "pch.h"
#include "JSONMessage.h"
#include "JSONParser.h"
#include "XMLParser.h"
#include "HTTPMessage.h"
#include "ConvertWideString.h"
#include <iterator>
#include <algorithm>

#ifdef _AFX
#ifdef _DEBUG 
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#endif

JSONvalue::JSONvalue()
{
}

JSONvalue::JSONvalue(const JSONvalue* p_other)
{
  *this = *p_other;
}

JSONvalue::JSONvalue(const JsonType p_type)
{
  SetDatatype(p_type);
}

JSONvalue::JSONvalue(const JsonConst  p_value)
{
  SetValue(p_value);
}

JSONvalue::JSONvalue(const XString p_value)
{
  SetValue(p_value);
}

JSONvalue::JSONvalue(const int p_value)
{
  SetValue(p_value);
}

JSONvalue::JSONvalue(const bcd& p_value)
{
  SetValue(p_value);
}

JSONvalue::JSONvalue(const bool p_value)
{
  SetValue(p_value ? JsonConst::JSON_TRUE : JsonConst::JSON_FALSE);
}

JSONvalue::~JSONvalue()
{
  m_array .clear();
  m_object.clear();
}

JSONvalue&
JSONvalue::operator=(const JSONvalue& p_other)
{
  // Check if we do not assign ourselves
  if(&p_other == this)
  {
    return *this;
  }
  // Copy values
  m_type      = p_other.m_type;
  m_string    = p_other.m_string;
  m_constant  = p_other.m_constant;
  m_intNumber = p_other.m_intNumber;
  m_bcdNumber = p_other.m_bcdNumber;
  m_mark      = p_other.m_mark;
  // Copy objects
  m_array.clear();
  m_object.clear();
  std::copy(p_other.m_object.begin(),p_other.m_object.end(),back_inserter(m_object));
  std::copy(p_other.m_array .begin(),p_other.m_array .end(),back_inserter(m_array));

  return *this;
}

JSONvalue& 
JSONvalue::operator=(const XString& p_other)
{
  m_type      = JsonType::JDT_string;
  m_constant  = JsonConst::JSON_NONE;
  m_string    = p_other;
  m_intNumber = 0;
  m_bcdNumber.Zero();
  m_array.clear();
  m_object.clear();

  return *this;
}

JSONvalue& 
JSONvalue::operator=(LPCTSTR p_other)
{
  m_type      = JsonType::JDT_string;
  m_constant  = JsonConst::JSON_NONE;
  m_string    = p_other;
  m_intNumber = 0;
  m_bcdNumber.Zero();
  m_array.clear();
  m_object.clear();

  return *this;
}

JSONvalue& 
JSONvalue::operator=(const int& p_other)
{
  m_type      = JsonType::JDT_number_int;
  m_constant  = JsonConst::JSON_NONE;
  m_intNumber = p_other;
  m_bcdNumber.Zero();
  m_string.Empty();
  m_array.clear();
  m_object.clear();

  return *this;
}

JSONvalue& 
JSONvalue::operator=(const bcd& p_other)
{
  m_type      = JsonType::JDT_number_bcd;
  m_constant  = JsonConst::JSON_NONE;
  m_intNumber = 0;
  m_bcdNumber = p_other;
  m_string.Empty();
  m_array.clear();
  m_object.clear();

  return *this;
}

JSONvalue& 
JSONvalue::operator=(JsonConst& p_other)
{
  m_type      = JsonType::JDT_const;
  m_constant  = p_other;
  m_intNumber = 0;
  m_bcdNumber.Zero();
  m_string.Empty();
  m_array.clear();
  m_object.clear();

  return *this;
}

JSONvalue& 
JSONvalue::operator=(const bool& p_other)
{
  m_type      = JsonType::JDT_const;
  m_constant  = p_other ? JsonConst::JSON_TRUE : JsonConst::JSON_FALSE;
  m_intNumber = 0;
  m_bcdNumber.Zero();
  m_string.Empty();
  m_array.clear();
  m_object.clear();

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
  m_intNumber = 0;
  m_bcdNumber.Zero();
  m_constant = JsonConst::JSON_NONE;
  // Remember our type
  m_type = p_type;
}

void
JSONvalue::SetValue(XString p_value)
{
  m_string = p_value;
  m_type   = JsonType::JDT_string;
  // Clear the rest
  m_array .clear();
  m_object.clear();
  m_intNumber = 0;
  m_bcdNumber.Zero();
  m_constant = JsonConst::JSON_NONE;
}

void
JSONvalue::SetValue(LPCTSTR p_value)
{
  m_string = p_value;
  m_type   = JsonType::JDT_string;
  // Clear the rest
  m_array .clear();
  m_object.clear();
  m_intNumber = 0;
  m_bcdNumber.Zero();
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
  m_intNumber = 0;
  m_bcdNumber.Zero();
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
  m_intNumber = 0;
  m_bcdNumber.Zero();
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
  // m_string.Empty();
  m_intNumber = 0;
  m_bcdNumber.Zero();
  m_constant = JsonConst::JSON_NONE;
}

void
JSONvalue::SetValue(int p_value)
{
  m_type = JsonType::JDT_number_int;
  m_intNumber = p_value;
  m_bcdNumber.Zero();
  // Clear the rest
  m_object.clear();
  m_array.clear();
  m_string.Empty();
  m_constant = JsonConst::JSON_NONE;
}

void
JSONvalue::SetValue(const bcd& p_value)
{
  m_type = JsonType::JDT_number_bcd;
  m_bcdNumber = p_value;
  m_intNumber = 0;
  // Clear the rest
  m_object.clear();
  m_array.clear();
  m_string.Empty();
  m_constant = JsonConst::JSON_NONE;
}

void
JSONvalue::SetMark(bool p_mark)
{
  m_mark = p_mark;
}

void
JSONvalue::Empty()
{
  SetValue(JsonConst::JSON_NONE);
}

bool
JSONvalue::IsEmpty()
{
  if(m_type == JsonType::JDT_const)
  {
    if(m_constant == JsonConst::JSON_NULL || 
       m_constant == JsonConst::JSON_NONE )
    {
      return true;
    }
  }
  return false;
}

void
JSONvalue::Add(JSONvalue& p_value)
{
  if(m_type == JsonType::JDT_array)
  {
    m_array.push_back(p_value);
    return;
  }
  throw StdException(_T("JSONvalue can only be added to a JSON array!"));
}

void
JSONvalue::Add(JSONpair& p_value)
{
  if(m_type == JsonType::JDT_object)
  {
    m_object.push_back(p_value);
    return;
  }
  throw StdException(_T("JSONpair can only be added to a JSON object!"));
}

XString
JSONvalue::GetAsJsonString(bool p_white,unsigned p_level /*=0*/,bool p_exponential /*= false*/)
{
  XString result;
  XString separ,less;
  XString newln = p_white ? _T("\n") : _T("");

  if(p_white)
  {
    for(unsigned ind = 0;ind < p_level; ++ind)
    {
      less   = separ;
      separ += _T("\t");
    }
  }

  switch(m_type)
  {
    case JsonType::JDT_const:       switch(m_constant)
                                    {
                                      case JsonConst::JSON_NONE:  return _T("");
                                      case JsonConst::JSON_NULL:  return _T("null");
                                      case JsonConst::JSON_FALSE: return _T("false");
                                      case JsonConst::JSON_TRUE:  return _T("true");
                                    }
                                    break;
    case JsonType::JDT_string:      return XMLParser::PrintJsonString(m_string);
    case JsonType::JDT_number_int:  result.Format(_T("%d"),m_intNumber);
                                    break;
    case JsonType::JDT_number_bcd:  result = m_bcdNumber.AsString(p_exponential ? bcd::Format::Engineering : bcd::Format::Bookkeeping,false,0);
                                    break;
    case JsonType::JDT_array:       result = _T("[") + newln;
                                    for(unsigned ind = 0;ind < m_array.size();++ind)
                                    {
                                      result += separ;
                                      result += m_array[ind].GetAsJsonString(p_white,p_level+1,p_exponential);
                                      if(ind < m_array.size() - 1)
                                      {
                                        result += _T(",");
                                      }
                                      result += newln;
                                    }
                                    result += separ;
                                    result += _T("]");
                                    break;
    case JsonType::JDT_object:      result = less + _T("{") + newln;
                                    for(unsigned ind = 0; ind < m_object.size(); ++ind)
                                    {
                                      // Check for empty object
                                      if(m_object.size() == 1 && m_object[0].m_name.IsEmpty() &&
                                         m_object[0].m_value.GetDataType() == JsonType::JDT_const &&
                                         m_object[0].m_value.GetConstant() == JsonConst::JSON_NONE)
                                      {
                                        break;
                                      }
                                      result += separ;
                                      result += p_white ? _T("\t") : _T("");
                                      result += XMLParser::PrintJsonString(m_object[ind].m_name);
                                      result += _T(":");
                                      result += m_object[ind].m_value.GetAsJsonString(p_white,p_level+1,p_exponential).TrimLeft('\t');
                                      if(ind < m_object.size() - 1)
                                      {
                                        result += _T(",");
                                      }
                                      result += newln;
                                    }
                                    result += separ;
                                    result += _T("}");
                                    break;
  }
  return result;
}

// Getting the value from an JSONarray
JSONvalue& 
JSONvalue::operator[](int p_index)
{
  if(m_type != JsonType::JDT_array)
  {
    throw StdException(_T("JSON array index used on a non-array node!"));
  }
  if(p_index >= 0 && p_index < (int)m_array.size())
  {
    return m_array[p_index];
  }
  throw StdException(_T("JSON array index out of bounds!"));
}

// Getting the value from an JSONobject
JSONvalue&
JSONvalue::operator[](XString p_name)
{
  if(m_type != JsonType::JDT_object)
  {
    throw StdException(_T("JSON object index used on an non-object node"));
  }
  for(auto& pair : m_object)
  {
    if(pair.m_name.Compare(p_name) == 0)
    {
      return pair.m_value;
    }
  }
  throw StdException(_T("JSON object index not found!"));
}

void
JSONvalue::JsonReplace(XString p_namePattern,XString p_tofind,XString p_replace,int& p_number,bool p_caseSensitive)
{
  switch(m_type)
  {
    case JsonType::JDT_object: JsonReplaceObject(p_namePattern,p_tofind,p_replace,p_number,p_caseSensitive); 
                               break;
    case JsonType::JDT_array:  JsonReplaceArray (p_namePattern,p_tofind,p_replace,p_number,p_caseSensitive);
                               break;
    default:                   // NOTHING TO DO!
                               break;
  }
}

void
JSONvalue::JsonReplaceObject(XString p_namePattern,XString p_tofind,XString p_replace,int& p_number,bool p_caseSensitive /*=true*/)
{
  for(auto& pair : m_object)
  {
    switch(pair.GetDataType())
    {
      case JsonType::JDT_string:  if(pair.m_name.Find(p_namePattern) >= 0)
                                  {
                                    XString value = pair.m_value.GetString();
                                    if(!p_caseSensitive)
                                    {
                                      value.MakeLower();
                                    }
                                    int pos = value.Find(p_tofind);
                                    if(pos >= 0)
                                    {
                                      XString replacement = value.Left(pos);
                                      replacement += p_replace;
                                      replacement += value.Mid(pos + p_tofind.GetLength());
                                      pair.m_value.SetValue(replacement);
                                      ++p_number;
                                    }
                                  }
                                  break;
      case JsonType::JDT_object:  [[fallthrough]];
      case JsonType::JDT_array:   pair.m_value.JsonReplace(p_namePattern,p_tofind,p_replace,p_number,p_caseSensitive);
                                  break;
      default:                    // DO NOTHING
                                  break;
    }
  }
}

void
JSONvalue::JsonReplaceArray(XString p_namePattern,XString p_tofind,XString p_replace,int& p_number,bool p_caseSensitive /*=true*/)
{
  for(auto& value : m_array)
  {
    if(value.GetDataType() == JsonType::JDT_object ||
       value.GetDataType() == JsonType::JDT_array  )
    {
      value.JsonReplace(p_namePattern,p_tofind,p_replace,p_number,p_caseSensitive);
    }
  }
}

// JSONvalues can be stored elsewhere. Use the reference mechanism to add/drop references
// With the drop of the last reference, the object WILL destroy itself

void
JSONvalue::AddReference()
{
  InterlockedIncrement(&m_references);
}

void
JSONvalue::DropReference()
{
  if(InterlockedDecrement(&m_references) <= 0)
  {
    delete this;
  }
}

//////////////////////////////////////////////////////////////////////////
//
// JSONpair
//
//////////////////////////////////////////////////////////////////////////

JSONpair::JSONpair(XString p_name) 
         :m_name(p_name)
{
}

JSONpair::JSONpair(XString p_name,JSONvalue& p_value)
         :m_name(p_name)
         ,m_value(p_value)
{
}

JSONpair::JSONpair(XString p_name, JsonType p_type)
         :m_name(p_name)
         ,m_value(p_type)
{
}

JSONpair::JSONpair(XString p_name,XString p_value)
         :m_name(p_name)
         ,m_value(p_value)
{
}

JSONpair::JSONpair(XString p_name,LPCTSTR p_value)
         :m_name(p_name)
         ,m_value(p_value)
{
}

JSONpair::JSONpair(XString p_name,int p_value)
         :m_name(p_name)
         ,m_value(p_value)
{
}

JSONpair::JSONpair(XString p_name,const bcd& p_value)
         :m_name(p_name)
         ,m_value(p_value)
{
}

JSONpair::JSONpair(XString p_name,bool p_value)
         :m_name(p_name)
         ,m_value(p_value)
{
}

JSONpair::JSONpair(XString p_name,JsonConst p_value)
         :m_name(p_name)
         ,m_value(p_value)
{
}


JSONpair& 
JSONpair::operator=(const JSONpair& p_other)
{
  m_name  = p_other.m_name;
  m_value = p_other.m_value;
  return *this;
}

//////////////////////////////////////////////////////////////////////////
//
// JSONMessage object
//
//////////////////////////////////////////////////////////////////////////

JSONMessage::JSONMessage()
{
  AddReference();

  // Set empty value
  m_value = new JSONvalue();

  // Set sender to null
  memset(&m_sender,0,sizeof(SOCKADDR_IN6));
}

// XTOR: From a string. 
// Incoming string , no whitespace preservation, expect it to be UTF-8
JSONMessage::JSONMessage(XString p_message)
{
  AddReference();

  // Set empty value
  m_value = new JSONvalue();

  // Overrides from the declaration
  m_incoming = true; // INCOMING!!

  // Set sender to null
  memset(&m_sender,0,sizeof(SOCKADDR_IN6));

  // Use the parameter
  ParseMessage(p_message);
}

// XTOR: From an internal string with explicit space and encoding
JSONMessage::JSONMessage(XString p_message,bool p_whitespace,Encoding p_encoding /*=Encoding::UTF8*/)
            :m_encoding(p_encoding)
{
  AddReference();

  // Set empty value
  m_value = new JSONvalue();

  // Preserve whitespace setting
  m_whitespace = p_whitespace;

  // Set sender to null
  memset(&m_sender,0,sizeof(SOCKADDR_IN6));

  // Use the parameter
  ParseMessage(p_message);
}

// XTOR: Outgoing message + url
JSONMessage::JSONMessage(XString p_message,XString p_url)
{
  AddReference();

  // Set empty value
  m_value = new JSONvalue();

  // Set sender to null
  memset(&m_sender,0,sizeof(SOCKADDR_IN6));

  // To this URL
  SetURL(p_url);

  // Use the parameter
  ParseMessage(p_message);
}

// XTOR: From another message
JSONMessage::JSONMessage(JSONMessage* p_other)
{
  AddReference();

  // Copy the primary message value, and reference it
  m_value = new JSONvalue(p_other->m_value);
  m_value->AddReference();

  // Copy all other data members
  m_whitespace = p_other->m_whitespace;
  m_incoming    = p_other->m_incoming;
  m_errorstate  = p_other->m_errorstate;
  m_lastError   = p_other->m_lastError;
  m_url         = p_other->m_url;
  m_cracked     = p_other->m_cracked;
  m_user        = p_other->m_user;
  m_password    = p_other->m_password;
  m_status      = p_other->m_status;
  m_request     = p_other->m_request;
  m_site        = p_other->m_site;
  m_desktop     = p_other->m_desktop;
  m_encoding    = p_other->m_encoding;
  m_sendBOM     = p_other->m_sendBOM;
  m_verbTunnel  = p_other->m_verbTunnel;
  m_headers     = p_other->m_headers;
  m_verb        = p_other->m_verb;
  m_acceptEncoding = p_other->m_acceptEncoding;
  // Duplicate all cookies
  m_cookies = p_other->GetCookies();
  // Duplicate all routing
  m_routing = p_other->GetRouting();

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
  AddReference();

  // Set empty value
  m_value          = new JSONvalue();

  // Copy all parts
  m_url            = p_message->GetURL();
  m_status         = p_message->GetStatus();
  m_request        = p_message->GetRequestHandle();
  m_site           = p_message->GetHTTPSite();
  m_cracked        = p_message->GetCrackedURL();
  m_desktop        = p_message->GetRemoteDesktop();
  m_contentType    = p_message->GetContentType();
  m_acceptEncoding = p_message->GetAcceptEncoding();
  m_verbTunnel     = p_message->GetVerbTunneling();
  m_referrer       = p_message->GetReferrer();
  m_user           = p_message->GetUser();
  m_password       = p_message->GetPassword();
  m_headers        =*p_message->GetHeaderMap();
  m_verb           = p_message->GetVerb();
  m_incoming       = (p_message->GetCommand() != HTTPCommand::http_response);

  // Duplicate all cookies
  m_cookies = p_message->GetCookies();

  // Copy routing
  m_routing = p_message->GetRouting();

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

  // THE MESSAGE ITSELF

  // Parsing the charset
  XString charset = FindCharsetInContentType(m_contentType);
  if(charset.IsEmpty())
  {
    charset = _T("utf-8");
  }
  m_encoding = (Encoding)CharsetToCodepage(charset);

  // Parse buffer to string, depending on the charset
  uchar* buffer = nullptr;
  size_t length = 0;
  p_message->GetRawBody(&buffer,length);

  if(length > 0)
  {
    XString message = ConstructFromRawBuffer(buffer,(unsigned)length,charset);
    // Parse the JSON tree
    if(!m_errorstate)
    {
      ParseMessage(message);
    }
  }
  delete [] buffer;
}

JSONMessage::JSONMessage(SOAPMessage* p_message)
{
  AddReference();

  // Set empty value
  m_value = new JSONvalue();

  // Copy all parts
  m_url             = p_message->GetURL();
  m_cracked         = p_message->GetCrackedURL();
  m_status          = p_message->GetStatus();
  m_request         = p_message->GetRequestHandle();
  m_site            = p_message->GetHTTPSite();
  m_desktop         = p_message->GetRemoteDesktop();
  m_contentType     = p_message->GetContentType();
  m_verbTunnel      = false;
  m_incoming        = p_message->GetIncoming();
  m_whitespace      =!p_message->GetCondensed();
  m_acceptEncoding  = p_message->GetAcceptEncoding();
  m_user            = p_message->GetUser();
  m_password        = p_message->GetPassword();
  m_headers         =*p_message->GetHeaderMap();

  // Duplicate all cookies
  m_cookies = p_message->GetCookies();

  // Duplicate routing
  m_routing = p_message->GetRouting();

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
  JSONParserSOAP parse(this,p_message);
}

// TO BE CALLED FROM THE XTOR!!
XString
JSONMessage::ConstructFromRawBuffer(uchar* p_buffer,unsigned p_length,XString p_charset)
{
  XString message;

#ifdef _UNICODE
  if(p_charset.CompareNoCase(_T("utf-16")) == 0)
  {
    // It's just our way of Unicode
    message = reinterpret_cast<LPCTSTR>(p_buffer);
    m_encoding = Encoding::LE_UTF16;
  }
  else
  {
    if(!TryConvertNarrowString(p_buffer,p_length,p_charset,message,m_sendBOM))
    {
      // We are now officially in error state
      m_errorstate = true;
      m_lastError  = _T("Cannot convert buffer to UTF-16");
      message.Empty();
    }
  }

#else

  if(p_charset.Left(6).CompareNoCase(_T("utf-16")) == 0)
  {
    int uni = IS_TEXT_UNICODE_UNICODE_MASK;  // Intel/AMD processors + BOM
    if(IsTextUnicode(p_buffer,(int) p_length,&uni))
    {
      if(TryConvertWideString(p_buffer,(int) p_length,_T(""),message,m_sendBOM) == false)
      {
        // We are now officially in error state
        m_errorstate = true;
        m_lastError  = _T("Cannot convert UTF-16 buffer");
      }
    }
    else
    {
      // Probably already processed in HandleTextContext of the server
      message = p_buffer;
    }
  }
  else
  {
    XString string(p_buffer);
    message = DecodeStringFromTheWire(string,p_charset);
  }
#endif
  return message;
}

JSONMessage::~JSONMessage()
{
  // Free the authentication token
  if(m_token)
  {
    CloseHandle(m_token);
    m_token = NULL;
  }

  // Drop reference, deleting it if it's the last
  m_value->DropReference();
}

void
JSONMessage::Reset(bool p_resetURL /*= false*/)
{
  // Let go of the value
  m_value->DropReference();

  // Set empty value
  m_value = new JSONvalue();

  m_incoming = false;
  // Reset error
  m_errorstate = false;
  m_lastError.Empty();

  // Routing is reset
  m_routing.clear();

  // Do not use incoming headers for outgoing headers
  m_headers.clear();

  // URL
  if(p_resetURL)
  {
    m_url.Empty();
    m_cracked.Reset();
  }

  // Leave the rest for the destination
  // Leave access token untouched!
  // Leave cookie cache untouched!
  // Leave HTTPSite and HTTP_REQUEST for the answer
}

// Parse the URL, true if legal
bool
JSONMessage::ParseURL(XString p_url)
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

XString
JSONMessage::GetContentType() const
{
  if(m_contentType.IsEmpty())
  {
    return _T("application/json");
  }
  return m_contentType;
}

// JSON messages mostly gets send as a POST
// But you can override it for e.g. OData 
XString
JSONMessage::GetVerb()
{
  if(m_verb.IsEmpty())
  {
    return _T("POST");
  }
  return m_verb;
}

XString
JSONMessage::GetRoute(int p_index)
{
  if(p_index >= 0 && p_index < (int) m_routing.size())
  {
    return m_routing[p_index];
  }
  return _T("");
}

void
JSONMessage::SetAcceptEncoding(XString p_encoding)
{
  m_acceptEncoding = p_encoding;
}

void 
JSONMessage::AddHeader(XString p_name,XString p_value)
{
  // Case-insensitive search!
  HeaderMap::iterator it = m_headers.find(p_name);
  if(it != m_headers.end())
  {
    // Check if we set it a duplicate time
    // If appended, we do not append it a second time
    if(it->second.Find(p_value) >= 0)
    {
      return;
    }
    if(p_name.CompareNoCase(_T("Set-Cookie")) == 0)
    {
      // Insert as a new header
      m_headers.insert(std::make_pair(p_name,p_value));
      return;
    }
    // New value of the header
    it->second = p_value;
  }
  else
  {
    // Insert as a new header
    m_headers.insert(std::make_pair(p_name,p_value));
  }
}

void
JSONMessage::DelHeader(XString p_name)
{
  HeaderMap::iterator it = m_headers.find(p_name);
  if(it != m_headers.end())
  {
    m_headers.erase(it);
  }
}

// Finding a header by lowercase name
XString
JSONMessage::GetHeader(XString p_name)
{
  HeaderMap::iterator it = m_headers.find(p_name);
  if(it != m_headers.end())
  {
    return it->second;
  }
  return _T("");
}

// Addressing the message's has three levels
// 1) The complete url containing both server and port number
// 2) Setting server/port/absolute path separately
// 3) By remembering the requestID of the caller
void
JSONMessage::SetURL(const XString& p_url)
{
  m_url = p_url;

  CrackedURL url;
  if(url.CrackURL(p_url))
  {
    m_cracked = url;
  }
  else
  {
    m_cracked.m_valid = false;
  }
}

void
JSONMessage::SetEncoding(Encoding p_encoding)
{
  m_encoding    = p_encoding;
}

// Go from JSON string to this message
// The p_encoding gives the encoding the incoming string is in!
bool
JSONMessage::ParseMessage(XString p_message)
{
  JSONParser parser(this);

  // Starting the parser, preserving it's whitespace state
  parser.ParseMessage(p_message,m_whitespace);

  return (m_errorstate == false);
}

// Reconstruct JSON string from this message
XString 
JSONMessage::GetJsonMessage() const
{
  return m_value->GetAsJsonString(m_whitespace,0,m_exponential);
}

// Use POST method for PUT/MERGE/PATCH/DELETE
// Also known as VERB-Tunneling
bool
JSONMessage::UseVerbTunneling()
{
  if(m_verbTunnel)
  {
    if(m_verb.Compare(_T("POST")) != 0)
    {
      // Change of identity!
      AddHeader(_T("X-HTTP-Method-Override"),m_verb);
      m_verb = _T("POST");
      return true;
    }
  }
  return false;
}

// Load from file
bool
JSONMessage::LoadFile(const XString& p_fileName)
{
  WinFile file(p_fileName);
  if(file.Open(winfile_read | open_trans_text | open_shared_read))
  {
    // Find the length of a file
    size_t length = file.GetFileSize();

    // Check for the streaming limit
    if(length > (size_t)g_streaming_limit)
    {
      file.Close();
      m_errorstate = true;
      m_lastError  = _T("JSON larger than the streaming limit.");
      return false;
    }

    // Read all the contents
    XString line;
    XString contents;
    while(file.Read(line))
    {
      contents += line;
    }

    // Record the found encodings
    m_encoding = file.GetEncoding();
    m_sendBOM  = file.GetFoundBOM();

    // No encoding in the file: presume it is a file from our OS
    // JSON is always UTF-8 by default
    if(!m_sendBOM && m_encoding == Encoding::EN_ACP)
    {
      m_encoding = Encoding::UTF8;
    }

    // Done with the file
    if(!file.Close())
    {
      return false;
    }

    // And parse it
    return ParseMessage(contents);
  }
  return false;
}

// Save to file
bool
JSONMessage::SaveFile(const XString& p_fileName)
{
  bool result = false;

  WinFile file(p_fileName);
  file.SetEncoding(m_encoding);
  if(file.Open(winfile_write))
  {
    XString contents = GetJsonMessage();
    if(file.Write(contents))
    {
      result = true;
    }
    if(!file.Close())
    {
      result = false;
    }
  }
  return result;
}

//////////////////////////////////////////////////////////////////////////
//
// Finding value nodes within the JSON structure
//
//////////////////////////////////////////////////////////////////////////

// Finding the first value of this name
JSONvalue*
JSONMessage::FindValue(XString p_name,bool p_recurse /*=true*/,bool p_object /*= false*/,JsonType* p_type /*=nullptr*/)
{
  // Find from the root value
  return FindValue(m_value,p_name,p_recurse,p_object,p_type);
}

// Finding the first value with this name AFTER the p_from value
JSONvalue* 
JSONMessage::FindValue(JSONvalue* p_from,XString p_name,bool p_recurse /*=true*/,bool p_object /*=false*/,JsonType* p_type /*= nullptr*/)
{
  // Stopping criterion: nothing more to find
  if(p_from == nullptr)
  {
    return nullptr;
  }

  // Recurse through an array
  if(p_from->GetDataType() == JsonType::JDT_array)
  {
    for(auto& val : p_from->GetArray())
    {
      JSONvalue* value = FindValue(&val,p_name,p_recurse,p_object,p_type);
      if(value)
      {
        return value;
      }
    }
  }

  // Recurse through an object
  if(p_from->GetDataType() == JsonType::JDT_object)
  {
    for(auto& val : p_from->GetObject())
    {
      // Stopping at this element
      if(val.m_name.Compare(p_name) == 0)
      {
        if(p_type == nullptr || (*p_type == val.m_value.GetDataType()))
        {
          // Return the object node instead of the pair
          if(p_object)
          {
            return p_from;
          }
          return &val.m_value;
        }
      }
      // Recurse for array and object
      if(p_recurse)
      {
        if(val.m_value.GetDataType() == JsonType::JDT_array ||
           val.m_value.GetDataType() == JsonType::JDT_object)
        {
          JSONvalue* value = FindValue(&val.m_value,p_name,true,p_object,p_type);
          if(value)
          {
            return value;
          }
        }
      }
    }
  }
  return nullptr;
}

// Finding the first name/value pair of this name
JSONpair*
JSONMessage::FindPair(XString p_name,bool p_recursief /*= true*/)
{
  if(m_value)
  {
    return FindPair(m_value,p_name,p_recursief);
  }
  return nullptr;
}

// Finding the first name/value pair AFTER this value
JSONpair* 
JSONMessage::FindPair(JSONvalue* p_value,XString p_name,bool p_recursief /*= true*/)
{
  if(p_value && p_value->GetDataType() == JsonType::JDT_object)
  {
    for(auto& pair : p_value->GetObject())
    {
      if(pair.m_name.Compare(p_name) == 0)
      {
        return &pair;
      }
      if(p_recursief)
      {
        JSONpair* found = FindPair(&(pair.m_value),p_name,true);
        if(found)
        {
          return found;
        }
      }
    }
  }
  if(p_recursief && p_value && (p_value->GetDataType() == JsonType::JDT_array))
  {
    for(auto& val : p_value->GetArray())
    {
      if(val.GetDataType() == JsonType::JDT_object ||
         val.GetDataType() == JsonType::JDT_array  )
      {
        JSONpair* pair = FindPair(&val,p_name,true);
        if(pair)
        {
          return pair;
        }
      }
    }
  }
  return nullptr;
}

// Deleting the first name/value pair of this name
bool
JSONMessage::DeletePair(XString p_name)
{
  if(m_value)
  {
    return DeletePair(m_value,p_name);
  }
  return false;
}

// Finding the first name/value pair AFTER this value
bool
JSONMessage::DeletePair(JSONvalue* p_value,XString p_name)
{
  if(p_value && p_value->GetDataType() == JsonType::JDT_object)
  {
    for(JSONobject::iterator it = p_value->GetObject().begin(); it != p_value->GetObject().end();++it)
    {
      if(it->m_name.Compare(p_name) == 0)
      {
        p_value->GetObject().erase(it);
        return true;
      }
      bool deleted = DeletePair(&(it->m_value),p_name);
      if(deleted)
      {
        return true;
      }
    }
  }
  if(p_value && (p_value->GetDataType() == JsonType::JDT_array))
  {
    for(auto& val : p_value->GetArray())
    {
      if(val.GetDataType() == JsonType::JDT_object ||
         val.GetDataType() == JsonType::JDT_array)
      {
        bool deleted = DeletePair(&val,p_name);
        if(deleted)
        {
          return true;
        }
      }
    }
  }
  return false;
}

// Finding the EXACT JSONvalue (can come from a JSONPath or JSONPointer) !!
bool
JSONMessage::DeletePair(JSONvalue* p_value,JSONvalue* p_base /*= nullptr*/)
{
  if(p_base == nullptr)
  {
    p_base = m_value;
  }

  if(p_base && p_base->GetDataType() == JsonType::JDT_object)
  {
    for(JSONobject::iterator it = p_base->GetObject().begin(); it != p_base->GetObject().end();++it)
    {
      if(&(it->m_value) == p_value)
      {
        p_base->GetObject().erase(it);
        return true;
      }
      bool deleted = DeletePair(p_value,&(it->m_value));
      if(deleted)
      {
        return true;
      }
    }
  }
  if(p_base && (p_base->GetDataType() == JsonType::JDT_array))
  {
    for(JSONarray::iterator it = p_base->GetArray().begin(); it != p_base->GetArray().end();++it)
    {
      if(&(*it) == p_value)
      {
        p_base->GetArray().erase(it);
        return true;
      }
      if(it->GetDataType() == JsonType::JDT_object ||
         it->GetDataType() == JsonType::JDT_array)
      {
        bool deleted = DeletePair(p_value,&(*it));
        if(deleted)
        {
          return true;
        }
      }
    }
  }
  return false;
}

// Get an array element of an array value node
JSONvalue* 
JSONMessage::GetArrayElement(JSONvalue* p_array,int p_index)
{
  if(p_array->GetDataType() == JsonType::JDT_array)
  {
    if(0 <= p_index && p_index < (int)p_array->GetArray().size())
    {
      return &(p_array->GetArray()[p_index]);
    }
  }
  return nullptr;
}

// Get an object element of an object value node
JSONpair* 
JSONMessage::GetObjectElement(JSONvalue* p_object, int p_index)
{
  if (p_object->GetDataType() == JsonType::JDT_object)
  {
    if (0 <= p_index && p_index < (int)p_object->GetObject().size())
    {
      return &(p_object->GetObject()[p_index]);
    }
  }
  return nullptr;
}

// Getting a string value by an unique name
XString
JSONMessage::GetValueString(XString p_name)
{
  XString value;
  const JSONvalue* val = FindValue(p_name);
  if(val && val->GetDataType() == JsonType::JDT_string)
  {
    value = val->GetString();
  }
  return value;
}

// Getting an integer value by an unique name
long
JSONMessage::GetValueInteger(XString p_name)
{
  long value = 0;
  const JSONvalue* val = FindValue(p_name);
  if (val && val->GetDataType() == JsonType::JDT_number_int)
  {
    value = val->GetNumberInt();
  }
  return value;
}

// Getting a number value by an unique name
bcd
JSONMessage::GetValueNumber(XString p_name)
{
  bcd value;
  const JSONvalue* val = FindValue(p_name);
  if (val && val->GetDataType() == JsonType::JDT_number_bcd)
  {
    value = val->GetNumberBcd();
  }
  return value;
}

// Getting a constant (boolean/null) by an unique name
JsonConst
JSONMessage::GetValueConstant(XString p_name)
{
  const JSONvalue* val = FindValue(p_name);
  if (val && val->GetDataType() == JsonType::JDT_const)
  {
    return val->GetConstant();
  }
  return JsonConst::JSON_NONE;
}

bool
JSONMessage::AddNamedObject(XString p_name,const JSONobject& p_object,bool p_forceArray /*=false*/)
{
  // if JSONMessage still empty, turn it into an object
  if(m_value->GetDataType() == JsonType::JDT_const && 
     m_value->GetConstant() == JsonConst::JSON_NONE)
  {
    m_value->SetDatatype(JsonType::JDT_object);
  }

  // Check that the first node is an object
  if(m_value->GetDataType() != JsonType::JDT_object)
  {
    return false;
  }

  // First object of this name?
  JSONpair* insert = FindPair(p_name);
  if (!insert)
  {
    JSONpair pair(p_name,JsonType::JDT_object);
    m_value->Add(pair);

    (*m_value)[p_name].GetObject() = p_object;
    return true;
  }
  else
  {
    JSONvalue* here = FindValue(p_name,true,true);

    // Add to the found pair of the same name
    if(p_forceArray && here->GetObject().size() == 1)
    {
      if(insert->GetDataType() != JsonType::JDT_array)
      {
        JSONarray arr;
        arr.push_back(insert->m_value);
        insert->m_value.SetValue(arr);
      }
      JSONvalue val;
      insert->GetArray().push_back(val);
      insert->GetArray().back().SetValue(p_object);
    }
    else
    {
      JSONpair pair(p_name, JsonType::JDT_object);
      here->Add(pair);
      here->GetObject().back().GetObject() = p_object;
      return true;
    }
  }
  return false;
}

// Start point of a JSON replace
// Searches for JSONpair's that have a matching name pattern
// p_namePattern   -> Must be found within a JSON name of a pair
// p_toFind        -> Text to find in the value part of the pair
// p_replace       -> Found text will be replaced by this text
// p_caseSensitive -> Replacement find is (not) case sensitve
int
JSONMessage::JsonReplace(XString p_namePattern,XString p_tofind,XString p_replace,bool p_caseSensitive /*=true*/)
{
  int number = 0;
  if(m_value)
  {
    if(!p_caseSensitive)
    {
      p_tofind.MakeLower();
    }
    m_value->JsonReplace(p_namePattern,p_tofind,p_replace,number,p_caseSensitive);
  }
  return number;
}

#pragma region References

// XMLElements can be stored elsewhere. Use the reference mechanism to add/drop references
// With the drop of the last reference, the object WILL destroy itself

void
JSONMessage::AddReference()
{
  InterlockedIncrement(&m_references);
}

void
JSONMessage::DropReference()
{
  if(InterlockedDecrement(&m_references) <= 0)
  {
    try
    {
      delete this;
    }
    catch (StdException&)
    {
    }
  }
}

#pragma endregion References
