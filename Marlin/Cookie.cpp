/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: Cookie.cpp
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
#include "stdafx.h"
#include "Cookie.h"
#include "Crypto.h"
#include "Winhttp.h"
#include "HTTPTime.h"
#include "StdException.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// XTOR: Empty cookie
Cookie::Cookie()
{
  memset(&m_expires,0,sizeof(SYSTEMTIME));
}

// XTOR as gotten from the server
Cookie::Cookie(CString p_fromHTTP)
{
  memset(&m_expires,0,sizeof(SYSTEMTIME));
  ParseCookie(p_fromHTTP);
}

// Set simple value "Cookie: value"
void  
Cookie::SetCookie(CString p_value)
{
  m_value    = p_value;
  m_secure   = false;
  m_httpOnly = false;
  memset(&m_expires,0,sizeof(SYSTEMTIME));
  m_name.Empty();
  CheckName();
}

// Set cookie "Cookie: name=value"
void  
Cookie::SetCookie(CString p_name,CString p_value)
{
  m_name     = p_name;
  m_value    = p_value;
  m_secure   = false;
  m_httpOnly = false;
  memset(&m_expires,0,sizeof(SYSTEMTIME));
  CheckName();
  CheckValue();
}

// Set std encrypted cookie "Set-Cookie: session=A7EB89D231; secure; httponly"
void  
Cookie::SetCookie(CString p_name,CString p_value,CString p_metadata)
{
  Crypto crypt;

  m_name     = p_name;
  m_value    = crypt.Encryption(p_value,p_metadata);
  m_secure   = true;   // Secure encoded trough HTTPS or by our encryption
  m_httpOnly = true;   // Intended for browser only, do not expose to JavaScript
  memset(&m_expires,0,sizeof(SYSTEMTIME));
  CheckName();
  CheckValue();
}

// Set cookie to your liking "Set-Cookie: [name=]value [; secure][; httponly]"
void  
Cookie::SetCookie(CString p_name,CString p_value,CString p_metadata,bool p_secure,bool p_httpOnly)
{
  Crypto crypt;

  m_name     = p_name;
  m_value    = p_metadata.IsEmpty() ? p_value : crypt.Encryption(p_value,p_metadata);
  m_secure   = p_secure;
  m_httpOnly = p_httpOnly;
  memset(&m_expires,0,sizeof(SYSTEMTIME));
  CheckName();
  CheckValue();
}

// Getting the value, optionally decrypting the cookie
CString
Cookie::GetValue(CString p_metadata /*=""*/)
{
  if(m_secure || !p_metadata.IsEmpty())
  {
    Crypto crypt;
    return crypt.Decryption(m_value,p_metadata);
  }
  else
  {
    return m_value;
  }
}

// Cookie text for server side "Set-Cookie:" header
CString     
Cookie::GetSetCookieText()
{
  CString cookie;

  // Elementary name-value pair (cookie-crumble)
  if(!m_name.IsEmpty())
  {
    cookie = m_name + "=";
  }
  cookie += m_value;

  // Path/domain/expires
  if(!m_path.IsEmpty())
  {
    cookie.AppendFormat("; Path=%s",m_path.GetString());
  }
  if(!m_domain.IsEmpty())
  {
    cookie.AppendFormat("; Domain=%s",m_domain.GetString());
  }
  if(m_expires.wYear != 0)
  {
    USES_CONVERSION;
    WCHAR pwszTimeStr[WINHTTP_TIME_FORMAT_BUFSIZE+2];

    // Convert the current time to HTTP format.
    if(!WinHttpTimeFromSystemTime(&m_expires,(LPWSTR) &pwszTimeStr))
    {
      CString error;
      error.Format("Error %u in WinHttpTimeFromSystemTime.\n",GetLastError());
      throw StdException(error.GetString());
    }
    CString expires = CW2A((LPCWSTR)&pwszTimeStr);
    cookie.AppendFormat("; Expires=%s",expires.GetString());
  }
  // Optional flags
  if(m_secure)
  {
    cookie += "; Secure";
  }
  if(m_httpOnly)
  {
    cookie += "; HttpOnly";
  }
  return cookie;
}

// Cookie text for client side "Cookie:" header
CString
Cookie::GetCookieText()
{
  CString cookie;
  if(!m_name.IsEmpty())
  {
    cookie = m_name + "=";
  }
  cookie += m_value;
  return cookie;
}

// Setting new system time
void  
Cookie::SetExpires(SYSTEMTIME* p_expires)
{
  memcpy(&m_expires,p_expires,sizeof(SYSTEMTIME));
}

// Setting new system time from HTTP text
void
Cookie::SetExpires(CString p_expires)
{
  if(!HTTPTimeToSystemTime(p_expires,&m_expires))
  {
    CString error;
    error.Format("Error %u in HTTPTimeToSystemTime.\n",GetLastError());
    throw StdException(error);
  }
}

// Parse cookie text (from HTTP or from internal source)
// Cannot throw (is called from XTOR)
void    
Cookie::ParseCookie(CString p_cookieText)
{
  CString cookie(p_cookieText);
  cookie.Trim();

  // Find name
  int pos = cookie.Find('=');
  if(pos > 0)
  {
    m_name = cookie.Left(pos);
    cookie = cookie.Mid(pos + 1);
    cookie.Trim();
  }

  // Find value
  pos = cookie.Find(';');
  if(pos > 0)
  {
    m_value = cookie.Left(pos);
    cookie  = cookie.Mid(pos + 1);
    cookie.Trim();
  }
  else
  {
    // That's about it. No more attributes
    m_value = cookie;
    return;
  }

  // Find attributes (path/domain/expires/secure/httponly
  while(cookie.GetLength())
  {
    CString part,partname;
    // Find ';'
    pos = cookie.Find(';');
    if(pos > 0)
    {
      part = cookie.Left(pos);
      cookie = cookie.Mid(pos+1);
      cookie.Trim();
    }
    else
    {
      part = cookie;
      cookie.Empty();
    }
    // Find the '='
    pos = part.Find('=');
    if(pos > 0)
    {
      partname = part.Left(pos);
      part     = part.Mid(pos + 1);
      part.Trim();
    }
    else
    {
      partname.Empty();
    }

    // interpret the part
    if(partname.CompareNoCase("path")    == 0) m_path = part;
    if(partname.CompareNoCase("domain")  == 0) m_domain = part;
    if(partname.CompareNoCase("expires") == 0) SetExpires(part);
    if(part.CompareNoCase("secure")      == 0) m_secure = true;
    if(part.CompareNoCase("httponly")    == 0) m_httpOnly = true;
  }
  return;
}

// Check if correct naming conventions are used for cookie's name
void
Cookie::CheckName()
{
  for(int ind = 0; ind < m_name.GetLength(); ++ind)
  {
    unsigned char ch = m_name.GetAt(ind);
    if((ch < MIN_COOKIE_CHAR || ch > MAX_COOKIE_CHAR) || (ch == '=') || (ch == ';'))
    {
      throw StdException("Cookie name characters out of range 0x21-0x7E");
    }
  }
}

// Check if correct naming conventions are used for cookie's value
void
Cookie::CheckValue()
{
  for(int ind = 0;ind < m_value.GetLength(); ++ind)
  {
    unsigned char ch = m_value.GetAt(ind);
    if((ch < MIN_COOKIE_CHAR || ch > MAX_COOKIE_CHAR) || (ch == ';'))
    {
      throw StdException("Cookie value characters out of range 0x21-0x7E");
    }
  }
}

// Test if our cookie is already expired
bool
Cookie::IsExpired()
{
  // Attribute never set
  if(m_expires.wYear == 0)
  {
    return false;
  }
  // Test against the system time
  SYSTEMTIME time;
  GetSystemTime(&time);
  return memcmp(&time,&m_expires,sizeof(SYSTEMTIME)) < 0;
}

// Copying a cookie (for the cookie cache)
Cookie&
Cookie::operator=(Cookie& p_other)
{
  // Check if we are not assigning to ourselves
  if(&p_other == this)
  {
    return *this;
  }
  // Assign all values
  m_name     = p_other.m_name;
  m_value    = p_other.m_value;
  m_secure   = p_other.m_secure;
  m_httpOnly = p_other.m_httpOnly;
  m_domain   = p_other.m_domain;
  m_path     = p_other.m_path;
  memcpy(&m_expires,&p_other.m_expires,sizeof(SYSTEMTIME));

  return *this;
}

//////////////////////////////////////////////////////////////////////////
//
// Cookie cache
//
//////////////////////////////////////////////////////////////////////////

// Add cookie to cache from HTTP
void
Cookies::AddCookie(CString p_fromHTTP)
{
  if(p_fromHTTP.IsEmpty())
  {
    return;
  }
  Cookie cookie(p_fromHTTP);
  AddCookieUnique(cookie);
}

// Add cookie from other cache
void    
Cookies::AddCookie(Cookie& p_cookie)
{
  AddCookieUnique(p_cookie);
}

// Add std encrypted cookie
void    
Cookies::AddCookie(CString p_name,CString p_value,CString p_metadata /*=""*/)
{
  if(p_name.IsEmpty() && p_value.IsEmpty())
  {
    return;
  }
  Cookie cookie;
  cookie.SetCookie(p_name,p_value,p_metadata);
  AddCookieUnique(cookie);
}

// Getting the cookies one-by-one
// Default is the first cookie for applications that handle only ONE cookie
Cookie* 
Cookies::GetCookie(unsigned p_index /*=0*/)
{
  if(m_cookies.size() > p_index)
  {
    return &m_cookies[p_index];
  }
  return nullptr;
}

// Getting a cookie by name
Cookie*
Cookies::GetCookie(CString p_name)
{
  for(auto& cookie : m_cookies)
  {
    if(cookie.GetName().CompareNoCase(p_name) == 0)
    {
      return &cookie;
    }
  }
  return nullptr;
}

// Client side only!!
// For constructing "Cookie: value1; value2; name=value3"
// Without any attributes to the cookie
// So only for "Cookie:", not for "Set-Cookie" !!!
CString
Cookies::GetCookieText()
{
  CString text;
  for(auto& cookie : m_cookies)
  {
    if(!text.IsEmpty())
    {
      text += "; ";
    }
    text += cookie.GetCookieText();
  }
  return text;
}

// Adding a cookie to the cache, but recycles cookies with existing names
void    
Cookies::AddCookieUnique(Cookie& p_cookie)
{
  for(auto& cookie : m_cookies)
  {
    if(cookie.GetName().IsEmpty() || p_cookie.GetName().IsEmpty())
    {
      continue;
    }
    if(cookie.GetName().CompareNoCase(p_cookie.GetName()) == 0)
    {
      // Keep this cookie's values
      cookie = p_cookie;
      return;
    }
  }
  m_cookies.push_back(p_cookie);
}
