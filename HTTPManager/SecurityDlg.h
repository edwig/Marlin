/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: SecurityDlg.h
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

// SecurityDlg dialog

class SecurityDlg : public CDialogEx
{
	DECLARE_DYNAMIC(SecurityDlg)

public:
  explicit SecurityDlg(CWnd* pParent = NULL);   // standard constructor
  virtual ~SecurityDlg();
  BOOL OnInitDialog();

// Dialog Data
	enum { IDD = IDD_SECURITYOPTIONS };

protected:
	void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
  int  ReadRegistry  (CString p_protocol,CString p_serverClient,CString p_variable,int  p_default);
  void WriteRegistry (CString p_protocol,CString p_serverClient,CString p_variable,bool p_set);
  void ReadServerOptions();
  void ReadClientOptions();
  bool Save();

	DECLARE_MESSAGE_MAP()

  CString m_text;
  bool    m_changed;

  CButton m_buttonServerSSL20;
  CButton m_buttonServerSSL30;
  CButton m_buttonServerTLS10;
  CButton m_buttonServerTLS11;
  CButton m_buttonServerTLS12;

  CButton m_buttonClientSSL20;
  CButton m_buttonClientSSL30;
  CButton m_buttonClientTLS10;
  CButton m_buttonClientTLS11;
  CButton m_buttonClientTLS12;
public:
  afx_msg void OnEnChangeEdit();
  afx_msg void OnBnClickedSsl20();
  afx_msg void OnBnClickedSsl30();
  afx_msg void OnBnClickedTls10();
  afx_msg void OnBnClickedTls11();
  afx_msg void OnBnClickedTls12();
  afx_msg void OnBnClickedSsl20Client();
  afx_msg void OnBnClickedSsl30Client();
  afx_msg void OnBnClickedTls10Client();
  afx_msg void OnBnClickedTls11Client();
  afx_msg void OnBnClickedTls12Client();
  afx_msg void OnBnClickedOk();
  afx_msg void OnBnClickedCancel();
};
