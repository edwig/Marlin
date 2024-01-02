/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: SecureClientDlg.h
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

class WebConfigClient;

// SecureClientDlg dialog

class SecureClientDlg : public CDialogEx
{
	DECLARE_DYNAMIC(SecureClientDlg)

public:
  explicit SecureClientDlg(CWnd* p_parent);   // standard constructor
  virtual ~SecureClientDlg();
  BOOL OnInitDialog();

// Dialog Data
	enum { IDD = IDD_SECURECLIENT };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()

  WebConfigClient* m_config;

  CButton m_buttonUseSSL20;
  CButton m_buttonUseSSL30;
  CButton m_buttonUseTLS10;
  CButton m_buttonUseTLS11;
  CButton m_buttonUseTLS12;

  CButton m_buttonSSL20;
  CButton m_buttonSSL30;
  CButton m_buttonTLS10;
  CButton m_buttonTLS11;
  CButton m_buttonTLS12;

public:
  afx_msg void OnBnClickedHttpSsl20();
  afx_msg void OnBnClickedUseSsl20();
  afx_msg void OnBnClickedHttpSsl30();
  afx_msg void OnBnClickedUseSsl30();
  afx_msg void OnBnClickedHttpTls10();
  afx_msg void OnBnClickedUseTls10();
  afx_msg void OnBnClickedHttpTls11();
  afx_msg void OnBnClickedUseTls11();
  afx_msg void OnBnClickedHttpTls12();
  afx_msg void OnBnClickedUseTls12();
};
