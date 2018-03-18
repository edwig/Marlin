/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: WebConfigAuthentication.h
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

class WebConfig;

// WebConfigDlg dialog

class WebConfigAuthentication : public CDialogEx
{
  DECLARE_DYNAMIC(WebConfigAuthentication)

public:
  WebConfigAuthentication(bool p_iis,CWnd* pParent = NULL);   // standard constructor
 ~WebConfigAuthentication();
  BOOL OnInitDialog();
  void ReadWebConfig(WebConfig& config);
  void WriteWebConfig(WebConfig& config);

// Dialog Data
  enum { IDD = IDD_WC_AUTHENTICATION };

protected:
  void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
  void InitComboboxes();
  void InitIIS();

  DECLARE_MESSAGE_MAP()

  bool        m_iis;
  // USE OVERRIDES
  bool        m_useScheme;
  bool        m_useRealm;
  bool        m_useDomain;
  bool        m_useNTLMCache;
  bool        m_useUsername;
  bool        m_usePassword;
  bool        m_useSSO;
  bool        m_useEncLevel;
  bool        m_useEncPassword;
  bool        m_useRequestCert;
  bool        m_useCertName;
  bool        m_useCertThumbprint;

  // AUTHENTICATION OVERRIDES
  CString     m_scheme;
  bool        m_ntlmCache;
  CString     m_realm;
  CString     m_domain;
  CString     m_user;
  CString     m_password;
  bool        m_sso;
  bool        m_requestCert;
  CString     m_certName;
  CString     m_certThumbprint;

  // Interface items
  CComboBox   m_comboScheme;
  CButton     m_buttonNtlmCache;
  CButton     m_buttonSso;
  CComboBox   m_comboEncryption;
  CButton     m_buttonRequestCert;

  CButton     m_buttonUseScheme;
  CButton     m_buttonUseRealm;
  CButton     m_buttonUseDomain;
  CButton     m_buttonUseNTLMCache;
  CButton     m_buttonUseUsername;
  CButton     m_buttonUsePassword;
  CButton     m_buttonUseSSO;
  CButton     m_buttonUseEncLevel;
  CButton     m_buttonUseEncPassword;
  CButton     m_buttonUseRequestCert;
  CButton     m_buttonUseCertName;
  CButton     m_buttonUseCertThumbprint;

public:
  afx_msg void OnCbnSelchangeAutScheme();
  afx_msg void OnEnChangeRealm();
  afx_msg void OnEnChangeDomain();
  afx_msg void OnBnClickedNtlmCache();
  afx_msg void OnEnChangeUsername();
  afx_msg void OnEnChangePassword();
  afx_msg void OnBnClickedAutSso();
  afx_msg void OnBnClickedRequestCertificate();
  afx_msg void OnEnChangeCertName();
  afx_msg void OnEnChangeCertThumbprint();

  afx_msg void OnBnClickedUseScheme();
  afx_msg void OnBnClickedUseRealm();
  afx_msg void OnBnClickedUseDomain();
  afx_msg void OnBnClickedUseNtlmCache();
  afx_msg void OnBnClickedUseUsername();
  afx_msg void OnBnClickedUsePassword();
  afx_msg void OnBnClickedUseAutSso();
  afx_msg void OnBnClickedUseRequestCertificate();
  afx_msg void OnBnClickedUseCertName();
  afx_msg void OnBnClickedUseCertThumbprint();
};
