/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: ConvertWideString.h
//
// BaseLibrary: Indispensable general objects and functions
//
// // Copyright (c) 2014-2022 ir. W.E. Huisman
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

using uchar = unsigned char;

// String coding of the messages over the Internet
// BEWARE: Values must be the same as JsonEncoding
enum class StringEncoding
{
   ENC_Plain    = 0  // No action taken, use GetACP(): windows-1252 in "The Netherlands"
  ,ENC_UTF8     = 1  // Default XML encoding
  ,ENC_UTF16    = 2  // Default MS-Windows encoding
  ,ENC_ISO88591 = 3  // Default (X)HTML 4.01 encoding
};

// Init the code page system
void    InitCodePageNames();
// Convert strings from/to Unicode
bool    TryConvertWideString(const uchar* p_buffer
                            ,int          p_length
                            ,XString      p_charset
                            ,XString&     p_string
                            ,bool&        p_foundBOM);
bool    TryCreateWideString (const XString& p_string
                            ,const XString  p_charset
                            ,const bool     p_doBom
                            ,      uchar**  p_buffer
                            ,      int&     p_length);
// Convert directly
std::wstring StringToWString(XString p_string);
XString      WStringToString(std::wstring p_string);
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
// Construct an UTF-8 Byte-Order-Mark
XString ConstructBOM();
// Construct a UTF-16 Byte-Order-Mark
std::wstring ConstructBOMUTF16();
// Construct a BOM
XString ConstructBOM(StringEncoding p_encoding);
// Decoding incoming strings from the internet. Defaults to UTF-8 encoding
XString DecodeStringFromTheWire(XString p_string,XString p_charset = "utf-8");
// Encode to string for internet. Defaults to UTF-8 encoding
XString EncodeStringForTheWire (XString p_string,XString p_charset = "utf-8");
// Scan for UTF-8 chars in a string
bool    DetectUTF8(XString& p_string);


