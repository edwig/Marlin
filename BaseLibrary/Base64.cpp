/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: Base64.cpp
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
#include "pch.h"
#include "Base64.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;

#endif

Base64::Base64(int p_method /*= CRYPT_STRING_BASE64*/,int p_options /*= CRYPT_STRING_NOCRLF*/)
{
  if(p_method >= 0 && p_method <= CRYPT_STRING_BASE64URI)
  {
    m_method = p_method;
  }
  else
  {
    m_method = CRYPT_STRING_BASE64;
  }
  if(p_options & (CRYPT_STRING_PERCENTESCAPE | 
                  CRYPT_STRING_HASHDATA      |
                  CRYPT_STRING_STRICT        | 
                  CRYPT_STRING_NOCRLF        |
                  CRYPT_STRING_NOCR ))
  {
    m_options = p_options;
  }
}

size_t 
Base64::B64_length(size_t len)
{
  size_t  npad = len%3;
  size_t  size = (npad > 0)? (len +3-npad ) : len; // padded for multiple of 3 bytes
  return  (size*8)/6;
}

size_t 
Base64::Ascii_length(size_t len)
{
  return  (len*6)/8;
}

// ANSI Version only
XString
Base64::Encrypt(BYTE* p_buffer,int p_length)
{
  if(p_length <= 0)
  {
    return XString();
  }
  DWORD tchars = 0;
  CryptBinaryToString(p_buffer,p_length,m_method | m_options,(LPTSTR)NULL,  &tchars);
  _TUCHAR* buffer = new _TUCHAR[tchars + 2];
  CryptBinaryToString(p_buffer,p_length,m_method | m_options,(LPTSTR)buffer,&tchars);
  buffer[tchars] = 0;
  XString result(buffer);
  delete[] buffer;
  return result;
}

// ANSI/UNICODE aware version
XString
Base64::Encrypt(XString p_unencrypted)
{
  if(p_unencrypted.GetLength() == 0)
  {
    return XString();
  }
  DWORD tchars = 0;
  CryptBinaryToString((const BYTE*)p_unencrypted.GetString(),p_unencrypted.GetLength() * sizeof(TCHAR),m_method | m_options,(LPTSTR) NULL,&tchars);
  _TUCHAR* buffer = new _TUCHAR[tchars + 2];
  CryptBinaryToString((const BYTE*)p_unencrypted.GetString(),p_unencrypted.GetLength() * sizeof(TCHAR),m_method | m_options,(LPTSTR) buffer,&tchars);
  buffer[tchars] = 0;
  XString result(buffer);
  delete[] buffer;
  return result;
}

XString
Base64::Decrypt(XString p_encrypted)
{
  if(p_encrypted.GetLength() == 0)
  {
    return XString();
  }
  DWORD length = 0;
  DWORD type = CRYPT_STRING_BASE64_ANY;
  CryptStringToBinary(p_encrypted.GetString(),p_encrypted.GetLength(),m_method,NULL,&length,0,&type);
  _TUCHAR* buffer = new _TUCHAR[length + 2];
  CryptStringToBinary(p_encrypted.GetString(),p_encrypted.GetLength(),m_method,(BYTE*) buffer,&length,0,&type);
  buffer[length / sizeof(TCHAR)] = 0;
  XString result(buffer);
  delete[] buffer;
  return result;
}

bool
Base64::Decrypt(XString p_encrypted,BYTE* p_buffer,int p_length)
{
  if(p_encrypted.GetLength() == 0 || p_length <= 0)
  {
    return XString();
  }
  DWORD length = 0;
  CryptStringToBinary(p_encrypted.GetString(),p_encrypted.GetLength(),m_method,NULL,&length,0,NULL);
  if((DWORD)p_length >= length)
  {
    CryptStringToBinary(p_encrypted.GetString(),p_encrypted.GetLength(),m_method,p_buffer,&length,0,NULL);
    p_buffer[length] = 0;
    return true;
  }
  return false;
}

// ANSI ONLY implementation. E.G. for UTF-8 traffic or HTTP headers
bool
Base64::Decrypt(BYTE* p_buffer,int p_blen,BYTE* p_output,int p_olen)
{
  if(p_blen <= 0 || p_olen <= 0)
  {
    p_output[0] = 0;
    return true;
  }
  DWORD length = 0;
  CryptStringToBinaryA((LPCSTR)p_buffer,p_blen,m_method,NULL,&length,0,NULL);
  if((DWORD) p_olen >= length)
  {
    CryptStringToBinaryA((LPCSTR)p_buffer,p_blen,m_method,p_output,&length,0,NULL);
    p_output[length] = 0;
    return true;
  }
  return false;
}

//////////////////////////////////////////////////////////////////////////
//
// CRC4 Encryption/Decryption
//
//////////////////////////////////////////////////////////////////////////

#define SWAP(a, b) ((a) ^= (b), (b) ^= (a), (a) ^= (b))

BYTE*
CRC4::Encrypt(BYTE* pszText,int plen,const BYTE* pszKey,int klen) 
{
  unsigned char sbox[256];      /* Encryption array             */
  unsigned char key [256];      /* Numeric key values           */

  memset(sbox,0,256);
  memset(key, 0,256);

  int i = 0;
  int j = 0;
  int n = 0;
  int m = 0;

  for (m = 0;  m < 256; m++)  /* Initialize the key sequence */
  {
    *(key + m)= *(pszKey + (m % plen));
    *(sbox + m) = (BYTE) m;
  }

  for (m=0; m < 256; m++)
  {
    n = (n + *(sbox+m) + *(key + m)) &0xff;
    SWAP(*(sbox + m),*(sbox + n));
  }

  // ilen = (int)_tcslen(pszText);
  for (m = 0; m < klen; m++)
  {
    i = (i + 1) &0xff;
    j = (j + *(sbox + i)) &0xff;
    SWAP(*(sbox+i),*(sbox + j));  /* randomly Initialize the key sequence */
    BYTE k = *(sbox + ((*(sbox + i) + *(sbox + j)) &0xff ));
    if(k == *(pszText + m))       /* avoid '\0' beween the decoded text; */
    {
      k = 0;
    }
    *(pszText + m) ^=  k;
  }
  // remove Key traces from memory
  memset(sbox,0,256);  
  memset(key, 0,256);   

  return pszText;
}

BYTE*
CRC4::Decrypt(BYTE* pszText,int plen,const BYTE* pszKey,int klen)
{
  // Using the same function as encoding
  return Encrypt(pszText,plen,pszKey,klen);
}

XString
CRC4::Encrypt(const XString p_unencrypted,const XString p_key)
{
  int plen = p_unencrypted.GetLength() * sizeof(TCHAR);
  int klen = p_key.GetLength()         * sizeof(TCHAR);

  XString result(p_unencrypted);
  Encrypt((BYTE*)result.GetString(),plen,(BYTE*)p_key.GetString(),klen);
  return result;
}

XString
CRC4::Decrypt(XString const p_encrypted,XString const p_key)
{
  // Using the same function as encoding
  return Encrypt(p_encrypted,p_key);
}

