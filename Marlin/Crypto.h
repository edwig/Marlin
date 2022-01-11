/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: Crypto.h
//
// Marlin Server: Internet server/client
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

// Standard signing hashing 
// 
// METHOD    MACRO         PROVIDER       KEY_HASH
// ========= ============= ============== ===============
// sha1      CALG_SHA1     PROV_RSA_FULL  CALG_RC4
// hmac-sha1 CALG_HMAC     PROV_RSA_FULL  CALG_RC4
// dsa-sha1  CALG_DSS_SIGN PROV_DSS_SH    CALG_CYLINK_MEK
// rsa-sha1  CALG_RSA_SIGN PROV_RSA_AES   CALG_AES_256
// --------- ------------- -------------- ---------------
// md2       CALG_MD2      PROV_RSA_FULL  CALG_RC4
// md4       CALG_MD4      PROV_RSA_FULL  CALG_RC4
// md5       CALG_MD5      PROV_RSA_FULL  CALG_RC4
// --------- ------------- -------------- ---------------
// sha2-256  CALG_SHA_256  PROV_RSA_AES   CALG_AES_256
// sha2-384  CALG_SHA_384  PROV_RSA_AES   CALG_AES_256
// sha2-512  CALG_SHA_512  PROV_RSA_AES   CALG_AES_256

// Extra memory might be needed in a decryption buffer
#define MEMORY_PARAGRAPH 16

class Crypto
{
public:
  Crypto();
  Crypto(unsigned p_hash);
 ~Crypto();

  // Set hashing digest method
  unsigned SetHashMethod(CString p_method);
  void     SetHashMethod(unsigned p_hash);
  unsigned GetHashMethod();
  CString  GetHashMethod(unsigned p_hash);

  // ENCRYPT a buffer in AES-256
  CString  Encryption(CString p_input,CString password);
  // DECRYPT a buffer in AES-256
  CString  Decryption(CString p_input,CString password);

  // ENCRYPT a buffer quickly in RC4 through the BCrypt interface
  CString  FastEncryption(CString p_input, CString password);
  // DECRYPT a buffer quickly in RC4 through BCrypt interface
  CString  FastDecryption(CString p_input, CString password);

  // Make a MD5 Hash value for a buffer
  CString& Digest(CString& p_buffer,CString& p_password);
  CString  Digest(const void* data,const size_t data_size,unsigned hashType);
  CString& GetDigest(void);

  // Get the protocol types
  CString  GetSSLProtocol(unsigned p_type);
  CString  GetCipherType (unsigned p_type);
  CString  GetKeyExchange(unsigned p_type);

  // Getting the error (if any)
  CString  GetError();

private:
  CString  m_error;
  CString	 m_digest;
  unsigned m_hashMethod;

  static   CRITICAL_SECTION m_lock;
};

inline CString&
Crypto::GetDigest()
{
  return m_digest;
}

inline CString
Crypto::GetError()
{
  return m_error;
}

inline unsigned 
Crypto::GetHashMethod()
{
  return m_hashMethod;
}
