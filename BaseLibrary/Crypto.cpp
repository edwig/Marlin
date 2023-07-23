/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: Crypto.cpp
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
#include "Crypto.h"
#include "Base64.h"
#include "GetLastErrorAsString.h"
#include "AutoCritical.h"
#include <wincrypt.h>
#include <bcrypt.h>
#include <schannel.h>
#include <vector>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// Encryption providers support password-hashing and encryption algorithms
// See the documentation on: "Cryptographic Provider Types"
// https://msdn.microsoft.com/en-us/library/windows/desktop/aa380244(v=vs.85).aspx
#define ENCRYPT_PROVIDER    PROV_RSA_AES
#define ENCRYPT_PASSWORD    CALG_SHA_256
#define ENCRYPT_ALGORITHM   CALG_AES_256

#ifndef STATUS_SUCCESS
#define STATUS_SUCCESS (0x00000000)
#endif

static bool m_crypt_init = false;
CRITICAL_SECTION Crypto::m_lock;

Crypto::Crypto()
       :m_hashMethod(CALG_SHA1)
{
  if(!m_crypt_init)
  {
    InitializeCriticalSection(&m_lock);
    m_crypt_init = true;
  }
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
XString&
Crypto::Digest(XString& p_buffer,XString& p_password)
{
  AutoCritSec lock(&m_lock);

  HCRYPTPROV  hCryptProv = NULL; 
  HCRYPTHASH  hHashPass  = NULL;
  HCRYPTHASH  hHashData  = NULL;
  HCRYPTKEY   hKey       = NULL;
  PBYTE       pbHash     = NULL;
  DWORD       dwHashLen  = 0;
  DWORD       cbContent  = p_buffer.GetLength(); 
  DWORD       cbPassword = p_password.GetLength();
  const BYTE* pbContent  = reinterpret_cast<const BYTE*>(p_buffer.GetString()); 
  const BYTE* pbPassword = reinterpret_cast<const BYTE*>(p_password.GetString());
  Base64      base64;
  HMAC_INFO   hMacInfo;
  unsigned    hashMethod = m_hashMethod;
  DWORD       provider   = PROV_RSA_FULL; // Default provider
  DWORD       keyHash    = CALG_RC4;      // Default password hashing

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
    case CALG_SHA_256:  [[fallthrough]];
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
    m_error.Format("Error acquiring encryption context: 0x%08Xl",GetLastError());
    goto error_exit;
  }

  // Make a password hash
  if(!CryptCreateHash(hCryptProv, 
                      hashMethod, // CALG_* algorithm identifier definitions see: wincrypt.h
                      0, 
                      0, 
                      &hHashPass)) 
  {
    m_error.Format("Error creating password hash: 0x%08Xl",GetLastError());
    goto error_exit;
  }

  // Crypt the password
  if(!CryptHashData(hHashPass,pbPassword,cbPassword,0))
  {
    m_error.Format("Error hashing the password: 0x%08Xl",GetLastError());
    goto error_exit;
  }

  // Getting the key handle of the password
  if(!CryptDeriveKey(hCryptProv,keyHash,hHashPass,0,&hKey)) //CALG_RC4
  {
    m_error.Format("Getting derived encryption key: 0x%08Xl",GetLastError());
    goto error_exit;
  }

  // Create a hash for the data
  if(!CryptCreateHash(hCryptProv,CALG_HMAC,hKey,0,&hHashData))
  {
    m_error.Format("Error creating data hash: 0x%08Xl",GetLastError());
    goto error_exit;
  }

  // Set info parameter
  if(!CryptSetHashParam(hHashData,HP_HMAC_INFO,reinterpret_cast<BYTE*>(&hMacInfo),0))
  {
    m_error.Format("Error setting hash-hmac-info parameter: 0x%08Xl",GetLastError());
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
    m_error.Format("Error getting hash data length: 0x%08Xl",GetLastError());
    goto error_exit;
  }

  // Alloc memory for the hash data
  pbHash = new BYTE[(size_t)dwHashLen + 1];

  // Getting the hash at last
  if(!CryptGetHashParam(hHashData, HP_HASHVAL, pbHash, &dwHashLen, 0))
  {
    m_error.Format("Getting final hash data: 0x%08Xl",GetLastError());
    goto error_exit;
  }

  {
    // Make a string version of the numeric digest value
    m_digest.Empty();

    // Create a base64 string of the hash data
    int b64length = (int)base64.B64_length(dwHashLen);
    char* buffer = m_digest.GetBufferSetLength(b64length);
    base64.Encrypt(reinterpret_cast<const unsigned char*>(pbHash),dwHashLen,reinterpret_cast<unsigned char*>(buffer));
    m_digest.ReleaseBuffer(b64length);
  }
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

XString
Crypto::Digest(const void* data,const size_t data_size,unsigned hashType)
{
  AutoCritSec lock(&m_lock);
  HCRYPTPROV hProv = NULL;

  if(!CryptAcquireContext(&hProv,NULL,NULL,PROV_RSA_AES,CRYPT_VERIFYCONTEXT))
  {
    return "";
  }

  BOOL hash_ok = FALSE;
  HCRYPTPROV hHash = NULL;
  switch(hashType)
  {
    case CALG_SHA1:   hash_ok = CryptCreateHash(hProv,CALG_SHA1,   0,0,&hHash); break;
    case CALG_MD5:    hash_ok = CryptCreateHash(hProv,CALG_MD5,    0,0,&hHash); break;
    case CALG_SHA_256:hash_ok = CryptCreateHash(hProv,CALG_SHA_256,0,0,&hHash); break;
    default:          return "";
  }

  if(!hash_ok)
  {
    CryptReleaseContext(hProv,0);
    return "";
  }

  if(!CryptHashData(hHash,static_cast<const BYTE *>(data),(DWORD)data_size,0))
  {
    CryptDestroyHash(hHash);
    CryptReleaseContext(hProv,0);
    return "";
  }

  DWORD cbHashSize = 0,dwCount = sizeof(DWORD);
  if(!CryptGetHashParam(hHash,HP_HASHSIZE,reinterpret_cast<BYTE *>(&cbHashSize),&dwCount,0))
  {
    CryptDestroyHash(hHash);
    CryptReleaseContext(hProv,0);
    return "";
  }

  std::vector<BYTE> buffer(cbHashSize);
  if(!CryptGetHashParam(hHash,HP_HASHVAL,reinterpret_cast<BYTE*>(&buffer[0]),&cbHashSize,0))
  {
    CryptDestroyHash(hHash);
    CryptReleaseContext(hProv,0);
    return "";
  }

  Base64 base;
  XString hash;
  char* pointer = hash.GetBufferSetLength((int)base.B64_length((size_t)cbHashSize + 1));
  base.Encrypt(reinterpret_cast<const unsigned char*>(&buffer[0]),cbHashSize,reinterpret_cast<unsigned char*>(pointer));
  hash.ReleaseBuffer();

  CryptDestroyHash(hHash);
  CryptReleaseContext(hProv,0);
  return hash;
}

// ENCRYPT a buffer
XString 
Crypto::Encryption(XString p_input,XString p_password)
{
  AutoCritSec lock(&m_lock);

  HCRYPTPROV hCryptProv = NULL;
  HCRYPTKEY  hCryptKey  = NULL;
  HCRYPTHASH hCryptHash = NULL;
  DWORD      dwDataLen  = 0;
  DWORD      dwBuffLen  = 0;
  BYTE*      pbData     = NULL;
  DWORD      blocklen   = 0;
  DWORD      cbBlocklen = sizeof(DWORD);
  DWORD      dwFlags    = 0;
  BOOL       bFinal     = FALSE;
  DWORD      totallen   = 0;
  BYTE*      crypting   = nullptr;
  int        b64length  = 0;
  char*      buffer     = nullptr;
  XString    result;
  Base64     base64;

  // Encrypt by way of a cryptographic provider
  if(!CryptAcquireContext(&hCryptProv,NULL,NULL,ENCRYPT_PROVIDER,CRYPT_VERIFYCONTEXT))
  {
    m_error.Format("Error acquiring encryption context: 0x%08x",GetLastError());
    goto error_exit;
  }

  if(!CryptCreateHash(hCryptProv,ENCRYPT_PASSWORD,0,0,&hCryptHash))
  {
    m_error.Format("Error creating encryption hash: 0x%08x",GetLastError());
    goto error_exit;
  }

  if(!CryptHashData(hCryptHash,reinterpret_cast<const BYTE *>(p_password.GetString()),p_password.GetLength(),0))
  {
    m_error.Format("Error hashing password for encryption: 0x%08x",GetLastError());
    goto error_exit;
  }

  if(!CryptDeriveKey(hCryptProv,ENCRYPT_ALGORITHM,hCryptHash,0,&hCryptKey))
  {
    m_error.Format("Error deriving password key for encryption: 0x%08x",GetLastError());
    goto error_exit;
  }

  if(!CryptGetKeyParam(hCryptKey,KP_BLOCKLEN,reinterpret_cast<BYTE*>(&blocklen),&cbBlocklen,0))
  {
    m_error = "Cannot get the block length of the encryption method";
    goto error_exit;
  }

  // Build data buffer that's large enough
  dwDataLen =   p_input.GetLength();
  dwBuffLen = ((p_input.GetLength() + blocklen) / blocklen) * blocklen;
  pbData = new BYTE[dwBuffLen];
  strcpy_s(reinterpret_cast<char*>(pbData),dwBuffLen,p_input.GetString());
  crypting = pbData;
  
  do
  {
    // Calculate the part to encrypt, and if we do the final block!
    DWORD dataLength = 0;
    if(dwDataLen <= blocklen)
    {
      bFinal = TRUE;
      dataLength = dwDataLen;
    }
    else
    {
      dataLength = blocklen;
    }
    
    // Do the encryption of the block
    if(CryptEncrypt(hCryptKey
                   ,NULL            // No hashing
                   ,bFinal          // Final action
                   ,dwFlags         // Reserved
                   ,crypting        // The data
                   ,&dataLength     // Pointer to data length
                   ,dwBuffLen))     // Pointer to buffer length
    {
      // Total encrypted bytes
      totallen  += dataLength;
      // This much space left in the receiving buffer
      dwBuffLen -= blocklen;
      dwDataLen -= blocklen;
      // Next pointer to data to be encrypted
      crypting  += blocklen;
    }
    else
    {
      m_error.Format("Error encrypting data: 0x%08x : %s",GetLastError(),GetLastErrorAsString().GetString());
      goto error_exit;
    }
  }
  while(bFinal == FALSE);

  // Create a base64 string of the hash data
  b64length = (int)base64.B64_length(totallen);
  buffer    = result.GetBufferSetLength(b64length);
  base64.Encrypt(reinterpret_cast<const unsigned char*>(pbData),totallen,reinterpret_cast<unsigned char*>(buffer));
  result.ReleaseBuffer(b64length);

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
XString 
Crypto::Decryption(XString p_input,XString p_password)
{
  AutoCritSec lock(&m_lock);

  HCRYPTPROV hCryptProv = NULL;
  HCRYPTKEY  hCryptKey  = NULL;
  HCRYPTHASH hCryptHash = NULL;
  DWORD      dwDataLen  = 0;
  DWORD      dataLength = 0;
  DWORD      bufferSize = 0;
  BYTE*      pbData     = NULL;
  DWORD      blocklen   = 0;
  DWORD      cbBlocklen = sizeof(DWORD);
  BOOL       bFinal     = FALSE;
  DWORD      dwFlags    = 0;
  DWORD      totallen   = 0;
  BYTE*      decrypting = nullptr;
  XString    result;
  Base64     base64;

  dwDataLen = p_input.GetLength();

  // Check if we have anything to do
  if(dwDataLen < 3)
  {
    return result;
  }

  // Decrypt by way of a cryptographic provider
  if(!CryptAcquireContext(&hCryptProv,NULL,NULL,ENCRYPT_PROVIDER,CRYPT_VERIFYCONTEXT))
  {
    m_error.Format("Crypto context not acquired: 0x%08x",GetLastError());
    goto error_exit;
  }
  if(!CryptCreateHash(hCryptProv,ENCRYPT_PASSWORD,0,0,&hCryptHash)) // MD5?
  {
    m_error.Format("Hash not created for decryption: 0x%08x",GetLastError());
    goto error_exit;
  }

  if(!CryptHashData(hCryptHash,reinterpret_cast<const BYTE *>(p_password.GetString()),p_password.GetLength(),0))
  {
    m_error.Format("Error hashing password data for decryption: 0x%08x",GetLastError());
    goto error_exit;
  }

  if(!CryptDeriveKey(hCryptProv,ENCRYPT_ALGORITHM,hCryptHash,0,&hCryptKey))
  {
    m_error.Format("Error creating derived key for decryption: 0x%08x",GetLastError());
    goto error_exit;
  }

  if(!CryptGetKeyParam(hCryptKey,KP_BLOCKLEN,reinterpret_cast<BYTE*>(&blocklen),&cbBlocklen,0))
  {
    m_error = "Cannot get the block length of the encryption method";
    goto error_exit;
  }

  // Create a data string of the base64 string
  dataLength = (DWORD)base64.Ascii_length(dwDataLen);

  // Maximum of 2 times a trailing zero at a base64 (because 64 is a multiple of 3!!)
  // You MUST take them of, otherwise decrypting will not work
  // as the block size of the algorithm is incorrect.
  if(p_input.GetAt(p_input.GetLength() - 1) == '=') --dataLength;
  if(p_input.GetAt(p_input.GetLength() - 2) == '=') --dataLength;

  bufferSize = ((dataLength + blocklen) / blocklen) * blocklen;
  pbData = new BYTE[bufferSize];
  base64.Decrypt(reinterpret_cast<const unsigned char*>(p_input.GetString()),dwDataLen,reinterpret_cast<unsigned char*>(pbData));
  decrypting = pbData;

  do 
  {
    // Calculate the part to decrypt, and if we do the final block!
    DWORD data = 0;
    if(dataLength <= blocklen)
    {
      data = dataLength;
      bFinal = TRUE;
      dwFlags = CRYPT_DECRYPT_RSA_NO_PADDING_CHECK;
    }
    else
    {
      data = blocklen;
    }

    // Decrypt
    if(CryptDecrypt(hCryptKey
                   ,NULL            // No hashing
                   ,bFinal          // Final action
                   ,dwFlags         // Reserved
                   ,decrypting      // The data
                   ,&data))         // Pointer to data length
    {
      // Totally decrypted length
      totallen   += data;
      // Work the lengths
      dataLength -= blocklen;
      // Next block of data to be decrypted
      decrypting += blocklen;
    }
    else
    {
      m_error.Format("Decrypting not done: 0x%08x : %s",GetLastError(),GetLastErrorAsString().GetString());
      goto error_exit;
    }
  }
  while(bFinal == FALSE);

  // Getting our result
  pbData[totallen] = 0;
  result = pbData;

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


XString
Crypto::FastEncryption(XString p_input, XString password)
{
  AutoCritSec lock(&m_lock);
  BCRYPT_ALG_HANDLE hAlgorithm;
  XString result;

  NTSTATUS res = BCryptOpenAlgorithmProvider(&hAlgorithm, L"RC4", NULL, /*BCRYPT_HASH_REUSABLE_FLAG*/ 0); 
  if(res != STATUS_SUCCESS)
  {
    return result;
  }

  BCRYPT_KEY_HANDLE hKey;
  if(BCryptGenerateSymmetricKey(hAlgorithm, &hKey, NULL, NULL,(PUCHAR)password.GetString(),password.GetLength(),NULL) != STATUS_SUCCESS)
  {
    return result;
  }

  ULONG cbCipherText;
  if(BCryptEncrypt(hKey, (PUCHAR)p_input.GetString(), p_input.GetLength(), NULL, NULL, NULL, NULL, NULL, &cbCipherText, NULL) != STATUS_SUCCESS)
  {
    return result;
  }
    
  XString data;
  char* datapointer = data.GetBufferSetLength(cbCipherText);
  if (BCryptEncrypt(hKey,(PUCHAR)p_input.GetString(), p_input.GetLength(), NULL, NULL, NULL, reinterpret_cast<PUCHAR>(datapointer), cbCipherText, &cbCipherText, NULL) != STATUS_SUCCESS)
  {
    return result;
  }
  data.ReleaseBufferSetLength(cbCipherText);


  BCryptDestroyKey(hKey);
  BCryptCloseAlgorithmProvider(hAlgorithm, 0);

  // Create a base64 string of the hash data
  Base64 base64;
  int b64length = (int)base64.B64_length(cbCipherText);
  char* buffer = result.GetBufferSetLength(b64length);
  base64.Encrypt(reinterpret_cast<const unsigned char*>(data.GetString()),cbCipherText,reinterpret_cast<unsigned char*>(buffer));
  result.ReleaseBuffer(b64length);

  return result;
}

XString
Crypto::FastDecryption(XString p_input,XString password)
{
  AutoCritSec lock(&m_lock);

  XString result;
  char* data = nullptr;
  DWORD dwDataLen  = p_input.GetLength();
  BCRYPT_ALG_HANDLE hAlgorithm = NULL;
  BCRYPT_KEY_HANDLE hKey = NULL;
  ULONG cbCipherText = 0;

  // Check if we have anything to do
  if(dwDataLen < 3)
  {
    return result;
  }

  // Create a data string of the base64 string
  Base64 base64;
  DWORD dataLength = (DWORD)base64.Ascii_length(dwDataLen);
  BYTE* pbData = new BYTE[(size_t)dataLength + 2];
  base64.Decrypt(reinterpret_cast<const unsigned char*>(p_input.GetString()),dwDataLen,reinterpret_cast<unsigned char*>(pbData));


  if(BCryptOpenAlgorithmProvider(&hAlgorithm, L"RC4", NULL, 0/*BCRYPT_HASH_REUSABLE_FLAG*/) != STATUS_SUCCESS)
  {
    goto error_exit;
  }

  if(BCryptGenerateSymmetricKey(hAlgorithm, &hKey, NULL, NULL,(PUCHAR)password.GetString(), password.GetLength(), NULL) != STATUS_SUCCESS)
  {
    goto error_exit;
  }

  if(BCryptDecrypt(hKey,reinterpret_cast<PUCHAR>(pbData), dataLength, NULL, NULL, NULL, NULL, NULL, &cbCipherText, NULL) != STATUS_SUCCESS)
  {
    goto error_exit;
  }

  data = result.GetBufferSetLength(cbCipherText);
  if(BCryptDecrypt(hKey,reinterpret_cast<PUCHAR>(pbData),dataLength,NULL,NULL,NULL,reinterpret_cast<PUCHAR>(data),cbCipherText,&cbCipherText,NULL) != STATUS_SUCCESS)
  {
    goto error_exit;
  }
  result.ReleaseBufferSetLength(cbCipherText);

error_exit:
  if(hKey)
  {
    BCryptDestroyKey(hKey);
  }
  if(hAlgorithm)
  {
    BCryptCloseAlgorithmProvider(hAlgorithm,0);
  }
  delete[] pbData;

  return result;
}

// Set hashing digest method
unsigned 
Crypto::SetHashMethod(XString p_method)
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

XString  
Crypto::GetHashMethod(unsigned p_hash)
{
  switch(p_hash)
  {
    // Standard ds-sign
    default:            [[fallthrough]];
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

XString
Crypto::GetCipherType(unsigned p_type)
{
  XString cipher;
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

XString  
Crypto::GetKeyExchange(unsigned p_type)
{
  XString exchange;

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

XString  
Crypto::GetSSLProtocol(unsigned p_type)
{
  XString protocol;

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
