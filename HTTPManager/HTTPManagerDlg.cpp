/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: HTTPManagerDlg.cpp
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
#include "HTTPManagerDlg.h"
#include "RunRedirect.h"
#include "ResultDlg.h"
#include "MarlinConfig.h"
#include "WebConfigDlg.h"
#include "HTTPServer.h"
#include "SecurityDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// CAboutDlg dialog used for App About

class CAboutDlg : public CDialogEx
{
public:
  CAboutDlg();

// Dialog Data
  enum { IDD = IDD_ABOUTBOX };

  protected:
  virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
  DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
  CDialogEx::DoDataExchange(pDX);

  CWnd* w = GetDlgItem(IDC_VERSION);
  if(w)
  {
    w->SetWindowText(_T(MARLIN_SERVER_VERSION) _T("\r\n")
                     _T("Date: ") _T(MARLIN_VERSION_DATE) );
  }
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()

// HTTPManagerDlg dialog
HTTPManagerDlg::HTTPManagerDlg(bool p_iis,CWnd* pParent /*=NULL*/)
               :CDialogEx(HTTPManagerDlg::IDD, pParent)
               ,m_iis(p_iis)
{
  m_hIcon    = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
  m_secure   = false;
  m_binding  = PrefixType::URLPRE_Strong;
  m_port     = 80;
  m_portUpto = 80;
  m_doRange  = false;
  m_version  = OSVERSIE_UNKNOWN;
}

HTTPManagerDlg::~HTTPManagerDlg()
{
  // Shut up SonarQube
}

void HTTPManagerDlg::DoDataExchange(CDataExchange* pDX)
{
  CDialogEx::DoDataExchange(pDX);

  DDX_Control(pDX,IDC_PROTOCOL,     m_comboProtocol);
  DDX_Control(pDX,IDC_SECURITY,     m_buttonSecurity);
  DDX_Control(pDX,IDC_BINDING,      m_comboBinding);
  DDX_Control(pDX,IDC_PORT,         m_editPort);
  DDX_Control(pDX,IDC_RANGE,        m_buttonRange);
  DDX_Control(pDX,IDC_PORT2,        m_editPortUpto);
  DDX_Control(pDX,IDC_ABSPATH,      m_editPath);
  DDX_Control(pDX,IDC_ASKURL,       m_buttonAskURL);
  DDX_Control(pDX,IDC_ADDURL,       m_buttonCreate);
  DDX_Control(pDX,IDC_DELURL,       m_buttonRemove);
  DDX_Control(pDX,IDC_ASK_FIREWALL, m_buttonAskFWR);
  DDX_Control(pDX,IDC_ADD_FIREWALL, m_buttonCreateFWR);
  DDX_Control(pDX,IDC_DEL_FIREWALL, m_buttonRemoveFWR);
  DDX_Control(pDX,IDC_CERTSTORE,    m_comboStore);
  DDX_Control(pDX,IDC_CLIENTCERT,   m_buttonClientCert);
  DDX_Control(pDX,IDC_LISTNER,      m_buttonListener);
  DDX_Control(pDX,IDC_LISTEN,       m_buttonListen);

  DDX_Control(pDX,IDC_THUMBPRINT,   m_editCertificate);
  DDX_Control(pDX,IDC_ASKCERT,      m_buttonAskCERT);
  DDX_Control(pDX,IDC_ADDCERT,      m_buttonConnect);
  DDX_Control(pDX,IDC_DELCERT,      m_buttonDisconnect);

  DDX_Control(pDX,IDC_RESULT,       m_editStatus);
  DDX_Control(pDX,IDC_WEBCONFIG,    m_buttonWebConfig);
  DDX_Control(pDX,IDC_SITECONFIG,   m_buttonSiteConfig);

  if(pDX->m_bSaveAndValidate == FALSE)
  {
    // Activate certificate buttons
    m_comboStore      .EnableWindow(m_secure && !m_iis);
    m_buttonClientCert.EnableWindow(m_secure && !m_iis);
    m_editCertificate .EnableWindow(m_secure && !m_iis);
    m_buttonAskCERT   .EnableWindow(m_secure && !m_iis);
    m_buttonConnect   .EnableWindow(m_secure && !m_iis);
    m_buttonDisconnect.EnableWindow(m_secure && !m_iis);
    m_buttonSecurity  .EnableWindow(m_secure);

    XString prefix = _T("URL Prefix: ") + CreateURLPrefix(m_binding,m_secure,m_port,m_absPath);
    m_editStatus.SetWindowText(prefix);

    m_editPortUpto.EnableWindow(m_doRange);

    // Enable button for site config
    CWnd* w = GetDlgItem(IDC_SITECONFIG);
    if(w)
    {
      w->EnableWindow(!m_absPath.IsEmpty());
    }
  }
}

BEGIN_MESSAGE_MAP(HTTPManagerDlg, CDialogEx)
  ON_WM_SYSCOMMAND()
  ON_WM_PAINT()
  ON_WM_QUERYDRAGICON()
  ON_CBN_SELCHANGE(IDC_PROTOCOL,    &HTTPManagerDlg::OnCbnSelchangeProtocol)
  ON_BN_CLICKED   (IDC_SECURITY,    &HTTPManagerDlg::OnBnClickedSecurity)
  ON_CBN_SELCHANGE(IDC_BINDING,     &HTTPManagerDlg::OnCbnSelchangeBinding)
  ON_EN_CHANGE    (IDC_PORT,		    &HTTPManagerDlg::OnEnChangePort)
  ON_EN_KILLFOCUS (IDC_PORT2,       &HTTPManagerDlg::OnEnChangePortUpto)
  ON_BN_CLICKED   (IDC_RANGE,       &HTTPManagerDlg::OnBnClickedRange)
  ON_EN_CHANGE    (IDC_ABSPATH,     &HTTPManagerDlg::OnEnChangeAbspath)
  ON_BN_CLICKED   (IDC_ASKURL,      &HTTPManagerDlg::OnBnClickedAskurl)
  ON_BN_CLICKED   (IDC_ADDURL,      &HTTPManagerDlg::OnBnClickedAddurl)
  ON_BN_CLICKED   (IDC_DELURL,      &HTTPManagerDlg::OnBnClickedDelurl)
  ON_BN_CLICKED   (IDC_ASK_FIREWALL,&HTTPManagerDlg::OnBnClickedAskFW)
  ON_BN_CLICKED   (IDC_ADD_FIREWALL,&HTTPManagerDlg::OnBnClickedAddFW)
  ON_BN_CLICKED   (IDC_DEL_FIREWALL,&HTTPManagerDlg::OnBnClickedDelFW)
  ON_EN_CHANGE    (IDC_THUMBPRINT,  &HTTPManagerDlg::OnEnChangeThumbprint)
  ON_BN_CLICKED   (IDC_ASKCERT,     &HTTPManagerDlg::OnBnClickedAskcert)
  ON_BN_CLICKED   (IDC_ADDCERT,     &HTTPManagerDlg::OnBnClickedAddcert)
  ON_BN_CLICKED   (IDC_DELCERT,     &HTTPManagerDlg::OnBnClickedDelcert)
  ON_BN_CLICKED   (IDC_LISTNER,     &HTTPManagerDlg::OnBnClickedListner)
  ON_BN_CLICKED   (IDC_LISTEN,      &HTTPManagerDlg::OnBnClickedListen)
  ON_BN_CLICKED   (IDC_NETSTAT,     &HTTPManagerDlg::OnBnClickedNetstat)
  ON_BN_CLICKED   (IDOK,            &HTTPManagerDlg::OnBnClickedOk)
  ON_BN_CLICKED   (IDCANCEL,        &HTTPManagerDlg::OnBnClickedCancel)
  ON_BN_CLICKED   (IDC_WEBCONFIG,   &HTTPManagerDlg::OnBnClickedWebconfig)
  ON_BN_CLICKED   (IDC_SITECONFIG,  &HTTPManagerDlg::OnBnClickedSiteWebConfig)
END_MESSAGE_MAP()


// HTTPManagerDlg message handlers

BOOL
HTTPManagerDlg::OnInitDialog()
{
  CDialogEx::OnInitDialog();

  // Add "About..." menu item to system menu.

  // IDM_ABOUTBOX must be in the system command range.
  ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
  ASSERT(IDM_ABOUTBOX < 0xF000);

  CMenu* pSysMenu = GetSystemMenu(FALSE);
  if (pSysMenu != NULL)
  {
    BOOL bNameValid;
    XString strAboutMenu;
    bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
    ASSERT(bNameValid);
    if (!strAboutMenu.IsEmpty())
    {
      pSysMenu->AppendMenu(MF_SEPARATOR);
      pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
    }
  }

  // Set the icon for this dialog.  The framework does this automatically
  //  when the application's main window is not a dialog
  SetIcon(m_hIcon, TRUE);			// Set big icon
  SetIcon(m_hIcon, FALSE);		// Set small icon

  // Getting the MS-Windows version
  SetVersion();

  // Protocol types
  m_comboProtocol.AddString(_T("http://"));
  m_comboProtocol.AddString(_T("https://"));
  m_comboProtocol.SetCurSel(0); 
  m_secure = false;

  m_comboBinding.AddString(_T("Strong (+)"));
  m_comboBinding.AddString(_T("Short name"));
  m_comboBinding.AddString(_T("Full DNS name"));
  m_comboBinding.AddString(_T("IP address"));
  m_comboBinding.AddString(_T("Weak (*)"));
  m_comboBinding.SetCurSel(0);
  m_binding = PrefixType::URLPRE_Strong;

  m_comboStore.AddString(_T("TrustedPublisher"));
  m_comboStore.AddString(_T("MY"));
  m_comboStore.AddString(_T("AuthRoot"));
  m_comboStore.AddString(_T("TrustedPeople"));
  m_comboStore.AddString(_T("Root"));
  m_comboStore.SetCurSel(0);

  m_buttonClientCert.SetCheck(FALSE);

  XString text;
  text.Format(_T("%d"),m_port);
  m_editPort.SetWindowText(text);
  text.Format(_T("%d"),m_portUpto);
  m_editPortUpto.SetWindowText(text);

  // Do the IIS thing
  if(m_iis)
  {
    ConfigureForIIS();
    SetWindowText(_T("HTTP Manager (Mode: IIS)"));
  }
  else
  {
    SetWindowText(_T("HTTP Manager (Mode: Standalone)"));
  }

  // Setting the values on screen
  UpdateData(FALSE);

  return TRUE;  // return TRUE  unless you set the focus to a control
}

#pragma warning(push)
#pragma warning(disable: 4996)
void
HTTPManagerDlg::SetVersion()
{
  OSVERSIONINFOEX version;
  version.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
  GetVersionEx((LPOSVERSIONINFO)&version);

  if(version.dwMajorVersion <= 5)
  {
    m_version = OSVERSIE_SERVER2003;
    ::MessageBox(GetSafeHwnd()
               ,_T("HTTPManager is configured for 'Windows Server 2003' and will use HTTPCFG.EXE")
                _T("BEWARE: You need to install httpcfg.exe on your system by hand!")
               ,_T("Warning")
               ,MB_OK|MB_ICONWARNING);
  }
  else
  {
    // version.dwMajorVersion 6 or greater = Vista / Windows 7 / 8
    m_version = OSVERSIE_VISTA_UP;
  }
}
#pragma warning(pop)

void
HTTPManagerDlg::ConfigureForIIS()
{
  // IP Listener
  m_buttonListener.EnableWindow(FALSE);
  // Listen on IP
  m_buttonListen.EnableWindow(FALSE);
  // Create URLACL reservation
  m_buttonCreate.EnableWindow(FALSE);
  // Server WEB.CONFIG
  m_buttonWebConfig.SetWindowText(_T("Server Marlin.Config"));
  // Site Web.Config
  m_buttonSiteConfig.SetWindowText(_T("Site Marlin.Config"));
}

void 
HTTPManagerDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
  if ((nID & 0xFFF0) == IDM_ABOUTBOX)
  {
    CAboutDlg dlgAbout;
    dlgAbout.DoModal();
  }
  else
  {
    CDialogEx::OnSysCommand(nID, lParam);
  }
}

// If you add a minimize button to your dialog, you will need the code below
// to draw the icon.  For MFC applications using the document/view model,
// this is automatically done for you by the framework.

void 
HTTPManagerDlg::OnPaint()
{
  if (IsIconic())
  {
    CPaintDC dc(this); // device context for painting

    SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

    // Center icon in client rectangle
    int cxIcon = GetSystemMetrics(SM_CXICON);
    int cyIcon = GetSystemMetrics(SM_CYICON);
    CRect rect;
    GetClientRect(&rect);
    int x = (rect.Width() - cxIcon + 1) / 2;
    int y = (rect.Height() - cyIcon + 1) / 2;

    // Draw the icon
    dc.DrawIcon(x, y, m_hIcon);
  }
  else
  {
    CDialogEx::OnPaint();
  }
}

void
HTTPManagerDlg::CheckPortRange()
{
  if(m_doRange)
  {
    if(m_portUpto == 0 || m_portUpto < m_port)
    {
      m_portUpto = m_port;
      XString text;
      text.Format(_T("%d"),m_portUpto);
      m_editPortUpto.SetWindowText(text);
      UpdateData(FALSE);
    }

    if(m_portUpto > (m_port + 200))
    {
      if(::MessageBox(GetSafeHwnd(),_T("You have selected more than 200 client ports!\nAre you sure?"),_T("HTTP Manager"),MB_YESNO|MB_DEFBUTTON2|MB_ICONQUESTION) == IDNO)
      {
        m_portUpto = m_port + 200;
        XString text;
        text.Format(_T("%d"),m_portUpto);
        m_editPortUpto.SetWindowText(text);
        UpdateData(FALSE);
      }
    }
  }
}

void
HTTPManagerDlg::CheckPathname(bool p_allow)
{
  // Check the absolute path
  if(m_absPath.IsEmpty())
  {
    ::MessageBox(GetSafeHwnd(),_T("An empty URL is not allowed"),_T("ERROR"),MB_OK | MB_ICONERROR);
    m_absPath = _T("/");
    UpdateData(FALSE);
  }
}

XString
HTTPManagerDlg::MakeFirewallRuleName(XString& p_ports)
{
  CheckPathname();

  XString naam(m_absPath);
  XString ports;

  // Make sure the range is 1 long
  if(!m_doRange)
  {
    m_portUpto = m_port;
    ports.Format(_T("%d"),m_port);
  }
  else
  {
    CheckPortRange();
    ports.Format(_T("%d-%d"),m_port,m_portUpto);
  }

  // Create a nice name from the URL and the port number
  if(naam.Left(1)  == _T("/")) naam = naam.Mid(1);
  if(naam.Right(1) == _T("/")) naam = naam.Left(naam.GetLength() - 1);
  naam += _T("_");
  naam += ports;
  naam.Replace(_T("/"),_T("_"));
  naam.Replace(_T("-"),_T("_"));

  // results
  p_ports = ports;
  return naam;
}

bool
HTTPManagerDlg::DoCommand(ConfigCmd p_config
                         ,XString   p_prefix
                         ,XString&  p_command
                         ,XString&  p_parameters
                         ,XString   p_prefix2
                         ,XString   p_prefix3
                         ,XString   p_prefix4)
{
  bool result = true;
  XString temp;

  // Get the command
  if(m_version == OSVERSIE_SERVER2003)
  {
    p_command = _T("httpcfg.exe");
    switch(p_config)
    {
      case CONFIG_ASKURL: p_parameters.Format(_T("query urlacl -u %s"),p_prefix);
                          break;
      case CONFIG_ADDURL: p_parameters.Format(_T("set urlacl -u %s -a D:(A;;GX;;;WD)"),p_prefix);
                          break;
      case CONFIG_DELURL: p_parameters.Format(_T("delete urlacl -u %s"),p_prefix);
                          break;
      case CONFIG_ASKSSL: p_parameters.Format(_T("query ssl -i 0.0.0.0:%s"),p_prefix);
                          break;
      case CONFIG_ADDSSL: p_parameters.Format(_T("set ssl -i 0.0.0.0:%s -h \"%s\" ")
                                              _T("-g {00112233-4455-6677-8899-AABBCCDDEEFF} -c %s")
                                             ,p_prefix,p_prefix2,p_prefix3);
                          break;
      case CONFIG_DELSSL: p_parameters.Format(_T("delete ssl -i 0.0.0.0:%s"),p_prefix);
                          break;
      case CONFIG_ASKLIST:p_parameters = _T("query iplisten");
                          break;
      case CONFIG_ADDLIST:p_parameters = _T("set iplisten -i 0.0.0.0");
                          break;
      default:            result = false;
                          break;
    }
  }
  else if(m_version == OSVERSIE_VISTA_UP)
  {
    p_command = _T("netsh.exe");
    switch(p_config)
    {
      case CONFIG_ASKURL: p_parameters.Format(_T("http show urlacl url=%s"),p_prefix);
                          break;
      case CONFIG_ADDURL: p_parameters.Format(_T("http add urlacl url=%s sddl=D:(A;;GX;;;WD)"),p_prefix);
                          break;
      case CONFIG_DELURL: p_parameters.Format(_T("http delete urlacl url=%s"),p_prefix);
                          break;
      case CONFIG_ASKSSL: p_parameters.Format(_T("http show sslcert ipport=0.0.0.0:%s"),p_prefix);
                          break;
      case CONFIG_ADDSSL: p_parameters.Format(_T("http add sslcert ipport=0.0.0.0:%s certhash=%s ")
                                              _T("appid={00112233-4455-6677-8899-AABBCCDDEEFF} certstorename=%s ")
                                              _T("clientcertnegotiation=%s")
                                              ,p_prefix,p_prefix2,p_prefix3,p_prefix4);
                          break;
      case CONFIG_DELSSL: p_parameters.Format(_T("http delete sslcert ipport=0.0.0.0:%s"),p_prefix);
                          break;
      case CONFIG_ASKLIST:p_parameters = _T("http show iplisten");
                          break;
      case CONFIG_ADDLIST:p_parameters = _T("http add iplisten ipaddress=0.0.0.0");
                          break;
      default:            result = false;
                          break;
    }
  }
  else
  {
    result = false;
  }
  // Check for an error
  if(!result)
  {
    ::MessageBox(GetSafeHwnd(),_T("INTERNAL ERROR: Incorrect configuration command"),_T("ERROR"),MB_OK);
  }
  return result;
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR 
HTTPManagerDlg::OnQueryDragIcon()
{
  return static_cast<HCURSOR>(m_hIcon);
}

// Update the protocol
void 
HTTPManagerDlg::OnCbnSelchangeProtocol()
{
  int ind = m_comboProtocol.GetCurSel();
  m_secure = (ind == 1);
  UpdateData(FALSE);
}

void 
HTTPManagerDlg::OnCbnSelchangeBinding()
{
  int ind = m_comboBinding.GetCurSel();
  if(ind >= 0 && ind <= 4)
  {
    m_binding = (PrefixType) ind;
  }
  UpdateData(FALSE);
}

void 
HTTPManagerDlg::OnEnChangePort()
{
  XString text;
  m_editPort.GetWindowText(text);
  m_port = _ttoi(text);
  UpdateData(FALSE);
  CheckPortRange();
}

void 
HTTPManagerDlg::OnEnChangePortUpto()
{
  XString text;
  m_editPortUpto.GetWindowText(text);
  m_portUpto = _ttoi(text);
  UpdateData(FALSE);
  CheckPortRange();
}

void
HTTPManagerDlg::OnBnClickedRange()
{
  m_doRange = m_buttonRange.GetCheck() == TRUE;
  UpdateData(FALSE);
  CheckPortRange();
}

void 
HTTPManagerDlg::OnEnChangeAbspath()
{
  m_editPath.GetWindowText(m_absPath);
  UpdateData(FALSE);
}

void
HTTPManagerDlg::OnBnClickedAskurl()
{
  XString show;
  XString result;
  XString parameters;
  XString command;
  CWaitCursor deep_sigh;

  CheckPathname(true);

  // Make sure the range is 1 long
  if(!m_doRange)
  {
    m_portUpto = m_port;
  }
  else
  {
    CheckPortRange();
  }

  // Do all the ports
  for(int port = m_port;port <= m_portUpto;++port)
  {
    XString prefix = CreateURLPrefix(m_binding,m_secure,port,m_absPath);
    m_editStatus.SetWindowText(_T("Inquiring for URL reservation: ") + prefix);
    MessagePump();

    if(DoCommand(CONFIG_ASKURL,prefix,command,parameters))
    {
      CallProgram_For_String(command,parameters,result);
      show   += XString(_T(">")) + command + _T(" ") + parameters + _T("\r\n") + result;
    }
    else return;
  }

  // Show result
  ResultDlg dlg(this,show);
  dlg.DoModal();

  // Reset status line
  UpdateData(FALSE);
}

void 
HTTPManagerDlg::OnBnClickedAddurl()
{
  // Create command line
  XString show;
  XString result;
  XString parameters;
  XString command;
  CWaitCursor deep_sigh;

  CheckPathname(true);

  // Make sure the range is 1 long
  if(!m_doRange)
  {
    m_portUpto = m_port;
  }
  else
  {
    CheckPortRange();
  }

  // Do all the ports
  for(int port = m_port;port <= m_portUpto;++port)
  {
    XString prefix  = CreateURLPrefix(m_binding,m_secure,port,m_absPath);
    m_editStatus.SetWindowText(_T("Adding URL reservation: ") + prefix);
    MessagePump();

    if(DoCommand(CONFIG_ADDURL,prefix,command,parameters))
    {
      CallProgram_For_String(command,parameters,result);
      show   += _T(">") + command + _T(" ") + parameters + _T("\r\n") + result + _T("\r\n");
    }
    else return;

    if(DoCommand(CONFIG_ASKURL,prefix,command,parameters))
    {
      CallProgram_For_String(command,parameters,result);
      show   += _T(">") + command + _T(" ") + parameters + _T("\r\n") + result;
    }
    else return;
  }

  // Show result
  ResultDlg dlg(this,show);
  dlg.DoModal();

  // Reset status line
  UpdateData(FALSE);
}

void 
HTTPManagerDlg::OnBnClickedDelurl()
{
  // Create command line
  XString show;
  XString result;
  XString parameters;
  XString command;
  CWaitCursor deep_sigh;

  CheckPathname(true);

  // Make sure the range is 1 long
  if(!m_doRange)
  {
    m_portUpto = m_port;
  }
  else
  {
    CheckPortRange();
  }

  for(int port = m_port;port <= m_portUpto;++port)
  {
    XString prefix  = CreateURLPrefix(m_binding,m_secure,port,m_absPath);
    m_editStatus.SetWindowText(_T("Deleting URL reservation: ") + prefix);
    MessagePump();

    if(DoCommand(CONFIG_DELURL,prefix,command,parameters))
    {
      CallProgram_For_String(command,parameters,result);
      show   += _T(">") + command + _T(" ") + parameters + _T("\r\n") + result;
    }
    else return;
  }

  // Show result
  ResultDlg dlg(this,show);
  dlg.DoModal();

  // Reset status line
  UpdateData(FALSE);
}

void
HTTPManagerDlg::OnBnClickedAskFW()
{
  XString show;
  XString result;
  XString command(_T("netsh"));
  XString parameters;
  XString ports;
  CWaitCursor diepe_zucht;

  XString naam = MakeFirewallRuleName(ports);
  parameters.Format(_T("advfirewall firewall show rule name=\"%s\" verbose"), naam);

  m_editStatus.SetWindowText(_T("Inquiring for Firewall rules for: ") + naam);
  MessagePump();

  CallProgram_For_String(command,parameters,result);
  show += XString(_T(">")) + command + _T(" ") + parameters + _T("\r\n") + result;

  if(m_rulesResult == _T("TEST"))
  {
    m_rulesResult = result;
    return;
  }

  // Show result
  ResultDlg dlg(this,show);
  dlg.DoModal();

  // Reset status line
  UpdateData(FALSE);
}

void
HTTPManagerDlg::OnBnClickedAddFW()
{
  XString ports;
  XString show;
  XString result;
  XString command(_T("netsh"));
  XString parameters1;
  XString parameters2;
  CWaitCursor diepe_zucht;

  // See if we must make a firewall rule
  m_rulesResult = _T("TEST");
  OnBnClickedAskFW();
  if((m_rulesResult.Left(10) != _T("\r\nNo rules")) &&
     (m_rulesResult.Left(21) != _T("\r\nEr zijn geen regels"))) // DUTCH OS
  {
    ::MessageBox(GetSafeHwnd(),_T("There already exists a Firewall-Rule for this combination of URL/Port"),_T("ERROR"),MB_OK|MB_ICONERROR);
    m_rulesResult.Empty();
    return;
  }
  m_rulesResult.Empty();

  XString naam = MakeFirewallRuleName(ports);
  parameters1.Format(_T("advfirewall firewall add rule name=\"%s\" ")
                     _T("dir=in protocol=TCP localport=%s edge=yes action=allow profile=any")
                    ,naam,ports);
  parameters2.Format(_T("advfirewall firewall add rule name=\"%s\" ")
                     _T("dir=out protocol=TCP localport=%s action=allow profile=any")
                     ,naam,ports);

  m_editStatus.SetWindowText(_T("Creating Firewall rules for: ") + naam);
  MessagePump();

  CallProgram_For_String(command,parameters1,result);
  show += XString(_T(">")) + command + _T(" ") + parameters1 + _T("\r\n") + result;

  CallProgram_For_String(command,parameters2,result);
  show += XString(_T(">")) + command + _T(" ") + parameters2 + _T("\r\n") + result;

  // Show result
  ResultDlg dlg(this,show);
  dlg.DoModal();

  // Reset status line
  UpdateData(FALSE);
}

void
HTTPManagerDlg::OnBnClickedDelFW()
{
  XString show;
  XString result;
  XString command(_T("netsh"));
  XString parameters;
  XString ports;
  CWaitCursor diepe_zucht;

  XString naam = MakeFirewallRuleName(ports);
  parameters.Format(_T("advfirewall firewall delete rule name=\"%s\""),naam);

  m_editStatus.SetWindowText(_T("Removing Firewall rules for: ") + naam);
  MessagePump();

  CallProgram_For_String(command,parameters,result);
  show += XString(_T(">")) + command + _T(" ") + parameters + _T("\r\n") + result;

  // Show result
  ResultDlg dlg(this,show);
  dlg.DoModal();

  // Reset status line
  UpdateData(FALSE);
}

void 
HTTPManagerDlg::OnEnChangeThumbprint()
{
  UpdateData();
}

void 
HTTPManagerDlg::OnBnClickedAskcert()
{
  XString show;
  XString result;
  XString parameters;
  XString prefix;
  XString command;
  CWaitCursor diepe_zucht;

  // Make sure the range is 1 long
  if(!m_doRange)
  {
    m_portUpto = m_port;
  }
  else
  {
    CheckPortRange();
  }

  for(int port = m_port;port <= m_portUpto;++port)
  {
    // See if it has been added
    result.Format(_T("Checking certificates for port: %d"),port);
    m_editStatus.SetWindowText(result);
    MessagePump();

    prefix.Format(_T("%d"),port);
    if(DoCommand(CONFIG_ASKSSL,prefix,command,parameters))
    {
      CallProgram_For_String(command,parameters,result);
      show   += XString(_T(">")) + command + _T(" ") + parameters + _T("\r\n") + result;
    }
    else return;
  }

  // Show result
  ResultDlg dlg(this,show);
  dlg.DoModal();
}


void 
HTTPManagerDlg::OnBnClickedAddcert()
{
  XString showme;
  XString result;
  XString prefix;
  XString parameters;
  XString command;
  XString certificate;
  XString storename(_T("TrustedPublisher"));
  XString clientCert;
  m_editCertificate.GetWindowText(certificate);
  certificate.Replace(_T(" "),_T(""));
  certificate = certificate.TrimLeft(_T("?"));
  CWaitCursor diepe_zucht;

  // Getting the storename
  m_comboStore.GetLBText(m_comboStore.GetCurSel(),storename);
  // Getting the client certification
  clientCert = m_buttonClientCert.GetCheck() ? _T("enable") : _T("disable");

  // Make sure the range is 1 long
  if(!m_doRange)
  {
    m_portUpto = m_port;
  }
  else
  {
    CheckPortRange();
  }

  for(int port = m_port;port <= m_portUpto;++port)
  {
    // See if it was added
    prefix.Format(_T("%d"),port);
    result.Format(_T("Adding certificate for port: %d"),port);
    m_editStatus.SetWindowText(result);
    MessagePump();

    if(DoCommand(CONFIG_ADDSSL,prefix,command,parameters,certificate,storename,clientCert))
    {
      CallProgram_For_String(command,parameters,result);
      showme += _T(">") + command + _T(" ") + parameters + _T("\r\n") + result;
    }
    else return;

    // Check if it was added correctly
    if(DoCommand(CONFIG_ASKSSL,prefix,command,parameters))
    {
      CallProgram_For_String(command,parameters,result);
      showme += _T(">") + command + _T(" ") + parameters + _T("\r\n") + result;
    }
    else return;
  }
  // Show result
  ResultDlg dlg(this,showme);
  dlg.DoModal();
}

void 
HTTPManagerDlg::OnBnClickedDelcert()
{
  XString showme;
  XString result;
  XString prefix;
  XString parameters;
  XString command;
  CWaitCursor deep_sigh;

  // Make sure the range is 1 long
  if(!m_doRange)
  {
    m_portUpto = m_port;
  }
  else
  {
    CheckPortRange();
  }

  for(int port = m_port;port <= m_portUpto;++port)
  {
    prefix.Format(_T("%d"),port);
    // See if it was added
    result.Format(_T("Delete certificate for port: %d"),port);
    m_editStatus.SetWindowText(result);
    MessagePump();

    if(DoCommand(CONFIG_DELSSL,prefix,command,parameters))
    {
      CallProgram_For_String(command,parameters,result);
      showme += _T(">") + command + _T(" ") + parameters + _T("\r\n") + result;
    }
    else return;

    // See if it was added
    if(DoCommand(CONFIG_ASKSSL,prefix,command,parameters))
    {
      CallProgram_For_String(command,parameters,result);
      showme  += _T(">") + command + _T(" ") + parameters + _T("\r\n") + result;
    }
    else return;
  }

  // Show result
  ResultDlg dlg(this,showme);
  dlg.DoModal();
}

void 
HTTPManagerDlg::OnBnClickedListner()
{
  XString result;
  XString parameters;
  XString command = _T("netsh");
  CWaitCursor deep_sigh;
  XString showme;

  // See if it was added
  if(DoCommand(CONFIG_ASKLIST,_T(""),command,parameters))
  {
    CallProgram_For_String(command,parameters,result);
    showme  = _T(">") + command + _T(" ") + parameters + _T("\r\n") + result;
  }
  // Show result
  ResultDlg dlg(this,showme);
  dlg.DoModal();
}

void 
HTTPManagerDlg::OnBnClickedListen()
{
  XString showme;
  XString result;
  XString parameters;
  XString command = _T("netsh");
  CWaitCursor deep_sigh;

  // See if it was added
  if(DoCommand(CONFIG_ADDLIST,_T(""),command,parameters))
  {
    CallProgram_For_String(command,parameters,result);
    showme  = _T(">") + command + _T(" ") + parameters + _T("\r\n") + result;
  }
  // Show result
  ResultDlg dlg(this,showme);
  dlg.DoModal();
}

void
HTTPManagerDlg::OnBnClickedNetstat()
{
  XString result;
  XString parameters;
  XString command = _T("netstat");
  CWaitCursor deep_sigh;

  // See if it was added
  parameters = _T("-a -p TCP");
  CallProgram_For_String(command,parameters,result);
  XString tonen = _T(">") + command + _T(" ") + parameters + _T("\r\n") + result;

  // Show result
  ResultDlg dlg(this,tonen);
  dlg.DoModal();
}

void
HTTPManagerDlg::OnBnClickedSecurity()
{
  SecurityDlg dlg(this);
  dlg.DoModal();
}

void
HTTPManagerDlg::MessagePump()
{
  // Do handle the paint messages for long processes where the
  // application must show parts of the interface.
  // Because we can have an endless loop (theoretically)
  // add a time limit to this process
  MSG msg;
  UINT ticks = GetTickCount();
  while(GetTickCount() - ticks < 500 && (
    PeekMessage(&msg, NULL, WM_PAINT,     WM_PAINT,     PM_REMOVE) ||
    PeekMessage(&msg, NULL, WM_SYSCOMMAND,WM_SYSCOMMAND,PM_REMOVE) ))
  {
    try
    {
      ::TranslateMessage(&msg);
      ::DispatchMessage(&msg);
    }
    catch(...)
    {
      // What now brown cow
    }
  }
}

XString
HTTPManagerDlg::GetSiteConfig(XString p_prefix)
{
  XString pathName = MarlinConfig::GetExePath();

  XString name(p_prefix);
  int pos = name.Find(_T("//"));
  if (pos)
  {
    name = _T("Site") + name.Mid(pos + 2);
    name.Replace(_T(':'), '-');
    name.Replace(_T('+'), '-');
    name.Replace(_T('*'), '-');
    name.Replace(_T('.'), '_');
    name.Replace(_T('/'), '-');
    name.Replace(_T('\\'), '-');
    name.Replace(_T("--"), _T("-"));
    name.TrimRight('-');
    name += _T(".config");

    return pathName + name;
  }
  return _T("");
}

void 
HTTPManagerDlg::OnBnClickedWebconfig()
{
  WebConfigDlg config(m_iis);
  config.DoModal();
}

void 
HTTPManagerDlg::OnBnClickedSiteWebConfig()
{
  if (m_absPath.IsEmpty())
  {
    AfxMessageBox(_T("Select a site URL first, before editing a site's own config"), MB_OK | MB_ICONERROR);
    return;
  }

  XString prefix = CreateURLPrefix(m_binding, m_secure, m_port, m_absPath);
  XString filenm = MarlinConfig::GetSiteConfig(prefix);

  WebConfigDlg config(m_iis);
  config.SetSiteConfig(prefix,filenm);
  config.DoModal();
}

void 
HTTPManagerDlg::OnBnClickedOk()
{
  CDialogEx::OnOK();
}

void 
HTTPManagerDlg::OnBnClickedCancel()
{
  CDialogEx::OnCancel();
}
