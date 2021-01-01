/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: ConfigDlg.h
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
#pragma once

// ConfigDlg dialog

class ConfigDlg : public CDialog
{
  DECLARE_DYNAMIC(ConfigDlg)

public:
  ConfigDlg(CWnd* p_parent,bool p_writeable = true);
  virtual ~ConfigDlg();
  BOOL     OnInitDialog();
  int      GetRunAsService();

// Dialog Data
  enum { IDD = IDD_CONFIG };

protected:
  void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
  void ReadConfig();
  bool WriteConfig();
  bool CheckConfig();
  bool SaveConfig();
  bool WarningWriteRights();
  bool CheckFRVeldIngevuld(const CString& frVeld, const CString& omschrijving);
  void SetItemActive(int p_item,bool p_active);
  bool CreateDirectories();
  void WriteableConfigDlg();
  void SetControlWriteable(UINT p_dlgID);
  void ConstructServerAddress();
  void ObviousChecks();
  void InitLogging(CComboBox& p_combo);
  
  DECLARE_MESSAGE_MAP()

  bool      m_dialogWriteable;
  bool      m_configWriteable;

  CString   m_role;
  CString   m_naam;
  int       m_instance;
  CString   m_serverAddress;
  CString   m_baseURL;
  CString   m_server;
  int       m_serverPort;
  CString   m_serverLogfile;
  int       m_serverLogging;
  CString   m_webroot;
  int       m_runAsService;
  bool      m_secureServer;
  // Foutrapportages
  CString   m_mailServer;
  bool      m_foutRapportVestuur;
  CString   m_foutRapportAfzender;
  CString   m_foutRapportOntvanger;
  bool      m_foutRapportTest;

  // Controls
  CComboBox   m_comboRole;
  CButton     m_buttonWebroot;
  CComboBox   m_comboServerLogging;
  CButton     m_buttonServerLog;
  CComboBox   m_comboRunAs;
  CButton     m_buttonFRVerstuur;
  CButton     m_buttonFRTest;
  CButton     m_buttonTestMail;
  CButton     m_buttonSecureServer;

public:
  afx_msg void OnTimer(UINT_PTR nIDEvent);
  afx_msg void OnCbnRole();
  afx_msg void OnEnChangeNaam();
  afx_msg void OnEnChangeInstantie();
  afx_msg void OnEnChangeServer();
  afx_msg void OnEnChangeServerport();
  afx_msg void OnEnChangeWebroot();
  afx_msg void OnBnClickedSearchroot();
  afx_msg void OnCbnSeverlogging();
  afx_msg void OnEnChangeServerlog();
  afx_msg void OnBnClickedSearchserverlog();
  afx_msg void OnBnClickedOk();
  afx_msg void OnCbnRunAs();
  afx_msg void OnEnChangeMailServer();
  afx_msg void OnEnChangeFRAfzender();
  afx_msg void OnEnChangeFROntvanger();
  afx_msg void OnBnClickedFRVerstuur();
  afx_msg void OnBnClickedFRTest();
  afx_msg void OnEnChangeBaseURL();
  afx_msg void OnEnChangeBriefserverURL();
  afx_msg void OnBnClickedTestMail();
  afx_msg void OnBnClickedSecureServer();
};

inline int
ConfigDlg::GetRunAsService()
{
  return m_runAsService;
}
