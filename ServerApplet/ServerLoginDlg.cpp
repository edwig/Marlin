/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: ServerLoginDlg.cpp
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
#include "stdafx.h"
#include "ServerApplet.h"
#include "ServerLoginDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define WHITE     RGB(255,255,255)
#define DARKBLUE  RGB(0,  0,  128)
#define LIGHTBLUE RGB(0,  0,  255)

// ServerLoginDlg dialog
IMPLEMENT_DYNAMIC(ServerLoginDlg, CDialog)

ServerLoginDlg::ServerLoginDlg(CString p_titel,CString p_url,CWnd* p_parent /*=NULL*/)
               :CDialog(ServerLoginDlg::IDD,p_parent)
               ,m_titel(p_titel)
               ,m_url(p_url)
{
}

ServerLoginDlg::~ServerLoginDlg()
{
}

void ServerLoginDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
  DDX_Text   (pDX,IDC_TITEL,   m_titel);
  DDX_Text   (pDX,IDC_URL,     m_url);
  DDX_Text   (pDX,IDC_LOGLINE2,    m_user);
  DDX_Control(pDX,IDC_PASSWORD,m_passwordEdit);
}

BEGIN_MESSAGE_MAP(ServerLoginDlg, CDialog)
  ON_EN_CHANGE(IDC_URL,       &ServerLoginDlg::OnEnChangeUrl)
  ON_EN_CHANGE(IDC_LOGLINE2,      &ServerLoginDlg::OnEnChangeUser)
  ON_EN_CHANGE(IDC_PASSWORD,  &ServerLoginDlg::OnEnChangePassword)
END_MESSAGE_MAP()

BOOL
ServerLoginDlg::OnInitDialog()
{
  CDialog::OnInitDialog();

  InitPassword();

  // Set focus on the username field
  CWnd* w = GetDlgItem(IDC_LOGLINE2);
  w->SetFocus();

  return FALSE;
}

void
ServerLoginDlg::InitPassword()
{
  m_passwordEdit.SetFontSize(90);
  m_passwordEdit.SetPassword(true);
  m_passwordEdit.SetBkColor(WHITE);
  m_passwordEdit.SetBkColorEmpty(WHITE);
  m_passwordEdit.SetBorderColor        (DARKBLUE,WHITE);
  m_passwordEdit.SetBorderColorFocus   (LIGHTBLUE,WHITE);
  m_passwordEdit.SetBorderColorNonFocus(LIGHTBLUE, WHITE);
}

// ServerLoginDlg message handlers
void 
ServerLoginDlg::OnEnChangeUrl()
{
  UpdateData();
}

void 
ServerLoginDlg::OnEnChangeUser()
{
  UpdateData();
}

void 
ServerLoginDlg::OnEnChangePassword()
{
  UpdateData();
  m_passwordEdit.GetWindowText(m_password);
}
