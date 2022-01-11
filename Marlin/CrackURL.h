/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: CrackURL.h
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

//////////////////////////////////////////////////////////////////////////
// 
// Crack an URL
//
//////////////////////////////////////////////////////////////////////////

#ifndef __CRACKURL__
#define __CRACKURL__
#include <winhttp.h>
#include <vector>

// Parameters of a URL
typedef struct _uri_param
{
  CString	m_key;
  CString m_value;
}
UriParam;
typedef std::vector<UriParam> UriParams;

// URL in cracked down parts
class CrackedURL
{
public:
  CrackedURL();
  CrackedURL(CString p_url);
 ~CrackedURL();

  void      SetPath(CString p_path);
  bool      CrackURL(CString p_url);
  bool      Valid();
  void      Reset();

  // Resulting URL
  CString   URL();
  // Safe URL (without the userinfo)
  CString   SafeURL();
  // Resulting absolute path, including parameters & anchor
  CString   AbsolutePath();
  // Resulting absolute designated resource
  CString   AbsoluteResource();
  // Getting the resource extension
  CString   GetExtension();
  // Resulting UNC
  CString   UNC();
  // Resulting parameter
  unsigned  GetParameterCount();
  UriParam* GetParameter(unsigned p_parameter);
  CString   GetParameter(CString p_parameter);
  bool      HasParameter(CString p_parameter);
  void      SetParameter(CString p_parameter,CString p_value);
  bool      DelParameter(CString p_parameter);

  static    CString   EncodeURLChars(CString p_text,bool p_queryValue = false);
  static    CString   DecodeURLChars(CString p_text,bool p_queryValue = false);

  CrackedURL* operator=(CrackedURL* p_orig);

  // Total status
  bool      m_valid;  // Is a valid URL

  // Cracked down parts of the URL
  CString 	m_scheme;
  bool      m_secure  { false };
  CString   m_host;
  int       m_port    { INTERNET_DEFAULT_HTTP_PORT };
  CString   m_path;
  CString   m_extension;
  UriParams m_parameters;
  CString   m_anchor;

  // Parts found in the url (otherwise empty)
  bool      m_foundScheme;
  bool      m_foundSecure;
  bool      m_foundHost;
  bool      m_foundPort;
  bool      m_foundPath;
  bool      m_foundParameters;
  bool      m_foundAnchor;

private:
  static LPCTSTR m_unsafeString;
  static LPCTSTR m_reservedString;
  static unsigned char GetHexcodedChar(CString& p_string,int& p_index,bool& p_queryValue);
};

inline bool
CrackedURL::Valid() 
{
  return m_valid;
};

inline unsigned
CrackedURL::GetParameterCount()
{
  return (unsigned)m_parameters.size();
}

inline CString
CrackedURL::GetExtension()
{
  return m_extension;
}

#endif
