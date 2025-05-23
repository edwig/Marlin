// WebConfigRewriter.cpp : implementation file
//
#include "stdafx.h"
#include "HTTPManager.h"
#include "afxdialogex.h"
#include "WebConfigRewriter.h"


// WebConfigRewriter dialog

IMPLEMENT_DYNAMIC(WebConfigRewriter, CDialog)

WebConfigRewriter::WebConfigRewriter(bool p_iis,CWnd* pParent /*=nullptr*/)
                 	:CDialog(IDD_WC_REWRITE, pParent)
                  ,m_iis(p_iis)
{
}

WebConfigRewriter::~WebConfigRewriter()
{
}

void WebConfigRewriter::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

  DDX_Control(pDX,IDC_USE_PROTOCOL,   m_buttonProtocol);
  DDX_Control(pDX,IDC_USE_SERVER,     m_buttonServer);
  DDX_Control(pDX,IDC_USE_PORT,       m_buttonPort);
  DDX_Control(pDX,IDC_USE_PATH,       m_buttonAbspath);
  DDX_Control(pDX,IDC_USE_ROUTE0,     m_buttonRoute0);
  DDX_Control(pDX,IDC_USE_ROUTE1,     m_buttonRoute1);
  DDX_Control(pDX,IDC_USE_ROUTE2,     m_buttonRoute2);
  DDX_Control(pDX,IDC_USE_ROUTE3,     m_buttonRoute3);
  DDX_Control(pDX,IDC_USE_ROUTE4,     m_buttonRoute4);
  DDX_Control(pDX,IDC_USE_REMROUT,    m_buttonRemoveRoute);
  DDX_Control(pDX,IDC_USE_STARTROUTE, m_buttonStartRoute);

  DDX_CBString(pDX,IDC_PROTOCOL,      m_protocol);
  DDX_Text    (pDX,IDC_SERVER,        m_server);
  DDX_Text    (pDX,IDC_PORT,          m_port);
  DDX_Text    (pDX,IDC_ABSPATH,       m_abspath);
  DDX_Text    (pDX,IDC_ROUTE0,        m_route0);
  DDX_Text    (pDX,IDC_ROUTE1,        m_route1);
  DDX_Text    (pDX,IDC_ROUTE2,        m_route2);
  DDX_Text    (pDX,IDC_ROUTE3,        m_route3);
  DDX_Text    (pDX,IDC_ROUTE4,        m_route4);
  DDX_Text    (pDX,IDC_REMOVE_ROUTE,  m_removeRoute);
  DDX_Text    (pDX,IDC_STARTROUTE,    m_startRoute);

  DDX_Text(pDX,IDC_TARGET_PROTOCOL,   m_targetProtocol);
  DDX_Text(pDX,IDC_TARGET_SERVER,     m_targetServer);
  DDX_Text(pDX,IDC_TARGET_PORT,       m_targetPort);
  DDX_Text(pDX,IDC_TARGET_ABSPATH,    m_targetAbspath);
  DDX_Text(pDX,IDC_TARGET_ROUTE0,     m_targetRoute0);
  DDX_Text(pDX,IDC_TARGET_ROUTE1,     m_targetRoute1);
  DDX_Text(pDX,IDC_TARGET_ROUTE2,     m_targetRoute2);
  DDX_Text(pDX,IDC_TARGET_ROUTE3,     m_targetRoute3);
  DDX_Text(pDX,IDC_TARGET_ROUTE4,     m_targetRoute4);

  if(pDX->m_bSaveAndValidate == FALSE)
  {
    CWnd* w = nullptr;

    w = GetDlgItem(IDC_PROTOCOL);       w->EnableWindow(m_useProtocol);
    w = GetDlgItem(IDC_SERVER);         w->EnableWindow(m_useServer);
    w = GetDlgItem(IDC_PORT);           w->EnableWindow(m_usePort);
    w = GetDlgItem(IDC_ABSPATH);        w->EnableWindow(m_useAbspath);
    w = GetDlgItem(IDC_ROUTE0);         w->EnableWindow(m_useRoute0);
    w = GetDlgItem(IDC_ROUTE1);         w->EnableWindow(m_useRoute1);
    w = GetDlgItem(IDC_ROUTE2);         w->EnableWindow(m_useRoute2);
    w = GetDlgItem(IDC_ROUTE3);         w->EnableWindow(m_useRoute3);
    w = GetDlgItem(IDC_ROUTE4);         w->EnableWindow(m_useRoute4);
    w = GetDlgItem(IDC_REMOVE_ROUTE);   w->EnableWindow(m_useRemoveRoute);
    w = GetDlgItem(IDC_STARTROUTE);     w->EnableWindow(m_useStartRoute);

    w = GetDlgItem(IDC_TARGET_PROTOCOL);w->EnableWindow(m_useProtocol);
    w = GetDlgItem(IDC_TARGET_SERVER);  w->EnableWindow(m_useServer);
    w = GetDlgItem(IDC_TARGET_PORT);    w->EnableWindow(m_usePort);
    w = GetDlgItem(IDC_TARGET_ABSPATH); w->EnableWindow(m_useAbspath);
    w = GetDlgItem(IDC_TARGET_ROUTE0);  w->EnableWindow(m_useRoute0);
    w = GetDlgItem(IDC_TARGET_ROUTE1);  w->EnableWindow(m_useRoute1);
    w = GetDlgItem(IDC_TARGET_ROUTE2);  w->EnableWindow(m_useRoute2);
    w = GetDlgItem(IDC_TARGET_ROUTE3);  w->EnableWindow(m_useRoute3);
    w = GetDlgItem(IDC_TARGET_ROUTE4);  w->EnableWindow(m_useRoute4);
  }
}

BEGIN_MESSAGE_MAP(WebConfigRewriter, CDialog)
  // USING
  ON_BN_CLICKED(IDC_USE_PROTOCOL,   &WebConfigRewriter::OnBnClickedUseProtocol)
  ON_BN_CLICKED(IDC_USE_SERVER,     &WebConfigRewriter::OnBnClickedUseServer)
  ON_BN_CLICKED(IDC_USE_PORT,       &WebConfigRewriter::OnBnClickedUsePort)
  ON_BN_CLICKED(IDC_USE_PATH,       &WebConfigRewriter::OnBnClickedUsePath)
  ON_BN_CLICKED(IDC_USE_ROUTE0,     &WebConfigRewriter::OnBnClickedUseRoute0)
  ON_BN_CLICKED(IDC_USE_ROUTE1,     &WebConfigRewriter::OnBnClickedUseRoute1)
  ON_BN_CLICKED(IDC_USE_ROUTE2,     &WebConfigRewriter::OnBnClickedUseRoute2)
  ON_BN_CLICKED(IDC_USE_ROUTE3,     &WebConfigRewriter::OnBnClickedUseRoute3)
  ON_BN_CLICKED(IDC_USE_ROUTE4,     &WebConfigRewriter::OnBnClickedUseRoute4)
  ON_BN_CLICKED(IDC_USE_REMROUT,    &WebConfigRewriter::OnBnClickedUseremrout)
  ON_BN_CLICKED(IDC_USE_STARTROUTE, &WebConfigRewriter::OnBnClickedUseStartroute)
  // FIELDS
  ON_CBN_SELCHANGE(IDC_PROTOCOL,    &WebConfigRewriter::OnCbnSelchangeProtocol)
  ON_EN_CHANGE(IDC_SERVER,          &WebConfigRewriter::OnEnChangeServer)
  ON_EN_CHANGE(IDC_PORT,            &WebConfigRewriter::OnEnChangePort)
  ON_EN_CHANGE(IDC_ABSPATH,         &WebConfigRewriter::OnEnChangeAbspath)
  ON_EN_CHANGE(IDC_ROUTE0,          &WebConfigRewriter::OnEnChangeRoute0)
  ON_EN_CHANGE(IDC_ROUTE1,          &WebConfigRewriter::OnEnChangeRoute1)
  ON_EN_CHANGE(IDC_ROUTE2,          &WebConfigRewriter::OnEnChangeRoute2)
  ON_EN_CHANGE(IDC_ROUTE3,          &WebConfigRewriter::OnEnChangeRoute3)
  ON_EN_CHANGE(IDC_ROUTE4,          &WebConfigRewriter::OnEnChangeRoute4)
  ON_EN_CHANGE(IDC_REMOVE_ROUTE,    &WebConfigRewriter::OnEnChangeRemoveRoute)
  ON_EN_CHANGE(IDC_STARTROUTE,      &WebConfigRewriter::OnEnChangeStartroute)
  // TARGETS
  ON_EN_CHANGE(IDC_TARGET_PROTOCOL, &WebConfigRewriter::OnEnChangeTargetProtocol)
  ON_EN_CHANGE(IDC_TARGET_SERVER,   &WebConfigRewriter::OnEnChangeTargetServer)
  ON_EN_CHANGE(IDC_TARGET_PORT,     &WebConfigRewriter::OnEnChangeTargetPort)
  ON_EN_CHANGE(IDC_TARGET_ABSPATH,  &WebConfigRewriter::OnEnChangeTargetAbspath)
  ON_EN_CHANGE(IDC_TARGET_ROUTE0,   &WebConfigRewriter::OnEnChangeTargetRoute0)
  ON_EN_CHANGE(IDC_TARGET_ROUTE1,   &WebConfigRewriter::OnEnChangeTargetRoute1)
  ON_EN_CHANGE(IDC_TARGET_ROUTE2,   &WebConfigRewriter::OnEnChangeTargetRoute2)
  ON_EN_CHANGE(IDC_TARGET_ROUTE3,   &WebConfigRewriter::OnEnChangeTargetRoute3)
  ON_EN_CHANGE(IDC_TARGET_ROUTE4,   &WebConfigRewriter::OnEnChangeTargetRoute4)
END_MESSAGE_MAP()

BOOL
WebConfigRewriter::OnInitDialog()
{
  CDialog::OnInitDialog();

  return TRUE;
}

void
WebConfigRewriter::ReadWebConfig(MarlinConfig& p_config)
{
  CString rewriter(_T("Rewriter"));

  // Do we have a URL rewriter?
  m_useProtocol     = p_config.HasParameter(rewriter,_T("Protocol"));
  m_useServer       = p_config.HasParameter(rewriter,_T("Server"));
  m_usePort         = p_config.HasParameter(rewriter,_T("Port"));
  m_useAbspath      = p_config.HasParameter(rewriter,_T("Path"));
  m_useRoute0       = p_config.HasParameter(rewriter,_T("Route0"));
  m_useRoute1       = p_config.HasParameter(rewriter,_T("Route1"));
  m_useRoute2       = p_config.HasParameter(rewriter,_T("Route2"));
  m_useRoute3       = p_config.HasParameter(rewriter,_T("Route3"));
  m_useRoute4       = p_config.HasParameter(rewriter,_T("Route4"));
  m_useRemoveRoute  = p_config.HasParameter(rewriter,_T("RemoveRoute"));
  m_useStartRoute   = p_config.HasParameter(rewriter,_T("StartRoute"));

  m_protocol        = p_config.GetParameterString(rewriter,_T("Protocol"),      _T(""));
  m_server          = p_config.GetParameterString(rewriter,_T("Server"),        _T(""));
  m_port            = p_config.GetParameterString(rewriter,_T("Port"),          _T(""));
  m_abspath         = p_config.GetParameterString(rewriter,_T("Path"),          _T(""));
  m_route0          = p_config.GetParameterString(rewriter,_T("Route0"),        _T(""));
  m_route1          = p_config.GetParameterString(rewriter,_T("Route1"),        _T(""));
  m_route2          = p_config.GetParameterString(rewriter,_T("Route2"),        _T(""));
  m_route3          = p_config.GetParameterString(rewriter,_T("Route3"),        _T(""));
  m_route4          = p_config.GetParameterString(rewriter,_T("Route4"),        _T(""));
  m_removeRoute     = p_config.GetParameterString(rewriter,_T("RemoveRoute"),   _T(""));
  m_startRoute      = p_config.GetParameterString(rewriter,_T("StartRoute"),    _T(""));

  m_targetProtocol  = p_config.GetParameterString(rewriter,_T("TargetProtocol"),_T(""));
  m_targetServer    = p_config.GetParameterString(rewriter,_T("TargetServer"),  _T(""));
  m_targetPort      = p_config.GetParameterString(rewriter,_T("TargetPort"),    _T(""));
  m_targetAbspath   = p_config.GetParameterString(rewriter,_T("TargetPath"),    _T(""));
  m_targetRoute0    = p_config.GetParameterString(rewriter,_T("TargetRoute0"),  _T(""));
  m_targetRoute1    = p_config.GetParameterString(rewriter,_T("TargetRoute1"),  _T(""));
  m_targetRoute2    = p_config.GetParameterString(rewriter,_T("TargetRoute2"),  _T(""));
  m_targetRoute3    = p_config.GetParameterString(rewriter,_T("TargetRoute3"),  _T(""));
  m_targetRoute4    = p_config.GetParameterString(rewriter,_T("TargetRoute4"),  _T(""));

  m_buttonProtocol   .SetCheck(m_useProtocol    ? BST_CHECKED : BST_UNCHECKED);
  m_buttonServer     .SetCheck(m_useServer      ? BST_CHECKED : BST_UNCHECKED);
  m_buttonPort       .SetCheck(m_usePort        ? BST_CHECKED : BST_UNCHECKED);
  m_buttonAbspath    .SetCheck(m_useAbspath     ? BST_CHECKED : BST_UNCHECKED);
  m_buttonRoute0     .SetCheck(m_useRoute0      ? BST_CHECKED : BST_UNCHECKED);
  m_buttonRoute1     .SetCheck(m_useRoute1      ? BST_CHECKED : BST_UNCHECKED);
  m_buttonRoute2     .SetCheck(m_useRoute2      ? BST_CHECKED : BST_UNCHECKED);
  m_buttonRoute3     .SetCheck(m_useRoute3      ? BST_CHECKED : BST_UNCHECKED);
  m_buttonRoute4     .SetCheck(m_useRoute4      ? BST_CHECKED : BST_UNCHECKED);
  m_buttonRemoveRoute.SetCheck(m_useRemoveRoute ? BST_CHECKED : BST_UNCHECKED);
  m_buttonStartRoute .SetCheck(m_useStartRoute  ? BST_CHECKED : BST_UNCHECKED);

  UpdateData(FALSE);
}

void 
WebConfigRewriter::WriteWebConfig(MarlinConfig& p_config)
{
  CString rewriter(_T("Rewriter"));
  p_config.SetSection(rewriter);

  if(m_useProtocol)    p_config.SetParameter   (rewriter,_T("Protocol"),    m_protocol);
  else                 p_config.RemoveParameter(rewriter,_T("Protocol"));
  if(m_useServer)      p_config.SetParameter   (rewriter,_T("Server"),      m_server);
  else                 p_config.RemoveParameter(rewriter,_T("Server"));
  if(m_usePort)        p_config.SetParameter   (rewriter,_T("Port"),        m_port);
  else                 p_config.RemoveParameter(rewriter,_T("Port"));
  if(m_useAbspath)     p_config.SetParameter   (rewriter,_T("Path"),        m_abspath);
  else                 p_config.RemoveParameter(rewriter,_T("Path"));
  if(m_useRoute0)      p_config.SetParameter   (rewriter,_T("Route0"),      m_route0);
  else                 p_config.RemoveParameter(rewriter,_T("Route0"));
  if(m_useRoute1)      p_config.SetParameter   (rewriter,_T("Route1"),      m_route1);
  else                 p_config.RemoveParameter(rewriter,_T("Route1"));
  if(m_useRoute2)      p_config.SetParameter   (rewriter,_T("Route2"),      m_route2);
  else                 p_config.RemoveParameter(rewriter,_T("Route2"));
  if(m_useRoute3)      p_config.SetParameter   (rewriter,_T("Route3"),      m_route3);
  else                 p_config.RemoveParameter(rewriter,_T("Route3"));
  if(m_useRoute4)      p_config.SetParameter   (rewriter,_T("Route4"),      m_route4);
  else                 p_config.RemoveParameter(rewriter,_T("Route4"));
  if(m_useRemoveRoute) p_config.SetParameter   (rewriter,_T("RemoveRoute"), m_removeRoute);
  else                 p_config.RemoveParameter(rewriter,_T("RemoveRoute"));
  if(m_useStartRoute)  p_config.SetParameter   (rewriter,_T("StartRoute"),  m_startRoute);
  else                 p_config.RemoveParameter(rewriter,_T("StartRoute"));


  if(m_useProtocol)    p_config.SetParameter   (rewriter,_T("TargetProtocol"),  m_targetProtocol);
  else                 p_config.RemoveParameter(rewriter,_T("TargetProtocol"));
  if(m_useServer)      p_config.SetParameter   (rewriter,_T("TargetServer"),    m_targetServer);
  else                 p_config.RemoveParameter(rewriter,_T("TargetServer"));
  if(m_usePort)        p_config.SetParameter   (rewriter,_T("TargetPort"),      m_targetPort);
  else                 p_config.RemoveParameter(rewriter,_T("TargetPort"));
  if(m_useAbspath)     p_config.SetParameter   (rewriter,_T("TargetPath"),      m_targetAbspath);
  else                 p_config.RemoveParameter(rewriter,_T("TargetPath"));
  if(m_useRoute0)      p_config.SetParameter   (rewriter,_T("TargetRoute0"),    m_targetRoute0);
  else                 p_config.RemoveParameter(rewriter,_T("TargetRoute0"));
  if(m_useRoute1)      p_config.SetParameter   (rewriter,_T("TargetRoute1"),    m_targetRoute1);
  else                 p_config.RemoveParameter(rewriter,_T("TargetRoute1"));
  if(m_useRoute2)      p_config.SetParameter   (rewriter,_T("TargetRoute2"),    m_targetRoute2);
  else                 p_config.RemoveParameter(rewriter,_T("TargetRoute2"));
  if(m_useRoute3)      p_config.SetParameter   (rewriter,_T("TargetRoute3"),    m_targetRoute3);
  else                 p_config.RemoveParameter(rewriter,_T("TargetRoute3"));
  if(m_useRoute4)      p_config.SetParameter   (rewriter,_T("TargetRoute4"),    m_targetRoute4);
  else                 p_config.RemoveParameter(rewriter,_T("TargetRoute4"));
}

//////////////////////////////////////////////////////////////////////////
// 
// WebConfigRewriter message handlers
//
//////////////////////////////////////////////////////////////////////////

void 
WebConfigRewriter::OnBnClickedUseProtocol()
{
  m_useProtocol = m_buttonProtocol.GetCheck();
  UpdateData(FALSE);
}

void 
WebConfigRewriter::OnBnClickedUseServer()
{
  m_useServer = m_buttonServer.GetCheck();
  UpdateData(FALSE);
}

void 
WebConfigRewriter::OnBnClickedUsePort()
{
  m_usePort = m_buttonPort.GetCheck();
  UpdateData(FALSE);
}

void 
WebConfigRewriter::OnBnClickedUsePath()
{
  m_useAbspath = m_buttonAbspath.GetCheck();
  UpdateData(FALSE);
}

void
WebConfigRewriter::OnBnClickedUseRoute0()
{
  m_useRoute0 = m_buttonRoute0.GetCheck();
  UpdateData(FALSE);
}

void
WebConfigRewriter::OnBnClickedUseRoute1()
{
  m_useRoute1 = m_buttonRoute1.GetCheck();
  UpdateData(FALSE);
}

void
WebConfigRewriter::OnBnClickedUseRoute2()
{
  m_useRoute2 = m_buttonRoute2.GetCheck();
  UpdateData(FALSE);
}

void
WebConfigRewriter::OnBnClickedUseRoute3()
{
  m_useRoute3 = m_buttonRoute3.GetCheck();
  UpdateData(FALSE);
}

void
WebConfigRewriter::OnBnClickedUseRoute4()
{
  m_useRoute4 = m_buttonRoute4.GetCheck();
  UpdateData(FALSE);
}

void
WebConfigRewriter::OnBnClickedUseremrout()
{
  m_useRemoveRoute = m_buttonRemoveRoute.GetCheck();
  UpdateData(FALSE);
}

void
WebConfigRewriter::OnBnClickedUseStartroute()
{
  m_useStartRoute = m_buttonStartRoute.GetCheck();
  UpdateData(FALSE);
}



void WebConfigRewriter::OnCbnSelchangeProtocol()
{
    // TODO: Add your control notification handler code here
}

void 
WebConfigRewriter::OnEnChangeServer()
{
  UpdateData();
}

void 
WebConfigRewriter::OnEnChangePort()
{
  UpdateData();
}

void 
WebConfigRewriter::OnEnChangeAbspath()
{
  UpdateData();
}

void 
WebConfigRewriter::OnEnChangeRoute0()
{
  UpdateData();
}

void
WebConfigRewriter::OnEnChangeRoute1()
{
  UpdateData();
}

void
WebConfigRewriter::OnEnChangeRoute2()
{
  UpdateData();
}

void
WebConfigRewriter::OnEnChangeRoute3()
{
  UpdateData();
}

void
WebConfigRewriter::OnEnChangeRoute4()
{
  UpdateData();
}

void
WebConfigRewriter::OnEnChangeRemoveRoute()
{
  UpdateData();
}

void
WebConfigRewriter::OnEnChangeStartroute()
{
  UpdateData();
}

void
WebConfigRewriter::OnEnChangeTargetProtocol()
{
  UpdateData();
}

void
WebConfigRewriter::OnEnChangeTargetServer()
{
  UpdateData();
}

void
WebConfigRewriter::OnEnChangeTargetPort()
{
  UpdateData();
}

void
WebConfigRewriter::OnEnChangeTargetAbspath()
{
  UpdateData();
}

void
WebConfigRewriter::OnEnChangeTargetRoute0()
{
  UpdateData();
}

void
WebConfigRewriter::OnEnChangeTargetRoute1()
{
  UpdateData();
}

void
WebConfigRewriter::OnEnChangeTargetRoute2()
{
  UpdateData();
}

void
WebConfigRewriter::OnEnChangeTargetRoute3()
{
  UpdateData();
}

void
WebConfigRewriter::OnEnChangeTargetRoute4()
{
  UpdateData();
}
