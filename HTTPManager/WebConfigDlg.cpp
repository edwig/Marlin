/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: WebConfigDlg.cpp
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
#include "stdafx.h"
#include "HTTPManager.h"
#include "WebConfigDlg.h"
#include "WebConfig.h"
#include "afxdialogex.h"

// WebConfigDlg dialog
IMPLEMENT_DYNAMIC(WebConfigDlg, CDialogEx)

WebConfigDlg::WebConfigDlg(CWnd* pParent)
             :CDialogEx(IDD_WEBCONFIG, pParent)
             ,m_page1(this)
             ,m_page2(this)
             ,m_page3(this)
             ,m_page4(this)
             ,m_page5(this)
{
  m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

WebConfigDlg::~WebConfigDlg()
{
  if(m_webconfig)
  {
    delete m_webconfig;
  }
  m_webconfig = nullptr;
}

void WebConfigDlg::DoDataExchange(CDataExchange* pDX)
{
  CDialogEx::DoDataExchange(pDX);
  DDX_Control(pDX,IDC_TAB,  m_tab);
  DDX_Text   (pDX,IDC_TITLE,m_title);
}

BEGIN_MESSAGE_MAP(WebConfigDlg, CDialogEx)
  ON_NOTIFY(TCN_SELCHANGE,IDC_TAB,&WebConfigDlg::OnTcnSelchangeTab)
  ON_BN_CLICKED(IDOK,     &WebConfigDlg::OnBnClickedOk)
  ON_BN_CLICKED(IDCANCEL, &WebConfigDlg::OnBnClickedCancel)
END_MESSAGE_MAP()

BOOL
WebConfigDlg::OnInitDialog()
{
  CDialogEx::OnInitDialog();

  // Set the icon for this dialog.  The framework does this automatically
  //  when the application's main window is not a dialog
  SetIcon(m_hIcon,TRUE);	  // Set big icon
  SetIcon(m_hIcon,FALSE);		// Set small icon

  // InitComboboxes();

  InitTabs();
  ReadWebConfig();

  if(m_siteConfigFile.IsEmpty())
  {
    m_title = "Web.Config Editor for: web.config";
  }
  else
  {
    m_title = "Web.Config Editor for: " + m_url;
  }
  UpdateData(FALSE);
  return TRUE;
}

void
WebConfigDlg::InitTabs()
{
  m_page1.Create(IDD_WC_SERVER,        this);
  m_page2.Create(IDD_WC_CLIENT,        this);
  m_page3.Create(IDD_WC_AUTHENTICATION,this);
  m_page4.Create(IDD_WC_WSERVICES,     this);
  m_page5.Create(IDD_WC_LOGGING,       this);

  // Set titles
  m_tab.InsertItem(0, "Server");
  m_tab.InsertItem(1, "Client");
  m_tab.InsertItem(2, "Authentication");
  m_tab.InsertItem(3, "Web Services");
  m_tab.InsertItem(4, "Logging");

  // Position relative to the parent window 
  // Including the tab titlebar
  CRect rect;
  CRect itemRect;
  m_tab.GetWindowRect(&rect);
  ScreenToClient(&rect);
  m_tab.GetItemRect(1, &itemRect);
  rect.top    += itemRect.bottom + 1;
  rect.left   += 1;
  rect.right  -= 4;
  rect.bottom += itemRect.bottom - 22;

  m_page1.MoveWindow(rect,true);
  m_page2.MoveWindow(rect,false);
  m_page3.MoveWindow(rect,false);
  m_page4.MoveWindow(rect,false);
  m_page5.MoveWindow(rect,false);

  // Start first page
  m_page1.ShowWindow(SW_SHOW);
  TabCtrl_SetCurSel(m_tab.GetSafeHwnd(), 0);
}

void
WebConfigDlg::SetSiteConfig(CString p_urlPrefix,CString p_fileName)
{
  m_url            = p_urlPrefix;
  m_siteConfigFile = p_fileName;
}

void
WebConfigDlg::ReadWebConfig()
{
  if(m_webconfig == nullptr)
  {
    m_webconfig = new WebConfig(m_siteConfigFile);
  }
  m_page1.ReadWebConfig(*m_webconfig);
  m_page2.ReadWebConfig(*m_webconfig);
  m_page3.ReadWebConfig(*m_webconfig);
  m_page4.ReadWebConfig(*m_webconfig);
  m_page5.ReadWebConfig(*m_webconfig);
}

bool
WebConfigDlg::WriteWebConfig()
{
  if(m_webconfig == nullptr)
  {
    m_webconfig = new WebConfig(m_siteConfigFile);
  }

  m_page1.WriteWebConfig(*m_webconfig);
  m_page2.WriteWebConfig(*m_webconfig);
  m_page3.WriteWebConfig(*m_webconfig);
  m_page4.WriteWebConfig(*m_webconfig);
  m_page5.WriteWebConfig(*m_webconfig);

  // WRITE THE WEB.CONFIG
  if(m_webconfig->WriteConfig() == false)
  {
    CString message("Cannot write the file. Check the rights of this file: ");
    if(m_siteConfigFile.IsEmpty())
    {
      message += "web.config";
    }
    else
    {
      message += "\nFile: ";
      message += m_siteConfigFile;
    }
    MessageBox(message,"Web.Config Editor",MB_OK | MB_ICONERROR);
    return false;
  }
  return true;
}

// WebConfigDlg message handlers

void
WebConfigDlg::OnTcnSelchangeTab(NMHDR *pNMHDR,LRESULT *pResult)
{
  int num = TabCtrl_GetCurSel(pNMHDR->hwndFrom);
    
  m_page1.ShowWindow(num == 0);
  m_page2.ShowWindow(num == 1);
  m_page3.ShowWindow(num == 2);
  m_page4.ShowWindow(num == 3);
  m_page5.ShowWindow(num == 4);
}

void WebConfigDlg::OnBnClickedOk()
{
  if(WriteWebConfig())
  {
    CDialogEx::OnOK();
  }
}

void WebConfigDlg::OnBnClickedCancel()
{
  CDialogEx::OnCancel();
}

