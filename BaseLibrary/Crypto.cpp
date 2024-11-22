/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: Crypto.cpp
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
#include "pch.h"
#include "Crypto.h"
#include "Base64.h"
#include "GetLastErrorAsString.h"
#include "AutoCritical.h"
#include <ConvertWideString.h>
#include <wincrypt.h>
#include <bcrypt.h>
#include <schannel.h>
#include <vector>

#ifdef _AFX
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
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

XString
Crypto::Digest(const void* data,const size_t data_size,unsigned hashType /*=0*/)
{
  AutoCritSec lock(&m_lock);
  HCRYPTPROV hProv = NULL;

  // Do we have input?
  if(data_size == 0 || data == nullptr)
  {
    return _T("");
  }

  // Get last modern encryption provider
  if(!CryptAcquireContext(&hProv,NULL,MS_ENH_RSA_AES_PROV,PROV_RSA_AES,CRYPT_VERIFYCONTEXT|CRYPT_MACHINE_KEYSET))
  {
    return _T("");
  }

  BOOL hash_ok = FALSE;
  HCRYPTPROV hHash = NULL;
  unsigned type = hashType > 0 ? hashType : m_hashMethod;
  switch(type)
  {
    case CALG_SHA1:   hash_ok = CryptCreateHash(hProv,CALG_SHA1,   0,0,&hHash); break;
    case CALG_MD2:    hash_ok = CryptCreateHash(hProv,CALG_MD2,    0,0,&hHash); break;
    case CALG_MD4:    hash_ok = CryptCreateHash(hProv,CALG_MD4,    0,0,&hHash); break;
    case CALG_MD5:    hash_ok = CryptCreateHash(hProv,CALG_MD5,    0,0,&hHash); break;
    case CALG_SHA_256:hash_ok = CryptCreateHash(hProv,CALG_SHA_256,0,0,&hHash); break;
    case CALG_SHA_384:hash_ok = CryptCreateHash(hProv,CALG_SHA_384,0,0,&hHash); break;
    case CALG_SHA_512:hash_ok = CryptCreateHash(hProv,CALG_SHA_512,0,0,&hHash); break;
    default:          return _T("");
  }

  if(!hash_ok)
  {
    CryptReleaseContext(hProv,0);
    return _T("");
  }

  if(!CryptHashData(hHash,static_cast<const BYTE *>(data),(DWORD)data_size,0))
  {
    CryptDestroyHash(hHash);
    CryptReleaseContext(hProv,0);
    return _T("");
  }

  DWORD cbHashSize = 0;
  DWORD dwCount = sizeof(DWORD);
  if(!CryptGetHashParam(hHash,HP_HASHSIZE,reinterpret_cast<BYTE *>(&cbHashSize),&dwCount,0))
  {
    CryptDestroyHash(hHash);
    CryptReleaseContext(hProv,0);
    return _T("");
  }

  BYTE* buffer = new BYTE[cbHashSize + 2];
  if(!CryptGetHashParam(hHash,HP_HASHVAL,buffer,&cbHashSize,0))
  {
    CryptDestroyHash(hHash);
    CryptReleaseContext(hProv,0);
    delete[] buffer;
    return _T("");
  }
  Base64 base64(m_base64 ? CRYPT_STRING_BASE64 : CRYPT_STRING_HEXRAW);
  XString hash = base64.Encrypt(buffer,cbHashSize);

  CryptDestroyHash(hHash);
  CryptReleaseContext(hProv,0);
  delete[] buffer;

  return hash;
}

// ENCRYPT a buffer

#ifdef _UNICODE
XString
Crypto::Encryption(XString p_input,XString p_password)
{
  XString encoded;
  int   lengthINP = 0;
  BYTE* bufferINP = nullptr;
  int   lengthPWD = 0;
  BYTE* bufferPWD = nullptr;

  if(TryCreateNarrowString(p_input,   _T("utf-8"),false,&bufferINP,lengthINP) &&
     TryCreateNarrowString(p_password,_T("utf-8"),false,&bufferPWD,lengthPWD))
  {
    encoded = ImplementEncryption(bufferINP,lengthINP,bufferPWD,lengthPWD);
  }
  delete [] bufferINP;
  delete [] bufferPWD;
  return encoded;
}
#else
XString
Crypto::Encryption(XString p_input,XString p_password)
{
  XString inputUTF8 = EncodeStringForTheWire(p_input);
  XString passwUTF8 = EncodeStringForTheWire(p_password);

  return ImplementEncryption((const BYTE*)inputUTF8.GetString(),inputUTF8.GetLength()
                            ,(const BYTE*)passwUTF8.GetString(),passwUTF8.GetLength());
}
#endif

// Implementation of the ENCRYPT of a buffer
XString
Crypto::ImplementEncryption(const BYTE* p_input,int p_lengthINP,const BYTE* p_password,int p_lengthPWD)
{
  AutoCritSec lock(&m_lock);

  HCRYPTPROV hCryptProv = NULL;
  HCRYPTKEY  hCryptKey  = NULL;
  HCRYPTHASH hCryptHash = NULL;
  DWORD      dwDataLen  = 0;
  BYTE*      pbData     = NULL;
  BYTE*      pbConvert  = nullptr;
  DWORD      blocklen   = 0;
  DWORD      cbBlocklen = sizeof(DWORD);
  DWORD      dwFlags    = 0;
  BOOL       bFinal     = FALSE;
  DWORD      totallen   = 0;
  BYTE*      crypting   = nullptr;
  XString    result;
  Base64     base64;

  // Do we have input?
  if(p_lengthINP == 0 || p_lengthPWD == 0)
  {
    return _T("");
  }

  // Get last modern encryption provider
  if(!CryptAcquireContext(&hCryptProv,NULL,MS_ENH_RSA_AES_PROV,PROV_RSA_AES,CRYPT_VERIFYCONTEXT | CRYPT_MACHINE_KEYSET))
  {
    m_error.Format(_T("Error acquiring encryption context: 0x%08x"),GetLastError());
    goto error_exit;
  }

  if(!CryptCreateHash(hCryptProv,ENCRYPT_PASSWORD,0,0,&hCryptHash))
  {
    m_error.Format(_T("Error creating encryption hash: 0x%08x"),GetLastError());
    goto error_exit;
  }

  if(!CryptHashData(hCryptHash,p_password,p_lengthPWD,0))
  {
    m_error.Format(_T("Error hashing password for encryption: 0x%08x"),GetLastError());
    goto error_exit;
  }

  if(!CryptDeriveKey(hCryptProv,ENCRYPT_ALGORITHM,hCryptHash,0,&hCryptKey))
  {
    m_error.Format(_T("Error deriving password key for encryption: 0x%08x"),GetLastError());
    goto error_exit;
  }

  if(!CryptGetKeyParam(hCryptKey,KP_BLOCKLEN,reinterpret_cast<BYTE*>(&blocklen),&cbBlocklen,0))
  {
    m_error = _T("Cannot get the block length of the encryption method");
    goto error_exit;
  }

  // Build data buffer that's large enough
  dwDataLen = (DWORD) strlen((const char*)p_input);
  crypting  = (BYTE*) p_input;
  pbData    = (BYTE*) malloc(2 * blocklen);
  pbConvert = pbData;
  
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
    // Copy a data block to be encrypted
    memcpy(pbConvert,crypting,dataLength);

// #ifdef _DEBUG
//     for(DWORD ind = 0; ind < dataLength; ++ind)
//     {
//       TRACE("RAW %X %c\n",pbConvert[ind],pbConvert[ind]);
//     }
// #endif

    DWORD processed = dataLength;

    // Do the encryption of the block
    if(CryptEncrypt(hCryptKey
                   ,NULL            // No hashing
                   ,bFinal          // Final action
                   ,dwFlags         // Reserved
                   ,pbConvert       // The data
                   ,&processed      // Pointer to data length
                   ,2 * blocklen))  // Pointer to buffer length
    {
      // Total encrypted bytes
      totallen  += processed;
      // This much space left in the receiving buffer
      dwDataLen -= blocklen;
      // Next pointer to data to be encrypted
      crypting  += blocklen;

      // Add to the result;
      pbData[totallen] = 0;
      
// #ifdef _DEBUG
//       for(DWORD ind = 0; ind < dataLength;++ind)
//       {
//         TRACE("ENCODED %X\n",pbConvert[ind]);
//       }
// #endif
      if(!bFinal)
      {
        // Make room for the next block and continue after totallen
        pbData    = (BYTE*) realloc(pbData,totallen + (2 * blocklen));
        pbConvert = pbData + totallen;
      }
    }
    else
    {
      m_error.Format(_T("Error encrypting data: 0x%08x : %s"),GetLastError(),GetLastErrorAsString().GetString());
      goto error_exit;
    }
  }
  while(bFinal == FALSE);

  // Create a base64 string of the hash data
  result = base64.Encrypt(pbData,totallen);

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
    free(pbData);
  }
  if(hCryptProv)
  {
    CryptReleaseContext(hCryptProv,0);
  }
  return result;
}

// DECRYPT a buffer

#ifdef _UNICODE
XString
Crypto::Decryption(XString p_input,XString p_password)
{
  XString decoded;
  int   lengthINP = 0;
  BYTE* bufferINP = nullptr;
  int   lengthPWD = 0;
  BYTE* bufferPWD = nullptr;

  if(TryCreateNarrowString(p_input,   _T("utf-8"),false,&bufferINP,lengthINP) &&
     TryCreateNarrowString(p_password,_T("utf-8"),false,&bufferPWD,lengthPWD))
  {
    bool bom(false);
    CStringA result = ImplementDecryption(bufferINP,lengthINP,bufferPWD,lengthPWD);
    if(!TryConvertNarrowString((const BYTE*)result.GetString(),result.GetLength(),_T("utf-8"),decoded,bom))
    {
      decoded.Empty();
    }
  }
  delete [] bufferPWD;
  delete [] bufferINP;
  return decoded;
}
#else
XString
Crypto::Decryption(XString p_input, XString p_password)
{
  XString inputUTF8 = EncodeStringForTheWire(p_input);
  XString passwUTF8 = EncodeStringForTheWire(p_password);

  return ImplementDecryption((const BYTE*)inputUTF8.GetString(),inputUTF8.GetLength(),
                             (const BYTE*)passwUTF8.GetString(),passwUTF8.GetLength());
}
#endif

// Implementation of the DECRYPT of a buffer
CStringA
Crypto::ImplementDecryption(const BYTE* p_input,int p_lengthINP,const BYTE* p_password,int p_lengthPWD)
{
  AutoCritSec lock(&m_lock);

  HCRYPTPROV hCryptProv = NULL;
  HCRYPTKEY  hCryptKey  = NULL;
  HCRYPTHASH hCryptHash = NULL;
  DWORD      dwDataLen  = 0;
  DWORD      dataLength = 0;
  BYTE*      pbData     = NULL;
  BYTE*      pbEncrypt  = nullptr;
  DWORD      blocklen   = 0;
  DWORD      cbBlocklen = sizeof(DWORD);
  BOOL       bFinal     = FALSE;
  DWORD      dwFlags    = 0;
  DWORD      totallen   = 0;
  BYTE*      decrypting = nullptr;
  CStringA   result;
  Base64     base64;

  dwDataLen = p_lengthINP;

  // Check if we have anything to do
  if(dwDataLen < 3)
  {
    return result;
  }

  // Decrypt by way of a cryptographic provider
  if(!CryptAcquireContext(&hCryptProv,NULL,MS_ENH_RSA_AES_PROV,PROV_RSA_AES,CRYPT_VERIFYCONTEXT | CRYPT_MACHINE_KEYSET))
  {
    m_error.Format(_T("Crypto context not acquired: 0x%08x"),GetLastError());
    goto error_exit;
  }
  if(!CryptCreateHash(hCryptProv,ENCRYPT_PASSWORD,0,0,&hCryptHash)) // MD5?
  {
    m_error.Format(_T("Hash not created for decryption: 0x%08x"),GetLastError());
    goto error_exit;
  }

  if(!CryptHashData(hCryptHash,p_password,p_lengthPWD,0))
  {
    m_error.Format(_T("Error hashing password data for decryption: 0x%08x"),GetLastError());
    goto error_exit;
  }

  if(!CryptDeriveKey(hCryptProv,ENCRYPT_ALGORITHM,hCryptHash,0,&hCryptKey))
  {
    m_error.Format(_T("Error creating derived key for decryption: 0x%08x"),GetLastError());
    goto error_exit;
  }

  if(!CryptGetKeyParam(hCryptKey,KP_BLOCKLEN,reinterpret_cast<BYTE*>(&blocklen),&cbBlocklen,0))
  {
    m_error = _T("Cannot get the block length of the encryption method");
    goto error_exit;
  }

  // Maximum of 2 times a trailing zero at a base64 (because 64 is a multiple of 3!!)
  // You MUST take them of, otherwise decrypting will not work
  // as the block size of the algorithm is incorrect.
  dataLength = (DWORD) base64.Ascii_length(dwDataLen);
  if(p_input[p_lengthINP - 1] == '=') --dataLength;
  if(p_input[p_lengthINP - 2] == '=') --dataLength;

  // Create a data string of the base64 string
  pbEncrypt  = new BYTE[dataLength + 2];
  base64.Decrypt(const_cast<BYTE*>(p_input),p_lengthINP,pbEncrypt,(int)(dataLength + 2));
  decrypting = pbEncrypt;
  // Buffer to do the conversion in
  pbData     = new BYTE[blocklen + 2];

  do 
  {
    // Calculate the part to decrypt, and if we do the final block!
    DWORD dataLen = 0;
    if(dataLength <= blocklen)
    {
      dataLen = (DWORD)dataLength;
      bFinal  = TRUE;
      dwFlags = 0;  // CRYPT_DECRYPT_RSA_NO_PADDING_CHECK;
    }
    else
    {
      dataLen = blocklen;
    }

    // Copy a block to be decrypted
    memcpy(pbData,decrypting,dataLen);

// #ifdef _DEBUG
//     for(DWORD ind = 0; ind < dataLen; ++ind)
//     {
//       TRACE("RAW %X\n",pbData[ind]);
//     }
// #endif
    DWORD processed = dataLen;

    // Decrypt
    if(CryptDecrypt(hCryptKey
                   ,NULL            // No hashing
                   ,bFinal          // Final action
                   ,dwFlags         // Reserved
                   ,pbData          // The data
                   ,&processed))    // Pointer to data length
    {
      // Totally decrypted length
      totallen   += processed;
      // Work the lengths
      dataLength -= dataLen;

      // Keep the result
      pbData[processed    ] = 0;
      pbData[processed + 1] = 0;
      result += (char*) pbData;

// #ifdef _DEBUG
//       for(DWORD ind = 0; ind < dataLen;++ind)
//       {
//         TRACE("DECODED %X %c\n",pbData[ind],pbData[ind]);
//       }
// #endif
      if(!bFinal)
      {
        // Next block of data to be decrypted
        decrypting += blocklen;
      }
    }
    else
    {
      m_error.Format(_T("Decrypting not done: 0x%08x : %s"),GetLastError(),GetLastErrorAsString().GetString());
      goto error_exit;
    }
  }
  while(bFinal == FALSE);

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
    delete[] pbData;
  }
  if(pbEncrypt)
  {
    delete[] pbEncrypt;
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
  BCRYPT_ALG_HANDLE hAlgorithm = NULL;
  XString result;

  // Do we have input?
  if(p_input.IsEmpty() || password.IsEmpty())
  {
    return result;
  }

  // Symmetric encryption 
  NTSTATUS res = BCryptOpenAlgorithmProvider(&hAlgorithm,L"RC4",NULL,0); 
  if(res != STATUS_SUCCESS)
  {
    return result;
  }

  BCRYPT_KEY_HANDLE hKey = NULL;
  if(BCryptGenerateSymmetricKey(hAlgorithm, &hKey, NULL, NULL,(PUCHAR)password.GetString(),password.GetLength() * sizeof(TCHAR),NULL) != STATUS_SUCCESS)
  {
    return result;
  }

  ULONG cbCipherText = 0;
  if(BCryptEncrypt(hKey, (PUCHAR)p_input.GetString(), p_input.GetLength() * sizeof(TCHAR),NULL,NULL,NULL,NULL,NULL,&cbCipherText,NULL) != STATUS_SUCCESS)
  {
    return result;
  }
    
  
  PUCHAR datapointer = new UCHAR[cbCipherText + 2];
  if (BCryptEncrypt(hKey,(PUCHAR)p_input.GetString(), p_input.GetLength() * sizeof(TCHAR),NULL,NULL,NULL,reinterpret_cast<PUCHAR>(datapointer),cbCipherText,&cbCipherText,NULL) != STATUS_SUCCESS)
  {
    return result;
  }

  BCryptDestroyKey(hKey);
  BCryptCloseAlgorithmProvider(hAlgorithm, 0);

  // Create a base64 string of the hash data
  Base64 base64;
  result = base64.Encrypt(datapointer,cbCipherText);

  delete[] datapointer;
  return result;
}

XString
Crypto::FastDecryption(XString p_input,XString password)
{
  AutoCritSec lock(&m_lock);

  XString result;
  PTCHAR data = nullptr;
  DWORD dwDataLen  = p_input.GetLength() * sizeof(TCHAR);
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
  DWORD dataLength = (DWORD)base64.Ascii_length(p_input.GetLength());
  BYTE* pbData = new BYTE[(size_t)dataLength + 2];
  base64.Decrypt(p_input,pbData,dataLength + 2);

  // Symmetric encryption
  if(BCryptOpenAlgorithmProvider(&hAlgorithm, L"RC4",NULL,0) != STATUS_SUCCESS)
  {
    goto error_exit;
  }

  if(BCryptGenerateSymmetricKey(hAlgorithm, &hKey, NULL, NULL,(PUCHAR)password.GetString(), password.GetLength() * sizeof(TCHAR),NULL) != STATUS_SUCCESS)
  {
    goto error_exit;
  }

  if(BCryptDecrypt(hKey,reinterpret_cast<PUCHAR>(pbData), dataLength, NULL, NULL, NULL, NULL, NULL, &cbCipherText, NULL) != STATUS_SUCCESS)
  {
    goto error_exit;
  }

  data = result.GetBufferSetLength(dataLength / sizeof(TCHAR) + 1);
  if(BCryptDecrypt(hKey,reinterpret_cast<PUCHAR>(pbData),dataLength,NULL,NULL,NULL,reinterpret_cast<PUCHAR>(data),cbCipherText,&cbCipherText,NULL) != STATUS_SUCCESS)
  {
    goto error_exit;
  }
  cbCipherText /= sizeof(TCHAR);
  data[cbCipherText    ] = 0;
  data[cbCipherText + 1] = 0;
  result.ReleaseBuffer();

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
  if(p_method.Compare(_T("sha1"))      == 0) m_hashMethod = CALG_SHA1;
  // Older methods (Not in the standard, but exists on the MS-Windows system)
  if(p_method.Compare(_T("md2")) == 0) m_hashMethod = CALG_MD2;
  if(p_method.Compare(_T("md4")) == 0) m_hashMethod = CALG_MD4;
  if(p_method.Compare(_T("md5")) == 0) m_hashMethod = CALG_MD5;
  // Newer (safer) methods.
  if(p_method.Compare(_T("sha2-256")) == 0) m_hashMethod = CALG_SHA_256;
  if(p_method.Compare(_T("sha2-384")) == 0) m_hashMethod = CALG_SHA_384;
  if(p_method.Compare(_T("sha2-512")) == 0) m_hashMethod = CALG_SHA_512;

  return m_hashMethod;
}

void
Crypto::SetHashMethod(unsigned p_hash)
{
  if(p_hash == CALG_SHA         ||
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
    // Standard hashing
    default:            [[fallthrough]];
    case CALG_SHA1:     return _T("sha1");
    // older
    case CALG_MD2:      return _T("md2");
    case CALG_MD4:      return _T("md4");
    case CALG_MD5:      return _T("md5");
    // Newer
    case CALG_SHA_256:  return _T("sha2-256");
    case CALG_SHA_384:  return _T("sha2-384");
    case CALG_SHA_512:  return _T("sha2-512");
    // No hash
    case 0:             return _T("no-hash");
  }
}

XString
Crypto::GetCipherType(unsigned p_type)
{
  XString cipher;
  switch(p_type)
  {
    case CALG_3DES:       cipher = _T("3DES Block");   break;
    case CALG_3DES_112:   cipher = _T("DES 112 Bits"); break;
    case CALG_AES_128:    cipher = _T("AES 128 Bits"); break;
    case CALG_AES_192:    cipher = _T("AES 192 Bits"); break;
    case CALG_AES_256:    cipher = _T("AES 256 Bits"); break;
    case CALG_AES:        cipher = _T("AES Block");    break;
    case CALG_DES:        cipher = _T("DES Block");    break;
    case CALG_DESX:       cipher = _T("DESX Block");   break;
    case CALG_RC2:        cipher = _T("RC2 Block");    break;
    case CALG_RC4:        cipher = _T("RC4 Block");    break;
    case CALG_RC5:        cipher = _T("RC5 Block");    break;
    case CALG_SEAL:       cipher = _T("SEAL");         break;
    case CALG_SKIPJACK:   cipher = _T("SkipJack");     break;
    case CALG_TEK:        cipher = _T("TEK");          break;
    case CALG_CYLINK_MEK: cipher = _T("Cylink");       break;
    case 0:               cipher = _T("No encryption");break;
    default:              cipher = _T("AES");          break;
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
    case SP_PROT_TLS1_CLIENT:  protocol = _T("TLS 1 Client");     break;
    case SP_PROT_TLS1_SERVER:  protocol = _T("TLS 1 Server");     break;
    case SP_PROT_SSL3_CLIENT:  protocol = _T("SSL 3 Client");     break;
    case SP_PROT_SSL3_SERVER:  protocol = _T("SSL 3 Server");     break;
    case SP_PROT_TLS1_1_CLIENT:protocol = _T("TLS 1.1 Client");   break;
    case SP_PROT_TLS1_1_SERVER:protocol = _T("TLS 1.1 Server");   break;
    case SP_PROT_TLS1_2_CLIENT:protocol = _T("TLS 1.2 Client");   break;
    case SP_PROT_TLS1_2_SERVER:protocol = _T("TLS 1.2 Server");   break;
    case SP_PROT_PCT1_CLIENT:  protocol = _T("Private Communications 1 Client"); break;
    case SP_PROT_PCT1_SERVER:  protocol = _T("Private Communications 1 Server"); break;
    case SP_PROT_SSL2_CLIENT:  protocol = _T("SSL 2 Client");     break;
    case SP_PROT_SSL2_SERVER:  protocol = _T("SSL 2 Server");     break;
    case SP_PROT_DTLS_CLIENT:  protocol = _T("DTLS Client");      break;
    case SP_PROT_DTLS_SERVER:  protocol = _T("DTLS Server");      break;
    default:                   protocol = _T("Combined SSL/TLS"); break;
  }
  return protocol;
}
