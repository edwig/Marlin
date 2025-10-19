/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: SetCookieDlg.cpp
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
#include "stdafx.h"
#include "HTTPManager.h"
#include "afxdialogex.h"
#include "WebConfigServer.h"
#include "SetCookieDlg.h"

// SetCookieDlg dialog

IMPLEMENT_DYNAMIC(SetCookieDlg, CDialogEx)

SetCookieDlg::SetCookieDlg(CWnd* p_parent)
	           :CDialogEx(IDD_SETCOOKIE, p_parent)
{
  m_config = reinterpret_cast<WebConfigServer*>(p_parent);
}

SetCookieDlg::~SetCookieDlg()
{
}

void 
SetCookieDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);

  DDX_Control(pDX,IDC_USE_COOKIESECURE,   m_buttonUseCookieSecure);
  DDX_Control(pDX,IDC_USE_COOKIEHTTPONLY, m_buttonUseCookieHttpOnly);
  DDX_Control(pDX,IDC_USE_COOKIESAMESITE, m_buttonUseCookieSameSite);
  DDX_Control(pDX,IDC_USE_COOKIEPATH,     m_buttonUseCookiePath);
  DDX_Control(pDX,IDC_USE_COOKIEDOMAIN,   m_buttonUseCookieDomain);
  DDX_Control(pDX,IDC_USE_COOKIEEXPIRES,  m_buttonUseCookieExpires);
  DDX_Control(pDX,IDC_USE_COOKIEMAXAGE,   m_buttonUseCookieMaxAge);

  DDX_Control(pDX,IDC_COOKIESECURE,       m_buttonCookieSecure);
  DDX_Control(pDX,IDC_COOKIEHTTPONLY,     m_buttonCookieHttpOnly);
  DDX_Control(pDX,IDC_COOKIESAMESITE,     m_comboCookieSameSite);
  DDX_Control(pDX,IDC_COOKIESPATH,        m_editCookiePath);
  DDX_Control(pDX,IDC_COOKIEDOMAIN,       m_editCookieDomain);
  DDX_Control(pDX,IDC_COOKIEEXPIRES,      m_editCookieExpires);
  DDX_Control(pDX,IDC_COOKIEMAXAGE,       m_editCookieMaxAge);

  if(pDX->m_bSaveAndValidate == FALSE)
  {
    CWnd* w = nullptr;
    w = GetDlgItem(IDC_COOKIESECURE);   w->EnableWindow(m_config->m_useCookieSecure);
    w = GetDlgItem(IDC_COOKIEHTTPONLY); w->EnableWindow(m_config->m_useCookieHttpOnly);
    w = GetDlgItem(IDC_COOKIESAMESITE); w->EnableWindow(m_config->m_useCookieSameSite);
    w = GetDlgItem(IDC_COOKIESPATH);    w->EnableWindow(m_config->m_useCookiePath);
    w = GetDlgItem(IDC_COOKIEDOMAIN);   w->EnableWindow(m_config->m_useCookieDomain);
    w = GetDlgItem(IDC_COOKIEEXPIRES);  w->EnableWindow(m_config->m_useCookieExpires);
    w = GetDlgItem(IDC_COOKIEMAXAGE);   w->EnableWindow(m_config->m_useCookieMaxAge);
  }
}

BEGIN_MESSAGE_MAP(SetCookieDlg, CDialogEx)
  ON_BN_CLICKED   (IDC_USE_COOKIESECURE,  &SetCookieDlg::OnBnClickedUseCookiesecure)
  ON_BN_CLICKED   (IDC_USE_COOKIEHTTPONLY,&SetCookieDlg::OnBnClickedUseCookiehttponly)
  ON_BN_CLICKED   (IDC_USE_COOKIESAMESITE,&SetCookieDlg::OnBnClickedUseCookiesamesite)
  ON_BN_CLICKED   (IDC_USE_COOKIEPATH,    &SetCookieDlg::OnBnClickedUseCookiepath)
  ON_BN_CLICKED   (IDC_USE_COOKIEDOMAIN,  &SetCookieDlg::OnBnClickedUseCookiedomain)
  ON_BN_CLICKED   (IDC_USE_COOKIEEXPIRES, &SetCookieDlg::OnBnClickedUseCookieexpires)
  ON_BN_CLICKED   (IDC_USE_COOKIEMAXAGE,  &SetCookieDlg::OnBnClickedUseCookieMaxAge)

  ON_BN_CLICKED   (IDC_COOKIESECURE,      &SetCookieDlg::OnBnClickedCookiesecure)
  ON_BN_CLICKED   (IDC_COOKIEHTTPONLY,    &SetCookieDlg::OnBnClickedCookiehttponly)
  ON_CBN_SELCHANGE(IDC_COOKIESAMESITE,    &SetCookieDlg::OnCbnSelchangeCookiesamesite)
  ON_EN_CHANGE    (IDC_COOKIESPATH,       &SetCookieDlg::OnEnChangeCookiespath)
  ON_EN_CHANGE    (IDC_COOKIEDOMAIN,      &SetCookieDlg::OnEnChangeCookiedomain)
  ON_EN_CHANGE    (IDC_COOKIEEXPIRES,     &SetCookieDlg::OnEnChangeCookieexpires)
  ON_EN_CHANGE    (IDC_COOKIEMAXAGE,      &SetCookieDlg::OnEnChangeCookieMaxAge)
  ON_BN_CLICKED   (IDOK,                  &SetCookieDlg::OnBnClickedOk)
END_MESSAGE_MAP()

BOOL
SetCookieDlg::OnInitDialog()
{
  CDialogEx::OnInitDialog();

  // Cookies same site
  m_comboCookieSameSite.AddString(_T("None"));
  m_comboCookieSameSite.AddString(_T("Lax"));
  m_comboCookieSameSite.AddString(_T("Strict"));

  InitFields();

  UpdateData(FALSE);
  return TRUE;
}

void
SetCookieDlg::InitFields()
{
  TCHAR buffer1[20];
  TCHAR buffer2[20];
  _itot_s(m_config->m_cookieExpires,buffer1,20,10);
  _itot_s(m_config->m_cookieMaxAge, buffer2,20,10);

  // Cookies
  if(m_config->m_cookieSameSite.CompareNoCase(_T("None"))   == 0) m_comboCookieSameSite.SetCurSel(0);
  if(m_config->m_cookieSameSite.CompareNoCase(_T("Lax"))    == 0) m_comboCookieSameSite.SetCurSel(1);
  if(m_config->m_cookieSameSite.CompareNoCase(_T("Strict")) == 0) m_comboCookieSameSite.SetCurSel(2);

  m_buttonCookieSecure  .SetCheck(m_config->m_cookieSecure);
  m_buttonCookieHttpOnly.SetCheck(m_config->m_cookieHttpOnly);
  m_editCookiePath      .SetWindowText(m_config->m_cookiePath);
  m_editCookieDomain    .SetWindowText(m_config->m_cookieDomain);
  m_editCookieExpires   .SetWindowText(buffer1);
  m_editCookieMaxAge    .SetWindowText(buffer2);

  m_buttonUseCookieSecure  .SetCheck(m_config->m_useCookieSecure);
  m_buttonUseCookieHttpOnly.SetCheck(m_config->m_useCookieHttpOnly);
  m_buttonUseCookieSameSite.SetCheck(m_config->m_useCookieSameSite);
  m_buttonUseCookiePath    .SetCheck(m_config->m_useCookiePath);
  m_buttonUseCookieDomain  .SetCheck(m_config->m_useCookieDomain);
  m_buttonUseCookieExpires .SetCheck(m_config->m_useCookieExpires);
  m_buttonUseCookieMaxAge  .SetCheck(m_config->m_useCookieMaxAge);
}

//////////////////////////////////////////////////////////////////////////
// 
// SetCookieDlg message handlers
// 
void 
SetCookieDlg::OnBnClickedUseCookiesecure()
{
  m_config->m_useCookieSecure = m_buttonUseCookieSecure.GetCheck() > 0;
  UpdateData(FALSE);
}

void 
SetCookieDlg::OnBnClickedCookiesecure()
{
  m_config->m_cookieSecure = m_buttonCookieSecure.GetCheck() > 0;
}

void 
SetCookieDlg::OnBnClickedUseCookiehttponly()
{
  m_config->m_useCookieHttpOnly = m_buttonUseCookieHttpOnly.GetCheck() > 0;
  UpdateData(FALSE);
}

void 
SetCookieDlg::OnBnClickedCookiehttponly()
{
  m_config->m_cookieHttpOnly = m_buttonCookieHttpOnly.GetCheck() > 0;
}

void 
SetCookieDlg::OnBnClickedUseCookiesamesite()
{
  m_config->m_useCookieSameSite = m_buttonUseCookieSameSite.GetCheck() > 0;
  UpdateData(FALSE);
}

void SetCookieDlg::OnCbnSelchangeCookiesamesite()
{
  int ind = m_comboCookieSameSite.GetCurSel();
  if(ind >= 0)
  {
    switch(ind)
    {
      case 0: m_config->m_cookieSameSite = _T("None");   break;
      case 1: m_config->m_cookieSameSite = _T("Lax");    break;
      case 2: m_config->m_cookieSameSite = _T("Strict"); break;
    }
  }
}

void 
SetCookieDlg::OnBnClickedUseCookiepath()
{
  m_config->m_useCookiePath = m_buttonUseCookiePath.GetCheck() > 0;
  UpdateData(FALSE);
}

void 
SetCookieDlg::OnEnChangeCookiespath()
{
  UpdateData();
  CString path;
  m_editCookiePath.GetWindowText(path);
  m_config->m_cookiePath = path;
}

void 
SetCookieDlg::OnBnClickedUseCookiedomain()
{
  m_config->m_useCookieDomain = m_buttonUseCookieDomain.GetCheck() > 0;
  UpdateData(FALSE);
}

void 
SetCookieDlg::OnEnChangeCookiedomain()
{
  UpdateData();
  CString domain;
  m_editCookieDomain.GetWindowText(domain);
  m_config->m_cookieDomain = domain;
}

void 
SetCookieDlg::OnBnClickedUseCookieexpires()
{
  m_config->m_useCookieExpires = m_buttonUseCookieExpires.GetCheck() > 0;
  UpdateData(FALSE);
}

void 
SetCookieDlg::OnEnChangeCookieexpires()
{
  UpdateData();
  CString expires;
  m_editCookieExpires.GetWindowText(expires);
  m_config->m_cookieExpires = _ttoi(expires);
}

void
SetCookieDlg::OnBnClickedUseCookieMaxAge()
{
  m_config->m_useCookieMaxAge = m_buttonUseCookieMaxAge.GetCheck() > 0;
  UpdateData(FALSE);
}

void
SetCookieDlg::OnEnChangeCookieMaxAge()
{
  UpdateData();
  CString maxage;
  m_editCookieMaxAge.GetWindowText(maxage);
  m_config->m_cookieMaxAge = _ttoi(maxage);
}

void 
SetCookieDlg::OnBnClickedOk()
{
  CDialogEx::OnOK();
}
