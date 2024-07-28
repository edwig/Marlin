/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: ConvertWideString.h
//
// BaseLibrary: Indispensable general objects and functions
//
// // Copyright (c) 2014-2024 ir. W.E. Huisman
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
#include "XString.h"
#include "Encoding.h"

// ----------------------------------------------------------------------
// Conversion code-page to code-page
// ---------------------- -------------------- --------------------------
// Extern text (file)    | MBCS application   | Unicode application
// ---------------------- -------------------- --------------------------
// UTF-8                 |-> UTF-16 -> ACP    | -> UTF-16
// UTF-16                |-> ACP              | No conversion!
// UTF-16 LE             |-> B/L -> ACP       | -> B/L
// ANSI/MBCS             |No conversion!      | -> UTF-16
// coding != ACP         |-> UTF-16 -> ACP    | -> UTF-16
// coding == ACP         |No conversion!      | -> UTF-16
// ---------------------- -------------------- --------------------------

// CODE PAGES AND NAMES

// Init the code page system
void    InitCodePageNames();
// Getting the codepage number from the charset
int     CharsetToCodepage(XString p_charset);
// Getting the name of the codepage
XString CodepageToCharset(int p_codepage);
// Getting the description of the codepage
XString CharsetToCodePageInfo(XString p_charset);
// Find a field value within a HTTP header
XString FindFieldInHTTPHeader(XString p_headervalue,XString p_field);
// Set (modify) the field value within a HTTP header
XString SetFieldInHTTPHeader(XString p_headervalue,XString p_field,XString p_value);
// Find the charset in the content-type header
XString FindCharsetInContentType(XString p_contentType);
// Find the mimetype in the content-type header
XString FindMimeTypeInContentType(XString p_contentType);

#ifdef _UNICODE
// Convert strings to/from Unicode-16
bool    TryConvertNarrowString(const BYTE* p_buffer
                              ,int         p_length
                              ,XString     p_charset
                              ,XString&    p_string
                              ,bool&       p_foundBOM);
bool    TryCreateNarrowString (const XString& p_string
                              ,const XString  p_charset
                              ,const bool     p_doBom
                              ,      BYTE**   p_buffer
                              ,      int&     p_length);
// Implode an UTF-16 string to a BYTE buffer (for UTF-8 purposes)
void    ImplodeString(XString p_string,BYTE* p_buffer,unsigned p_length);
XString ExplodeString(BYTE* p_buffer,unsigned p_length);
// Construct a UTF-16 Byte-Order-Mark
XString ConstructBOMUTF16();

#else
// Convert strings from/to Unicode-16
bool    TryConvertWideString(const BYTE* p_buffer
                            ,int         p_length
                            ,XString     p_charset
                            ,XString&    p_string
                            ,bool&       p_foundBOM);
bool    TryCreateWideString (const XString& p_string
                            ,const XString  p_charset
                            ,const bool     p_doBom
                            ,      BYTE**   p_buffer
                            ,      int&     p_length);
// Implode an UTF-16 string to a MBCS XString
XString ImplodeString(BYTE* p_buffer,unsigned p_length);
void    ExplodeString(XString p_string,BYTE* p_buffer,unsigned p_length);
// Construct a UTF-16 Byte-Order-Mark
std::wstring ConstructBOMUTF16();
#endif

// Convert directly
std::wstring StringToWString(XString p_string);
XString      WStringToString(std::wstring p_string);

// Decoding incoming strings from the internet. Defaults to UTF-8 encoding
XString DecodeStringFromTheWire(XString p_string,XString p_charset = _T("utf-8"),bool* p_foundBom = nullptr);
// Encode to string for internet. Defaults to UTF-8 encoding
XString EncodeStringForTheWire (XString p_string,XString p_charset = _T("utf-8"));

// Construct an UTF-8 Byte-Order-Mark
XString ConstructBOMUTF8();
// Scan for UTF-8 chars in a string
bool    DetectUTF8(XString& p_string);
bool    DetectUTF8(const BYTE* p_bytes);

// Convert directly from LPCSTR (No 'T' !!) to XString and vice versa
XString LPCSTRToString(LPCSTR p_string,bool p_utf8 = false);
int     StringToLPCSTR(XString p_string,LPCSTR* p_buffer,int& p_size,bool p_utf8 = false);

// Auto class to convert a XString with ANSI/UTF-16 to a CSTR on the stack
// So that we can call a IIS or MS-Windows API with an explicit LPCSTR parameter
class AutoCSTR
{
public:
  AutoCSTR()
  {
    m_string = nullptr;
    m_length = 0;
  }
  AutoCSTR(XString p_string,bool p_utf8 = false)
   :m_string(nullptr)
   ,m_length(0)
  {
    StringToLPCSTR(p_string,&m_string,m_length,p_utf8);
  }
 ~AutoCSTR()
  {
    delete[] m_string;
  }
  // Results
  LPCSTR cstr()            { return m_string; }
  int    size()            { return m_length; }
  LPCSTR operator() (void) { return m_string; }
  AutoCSTR& operator=(XString p_string)
  {
    delete[] m_string;
    StringToLPCSTR(p_string,&m_string,m_length,false);
    return *this;
  }
  LPCSTR grab()
  {
    LPCSTR temp = m_string;
    m_string = nullptr;
    m_length = 0;
    return temp;
  }
private:
  LPCSTR m_string;  // The ANSI/MBCS string on the internal heap
  int    m_length;  // Length (so we need not call strlen)
};
