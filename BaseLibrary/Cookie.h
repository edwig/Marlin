/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: Cookie.h
//
// BaseLibrary: Indispensable general objects and functions
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
#include <vector>

constexpr auto MIN_COOKIE_CHAR = 0x21;
constexpr auto MAX_COOKIE_CHAR = 0x7E;

// Forward declaration of a set of cookies
class Cookies;

// SameSite attribute values
enum class CookieSameSite
{
   NoSameSite = 0  // Do not append SameSite attribute
  ,None       = 1  // Append "SameSite=None"
  ,Lax        = 2  // Append "SameSite=Lax"
  ,Strict     = 3  // Append "SameSite=Strict"
};

class Cookie
{
public:
  Cookie();
  explicit Cookie(XString p_fromHTTP);

  // SETTERS

  void  SetCookie(XString p_value);
  void  SetCookie(XString p_name,XString p_value);
  void  SetCookie(XString p_name,XString p_value,XString p_metadata);
  void  SetCookie(XString p_name,XString p_value,XString p_metadata,bool p_secure,bool p_httpOnly);

  void  SetSecure   (bool p_secure)             { m_secure   = p_secure;   }
  void  SetHttpOnly (bool p_httpOnly)           { m_httpOnly = p_httpOnly; }
  void  SetDomain   (XString p_domain)          { m_domain   = p_domain;   }
  void  SetPath     (XString p_path)            { m_path     = p_path;     }
  void  SetSameSite (CookieSameSite p_sameSite) { m_sameSite = p_sameSite; }
  void  SetMaxAge   (int p_maxAge)              { m_maxAge   = p_maxAge;   }
  void  SetExpires  (SYSTEMTIME* p_expires);

  // GETTERS

  // Compound getters
  XString        GetSetCookieText();
  XString        GetCookieText();
  XString        GetValue(XString p_metadata = _T(""));
  // Individual getters
  XString        GetName()       { return m_name;     }
  bool           GetSecure()     { return m_secure;   }
  bool           GetHttpOnly()   { return m_httpOnly; }
  XString        GetDomain()     { return m_domain;   }
  XString        GetPath()       { return m_path;     }
  CookieSameSite GetSameSite()   { return m_sameSite; }
  SYSTEMTIME*    GetExpires()    { return &m_expires; }
  int            GetMaxAge()     { return m_maxAge;   }
  void           SetExpires(XString p_expires);

  // FUNCTIONS
  bool           IsExpired();

  // OPERATORS
  Cookie&     operator=(Cookie& p_other);

private:
  // Parse cookie text (from HTTP or from internal source)
  void    ParseCookie(XString p_cookieText);
  // Check the contents of the strings
  void    CheckName();
  void    CheckValue();

  // Cookies class copies shallow/deep
  friend  Cookies;

  // The cookie itself
  XString m_name;                                         // Optional!
  XString m_value;                                        // The dough of the cookie 
  // Attributes to the cookie
  bool           m_secure    { false };                   // Secure attribute of the cookie
  bool           m_httpOnly  { false };                   // HTTP Only attribute 
  XString        m_domain;                                // Optional domain
  XString        m_path;                                  // Optional path
  CookieSameSite m_sameSite  { CookieSameSite::NoSameSite }; // SameSite attribute
  // Expiration time
  int            m_maxAge    { 0 };                       // Maximum number of seconds valid
  SYSTEMTIME     m_expires;                               // Valid until this timestamp
};

using BiscuitTin = std::vector<Cookie>;

// Set of cookies for HTTP/SOAP messages
// And for storage in the HTTPClient
class Cookies
{
public:
  void        AddCookie(XString p_fromHTTP);
  void        AddCookie(Cookie& p_cookie);
  void        AddCookie(XString p_name,XString p_value,XString p_metadata = _T(""));
  Cookie*     GetCookie(unsigned p_index = 0);
  Cookie*     GetCookie(XString p_name);
  XString     GetCookieText(); // Client side only!!
  void        Clear()      { m_cookies.clear();       };
  size_t      GetSize()    { return m_cookies.size(); };
  BiscuitTin& GetCookies() { return m_cookies;        };

private:
  void        AddCookieUnique(Cookie& p_cookie);
  // All of our cookies are in the tin
  BiscuitTin  m_cookies;
};
