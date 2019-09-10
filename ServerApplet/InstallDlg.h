/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: InstallDlg.h
//
// Marlin Component: Internet server/client
// 
// Copyright (c) 2016 ir. W.E. Huisman
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
#include "ColorEdit.h"

// InstallDlg dialog

class InstallDlg : public CDialog
{
	DECLARE_DYNAMIC(InstallDlg)

public:
	InstallDlg(CWnd* pParent = NULL,int p_install = 0);   // standard constructor
	virtual ~InstallDlg();
  BOOL OnInitDialog();

// Dialog Data
	enum { IDD = IDD_INSTALL };

protected:
  void InitVelden();
	void DoDataExchange(CDataExchange* pDX);
  void RemoveService(bool p_tonen);

	DECLARE_MESSAGE_MAP()

  int      m_install;
  CString  m_domain;
  CString  m_username;
  CString  m_password;
  CString  m_confirm;
  CButton  m_buttonInstall;
  CButton  m_buttonRemove;

  ColorEdit m_editPassword;
  ColorEdit m_editConfirm;

public:
  afx_msg void OnTimer(UINT_PTR nIDEvent);
  afx_msg void OnEnChangeDomain();
  afx_msg void OnEnChangeUsername();
  afx_msg void OnEnChangePassword();
  afx_msg void OnEnChangeConfirm();
  afx_msg void OnBnClickedInstall();
  afx_msg void OnBnClickedRemove();
};
