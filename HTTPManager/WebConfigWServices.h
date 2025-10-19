/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: WebConfigWServices.h
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
#pragma once

class MarlinConfig;

// WebConfigDlg dialog

class WebConfigWServices : public CDialogEx
{
  DECLARE_DYNAMIC(WebConfigWServices)

public:
  explicit WebConfigWServices(bool p_iis,CWnd* pParent = NULL);   // standard constructor
  virtual ~WebConfigWServices();
  BOOL OnInitDialog();
  void ReadWebConfig (MarlinConfig& config);
  void WriteWebConfig(MarlinConfig& config);

// Dialog Data
  enum { IDD = IDD_WC_WSERVICES };

protected:
  void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
  void InitComboboxes();

  DECLARE_MESSAGE_MAP()

  bool        m_iis;
  // USING OVERRIDES
  bool        m_useEncLevel;
  bool        m_useEncPassword;
  bool        m_useReliable;
  bool        m_useReliableLogin;
  bool        m_useCheckWSDLIn;
  bool        m_useCheckWSDLOut;
  bool        m_useFieldCheck;

  // WEBSERVICE OVERRIDES
  CString     m_encLevel;
  CString     m_encPassword;
  bool        m_reliable;
  bool        m_reliableLogin;
  bool        m_checkWSDLin;
  bool        m_checkWSDLout;
  bool        m_fieldCheck;

  // Interface items
  CComboBox   m_comboEncryption;
  CButton     m_buttonReliable;
  CButton     m_buttonReliableLogin;
  CButton     m_buttonCheckWSDLin;
  CButton     m_buttonCheckWSDLout;
  CButton     m_buttonFieldCheck;

  CButton     m_buttonUseEncLevel;
  CButton     m_buttonUseEncPassword;
  CButton     m_buttonUseReliable;
  CButton     m_buttonUseReliableLogin;
  CButton     m_buttonUseCheckWSDLin;
  CButton     m_buttonUseCheckWSDLout;
  CButton     m_buttonUseFieldCheck;

public:
  afx_msg void OnCbnSelchangeWsEncrypt();
  afx_msg void OnEnChangeWsencPassword();
  afx_msg void OnBnClickedReliable();
  afx_msg void OnBnClickedReliableLogin();
  afx_msg void OnBnClickedCheckWSDLin();
  afx_msg void OnBnClickedCheckWSDLout();
  afx_msg void OnBnClickedFieldCheck();

  afx_msg void OnBnClickedUseWsEncrypt();
  afx_msg void OnBnClickedUseWsencPw();
  afx_msg void OnBnClickedUseReliable();
  afx_msg void OnBnClickedUseReliableLogin();
  afx_msg void OnBnClickedUseCheckWSDLin();
  afx_msg void OnBnClickedUseCheckWSDLout();
  afx_msg void OnBnClickedUseFieldCheck();
};
