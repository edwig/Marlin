/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: ConfigDlg.cpp
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
#include "ServerApplet.h"
#include "ConfigDlg.h"
#include "TestMail.h"
#include "BrowseForDirectory.h"
#include "BrowseForFilename.h"
#include <AppConfig.h>
#include <MarlinConfig.h>
#include <afxdialogex.h>
#include <CrackURL.h>
#include <CreateURLPrefix.h>
#include <version.h>
#include <winhttp.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// ConfigDlg dialog

IMPLEMENT_DYNAMIC(ConfigDlg, CDialog)

ConfigDlg::ConfigDlg(CWnd* p_parent,bool p_writeable)
          :CDialog(ConfigDlg::IDD,p_parent)
          ,m_dialogWriteable(p_writeable)
{
  m_configWriteable     = false;
  m_instance            = 0;
  m_serverPort          = 0;
  m_serverLogging       = 0;
  m_runAsService        = RUNAS_NTSERVICE;
  m_foutRapportVestuur  = false;
  m_foutRapportTest     = false;
  m_secureServer        = false;
}

ConfigDlg::~ConfigDlg()
{
}

void ConfigDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_NAAM,         m_naam);
	DDX_Text(pDX, IDC_INSTANTIE,    m_instance);
	DDX_Text(pDX, IDC_SERVERADRES,  m_serverAddress);
	DDX_Text(pDX, IDC_SERVER,       m_server);
	DDX_Text(pDX, IDC_SERVERPORT,   m_serverPort);
	DDX_Text(pDX, IDC_BASE_URL,     m_baseURL);
	DDX_Text(pDX, IDC_WEBROOT,      m_webroot);
	DDX_Text(pDX, IDC_SERVERLOG,    m_serverLogfile);

	DDX_Control(pDX, IDC_ROLE,            m_comboRole);
	DDX_Control(pDX, IDC_SEARCHROOT,      m_buttonWebroot);
	DDX_Control(pDX, IDC_SERVERLOGGING,   m_comboServerLogging);
	DDX_Control(pDX, IDC_SEARCHSERVLOG,   m_buttonServerLog);
	DDX_Control(pDX, IDC_RUNAS,           m_comboRunAs);
  DDX_Control(pDX, IDC_SECURESERVER,    m_buttonSecureServer);
// 	DDX_Control(pDX, IDC_FR_VERSTUUR,     m_buttonFRVerstuur);
// 	DDX_Control(pDX, IDC_FR_TESTEN,       m_buttonFRTest);
//  DDX_Control(pDX, IDC_TESTMAIL,        m_buttonTestMail);
// 	DDX_Text(pDX, IDC_MAIL_SERVER,  m_mailServer);
// 	DDX_Text(pDX, IDC_FR_AFZENDER,  m_foutRapportAfzender);
// 	DDX_Text(pDX, IDC_FR_ONTVANGER, m_foutRapportOntvanger);

	if(pDX->m_bSaveAndValidate == FALSE)
	{
    m_buttonSecureServer.SetCheck(m_secureServer);

		if(m_dialogWriteable)
		{
			bool client = false;
			bool server = false;

			if (m_role.Find(_T("Client")) >= 0) client = true;
			if (m_role.Find(_T("Server")) >= 0) server = true;

			// Server parts
			SetItemActive(IDC_NAAM,         server);
			SetItemActive(IDC_WEBROOT,      server);
			SetItemActive(IDC_SERVERLOG,    server);
			m_buttonWebroot     .EnableWindow(server);
			m_comboServerLogging.EnableWindow(server);
			m_buttonServerLog   .EnableWindow(server);
			m_comboRunAs        .EnableWindow(server);
    }
	}
}

BEGIN_MESSAGE_MAP(ConfigDlg, CDialog)
  ON_WM_TIMER()
  ON_CBN_CLOSEUP (IDC_ROLE,            &ConfigDlg::OnCbnRole)
  ON_EN_CHANGE   (IDC_NAAM,            &ConfigDlg::OnEnChangeNaam)
  ON_EN_KILLFOCUS(IDC_SERVERADRES,     &ConfigDlg::OnEnChangeBriefserverURL)
  ON_EN_CHANGE   (IDC_INSTANTIE,       &ConfigDlg::OnEnChangeInstantie)
  ON_EN_KILLFOCUS(IDC_SERVER,          &ConfigDlg::OnEnChangeServer)
  ON_EN_KILLFOCUS(IDC_SERVERPORT,      &ConfigDlg::OnEnChangeServerport)
  ON_EN_KILLFOCUS(IDC_BASE_URL,        &ConfigDlg::OnEnChangeBaseURL)
  ON_EN_CHANGE   (IDC_WEBROOT,         &ConfigDlg::OnEnChangeWebroot)
  ON_BN_CLICKED  (IDC_SEARCHROOT,      &ConfigDlg::OnBnClickedSearchroot)
  ON_CBN_CLOSEUP (IDC_SERVERLOGGING,   &ConfigDlg::OnCbnSeverlogging)
  ON_EN_CHANGE   (IDC_SERVERLOG,       &ConfigDlg::OnEnChangeServerlog)
  ON_BN_CLICKED  (IDC_SEARCHSERVLOG,   &ConfigDlg::OnBnClickedSearchserverlog)
  ON_CBN_CLOSEUP (IDC_RUNAS,           &ConfigDlg::OnCbnRunAs)
  ON_EN_CHANGE   (IDC_MAIL_SERVER,     &ConfigDlg::OnEnChangeMailServer)
  ON_EN_CHANGE   (IDC_FR_AFZENDER,     &ConfigDlg::OnEnChangeFRAfzender)
  ON_EN_CHANGE   (IDC_FR_ONTVANGER,    &ConfigDlg::OnEnChangeFROntvanger)
  ON_BN_CLICKED  (IDC_FR_VERSTUUR,     &ConfigDlg::OnBnClickedFRVerstuur)
  ON_BN_CLICKED  (IDC_FR_TESTEN,       &ConfigDlg::OnBnClickedFRTest)
  ON_BN_CLICKED  (IDC_TESTMAIL,        &ConfigDlg::OnBnClickedTestMail)
  ON_BN_CLICKED  (IDC_SECURESERVER,    &ConfigDlg::OnBnClickedSecureServer)
  ON_BN_CLICKED  (IDOK,                &ConfigDlg::OnBnClickedOk)
END_MESSAGE_MAP()

BOOL
ConfigDlg::OnInitDialog()
{
  CDialog::OnInitDialog();

  m_comboRole.AddString(_T("Client"));
  m_comboRole.AddString(_T("Server"));
  m_comboRole.AddString(_T("Client & Server"));

  m_comboRunAs.AddString(_T("Start as a stand-alone program"));
  m_comboRunAs.AddString(_T("Integrate as a MS-Windows service"));
  m_comboRunAs.AddString(_T("Run on Internet Information Services (IIS)"));

  InitLogging(m_comboServerLogging);

  ReadConfig();
  UpdateData(FALSE);
  WarningWriteRights();

  // Possibly turn the whole window to read-only
  // in case the server is running
  WriteableConfigDlg();

  SetTimer(1,100,NULL);

  return FALSE;
}

void
ConfigDlg::InitLogging(CComboBox& p_combo)
{
  // See Marlin "HTTPLoglevel.h"
  p_combo.AddString(_T("No logging"));                                          // HLL_NOLOG      0       // No logging is ever done
  p_combo.AddString(_T("Error logging only"));                                  // HLL_ERRORS     1       // Only errors are logged
  p_combo.AddString(_T("Logging of HTTP actions"));                             // HLL_LOGGING    2       // 1 + Logging of actions
  p_combo.AddString(_T("HTTP actions and SOAP bodies"));                        // HLL_LOGBODY    3       // 2 + Logging of actions and soap bodies
  p_combo.AddString(_T("HTTP actions, SOAP bodies and tracing"));               // HLL_TRACE      4       // 3 + Tracing of settings
  p_combo.AddString(_T("HTTP actions, SOAP bodies, tracing and HEX dumping"));  // HLL_TRACEDUMP  5       // 4 + Tracing and HEX dumping of objects
}

void
ConfigDlg::OnTimer(UINT_PTR nIDEvent)
{
  if(nIDEvent == 1)
  {
    KillTimer(1);
    OnCbnRole();
  }
}

void
ConfigDlg::WriteableConfigDlg()
{
  SetControlWriteable(IDC_ROLE);
  SetControlWriteable(IDC_SERVER);
  SetControlWriteable(IDC_SERVERADRES);
  SetControlWriteable(IDC_INSTANTIE);
  SetControlWriteable(IDC_SERVERPORT);
  SetControlWriteable(IDC_BASE_URL);
  SetControlWriteable(IDC_KEEPALIVE);
  SetControlWriteable(IDC_FR_VERSTUUR);
  SetControlWriteable(IDC_FR_AFZENDER);
  SetControlWriteable(IDC_FR_ONTVANGER);
  SetControlWriteable(IDC_FR_TESTEN);
  SetControlWriteable(IDC_MAIL_SERVER);
  SetControlWriteable(IDC_NAAM);
  SetControlWriteable(IDC_WEBROOT);
  SetControlWriteable(IDC_SEARCHROOT);
  SetControlWriteable(IDC_SERVERLOG);
  SetControlWriteable(IDC_SEARCHSERVLOG);
  SetControlWriteable(IDC_SERVERLOGGING);
  SetControlWriteable(IDC_RUNAS);
  SetControlWriteable(IDC_DESTROY);
  SetControlWriteable(IDC_INTERFACE);
  SetControlWriteable(IDC_SECURESERVER);
  SetControlWriteable(IDOK);
}

void
ConfigDlg::SetControlWriteable(UINT p_dlgID)
{
  CWnd* wnd = GetDlgItem(p_dlgID);
  if(wnd)
  {
    wnd->EnableWindow(m_dialogWriteable);
  }
}

void
ConfigDlg::ReadConfig()
{
  AppConfig config;
  config.ReadConfig();
  CString section(SECTION_APPLICATION);

  m_configWriteable       = config.GetConfigWritable();
  m_role                  = config.GetParameterString (section,_T("Role"),_T(""));
  m_naam                  = config.GetParameterString (section,_T("Name"),_T(""));
  m_instance              = config.GetParameterInteger(section,_T("Instance"),0);
  m_server                = config.GetParameterString (section,_T("Server"),_T(""));
  m_serverPort            = config.GetParameterInteger(section,_T("ServerPort"),0);
  m_baseURL               = config.GetParameterString (section,_T("BaseURL"),_T("/"));
  m_serverLogfile         = config.GetParameterString (section,_T("ServerLog"),_T(""));
  m_serverLogging         = config.GetParameterInteger(section,_T("ServerLoglevel"),0);
  m_secureServer          = config.GetParameterBoolean(section,_T("Secure"),false);
  m_webroot               = config.GetParameterString (section,_T("WebRoot"),_T(""));
  m_runAsService          = config.GetParameterInteger(section,_T("RunAsService"),RUNAS_NTSERVICE);
//   m_mailServer            = config.GetMailServer();
//   m_foutRapportVestuur    = config.GetFoutRapportageVerstuur();
//   m_foutRapportAfzender   = config.GetFoutRapportageAfzender();
//   m_foutRapportOntvanger  = config.GetFoutRapportageOntvanger();
//   m_foutRapportTest       = config.GetFoutRapportageTest();

  m_comboServerLogging.SetCurSel(m_serverLogging);
  m_comboRunAs        .SetCurSel(m_runAsService);
//   m_buttonFRVerstuur  .SetCheck(m_foutRapportVestuur ? TRUE : FALSE);
//   m_buttonFRTest      .SetCheck(m_foutRapportTest    ? TRUE : FALSE);
//   m_buttonSecureServer.SetCheck(m_secureServer);
  
  // Set role of this machine
  if(m_role == _T("Client"))       m_comboRole.SetCurSel(0);
  if(m_role == _T("Server"))       m_comboRole.SetCurSel(1);
  if(m_role == _T("ClientServer")) m_comboRole.SetCurSel(2);

  // Construct the compound server address
  ConstructServerAddress();
}

void
ConfigDlg::ConstructServerAddress()
{
  ObviousChecks();
  // Construct server address
  if(m_serverPort == INTERNET_DEFAULT_HTTP_PORT)
  {
    m_serverAddress.Format(_T("http://%s%s"),m_server.GetString(),m_baseURL.GetString());
    m_secureServer = false;
  }
  else if(m_serverPort == INTERNET_DEFAULT_HTTPS_PORT)
  {
    m_serverAddress.Format(_T("https://%s%s"),m_server.GetString(),m_baseURL.GetString());
    m_secureServer = true;
  }
  else
  {
    m_serverAddress.Format(_T("http%s://%s:%d%s")
                           ,m_secureServer ? _T("s") : _T("")
                           ,m_server.GetString()
                           ,m_serverPort
                           ,m_baseURL.GetString());
  }
}

bool
ConfigDlg::SaveConfig()
{
  if(CheckConfig())
  {
    return WriteConfig();
  }
  return false;
}

bool
ConfigDlg::CheckConfig()
{
  // Check for correct values
  if(!(m_role == _T("Client") || m_role == _T("Server") || m_role == _T("ClientServer")))
  {
    MessageBox(_T("Choose the correct role for the current machine (Client / Server / Client & Server)")
              ,_T("ERROR")
              ,MB_OK|MB_ICONERROR);
    return false;
  }

  if(m_instance < 1 || m_instance > MAXIMUM_INSTANCE)
  {
    MessageBox(_T("The instance number of the server must be between 1 and 100 (inclusive)")
              ,_T("ERROR")
              ,MB_OK|MB_ICONERROR);
    return false;
  }
  if(m_serverPort != INTERNET_DEFAULT_HTTP_PORT && 
     m_serverPort != INTERNET_DEFAULT_HTTPS_PORT)
  {
    if(m_serverPort < 1025 || m_serverPort > 65535)
    {
      MessageBox(_T("The server port must be 80, 443 or between 1025 and 65535 (inclusive)")
                ,_T("ERROR")
                ,MB_OK|MB_ICONERROR);
      return false;
    }
  }

  if (m_foutRapportVestuur)
  {
    if (!(   CheckFRVeldIngevuld(m_mailServer,           _T("mail server"))
          && CheckFRVeldIngevuld(m_foutRapportAfzender,  _T("sender")   )
          && CheckFRVeldIngevuld(m_foutRapportOntvanger, _T("receiver")  )))
    {
      return false;
    }
  }

  // Minimum base URL is one (1) char site "/x/"
  if(m_baseURL.IsEmpty() || m_baseURL.GetLength() < 3 || m_baseURL == _T("/") || m_baseURL.Left(1) != _T("/") || m_baseURL.Right(1) != _T("/"))
  {
    MessageBox(XString(_T("The base URL of ")) + PRODUCT_NAME + _T(" cannot be empty or just the root reference!")
              ,_T("ERROR"),MB_OK|MB_ICONERROR);
    return false;
  }

  return true;
}

bool
ConfigDlg::WriteConfig()
{
  // Save
  AppConfig config;

  m_configWriteable = config.GetConfigWritable();
  if(!m_configWriteable)
  {
    WarningWriteRights();
    return false;
  }
  CString section(SECTION_APPLICATION);

  config.SetParameter(section,_T("Role"),           m_role);
  config.SetParameter(section,_T("Name"),           m_naam);
  config.SetParameter(section,_T("Instance"),       m_instance);
  config.SetParameter(section,_T("Server"),         m_server);
  config.SetParameter(section,_T("ServerPort"),     m_serverPort);
  config.SetParameter(section,_T("BaseURL"),        m_baseURL);
  config.SetParameter(section,_T("Secure"),         m_secureServer);
  config.SetParameter(section,_T("ServerLog"),      m_serverLogfile);
  config.SetParameter(section,_T("ServerLoglevel"), m_serverLogging);
  config.SetParameter(section,_T("WebRoot"),        m_webroot);
  config.SetParameter(section,_T("RunAsService"),   m_runAsService);

  //Mail server
//   config.SetMailServer(m_mailServer);
// 
//   config.SetFoutRapportageVerstuur (m_foutRapportVestuur);
//   config.SetFoutRapportageAfzender (m_foutRapportAfzender);
//   config.SetFoutRapportageOntvanger(m_foutRapportOntvanger);
//   config.SetFoutRapportageTest     (m_foutRapportTest);

  if(config.WriteConfig() == false)
  {
    ::MessageBox(GetSafeHwnd()
                ,XString(_T("Cannot write the configuration file '")) + PRODUCT_NAME + _T(".config' to disk.\n")
                 _T("Check the rights on the file and the rights on the directory it is in!")
                ,PROGRAM_NAME
                ,MB_OK | MB_ICONERROR);
    return false;
  }
  return true;
}

bool
ConfigDlg::CreateDirectories()
{
  if(!m_configWriteable)
  {
    return false;
  }
  return true;
}

bool
ConfigDlg::WarningWriteRights()
{
  if(!m_configWriteable)
  {
    ::MessageBox(GetSafeHwnd()
                ,_T("You do not have the right to write to the config file.\n")
                 _T("So the file cannot be saved to disk!")
                ,_T("Warning")
                ,MB_OK | MB_ICONEXCLAMATION);
    return true;
  }
  return false;
}

bool 
ConfigDlg::CheckFRVeldIngevuld(const XString& frVeld, const XString& description)
{
  if (frVeld.IsEmpty())
  {
    XString error;
    error.Format(_T("If sending error reports is 'on', the %s field needs to be filled in."),description.GetString());

    MessageBox(error,_T("ERROR"),MB_OK|MB_ICONERROR);
    return false;
  }
  return true;
}

void
ConfigDlg::SetItemActive(int p_item,bool p_active)
{
  CWnd* window = GetDlgItem(p_item);
  if(window)
  {
    window->EnableWindow(p_active);
  }
}

void
ConfigDlg::ObviousChecks()
{
  if(!m_baseURL.IsEmpty())
  {
    if(m_baseURL.Left(1) != _T("/"))
    {
      m_baseURL = _T("/") + m_baseURL;
      UpdateData(FALSE);
    }
    if(m_baseURL.Right(1) != _T("/"))
    {
      m_baseURL += _T("/");
      UpdateData(FALSE);
    }
  }
}

// ConfigDlg message handlers

void
ConfigDlg::OnCbnRole()
{
  int ind = m_comboRole.GetCurSel();
  if(ind >= 0)
  {
    switch(ind)
    {
      case 0: m_role = _T("Client");        break;
      case 1: m_role = _T("Server");        break;
      case 2: m_role = _T("ClientServer");  break;
    }
    UpdateData(FALSE);
  }
}

void 
ConfigDlg::OnEnChangeNaam()
{
  UpdateData();
}

void 
ConfigDlg::OnEnChangeInstantie()
{
  UpdateData();
}

void
ConfigDlg::OnEnChangeBriefserverURL()
{
  UpdateData();
  CrackedURL cracked(m_baseURL);
  if(cracked.Valid())
  {
    m_server       = cracked.m_host;
    m_serverPort   = cracked.m_port;
    m_baseURL      = cracked.m_path;
    m_secureServer = cracked.m_secure;
    ObviousChecks();
  }
  else
  {
    ::MessageBox(GetSafeHwnd(),_T("Not a valid server URL"),PROGRAM_NAME,MB_OK|MB_ICONERROR);
    m_server.Empty();
    m_baseURL.Empty();
    m_serverPort = 80;
    m_secureServer = false;
  }
  UpdateData(FALSE);
}

void 
ConfigDlg::OnEnChangeServer()
{
  UpdateData();

  // Guard against using 'localhost'
  if(m_server.CompareNoCase(_T("localhost")) == 0)
  {
    m_server = GetHostName(HOSTNAME_FULL);
  }
  ConstructServerAddress();
  UpdateData(FALSE);
}

void 
ConfigDlg::OnEnChangeServerport()
{
  UpdateData();
  ConstructServerAddress();
  UpdateData(FALSE);
}

void
ConfigDlg::OnEnChangeBaseURL()
{
  UpdateData();
  ConstructServerAddress();
  UpdateData(FALSE);
}

void
ConfigDlg::OnBnClickedSecureServer()
{
  m_secureServer = m_buttonSecureServer.GetCheck() > 0;
  m_serverPort   = m_secureServer ? INTERNET_DEFAULT_HTTPS_PORT : INTERNET_DEFAULT_HTTP_PORT;
  ConstructServerAddress();
  UpdateData(FALSE);
}

void 
ConfigDlg::OnEnChangeWebroot()
{
  UpdateData();
}

void 
ConfigDlg::OnBnClickedSearchroot()
{
  XString rootdir;
  BrowseForDirectory map;
  if(map.Browse(GetSafeHwnd()
                ,_T("Get path to the WEBROOT directory")
                ,m_webroot
                ,rootdir
                ,false    // files
                ,true))   // status
  {
    XString path = map.GetPath();
    if(m_webroot.CompareNoCase(path))
    {
      m_webroot = path;
      UpdateData(FALSE);
    }
  }
}

void 
ConfigDlg::OnCbnSeverlogging()
{
  int ind = m_comboServerLogging.GetCurSel();
  if(ind >= 0)
  {
    m_serverLogging = ind;
  }
}

void 
ConfigDlg::OnEnChangeServerlog()
{
  UpdateData();
}

void 
ConfigDlg::OnBnClickedSearchserverlog()
{
  BrowseForFilename file(true
                        ,_T("Getting the path to the server logfile")
                        ,_T("txt")
                        ,m_serverLogfile
                        ,0
                        ,_T("Textfiles (*.txt)|*.txt|")
                         _T("All files (*.*)|*.*|")
                        ,_T(""));
  if(file.DoModal() == IDOK)
  {
    XString path = file.GetChosenFile();
    if(m_serverLogfile.CompareNoCase(path))
    {
      m_serverLogfile = path;
      UpdateData(FALSE);
    }
  }
}

void
ConfigDlg::OnCbnRunAs()
{
  int ind = m_comboRunAs.GetCurSel();
  if(ind >= 0)
  {
    // Set the server run mode
    m_runAsService = ind;


    // Prepare the message box
    XString message;
    int     buttons = MB_OK|MB_ICONWARNING;

    // Get the correct warning
    switch(m_runAsService)
    {
      case RUNAS_STANDALONE:  message = _T("If you turn off the integration with MS-Windows NT-Services, you are limited to the following:\n")
                                        _T("- Automatic starting and stopping of the service at the stopping and starting of the system\n")
                                        _T("- Automatic restarting of the service in case of a calamity or a program bug\n")
                                        _T("- Working in an isolated MS-Windows service account and system session");
                              break;
      case RUNAS_NTSERVICE:   message = _T("If you are working integrated with the service manager, you should be aware of the following:\n")
                                        _T("- Configure the service (and account) through the main menu 'Installation...'\n")
                                        _T("- Use a system account that has authorization on UNC paths");
                              break;
      case RUNAS_IISAPPPOOL:  message = _T("If you are working integrated with Microsoft Internet-Information-Services (IIS), you should be aware of the following:\n")
                                        _T("- The name of the server must be the same as the application pool in IIS\n")
                                        _T("- The base-URL must be the same as in the configured site in IIS\n")
                                        _T("- The server port number must conform to the site bindings in IIS");
                              break;
      default:                message = _T("ALARM: No runmode configured!");
                              buttons = MB_OK | MB_ICONERROR;
    }
    ::MessageBox(GetSafeHwnd(),message,PROGRAM_NAME,buttons);
    UpdateData(FALSE);
  }
}


void
ConfigDlg::OnEnChangeMailServer()
{
  UpdateData();
}

void
ConfigDlg::OnEnChangeFRAfzender()
{
  UpdateData();
}

void
ConfigDlg::OnEnChangeFROntvanger()
{
  UpdateData();
}

void
ConfigDlg::OnBnClickedFRVerstuur()
{
  m_foutRapportVestuur = m_buttonFRVerstuur.GetCheck() == TRUE;
}

void
ConfigDlg::OnBnClickedFRTest()
{
  m_foutRapportTest = m_buttonFRTest.GetCheck() == TRUE;
}

void
ConfigDlg::OnBnClickedTestMail()
{
  TestMail test(m_foutRapportAfzender,m_foutRapportOntvanger,m_mailServer);
  test.Send();
}

void
ConfigDlg::OnBnClickedOk()
{
  if(SaveConfig() && CreateDirectories())
  {
    CDialog::OnOK();
  }
}
