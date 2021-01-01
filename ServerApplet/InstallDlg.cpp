/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: InstallDlg.cpp
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
#include "InstallDlg.h"
#include "afxdialogex.h"
#include <RunRedirect.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define WHITE     RGB(255,255,255)
#define DARKBLUE  RGB(0,  0,  128)
#define LIGHTBLUE RGB(0,  0,  255)

// InstallDlg dialog
IMPLEMENT_DYNAMIC(InstallDlg, CDialog)

InstallDlg::InstallDlg(CWnd* pParent /*=NULL*/,int p_install)
           :CDialog(InstallDlg::IDD, pParent)
           ,m_install(p_install)
{
}

InstallDlg::~InstallDlg()
{
}

void InstallDlg::DoDataExchange(CDataExchange* pDX)
{
  CDialog::DoDataExchange(pDX);
  DDX_Text   (pDX,IDC_DOMEIN,    m_domain);
  DDX_Text   (pDX,IDC_GEBRUIKER, m_username);
  DDX_Control(pDX,IDC_WACHTWOORD,m_editPassword);
  DDX_Control(pDX,IDC_BEVESTIG,  m_editConfirm);

  DDX_Control(pDX,IDC_INSTALLEER,m_buttonInstall);
  DDX_Control(pDX,IDC_VERWIJDER, m_buttonRemove);
}

BEGIN_MESSAGE_MAP(InstallDlg, CDialog)
  ON_WM_TIMER()
  ON_EN_CHANGE (IDC_DOMEIN,    &InstallDlg::OnEnChangeDomain)
  ON_EN_CHANGE (IDC_GEBRUIKER, &InstallDlg::OnEnChangeUsername)
  ON_EN_CHANGE (IDC_WACHTWOORD,&InstallDlg::OnEnChangePassword)
  ON_EN_CHANGE (IDC_BEVESTIG,  &InstallDlg::OnEnChangeConfirm)
  ON_BN_CLICKED(IDC_INSTALLEER,&InstallDlg::OnBnClickedInstall)
  ON_BN_CLICKED(IDC_VERWIJDER, &InstallDlg::OnBnClickedRemove)
END_MESSAGE_MAP()

BOOL
InstallDlg::OnInitDialog()
{
  CDialog::OnInitDialog();

  InitVelden();
  UpdateData(FALSE);
  if(m_install == 1)
  {
    m_buttonRemove.EnableWindow(FALSE);
  }
  else if(m_install == 2)
  {
    m_buttonInstall.EnableWindow(FALSE);
    RemoveService(false);
  }
  CenterWindow();
  SetWindowPos(&CWnd::wndTopMost,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_SHOWWINDOW|SWP_NOCOPYBITS);
  SetTimer(1,1000,NULL);

  return FALSE;
}

void
InstallDlg::OnTimer(UINT_PTR nIDEvent)
{
  if(nIDEvent == 1)
  {
    KillTimer(1);
    SetWindowPos(&CWnd::wndNoTopMost,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_SHOWWINDOW|SWP_NOCOPYBITS);
  }
}

void
InstallDlg::InitVelden()
{
  // Password
  m_editPassword.SetPassword(true);
  m_editPassword.SetBkColor(WHITE);
  m_editPassword.SetBkColorEmpty(WHITE);
  m_editPassword.SetBorderColor        (DARKBLUE, WHITE);
  m_editPassword.SetBorderColorFocus   (LIGHTBLUE,WHITE);
  m_editPassword.SetBorderColorNonFocus(LIGHTBLUE,WHITE);
  // Confirmation field
  m_editConfirm.SetPassword(true);
  m_editConfirm.SetBkColor(WHITE);
  m_editConfirm.SetBkColorEmpty(WHITE);
  m_editConfirm.SetBorderColor        (DARKBLUE, WHITE);
  m_editConfirm.SetBorderColorFocus   (LIGHTBLUE,WHITE);
  m_editConfirm.SetBorderColorNonFocus(LIGHTBLUE,WHITE);
}

// InstallDlg message handlers

void 
InstallDlg::OnEnChangeDomain()
{
  UpdateData();
}

void 
InstallDlg::OnEnChangeUsername()
{
  UpdateData();
}

void 
InstallDlg::OnEnChangePassword()
{
  UpdateData();
  m_editPassword.GetWindowText(m_password);
}

void
InstallDlg::OnEnChangeConfirm()
{
  UpdateData();
  m_editConfirm.GetWindowText(m_confirm);
}

void 
InstallDlg::OnBnClickedInstall()
{
  if(m_username.IsEmpty() || m_password.IsEmpty())
  {
    ::MessageBox(GetSafeHwnd(),"U need to fill in the USER and the PASSWORD fields","ERROR",MB_OK);
    return;
  }

  if(m_password.Compare(m_confirm))
  {
    ::MessageBox(GetSafeHwnd(),"The two password fields do not match!","ERROR",MB_OK);
    return;
  }

  if(::MessageBox(GetSafeHwnd()
                 ,"Are you certain that you wish to install the NT service?"
                 ,"Be Sure!"
                 ,MB_YESNO|MB_DEFBUTTON2|MB_ICONQUESTION) == IDYES) 
  {
    CString result;
    CString arguments;
    CString program(CString(PRODUCT_NAME) + ".exe");
    CString error("Cannot start the server application for a NT-Service installation");

    // Format the command
    CString user(m_username);
    if(!m_domain.IsEmpty())
    {
      user = m_domain + "\\" + m_username;
    }
    arguments.Format("install %s %s",user.GetString(),m_password.GetString());
    int res = CallProgram_For_String(program,arguments,result);
    if(res)
    {
      CString melding;
      melding.Format("Installing the NT-Service failed. Error code: %d\n\n",res);
      melding += result;
      ::MessageBox(GetSafeHwnd(),melding,"ERROR",MB_OK|MB_ICONERROR);
    }
    else
    {
      CDialog::EndDialog(IDOK);
    }
  }
}

void 
InstallDlg::OnBnClickedRemove()
{
  if(::MessageBox(GetSafeHwnd()
                ,"Are you sure you want to remove the NT-Service definition?"
                ,"Be Sure?"
                ,MB_YESNO|MB_DEFBUTTON2|MB_ICONQUESTION) == IDYES) 
  {
    RemoveService(true);
  }
}

void
InstallDlg::RemoveService(bool p_tonen)
{
  CString result;
  CString error("Cannot stop the NT-service");
  CString arguments("stop");
  CString program(CString(PRODUCT_NAME) + ".exe");

  // First: stop the service
  int res = CallProgram_For_String(program,arguments,result);
  if(p_tonen)
  {
    if(res)
    {
      CString melding;
      melding.Format("Stopping the NT-Service has failed. Error code: %d\n\n",res);
      melding += result;
      ::MessageBox(GetSafeHwnd(),melding,"ERROR",MB_OK | MB_ICONERROR);
    }
  }

  // Now de-install
  arguments = "uninstall";
  error = "Cannot stop the server before de-installation";
  res = CallProgram_For_String(program,arguments,result);
  if(p_tonen)
  {
    if(res)
    {
      CString message;
      message.Format("Removing the NT-Service definition has failed. Error code: %d\n\n",res);
      message += result;
      ::MessageBox(GetSafeHwnd(),message,"ERROR",MB_OK|MB_ICONERROR);
    }
    else
    {
      CDialog::EndDialog(IDOK);
    }
  }
  else
  {
    // Always ending the dialog
    CDialog::EndDialog(IDOK);
  }
}

