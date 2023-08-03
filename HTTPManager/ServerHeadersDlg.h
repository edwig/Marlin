/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: ServerHeadersDlg.cpp
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

class WebConfigServer;

// ServerHeadersDlg dialog

class ServerHeadersDlg : public CDialog
{
	DECLARE_DYNAMIC(ServerHeadersDlg)
public:
  explicit ServerHeadersDlg(CWnd* p_parent);
  virtual ~ServerHeadersDlg();

// Dialog Data
	enum { IDD = IDD_SERVERHEADERS };

protected:
	void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
  BOOL OnInitDialog();
  void CorrectXFrameOption();
  bool CheckFields();

	DECLARE_MESSAGE_MAP()

private:
  WebConfigServer* m_config;

  XString       m_warning;
  XString       m_XFrameURL;
  int           m_hstsMaxAge;
  XString       m_allowOrigin;
  XString       m_allowHeaders;
  int           m_allowMaxAge;

  CButton       m_buttonUseXFrame;
  CButton       m_buttonUseHstsMaxAge;
  CButton       m_buttonUseHstsSubDomain;
  CButton       m_buttonUseNoSniff;
  CButton       m_buttonUseXssProtection;
  CButton       m_buttonUseXssBlockMode;
  CButton       m_buttonUseNoCacheControl;
  CButton       m_buttonUseCORS;

  CComboBox     m_comboXFrameOptions;
  CButton       m_buttonHstsSubDomain;
  CButton       m_buttonNoSniff;
  CButton       m_buttonXssProtection;
  CButton       m_buttonXssBlockMode;
  CButton       m_buttonNoCacheControl;
  CButton       m_buttonCORS;
  CButton       m_buttonCORSCredentials;

public:
  afx_msg void OnBnClickedUseNocache();
  afx_msg void OnBnClickedUseXframe();
  afx_msg void OnBnClickedUseXssprotect();
  afx_msg void OnBnClickedUseXssblock();
  afx_msg void OnBnClickedUseHsts();
  afx_msg void OnBnClickedUseHstssub();
  afx_msg void OnBnClickedUseNosniff();
  afx_msg void OnBnClickedUseCORS();
  afx_msg void OnBnClickedNocache();
  afx_msg void OnCbnSelchangeXframe();
  afx_msg void OnEnChangeXfurl();
  afx_msg void OnBnClickedXssprotect();
  afx_msg void OnBnClickedXssblock();
  afx_msg void OnEnChangeHsts();
  afx_msg void OnBnClickedHstssub();
  afx_msg void OnBnClickedNosniff();
  afx_msg void OnBnClickedCORS();
  afx_msg void OnEnChangeAllowOrigin();
  afx_msg void OnEnChangeAllowHeaders();
  afx_msg void OnEnChangeAllowMaxAge();
  afx_msg void OnBnClickedAllowCredentials();
  afx_msg void OnOK();
};
