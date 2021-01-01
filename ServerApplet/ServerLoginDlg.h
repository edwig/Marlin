/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: ServerLoginDlg.h
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
#include "ColorEdit.h"

// ServerLoginDlg dialog

class ServerLoginDlg : public CDialog
{
	DECLARE_DYNAMIC(ServerLoginDlg)

public:
	ServerLoginDlg(CString p_titel,CString p_url,CWnd* p_parent = NULL);
 ~ServerLoginDlg();
  BOOL    OnInitDialog();
  CString GetURL()      { return m_url;   };
  CString GetUser()     { return m_user;     };
  CString GetPassword() { return m_password; };

// Dialog Data
	enum { IDD = IDD_LOGIN };

protected:
  void InitPassword();
	void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()

  CString   m_titel;
  CString   m_url;
  CString   m_user;
  CString   m_password;
  ColorEdit m_passwordEdit;
public:
  afx_msg void OnEnChangeUrl();
  afx_msg void OnEnChangeUser();
  afx_msg void OnEnChangePassword();
};
