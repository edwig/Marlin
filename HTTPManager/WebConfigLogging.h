/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: WebConfigLogging.h
//
// Marlin Component: Internet server/client
// 
// Copyright (c) 2014-2022 ir. W.E. Huisman
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

class WebConfigLogging : public CDialogEx
{
  DECLARE_DYNAMIC(WebConfigLogging)

public:
  explicit WebConfigLogging(bool p_iis,CWnd* pParent = NULL);   // standard constructor
  virtual ~WebConfigLogging();
  BOOL OnInitDialog();
  void ReadWebConfig (MarlinConfig& p_config);
  void WriteWebConfig(MarlinConfig& p_config);

// Dialog Data
  enum { IDD = IDD_WC_LOGGING };

protected:
  void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

  DECLARE_MESSAGE_MAP()

  bool        m_iis;
  // SERVER OVERRIDES
  bool        m_useLogfile;
  bool        m_useLogCaching;
  bool        m_useLogging;
  bool        m_useLogTiming;
  bool        m_useLogEvents;
  bool        m_useLogLevel;

  // LOGFILE OVERRIDES
  XString     m_logfile;
  int         m_logCache;
  bool        m_doLogging;
  bool        m_doTiming;
  bool        m_doEvents;
  int         m_logLevel;

  // Interface items
  CButton     m_buttonLogging;
  CButton     m_buttonTiming;
  CButton     m_buttonEvents;
  CComboBox   m_comboLogLevel;
  CButton     m_buttonUseLogfile;
  CButton     m_buttonUseLogCaching;
  CButton     m_buttonUseLogging;
  CButton     m_buttonUseLogTiming;
  CButton     m_buttonUseLogEvents;
  CButton     m_buttonUseLogLevel;

public:
  afx_msg void OnEnChangeLogfile();
  afx_msg void OnBnClickedButtLogfile();
  afx_msg void OnEnChangeLogCachelines();
  afx_msg void OnBnClickedLogging();
  afx_msg void OnBnClickedLogTiming();
  afx_msg void OnBnClickedLogEvents();
  afx_msg void OnCbnSelchangeLogLevel();

  afx_msg void OnBnClickedUseLogfile();
  afx_msg void OnBnClickedUseCaching();
  afx_msg void OnBnClickedUseLogging();
  afx_msg void OnBnClickedUseLogTiming();
  afx_msg void OnBnClickedUseLogEvents();
  afx_msg void OnBnClickedUseLogLevel();
};
