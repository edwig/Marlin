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
#include "stdafx.h"
#include "HTTPManager.h"
#include "ServerHeadersDlg.h"
#include "WebConfigServer.h"
#include "afxdialogex.h"

// ServerHeadersDlg dialog

IMPLEMENT_DYNAMIC(ServerHeadersDlg, CDialog)

ServerHeadersDlg::ServerHeadersDlg(CWnd* p_parent)
                 :CDialog(ServerHeadersDlg::IDD, p_parent)
                 ,m_hstsMaxAge(0)
                 ,m_allowMaxAge(0)
{
  m_config = reinterpret_cast<WebConfigServer*>(p_parent);
}

ServerHeadersDlg::~ServerHeadersDlg()
{
}

void ServerHeadersDlg::DoDataExchange(CDataExchange* pDX)
{
  CDialog::DoDataExchange(pDX);

  DDX_Text   (pDX,IDC_WARNING,          m_warning);
  DDX_Control(pDX,IDC_USE_NOCACHE,      m_buttonUseNoCacheControl);
  DDX_Control(pDX,IDC_USE_XFRAME,       m_buttonUseXFrame);
  DDX_Control(pDX,IDC_USE_XSSPROTECT,   m_buttonUseXssProtection);
  DDX_Control(pDX,IDC_USE_XSSBLOCK,     m_buttonUseXssBlockMode);
  DDX_Control(pDX,IDC_USE_HSTS,         m_buttonUseHstsMaxAge);
  DDX_Control(pDX,IDC_USE_HSTSSUB,      m_buttonUseHstsSubDomain);
  DDX_Control(pDX,IDC_USE_NOSNIFF,      m_buttonUseNoSniff);
  DDX_Control(pDX,IDC_USE_CORS,         m_buttonUseCORS);

  DDX_Control(pDX,IDC_XFRAME,           m_comboXFrameOptions);
  DDX_Control(pDX,IDC_NOCACHE,          m_buttonNoCacheControl);
  DDX_Control(pDX,IDC_XSSPROTECT,       m_buttonXssProtection);
  DDX_Control(pDX,IDC_XSSBLOCK,         m_buttonXssBlockMode);
  DDX_Control(pDX,IDC_NOSNIFF,          m_buttonNoSniff);
  DDX_Control(pDX,IDC_HSTSSUB,          m_buttonHstsSubDomain);
  DDX_Control(pDX,IDC_CORS,             m_buttonCORS);
  DDX_Text   (pDX,IDC_XFURL,            m_XFrameURL);
  DDX_Text   (pDX,IDC_HSTS,             m_hstsMaxAge);
  DDX_Text   (pDX,IDC_ALLOW_ORIGIN,     m_allowOrigin);
  DDX_Text   (pDX,IDC_ALLOW_HEADERS,    m_allowHeaders);
  DDX_Text   (pDX,IDC_ALLOW_MAXAGE,     m_allowMaxAge);
  DDX_Control(pDX,IDC_ALLOW_CREDENTIALS,m_buttonCORSCredentials);

  if(pDX->m_bSaveAndValidate == FALSE)
  {
    CWnd* w = nullptr;

    w = GetDlgItem(IDC_NOCACHE);            w->EnableWindow(m_config->m_useNoCache);
    w = GetDlgItem(IDC_XFRAME);             w->EnableWindow(m_config->m_useXFrameOpt);
    w = GetDlgItem(IDC_XFURL);              w->EnableWindow(m_config->m_useXFrameAllow);
    w = GetDlgItem(IDC_XSSPROTECT);         w->EnableWindow(m_config->m_useXssProtect);
    w = GetDlgItem(IDC_XSSBLOCK);           w->EnableWindow(m_config->m_useXssBlock);
    w = GetDlgItem(IDC_HSTS);               w->EnableWindow(m_config->m_useHstsMaxAge);
    w = GetDlgItem(IDC_HSTSSUB);            w->EnableWindow(m_config->m_useHstsDomain);
    w = GetDlgItem(IDC_NOSNIFF);            w->EnableWindow(m_config->m_useNoSniff);
    w = GetDlgItem(IDC_CORS);               w->EnableWindow(m_config->m_useCORS);
    w = GetDlgItem(IDC_ALLOW_ORIGIN);       w->EnableWindow(m_config->m_useCORS);
    w = GetDlgItem(IDC_ALLOW_HEADERS);      w->EnableWindow(m_config->m_useCORS);
    w = GetDlgItem(IDC_ALLOW_MAXAGE);       w->EnableWindow(m_config->m_useCORS);
    w = GetDlgItem(IDC_ALLOW_CREDENTIALS);  w->EnableWindow(m_config->m_useCORS);
  }
}

BEGIN_MESSAGE_MAP(ServerHeadersDlg, CDialog)
  ON_BN_CLICKED(IDC_USE_NOCACHE,      &ServerHeadersDlg::OnBnClickedUseNocache)
  ON_BN_CLICKED(IDC_USE_XFRAME,       &ServerHeadersDlg::OnBnClickedUseXframe)
  ON_BN_CLICKED(IDC_USE_XSSPROTECT,   &ServerHeadersDlg::OnBnClickedUseXssprotect)
  ON_BN_CLICKED(IDC_USE_XSSBLOCK,     &ServerHeadersDlg::OnBnClickedUseXssblock)
  ON_BN_CLICKED(IDC_USE_HSTS,         &ServerHeadersDlg::OnBnClickedUseHsts)
  ON_BN_CLICKED(IDC_USE_HSTSSUB,      &ServerHeadersDlg::OnBnClickedUseHstssub)
  ON_BN_CLICKED(IDC_USE_NOSNIFF,      &ServerHeadersDlg::OnBnClickedUseNosniff)
  ON_BN_CLICKED(IDC_USE_CORS,         &ServerHeadersDlg::OnBnClickedUseCORS)
  ON_BN_CLICKED(IDC_NOCACHE,          &ServerHeadersDlg::OnBnClickedNocache)
  ON_CBN_SELCHANGE(IDC_XFRAME,        &ServerHeadersDlg::OnCbnSelchangeXframe)
  ON_EN_CHANGE (IDC_XFURL,            &ServerHeadersDlg::OnEnChangeXfurl)
  ON_BN_CLICKED(IDC_XSSPROTECT,       &ServerHeadersDlg::OnBnClickedXssprotect)
  ON_BN_CLICKED(IDC_XSSBLOCK,         &ServerHeadersDlg::OnBnClickedXssblock)
  ON_EN_CHANGE (IDC_HSTS,             &ServerHeadersDlg::OnEnChangeHsts)
  ON_BN_CLICKED(IDC_HSTSSUB,          &ServerHeadersDlg::OnBnClickedHstssub)
  ON_BN_CLICKED(IDC_NOSNIFF,          &ServerHeadersDlg::OnBnClickedNosniff)
  ON_BN_CLICKED(IDC_CORS,             &ServerHeadersDlg::OnBnClickedCORS)
  ON_EN_CHANGE (IDC_ALLOW_ORIGIN,     &ServerHeadersDlg::OnEnChangeAllowOrigin)
  ON_EN_CHANGE (IDC_ALLOW_HEADERS,    &ServerHeadersDlg::OnEnChangeAllowHeaders)
  ON_EN_CHANGE (IDC_ALLOW_MAXAGE,     &ServerHeadersDlg::OnEnChangeAllowMaxAge)
  ON_BN_CLICKED(IDC_ALLOW_CREDENTIALS,&ServerHeadersDlg::OnBnClickedAllowCredentials)
  ON_BN_CLICKED(IDOK,                 &ServerHeadersDlg::OnOK)
END_MESSAGE_MAP()

BOOL
ServerHeadersDlg::OnInitDialog()
{
  CDialog::OnInitDialog();

  // Warn the user
  m_warning = "All specified headers in this scope will automatically be added to **ALL** server responses\r\n"
              "This can seriously impact the server performance, so use with care!!\r\n"
              "In general: these options should only be used for responsive applications.";

  // Build the combo box contents
  m_comboXFrameOptions.AddString("");
  m_comboXFrameOptions.AddString("DENY");
  m_comboXFrameOptions.AddString("SAMEORIGIN");
  m_comboXFrameOptions.AddString("ALLOW-FROM");
  m_comboXFrameOptions.SetCurSel(0);

  // Init usage
  m_buttonUseXFrame        .SetCheck(m_config->m_useXFrameOpt);
  m_buttonUseHstsMaxAge    .SetCheck(m_config->m_useHstsMaxAge);
  m_buttonUseHstsSubDomain .SetCheck(m_config->m_useHstsDomain);
  m_buttonUseNoSniff       .SetCheck(m_config->m_useNoSniff);
  m_buttonUseXssProtection .SetCheck(m_config->m_useXssProtect);
  m_buttonUseXssBlockMode  .SetCheck(m_config->m_useXssBlock);
  m_buttonUseNoCacheControl.SetCheck(m_config->m_useNoCache);
  m_buttonUseCORS          .SetCheck(m_config->m_useCORS);

  // Init the fields
  m_XFrameURL    = m_config->m_xFrameAllowed;
  m_hstsMaxAge   = m_config->m_hstsMaxAge;
  m_allowOrigin  = m_config->m_allowOrigin;
  m_allowHeaders = m_config->m_allowHeaders;
  m_allowMaxAge  = m_config->m_allowMaxAge;

  m_buttonHstsSubDomain  .SetCheck(m_config->m_hstsSubDomain);
  m_buttonNoSniff        .SetCheck(m_config->m_xNoSniff);
  m_buttonXssProtection  .SetCheck(m_config->m_XSSProtection);
  m_buttonXssBlockMode   .SetCheck(m_config->m_XSSBlockMode);
  m_buttonNoCacheControl .SetCheck(m_config->m_noCacheControl);
  m_buttonCORS           .SetCheck(m_config->m_cors);
  m_buttonCORSCredentials.SetCheck(m_config->m_corsCredentials);

  // Set correct combo entry
  CorrectXFrameOption();

  return FALSE;
}

void
ServerHeadersDlg::CorrectXFrameOption()
{
  if(m_config->m_xFrameOption.CompareNoCase("deny") == 0)
  {
    m_comboXFrameOptions.SetCurSel(1);
    m_config->m_useXFrameAllow = false;
    m_config->m_xFrameAllowed.Empty();
  }
  if(m_config->m_xFrameOption.CompareNoCase("sameorigin") == 0)
  {
    m_comboXFrameOptions.SetCurSel(2);
    m_config->m_useXFrameAllow = false;
    m_config->m_xFrameAllowed.Empty();
  }
  if(m_config->m_xFrameOption.CompareNoCase("allow-from") == 0)
  {
    m_comboXFrameOptions.SetCurSel(3);
    m_config->m_useXFrameAllow = true;
  }
  UpdateData(FALSE);
}

bool
ServerHeadersDlg::CheckFields()
{
  if(m_config->m_xFrameOption.CompareNoCase("allow-from") == 0 &&
     m_config->m_xFrameAllowed.IsEmpty())
  {
    MessageBox("Specify an URL to allow IFRAMEd application from!","Server headers",MB_OK|MB_ICONERROR);
    return false;
  }
  if(m_config->m_XSSBlockMode && !m_config->m_XSSProtection)
  {
    MessageBox("In order to use XSS blocking mode, you must turn on XSS protection!","Server headers",MB_OK|MB_ICONERROR);
    return false;
  }
  if(m_config->m_hstsSubDomain && m_config->m_hstsMaxAge == 0)
  {
    MessageBox("In order to allow HSTS sub-domains, specify a HSTS max-age (standard value = 16070400)","Server headers",MB_OK|MB_ICONERROR);
    return false;
  }
  if(m_config->m_cors && m_config->m_allowMaxAge <= 0)
  {
    MessageBox("In order to allow CORS security, specifiy a CORS max-age (standard value = 86400 seconds)","Server headers",MB_OK | MB_ICONERROR);
    return false;
  }
  if(m_config->m_cors && m_config->m_allowOrigin.IsEmpty())
  {
    MessageBox("In order to allow CORS security, specify one (1) site as an allowed origin, or specifiy '*' (all sites)","Server headers",MB_OK | MB_ICONERROR);
    return false;
  }
  if(m_config->m_cors && m_config->m_corsCredentials && m_config->m_allowOrigin == "*")
  {
    MessageBox("In order to let credentials through in CORS situations, an explicit origin must be set. '*' cannot be used!", "Server headers", MB_OK | MB_ICONERROR);
    return false;
  }
  return true;
}

// ServerHeadersDlg message handlers

// USE HANDLERS

void 
ServerHeadersDlg::OnBnClickedUseNocache()
{
  m_config->m_useNoCache = m_buttonUseNoCacheControl.GetCheck() > 0;
  UpdateData(FALSE);
}

void 
ServerHeadersDlg::OnBnClickedUseXframe()
{
  m_config->m_useXFrameOpt = m_buttonUseXFrame.GetCheck() > 0;
  UpdateData(FALSE);
}

void 
ServerHeadersDlg::OnBnClickedUseXssprotect()
{
  m_config->m_useXssProtect = m_buttonUseXssProtection.GetCheck() > 0;
  UpdateData(FALSE);
}

void 
ServerHeadersDlg::OnBnClickedUseXssblock()
{
  m_config->m_useXssBlock = m_buttonUseXssBlockMode.GetCheck() > 0;
  UpdateData(FALSE);
}

void 
ServerHeadersDlg::OnBnClickedUseHsts()
{
  m_config->m_useHstsMaxAge = m_buttonUseHstsMaxAge.GetCheck() > 0;
  UpdateData(FALSE);
}

void 
ServerHeadersDlg::OnBnClickedUseHstssub()
{
  m_config->m_useHstsDomain = m_buttonUseHstsSubDomain.GetCheck() > 0;
  UpdateData(FALSE);
}

void 
ServerHeadersDlg::OnBnClickedUseNosniff()
{
  m_config->m_useNoSniff = m_buttonUseNoSniff.GetCheck() > 0;
  UpdateData(FALSE);
}

void
ServerHeadersDlg::OnBnClickedUseCORS()
{
  m_config->m_useCORS = m_buttonUseCORS.GetCheck() > 0;
  UpdateData(FALSE);
}

//////////////////////////////////////////////////////////////////////////
//
// VELD HANDLERS
//
//////////////////////////////////////////////////////////////////////////

void 
ServerHeadersDlg::OnBnClickedNocache()
{
  m_config->m_noCacheControl = m_buttonNoCacheControl.GetCheck() > 0;
}

void 
ServerHeadersDlg::OnCbnSelchangeXframe()
{
  int ind = m_comboXFrameOptions.GetCurSel();
  if(ind >= 0)
  {
    m_comboXFrameOptions.GetLBText(ind,m_config->m_xFrameOption);
    CorrectXFrameOption();
  }
}

void 
ServerHeadersDlg::OnEnChangeXfurl()
{
  UpdateData();
  m_config->m_xFrameAllowed = m_XFrameURL;
}

void 
ServerHeadersDlg::OnBnClickedXssprotect()
{
  m_config->m_XSSProtection = m_buttonXssProtection.GetCheck() > 0;
}

void 
ServerHeadersDlg::OnBnClickedXssblock()
{
  m_config->m_XSSBlockMode = m_buttonXssBlockMode.GetCheck() > 0;
}

void 
ServerHeadersDlg::OnEnChangeHsts()
{
  UpdateData();
  m_config->m_hstsMaxAge = m_hstsMaxAge;
}

void 
ServerHeadersDlg::OnBnClickedHstssub()
{
  m_config->m_hstsSubDomain = m_buttonHstsSubDomain.GetCheck() > 0;
}

void 
ServerHeadersDlg::OnBnClickedNosniff()
{
  m_config->m_xNoSniff = m_buttonNoSniff.GetCheck() > 0;
}

void
ServerHeadersDlg::OnBnClickedCORS()
{
  m_config->m_cors = m_buttonCORS.GetCheck() > 0;
  if(!m_config->m_cors)
  {
    m_allowOrigin.Empty();
    m_config->m_allowOrigin.Empty();
    UpdateData(FALSE);
  }
}

void
ServerHeadersDlg::OnEnChangeAllowOrigin()
{
  UpdateData();
  m_config->m_allowOrigin = m_allowOrigin;
}

void
ServerHeadersDlg::OnEnChangeAllowHeaders()
{
  UpdateData();
  m_config->m_allowHeaders = m_allowHeaders;
}

void
ServerHeadersDlg::OnEnChangeAllowMaxAge()
{
  UpdateData();
  m_config->m_allowMaxAge = m_allowMaxAge;
}

void
ServerHeadersDlg::OnBnClickedAllowCredentials()
{
  m_config->m_corsCredentials = m_buttonCORSCredentials.GetCheck();
}

void
ServerHeadersDlg::OnOK()
{
  if(CheckFields())
  {
    CDialog::OnOK();
  }
}
