/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: WebConfigClient.h
//
// Marlin Component: Internet server/client
// 
// Copyright (c) 2015-2018 ir. W.E. Huisman
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
#include "SecureClientDlg.h"
#include "ServerHeadersDlg.h"

class WebConfig;

// WebConfigDlg dialog

class WebConfigClient : public CDialog
{
  DECLARE_DYNAMIC(WebConfigClient)

public:
  WebConfigClient(bool p_iis,CWnd* pParent = NULL);   // standard constructor
 ~WebConfigClient();
  BOOL OnInitDialog();
  void ReadWebConfig (WebConfig& p_config);
  void WriteWebConfig(WebConfig& p_config);
 
// Dialog Data
  enum { IDD = IDD_WC_CLIENT };

protected:
  void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
  void InitComboboxes();

  DECLARE_MESSAGE_MAP()

  // Sub dialog uses our members
  friend      SecureClientDlg;

  bool        m_iis;
  // USE CLIENT OVERRIDES
  bool        m_useClientUnicode;
  bool        m_useUseProxy;
  bool        m_useProxy;
  bool        m_useProxyBypass;
  bool        m_useProxyUser;
  bool        m_useProxyPassword;
  bool        m_useAgent;
  bool        m_useRetry;
  bool        m_useSoap;
  bool        m_useCertStore;
  bool        m_useCertName;
  bool        m_useCertPreset;
  bool        m_useOrigin;
  bool        m_useResolve;
  bool        m_useConnect;
  bool        m_useSend;
  bool        m_useReceive;
  bool        m_useRelaxValid;
  bool        m_useRelaxDate;
  bool        m_useRelaxAuthor;
  bool        m_useRelaxUsage;
  bool        m_useForceTunnel;
  bool        m_useGzip;
  bool        m_useSendBOM;

  // CLIENT OVERRIDES
  CString     m_agent;
  int         m_retry;
  int         m_proxyType;
  CString     m_proxy;
  CString     m_proxyBypass;
  CString     m_proxyUser;
  CString     m_proxyPassword;
  bool        m_clientUnicode;
  bool        m_soapCompress;
  CString     m_certStore;
  CString     m_certName;
  bool        m_certPreset;
  CString     m_origin;
  int         m_TO_resolve;
  int         m_TO_connect;
  int         m_TO_send;
  int         m_TO_receive;
  bool        m_relaxValid;
  bool        m_relaxDate;
  bool        m_relaxAuthor;
  bool        m_relaxUsage;
  bool        m_forceTunnel;
  bool        m_gzip;
  bool        m_sendBOM;

  bool        m_useSSL20;
  bool        m_useSSL30;
  bool        m_useTLS10;
  bool        m_useTLS11;
  bool        m_useTLS12;

  bool        m_ssl20;
  bool        m_ssl30;
  bool        m_tls10;
  bool        m_tls11;
  bool        m_tls12;

  CComboBox   m_comboUseProxy;
  CButton     m_buttonClientUnicode;
  CComboBox   m_comboCertStore;
  CButton     m_buttonCertPreset;
  CButton     m_buttonRelaxValid;
  CButton     m_buttonRelaxDate;
  CButton     m_buttonRelaxAuthor;
  CButton     m_buttonRelaxUsage;
  CButton     m_buttonSoap;
  CButton     m_buttonForceTunnel;
  CButton     m_buttonGzip;
  CButton     m_buttonSendBOM;

  CButton     m_buttonUseClientUnicode;
  CButton     m_buttonUseUseProxy;
  CButton     m_buttonUseProxy;
  CButton     m_buttonUseProxyBypass;
  CButton     m_buttonUseProxyUser;
  CButton     m_buttonUseProxyPassword;
  CButton     m_buttonUseAgent;
  CButton     m_buttonUseRetry;
  CButton     m_buttonUseSoap;
  CButton     m_buttonUseCertStore;
  CButton     m_buttonUseCertName;
  CButton     m_buttonUseCertPreset;
  CButton     m_buttonUseOrigin;
  CButton     m_buttonUseResolve;
  CButton     m_buttonUseConnect;
  CButton     m_buttonUseSend;
  CButton     m_buttonUseReceive;
  CButton     m_buttonUseRelaxValid;
  CButton     m_buttonUseRelaxDate;
  CButton     m_buttonUseRelaxAuthor;
  CButton     m_buttonUseRelaxUsage;
  CButton     m_buttonUseForceTunnel;
  CButton     m_buttonUseGzip;
  CButton     m_buttonUseSendBOM;

public:
  afx_msg void OnBnClickedClientUnicode();
  afx_msg void OnCbnSelchangeUseProxy();
  afx_msg void OnEnChangeProxy();
  afx_msg void OnEnChangeProxyBypass();
  afx_msg void OnEnChangeProxyUser();
  afx_msg void OnEnChangeProxyPassword();
  afx_msg void OnEnChangeAgent();
  afx_msg void OnEnChangeRetry();
  afx_msg void OnBnClickedSoap();
  afx_msg void OnCbnSelchangeCertStore();
  afx_msg void OnEnChangeCertName();
  afx_msg void OnBnClickedCertPreset();
  afx_msg void OnEnChangeOrigin();
  afx_msg void OnEnChangeOutResolve();
  afx_msg void OnEnChangeOutConnect();
  afx_msg void OnEnChangeOutSend();
  afx_msg void OnEnChangeOutReceive();
  afx_msg void OnBnClickedCertValid();
  afx_msg void OnBnClickedCertDate();
  afx_msg void OnBnClickedCertAuthor();
  afx_msg void OnBnClickedCertUsage();
  afx_msg void OnBnClickedForceTunnel();
  afx_msg void OnBnClickedClientSecure();
  afx_msg void OnBnClickedGzip();
  afx_msg void OnBnClickedSendBOM();

  afx_msg void OnBnClickedUseClientUnicode();
  afx_msg void OnBnClickedUseUseProxy();
  afx_msg void OnBnClickedUseProxy();
  afx_msg void OnBnClickedUseProxyBypass();
  afx_msg void OnBnClickedUseProxyUser();
  afx_msg void OnBnClickedUseProxyPassword();
  afx_msg void OnBnClickedUseAgent();
  afx_msg void OnBnClickedUseRetry();
  afx_msg void OnBnClickedUseSoap();
  afx_msg void OnBnClickedUseCertStore();
  afx_msg void OnBnClickedUseCertName();
  afx_msg void OnBnClickedUseCertPreset();
  afx_msg void OnBnClickedUseOrigin();
  afx_msg void OnBnClickedUseResolve();
  afx_msg void OnBnClickedUseConnect();
  afx_msg void OnBnClickedUseSend();
  afx_msg void OnBnClickedUseReceive();
  afx_msg void OnBnClickedUseCertValid();
  afx_msg void OnBnClickedUseCertDate();
  afx_msg void OnBnClickedUseCertAuthor();
  afx_msg void OnBnClickedUseCertUsage();
  afx_msg void OnBnClickedUseForceTunnel();
  afx_msg void OnBnClickedUseGzip();
  afx_msg void OnBnClickedUseSendBOM();
};
