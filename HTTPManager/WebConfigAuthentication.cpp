/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: WebConfigAuthentication.cpp
//
// Marlin Component: Internet server/client
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
#include "HTTPManager.h"
#include "WebConfigAuthentication.h"
#include "MarlinConfig.h"
#include "MapDialog.h"
#include "FileDialog.h"
#include "HTTPClient.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// WebConfigDlg dialog

IMPLEMENT_DYNAMIC(WebConfigAuthentication, CDialogEx)

WebConfigAuthentication::WebConfigAuthentication(bool p_iis,CWnd* pParent /*=NULL*/)
                        :CDialogEx(WebConfigAuthentication::IDD, pParent)
                        ,m_iis(p_iis)
{
  m_useScheme       = false;
  m_useRealm        = false;
  m_useDomain       = false;
  m_useNTLMCache    = false;
  m_useUsername     = false;
  m_usePassword     = false;
  m_useSSO          = false;
  m_useEncLevel     = false;
  m_useEncPassword  = false;
  m_useRequestCert  = false;
  m_useCertName     = false;
  m_useCertThumbprint = false;

  // AUTHENTICATION OVERRIDES
  m_ntlmCache       = false;
  m_sso             = false;
  m_requestCert     = false;
}

WebConfigAuthentication::~WebConfigAuthentication()
{
}

void WebConfigAuthentication::DoDataExchange(CDataExchange* pDX)
{
  CDialogEx::DoDataExchange(pDX);
  // USING FIELDS
  DDX_Control(pDX,IDC_USE_SCHEME,     m_buttonUseScheme);
  DDX_Control(pDX,IDC_USE_REALM,      m_buttonUseRealm);
  DDX_Control(pDX,IDC_USE_DOMAIN,     m_buttonUseDomain);
  DDX_Control(pDX,IDC_USE_NTLM_CACHE, m_buttonUseNTLMCache);
  DDX_Control(pDX,IDC_USE_USERNAME,   m_buttonUseUsername);
  DDX_Control(pDX,IDC_USE_PASSWORD,   m_buttonUsePassword);
  DDX_Control(pDX,IDC_USE_AUT_SSO,    m_buttonUseSSO);
  DDX_Control(pDX,IDC_USE_REQUEST,    m_buttonUseRequestCert);
  DDX_Control(pDX,IDC_USE_CNAME,      m_buttonUseCertName);
  DDX_Control(pDX,IDC_USE_CTHUMB,     m_buttonUseCertThumbprint);

  // AUTHENTICATION OVERRIDES
  DDX_Control(pDX,IDC_AUT_SCHEMA,     m_comboScheme);
  DDX_Text   (pDX,IDC_REALM,          m_realm);
  DDX_Text   (pDX,IDC_DOMAIN,         m_domain);
  DDX_Control(pDX,IDC_NTLM_CACHE,     m_buttonNtlmCache);
  DDX_Text   (pDX,IDC_USERNAME,       m_user);
  DDX_Text   (pDX,IDC_PASSWORD,       m_password);
  DDX_Control(pDX,IDC_AUT_SSO,        m_buttonSso);
  DDX_Control(pDX,IDC_REQUEST_CERT,   m_buttonRequestCert);
  DDX_Text   (pDX,IDC_CERTNAME,       m_certName);
  DDX_Text   (pDX,IDC_THUMBPRINT,     m_certThumbprint);

  if(pDX->m_bSaveAndValidate == FALSE)
  {
    CWnd* w = nullptr;
    
    w = GetDlgItem(IDC_REALM);         w->EnableWindow(m_useRealm);
    w = GetDlgItem(IDC_DOMAIN);        w->EnableWindow(m_useDomain);
    w = GetDlgItem(IDC_USERNAME);      w->EnableWindow(m_useUsername);
    w = GetDlgItem(IDC_PASSWORD);      w->EnableWindow(m_usePassword);
    w = GetDlgItem(IDC_CERTNAME);      w->EnableWindow(m_useCertName);
    w = GetDlgItem(IDC_THUMBPRINT);    w->EnableWindow(m_useCertThumbprint);

    m_comboScheme        .EnableWindow(m_useScheme);
    m_buttonNtlmCache    .EnableWindow(m_useNTLMCache);
    m_buttonUseNTLMCache .EnableWindow(m_scheme.Compare(_T("NTLM")) == 0);
    m_buttonSso          .EnableWindow(m_useSSO);
    m_buttonRequestCert  .EnableWindow(m_useRequestCert);
 }
}

BEGIN_MESSAGE_MAP(WebConfigAuthentication, CDialogEx)
  // AUTHENTICATION OVERRIDES
  ON_CBN_SELCHANGE(IDC_AUT_SCHEMA,    &WebConfigAuthentication::OnCbnSelchangeAutScheme)
  ON_EN_CHANGE    (IDC_REALM,         &WebConfigAuthentication::OnEnChangeRealm)
  ON_EN_CHANGE    (IDC_DOMAIN,        &WebConfigAuthentication::OnEnChangeDomain)
  ON_BN_CLICKED   (IDC_NTLM_CACHE,    &WebConfigAuthentication::OnBnClickedNtlmCache)
  ON_EN_CHANGE    (IDC_USERNAME,      &WebConfigAuthentication::OnEnChangeUsername)
  ON_EN_CHANGE    (IDC_PASSWORD,      &WebConfigAuthentication::OnEnChangePassword)
  ON_BN_CLICKED   (IDC_AUT_SSO,       &WebConfigAuthentication::OnBnClickedAutSso)
  ON_BN_CLICKED   (IDC_REQUEST_CERT,  &WebConfigAuthentication::OnBnClickedRequestCertificate)
  ON_EN_CHANGE    (IDC_CERTNAME,      &WebConfigAuthentication::OnEnChangeCertName)
  ON_EN_CHANGE    (IDC_THUMBPRINT,    &WebConfigAuthentication::OnEnChangeCertThumbprint)
  // USING BUTTONS
  ON_BN_CLICKED(IDC_USE_SCHEME,       &WebConfigAuthentication::OnBnClickedUseScheme)
  ON_BN_CLICKED(IDC_USE_REALM,        &WebConfigAuthentication::OnBnClickedUseRealm)
  ON_BN_CLICKED(IDC_USE_DOMAIN,       &WebConfigAuthentication::OnBnClickedUseDomain)
  ON_BN_CLICKED(IDC_USE_NTLM_CACHE,   &WebConfigAuthentication::OnBnClickedUseNtlmCache)
  ON_BN_CLICKED(IDC_USE_USERNAME,     &WebConfigAuthentication::OnBnClickedUseUsername)
  ON_BN_CLICKED(IDC_USE_PASSWORD,     &WebConfigAuthentication::OnBnClickedUsePassword)
  ON_BN_CLICKED(IDC_USE_AUT_SSO,      &WebConfigAuthentication::OnBnClickedUseAutSso)
  ON_BN_CLICKED(IDC_USE_REQUEST,      &WebConfigAuthentication::OnBnClickedUseRequestCertificate)
  ON_BN_CLICKED(IDC_USE_CNAME,        &WebConfigAuthentication::OnBnClickedUseCertName)
  ON_BN_CLICKED(IDC_USE_CTHUMB,       &WebConfigAuthentication::OnBnClickedUseCertThumbprint)
END_MESSAGE_MAP()

BOOL
WebConfigAuthentication::OnInitDialog()
{
  CDialogEx::OnInitDialog();

  InitComboboxes();
  InitIIS();

  UpdateData(FALSE);
  return TRUE;
}

void
WebConfigAuthentication::InitIIS()
{
  if(m_iis)
  {
 // m_buttonUseScheme        .EnableWindow(false);
//  m_buttonUseNTLMCache     .EnableWindow(false);
    m_buttonUseRealm         .EnableWindow(false);
    m_buttonUseDomain        .EnableWindow(false);
    m_buttonUseRequestCert   .EnableWindow(false);
    m_buttonUseCertName      .EnableWindow(false);
    m_buttonUseCertThumbprint.EnableWindow(false);
  }
}

void
WebConfigAuthentication::InitComboboxes()
{
  // Authentication scheme's
  m_comboScheme.AddString(_T("Anonymous"));
  m_comboScheme.AddString(_T("Basic"));
  m_comboScheme.AddString(_T("NTLM"));
  m_comboScheme.AddString(_T("Negotiate"));
  m_comboScheme.AddString(_T("Digest"));
  m_comboScheme.AddString(_T("Kerberos"));
}

void
WebConfigAuthentication::ReadWebConfig(MarlinConfig& config)
{
  // AUTHENTICATION OVERRIDES
  m_useScheme         = config.HasParameter(_T("Authentication"),_T("Scheme"));
  m_useRealm          = config.HasParameter(_T("Authentication"),_T("Realm"));
  m_useDomain         = config.HasParameter(_T("Authentication"),_T("Domain"));
  m_useNTLMCache      = config.HasParameter(_T("Authentication"),_T("NTLMCache"));
  m_useUsername       = config.HasParameter(_T("Authentication"),_T("User"));
  m_usePassword       = config.HasParameter(_T("Authentication"),_T("Password"));
  m_useSSO            = config.HasParameter(_T("Authentication"),_T("SSO"));
  m_useRequestCert    = config.HasParameter(_T("Authentication"),_T("ClientCertificate"));
  m_useCertName       = config.HasParameter(_T("Authentication"),_T("CertificateName"));
  m_useCertThumbprint = config.HasParameter(_T("Authentication"),_T("CertificateThumbprint"));

  m_scheme            = config.GetParameterString (_T("Authentication"),_T("Scheme"),               _T(""));
  m_ntlmCache         = config.GetParameterBoolean(_T("Authentication"),_T("NTLMCache"),              true);
  m_realm             = config.GetParameterString (_T("Authentication"),_T("Realm"),                _T(""));
  m_domain            = config.GetParameterString (_T("Authentication"),_T("Domain"),               _T(""));
  m_user              = config.GetParameterString (_T("Authentication"),_T("User"),                 _T(""));
  m_password          = config.GetEncryptedString (_T("Authentication"),_T("Password"),             _T(""));
  m_sso               = config.GetParameterBoolean(_T("Authentication"),_T("SSO"),                   false);
  m_requestCert       = config.GetParameterBoolean(_T("Authentication"),_T("ClientCertificate"),     false);
  m_certName          = config.GetParameterString (_T("Authentication"),_T("CertificateName"),      _T(""));
  m_certThumbprint    = config.GetParameterString (_T("Authentication"),_T("CertificateThumbprint"),_T(""));

  SetFields();
}

void
WebConfigAuthentication::SetFields()
{
  // INIT THE COMBO BOXES
  // authentication scheme's
       if(m_scheme.CompareNoCase(_T("anonymous")) == 0) m_comboScheme.SetCurSel(0);
  else if(m_scheme.CompareNoCase(_T("basic"))     == 0) m_comboScheme.SetCurSel(1);
  else if(m_scheme.CompareNoCase(_T("ntlm"))      == 0) m_comboScheme.SetCurSel(2);
  else if(m_scheme.CompareNoCase(_T("negotiate")) == 0) m_comboScheme.SetCurSel(3);
  else if(m_scheme.CompareNoCase(_T("digest"))    == 0) m_comboScheme.SetCurSel(4);
  else if(m_scheme.CompareNoCase(_T("kerberos"))  == 0) m_comboScheme.SetCurSel(5);

  // INIT THE CHECKBOXES
  m_buttonNtlmCache  .SetCheck(m_ntlmCache);
  m_buttonSso        .SetCheck(m_sso);
  m_buttonRequestCert.SetCheck(m_requestCert);

  // INIT ALL USING FIELDS
  m_buttonUseScheme        .SetCheck(m_useScheme);
  m_buttonUseRealm         .SetCheck(m_useRealm);
  m_buttonUseDomain        .SetCheck(m_useDomain);
  m_buttonUseNTLMCache     .SetCheck(m_useNTLMCache);
  m_buttonUseUsername      .SetCheck(m_useUsername);
  m_buttonUsePassword      .SetCheck(m_usePassword);
  m_buttonUseSSO           .SetCheck(m_useSSO);
  m_buttonUseRequestCert   .SetCheck(m_useRequestCert);
  m_buttonUseCertName      .SetCheck(m_useCertName);
  m_buttonUseCertThumbprint.SetCheck(m_useCertThumbprint);

  UpdateData(FALSE);
}

void
WebConfigAuthentication::WriteWebConfig(MarlinConfig& config)
{
  // WRITE THE CONFIG PARAMETERS

  config.SetSection(_T("Authentication"));

  if(m_useScheme)         config.SetParameter   (_T("Authentication"),_T("Scheme"),   m_scheme);
  else                    config.RemoveParameter(_T("Authentication"),_T("Scheme"));
  if(m_useRealm)          config.SetParameter   (_T("Authentication"),_T("Realm"),    m_realm);
  else                    config.RemoveParameter(_T("Authentication"),_T("Realm"));
  if(m_useDomain)         config.SetParameter   (_T("Authentication"),_T("Domain"),   m_domain);
  else                    config.RemoveParameter(_T("Authentication"),_T("Domain"));
  if(m_useNTLMCache)      config.SetParameter   (_T("Authentication"),_T("NTLMCache"),m_ntlmCache);
  else                    config.RemoveParameter(_T("Authentication"),_T("NTLMCache"));
  if(m_useUsername)       config.SetParameter   (_T("Authentication"),_T("User"),     m_user);
  else                    config.RemoveParameter(_T("Authentication"),_T("User"));
  if(m_usePassword)       config.SetEncrypted   (_T("Authentication"),_T("Password"), m_password);
  else                    config.RemoveParameter(_T("Authentication"),_T("Password"));
  if(m_useSSO)            config.SetParameter   (_T("Authentication"),_T("SSO"),      m_sso);
  else                    config.RemoveParameter(_T("Authentication"),_T("SSO"));
  if(m_useRequestCert)    config.SetParameter   (_T("Authentication"),_T("ClientCertificate"),m_requestCert);
  else                    config.RemoveParameter(_T("Authentication"),_T("ClientCertificate"));
  if(m_useCertName)       config.SetParameter   (_T("Authentication"),_T("CertificateName")  ,m_certName);
  else                    config.RemoveParameter(_T("Authentication"),_T("CertificateName"));
  if(m_useCertThumbprint) config.SetParameter   (_T("Authentication"),_T("CertificateThumbprint"),m_certThumbprint);
  else                    config.RemoveParameter(_T("Authentication"),_T("CertificateThumbprint"));
}

// WebConfigDlg message handlers

//////////////////////////////////////////////////////////////////////////
//
// AUTHENTICATION OVERRIDES
//
//////////////////////////////////////////////////////////////////////////

void WebConfigAuthentication::OnCbnSelchangeAutScheme()
{
  int sel = m_comboScheme.GetCurSel();
  if(sel >= 0)
  {
    m_comboScheme.GetLBText(sel,m_scheme);    
  }
  else
  {
    m_scheme.Empty();
  }
  if(m_scheme.Compare(_T("NTLM")))
  {
    m_useNTLMCache = false;
    m_ntlmCache    = false;
  }
  UpdateData(FALSE);
}

void WebConfigAuthentication::OnEnChangeRealm()
{
  UpdateData();
}

void WebConfigAuthentication::OnEnChangeDomain()
{
  UpdateData();
}

void WebConfigAuthentication::OnBnClickedNtlmCache()
{
  m_ntlmCache = m_buttonNtlmCache.GetCheck() > 0;
}

void WebConfigAuthentication::OnEnChangeUsername()
{
  UpdateData();
}

void WebConfigAuthentication::OnEnChangePassword()
{
  UpdateData();
}

void WebConfigAuthentication::OnBnClickedAutSso()
{
  m_sso = m_buttonSso.GetCheck() > 0;
  if(m_sso)
  {
    m_password.Empty();
    UpdateData(FALSE);
  }
}

void WebConfigAuthentication::OnBnClickedRequestCertificate()
{
  m_requestCert = m_buttonRequestCert.GetCheck() > 0;
}

void WebConfigAuthentication::OnEnChangeCertName()
{
  UpdateData();
}

void WebConfigAuthentication::OnEnChangeCertThumbprint()
{
  UpdateData();
}

//////////////////////////////////////////////////////////////////////////
//
// USING FIELDS EVENTS
//
//////////////////////////////////////////////////////////////////////////

void 
WebConfigAuthentication::OnBnClickedUseScheme()
{
  m_useScheme = m_buttonUseScheme.GetCheck() > 0;
  if(m_useScheme == false)
  {
    m_user.Empty();
    m_password.Empty();
    m_scheme        = _T("Anonymous");
    m_useNTLMCache  = false;
    m_ntlmCache     = false;
    m_useSSO        = false;
    m_sso           = false;
    SetFields();
    return;
  }
  UpdateData(FALSE);
}

void 
WebConfigAuthentication::OnBnClickedUseRealm()
{
  m_useRealm = m_buttonUseRealm.GetCheck() > 0;
  UpdateData(FALSE);
}

void 
WebConfigAuthentication::OnBnClickedUseDomain()
{
  m_useDomain = m_buttonUseDomain.GetCheck() > 0;
  UpdateData(FALSE);
}

void 
WebConfigAuthentication::OnBnClickedUseNtlmCache()
{
  m_useNTLMCache = m_buttonUseNTLMCache.GetCheck() > 0;
  if(!(m_scheme.Compare(_T("NTLM")) == 0 && m_useNTLMCache == true))
  {
    m_useNTLMCache = false;
    m_buttonUseNTLMCache.SetCheck(false);
  }
  UpdateData(FALSE);
}

void 
WebConfigAuthentication::OnBnClickedUseUsername()
{
  m_useUsername = m_buttonUseUsername.GetCheck() > 0;
  UpdateData(FALSE);
}

void 
WebConfigAuthentication::OnBnClickedUsePassword()
{
  m_usePassword = m_buttonUsePassword.GetCheck() > 0;
  UpdateData(FALSE);
}

void 
WebConfigAuthentication::OnBnClickedUseAutSso()
{
  m_useSSO = m_buttonUseSSO.GetCheck() > 0;
  UpdateData(FALSE);
}

void
WebConfigAuthentication::OnBnClickedUseRequestCertificate()
{
 m_useRequestCert = m_buttonUseRequestCert.GetCheck() > 0;
 UpdateData(FALSE);
}

void
WebConfigAuthentication::OnBnClickedUseCertName()
{
  m_useCertName = m_buttonUseCertName.GetCheck() > 0;
  UpdateData(FALSE);
}

void
WebConfigAuthentication::OnBnClickedUseCertThumbprint()
{
  m_useCertThumbprint = m_buttonUseCertThumbprint.GetCheck() > 0;
  UpdateData(FALSE);
}
