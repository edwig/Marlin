/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: CrackURL.cpp
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

//////////////////////////////////////////////////////////////////////////
//
// CRACK / SPLIT A HTTP URI (URL in HTTP) IN IT'S COMPOSING PARTS
//
//
//  foo://example.com:8042/over/there/index.dtb?type=animal&name=white%20narwhal#nose
//  \_/   \_________/ \__/            \___/ \_/ \______________________________/ \__/
//   |         |       |                |    |            |                       |
//   |      hostname  port              |    |          query                   anchor
//   |    \______________/\_____________|____|/ \__/       \___/
//   |            |                |    |    |    |          |
//scheme      authority          path   |    |    interpretable as keys
// name   \_____________________________|____|/     \_____/     \______________/
//   |          |                       |    |         |             |
//   |       hierarchical part          |    |    interpretable as values
//   |                                  |    |
//   |            path               interpretable as filename 
//   |   ___________|____________            |
//  / \ /                        \           |
//  urn:example:animal:ferret:nose           interpretable as extension
//
// HTTP EXAMPLE
//
// scheme
//  name       userinfo       hostname       path        query      anchor
//  _|__   _______|_______   ____|____   _____|_____  _____|______  ___|___
// /    \ /               \ /         \ /           \/            \/       \
// http://username:password@example.com/path/file.ext?subject=Topic#position
//
#include "pch.h"
#include "CrackURL.h"
#include "ConvertWideString.h"
#include <winhttp.h>

#ifdef _AFX
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#endif

LPCTSTR CrackedURL::m_unsafeString   = _T(" \"@<>#{}|\\^~[]`");
LPCTSTR CrackedURL::m_reservedString = _T("$&/;?-!*()'"); // ".,+_:="

CrackedURL::CrackedURL()
{
  Reset();
}

// CRACK A URL/URI
CrackedURL::CrackedURL(XString p_url)
{
  CrackURL(p_url);
}

// Explicit DTOR was needed to prevent crashes in production code
// Several crash reports support this sad behavior
CrackedURL::~CrackedURL()
{
  m_parameters.clear();
}

void
CrackedURL::Reset()
{
  // Reset status
  m_valid = false;
  // Reset the answering structure
  m_scheme = _T("http");
  m_secure = false;
  m_host.Empty();
  m_port = INTERNET_DEFAULT_HTTP_PORT;
  m_path.Empty();
  m_parameters.clear();
  m_anchor.Empty();
  m_extension.Empty();

  // reset the found-statuses
  m_foundScheme     = false;
  m_foundSecure     = false;
  m_foundHost       = false;
  m_foundPort       = false;
  m_foundPath       = false;
  m_foundExtension  = false;
  m_foundParameters = false;
  m_foundAnchor     = false;
}

bool
CrackedURL::CrackURL(XString p_url)
{
  // Reset total url
  Reset();

  // Check that there IS an URL!
  if(p_url.IsEmpty())
  {
    return false;
  }
  // Find the scheme
  int pos = p_url.Find(':');
  if(pos <= 0)
  {
    return false;
  }
  m_foundScheme = true;
  m_scheme = p_url.Left(pos);
  if(m_scheme.CompareNoCase(_T("https")) == 0)
  {
    m_secure      = true;
    m_foundSecure = true;
    m_port        = INTERNET_DEFAULT_HTTPS_PORT;
  }
  // Remove the scheme
  p_url = p_url.Mid(pos + 1);
  
  // Check for '//'
  if(p_url.GetAt(0) != '/' && p_url.GetAt(1) != '/')
  {
    return false;
  }
  // Remove '//'
  p_url = p_url.Mid(2);
  
  // Check for server:port
  XString server;
  pos = p_url.Find('/');
  if(pos <= 0)
  {
    // No absolute pathname
    server = p_url;
    p_url.Empty();
  }
  else
  {
    m_foundPath = true;
    server = p_url.Left(pos);
    p_url = p_url.Mid(pos);
  }
  // Find the port. 
  // BEWARE OF IPv6 TEREDO ADDRESSES!!
  // So skip over '[ad:::::a:b:c]' part
  pos = server.Find('[');
  if(pos >= 0)
  {
    pos = server.Find(']',pos+1);
  }
  pos = server.Find(':',pos + 1);
  if(pos > 0)
  {
    m_host = server.Left(pos);
    m_port = _ttoi(server.Mid(pos + 1));
    m_foundPort = true;
  }
  else
  {
    // No port specified
    m_host = server;
    m_port = m_secure ? INTERNET_DEFAULT_HTTPS_PORT 
                      : INTERNET_DEFAULT_HTTP_PORT;
  }
  
  // Check that there IS a server host
  if(m_host.IsEmpty())
  {
    return false;
  }
  
  // Find Query or Anchor
  int query  = p_url.Find('?');
  int anchor = p_url.Find('#');
  
  if(query > 0 || anchor > 0)
  {
    // Chop of the anchor
    if(anchor > 0)
    {
      m_foundAnchor = true;
      m_anchor = DecodeURLChars(p_url.Mid(anchor + 1));
      p_url    = p_url.Left(anchor);
    }
    // Remember the absolute path name
    if(query > 0)
    {
      m_path = p_url.Left(query);
      p_url  = p_url.Mid(query + 1);
    }
    else
    {
      m_path = p_url;
    }
    if(m_path.GetLength() > 0)
    {
      m_foundPath = true;
      m_path = DecodeURLChars(m_path);
    }

    // Find all query parameters
    while(query > 0)
    {
      // FindNext query
      query = p_url.Find('&');
      XString part;
      if(query > 0)
      {
        part  = p_url.Left(query);
        p_url = p_url.Mid(query + 1);
      }
      else
      {
        part = p_url;
      }

      UriParam param;
      pos = part.Find('=');
      if(pos > 0)
      {
        param.m_key   = DecodeURLChars(part.Left(pos));
        param.m_value = DecodeURLChars(part.Mid(pos + 1),true);
      }
      else
      {
        // No value. Use as key only
        param.m_key = DecodeURLChars(part);
      }
      // Save parameter
      m_parameters.push_back(param);
      m_foundParameters = true;
    }
  }
  else
  {
    // No query and no anchor	
    m_path = p_url;
    m_foundPath = !m_path.IsEmpty();
  }
  // Reduce path: various 'mistakes'
  m_path.Replace(_T("//"),  _T("/"));
  m_path.Replace(_T("\\\\"),_T("\\"));
  m_path.Replace(_T("\\"),  _T("/"));

  // Find the extension for the media type
  // Media types are stored without the '.'
  int posp = m_path.ReverseFind('.');
  int poss = m_path.ReverseFind('/');
  
  if(posp >= 0 && posp > poss)
  {
    m_extension = m_path.Mid(posp + 1);
    // Most possibly part of an embedded string in the URL
    // and not file extension of some sort
    if(m_extension.Find('\'') >= 0 || m_extension.Find('\"') >= 0)
    {
      m_extension.Empty();
    }
    else
    {
      m_foundExtension = true;
    }
  }
  // Now a valid URL
  return (m_valid = true);
}

// Allow a '+' in an URL (webservers should be configured!)
void
CrackedURL::SetAllowPlusSign(bool p_allow)
{
  m_allowPlus = p_allow;
}

void
CrackedURL::SetPath(XString p_path)
{
  // Strip parameters and anchors
  int pos = p_path.FindOneOf(_T("#?"));
  if (pos >= 0)
  {
    p_path = p_path.Left(pos);
  }
  m_path = p_path;
}

// Convert string to URL encoding, using UTF-8 chars
XString
CrackedURL::EncodeURLChars(XString p_text,bool p_queryValue /*=false*/)
{
  XString encoded;
  uchar*  buffer = nullptr;
  int     length = 0;

#ifdef _UNICODE
  if(!TryCreateNarrowString(p_text,_T("utf-8"),false,&buffer,length))
  {
    return p_text;
  }
#else
  // Now encode MBCS to UTF-8
  if(TryCreateWideString(p_text,"",false,&buffer,length))
  {
    bool foundBom = false;
    if(TryConvertWideString(buffer,length,_T("utf-8"),encoded,foundBom))
    {
      delete[] buffer;
      length = encoded.GetLength();
      buffer = new uchar[length + 1];
      memcpy(buffer,encoded.GetString(),length + 1);
      encoded.Empty();
    }
  }
#endif

  // Re-encode the string: Now in buffer/length
  // Watch out: strange code ahead!
  uchar last = 0;
  int singlequote = 0;
  int doublequote = 0;

  for(int ind = 0;ind < length; ++ind)
  {
    uchar ch = buffer[ind];
    if(ch == '?')
    {
      p_queryValue = true;
    }
    else if(ch == '\'') ++singlequote;
    else if(ch == '\"') ++doublequote;
    else if(ch == '0x09' || ch == '0x0D' || ch == '0x20')
    {
      // Collapse all red/white space chars to space
      ch = ' ';
    }
    if(last == ' ' && ch == ' ')
    {
      // Collapse whitespace to one single space
      if((singlequote % 2 == 0) &&
         (doublequote % 2 == 0) )
      {
        continue;
      }
    }
    else if(_tcschr(m_unsafeString,ch) ||                      // " \"<>#{}|\\^~[]`"    -> All strings
           (p_queryValue && _tcsrchr(m_reservedString,ch)) ||  // "$&+,./:;=?@-!*()'"   -> In query parameter value strings
           (ch < 0x20) ||                                      // 7BITS ASCII Control characters
           (ch > 0x7F) )                                       // Converted UTF-8 characters
    {
      encoded.AppendFormat(_T("%%%2.2X"),(int)ch);
    }
    else
    {
      encoded += (_TUCHAR) ch;
    }
    last = ch;
  }
  delete[] buffer;
  return encoded;
}

// Decode URL strings, including UTF-8 encoding
XString
CrackedURL::DecodeURLChars(XString p_text,bool p_queryValue /*=false*/,bool p_allowPlus /*=true*/)
{
  XString encoded;
  XString decoded;
  bool  convertUTF = false;
  bool  percent    = false;

  // Whole string decoded for %XX strings
  for(int ind = 0;ind < p_text.GetLength(); ++ind)
  {
    uchar ch = GetHexcodedChar(p_text,ind,percent,p_queryValue,p_allowPlus);
    decoded += ch;
    if(ch > 0x7F && percent)
    {
      convertUTF = true;
    }
  }
#ifdef _UNICODE
  if(convertUTF)
  {
    // Compress to real UTF-8
    int length = decoded.GetLength();
    uchar* buffer = new uchar[length + 1];
    bool foundBom = false;

    // Make a real UTF-8 memory string
    ImplodeString(decoded,buffer,length);

    // Convert to UTF-16
    bool converted = TryConvertNarrowString(reinterpret_cast<const uchar*>(buffer),length,_T("utf-8"),decoded,foundBom);
    delete[] buffer;

    // No glory, end of the line!
    if(!converted)
    {
      return p_text;
    }
  }
#else

  // Only if we found Unicode chars, should we start the whole shebang!
  if(convertUTF)
  {
    // Now decode the UTF-8 in the encoded string, to decoded MBCS
    uchar* buffer = nullptr;
    int    length = 0;
    if(TryCreateWideString(decoded,_T("utf-8"),false,&buffer,length))
    {
      bool foundBom = false;
      if(TryConvertWideString(buffer,length,"",encoded,foundBom))
      {
        decoded = encoded;
      }
    }
    delete [] buffer;
  }
#endif
  return decoded;
}

// Decode 1 hex char for URL decoding
uchar
CrackedURL::GetHexcodedChar(XString& p_text
                           ,int&     p_index
                           ,bool&    p_percent
                           ,bool&    p_queryValue
                           ,bool     p_allowPlus /* = true */)
{
  uchar ch = (uchar) p_text.GetAt(p_index);

  if(ch == '%')
  {
    int num1 = 0;
    int num2 = 0;

    p_percent = true;
         if(isdigit (p_text.GetAt(++p_index))) num1 = p_text.GetAt(p_index) - '0';
    else if(isxdigit(p_text.GetAt(  p_index))) num1 = toupper(p_text.GetAt(p_index)) - 'A' + 10;
         if(isdigit (p_text.GetAt(++p_index))) num2 = p_text.GetAt(p_index) - '0';
    else if(isxdigit(p_text.GetAt(  p_index))) num2 = toupper(p_text.GetAt(p_index)) - 'A' + 10;

    ch = (uchar)(16 * num1 + num2);
  }
  else if(ch == '?')
  {
    p_queryValue = true;
  }
  else if(ch == '+' && p_queryValue)
  {
    // See RFC 1630: Google Chrome still does this!
    ch = p_allowPlus ? '+' : ' ';
  }
  return ch;
}

// Resulting URL
// Reconstruct the URL parts to a complete URL
// foo://example.com:8042/over/there/index.dtb?type=animal&name=white%20narwhal#nose
XString
CrackedURL::URL() const
{
  // Check if we have an empty URL in effect
  if(m_host.IsEmpty())
  {
    return _T("");
  }

  // Reconstruct the URL from here
  XString url(m_scheme);

  // Secure HTTP 
  if(m_secure && m_scheme.CompareNoCase(_T("https")) != 0)
  {
    url += _T("s");
  }
  // Separator
  url += _T("://");

  // Add hostname
  url += m_host;

  // Possibly add a port
  if( (m_secure == false && m_port != INTERNET_DEFAULT_HTTP_PORT) ||
      (m_secure == true  && m_port != INTERNET_DEFAULT_HTTPS_PORT) )
  {
    XString portnum;
    portnum.Format(_T(":%d"),m_port);
    url += portnum;
  }

  // Now add absolute path and possibly parameters and anchor
  url += AbsolutePath();

  // This is the result
  return url;
}

// Safe URL (without the userinfo)
XString
CrackedURL::SafeURL() const
{
  XString url(m_scheme);

  // Secure HTTP 
  if(m_secure && m_scheme.CompareNoCase(_T("https")) != 0)
  {
    url += _T("s");
  }
  // Separator
  url += _T("://");

  // Add host name
  url += m_host;

  // Possibly add a port
  if((m_secure == false && m_port != INTERNET_DEFAULT_HTTP_PORT) ||
     (m_secure == true  && m_port != INTERNET_DEFAULT_HTTPS_PORT))
  {
    XString portnum;
    portnum.Format(_T(":%d"),m_port);
    url += portnum;
  }

  // Now add absolute path and possibly parameters and anchor
  url += AbsolutePath();

  // This is the result
  return url;
}

// Resulting absolute path
XString 
CrackedURL::AbsoluteResource() const
{
  // Begin with the base path
  return EncodeURLChars(m_path);
}

// Resulting absolute path + parameters + anchor
// But without protocol and user/password/server/port
XString
CrackedURL::AbsolutePath() const
{
  // Begin with the base path
  XString url = EncodeURLChars(m_path);

  // Adding parameters?
  if(m_parameters.size())
  {
    url += _T("?");

    UriParams::const_iterator it;
    for(it = m_parameters.begin();it != m_parameters.end(); ++it)
    {
      const UriParam& param = *it;

      // Next parameter
      if(it != m_parameters.begin())
      {
        url += _T("&");
      }
      url += EncodeURLChars(param.m_key);
      if(!param.m_value.IsEmpty())
      {
        url += _T("=");
        url += EncodeURLChars(param.m_value,true);
      }
    }
  }

  // Adding the anchor
  if(!m_anchor.IsEmpty())
  {
    url += _T("#");
    url += EncodeURLChars(m_anchor);
  }

  // This is the result
  return url;
}

// Setting a new extension on the path
void
CrackedURL::SetExtension(XString p_extension)
{
  m_extension = p_extension;

  int pos1 = m_path.ReverseFind('/');
  int pos2 = m_path.ReverseFind('.');

  if(pos1 > 0 && pos2 > 0 && pos2 > pos1)
  {
    m_path = m_path.Left(pos2);
  }
  if(!p_extension.IsEmpty())
  {
    m_path = m_path + _T(".") + p_extension;
  }
}

// As a resulting UNC path for a file
// \\server@port\abs-path\file.ext
XString
CrackedURL::UNC() const
{
  XString path(_T("\\\\"));

  path += m_host;

  if(m_port != INTERNET_DEFAULT_HTTP_PORT)
  {
    path += _T("@");
    path.AppendFormat(_T("%d"),m_port);
  }
  
  if(!m_path.IsEmpty())
  {
    XString pathname(m_path);
    pathname.Replace(_T("/"),_T("\\"));

    path += _T("\\");//??
    path += pathname;
  }
  return path;
}

CrackedURL* 
CrackedURL::operator=(CrackedURL* p_orig)
{
  // Copy the info
  m_valid     = p_orig->m_valid;
  m_scheme    = p_orig->m_scheme;
  m_secure    = p_orig->m_secure;
  m_host      = p_orig->m_host;
  m_port      = p_orig->m_port;
  m_path      = p_orig->m_path;
  m_extension = p_orig->m_extension;
  m_anchor    = p_orig->m_anchor;

  // Copy status info
  m_foundScheme     = p_orig->m_foundScheme;
  m_foundSecure     = p_orig->m_foundSecure;
  m_foundHost       = p_orig->m_foundHost;
  m_foundPort       = p_orig->m_foundPort;
  m_foundPath       = p_orig->m_foundPath;
  m_foundParameters = p_orig->m_foundParameters;
  m_foundAnchor     = p_orig->m_foundAnchor;

  // Copy all parameters
  UriParams::iterator it;
  for(it  = p_orig->m_parameters.begin();
      it != p_orig->m_parameters.end(); 
      ++it)
  {
     UriParam param;
     param.m_key   = it->m_key;
     param.m_value = it->m_value;
     m_parameters.push_back(param);
  }
  
  // Return ourselves
  return this;
}

// Has parameter value
const bool
CrackedURL::HasParameter(XString p_parameter) const
{
  for(unsigned i = 0; i < m_parameters.size(); ++i)
  {
    if(m_parameters[i].m_key.CompareNoCase(p_parameter) == 0)
    {
      return true;
    }
  }
  return false;
}

const XString 
CrackedURL::GetParameter(XString p_parameter) const
{
  for(auto& param : m_parameters)
  {
    if(param.m_key.CompareNoCase(p_parameter) == 0)
    {
      return param.m_value;
    }
  }
  return _T("");
}

void    
CrackedURL::SetParameter(XString p_parameter,XString p_value)
{
  // See if we already have this parameter name
  for(auto& param : m_parameters)
  {
    if(param.m_key.CompareNoCase(p_parameter) == 0)
    {
      param.m_value = p_value;
      return;
    }
  }

  // Make a new parameter for the URL
  UriParam param;
  param.m_key   = p_parameter;
  param.m_value = p_value;

  // Save them
  m_parameters.push_back(param);
  m_foundParameters = true;
}

const UriParam*
CrackedURL::GetParameter(unsigned p_parameter) const
{
  if(p_parameter < (unsigned)m_parameters.size())
  {
    return &m_parameters[p_parameter];
  }
  return nullptr;
}

bool
CrackedURL::DelParameter(XString p_parameter)
{
//   auto it = std::find_if(m_parameters.begin(),m_parameters.end(),p_parameter);
//   if(it != m_parameters.end())
//   {
//     m_parameters.erase(it);
//     return true;
//   }

  for(UriParams::iterator it = m_parameters.begin(); it != m_parameters.end();++it)
  {
    if(it->m_key.CompareNoCase(p_parameter) == 0)
    {
      m_parameters.erase(it);
      return true;
    }
  }
  return false;
}
