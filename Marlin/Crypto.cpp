/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: Crypto.cpp
//
// Marlin Server: Internet server/client
// 
// Copyright (c) 2016 ir. W.E. Huisman
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
#include "Stdafx.h"
#include "Crypto.h"
#include "Base64.h"
#include <wincrypt.h>
#include <schannel.h>

// Encryption providers support password-hashing and encryption algorithms
// See the documentation on: "Cryptographic Provider Types"
// https://msdn.microsoft.com/en-us/library/windows/desktop/aa380244(v=vs.85).aspx
#define ENCRYPT_PROVIDER    PROV_RSA_AES  // PROV_RSA_FULL
#define ENCRYPT_PASSWORD    CALG_SHA_256  // CALG_MD5
#define ENCRYPT_ALGORITHM   CALG_AES_256  // CALG_RC2
#define ENCRYPT_BLOCK_SIZE  256 

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

Crypto::Crypto()
       :m_hashMethod(CALG_SHA1)
{
}

Crypto::Crypto(unsigned p_hash)
       :m_hashMethod(p_hash)
{
}

Crypto::~Crypto()
{
}

// Make a MD5 Hash value for a buffer
// Mostly taken from: 
// https://msdn.microsoft.com/en-us/library/windows/desktop/aa380270(v=vs.85).aspx
//
CString&
Crypto::Digest(CString& p_buffer,CString& p_password)
{
  HCRYPTPROV hCryptProv = NULL; 
  HCRYPTHASH hHashPass  = NULL;
  HCRYPTHASH hHashData  = NULL;
  HCRYPTKEY  hKey       = NULL;
  PBYTE      pbHash     = NULL;
  DWORD      dwHashLen  = 0;
  DWORD      cbContent  = p_buffer.GetLength(); 
  DWORD      cbPassword = p_password.GetLength();
  BYTE*      pbContent  = (BYTE*)p_buffer.GetBuffer(cbContent); 
  BYTE*      pbPassword = (BYTE*)p_password.GetBuffer(cbPassword);
  Base64     base64;
  HMAC_INFO  hMacInfo;
  unsigned   hashMethod = m_hashMethod;
  DWORD      provider   = PROV_RSA_FULL; // Default provider
  DWORD      keyHash    = CALG_RC4;      // Default password hashing

  // Check combinations to get the MS-Windows standard crypto provider working
  // See remarks in MSDN for ::GetHashMethod()
  switch(hashMethod)
  {
    case CALG_SHA1:     // Secure-Hash-Algorithm.
                        // one of: PROV_RSA_FULL/PROV_RSA_SCHANNEL/PROV_RSA_AES/PROV_DSS_DH
                        provider   = PROV_RSA_FULL;   
                        break;
    case CALG_HMAC:     // Hashbased Message Authentication Code (HMAC)
                        // hmac-sha1 is a fallback to SHA1
                        hashMethod = CALG_SHA1;
                        provider   = PROV_RSA_FULL;
                        break;
    case CALG_DSS_SIGN: // Digital Signature Standard
                        provider   = PROV_DSS_DH;
                        hashMethod = CALG_SHA; 
                        keyHash    = CALG_CYLINK_MEK;
                        break;
    case CALG_SHA_256:  // Fall through
    case CALG_RSA_SIGN: // RSA-Labratories Signature
                        provider   = PROV_RSA_AES;
                        hashMethod = CALG_SHA_256;
                        keyHash    = CALG_AES_256;
                        break;
    case CALG_SHA_384:  provider   = PROV_RSA_AES;
                        hashMethod = CALG_SHA_384;
                        keyHash    = CALG_AES_256;
                        break;
    case CALG_SHA_512:  provider   = PROV_RSA_AES;
                        hashMethod = CALG_SHA_512;
                        keyHash    = CALG_AES_256;
                        break;
  }

  ZeroMemory(&hMacInfo,sizeof(hMacInfo));
  hMacInfo.HashAlgid = hashMethod;

  if(!CryptAcquireContext(&hCryptProv
                        ,NULL
                        ,NULL
                        ,provider // PROV_* macro
                        ,CRYPT_VERIFYCONTEXT))
  {
    m_error.Format("Error acquiring encryption context: 0x%08x",GetLastError());
    goto error_exit;
  }

  // Make a password hash
  if(!CryptCreateHash(hCryptProv, 
                      hashMethod, // CALG_* algorithm identifier definitions see: wincrypt.h
                      0, 
                      0, 
                      &hHashPass)) 
  {
    m_error.Format("Error creating password hash: 0x%08x",GetLastError());
    goto error_exit;
  }

  // Crypt the password
  if(!CryptHashData(hHashPass,pbPassword,cbPassword,0))
  {
    m_error.Format("Error hashing the password: 0x%08x",GetLastError());
    goto error_exit;
  }

  // Getting the key handle of the password
  if(!CryptDeriveKey(hCryptProv,keyHash,hHashPass,0,&hKey)) //CALG_RC4
  {
    m_error.Format("Getting derived encryption key: 0x%08x",GetLastError());
    goto error_exit;
  }

  // Create a hash for the data
  if(!CryptCreateHash(hCryptProv,CALG_HMAC,hKey,0,&hHashData))
  {
    m_error.Format("Error creating data hash: 0x%08x",GetLastError());
    goto error_exit;
  }

  // Set info parameter
  if(!CryptSetHashParam(hHashData,HP_HMAC_INFO,(BYTE*)&hMacInfo,0))
  {
    m_error.Format("Error setting hash-hmac-info parameter: 0x%08x",GetLastError());
    goto error_exit;
  }

  // Provide locked buffer to Crypt API
  if(!CryptHashData(hHashData,pbContent,cbContent,0))
  {
    m_error.Format("Error hashing the data: 0x%08x",GetLastError());
    goto error_exit;
  }

  // Getting the hashvalue data length
  if(!CryptGetHashParam(hHashData, HP_HASHVAL, NULL, &dwHashLen, 0)) 
  {
    m_error.Format("Error getting hash data length: 0x%08x",GetLastError());
    goto error_exit;
  }

  // Alloc memory for the hash data
  pbHash = new BYTE[dwHashLen + 1];

  // Getting the hash at last
  if(!CryptGetHashParam(hHashData, HP_HASHVAL, pbHash, &dwHashLen, 0))
  {
    m_error.Format("Getting final hash data: 0x%08x",GetLastError());
    goto error_exit;
  }

  // Make a string version of the numeric digest value
  m_digest.Empty();

  // Create a base64 string of the hash data
  int b64length = (int) base64.B64_length(dwHashLen);
  char* buffer  = m_digest.GetBufferSetLength(b64length);
  base64.Encrypt((const unsigned char*)pbHash,dwHashLen,(unsigned char*)buffer);
  m_digest.ReleaseBuffer(b64length);

error_exit:
  if(pbHash)
  {
    delete [] pbHash;
  }
  if(hHashData)
  {
    CryptDestroyHash(hHashData);
  }
  if(hKey)
  {
    CryptDestroyKey(hKey);
  }
  if(hHashPass)
  {
    CryptDestroyHash(hHashPass);
  }
  CryptReleaseContext(hCryptProv,0); 
  p_buffer.ReleaseBuffer();
  p_password.ReleaseBuffer();

  return m_digest; 
}

// ENCRYPT a buffer
CString 
Crypto::Encryptie(CString p_input,CString p_password)
{
  HCRYPTPROV hCryptProv = NULL; 
  HCRYPTKEY  hCryptKey  = NULL;
  HCRYPTHASH hCryptHash = NULL;
  DWORD      dwDataLen  = 0;
  DWORD      dwBuffLen  = 0;
  BYTE*      pbData     = NULL;
  CString    result;
  Base64     base64;

  // Build data buffer that's large enough
  dwDataLen = p_input.GetLength();
  dwBuffLen = p_input.GetLength() + ENCRYPT_BLOCK_SIZE;
  pbData    = new BYTE[dwBuffLen];

  strcpy_s((char*)pbData,dwBuffLen,p_input.GetString());

  // Encrypt by way of a cryptographic provider
  if(!CryptAcquireContext(&hCryptProv,NULL,NULL,ENCRYPT_PROVIDER,CRYPT_VERIFYCONTEXT))
  {
    m_error.Format("Error acquiring encryption context: 0x%08x",GetLastError());
    goto error_exit;
  }

  if(!CryptCreateHash(hCryptProv,ENCRYPT_PASSWORD,0,0,&hCryptHash)) // MD5?
  {
    m_error.Format("Error creating encryption hash: 0x%08x",GetLastError());
    goto error_exit;
  }

  if(!CryptHashData(hCryptHash,(BYTE *)p_password.GetString(),p_password.GetLength(),0))
  {
    m_error.Format("Error hashing password for encryption: 0x%08x",GetLastError());
    goto error_exit;
  }

  if(!CryptDeriveKey(hCryptProv,ENCRYPT_ALGORITHM,hCryptHash,0,&hCryptKey))
  {
    m_error.Format("Error deriving password key for encryption: 0x%08x",GetLastError());
    goto error_exit;
  }

  // TRACING
  // TRACE("ENCRYPT PASSW: %s\n",p_password);
  // TRACE("ENCRYPT INPUT: %s\n",pbData);

  DWORD dwFlags = 0;
  if(CryptEncrypt(hCryptKey
                  ,NULL            // No hashing
                  ,TRUE            // Final action
                  ,dwFlags         // Reserved
                  ,pbData          // The data
                  ,&dwDataLen      // Pointer to data length
                  ,dwBuffLen))     // Pointer to buffer length
  {
    // TRACING
    // for(unsigned ind = 0;ind < dwDataLen; ++ind)
    // {
    //   TRACE("ENCRYPT: %02X\n",pbData[ind]);
    // }

    // Create a base64 string of the hash data
    int b64length = (int) base64.B64_length(dwDataLen);
    char* buffer  = result.GetBufferSetLength(b64length);
    base64.Encrypt((const unsigned char*)pbData,dwDataLen,(unsigned char*)buffer);
    result.ReleaseBuffer(b64length);

    // TRACING
    // TRACE("ENCRYPT OUTPUT: %s\n",result);
  }
  else
  {
    m_error.Format("Error encrypting data: 0x%08x",GetLastError());
  }
error_exit:
  // Freeing everything
  if(hCryptKey)
  {
    CryptDestroyKey(hCryptKey);
  }
  if(hCryptHash)
  {
    CryptDestroyHash(hCryptHash);
  }
  if(pbData)
  {
    delete [] pbData;
  }
  if(hCryptProv)
  {
    CryptReleaseContext(hCryptProv,0);
  }
  return result;
}

// DECRYPT a buffer
CString 
Crypto::Decryptie(CString p_input,CString p_password)
{
  HCRYPTPROV hCryptProv = NULL; 
  HCRYPTKEY  hCryptKey  = NULL;
  HCRYPTHASH hCryptHash = NULL;
  DWORD      dwDataLen  = 0;
  DWORD      dwBuffLen  = 0;
  BYTE*      pbData     = NULL;
  CString    result;
  Base64     base64;

  dwDataLen = p_input.GetLength();

  // Check if we have anything to do
  if(dwDataLen < 3)
  {
    return result;
  }

  // TRACING
  // TRACE("DECRYPT PASSW: %s\n",p_password);
  // TRACE("DECRYPT INPUT: %s\n",p_input);

  // Create a data string of the base64 string
  dwBuffLen = (DWORD) base64.Ascii_length(dwDataLen);
  pbData    = new BYTE[dwBuffLen + 1];
  base64.Decrypt((const unsigned char*) p_input.GetString(),dwDataLen,(unsigned char*)pbData);

  // TRACING
  // for(unsigned ind = 0; ind < dwBuffLen; ++ind)
  // {
  //   TRACE("DECRYPT: %02X\n",pbData[ind]);
  // }
  // Maximum of 2 times a trailing zero at a base64
  // You MUST take them of, otherwise decrypting will not work
  // as the block size of the algorithm is incorrect.
  if(!pbData[dwBuffLen - 1]) --dwBuffLen;
  if(!pbData[dwBuffLen - 1]) --dwBuffLen;
  
  // Decrypt by way of a cryptographic provider
  if(!CryptAcquireContext(&hCryptProv,NULL,NULL,ENCRYPT_PROVIDER,CRYPT_VERIFYCONTEXT))
  {
    m_error.Format("Crypto context not aquired: 0x%08x",GetLastError());
    goto error_exit;
  }
  if(!CryptCreateHash(hCryptProv,ENCRYPT_PASSWORD,0,0,&hCryptHash)) // MD5?
  {
    m_error.Format("Hash not created for decryption: 0x%08x",GetLastError());
    goto error_exit;
  }

  if(!CryptHashData(hCryptHash,(BYTE *)p_password.GetString(),p_password.GetLength(),0))
  {
    m_error.Format("Error hashing password data for decryption: 0x%08x",GetLastError());
    goto error_exit;
  }

  if(!CryptDeriveKey(hCryptProv,ENCRYPT_ALGORITHM,hCryptHash,0,&hCryptKey))
  {
    m_error.Format("Error creating derived key for decryption: 0x%08x",GetLastError());
    goto error_exit;
  }
  DWORD dwFlags = CRYPT_OAEP;
  if(CryptDecrypt(hCryptKey
                  ,NULL            // No hashing
                  ,TRUE            // Final action
                  ,dwFlags         // Reserved
                  ,pbData          // The data
                  ,&dwBuffLen))    // Pointer to data length
  {
    pbData[dwBuffLen] = 0;
    result = pbData;

    // TRACING
    // TRACE("DECRYPT OUTPUT: %s\n",result);
  }
  else
  {
    m_error.Format("Decrypting not done: 0x%08x",GetLastError());
  }
error_exit:
  // Freeing everything
  if(hCryptKey)
  {
    CryptDestroyKey(hCryptKey);
  }
  if(hCryptHash)
  {
    CryptDestroyHash(hCryptHash);
  }
  if(pbData)
  {
    delete [] pbData;
  }
  if(hCryptProv)
  {
    CryptReleaseContext(hCryptProv,0);
  }
  return result;
}

// Set hashing digest method
unsigned 
Crypto::SetHashMethod(CString p_method)
{
  // Preferred by the standard: http://www.w3.org/2000/09/xmldsig#
  if(p_method.Compare("sha1")      == 0) m_hashMethod = CALG_SHA1;
  if(p_method.Compare("hmac-sha1") == 0) m_hashMethod = CALG_HMAC;
  if(p_method.Compare("dsa-sha1")  == 0) m_hashMethod = CALG_DSS_SIGN; 
  if(p_method.Compare("rsa-sha1")  == 0) m_hashMethod = CALG_RSA_SIGN; 

  // Older methods (Not in the standard, but exists on the MS-Windows system)
  if(p_method.Compare("md2") == 0) m_hashMethod = CALG_MD2;
  if(p_method.Compare("md4") == 0) m_hashMethod = CALG_MD4;
  if(p_method.Compare("md5") == 0) m_hashMethod = CALG_MD5;

  // Newer (safer?) methods. Not in the year 2000 standard
  if(p_method.Compare("sha2-256") == 0) m_hashMethod = CALG_SHA_256;
  if(p_method.Compare("sha2-384") == 0) m_hashMethod = CALG_SHA_384;
  if(p_method.Compare("sha2-512") == 0) m_hashMethod = CALG_SHA_512;

  return m_hashMethod;
}

void
Crypto::SetHashMethod(unsigned p_hash)
{
  if(p_hash == CALG_SHA         ||
   //p_hash == CALG_SHA1        ||
     p_hash == CALG_HMAC        ||
     p_hash == CALG_DSS_SIGN    ||
     p_hash == CALG_RSA_SIGN    ||
     p_hash == CALG_MD2         ||
     p_hash == CALG_MD4         ||
     p_hash == CALG_MD5         ||
     p_hash == CALG_SHA_256     ||
     p_hash == CALG_SHA_384     ||
     p_hash == CALG_SHA_512     )
  {
    // Setting the hash method
    m_hashMethod = p_hash;
  }
}

CString  
Crypto::GetHashMethod(unsigned p_hash)
{
  switch(p_hash)
  {
    // Standard ds-sign
    default:            // Fall through
    case CALG_SHA1:     return "sha1";      // WERKT
    case CALG_HMAC:     return "hmac-sha1"; // WERKT Synonym for SHA1
    case CALG_DSS_SIGN: return "dsa-sha1";  // WERKT
    case CALG_RSA_SIGN: return "rsa-sha1";  // WERKT
    // older
    case CALG_MD2:      return "md2";       // WERKT
    case CALG_MD4:      return "md4";       // WERKT
    case CALG_MD5:      return "md5";       // WERKT
    // Newer
    case CALG_SHA_256:  return "sha2-256";  // WERKT 
    case CALG_SHA_384:  return "sha2-384";  // WERKT
    case CALG_SHA_512:  return "sha2-512";  // WERKT
    // No hash
    case 0:             return "no-hash";
  }
}

CString
Crypto::GetCipherType(unsigned p_type)
{
  CString cipher;
  switch(p_type)
  {
    case CALG_3DES:       cipher = "3DES Block";   break;
    case CALG_3DES_112:   cipher = "DES 112 Bits"; break;
    case CALG_AES_128:    cipher = "AES 128 Bits"; break;
    case CALG_AES_192:    cipher = "AES 192 Bits"; break;
    case CALG_AES_256:    cipher = "AES 256 Bits"; break;
    case CALG_AES:        cipher = "AES Block";    break;
    case CALG_DES:        cipher = "DES Block";    break;
    case CALG_DESX:       cipher = "DESX Block";   break;
    case CALG_RC2:        cipher = "RC2 Block";    break;
    case CALG_RC4:        cipher = "RC4 Block";    break;
    case CALG_RC5:        cipher = "RC5 Block";    break;
    case CALG_SEAL:       cipher = "SEAL";         break;
    case CALG_SKIPJACK:   cipher = "SkipJack";     break;
    case CALG_TEK:        cipher = "TEK";          break;
    case CALG_CYLINK_MEK: cipher = "Cylink";       break;
    case 0:               cipher = "No encryption";break;
    default:              cipher = "AES";          break;
  }
  return cipher;
}

CString  
Crypto::GetKeyExchange(unsigned p_type)
{
  CString exchange;

  switch(p_type)
  {
    case CALG_RSA_KEYX:      exchange = "RSA KEYX";       break;
    case CALG_DH_SF:         exchange = "Diffie-Hellman"; break;
    case CALG_DH_EPHEM:      exchange = "DH EPHEM";       break;
    case CALG_AGREEDKEY_ANY: exchange = "Any-AgreedKey";  break;
    case CALG_KEA_KEYX:      exchange = "KEA KEYX";       break;
    case CALG_HUGHES_MD5:    exchange = "Hughes MD5";     break;
    case CALG_ECDH:          exchange = "ECDH";           break;
    case CALG_ECMQV:         exchange = "ECMQV";          break;
    default:                 exchange = "AES";            break;
  }
  return exchange;
}

CString  
Crypto::GetSSLProtocol(unsigned p_type)
{
  CString protocol;

  switch(p_type)
  {
    case SP_PROT_TLS1_CLIENT:  protocol = "TLS 1 Client";     break;
    case SP_PROT_TLS1_SERVER:  protocol = "TLS 1 Server";     break;
    case SP_PROT_SSL3_CLIENT:  protocol = "SSL 3 Client";     break;
    case SP_PROT_SSL3_SERVER:  protocol = "SSL 3 Server";     break;
    case SP_PROT_TLS1_1_CLIENT:protocol = "TLS 1.1 Client";   break;
    case SP_PROT_TLS1_1_SERVER:protocol = "TLS 1.1 Server";   break;
    case SP_PROT_TLS1_2_CLIENT:protocol = "TLS 1.2 Client";   break;
    case SP_PROT_TLS1_2_SERVER:protocol = "TLS 1.2 Server";   break;
    case SP_PROT_PCT1_CLIENT:  protocol = "Private Communications 1 Client"; break;
    case SP_PROT_PCT1_SERVER:  protocol = "Private Communications 1 Server"; break;
    case SP_PROT_SSL2_CLIENT:  protocol = "SSL 2 Client";     break;
    case SP_PROT_SSL2_SERVER:  protocol = "SSL 2 Server";     break;
    case SP_PROT_DTLS_CLIENT:  protocol = "DTLS Client";      break;
    case SP_PROT_DTLS_SERVER:  protocol = "DTLS Server";      break;
    default:                   protocol = "Combined SSL/TLS"; break;
  }
  return protocol;
}
