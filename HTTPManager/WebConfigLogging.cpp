/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: WebConfigLogging.cpp
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
#include "HTTPLoglevel.h"
#include "WebConfigLogging.h"
#include "MarlinConfig.h"
#include "MapDialog.h"
#include "FileDialog.h"
#include "HTTPClient.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// WebConfigDlg dialog

IMPLEMENT_DYNAMIC(WebConfigLogging, CDialogEx)

WebConfigLogging::WebConfigLogging(bool p_iis,CWnd* pParent /*=NULL*/)
                 :CDialogEx(WebConfigLogging::IDD, pParent)
                 ,m_iis(p_iis)
{
  m_useLogfile      = false;
  m_useLogCaching   = false;
  m_useLogging      = false;
  m_useLogTiming    = false;
  m_useLogEvents    = false;
  m_useLogLevel     = false;
  m_useRotate       = false;
  m_usePerUser      = false;

  // LOGFILE OVERRIDES
  m_logCache        = false;
  m_doLogging       = false;
  m_doTiming        = false;
  m_doEvents        = false;
  m_doRotate        = false;
  m_doPerUser       = false;
  m_logLevel        = HLL_NOLOG;
}

WebConfigLogging::~WebConfigLogging()
{
}

void WebConfigLogging::DoDataExchange(CDataExchange* pDX)
{
  CDialogEx::DoDataExchange(pDX);
  // USING FIELDS
  DDX_Control(pDX,IDC_USE_LOGFILE,    m_buttonUseLogfile);
  DDX_Control(pDX,IDC_USE_CACHING,    m_buttonUseLogCaching);
  DDX_Control(pDX,IDC_USE_LOGGING,    m_buttonUseLogging);
  DDX_Control(pDX,IDC_USE_LOG_TIMING, m_buttonUseLogTiming);
  DDX_Control(pDX,IDC_USE_LOG_EVENTS, m_buttonUseLogEvents);
  DDX_Control(pDX,IDC_USE_LOGLEVEL,   m_buttonUseLogLevel);
  DDX_Control(pDX,IDC_USE_ROTATION,   m_buttonUseRotation);
  DDX_Control(pDX,IDC_USE_PERUSER,    m_buttonUsePerUser);

  // LOGFILE OVERRIDES
  DDX_Text   (pDX,IDC_LOGFILE,        m_logfile);
  DDX_Text   (pDX,IDC_LOG_CACHELINES, m_logCache);
  DDX_Control(pDX,IDC_LOGGING,        m_buttonLogging);
  DDX_Control(pDX,IDC_LOG_TIMING,     m_buttonTiming);
  DDX_Control(pDX,IDC_LOG_EVENTS,     m_buttonEvents);
  DDX_Control(pDX,IDC_LOGLEVEL,       m_comboLogLevel);
  DDX_Control(pDX,IDC_LOG_ROTATE,     m_buttonRotation);
  DDX_Control(pDX,IDC_LOG_PERUSER,    m_buttonPerUser);

  if(pDX->m_bSaveAndValidate == FALSE)
  {
    CWnd* w = nullptr;
    
    w = GetDlgItem(IDC_LOGFILE);        w->EnableWindow(m_useLogfile);
    w = GetDlgItem(IDC_BUTT_LOGFILE);   w->EnableWindow(m_useLogfile);
    w = GetDlgItem(IDC_LOG_CACHELINES); w->EnableWindow(m_useLogCaching);

    m_buttonLogging .EnableWindow(m_useLogging);
    m_buttonTiming  .EnableWindow(m_useLogTiming);
    m_buttonEvents  .EnableWindow(m_useLogEvents);
    m_comboLogLevel .EnableWindow(m_useLogLevel);
    m_buttonRotation.EnableWindow(m_useRotate);
    m_buttonPerUser. EnableWindow(m_usePerUser);
  }
}

BEGIN_MESSAGE_MAP(WebConfigLogging, CDialogEx)
  // LOGFILE OVERRIDES
  ON_EN_CHANGE    (IDC_LOGFILE,       &WebConfigLogging::OnEnChangeLogfile)
  ON_BN_CLICKED   (IDC_BUTT_LOGFILE,  &WebConfigLogging::OnBnClickedButtLogfile)
  ON_EN_CHANGE    (IDC_LOG_CACHELINES,&WebConfigLogging::OnEnChangeLogCachelines)
  ON_BN_CLICKED   (IDC_LOGGING,       &WebConfigLogging::OnBnClickedLogging)
  ON_BN_CLICKED   (IDC_LOG_TIMING,    &WebConfigLogging::OnBnClickedLogTiming)
  ON_BN_CLICKED   (IDC_LOG_EVENTS,    &WebConfigLogging::OnBnClickedLogEvents)
  ON_CBN_CLOSEUP  (IDC_LOGLEVEL,      &WebConfigLogging::OnCbnSelchangeLogLevel)
  ON_BN_CLICKED   (IDC_LOG_ROTATE,    &WebConfigLogging::OnBnClickedRotation)
  ON_BN_CLICKED   (IDC_LOG_PERUSER,   &WebConfigLogging::OnBnClickedPerUser)
  // USING BUTTONS
  ON_BN_CLICKED(IDC_USE_LOGFILE,      &WebConfigLogging::OnBnClickedUseLogfile)
  ON_BN_CLICKED(IDC_USE_CACHING,      &WebConfigLogging::OnBnClickedUseCaching)
  ON_BN_CLICKED(IDC_USE_LOGGING,      &WebConfigLogging::OnBnClickedUseLogging)
  ON_BN_CLICKED(IDC_USE_LOG_TIMING,   &WebConfigLogging::OnBnClickedUseLogTiming)
  ON_BN_CLICKED(IDC_USE_LOG_EVENTS,   &WebConfigLogging::OnBnClickedUseLogEvents)
  ON_BN_CLICKED(IDC_USE_LOG_DETAILS,  &WebConfigLogging::OnBnClickedUseLogLevel)
  ON_BN_CLICKED(IDC_USE_ROTATION,     &WebConfigLogging::OnBnClickedUseRotation)
  ON_BN_CLICKED(IDC_USE_PERUSER,      &WebConfigLogging::OnBnClickedUserPerUser)
END_MESSAGE_MAP()

BOOL
WebConfigLogging::OnInitDialog()
{
  CDialogEx::OnInitDialog();

  m_comboLogLevel.AddString("No logging");
  m_comboLogLevel.AddString("Errors and warnings");
  m_comboLogLevel.AddString("Info logging");
  m_comboLogLevel.AddString("Info & bodies");
  m_comboLogLevel.AddString("Tracing");
  m_comboLogLevel.AddString("Tracing & HEX-Dumping");

  UpdateData(FALSE);
  return TRUE;
}

void
WebConfigLogging::ReadWebConfig(MarlinConfig& config)
{
  // LOGFILE OVERRIDES
  m_useLogfile    = config.HasParameter("Logging","Logfile");
  m_useLogCaching = config.HasParameter("Logging","Cache");
  m_useLogging    = config.HasParameter("Logging","DoLogging");
  m_useLogTiming  = config.HasParameter("Logging","DoTiming");
  m_useLogEvents  = config.HasParameter("Logging","DoEvents");
  m_useLogLevel   = config.HasParameter("Logging","LogLevel");
  m_useRotate     = config.HasParameter("Logging","Rotate");
  m_usePerUser    = config.HasParameter("Logging","PerUser");

  m_logfile       = config.GetParameterString ("Logging","Logfile",         "");
  m_logCache      = config.GetParameterInteger("Logging","Cache",            0);
  m_doLogging     = config.GetParameterBoolean("Logging","DoLogging",    false);
  m_doTiming      = config.GetParameterBoolean("Logging","DoTiming",     false);
  m_doEvents      = config.GetParameterBoolean("Logging","DoEvents",     false);
  m_logLevel      = config.GetParameterInteger("Logging","LogLevel",         0);
  m_doRotate      = config.GetParameterBoolean("Logging","Rotate",       false);
  m_doPerUser     = config.GetParameterBoolean("Logging","PerUser",      false);

  // INIT THE CHECKBOXES
  m_buttonLogging .SetCheck(m_doLogging);
  m_buttonTiming  .SetCheck(m_doTiming);
  m_buttonEvents  .SetCheck(m_doEvents);
  m_comboLogLevel .SetCurSel(m_logLevel);
  m_buttonRotation.SetCheck(m_doRotate);
  m_buttonPerUser .SetCheck(m_doPerUser);

  // INIT ALL USING FIELDS
  m_buttonUseLogfile   .SetCheck(m_useLogfile);
  m_buttonUseLogCaching.SetCheck(m_useLogCaching);
  m_buttonUseLogging   .SetCheck(m_useLogging);
  m_buttonUseLogTiming .SetCheck(m_useLogTiming);
  m_buttonUseLogEvents .SetCheck(m_useLogEvents);
  m_buttonUseLogLevel  .SetCheck(m_useLogLevel);
  m_buttonUseRotation  .SetCheck(m_useRotate);
  m_buttonUsePerUser   .SetCheck(m_usePerUser);

  UpdateData(FALSE);
}

void
WebConfigLogging::WriteWebConfig(MarlinConfig& config)
{
  config.SetSection("Logging");

  if(m_useLogfile)    config.SetParameter   ("Logging","Logfile",  m_logfile);
  else                config.RemoveParameter("Logging","Logfile");
  if(m_useLogCaching) config.SetParameter   ("Logging","Cache",    m_logCache);
  else                config.RemoveParameter("Logging","Cache");
  if(m_useLogging)    config.SetParameter   ("Logging","DoLogging",m_doLogging);
  else                config.RemoveParameter("Logging","DoLogging");
  if(m_useLogTiming)  config.SetParameter   ("Logging","DoTiming", m_doTiming);
  else                config.RemoveParameter("Logging","DoTiming");
  if(m_useLogEvents)  config.SetParameter   ("Logging","DoEvents", m_doEvents);
  else                config.RemoveParameter("Logging","DoEvents");
  if(m_useLogLevel)   config.SetParameter   ("Logging","LogLevel", m_logLevel);
  else                config.RemoveParameter("Logging","LogLevel");
  if(m_useRotate)     config.SetParameter   ("Logging","Rotate",   m_doRotate);
  else                config.RemoveParameter("Logging","Rotate");
  if(m_usePerUser)    config.SetParameter   ("Logging","PerUser",  m_doPerUser);
  else                config.RemoveParameter("Logging","PerUsër");
}

// WebConfigDlg message handlers

//////////////////////////////////////////////////////////////////////////
//
// LOGGING OVERRIDES
//
//////////////////////////////////////////////////////////////////////////

void WebConfigLogging::OnEnChangeLogfile()
{
  UpdateData();
}

void WebConfigLogging::OnBnClickedButtLogfile()
{
  DocFileDialog file(GetSafeHwnd()
                    ,true
                    ,"Enter the path to the logfile"
                    ,"txt"
                    ,m_logfile
                    ,0
                    ,"Text log files (*.txt)|*.txt|"
                    "All file types (*.*)|*.*|"
                    ,"");
  if(file.DoModal() == IDOK)
  {
    XString pad = file.GetChosenFile();
    if(m_logfile.CompareNoCase(pad))
    {
      m_logfile = pad;
      UpdateData(FALSE);
    }
  }
}

void WebConfigLogging::OnEnChangeLogCachelines()
{
  UpdateData();
}

void WebConfigLogging::OnBnClickedLogging()
{
  m_doLogging = m_buttonLogging.GetCheck() > 0;
}

void WebConfigLogging::OnBnClickedLogTiming()
{
  m_doTiming = m_buttonTiming.GetCheck() > 0;
}

void WebConfigLogging::OnBnClickedLogEvents()
{
  m_doEvents = m_buttonEvents.GetCheck() > 0;
}

void WebConfigLogging::OnCbnSelchangeLogLevel()
{
  int ind = m_comboLogLevel.GetCurSel();
  if(ind >= 0)
  {
    m_logLevel = ind;
  }
}

void
WebConfigLogging::OnBnClickedRotation()
{
  m_doRotate = m_buttonRotation.GetCheck() > 0;
}

void
WebConfigLogging::OnBnClickedPerUser()
{
  m_doPerUser = m_buttonPerUser.GetCheck() > 0;
}

//////////////////////////////////////////////////////////////////////////
//
// USING FIELDS EVENTS
//
//////////////////////////////////////////////////////////////////////////

void 
WebConfigLogging::OnBnClickedUseLogfile()
{
  m_useLogfile = m_buttonUseLogfile.GetCheck() > 0;
  UpdateData(FALSE);
}

void 
WebConfigLogging::OnBnClickedUseCaching()
{
  m_useLogCaching = m_buttonUseLogCaching.GetCheck() > 0;
  UpdateData(FALSE);
}

void 
WebConfigLogging::OnBnClickedUseLogging()
{
  m_useLogging = m_buttonUseLogging.GetCheck() > 0;
  UpdateData(FALSE);
}

void 
WebConfigLogging::OnBnClickedUseLogTiming()
{
  m_useLogTiming = m_buttonUseLogTiming.GetCheck() > 0;
  UpdateData(FALSE);
}

void 
WebConfigLogging::OnBnClickedUseLogEvents()
{
  m_useLogEvents = m_buttonUseLogEvents.GetCheck() > 0;
  UpdateData(FALSE);
}

void 
WebConfigLogging::OnBnClickedUseLogLevel()
{
  m_useLogLevel = m_buttonUseLogLevel.GetCheck() > 0;
  UpdateData(FALSE);
}

void
WebConfigLogging::OnBnClickedUseRotation()
{
  m_useRotate = m_buttonUseRotation.GetCheck() > 0;
  UpdateData(FALSE);
}

void
WebConfigLogging::OnBnClickedUserPerUser()
{
  m_usePerUser = m_buttonUsePerUser.GetCheck() > 0;
  UpdateData(FALSE);
}

