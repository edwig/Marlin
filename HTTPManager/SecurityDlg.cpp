/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: SecurityDlg.cpp
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
#include "SecurityDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define BUFF_LEN 1024

// SecurityDlg dialog

IMPLEMENT_DYNAMIC(SecurityDlg, CDialogEx)

SecurityDlg::SecurityDlg(CWnd* pParent /*=NULL*/)
          	:CDialogEx(SecurityDlg::IDD, pParent)
            ,m_changed(false)
{
}

SecurityDlg::~SecurityDlg()
{
}

void 
SecurityDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
  DDX_Text(pDX,IDC_EDIT,m_text);

  DDX_Control(pDX,IDC_SSL20,        m_buttonServerSSL20);
  DDX_Control(pDX,IDC_SSL30,        m_buttonServerSSL30);
  DDX_Control(pDX,IDC_TLS10,        m_buttonServerTLS10);
  DDX_Control(pDX,IDC_TLS11,        m_buttonServerTLS11);
  DDX_Control(pDX,IDC_TLS12,        m_buttonServerTLS12);
  DDX_Control(pDX,IDC_SSL20_CLIENT, m_buttonClientSSL20);
  DDX_Control(pDX,IDC_SSL30_CLIENT, m_buttonClientSSL30);
  DDX_Control(pDX,IDC_TLS10_CLIENT, m_buttonClientTLS10);
  DDX_Control(pDX,IDC_TLS11_CLIENT, m_buttonClientTLS11);
  DDX_Control(pDX,IDC_TLS12_CLIENT, m_buttonClientTLS12);
}

BEGIN_MESSAGE_MAP(SecurityDlg, CDialogEx)
  ON_EN_CHANGE (IDC_EDIT,         &SecurityDlg::OnEnChangeEdit)
  ON_BN_CLICKED(IDC_SSL20,        &SecurityDlg::OnBnClickedSsl20)
  ON_BN_CLICKED(IDC_SSL30,        &SecurityDlg::OnBnClickedSsl30)
  ON_BN_CLICKED(IDC_TLS10,        &SecurityDlg::OnBnClickedTls10)
  ON_BN_CLICKED(IDC_TLS11,        &SecurityDlg::OnBnClickedTls11)
  ON_BN_CLICKED(IDC_TLS12,        &SecurityDlg::OnBnClickedTls12)
  ON_BN_CLICKED(IDC_SSL20_CLIENT, &SecurityDlg::OnBnClickedSsl20Client)
  ON_BN_CLICKED(IDC_SSL30_CLIENT, &SecurityDlg::OnBnClickedSsl30Client)
  ON_BN_CLICKED(IDC_TLS10_CLIENT, &SecurityDlg::OnBnClickedTls10Client)
  ON_BN_CLICKED(IDC_TLS11_CLIENT, &SecurityDlg::OnBnClickedTls11Client)
  ON_BN_CLICKED(IDC_TLS12_CLIENT, &SecurityDlg::OnBnClickedTls12Client)
  ON_BN_CLICKED(IDOK,             &SecurityDlg::OnBnClickedOk)
  ON_BN_CLICKED(IDCANCEL,         &SecurityDlg::OnBnClickedCancel)
END_MESSAGE_MAP()

BOOL
SecurityDlg::OnInitDialog()
{
  CDialogEx::OnInitDialog();

  m_text = "Changes the configuration of the WHOLE MS-Windows machine!!\r\n"
           "You are strongly advised **NOT** to enable SSL 2.0 any more.\r\n"
           "TLS 1.1 and TLS 1.2 are not supported by all OS'es and browsers.";
  UpdateData(FALSE);

  ReadServerOptions();
  ReadClientOptions();

  return TRUE;
}

void
SecurityDlg::ReadServerOptions()
{
  int serverSSL20 = ReadRegistry("SSL 2.0","Server","Enabled",1);
  int serverSSL30 = ReadRegistry("SSL 3.0","Server","Enabled",1);
  int serverTLS10 = ReadRegistry("TLS 1.0","Server","Enabled",1);
  int serverTLS11 = ReadRegistry("TLS 1.1","Server","Enabled",1);
  int serverTLS12 = ReadRegistry("TLS 1.2","Server","Enabled",1);

  if(ReadRegistry("SSL 2.0","Server","DisabledByDefault",0) == 1)
  {
    serverSSL20 = 0;
    m_buttonServerSSL20.EnableWindow(false);
  }
  if(ReadRegistry("SSL 3.0","Server","DisabledByDefault",0) == 1)
  {
    serverSSL30 = 0;
    m_buttonServerSSL30.EnableWindow(false);
  }

  m_buttonServerSSL20.SetCheck(serverSSL20);
  m_buttonServerSSL30.SetCheck(serverSSL30);
  m_buttonServerTLS10.SetCheck(serverTLS10);
  m_buttonServerTLS11.SetCheck(serverTLS11);
  m_buttonServerTLS12.SetCheck(serverTLS12);
}

void
SecurityDlg::ReadClientOptions()
{
  int clientSSL20 = ReadRegistry("SSL 2.0","Client","Enabled",1);
  int clientSSL30 = ReadRegistry("SSL 3.0","Client","Enabled",1);
  int clientTLS10 = ReadRegistry("TLS 1.0","Client","Enabled",1);
  int clientTLS11 = ReadRegistry("TLS 1.1","Client","Enabled",1);
  int clientTLS12 = ReadRegistry("TLS 1.2","Client","Enabled",1);

  if(ReadRegistry("SSL 2.0","Client","DisabledByDefault",0) == 1)
  {
    clientSSL20 = 0;
    m_buttonClientSSL20.EnableWindow(false);
  }
  if(ReadRegistry("SSL 3.0","Client","DisabledByDefault",0) == 1)
  {
    clientSSL30 = 0;
    m_buttonClientSSL30.EnableWindow(false);
  }

  m_buttonClientSSL20.SetCheck(clientSSL20);
  m_buttonClientSSL30.SetCheck(clientSSL30);
  m_buttonClientTLS10.SetCheck(clientTLS10);
  m_buttonClientTLS11.SetCheck(clientTLS11);
  m_buttonClientTLS12.SetCheck(clientTLS12);
}

bool
SecurityDlg::Save()
{
  if(m_changed == false)
  {
    return true;
  }
  if(::MessageBox(GetSafeHwnd()
                 ,"Are you very sure that you want to change the HTTP configuration of this machine?\n"
                  "\n"
                  "Continue ?"
                 ,"HTTPManager security"
                 ,MB_YESNO|MB_DEFBUTTON2|MB_ICONEXCLAMATION) == IDNO)
  {
    return false;
  }
 
  WriteRegistry("SSL 2.0","Server","Enabled",m_buttonServerSSL20.GetCheck() > 0);
  WriteRegistry("SSL 3.0","Server","Enabled",m_buttonServerSSL30.GetCheck() > 0);
  WriteRegistry("TLS 1.0","Server","Enabled",m_buttonServerTLS10.GetCheck() > 0);
  WriteRegistry("TLS 1.1","Server","Enabled",m_buttonServerTLS11.GetCheck() > 0);
  WriteRegistry("TLS 1.2","Server","Enabled",m_buttonServerTLS12.GetCheck() > 0);

  WriteRegistry("SSL 2.0","Client","Enabled",m_buttonClientSSL20.GetCheck() > 0);
  WriteRegistry("SSL 3.0","Client","Enabled",m_buttonClientSSL30.GetCheck() > 0);
  WriteRegistry("TLS 1.0","Client","Enabled",m_buttonClientTLS10.GetCheck() > 0);
  WriteRegistry("TLS 1.1","Client","Enabled",m_buttonClientTLS11.GetCheck() > 0);
  WriteRegistry("TLS 1.2","Client","Enabled",m_buttonClientTLS12.GetCheck() > 0);

  m_changed = false;
  ::MessageBox(GetSafeHwnd()
             ,"Your changes have made it necessary to reboot the operating system.\r\n"
              "Only after rebooting will these changes be activated!"
             ,"Warning!"
             ,MB_OK|MB_ICONASTERISK);
  return true;
}

// Reading of the MS-WIndows registry
// Only to be used for the protocol registration
int
SecurityDlg::ReadRegistry(XString p_protocol,XString p_serverClient,XString p_variable,int p_default)
{
  int waarde = p_default;
  HKEY hkUserURL;
  XString sleutel = "SYSTEM\\CurrentControlSet\\Control\\SecurityProviders\\SCHANNEL\\Protocols\\" + 
                    p_protocol + "\\" + p_serverClient;

  DWORD dwErr = RegOpenKeyEx(HKEY_LOCAL_MACHINE
                            ,(LPCSTR) sleutel
                            ,0
                            ,KEY_QUERY_VALUE
                            ,&hkUserURL);
  if(dwErr == ERROR_SUCCESS)
  {
    DWORD dwIndex    = 0;
    DWORD dwType     = 0;
    DWORD dwNameSize = 0;
    DWORD dwDataSize = 0;
    TCHAR buffName[BUFF_LEN];
    BYTE  buffData[BUFF_LEN];

    //enumerate this key's values
    while(ERROR_SUCCESS == dwErr)
    {
      dwNameSize = BUFF_LEN;
      dwDataSize = BUFF_LEN;
      dwErr = RegEnumValue(hkUserURL
                          ,dwIndex++
                          ,buffName
                          ,&dwNameSize
                          ,NULL
                          ,&dwType
                          ,buffData
                          ,&dwDataSize);
      if(dwErr == ERROR_SUCCESS && dwType == REG_DWORD)
      {
        if(p_variable.CompareNoCase(buffName) == 0)
        {
          waarde = *((DWORD*)buffData);
          break;
        }
      }
    }
    RegCloseKey(hkUserURL);
  }
  return waarde;
}

// Writing to the MS-Windows registry
void
SecurityDlg::WriteRegistry(XString p_protocol,XString p_serverClient,XString p_variable,bool p_set)
{
  HKEY    hUserKey;
  DWORD   disposition = 0;
  DWORD   value = p_set ? 1 : 0;
  DWORD   dwErr = 0;
  XString key   = "SYSTEM\\CurrentControlSet\\Control\\SecurityProviders\\SCHANNEL\\Protocols\\" + 
                    p_protocol + "\\" + p_serverClient;

  dwErr = RegCreateKeyEx(HKEY_LOCAL_MACHINE
                        ,(LPCSTR)key
                        ,0
                        ,NULL
                        ,REG_OPTION_NON_VOLATILE
                        ,KEY_WRITE
                        ,NULL
                        ,&hUserKey
                        ,&disposition);
  if(dwErr == ERROR_SUCCESS)
  {
    dwErr = RegSetValueEx(hUserKey
                         ,(LPCSTR)p_variable
                         ,0
                         ,REG_DWORD
                         ,(const BYTE*) &value
                         ,sizeof(DWORD));
  }
  if(dwErr != ERROR_SUCCESS)
  {
    XString message;
    message.Format("Cannot write registry key [%s] with value [%d]",key,value);
    ::MessageBox(GetSafeHwnd(),message,"ERROR",MB_OK|MB_ICONERROR);
  }
  RegCloseKey(hUserKey);
}

// SecurityDlg message handlers

void 
SecurityDlg::OnEnChangeEdit()
{
  UpdateData();
}

void 
SecurityDlg::OnBnClickedSsl20()
{
  m_changed = true;
}

void 
SecurityDlg::OnBnClickedSsl30()
{
  m_changed = true;
}

void 
SecurityDlg::OnBnClickedTls10()
{
  m_changed = true;
}

void 
SecurityDlg::OnBnClickedTls11()
{
  m_changed = true;
}

void 
SecurityDlg::OnBnClickedTls12()
{
  m_changed = true;
}

void 
SecurityDlg::OnBnClickedSsl20Client()
{
  m_changed = true;
}

void 
SecurityDlg::OnBnClickedSsl30Client()
{
  m_changed = true;
}

void 
SecurityDlg::OnBnClickedTls10Client()
{
  m_changed = true;
}

void 
SecurityDlg::OnBnClickedTls11Client()
{
  m_changed = true;
}

void 
SecurityDlg::OnBnClickedTls12Client()
{
  m_changed = true;
}

void 
SecurityDlg::OnBnClickedOk()
{
  if(Save())
  {
    CDialogEx::OnOK();
  }
}

void 
SecurityDlg::OnBnClickedCancel()
{
  CDialogEx::OnCancel();
}
