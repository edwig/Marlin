/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: SiteFilterClientCertificate.cpp
//
// Marlin Server: Internet server/client
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
#include "stdafx.h"
#include "SiteFilterClientCertificate.h"
#include "SiteHandler.h"
#include "HTTPCertificate.h"

#ifdef _AFX
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#endif

// Remember the certificate for this thread
__declspec(thread) PHTTP_SSL_CLIENT_CERT_INFO g_certificate = nullptr;

SiteFilterClientCertificate::SiteFilterClientCertificate(unsigned p_priority,XString p_name)
                            :SiteFilter(p_priority,p_name)
{
}

SiteFilterClientCertificate::~SiteFilterClientCertificate()
{
  FreeCertificate();
}

void
SiteFilterClientCertificate::FreeCertificate()
{
  if(g_certificate)
  {
    // Release any access token, if any
    if(g_certificate->Token)
    {
      CloseHandle(g_certificate->Token);
    }
    // Free the certificate itself
    delete [] g_certificate;
    g_certificate = nullptr;
  }
}

void
SiteFilterClientCertificate::SetClientCertificate(LPCTSTR p_name,LPCTSTR p_thumbprint)
{
  m_certName       = p_name;
  m_certThumbprint = p_thumbprint;
}

void 
SiteFilterClientCertificate::SetSite(HTTPSite* p_site)
{
  m_site = p_site;
  MarlinConfig& config = m_site->GetHTTPServer()->GetWebConfig();

  m_request        = config.GetParameterBoolean(_T("Authentication"),_T("ClientCertificate"),    m_request);
  m_certName       = config.GetParameterString (_T("Authentication"),_T("CertificateName"),      m_certName);
  m_certThumbprint = config.GetParameterString (_T("Authentication"),_T("CertificateThumbprint"),m_certThumbprint);

  if(m_request)
  {
    SITE_DETAILLOG1(_T("Site requests a client certificate : ON"));
    SITE_DETAILLOGS(_T("Site requests for named certificate: "),m_certName);
    SITE_DETAILLOGS(_T("Site requests for cert thumbprint  : "),m_certThumbprint);
  }
}

// Override from SiteFilter::Handle
bool 
SiteFilterClientCertificate::Handle(HTTPMessage* p_message)
{
  DWORD status = ReceiveClientCertificate(p_message);
  if(status == NO_ERROR)
  {
    if(CheckCertificateStatus())
    {
      if(CheckClientCertificate())
      {
        FreeCertificate();
        return true;
      }
    }
  }
  // Tell the log that we are very sorry
  SITE_ERRORLOG(status,_T("Did not received the expected client certificate"));

  // Bounce the HTTPMessage immediately as a 401: Status denied
  p_message->Reset();
  p_message->SetStatus(HTTP_STATUS_DENIED);
  m_site->SendResponse(p_message);
  
  // Free our certificate (if any)
  FreeCertificate();

  // Do not execute the handlers 
  return false;
}

DWORD    
SiteFilterClientCertificate::ReceiveClientCertificate(HTTPMessage* p_message)
{
  DWORD answer        = 0;
  ULONG bytesReceived = 0;
  HANDLE requestQueue = m_site->GetHTTPServer()->GetRequestQueue();
  HTTP_CONNECTION_ID id = p_message->GetConnectionID();
  g_certificate = new HTTP_SSL_CLIENT_CERT_INFO[1];
  ZeroMemory(g_certificate,sizeof(HTTP_SSL_CLIENT_CERT_INFO));

  try
  {
    // Find the needed size of the client certificate
    answer = HttpReceiveClientCertificate(requestQueue
                                         ,id
                                         ,0   // or 1 for the SSL token
                                         ,g_certificate
                                         ,sizeof(HTTP_SSL_CLIENT_CERT_INFO)
                                         ,&bytesReceived
                                         ,NULL);
    if(answer == ERROR_MORE_DATA)
    {
      // A somewhat larger Client certificate has been found
      // Allocate memory for the client certificate
      DWORD size = sizeof(HTTP_SSL_CLIENT_CERT_INFO) + g_certificate->CertEncodedSize;
      delete [] g_certificate;
      g_certificate = (PHTTP_SSL_CLIENT_CERT_INFO) new uchar[size];
      ZeroMemory(g_certificate,size);
      // Requery the client certificate. Now for real!!
      answer = HttpReceiveClientCertificate(requestQueue
                                           ,id
                                           ,0   // or 1 for the SSL token
                                           ,g_certificate
                                           ,size
                                           ,&bytesReceived
                                           ,NULL);
    }
  }
  catch(StdException& ex)
  {
    answer = ERROR_NOT_FOUND;
    SITE_ERRORLOG(answer,_T("Client certificate not received: ") + ex.GetErrorMessage());
  }
  return answer;
}

bool
SiteFilterClientCertificate::CheckCertificateStatus()
{
  bool result = false;

  if(g_certificate)
  {
    if(g_certificate->CertFlags)
    {
      // Possible error situations
      XString status;
      switch(g_certificate->CertFlags)
      {
        case CERT_E_EXPIRED:        status = _T("expired");                           break;
        case CERT_E_UNTRUSTEDCA:    status = _T("untrusted certification authority"); break;
        case CERT_E_WRONG_USAGE:    status = _T("wrong usage");                       break;
        case CERT_E_UNTRUSTEDROOT:  status = _T("from an untrusted root");            break;
        case CERT_E_REVOKED:        status = _T("revoked");                           break;
        case CERT_E_CN_NO_MATCH:    status = _T("without matching certificate name"); break;
        default:                    status = _T("unknown certificate status");        break;
      }
      XString logstring(_T("Received client certificate with status: Certificate "));
      SITE_ERRORLOG(ERROR_ACCESS_DENIED,logstring + status);
    }
    else
    {
      result = true;
    }
  }
  return result;
}

bool
SiteFilterClientCertificate::CheckClientCertificate()
{
  bool byName  = false;
  bool byThumb = false;
  HTTPCertificate cert(g_certificate->pCertEncoded,g_certificate->CertEncodedSize);

  // See if we have a thumbprint to check
  if(!m_certThumbprint.IsEmpty())
  {
    if(cert.VerifyThumbprint(m_certThumbprint) == false)
    {
      SITE_ERRORLOG(ERROR_ACCESS_DENIED,_T("Client-Certificate with incorrect thumbprint"));
      return false;
    }
    byThumb = true;
  }

  // See if we have a subject name's to check
  if(!m_certName.IsEmpty())
  {
    XString toCheck(m_certName);
    XString subject = cert.GetSubject();
    subject.MakeLower();
    toCheck.MakeLower();
    if(subject.Find(toCheck) < 0)
    {
      SITE_ERRORLOG(ERROR_ACCESS_DENIED,_T("Certificate with incorrect subject name"));
      return false;
    }
    byName = true;
  }
  if(m_site->GetHTTPServer()->GetLogLevel() >= HLL_LOGGING)
  {
    SITE_DETAILLOGV(_T("Client Certificate is correct. Name: %s Thumbprint: %s"),byName ? _T("OK") : _T(""),byThumb ? _T("OK") : _T(""));
  }
  return true;
}
