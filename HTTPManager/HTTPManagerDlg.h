/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: HTTPManagerDlg.h
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
#include "CreateURLPrefix.h"

// Known valid combinations of the MS-Windows OS
typedef enum _versie
{
  OSVERSIE_UNKNOWN
 ,OSVERSIE_SERVER2003
 ,OSVERSIE_VISTA_UP
}
OsVersie;

// Configuration command by the interface
typedef enum _configCmd
{
  CONFIG_ASKURL  // Query/Show an URL configuration
 ,CONFIG_ADDURL  // Set/Add    an URL configuration
 ,CONFIG_DELURL  // Delete     an URL configuration
 ,CONFIG_ASKSSL  // Query/Show a SSL Certificate
 ,CONFIG_ADDSSL  // Set/Add    a SSL Certificate
 ,CONFIG_DELSSL  // Delete     a SSL Certificate
 ,CONFIG_ASKLIST // Query/Show listener
 ,CONFIG_ADDLIST // Set/ADD    listener
}
ConfigCmd;

// HTTPManagerDlg dialog
class HTTPManagerDlg : public CDialogEx
{
// Construction
public:
  explicit HTTPManagerDlg(bool p_iis,CWnd* pParent = NULL);
  virtual ~HTTPManagerDlg();

  // Dialog Data
  enum { IDD = IDD_HTTPMANAGER };

  protected:
  virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
  virtual BOOL OnInitDialog();

// Implementation
protected:
  void        ConfigureForIIS();
  XString     GetSiteConfig(XString p_prefix);
  void        CheckPortRange();
  void        MessagePump();
  void        SetVersion();
  void        CheckPathname(bool p_allow = false);
  XString     MakeFirewallRuleName(XString& p_ports);
  bool        DoCommand(ConfigCmd p_config
                       ,XString   p_prefix
                       ,XString&  p_command
                       ,XString&  p_parameters
                       ,XString   p_prefix2 = _T("")
                       ,XString   p_prefix3 = _T("")
                       ,XString   p_prefix4 = _T(""));
    
  // Runmode: true for Microsoft-IIS, false for Marlin stand-alone
  bool        m_iis;
  // Info
  bool        m_secure;
  PrefixType  m_binding;
  int         m_port;
  int         m_portUpto;
  bool        m_doRange;
  CButton     m_buttonSecurity;
  CButton     m_buttonRange;
  XString     m_absPath;
  OsVersie    m_version;
  XString     m_rulesResult;

  HICON       m_hIcon;
  CComboBox   m_comboProtocol;
  CComboBox   m_comboBinding;
  CComboBox   m_comboStore;
  CEdit       m_editPort;
  CEdit       m_editPortUpto;
  CEdit	      m_editPath;
  CEdit       m_editCertificate;
  CEdit	      m_editStatus;

  CButton     m_buttonListener;
  CButton     m_buttonListen;
  CButton		  m_buttonAskURL;
  CButton	    m_buttonCreate;
  CButton     m_buttonRemove;
  CButton     m_buttonAskFWR;
  CButton     m_buttonCreateFWR;
  CButton     m_buttonRemoveFWR;
  CButton     m_buttonClientCert;
  CButton     m_buttonAskCERT;
  CButton	    m_buttonConnect;
  CButton     m_buttonDisconnect;
  CButton     m_buttonWebConfig;
  CButton     m_buttonSiteConfig;
  CButton	    m_buttonOK;
  CButton	    m_buttonCancel;

public:
  DECLARE_MESSAGE_MAP()
  
  // Generated message map functions
  afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
  afx_msg void OnPaint();
  afx_msg HCURSOR OnQueryDragIcon();
  afx_msg void OnCbnSelchangeProtocol();
  afx_msg void OnCbnSelchangeBinding();
  afx_msg void OnEnChangePort();
  afx_msg void OnEnChangePortUpto();
  afx_msg void OnEnChangeAbspath();
  afx_msg void OnBnClickedAddurl();
  afx_msg void OnBnClickedDelurl();
  afx_msg void OnEnChangeThumbprint();
  afx_msg void OnBnClickedAddcert();
  afx_msg void OnBnClickedDelcert();
  afx_msg void OnBnClickedOk();
  afx_msg void OnBnClickedCancel();
  afx_msg void OnBnClickedAskurl();
  afx_msg void OnBnClickedAskcert();
  afx_msg void OnBnClickedRange();
  afx_msg void OnBnClickedListner();
  afx_msg void OnBnClickedListen();
  afx_msg void OnBnClickedNetstat();
  afx_msg void OnBnClickedWebconfig();
  afx_msg void OnBnClickedSecurity();
  afx_msg void OnBnClickedAskFW();
  afx_msg void OnBnClickedAddFW();
  afx_msg void OnBnClickedDelFW();
  afx_msg void OnBnClickedSiteWebConfig();
};
