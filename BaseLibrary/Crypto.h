/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: Crypto.h
//
// BaseLibrary: Indispensable general objects and functions
// 
// Created: 2014-2025 ir. W.E. Huisman
// MIT License
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
#include <wincrypt.h>
#include <string>

// Standard signing hashing 
// 
// METHOD    MACRO         PROVIDER       KEY_HASH
// ========= ============= ============== ===============
// sha1      CALG_SHA1     PROV_RSA_FULL  CALG_RC4
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
  explicit Crypto(unsigned p_hash);
 ~Crypto();

  // Set hashing digest method
  unsigned SetHashMethod(XString p_method);
  void     SetHashMethod(unsigned p_hash);
  unsigned GetHashMethod();
  XString  GetHashMethod(unsigned p_hash);

  // ENCRYPT a buffer in AES-256
  XString  Encryption(XString p_input,XString password);
  // DECRYPT a buffer in AES-256
  XString  Decryption(XString p_input,XString password);

  // ENCRYPT a buffer quickly in RC4 through the BCrypt interface
  XString  FastEncryption(const XString& p_input, XString password);
  // DECRYPT a buffer quickly in RC4 through BCrypt interface
  XString  FastDecryption(const XString& p_input, XString password);

  // Make a MD5 Hash value for a buffer
  XString  Digest(const void* data,const size_t data_size,unsigned hashType = 0);
  XString& GetDigest(void);
  void     SetDigestBase64(bool p_base64);

  // Get the protocol types
  XString  GetSSLProtocol(unsigned p_type);
  XString  GetCipherType (unsigned p_type);
  XString  GetKeyExchange(unsigned p_type);

  // Getting the error (if any)
  XString  GetError();

private:
  // ENCRYPT a buffer in AES-256
  XString     ImplementEncryption(const BYTE* p_input,int p_lengthINP,const BYTE* p_password,int p_lengthPWD);
  // DECRYPT a buffer in AES-256
  std::string ImplementDecryption(const BYTE* p_input,int p_lengthINP,const BYTE* p_password,int p_lengthPWD);

  XString  m_error;
  XString	 m_digest;
  unsigned m_hashMethod { CALG_SHA1 };  // Digests are mostly in SHA1
  bool     m_base64     { true      };  // Default as a base64 string instead of binary

  static   CRITICAL_SECTION m_lock;
};

inline XString&
Crypto::GetDigest()
{
  return m_digest;
}

inline void
Crypto::SetDigestBase64(bool p_base64)
{
  m_base64 = p_base64;
}

inline XString
Crypto::GetError()
{
  return m_error;
}

inline unsigned 
Crypto::GetHashMethod()
{
  return m_hashMethod;
}
