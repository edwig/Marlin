/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: SecureClientDlg.cpp
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
#include "WebConfigClient.h"
#include "SecureClientDlg.h"
#include "afxdialogex.h"

// SecureClientDlg dialog

IMPLEMENT_DYNAMIC(SecureClientDlg, CDialogEx)

SecureClientDlg::SecureClientDlg(CWnd* p_parent)
                :CDialogEx(SecureClientDlg::IDD, p_parent)
{
  m_config = dynamic_cast<WebConfigClient*> (p_parent);
}

SecureClientDlg::~SecureClientDlg()
{
}

void SecureClientDlg::DoDataExchange(CDataExchange* pDX)
{
  CDialogEx::DoDataExchange(pDX);

  DDX_Control(pDX,IDC_HTTP_SSL20, m_buttonSSL20);
  DDX_Control(pDX,IDC_USE_SSL20,  m_buttonUseSSL20);
  DDX_Control(pDX,IDC_HTTP_SSL30, m_buttonSSL30);
  DDX_Control(pDX,IDC_USE_SSL30,  m_buttonUseSSL30);
  DDX_Control(pDX,IDC_HTTP_TLS10, m_buttonTLS10);
  DDX_Control(pDX,IDC_USE_TLS10,  m_buttonUseTLS10);
  DDX_Control(pDX,IDC_HTTP_TLS11, m_buttonTLS11);
  DDX_Control(pDX,IDC_USE_TLS11,  m_buttonUseTLS11);
  DDX_Control(pDX,IDC_HTTP_TLS12, m_buttonTLS12);
  DDX_Control(pDX,IDC_USE_TLS12,  m_buttonUseTLS12);

  if(pDX->m_bSaveAndValidate == FALSE)
  {
    CWnd* w = nullptr;

    w = GetDlgItem(IDC_USE_SSL20);  w->EnableWindow(m_config->m_useSSL20);
    w = GetDlgItem(IDC_USE_SSL30);  w->EnableWindow(m_config->m_useSSL30);
    w = GetDlgItem(IDC_USE_TLS10);  w->EnableWindow(m_config->m_useTLS10);
    w = GetDlgItem(IDC_USE_TLS11);  w->EnableWindow(m_config->m_useTLS11);
    w = GetDlgItem(IDC_USE_TLS12);  w->EnableWindow(m_config->m_useTLS12);
  }
}

BEGIN_MESSAGE_MAP(SecureClientDlg, CDialogEx)
  ON_BN_CLICKED(IDC_HTTP_SSL20, &SecureClientDlg::OnBnClickedHttpSsl20)
  ON_BN_CLICKED(IDC_USE_SSL20,  &SecureClientDlg::OnBnClickedUseSsl20)
  ON_BN_CLICKED(IDC_HTTP_SSL30, &SecureClientDlg::OnBnClickedHttpSsl30)
  ON_BN_CLICKED(IDC_USE_SSL30,  &SecureClientDlg::OnBnClickedUseSsl30)
  ON_BN_CLICKED(IDC_HTTP_TLS10, &SecureClientDlg::OnBnClickedHttpTls10)
  ON_BN_CLICKED(IDC_USE_TLS10,  &SecureClientDlg::OnBnClickedUseTls10)
  ON_BN_CLICKED(IDC_HTTP_TLS11, &SecureClientDlg::OnBnClickedHttpTls11)
  ON_BN_CLICKED(IDC_USE_TLS11,  &SecureClientDlg::OnBnClickedUseTls11)
  ON_BN_CLICKED(IDC_HTTP_TLS12, &SecureClientDlg::OnBnClickedHttpTls12)
  ON_BN_CLICKED(IDC_USE_TLS12,  &SecureClientDlg::OnBnClickedUseTls12)
END_MESSAGE_MAP()

BOOL
SecureClientDlg::OnInitDialog()
{
  CDialogEx::OnInitDialog();

  m_buttonSSL20.SetCheck(m_config->m_useSSL20);
  m_buttonUseSSL20.SetCheck(m_config->m_ssl20);
  m_buttonSSL30.SetCheck(m_config->m_useSSL30);
  m_buttonUseSSL30.SetCheck(m_config->m_ssl30);
  m_buttonTLS10.SetCheck(m_config->m_useTLS10);
  m_buttonUseTLS10.SetCheck(m_config->m_tls10);
  m_buttonTLS11.SetCheck(m_config->m_useTLS11);
  m_buttonUseTLS11.SetCheck(m_config->m_tls11);
  m_buttonTLS12.SetCheck(m_config->m_useTLS12);
  m_buttonUseTLS12.SetCheck(m_config->m_tls12);

  UpdateData(FALSE);

  return FALSE;
}

// SecureClientDlg message handlers

void 
SecureClientDlg::OnBnClickedHttpSsl20()
{
  m_config->m_useSSL20 = m_buttonSSL20.GetCheck() > 0;
  UpdateData(FALSE);
}

void 
SecureClientDlg::OnBnClickedUseSsl20()
{
  m_config->m_ssl20 = m_buttonUseSSL20.GetCheck() > 0;
}

void 
SecureClientDlg::OnBnClickedHttpSsl30()
{
  m_config->m_useSSL30 = m_buttonSSL30.GetCheck() > 0;
  UpdateData(FALSE);
}

void 
SecureClientDlg::OnBnClickedUseSsl30()
{
  m_config->m_ssl30 = m_buttonUseSSL30.GetCheck() > 0;
}

void 
SecureClientDlg::OnBnClickedHttpTls10()
{
  m_config->m_useTLS10 = m_buttonTLS10.GetCheck() > 0;
  UpdateData(FALSE);
}

void 
SecureClientDlg::OnBnClickedUseTls10()
{
  m_config->m_tls10 = m_buttonUseTLS10.GetCheck() > 0;
}

void 
SecureClientDlg::OnBnClickedHttpTls11()
{
  m_config->m_useTLS11 = m_buttonTLS11.GetCheck() > 0;
  UpdateData(FALSE);
}

void 
SecureClientDlg::OnBnClickedUseTls11()
{
  m_config->m_tls11 = m_buttonUseTLS11.GetCheck() > 0;
}

void 
SecureClientDlg::OnBnClickedHttpTls12()
{
  m_config->m_useTLS12 = m_buttonTLS12.GetCheck() > 0;
  UpdateData(FALSE);
}

void 
SecureClientDlg::OnBnClickedUseTls12()
{
  m_config->m_tls12 = m_buttonUseTLS12.GetCheck() > 0;
}
