/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: CrackURL.h
//
// BaseLibrary: Indispensable general objects and functions
//
// // Copyright (c) 2014-2025 ir. W.E. Huisman
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
// Crack an URL
//
//////////////////////////////////////////////////////////////////////////

#pragma once
#include "XString.h"
#include <winhttp.h>
#include <vector>

// Parameters of a URL
typedef struct _uri_param
{
  XString m_key;
  XString m_value;
}
UriParam;
typedef std::vector<UriParam> UriParams;

// URL in cracked down parts
class CrackedURL
{
public:
  CrackedURL();
  explicit CrackedURL(XString p_url);
 ~CrackedURL();

  void      SetPath(XString p_path);
  bool      CrackURL(XString p_url);
  bool      Valid() const;
  void      Reset();

  // Resulting URL
  XString   URL() const;
  // Safe URL (without the userinfo)
  XString   SafeURL() const;
  // Resulting absolute path, including parameters & anchor
  XString   AbsolutePath() const;
  // Resulting absolute designated resource
  XString   AbsoluteResource() const;
  // Getting the resource extension
  XString   GetExtension() const;
  // Setting a new extension on the path
  void      SetExtension(XString p_extension);
  // Allow a '+' in an URL (webservers should be configured!)
  void      SetAllowPlusSign(bool p_allow);
  // Resulting UNC
  XString   UNC() const;
  // Resulting parameter
  unsigned  GetParameterCount() const;
  const UriParam* GetParameter(unsigned p_parameter) const;
  const XString   GetParameter(XString p_parameter)  const;
  const bool      HasParameter(XString p_parameter)  const;
  void      SetParameter(XString p_parameter,XString p_value);
  bool      DelParameter(XString p_parameter);

  static    XString   EncodeURLChars(XString p_text,bool p_queryValue = false);
  static    XString   DecodeURLChars(XString p_text,bool p_queryValue = false,bool p_allowPlus = true);

  CrackedURL* operator=(CrackedURL* p_orig);

  // Total status
  bool      m_valid;  // Is a valid URL

  // Cracked down parts of the URL
  XString 	m_scheme;
  bool      m_secure  { false };
  XString   m_host;
  int       m_port    { INTERNET_DEFAULT_HTTP_PORT };
  XString   m_path;
  XString   m_extension;
  UriParams m_parameters;
  XString   m_anchor;
  bool      m_allowPlus { true };

  // Parts found in the url (otherwise empty)
  bool      m_foundScheme;
  bool      m_foundSecure;
  bool      m_foundHost;
  bool      m_foundPort;
  bool      m_foundPath;
  bool      m_foundExtension;
  bool      m_foundParameters;
  bool      m_foundAnchor;

private:
  static LPCTSTR m_unsafeString;
  static LPCTSTR m_reservedString;
  static uchar   GetHexcodedChar(XString& p_string
                                ,int&     p_index
                                ,bool&    p_percent
                                ,bool&    p_queryValue
                                ,bool     p_allowPlus = true);
};

inline bool
CrackedURL::Valid() const
{
  return m_valid;
};

inline unsigned
CrackedURL::GetParameterCount() const
{
  return (unsigned)m_parameters.size();
}

inline XString
CrackedURL::GetExtension() const
{
  return m_extension;
}
