/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: HTTPCertificate.cpp
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
#include "stdafx.h"
#include "HTTPCertificate.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

HTTPCertificate::HTTPCertificate()
{
}

HTTPCertificate::HTTPCertificate(PUCHAR p_blob,ULONG p_size)
                :m_blob(p_blob)
                ,m_size(p_size)
{
}

HTTPCertificate::~HTTPCertificate()
{
  if(m_context)
  {
    // Free the context again
    CertFreeCertificateContext(m_context);
    m_context = nullptr;
  }
}

bool
HTTPCertificate::OpenContext()
{
  if(m_context == nullptr)
  {
    if(m_blob && m_size)
    {
      m_context = CertCreateCertificateContext(PKCS_7_ASN_ENCODING | X509_ASN_ENCODING,m_blob,m_size);
    }
  }
  return (m_context != nullptr);
}

void
HTTPCertificate::SetRawCertificate(PUCHAR p_blob,ULONG p_size)
{
  if(m_context == nullptr && m_blob == nullptr && m_size == 0L)
  {
    m_blob = p_blob;
    m_size = p_size;
  }
}

CString
HTTPCertificate::GetSubject()
{
  if(m_subject.IsEmpty())
  {
    if(!m_context)
    {
      if(m_blob && m_size)
      {
        OpenContext();
      }
    }
    if(m_context)
    {
      if(m_context->pCertInfo)
      {
        DWORD len = m_context->pCertInfo->Subject.cbData;
        BYTE* ptr = m_context->pCertInfo->Subject.pbData;

        // Copy subject name from certificate blob
        m_subject = (char*)ptr;

        char* str = m_subject.GetBufferSetLength(len + 1);
        strncpy_s(str,len + 1,(char*)ptr,len);
        str[len] = 0;
        m_subject.ReleaseBuffer(len);
        m_subject = CleanupCertificateString(m_subject);
      }
    }
  }
  return m_subject;
}

CString
HTTPCertificate::GetIssuer()
{
  if(m_issuer.IsEmpty())
  {
    if(!m_context)
    {
      if(m_blob && m_size)
      {
        OpenContext();
      }
    }
    if(m_context)
    {
      if(m_context->pCertInfo)
      {
        DWORD len = m_context->pCertInfo->Issuer.cbData;
        BYTE* ptr = m_context->pCertInfo->Issuer.pbData;

        // Copy subject name from certificate blob
        char* str = m_issuer.GetBufferSetLength(len + 1);
        strncpy_s(str,len + 1,(char*)ptr,len);
        str[len] = 0;
        m_issuer.ReleaseBuffer(len);
        m_issuer = CleanupCertificateString(m_issuer);
      }
    }
  }
  return m_issuer;
}

// Finding a thumbprint in a certificate
bool 
HTTPCertificate::VerifyThumbprint(CString p_thumbprint)
{
  bool result = false;

  // Buffers on the stack for the thumbprints
  BYTE buffer1[THUMBPRINT_RAW_SIZE];
  BYTE buffer2[THUMBPRINT_RAW_SIZE];
  CRYPT_HASH_BLOB blob1;
  CRYPT_HASH_BLOB blob2;
  blob1.cbData = 0;
  blob1.pbData = buffer1;
  blob2.cbData = THUMBPRINT_RAW_SIZE;
  blob2.pbData = buffer2;

  // Encode thumbprint in blob buffer1
  EncodeThumbprint(p_thumbprint,&blob1,THUMBPRINT_RAW_SIZE);
  m_thumbprint = p_thumbprint;

  if(!m_context)
  {
    OpenContext();
  }
  if(m_context)
  {
    // Encode ourselves in blob buffer2
    if(CryptHashCertificate(0
                           ,CALG_SHA1
                           ,0
                           ,m_context->pbCertEncoded
                           ,m_context->cbCertEncoded
                           , blob2.pbData
                           ,&blob2.cbData))
    {
      // See if the buffers have the same size AND contain the same data
      if(memcmp(buffer1,buffer2,blob1.cbData) == 0 && 
                blob1.cbData == blob2.cbData)
      {
        // Match found of the two thumbprints
        result = true;
      }
    }
  }
  return result;
}

// Encoding a thumbprint string 
// Preparing it to search in the certificate stores
// Accepts two kinds of thumbprints
// With    spaces: "db 34 40 64 f2 fc 13 18 dd 90 f5 07 fe 78 e8 1b 03 16 00"
// Without spaces: "db344064f2fc1318dd90f507fe78e81b031600"
// P_blob must point to a blob that is sufficiently large (20 bytes)
bool 
HTTPCertificate::EncodeThumbprint(CString& p_thumbprint,PCRYPT_HASH_BLOB p_blob,DWORD p_len)
{
  // Removing
  p_thumbprint.Replace(" ","");

  BYTE* bpointer = p_blob->pbData;

  for(int ind = 0;ind < p_thumbprint.GetLength();)
  {
    int c1 = toupper(p_thumbprint.GetAt(ind++));
    int c2 = toupper(p_thumbprint.GetAt(ind++));

    c1 = (c1 <= '9') ? c1 = c1 - '0' : c1 - 'A' + 10;
    c2 = (c2 <= '9') ? c2 = c2 - '0' : c2 - 'A' + 10;
    *bpointer++ = static_cast<BYTE>((c1 << 4) + c2);

    p_blob->cbData = ind / 2;

    if(p_blob->cbData == p_len)
    {
      return false;
    }
  }
  return true;
}

CString 
HTTPCertificate::CleanupCertificateString(CString p_name)
{
  int pos = p_name.GetLength();
  while(pos-- > 0)
  {
    if(p_name.GetAt(pos) < 32)
    {
      break;
    }
  }
  return p_name.Mid(pos + 1);
}
