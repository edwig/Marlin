/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: WebConfigClient.cpp
//
// Marlin Component: Internet server/client
// 
// Copyright (c) 2014-2021 ir. W.E. Huisman
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
#include "WebConfigClient.h"
#include "WebConfig.h"
#include "MapDialoog.h"
#include "FileDialog.h"
#include "HTTPClient.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// WebConfigDlg dialog

IMPLEMENT_DYNAMIC(WebConfigClient, CDialog)

WebConfigClient::WebConfigClient(bool p_iis,CWnd* pParent /*=NULL*/)
                :CDialog(WebConfigClient::IDD, pParent)
                ,m_iis(p_iis)
{
  m_useClientUnicode= false;
  m_useUseProxy     = false;
  m_useProxy        = false;
  m_useProxyBypass  = false;
  m_useProxyUser    = false;
  m_useProxyPassword= false;
  m_useAgent        = false;
  m_useRetry        = false;
  m_useSoap         = false;
  m_useCertStore    = false;
  m_useCertName     = false;
  m_useCertPreset   = false;
  m_useOrigin       = false;
  m_useResolve      = false;
  m_useConnect      = false;
  m_useSend         = false;
  m_useReceive      = false;
  m_useRelaxValid   = false;
  m_useRelaxDate    = false;
  m_useRelaxAuthor  = false;
  m_useRelaxUsage   = false;
  m_useForceTunnel  = false;
  m_useGzip         = false;
  m_useSendBOM      = false;

  // CLIENT OVERRIDES
  m_retry           = 0;
  m_TO_resolve      = 0;
  m_TO_connect      = 0;
  m_TO_send         = 0;
  m_TO_receive      = 0;
  m_proxyType       = 0;
  m_clientUnicode   = false;
  m_certPreset      = false;
  m_relaxValid      = false;
  m_relaxDate       = false;
  m_relaxAuthor     = false;
  m_relaxUsage      = false;
  m_useSSL20        = false;
  m_useSSL30        = false;
  m_useTLS10        = false;
  m_useTLS11        = false;
  m_useTLS12        = false;
  m_ssl20           = false;
  m_ssl30           = false;
  m_tls10           = false;
  m_tls11           = false;
  m_tls12           = false;
  m_forceTunnel     = false;
  m_gzip            = false;
  m_sendBOM         = false;
  m_soapCompress    = false;
}

WebConfigClient::~WebConfigClient()
{
}

void WebConfigClient::DoDataExchange(CDataExchange* pDX)
{
  CDialog::DoDataExchange(pDX);
  // USING FIELDS
  DDX_Control(pDX,IDC_USE_CLIENTUNI,  m_buttonUseClientUnicode);
  DDX_Control(pDX,IDC_USE_USEPROXY,   m_buttonUseUseProxy);
  DDX_Control(pDX,IDC_USE_PROXY,      m_buttonUseProxy);
  DDX_Control(pDX,IDC_USE_PROXY_BYPASS,m_buttonUseProxyBypass);
  DDX_Control(pDX,IDC_USE_PROXYUSER,  m_buttonUseProxyUser);
  DDX_Control(pDX,IDC_USE_PROXYPWD,   m_buttonUseProxyPassword);
  DDX_Control(pDX,IDC_USE_AGENT,      m_buttonUseAgent);
  DDX_Control(pDX,IDC_USE_RETRY,      m_buttonUseRetry);
  DDX_Control(pDX,IDC_USE_SOAP,       m_buttonUseSoap);
  DDX_Control(pDX,IDC_USE_CERTSTORE,  m_buttonUseCertStore);
  DDX_Control(pDX,IDC_USE_CERTNAME,   m_buttonUseCertName);
  DDX_Control(pDX,IDC_USE_PRESETCERT, m_buttonUseCertPreset);
  DDX_Control(pDX,IDC_USE_ORIGIN,     m_buttonUseOrigin);
  DDX_Control(pDX,IDC_USE_RESOLVE,    m_buttonUseResolve);
  DDX_Control(pDX,IDC_USE_CONNECT,    m_buttonUseConnect);
  DDX_Control(pDX,IDC_USE_SEND,       m_buttonUseSend);
  DDX_Control(pDX,IDC_USE_RECEIVE,    m_buttonUseReceive);
  DDX_Control(pDX,IDC_USE_CERT_VALID, m_buttonUseRelaxValid);
  DDX_Control(pDX,IDC_USE_CERT_DATE,  m_buttonUseRelaxDate);
  DDX_Control(pDX,IDC_USE_CERT_AUTHOR,m_buttonUseRelaxAuthor);
  DDX_Control(pDX,IDC_USE_CERT_USAGE, m_buttonUseRelaxUsage);
  DDX_Control(pDX,IDC_USE_FORCETUNNEL,m_buttonUseForceTunnel);
  DDX_Control(pDX,IDC_USE_GZIP,       m_buttonUseGzip);
  DDX_Control(pDX,IDC_USE_SENDBOM,    m_buttonUseSendBOM);

  // CLIENT OVERRIDES
  DDX_Control(pDX,IDC_USEPROXY,       m_comboUseProxy);
  DDX_Text   (pDX,IDC_PROXY,          m_proxy);
  DDX_Text   (pDX,IDC_PROXY_BYPASS,   m_proxyBypass);
  DDX_Text   (pDX,IDC_PROXY_USER,     m_proxyUser);
  DDX_Text   (pDX,IDC_PROXY_PASSWORD, m_proxyPassword);
  DDX_Control(pDX,IDC_CLIENTUNI,      m_buttonClientUnicode);
  DDX_Text   (pDX,IDC_AGENT,          m_agent);
  DDX_Text   (pDX,IDC_RETRY,          m_retry);
  DDX_Control(pDX,IDC_SOAPCOMPRESS,   m_buttonSoap);
  DDX_Control(pDX,IDC_CERTSTORE,      m_comboCertStore);
  DDX_Text   (pDX,IDC_CERTNAME,       m_certName);
  DDX_Control(pDX,IDC_PRESETCERT,     m_buttonCertPreset);
  DDX_Text   (pDX,IDC_ORIGIN,         m_origin);
  DDX_Text   (pDX,IDC_OUT_RESOLVE,    m_TO_resolve);
  DDX_Text   (pDX,IDC_OUT_CONNECT,    m_TO_connect);
  DDX_Text   (pDX,IDC_OUT_SEND,       m_TO_send);
  DDX_Text   (pDX,IDC_OUT_RECEIVE,    m_TO_receive);
  DDX_Control(pDX,IDC_CERT_VALID,     m_buttonRelaxValid);
  DDX_Control(pDX,IDC_CERT_DATE,      m_buttonRelaxDate);
  DDX_Control(pDX,IDC_CERT_AUTHOR,    m_buttonRelaxAuthor);
  DDX_Control(pDX,IDC_CERT_USAGE,     m_buttonRelaxUsage);
  DDX_Control(pDX,IDC_FORCETUNNEL,    m_buttonForceTunnel);
  DDX_Control(pDX,IDC_GZIP,           m_buttonGzip);
  DDX_Control(pDX,IDC_SENDBOM,        m_buttonSendBOM);

  if(pDX->m_bSaveAndValidate == FALSE)
  {
    CWnd* w = nullptr;
    
    w = GetDlgItem(IDC_PROXY);          w->EnableWindow(m_useProxy);
    w = GetDlgItem(IDC_PROXY_BYPASS);   w->EnableWindow(m_useProxyBypass);
    w = GetDlgItem(IDC_PROXY_USER);     w->EnableWindow(m_useProxyUser);
    w = GetDlgItem(IDC_PROXY_PASSWORD); w->EnableWindow(m_useProxyPassword);
    w = GetDlgItem(IDC_CERTSTORE);      w->EnableWindow(m_useCertStore);
    w = GetDlgItem(IDC_CERTNAME);       w->EnableWindow(m_useCertName);
    w = GetDlgItem(IDC_AGENT);          w->EnableWindow(m_useAgent);
    w = GetDlgItem(IDC_RETRY);          w->EnableWindow(m_useRetry);
    w = GetDlgItem(IDC_ORIGIN);         w->EnableWindow(m_useOrigin);
    w = GetDlgItem(IDC_OUT_RESOLVE);    w->EnableWindow(m_useResolve);
    w = GetDlgItem(IDC_OUT_CONNECT);    w->EnableWindow(m_useConnect);
    w = GetDlgItem(IDC_OUT_SEND);       w->EnableWindow(m_useSend);
    w = GetDlgItem(IDC_OUT_RECEIVE);    w->EnableWindow(m_useReceive);

    m_buttonClientUnicode.EnableWindow(m_useClientUnicode);
    m_comboUseProxy      .EnableWindow(m_useUseProxy);
    m_buttonCertPreset   .EnableWindow(m_useCertPreset);
    m_buttonRelaxValid   .EnableWindow(m_useRelaxValid);
    m_buttonRelaxDate    .EnableWindow(m_useRelaxDate);
    m_buttonRelaxAuthor  .EnableWindow(m_useRelaxAuthor);
    m_buttonRelaxUsage   .EnableWindow(m_useRelaxUsage);
    m_buttonSoap         .EnableWindow(m_useSoap);
    m_buttonForceTunnel  .EnableWindow(m_useForceTunnel);
    m_buttonGzip         .EnableWindow(m_useGzip);
    m_buttonSendBOM      .EnableWindow(m_useSendBOM);
  }
}

BEGIN_MESSAGE_MAP(WebConfigClient, CDialog)
  // CLIENT OVERRIDES
  ON_CBN_SELCHANGE(IDC_USEPROXY,      &WebConfigClient::OnCbnSelchangeUseProxy)
  ON_EN_CHANGE    (IDC_PROXY,         &WebConfigClient::OnEnChangeProxy)
  ON_EN_CHANGE    (IDC_PROXY_BYPASS,  &WebConfigClient::OnEnChangeProxyBypass)
  ON_EN_CHANGE    (IDC_PROXY_USER,    &WebConfigClient::OnEnChangeProxyUser)
  ON_EN_CHANGE    (IDC_PROXY_PASSWORD,&WebConfigClient::OnEnChangeProxyPassword)
  ON_BN_CLICKED   (IDC_CLIENTUNI,     &WebConfigClient::OnBnClickedClientUnicode)
  ON_EN_CHANGE    (IDC_AGENT,         &WebConfigClient::OnEnChangeAgent)
  ON_EN_CHANGE    (IDC_RETRY,         &WebConfigClient::OnEnChangeRetry)
  ON_BN_CLICKED   (IDC_SOAPCOMPRESS,  &WebConfigClient::OnBnClickedSoap)
  ON_CBN_SELCHANGE(IDC_CERTSTORE,     &WebConfigClient::OnCbnSelchangeCertStore)
  ON_EN_CHANGE    (IDC_CERTNAME,      &WebConfigClient::OnEnChangeCertName)
  ON_BN_CLICKED   (IDC_PRESETCERT,    &WebConfigClient::OnBnClickedCertPreset)
  ON_EN_CHANGE    (IDC_ORIGIN,        &WebConfigClient::OnEnChangeOrigin)
  ON_EN_CHANGE    (IDC_OUT_RESOLVE,   &WebConfigClient::OnEnChangeOutResolve)
  ON_EN_CHANGE    (IDC_OUT_CONNECT,   &WebConfigClient::OnEnChangeOutConnect)
  ON_EN_CHANGE    (IDC_OUT_SEND,      &WebConfigClient::OnEnChangeOutSend)
  ON_EN_CHANGE    (IDC_OUT_RECEIVE,   &WebConfigClient::OnEnChangeOutReceive)
  ON_BN_CLICKED   (IDC_CERT_VALID,    &WebConfigClient::OnBnClickedCertValid)
  ON_BN_CLICKED   (IDC_CERT_DATE,     &WebConfigClient::OnBnClickedCertDate)
  ON_BN_CLICKED   (IDC_CERT_AUTHOR,   &WebConfigClient::OnBnClickedCertAuthor)
  ON_BN_CLICKED   (IDC_CERT_USAGE,    &WebConfigClient::OnBnClickedCertUsage)
  ON_BN_CLICKED   (IDC_CLIENT_SECURE, &WebConfigClient::OnBnClickedClientSecure)
  ON_BN_CLICKED   (IDC_FORCETUNNEL,   &WebConfigClient::OnBnClickedForceTunnel)
  ON_BN_CLICKED   (IDC_GZIP,          &WebConfigClient::OnBnClickedGzip)
  ON_BN_CLICKED   (IDC_SENDBOM,       &WebConfigClient::OnBnClickedSendBOM)

  ON_BN_CLICKED(IDC_USE_CLIENTUNI,    &WebConfigClient::OnBnClickedUseClientUnicode)
  ON_BN_CLICKED(IDC_USE_USEPROXY,     &WebConfigClient::OnBnClickedUseUseProxy)
  ON_BN_CLICKED(IDC_USE_PROXY,        &WebConfigClient::OnBnClickedUseProxy)
  ON_BN_CLICKED(IDC_USE_PROXY_BYPASS, &WebConfigClient::OnBnClickedUseProxyBypass)
  ON_BN_CLICKED(IDC_USE_PROXYUSER,    &WebConfigClient::OnBnClickedUseProxyUser)
  ON_BN_CLICKED(IDC_USE_PROXYPWD,     &WebConfigClient::OnBnClickedUseProxyPassword)
  ON_BN_CLICKED(IDC_USE_AGENT,        &WebConfigClient::OnBnClickedUseAgent)
  ON_BN_CLICKED(IDC_USE_RETRY,        &WebConfigClient::OnBnClickedUseRetry)
  ON_BN_CLICKED(IDC_USE_SOAP,         &WebConfigClient::OnBnClickedUseSoap)
  ON_BN_CLICKED(IDC_USE_CERTSTORE,    &WebConfigClient::OnBnClickedUseCertStore)
  ON_BN_CLICKED(IDC_USE_CERTNAME,     &WebConfigClient::OnBnClickedUseCertName)
  ON_BN_CLICKED(IDC_USE_PRESETCERT,   &WebConfigClient::OnBnClickedUseCertPreset)
  ON_BN_CLICKED(IDC_USE_ORIGIN,       &WebConfigClient::OnBnClickedUseOrigin)
  ON_BN_CLICKED(IDC_USE_RESOLVE,      &WebConfigClient::OnBnClickedUseResolve)
  ON_BN_CLICKED(IDC_USE_CONNECT,      &WebConfigClient::OnBnClickedUseConnect)
  ON_BN_CLICKED(IDC_USE_SEND,         &WebConfigClient::OnBnClickedUseSend)
  ON_BN_CLICKED(IDC_USE_RECEIVE,      &WebConfigClient::OnBnClickedUseReceive)
  ON_BN_CLICKED(IDC_USE_CERT_VALID,   &WebConfigClient::OnBnClickedUseCertValid)
  ON_BN_CLICKED(IDC_USE_CERT_DATE,    &WebConfigClient::OnBnClickedUseCertDate)
  ON_BN_CLICKED(IDC_USE_CERT_AUTHOR,  &WebConfigClient::OnBnClickedUseCertAuthor)
  ON_BN_CLICKED(IDC_USE_CERT_USAGE,   &WebConfigClient::OnBnClickedUseCertUsage)
  ON_BN_CLICKED(IDC_USE_FORCETUNNEL,  &WebConfigClient::OnBnClickedUseForceTunnel)
  ON_BN_CLICKED(IDC_USE_GZIP,         &WebConfigClient::OnBnClickedUseGzip)
  ON_BN_CLICKED(IDC_USE_SENDBOM,      &WebConfigClient::OnBnClickedUseSendBOM)
END_MESSAGE_MAP()

BOOL
WebConfigClient::OnInitDialog()
{
  CDialog::OnInitDialog();

  InitComboboxes();

  return TRUE;
}

void
WebConfigClient::InitComboboxes()
{
  // Proxy use
  m_comboUseProxy.AddString("Use IE autoproxy if possible");
  m_comboUseProxy.AddString("Use IE autoproxy as fallback only");
  m_comboUseProxy.AddString("Use MY proxy settings");
  m_comboUseProxy.AddString("Do NOT use a proxy");

  // Certificate stores
  m_comboCertStore.AddString("My");
  m_comboCertStore.AddString("TrustedPeople");
  m_comboCertStore.AddString("TrustedPublisher");
  m_comboCertStore.AddString("AuthRoot");
  m_comboCertStore.AddString("Root");

  m_comboUseProxy .SetCurSel(0);
  m_comboCertStore.SetCurSel(0);
}

void
WebConfigClient::ReadWebConfig(WebConfig& config)
{
  // READ THE CLIENT OVERRIDES
  m_useUseProxy       = config.HasParameter("Client","UseProxy");
  m_useProxy          = config.HasParameter("Client","Proxy");
  m_useProxyBypass    = config.HasParameter("Client","ProxyBypass");
  m_useProxyUser      = config.HasParameter("Client","ProxyUser");
  m_useProxyPassword  = config.HasParameter("Client","ProxyPassword");
  m_useClientUnicode  = config.HasParameter("Client","SendUnicode");
  m_useAgent          = config.HasParameter("Client","Agent");
  m_useRetry          = config.HasParameter("Client","RetryCount");
  m_useSoap           = config.HasParameter("Client","SOAPCompress");
  m_useCertStore      = config.HasParameter("Client","CertificateStore");
  m_useCertName       = config.HasParameter("Client","CertificateName");
  m_useCertPreset     = config.HasParameter("Client","CertificatePreset");
  m_useOrigin         = config.HasParameter("Client","CORS_Origin");
  m_useResolve        = config.HasParameter("Client","TimeoutResolve");
  m_useConnect        = config.HasParameter("Client","TimeoutConnect");
  m_useSend           = config.HasParameter("Client","TimeoutSend");
  m_useReceive        = config.HasParameter("Client","TimeoutReceive");
  m_useRelaxValid     = config.HasParameter("Client","RelaxCertificateValid");
  m_useRelaxDate      = config.HasParameter("Client","RelaxCertificateDate");
  m_useRelaxAuthor    = config.HasParameter("Client","RelaxCertificateAuthor");
  m_useRelaxUsage     = config.HasParameter("Client","RelaxCertificateUsage");
  m_useForceTunnel    = config.HasParameter("Client","VerbTunneling");
  m_useGzip           = config.HasParameter("Client","HTTPCompression");
  m_useSendBOM        = config.HasParameter("Client","SendBOM");
  
  m_proxyType         = config.GetParameterInteger("Client","UseProxy",         1);
  m_proxy             = config.GetParameterString ("Client","Proxy",           "");
  m_proxyBypass       = config.GetParameterString ("Client","ProxyBypass",     "");
  m_proxyUser         = config.GetParameterString ("Client","ProxyUser",       "");
  m_proxyPassword     = config.GetEncryptedString ("Client","ProxyPassword",   "");
  m_agent             = config.GetParameterString ("Client","Agent",           "");
  m_clientUnicode     = config.GetParameterBoolean("Client","SendUnicode",  false);
  m_retry             = config.GetParameterInteger("Client","RetryCount",       0);
  m_soapCompress      = config.GetParameterBoolean("Client","SOAPCompress", false);
  m_forceTunnel       = config.GetParameterBoolean("Client","VerbTunneling",false);
  m_certStore         = config.GetParameterString ("Client","CertificateStore","");
  m_certName          = config.GetParameterString ("Client","CertificateName", "");
  m_certPreset        = config.GetParameterBoolean("Client","CertificatePreset",false);
  m_origin            = config.GetParameterString ("Client","CORS_Origin",     "");
  m_TO_resolve        = config.GetParameterInteger("Client","TimeoutResolve",  DEF_TIMEOUT_RESOLVE);
  m_TO_connect        = config.GetParameterInteger("Client","TimeoutConnect",  DEF_TIMEOUT_CONNECT);
  m_TO_send           = config.GetParameterInteger("Client","TimeoutSend",     DEF_TIMEOUT_SEND);
  m_TO_receive        = config.GetParameterInteger("Client","TimeoutReceive",  DEF_TIMEOUT_RECEIVE);
  m_relaxValid      = ! config.GetParameterBoolean("Client","RelaxCertificateValid",  false);
  m_relaxDate       = ! config.GetParameterBoolean("Client","RelaxCertificateDate",   false);
  m_relaxAuthor     = ! config.GetParameterBoolean("Client","RelaxCertificateAuthor", false);
  m_relaxUsage      = ! config.GetParameterBoolean("Client","RelaxCertificateUsage",  false);
  m_gzip              = config.GetParameterBoolean("Client","HTTPCompression",        false);
  m_sendBOM           = config.GetParameterBoolean("Client","SendBOM",                false);

  m_useSSL20          = config.HasParameter("Client","SecureSSL20");
  m_useSSL30          = config.HasParameter("Client","SecureSSL30");
  m_useTLS10          = config.HasParameter("Client","SecureTLS10");
  m_useTLS11          = config.HasParameter("Client","SecureTLS11");
  m_useTLS12          = config.HasParameter("Client","SecureTLS12");

  m_ssl20             = config.GetParameterBoolean("Client","SecureSSL20", false);
  m_ssl30             = config.GetParameterBoolean("Client","SecureSSL30", true);
  m_tls10             = config.GetParameterBoolean("Client","SecureTLS10", true);
  m_tls11             = config.GetParameterBoolean("Client","SecureTLS11", true);
  m_tls12             = config.GetParameterBoolean("Client","SecureTLS12", true);
  
  // INIT THE COMBO BOXES
  m_comboUseProxy.SetCurSel(m_proxyType - 1);

  int ind = m_comboCertStore.FindStringExact(0,m_certStore);
  if(ind >= 0)
  {
    m_comboCertStore.SetCurSel(ind);
  }

  // INIT THE CHECKBOXES
  m_buttonRelaxValid   .SetCheck(m_relaxValid);
  m_buttonRelaxDate    .SetCheck(m_relaxDate);
  m_buttonRelaxAuthor  .SetCheck(m_relaxAuthor);
  m_buttonRelaxUsage   .SetCheck(m_relaxUsage);
  m_buttonClientUnicode.SetCheck(m_clientUnicode);
  m_buttonSoap         .SetCheck(m_soapCompress);
  m_buttonForceTunnel  .SetCheck(m_forceTunnel);
  m_buttonCertPreset   .SetCheck(m_certPreset);
  m_buttonGzip         .SetCheck(m_gzip);
  m_buttonSendBOM      .SetCheck(m_sendBOM);

  // INIT ALL USING FIELDS
  m_buttonUseClientUnicode.SetCheck(m_useClientUnicode);
  m_buttonUseUseProxy     .SetCheck(m_useUseProxy);
  m_buttonUseProxy        .SetCheck(m_useProxy);
  m_buttonUseProxyBypass  .SetCheck(m_useProxyBypass);
  m_buttonUseProxyUser    .SetCheck(m_useProxyUser);
  m_buttonUseProxyPassword.SetCheck(m_useProxyPassword);
  m_buttonUseAgent        .SetCheck(m_useAgent);
  m_buttonUseRetry        .SetCheck(m_useRetry);
  m_buttonUseSoap         .SetCheck(m_useSoap);
  m_buttonUseCertStore    .SetCheck(m_useCertStore);
  m_buttonUseCertName     .SetCheck(m_useCertName);
  m_buttonUseCertPreset   .SetCheck(m_useCertPreset);
  m_buttonUseOrigin       .SetCheck(m_useOrigin);
  m_buttonUseResolve      .SetCheck(m_useResolve);
  m_buttonUseConnect      .SetCheck(m_useConnect);
  m_buttonUseSend         .SetCheck(m_useSend);
  m_buttonUseReceive      .SetCheck(m_useReceive);
  m_buttonUseRelaxValid   .SetCheck(m_useRelaxValid);
  m_buttonUseRelaxDate    .SetCheck(m_useRelaxDate);
  m_buttonUseRelaxAuthor  .SetCheck(m_useRelaxAuthor);
  m_buttonUseRelaxUsage   .SetCheck(m_useRelaxUsage);
  m_buttonUseForceTunnel  .SetCheck(m_useForceTunnel);
  m_buttonUseGzip         .SetCheck(m_useGzip);
  m_buttonUseSendBOM      .SetCheck(m_useSendBOM);

  UpdateData(FALSE);
}

void
WebConfigClient::WriteWebConfig(WebConfig& config)
{
  // GET STRINGS
  CString retry;      retry     .Format("%d",m_retry);
  CString toResolve;  toResolve .Format("%d",m_TO_resolve);
  CString toConnect;  toConnect .Format("%d",m_TO_connect);
  CString toSend;     toSend    .Format("%d",m_TO_send);
  CString toReceive;  toReceive .Format("%d",m_TO_receive);

  // WRITE THE CONFIG PARAMETERS
  config.SetSection("Client");

  if(m_useUseProxy)     config.SetParameter   ("Client","UseProxy",         m_proxyType);
  else                  config.RemoveParameter("Client","UseProxy");
  if(m_useProxy)        config.SetParameter   ("Client","Proxy",            m_proxy);
  else                  config.RemoveParameter("Client","Proxy");
  if(m_useProxyBypass)  config.SetParameter   ("Client","ProxyBypass",      m_proxyBypass);
  else                  config.RemoveParameter("Client","ProxyBypass");
  if(m_useProxyUser)    config.SetParameter   ("Client","ProxyUser",        m_proxyUser);
  else                  config.RemoveParameter("Client","ProxyUser");
  if(m_useProxyPassword)config.SetEncrypted   ("Client","ProxyPassword",    m_proxyPassword);
  else                  config.RemoveParameter("Client","ProxyPassword");
  if(m_useClientUnicode)config.SetParameter   ("Client","SendUnicode",      m_clientUnicode);
  else                  config.RemoveParameter("Client","SendUnicode");
  if(m_useAgent)        config.SetParameter   ("Client","Agent",            m_agent);
  else                  config.RemoveParameter("Client","Agent");
  if(m_useRetry)        config.SetParameter   ("Client","RetryCount",       retry);
  else                  config.RemoveParameter("Client","RetryCount");
  if(m_useSoap)         config.SetParameter   ("Client","SOAPCompress",     m_soapCompress);
  else                  config.RemoveParameter("Client","SOAPCompress");
  if(m_useGzip)         config.SetParameter   ("Client","HTTPCompression",  m_gzip);
  else                  config.RemoveParameter("Client","HTTPCompression");
  if(m_useSendBOM)      config.SetParameter   ("Client","SendBOM",          m_sendBOM);
  else                  config.RemoveParameter("Client","SendBOM");
  if(m_useForceTunnel)  config.SetParameter   ("Client","VerbTunneling",    m_forceTunnel);
  else                  config.RemoveParameter("Client","VerbTunneling");
  if(m_useCertStore)    config.SetParameter   ("Client","CertificateStore", m_certStore);
  else                  config.RemoveParameter("Client","CertificateStore");
  if(m_useCertName)     config.SetParameter   ("Client","CertificateName",  m_certName);
  else                  config.RemoveParameter("Client","CertificateName");
  if(m_useCertPreset)   config.SetParameter   ("Client","CertificatePreset",m_certPreset);
  else                  config.RemoveParameter("Client","CertificatePreset");
  if(m_useOrigin)       config.SetParameter   ("Client","CORS_Origin",      m_origin);
  else                  config.RemoveParameter("Client","CORS_Origin");
  if(m_useResolve)      config.SetParameter   ("Client","TimeoutResolve",   toResolve);
  else                  config.RemoveParameter("Client","TimeoutResolve");
  if(m_useConnect)      config.SetParameter   ("Client","TimeoutConnect",   toConnect);
  else                  config.RemoveParameter("Client","TimeoutConnect");
  if(m_useSend)         config.SetParameter   ("Client","TimeoutSend",      toSend);
  else                  config.RemoveParameter("Client","TimeoutSend");
  if(m_useReceive)      config.SetParameter   ("Client","TimeoutReceive",   toReceive);
  else                  config.RemoveParameter("Client","TimeoutReceive");
  if(m_useRelaxValid)   config.SetParameter   ("Client","RelaxCertificateValid", !m_relaxValid);
  else                  config.RemoveParameter("Client","RelaxCertificateValid");
  if(m_useRelaxDate)    config.SetParameter   ("Client","RelaxCertificateDate",  !m_relaxDate);
  else                  config.RemoveParameter("Client","RelaxCertificateDate");
  if(m_useRelaxAuthor)  config.SetParameter   ("Client","RelaxCertificateAuthor",!m_relaxAuthor);
  else                  config.RemoveParameter("Client","RelaxCertificateAuthor");
  if(m_useRelaxUsage)   config.SetParameter   ("Client","RelaxCertificateUsage", !m_relaxUsage);
  else                  config.RemoveParameter("Client","RelaxCertificateUsage");
  if(m_useSSL20)        config.SetParameter   ("Client","SecureSSL20",m_ssl20);
  else                  config.RemoveParameter("Client","SecureSSL20");
  if(m_useSSL30)        config.SetParameter   ("Client","SecureSSL30",m_ssl30);
  else                  config.RemoveParameter("Client","SecureSSL30");
  if(m_useTLS10)        config.SetParameter   ("Client","SecureTLS10",m_tls10);
  else                  config.RemoveParameter("Client","SecureTLS10");
  if(m_useTLS11)        config.SetParameter   ("Client","SecureTLS11",m_tls11);
  else                  config.RemoveParameter("Client","SecureTLS11");
  if(m_useTLS12)        config.SetParameter   ("Client","SecureTLS12",m_tls12);
  else                  config.RemoveParameter("Client","SecureTLS12");
}

// WebConfigDlg message handlers

//////////////////////////////////////////////////////////////////////////
//
// CLIENT OVERRIDES
//
//////////////////////////////////////////////////////////////////////////

void WebConfigClient::OnCbnSelchangeUseProxy()
{
  int sel = m_comboUseProxy.GetCurSel();
  if(sel >= 0)
  {
    m_proxyType = sel + 1;
  }
}

void WebConfigClient::OnEnChangeProxy()
{
  UpdateData();
}

void WebConfigClient::OnEnChangeProxyBypass()
{
  UpdateData();
}

void WebConfigClient::OnEnChangeProxyUser()
{
  UpdateData();
}

void WebConfigClient::OnEnChangeProxyPassword()
{
  UpdateData();
}

void WebConfigClient::OnBnClickedClientUnicode()
{
  m_clientUnicode = m_buttonClientUnicode.GetCheck() > 0;
}

void WebConfigClient::OnEnChangeAgent()
{
  UpdateData();
}

void WebConfigClient::OnEnChangeRetry()
{
  UpdateData();
}

void WebConfigClient::OnCbnSelchangeCertStore()
{
  int ind = m_comboCertStore.GetCurSel();
  if(ind >= 0)
  {
    m_comboCertStore.GetLBText(ind,m_certStore);
  }
}

void WebConfigClient::OnEnChangeCertName()
{
  UpdateData();
}

void WebConfigClient::OnBnClickedCertPreset()
{
  m_certPreset = m_buttonCertPreset.GetCheck() > 0;
}

void WebConfigClient::OnEnChangeOrigin()
{
  UpdateData();
}

void WebConfigClient::OnEnChangeOutResolve()
{
  UpdateData();
}

void WebConfigClient::OnEnChangeOutConnect()
{
  UpdateData();
}

void WebConfigClient::OnEnChangeOutSend()
{
  UpdateData();
}

void WebConfigClient::OnEnChangeOutReceive()
{
  UpdateData();
}

void WebConfigClient::OnBnClickedSoap()
{
  m_soapCompress = m_buttonSoap.GetCheck() > 0;
}

void WebConfigClient::OnBnClickedCertValid()
{
  m_relaxValid = m_buttonRelaxValid.GetCheck() > 0;
}

void WebConfigClient::OnBnClickedCertDate()
{
  m_relaxDate = m_buttonRelaxDate.GetCheck() > 0;
}

void WebConfigClient::OnBnClickedCertAuthor()
{
  m_relaxAuthor = m_buttonRelaxAuthor.GetCheck() > 0;
}

void WebConfigClient::OnBnClickedCertUsage()
{
  m_relaxUsage = m_buttonRelaxUsage.GetCheck() > 0;
}

void 
WebConfigClient::OnBnClickedClientSecure()
{
  SecureClientDlg dlg(this);
  dlg.DoModal();
}

void
WebConfigClient::OnBnClickedForceTunnel()
{
  m_forceTunnel = m_buttonForceTunnel.GetCheck() > 0;
}

void
WebConfigClient::OnBnClickedGzip()
{
  m_gzip = m_buttonGzip.GetCheck() > 0;
}

void
WebConfigClient::OnBnClickedSendBOM()
{
  m_sendBOM = m_buttonSendBOM.GetCheck() > 0;
}

//////////////////////////////////////////////////////////////////////////
//
// USING FIELDS EVENTS
//
//////////////////////////////////////////////////////////////////////////

void
WebConfigClient::OnBnClickedUseClientUnicode()
{
  m_useClientUnicode = m_buttonUseClientUnicode.GetCheck() > 0;
  UpdateData(FALSE);
}

void
WebConfigClient::OnBnClickedUseUseProxy()
{
  m_useUseProxy = m_buttonUseUseProxy.GetCheck() > 0;
  UpdateData(FALSE);
}

void 
WebConfigClient::OnBnClickedUseProxy()
{
  m_useProxy = m_buttonUseProxy.GetCheck() > 0;
  UpdateData(FALSE);
}

void 
WebConfigClient::OnBnClickedUseProxyBypass()
{
  m_useProxyBypass = m_buttonUseProxyBypass.GetCheck() > 0;
  UpdateData(FALSE);
}

void
WebConfigClient::OnBnClickedUseProxyUser()
{
  m_useProxyUser = m_buttonUseProxyUser.GetCheck() > 0;
  UpdateData(FALSE);
}

void
WebConfigClient::OnBnClickedUseProxyPassword()
{
  m_useProxyPassword = m_buttonUseProxyPassword.GetCheck() > 0;
  UpdateData(FALSE);
}

void 
WebConfigClient::OnBnClickedUseAgent()
{
  m_useAgent = m_buttonUseAgent.GetCheck() > 0;
  UpdateData(FALSE);
}

void 
WebConfigClient::OnBnClickedUseRetry()
{
  m_useRetry = m_buttonUseRetry.GetCheck() > 0;
  UpdateData(FALSE);
}

void
WebConfigClient::OnBnClickedUseSoap()
{
  m_useSoap = m_buttonUseSoap.GetCheck() > 0;
  UpdateData(FALSE);
}

void
WebConfigClient::OnBnClickedUseCertStore()
{
  m_useCertStore = m_buttonUseCertStore.GetCheck() > 0;
  UpdateData(FALSE);
}

void
WebConfigClient::OnBnClickedUseCertName()
{
  m_useCertName = m_buttonUseCertName.GetCheck() > 0;
  UpdateData(FALSE);
}

void
WebConfigClient::OnBnClickedUseCertPreset()
{
  m_useCertPreset = m_buttonUseCertPreset.GetCheck() > 0;
  UpdateData(FALSE);
}

void
WebConfigClient::OnBnClickedUseOrigin()
{
  m_useOrigin = m_buttonUseOrigin.GetCheck() > 0;
  UpdateData(FALSE);
}

void 
WebConfigClient::OnBnClickedUseResolve()
{
  m_useResolve = m_buttonUseResolve.GetCheck() > 0;
  UpdateData(FALSE);
}

void 
WebConfigClient::OnBnClickedUseConnect()
{
  m_useConnect = m_buttonUseConnect.GetCheck() > 0;
  UpdateData(FALSE);
}

void 
WebConfigClient::OnBnClickedUseSend()
{
  m_useSend = m_buttonUseSend.GetCheck() > 0;
  UpdateData(FALSE);
}

void 
WebConfigClient::OnBnClickedUseReceive()
{
  m_useReceive = m_buttonUseReceive.GetCheck() > 0;
  UpdateData(FALSE);
}

void 
WebConfigClient::OnBnClickedUseCertValid()
{
  m_useRelaxValid = m_buttonUseRelaxValid.GetCheck() > 0;
  UpdateData(FALSE);
}

void 
WebConfigClient::OnBnClickedUseCertDate()
{
  m_useRelaxDate = m_buttonUseRelaxDate.GetCheck() > 0;
  UpdateData(FALSE);
}

void 
WebConfigClient::OnBnClickedUseCertAuthor()
{
  m_useRelaxAuthor = m_buttonUseRelaxAuthor.GetCheck() > 0;
  UpdateData(FALSE);
}

void 
WebConfigClient::OnBnClickedUseCertUsage()
{
  m_useRelaxUsage = m_buttonUseRelaxUsage.GetCheck() > 0;
  UpdateData(FALSE);
}

void
WebConfigClient::OnBnClickedUseForceTunnel()
{
  m_useForceTunnel = m_buttonUseForceTunnel.GetCheck() > 0;
  UpdateData(FALSE);
}

void
WebConfigClient::OnBnClickedUseGzip()
{
  m_useGzip = m_buttonUseGzip.GetCheck() > 0;
  UpdateData(FALSE);
}

void
WebConfigClient::OnBnClickedUseSendBOM()
{
  m_useSendBOM = m_buttonUseSendBOM.GetCheck() > 0;
  UpdateData(FALSE);
}
