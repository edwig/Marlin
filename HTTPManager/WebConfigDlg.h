/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: WebConfigDlg.h
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
#include "WebConfigServer.h"
#include "WebConfigClient.h"
#include "WebConfigAuthentication.h"
#include "WebConfigWServices.h"
#include "WebConfigLogging.h"

class WebConfig;

// WebConfigDlg dialog

class WebConfigDlg : public CDialogEx
{
  DECLARE_DYNAMIC(WebConfigDlg)

public:
  WebConfigDlg(bool p_iis,CWnd* pParent = NULL);   // standard constructor
  virtual ~WebConfigDlg();
  BOOL     OnInitDialog();
  void     SetSiteConfig(CString p_urlPrefix,CString p_fileName);

// Dialog Data
#ifdef AFX_DESIGN_TIME
  enum { IDD = IDD_WEBCONFIG };
#endif

protected:
  void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
  void ReadWebConfig();
  bool WriteWebConfig();
  void InitTabs();
  void RemoveLogTab();

  DECLARE_MESSAGE_MAP()

  bool     m_iis;
  CString  m_url;
  CString  m_siteConfigFile;
  CString  m_title;
  CTabCtrl m_tab;
  HICON    m_hIcon;

  WebConfigServer         m_page1 { m_iis,this };
  WebConfigClient         m_page2 { m_iis,this };
  WebConfigAuthentication m_page3 { m_iis,this };
  WebConfigWServices      m_page4 { m_iis,this };
  WebConfigLogging        m_page5 { m_iis,this };

  WebConfig* m_webconfig { nullptr };
public:
  afx_msg void OnTcnSelchangeTab(NMHDR *pNMHDR,LRESULT *pResult);
  afx_msg void OnBnClickedOk();
  afx_msg void OnBnClickedCancel();
};
