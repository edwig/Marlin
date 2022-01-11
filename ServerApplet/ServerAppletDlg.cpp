/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: ServerAppletDlg.cpp
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
#include "ServerAppletDlg.h"
#include "ServerLoginDlg.h"
#include "ConfigDlg.h"
#include "InstallDlg.h"
#include "ResultDlg.h"
#include "RunRedirect.h"
#include <AppConfig.h>
#include <SOAPMessage.h>
#include <MarlinConfig.h>
#include <StdException.h>
#include <Version.h>
#include <io.h>
#include <winsvc.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
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

  CString m_versie;
  CString m_copyright;
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
  m_versie    = CString(PROGRAM_NAME) +  " " + MARLIN_VERSION_NUMBER;
  m_copyright = "Copyright (c) 2021 ir. W.E. Huisman";
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
  CDialog::DoDataExchange(pDX);
  DDX_Text(pDX,IDC_VERSIE,    m_versie);
  DDX_Text(pDX,IDC_COPYRIGHT, m_copyright);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()

// ServerAppletDlg dialog
ServerAppletDlg::ServerAppletDlg(CWnd* pParent /*=NULL*/)
                :CDialog(ServerAppletDlg::IDD, pParent)
{
  m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

  m_role         = "Machine unknown";
  m_status       = "Server not running";
  m_logline         = "<Logging message>";
  m_loginStatus  = false;
  m_serverStatus = Server_stopped;
  m_trafficLight    = NULL;
  m_service      = RUNAS_NTSERVICE;
}

ServerAppletDlg::~ServerAppletDlg()
{
  if(m_trafficLight)
  {
    delete m_trafficLight;
    m_trafficLight = NULL;
  }
}

void ServerAppletDlg::DoDataExchange(CDataExchange* pDX)
{
  CDialog::DoDataExchange(pDX);

  DDX_Text   (pDX,IDC_ROLE,       m_role);
  DDX_Text   (pDX,IDC_STATUS,     m_status);
  DDX_Text   (pDX,IDC_LOGLINE2,   m_logline);
  DDX_Control(pDX,IDC_CONFIG,     m_buttonConfig);
  DDX_Control(pDX,IDC_START,      m_buttonStart);
  DDX_Control(pDX,IDC_STOP,       m_buttonStop);
  DDX_Control(pDX,IDC_FUNCTION1,  m_buttonFunction1);
  DDX_Control(pDX,IDC_FUNCTION2,  m_buttonFunction2);
  DDX_Control(pDX,IDC_FUNCTION3,  m_buttonFunction3);
  DDX_Control(pDX,IDC_FUNCTION4,  m_buttonFunction4);
  DDX_Control(pDX,IDC_FUNCTION5,  m_buttonFunction5);
  DDX_Control(pDX,IDC_FUNCTION6,  m_buttonFunction6);
  DDX_Control(pDX,IDC_FUNCTION7,  m_buttonFunction7);

  // m_buttonLogin.EnableWindow(!m_loginStatus);

  if(pDX->m_bSaveAndValidate == FALSE)
  {
    if(m_trafficLight) 
    {
      delete m_trafficLight;
      m_trafficLight = NULL;
    }
    m_trafficLight = new CImage();
    HMODULE hmod = GetModuleHandle(NULL);
    switch(m_serverStatus)
    {
      case Server_stopped:  m_trafficLight->LoadFromResource(hmod,IDB_ROOD);
                            break;
      case Server_transit:  m_trafficLight->LoadFromResource(hmod,IDB_ORANJE);
                            break;
      case Server_running:  m_trafficLight->LoadFromResource(hmod,IDB_GROEN);
                            break;
    }
    // Make sure we hit ::OnPaint()
    Invalidate(FALSE);

    // Start/stop buttons
    m_buttonStart.EnableWindow(false);
    m_buttonStop .EnableWindow(false);
    if(m_role.Find("Server") >= 0)
    {
      m_buttonStart.EnableWindow(m_serverStatus != Server_running);
      m_buttonStop .EnableWindow(m_serverStatus != Server_stopped);
    }
    // Other buttons
    //     m_buttonFunction1.EnableWindow(m_serverStatus == Server_running);
    //     m_buttonFunction2.EnableWindow(m_serverStatus == Server_running);
    //     m_buttonFunction3.EnableWindow(m_serverStatus == Server_running);
    //     m_buttonFunction6.EnableWindow(m_serverStatus == Server_running);
    //     m_buttonFunction7.EnableWindow(m_loginStatus);
    //     m_buttonFunction4.EnableWindow(m_loginStatus);
    //     m_buttonFunction5.EnableWindow(m_loginStatus);
  }
}

BEGIN_MESSAGE_MAP(ServerAppletDlg, CDialog)
  ON_WM_SYSCOMMAND()
  ON_WM_PAINT()
  ON_WM_QUERYDRAGICON()
  //}}AFX_MSG_MAP
  ON_BN_CLICKED(IDC_CONFIG,     &ServerAppletDlg::OnBnClickedConfig)
  ON_EN_CHANGE (IDC_STATUS,     &ServerAppletDlg::OnEnChangeStatus)
  ON_BN_CLICKED(IDC_QUERY,      &ServerAppletDlg::OnBnClickedQuery)
  ON_BN_CLICKED(IDC_START,      &ServerAppletDlg::OnBnClickedStart)
  ON_BN_CLICKED(IDC_STOP,       &ServerAppletDlg::OnBnClickedStop)
  ON_BN_CLICKED(IDC_BOUNCE,     &ServerAppletDlg::OnBnClickedBounce)
  ON_EN_CHANGE (IDC_LOGLINE2,   &ServerAppletDlg::OnEnChangeUser)
  ON_BN_CLICKED(IDC_FUNCTION1,  &ServerAppletDlg::OnBnClickedFunction1)
  ON_BN_CLICKED(IDC_FUNCTION2,  &ServerAppletDlg::OnBnClickedFunction2)
  ON_BN_CLICKED(IDC_FUNCTION3,  &ServerAppletDlg::OnBnClickedFunction3)
  ON_BN_CLICKED(IDC_FUNCTION4,  &ServerAppletDlg::OnBnClickedFunction4)
  ON_BN_CLICKED(IDC_FUNCTION5,  &ServerAppletDlg::OnBnClickedFunction5)
  ON_BN_CLICKED(IDC_FUNCTION6,  &ServerAppletDlg::OnBnClickedFunction6)
  ON_BN_CLICKED(IDC_FUNCTION7,  &ServerAppletDlg::OnBnClickedFunction7)
  ON_BN_CLICKED(IDOK,           &ServerAppletDlg::OnBnClickedOk)
END_MESSAGE_MAP()


// ServerAppletDlg message handlers

BOOL ServerAppletDlg::OnInitDialog()
{
  CDialog::OnInitDialog();

  // Add "About..." menu item to system menu.

  // IDM_ABOUTBOX must be in the system command range.
  ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
  ASSERT (IDM_ABOUTBOX < 0xF000);

  CMenu* sysMenu = GetSystemMenu(FALSE);
  if (sysMenu != NULL)
  {
    sysMenu->AppendMenu(MF_SEPARATOR);
    sysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX,  "&About...");
    sysMenu->AppendMenu(MF_STRING, IDM_INSTALL,   "&Installation...");
    sysMenu->AppendMenu(MF_STRING, IDM_IISRESTART,"IIS &Restart");
    sysMenu->AppendMenu(MF_STRING, IDM_IISSTART,  "IIS &Start");
    sysMenu->AppendMenu(MF_STRING, IDM_IISSTOP,   "IIS Stop");
  }
  // Set the icon for this dialog.  The framework does this automatically
  // when the application's main window is not a dialog
  SetIcon(m_hIcon, TRUE);         // Set big icon
  SetIcon(m_hIcon, FALSE);        // Set small icon

  // Get machine role & run-modus
  GetConfigVariables();

  // And enable/disable the menu item for installation
  SetInstallMenu(m_service);

  // extra initialization here
  GetServerStatus();

  return TRUE;  // return TRUE  unless you set the focus to a control
}

void
ServerAppletDlg::SetInstallMenu(int p_service)
{
  CMenu* sysMenu = GetSystemMenu(FALSE);
  if(sysMenu)
  {
    UINT enable = MF_BYCOMMAND;
    UINT iismen = MF_BYCOMMAND;
    switch(p_service)
    {
      case RUNAS_STANDALONE:  iismen |= MF_GRAYED;
                              [[fallthrough]];
      case RUNAS_IISAPPPOOL:  enable |= MF_GRAYED;
                              break;
      case RUNAS_NTSERVICE:   enable |= MF_ENABLED; 
                              iismen |= MF_GRAYED;
                              break;
    }
    sysMenu->EnableMenuItem(IDM_INSTALL,enable);
    sysMenu->EnableMenuItem(IDM_IISRESTART,iismen);
    sysMenu->EnableMenuItem(IDM_IISSTART,  iismen);
    sysMenu->EnableMenuItem(IDM_IISSTOP,   iismen);
  }
}

void 
ServerAppletDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
  if ((nID & 0xFFF0) == IDM_ABOUTBOX)
  {
    CAboutDlg dlgAbout;
    dlgAbout.DoModal();
  }
  else if((nID & 0xFFF0) == IDM_INSTALL)
  {
    if(m_serverStatus != Server_stopped)
    {
      MessageBox("The server is now running.\n"
                 "You cannot alter the current configuration.\n"
                 "Stop the server first, and install/de-install after that"
                ,"Error",MB_OK|MB_ICONEXCLAMATION);
      return;

    }
    InstallDlg dlgInstall(this);
    dlgInstall.DoModal();
    GetServerStatus();
  }
  else if((nID & 0xFFF0) == IDM_IISRESTART)
  {
    IISRestart();
  }
  else if((nID & 0xFFF0) == IDM_IISSTART)
  {
    IISStart();
  }
  else if((nID & 0xFFF0) == IDM_IISSTOP)
  {
    IISStop();
  }
  else
  {
    CDialog::OnSysCommand(nID, lParam);
  }
}


void 
ServerAppletDlg::GetWSStatus()
{
  // Finding the status from the client
  // at the server through some sort of service answer
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void 
ServerAppletDlg::OnPaint()
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
    CDialog::OnPaint();

    CDC* dc = GetDC();
    m_trafficLight->BitBlt(dc->GetSafeHdc(),10,10,42,42,0,0,SRCCOPY);
  }
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR ServerAppletDlg::OnQueryDragIcon()
{
  return static_cast<HCURSOR>(m_hIcon);
}

void 
ServerAppletDlg::OnEnChangeStatus()
{
  // Nothing here
}

bool
ServerAppletDlg::AreYouSure(CString p_actie)
{
  CString melding("You are about to  ");
  melding += p_actie;
  melding += " the server.\n\nAre you sure?";
  if(::MessageBox(GetSafeHwnd(),melding,"ServerApplet",MB_YESNO|MB_DEFBUTTON2|MB_ICONQUESTION) == IDYES)
  {
    return true;
  }
  return false;
}

// Service handlers

void
ServerAppletDlg::OnBnClickedConfig()
{
  bool update = true;

  // Find the machine 'role'
  if(m_role.Right(6) != "Client")
  {
    if(m_serverStatus != Server_stopped)
    {
      update = false;
    }
  }
  ConfigDlg dlgConfig(this,update);
  dlgConfig.DoModal();


  SetInstallMenu(dlgConfig.GetRunAsService());
  // Get machine role & server status anew
  GetConfigVariables();
  GetServerStatus();
}

void 
ServerAppletDlg::OnBnClickedQuery()
{
  GetServerStatus();

  if(m_serverStatus == Server_running)
  {
    GetWSStatus();
  }
  UpdateData(FALSE);
}

bool
ServerAppletDlg::CheckConfiguration()
{
  // Perform service to be sure the server can be started
  // Running programs like
  // "CheckInstall.exe"
  // "CheckConfig.exe"

  return true;
}

void 
ServerAppletDlg::OnBnClickedStart()
{
  if(m_serverStatus == Server_stopped)
  {
    m_serverStatus = Server_transit;
    m_status = "Server is starting";

    UpdateData(FALSE);
    PumpMessage();

    // Put a wait cursor on the stack
    CWaitCursor takeADeepSigh;

    // First: check the general server configuration
    if(CheckConfiguration() == false)
    {
      GetServerStatus();
      return;
    }

    // Starting the server
    CString result;
    CString program = MarlinConfig::GetExePath() +  PRODUCT_NAME + ".exe";
    CString arguments = "start";
    CString fout(CString("Cannot start the ") + PRODUCT_NAME);
    CString actie = CString("START: ") + program + "\n\n";
    int stat = CallProgram_For_String(program,arguments,result,3000);
    m_serverStatus = stat == 0 ? Server_running : Server_stopped;
    UpdateData(FALSE);
    PumpMessage();

    int icon = MB_OK + (stat == 0 ? MB_ICONINFORMATION : MB_ICONERROR);
    ::MessageBox(GetSafeHwnd(),actie + result,PROGRAM_NAME,icon);

    // Re-Get the server status to be sure
    GetServerStatus();
  }
}

void 
ServerAppletDlg::OnBnClickedStop()
{
  if(m_serverStatus == Server_running)
  {
    if(AreYouSure("STOP"))
    {
      m_serverStatus = Server_transit;
      m_status = "Server is stopping";

      UpdateData(FALSE);
      PumpMessage();

      // Put a wait cursor on the stack
      CWaitCursor takeADeepSigh;

      CString result;
      CString program = MarlinConfig::GetExePath() + PRODUCT_NAME + ".exe";
      CString arguments = "stop";
      CString fout(CString("Cannot stop the ") +  PRODUCT_NAME);
      CString actie = CString("START: ") + program + "\n\n";

      int stat = CallProgram_For_String(program,arguments,result);
      m_serverStatus = stat == 0 ? Server_stopped : Server_running;
      UpdateData(FALSE);
      PumpMessage();

      int icon = MB_OK + (stat == 0 ? MB_ICONINFORMATION : MB_ICONERROR);
      ::MessageBox(GetSafeHwnd(),actie + result,PROGRAM_NAME,icon);

      // Re-Get the server status to be sure
      GetServerStatus();
    }
  }
}

void 
ServerAppletDlg::OnBnClickedBounce()
{
  if(!(m_serverStatus == Server_running || m_serverStatus == Server_stopped))
  {
    ::MessageBox(GetSafeHwnd()
                ,"You can only bounce (stopping & starting) if the server is running or is stopped\n"
                 "Currently the server is in an 'in-between' stadium. Use the CMD.EXE command line please."
                ,PROGRAM_NAME
                ,MB_OK|MB_ICONINFORMATION);
    return;
  }

  // Are we sure?
  if(AreYouSure("BOUNCE (stopping & starting)") == false)
  {
    return;
  }

  // Tell about the bouncing attempt and display wait cursor
  CWaitCursor takeADeepSigh;
  m_serverStatus = Server_transit;
  m_status = "Server is bouncing";

  UpdateData(FALSE);
  PumpMessage();

  CString program = CString(PRODUCT_NAME) + ".exe";
  CString arguments = "restart";
  CString fout("Cannot restart (bounce) the server");
  theApp.StartProgram(program,arguments,true,fout);

  // Try multiple times
  for(int x = 0;x < 20; ++x)
  {
    Sleep(500);
    GetServerStatus();
    if(m_serverStatus == Server_running)
    {
      // Let's assume the bouncing went well.
      // Clearing the status line
      m_logline = "<Empty status line>";
      m_password.Empty();
      m_loginURL.Empty();
      m_loginStatus = false;
      UpdateData(FALSE);
      break;
    }
  }
}

void 
ServerAppletDlg::OnEnChangeUser()
{
  UpdateData();
}

void ServerAppletDlg::OnBnClickedFunction1()
{
  ::MessageBox(GetSafeHwnd(),"Still to implement function 1",PROGRAM_NAME,MB_OK);
}

void 
ServerAppletDlg::OnBnClickedFunction2()
{
  ::MessageBox(GetSafeHwnd(),"Still to implement function 2",PROGRAM_NAME,MB_OK);
}

void 
ServerAppletDlg::OnBnClickedFunction3()
{
  ::MessageBox(GetSafeHwnd(),"Still to implement function 3",PROGRAM_NAME,MB_OK);
}

void 
ServerAppletDlg::OnBnClickedFunction4()
{
  ::MessageBox(GetSafeHwnd(),"Still to implement function 4",PROGRAM_NAME,MB_OK);
}

void
ServerAppletDlg::OnBnClickedFunction5()
{
  ::MessageBox(GetSafeHwnd(),"Still to implement function 5",PROGRAM_NAME,MB_OK);
}


void ServerAppletDlg::OnBnClickedFunction6()
{
  ::MessageBox(GetSafeHwnd(),"Still to implement function 6",PROGRAM_NAME,MB_OK);
}

void 
ServerAppletDlg::OnBnClickedFunction7()
{
  ::MessageBox(GetSafeHwnd(),"Still to implement function 7",PROGRAM_NAME,MB_OK);
}

void 
ServerAppletDlg::OnBnClickedOk()
{
  CDialog::OnOK();
}

void
ServerAppletDlg::GetServerStatus()
{
  if(m_role.Find("Server") >= 0)
  {
    GetServerStatusLocally();
  }
  else
  {
    GetWSStatus();
  }
  UpdateData(FALSE);
  PumpMessage();
}

void
ServerAppletDlg::GetServerStatusLocally()
{
  CString program   = CString(PRODUCT_NAME) + ".exe";
  CString arguments = "query";
  CString fout(CString("Cannot get the status of the ") + PRODUCT_NAME + " server");

  int stat = theApp.StartProgram(program,arguments,true,fout);

  switch(stat)
  {
    case SERVICE_STOPPED:           m_status = "Service is stopped";
                                    m_serverStatus = Server_stopped;
                                    break;
    case SERVICE_START_PENDING:     m_status = "Service has a pending start";
                                    m_serverStatus = Server_transit;
                                    break;
    case SERVICE_STOP_PENDING:      m_status = "Service has a pending stop";
                                    m_serverStatus = Server_transit;
                                    break;
    case SERVICE_RUNNING:           m_status = "Service is running";
                                    m_serverStatus = Server_running;
                                    break;
    case SERVICE_CONTINUE_PENDING:  m_status = "Service has a pending continue";
                                    m_serverStatus = Server_transit;
                                    break;
    case SERVICE_PAUSE_PENDING:     m_status = "Service has a pending pause";    
                                    m_serverStatus = Server_transit;
                                    break;
    case SERVICE_PAUSED:            m_status = "Service is paused";
                                    m_serverStatus = Server_transit;
                                    break;
    default:                        m_status = "No server status found";
                                    m_serverStatus = Server_stopped;
                                    break;
  }
}

void
ServerAppletDlg::GetConfigVariables()
{
  AppConfig config(PRODUCT_NAME);

  config.ReadConfig();
  m_role    = "Machine: " + config.GetRole();
  m_service = config.GetRunAsService();
  m_url     = config.GetServerURL();
}

void
ServerAppletDlg::IISRestart()
{
  CWaitCursor takeADeepSigh;
  CString program;
 
  program.GetEnvironmentVariable("windir");
  program += "\\system32\\iisreset.exe";
  CString parameter("");
  CString result;
  CallProgram_For_String(program,parameter,result,10000);
  result.Remove('\r');
  result = result.Trim('\n');
  ::MessageBox(GetSafeHwnd(),result,"Restart IIS",MB_OK | MB_ICONINFORMATION);
}

void
ServerAppletDlg::IISStart()
{
  CWaitCursor takeADeepSigh;
  CString program;

  program.GetEnvironmentVariable("windir");
  program += "\\system32\\iisreset.exe";
  CString parameter("/start");
  CString result;
  CallProgram_For_String(program,parameter,result,10000);
  result.Remove('\r');
  result = result.TrimLeft("\n");
  ::MessageBox(GetSafeHwnd(),result,"Start IIS",MB_OK | MB_ICONINFORMATION);
}

void
ServerAppletDlg::IISStop()
{
  CWaitCursor takeADeepSigh;
  CString program;

  program.GetEnvironmentVariable("windir");
  program += "\\system32\\iisreset.exe";
  CString parameter("/stop");
  CString result;
  CallProgram_For_String(program,parameter,result,10000);
  result.Remove('\r');
  result = result.TrimLeft("\n");
  ::MessageBox(GetSafeHwnd(),result,"Stop IIS",MB_OK | MB_ICONINFORMATION);
}

void
ServerAppletDlg::PumpMessage()
{
  // Now handle only the paint messages for larger process
  // where we must display our application (again)
  // Potentially this can be an endless loop.
  // As this is non-crucial, we put a time limit on it for 250 ms.
  MSG  msg;
  UINT ticks = GetTickCount();
  while(GetTickCount() - ticks < 250 && (PeekMessage(&msg, NULL, WM_MOVE, WM_USER, PM_REMOVE)))
  {
    try
    {
      ::TranslateMessage(&msg);
      ::DispatchMessage(&msg);
    }
    catch(StdException& er)
    {
      // How now, holy cow?
      UNREFERENCED_PARAMETER(er);
    }
  }
}
