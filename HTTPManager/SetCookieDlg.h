/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: SetCookieDlg.h
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
#include "afxdialogex.h"

class WebConfigServer;

// SetCookieDlg dialog

class SetCookieDlg : public CDialogEx
{
	DECLARE_DYNAMIC(SetCookieDlg)

public:
	SetCookieDlg(CWnd* p_parent);
	virtual ~SetCookieDlg();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_SETCOOKIE };
#endif

protected:
  virtual void DoDataExchange(CDataExchange* pDX) override;
  virtual BOOL OnInitDialog() override;
          void InitFields();

  WebConfigServer* m_config;

  CButton     m_buttonUseCookieSecure;
  CButton     m_buttonUseCookieHttpOnly;
  CButton     m_buttonUseCookieSameSite;
  CButton     m_buttonUseCookiePath;
  CButton     m_buttonUseCookieDomain;
  CButton     m_buttonUseCookieExpires;

  CButton     m_buttonCookieSecure;
  CButton     m_buttonCookieHttpOnly;
  CComboBox   m_comboCookieSameSite;
  CEdit       m_editCookiePath;
  CEdit       m_editCookieDomain;
  CEdit       m_editCookieExpires;

	DECLARE_MESSAGE_MAP()
public:
  afx_msg void OnBnClickedUseCookiesecure();
  afx_msg void OnBnClickedCookiesecure();
  afx_msg void OnBnClickedUseCookiehttponly();
  afx_msg void OnBnClickedCookiehttponly();
  afx_msg void OnBnClickedUseCookiesamesite();
  afx_msg void OnCbnSelchangeCookiesamesite();
  afx_msg void OnBnClickedUseCookiepath();
  afx_msg void OnEnChangeCookiespath();
  afx_msg void OnBnClickedUseCookiedomain();
  afx_msg void OnEnChangeCookiedomain();
  afx_msg void OnBnClickedUseCookieexpires();
  afx_msg void OnEnChangeCookieexpires();
  afx_msg void OnBnClickedOk();
};
