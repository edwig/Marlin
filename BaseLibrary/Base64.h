/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: Base64.h
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
#include <WinCrypt.h>

class Base64
{
public:
  Base64(int p_method = CRYPT_STRING_BASE64,int p_options = CRYPT_STRING_NOCRLF);

  // XString variants
  XString  Encrypt(XString p_unencrypted);
  XString  Decrypt(XString p_encrypted);
  // Buffer to string and vice-versa
  XString  Encrypt(BYTE* p_buffer,int p_length);
  bool     Decrypt(XString p_encrypted,BYTE* p_buffer,int p_length);
  int      Decrypt(BYTE* p_buffer,int p_blen,BYTE* p_output,int p_olen);
  // Encrypt a ANSI/UNICODE string to a ANSI/UNICODE base64, where chars > 0x0FF
  XString  EncryptUnicode(XString p_unencrypted);

  // Helper methods
  size_t   B64_length  (size_t len);
  size_t   Ascii_length(size_t len);
private:
  int m_method;
  int m_options;
};

class CRC4 
{
public:
  BYTE*   Encrypt(BYTE* pszText,int plen,const BYTE* pszKey,int klen);
  BYTE*   Decrypt(BYTE* pszText,int plen,const BYTE* pszKey,int klen);
  XString Encrypt(const XString p_unencrypted,const XString p_key);
  XString Decrypt(const XString p_encrypted,  const XString p_key);
};