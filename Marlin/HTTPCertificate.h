/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: HTTPCertificate.h
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
#include <wincrypt.h>

// CALG_SHA1 THUMBPRINTS are below 20 bytes
#define THUMBPRINT_RAW_SIZE 40

class HTTPCertificate
{
public:
  HTTPCertificate();
  HTTPCertificate(PUCHAR p_blob,ULONG p_size);
 ~HTTPCertificate();

  XString GetSubject();
  XString GetIssuer();
  bool    VerifyThumbprint(XString p_thumbprint);
  void    SetRawCertificate(PUCHAR p_blob,ULONG p_size);
  // Encoding a thumbprint string
  // Preparing it to search in the certificate stores
  // Or for verification purposes.
  static  bool EncodeThumbprint(XString& p_thumbprint,PCRYPT_HASH_BLOB p_blob,DWORD p_len);

private:
  bool    OpenContext();
  XString CleanupCertificateString(XString p_name);

  PUCHAR         m_blob    { nullptr };
  ULONG          m_size    { 0L      };
  PCCERT_CONTEXT m_context { nullptr };
  XString        m_subject;
  XString        m_issuer;
  XString        m_thumbprint;
};
