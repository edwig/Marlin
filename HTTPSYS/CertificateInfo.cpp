//////////////////////////////////////////////////////////////////////////
//
// USER-SPACE IMPLEMENTTION OF HTTP.SYS
//
// 2018 (c) ir. W.E. Huisman
// License: MIT
//
//////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "http_private.h"
#include "CertificateInfo.h"

CertificateInfo::CertificateInfo()
{
  Reset();
}

CertificateInfo::~CertificateInfo()
{
  Reset();
}

// Add a certificate blob from the certificate context
bool    
CertificateInfo::AddCertificateByContext(PCERT_CONTEXT p_context)
{
  Reset();

  // Copy the raw certificate blob
  m_size = p_context->cbCertEncoded;
  m_blob = (PUCHAR) malloc(m_size);
  memcpy_s(m_blob,m_size,p_context->pbCertEncoded,m_size);

  // Set in the info structure
  m_info.CertEncodedSize = m_size;
  m_info.pCertEncoded    = m_blob;

  return true;
}

// Getting the raw certificate data
void    
CertificateInfo::GetCertificateData(PUCHAR& p_blob,ULONG& p_size)
{
  p_blob = m_blob;
  p_size = m_size;
}

// Copy Certificate info to blob space
// return size -> More space needed
// return 0    -> Copy succeeded
ULONG   
CertificateInfo::GetSSLCertificateInfoBlob(PHTTP_SSL_CLIENT_CERT_INFO p_sslClientCertInfo,ULONG p_size,PULONG p_bytes)
{
  // Check how much buffer we got
  ULONG needed = sizeof(HTTP_SSL_CLIENT_CERT_INFO) + m_size;
  if(p_size < needed)
  {
    p_sslClientCertInfo->CertEncodedSize = m_size;
    *p_bytes = needed;
    return ERROR_MORE_DATA;
  }

  // OK, We are in business
  ZeroMemory(p_sslClientCertInfo, p_size);
  PUCHAR blob = (PUCHAR)((ULONGLONG)p_sslClientCertInfo + sizeof(HTTP_SSL_CLIENT_CERT_INFO));
  memcpy_s(blob,m_size,m_blob,m_size);
  p_sslClientCertInfo->CertEncodedSize = m_size;
  p_sslClientCertInfo->pCertEncoded    = blob;

  // OK
  *p_bytes = m_size + sizeof(HTTP_SSL_CLIENT_CERT_INFO);
  return NO_ERROR;
}

//////////////////////////////////////////////////////////////////////////
//
// PRIVATE
//
//////////////////////////////////////////////////////////////////////////

void
CertificateInfo::Reset()
{
  ZeroMemory(&m_info, sizeof(HTTP_SSL_CLIENT_CERT_INFO));
  if(m_blob)
  {
    free(m_blob);
    m_blob = nullptr;
  }
}
