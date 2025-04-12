/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: WebConfigRewriter.h
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
#include "afxdialogex.h"
#include <MarlinConfig.h>


// WebConfigRewriter dialog

class WebConfigRewriter : public CDialog
{
	DECLARE_DYNAMIC(WebConfigRewriter)

public:
	WebConfigRewriter(bool p_iis,CWnd* pParent = nullptr);   // standard constructor
	virtual ~WebConfigRewriter();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_WC_REWRITE };
#endif

 BOOL OnInitDialog() override;
 void ReadWebConfig (MarlinConfig& p_config);
 void WriteWebConfig(MarlinConfig& p_config);


protected:
	virtual void DoDataExchange(CDataExchange* pDX) override;

  bool m_iis;

  // INPUT FIELDS
  CString     m_protocol; 
  CString     m_server;
  CString     m_port;
  CString     m_abspath;
  CString     m_route0;
  CString     m_route1;
  CString     m_route2;
  CString     m_route3;
  CString     m_route4;
  CString     m_removeRoute;
  CString     m_startRoute;
  // TARGET FIELDS
  CString     m_targetProtocol;
  CString     m_targetServer;
  CString     m_targetPort;
  CString     m_targetAbspath;
  CString     m_targetRoute0;
  CString     m_targetRoute1;
  CString     m_targetRoute2;
  CString     m_targetRoute3;
  CString     m_targetRoute4;

  // SERVER OVERRIDES
  bool        m_useProtocol     { false };
  bool        m_useServer       { false };
  bool        m_usePort         { false };
  bool        m_useAbspath      { false };
  bool        m_useRoute0       { false };
  bool        m_useRoute1       { false };
  bool        m_useRoute2       { false };
  bool        m_useRoute3       { false };
  bool        m_useRoute4       { false };
  bool        m_useRemoveRoute  { false };
  bool        m_useStartRoute   { false };

  // INTERFACE
  CButton     m_buttonProtocol; 
  CButton     m_buttonServer;
  CButton     m_buttonPort;
  CButton     m_buttonAbspath;
  CButton     m_buttonRoute0;
  CButton     m_buttonRoute1;
  CButton     m_buttonRoute2;
  CButton     m_buttonRoute3;
  CButton     m_buttonRoute4;
  CButton     m_buttonRemoveRoute;
  CButton     m_buttonStartRoute;
  CComboBox   m_comboProtocol;
  CEdit       m_editServer;
  CEdit       m_editPort;
  CEdit       m_editAbspath;
  CEdit       m_editRoute0;
  CEdit       m_editRoute1;
  CEdit       m_editRoute2;
  CEdit       m_editRoute3;
  CEdit       m_editRoute4;
  CEdit       m_editRemoveRoute;
  CEdit       m_editStartRoute;
  CEdit       m_editTargetProtocol;
  CEdit       m_editTargetServer;
  CEdit       m_editTargetPort;
  CEdit       m_editTargetAbspath;
  CEdit       m_editTargetRoute0;
  CEdit       m_editTargetRoute1;
  CEdit       m_editTargetRoute2;
  CEdit       m_editTargetRoute3;
  CEdit       m_editTargetRoute4;

	DECLARE_MESSAGE_MAP()
public:
  // Use check boxes
  afx_msg void OnBnClickedUseProtocol();
  afx_msg void OnBnClickedUseServer();
  afx_msg void OnBnClickedUsePort();
  afx_msg void OnBnClickedUsePath();
  afx_msg void OnBnClickedUseRoute0();
  afx_msg void OnBnClickedUseRoute1();
  afx_msg void OnBnClickedUseRoute2();
  afx_msg void OnBnClickedUseRoute3();
  afx_msg void OnBnClickedUseRoute4();
  afx_msg void OnBnClickedUseremrout();
  afx_msg void OnBnClickedUseStartroute();
  // To change
  afx_msg void OnCbnSelchangeProtocol();
  afx_msg void OnEnChangeServer();
  afx_msg void OnEnChangePort();
  afx_msg void OnEnChangeAbspath();
  afx_msg void OnEnChangeRoute0();
  afx_msg void OnEnChangeRoute1();
  afx_msg void OnEnChangeRoute2();
  afx_msg void OnEnChangeRoute3();
  afx_msg void OnEnChangeRoute4();
  afx_msg void OnEnChangeRemoveRoute();
  afx_msg void OnEnChangeStartroute();
  // Targets
  afx_msg void OnEnChangeTargetProtocol();
  afx_msg void OnEnChangeTargetServer();
  afx_msg void OnEnChangeTargetPort();
  afx_msg void OnEnChangeTargetAbspath();
  afx_msg void OnEnChangeTargetRoute0();
  afx_msg void OnEnChangeTargetRoute1();
  afx_msg void OnEnChangeTargetRoute2();
  afx_msg void OnEnChangeTargetRoute3();
  afx_msg void OnEnChangeTargetRoute4();
};
