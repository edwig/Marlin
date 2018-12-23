//////////////////////////////////////////////////////////////////////////
//
// USER-SPACE IMPLEMENTTION OF HTTP.SYS
//
// 2018 (c) ir. W.E. Huisman
// License: MIT
//
//////////////////////////////////////////////////////////////////////////

#pragma once
#include <wincrypt.h>


class CertificateInfo
{
public:
  CertificateInfo();
 ~CertificateInfo();

 // Add a certificate blob from the certificate context
 bool    AddCertificateByContext(PCERT_CONTEXT p_context);
 // Getting the raw certificate data
 void    GetCertificateData(PUCHAR& p_blob,ULONG& p_size);
 // Copy Certificate info to blob space
 ULONG   GetSSLCertificateInfoBlob(PHTTP_SSL_CLIENT_CERT_INFO p_sslClientCertInfo,ULONG p_size,PULONG p_bytes);

private:
  void   Reset();

  // DATA
  HTTP_SSL_CLIENT_CERT_INFO m_info;
  // The actual data for the certificate
  ULONG    m_size { 0       };
  PUCHAR   m_blob { nullptr };
};
