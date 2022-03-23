/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: WebConfigWServices.cpp
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
#include "WebConfigWServices.h"
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

IMPLEMENT_DYNAMIC(WebConfigWServices, CDialogEx)

WebConfigWServices::WebConfigWServices(bool p_iis,CWnd* pParent /*=NULL*/)
                   :CDialogEx(WebConfigWServices::IDD, pParent)
                   ,m_iis(p_iis)
{
  m_checkWSDLin       = false;
  m_checkWSDLout      = false;
  m_fieldCheck        = false;
  m_reliable          = false;
  m_reliableLogin     = true;
  m_useEncLevel       = false;
  m_useEncPassword    = false;
  m_useReliable       = false;
  m_useReliableLogin  = false;
  m_useCheckWSDLIn    = false;
  m_useCheckWSDLOut   = false;
  m_useFieldCheck     = false;
}

WebConfigWServices::~WebConfigWServices()
{
}

void WebConfigWServices::DoDataExchange(CDataExchange* pDX)
{
  CDialogEx::DoDataExchange(pDX);
  // USING FIELDS
  DDX_Control(pDX,IDC_USE_WS_ENCRYPT, m_buttonUseEncLevel);
  DDX_Control(pDX,IDC_USE_WSENC_PW,   m_buttonUseEncPassword);
  DDX_Control(pDX,IDC_USE_RELIABLE,   m_buttonUseReliable);
  DDX_Control(pDX,IDC_USE_RMLOGIN,    m_buttonUseReliableLogin);
  DDX_Control(pDX,IDC_USE_CHECK_IN,   m_buttonUseCheckWSDLin);
  DDX_Control(pDX,IDC_USE_CHECK_OUT,  m_buttonUseCheckWSDLout);
  DDX_Control(pDX,IDC_USE_FIELDCHECK, m_buttonUseFieldCheck);

  // WEBSERVICE OVERRIDES
  DDX_Control(pDX,IDC_WS_ENCRYPT,     m_comboEncryption);
  DDX_Text   (pDX,IDC_WSENC_PASSWORD, m_encPassword);
  DDX_Control(pDX,IDC_RELIABLE,       m_buttonReliable);
  DDX_Control(pDX,IDC_RMLOGIN,        m_buttonReliableLogin);
  DDX_Control(pDX,IDC_CHECK_IN,       m_buttonCheckWSDLin);
  DDX_Control(pDX,IDC_CHECK_OUT,      m_buttonCheckWSDLout);
  DDX_Control(pDX,IDC_FIELDCHECK,     m_buttonFieldCheck);

  if(pDX->m_bSaveAndValidate == FALSE)
  {
    CWnd* w = nullptr;
    
    w = GetDlgItem(IDC_WSENC_PASSWORD); w->EnableWindow(m_useEncPassword);
    m_comboEncryption    .EnableWindow(m_useEncLevel);
    m_buttonReliable     .EnableWindow(m_useReliable);
    m_buttonReliableLogin.EnableWindow(m_useReliableLogin);
    m_buttonCheckWSDLin  .EnableWindow(m_useCheckWSDLIn);
    m_buttonCheckWSDLout .EnableWindow(m_useCheckWSDLOut);
    m_buttonFieldCheck   .EnableWindow(m_useFieldCheck);
  }
}

BEGIN_MESSAGE_MAP(WebConfigWServices, CDialogEx)
  // WEBSERVICE OVERRIDES
  ON_CBN_SELCHANGE(IDC_WS_ENCRYPT,    &WebConfigWServices::OnCbnSelchangeWsEncrypt)
  ON_EN_CHANGE    (IDC_WSENC_PASSWORD,&WebConfigWServices::OnEnChangeWsencPassword)
  ON_BN_CLICKED   (IDC_RELIABLE,      &WebConfigWServices::OnBnClickedReliable)
  ON_BN_CLICKED   (IDC_RMLOGIN,       &WebConfigWServices::OnBnClickedReliableLogin)
  ON_BN_CLICKED   (IDC_CHECK_IN,      &WebConfigWServices::OnBnClickedCheckWSDLin)
  ON_BN_CLICKED   (IDC_CHECK_OUT,     &WebConfigWServices::OnBnClickedCheckWSDLout)
  ON_BN_CLICKED   (IDC_FIELDCHECK,    &WebConfigWServices::OnBnClickedFieldCheck)

  // USING BUTTONS
  ON_BN_CLICKED(IDC_USE_WS_ENCRYPT,   &WebConfigWServices::OnBnClickedUseWsEncrypt)
  ON_BN_CLICKED(IDC_USE_WSENC_PW,     &WebConfigWServices::OnBnClickedUseWsencPw)
  ON_BN_CLICKED(IDC_USE_RELIABLE,     &WebConfigWServices::OnBnClickedUseReliable)
  ON_BN_CLICKED(IDC_USE_RMLOGIN,      &WebConfigWServices::OnBnClickedUseReliableLogin)
  ON_BN_CLICKED(IDC_USE_CHECK_IN,     &WebConfigWServices::OnBnClickedUseCheckWSDLin)
  ON_BN_CLICKED(IDC_USE_CHECK_OUT,    &WebConfigWServices::OnBnClickedUseCheckWSDLout)
  ON_BN_CLICKED(IDC_USE_FIELDCHECK,   &WebConfigWServices::OnBnClickedUseFieldCheck)
END_MESSAGE_MAP()

BOOL
WebConfigWServices::OnInitDialog()
{
  CDialogEx::OnInitDialog();

  InitComboboxes();

  UpdateData(FALSE);
  return TRUE;
}

void
WebConfigWServices::InitComboboxes()
{
  // Encryption levels
  m_comboEncryption.AddString("Signing");
  m_comboEncryption.AddString("Body encrypted");
  m_comboEncryption.AddString("Message encrypted");
}

void
WebConfigWServices::ReadWebConfig(MarlinConfig& config)
{
  // WEBSERVICE OVERRIDES
  m_useEncLevel       = config.HasParameter("Encryption", "Level");
  m_useEncPassword    = config.HasParameter("Encryption", "Password");
  m_useReliable       = config.HasParameter("WebServices","Reliable");
  m_useReliableLogin  = config.HasParameter("WebServices","ReliableLogin");
  m_useCheckWSDLIn    = config.HasParameter("WebServices","CheckWSDLIncoming");
  m_useCheckWSDLOut   = config.HasParameter("WebServices","CheckWSDLOutgoing");
  m_useFieldCheck     = config.HasParameter("WebServices","CheckFieldValues");

  m_encLevel          = config.GetParameterString ("Encryption", "Level",   "");
  m_encPassword       = config.GetEncryptedString ("Encryption", "Password","");
  m_reliable          = config.GetParameterBoolean("WebServices","Reliable",         false);
  m_reliableLogin     = config.GetParameterBoolean("WebServices","ReliableLogin",    true);
  m_checkWSDLin       = config.GetParameterBoolean("WebServices","CheckWSDLIncoming",false);
  m_checkWSDLout      = config.GetParameterBoolean("WebServices","CheckWSDLOutgoing",false);
  m_fieldCheck        = config.GetParameterBoolean("WebServices","CheckFieldValues", false);


  // Encryption levels
  if(m_encLevel.CompareNoCase("sign")    == 0) m_comboEncryption.SetCurSel(0);
  if(m_encLevel.CompareNoCase("body")    == 0) m_comboEncryption.SetCurSel(1);
  if(m_encLevel.CompareNoCase("message") == 0) m_comboEncryption.SetCurSel(2);

  // INIT THE CHECKBOXES
  m_buttonCheckWSDLin  .SetCheck(m_checkWSDLin);
  m_buttonCheckWSDLout .SetCheck(m_checkWSDLout);
  m_buttonFieldCheck   .SetCheck(m_fieldCheck);
  m_buttonReliable     .SetCheck(m_reliable);
  m_buttonReliableLogin.SetCheck(m_reliableLogin);

  // INIT ALL USING FIELDS
  m_buttonUseEncLevel     .SetCheck(m_useEncLevel);
  m_buttonUseEncPassword  .SetCheck(m_useEncPassword);
  m_buttonUseCheckWSDLin  .SetCheck(m_useCheckWSDLIn);
  m_buttonUseCheckWSDLout .SetCheck(m_useCheckWSDLOut);
  m_buttonUseFieldCheck   .SetCheck(m_useFieldCheck);
  m_buttonUseReliable     .SetCheck(m_useReliable);
  m_buttonUseReliableLogin.SetCheck(m_useReliableLogin);

  UpdateData(FALSE);
}

void
WebConfigWServices::WriteWebConfig(MarlinConfig& config)
{

  // WRITE THE CONFIG PARAMETERS

  config.SetSection("Encryption");

  if(m_useEncLevel)     config.SetParameter   ("Encryption","Level",   m_encLevel);
  else                  config.RemoveParameter("Encryption","Level");
  if(m_useEncPassword)  config.SetEncrypted   ("Encryption","Password",m_encPassword);
  else                  config.RemoveParameter("Encryption","Password");

  config.SetSection("WebServices");

  if(m_useCheckWSDLIn)  config.SetParameter   ("WebServices","CheckWSDLIncoming", m_checkWSDLin);
  else                  config.RemoveParameter("WebServices","CheckWSDLIncoming");
  if(m_useCheckWSDLOut) config.SetParameter   ("WebServices","CheckWSDLOutgoing", m_checkWSDLout);
  else                  config.RemoveParameter("WebServices","CheckWSDLOutgoing");
  if(m_useFieldCheck)   config.SetParameter   ("WebServices","CheckFieldValues",  m_fieldCheck);
  else                  config.RemoveParameter("WebServices","CheckFieldValues");
  if(m_useReliable)     config.SetParameter   ("WebServices","Reliable",          m_reliable);
  else                  config.RemoveParameter("WebServices","Reliable");
  if(m_useReliableLogin)config.SetParameter   ("WebServices","ReliableLogin",     m_reliableLogin);
  else                  config.RemoveParameter("WebServices","ReliableLogin");
}

// WebConfigDlg message handlers

//////////////////////////////////////////////////////////////////////////
//
// SERVER OVERRIDES
//
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
//
// MESSAGE ENCRYPTION
//
//////////////////////////////////////////////////////////////////////////

void WebConfigWServices::OnCbnSelchangeWsEncrypt()
{
  int sel = m_comboEncryption.GetCurSel();
  if(sel >= 0)
  {
    switch(sel)
    {
      case 0: m_encLevel = "sign";    break;
      case 1: m_encLevel = "body";    break;
      case 2: m_encLevel = "message"; break;
      default:m_encLevel.Empty();     break;
    }
  }
  else
  {
    m_encLevel.Empty();
  }
  // Check for no encryption = no password
  if(m_encLevel.IsEmpty())
  {
    m_encPassword.Empty();
    UpdateData(FALSE);
  }
}

void WebConfigWServices::OnEnChangeWsencPassword()
{
  UpdateData();
}

//////////////////////////////////////////////////////////////////////////
//
// WEBSERVICE OVERRIDES
//
//////////////////////////////////////////////////////////////////////////

void WebConfigWServices::OnBnClickedReliable()
{
  m_reliable = m_buttonReliable.GetCheck() > 0;
}

void WebConfigWServices::OnBnClickedReliableLogin()
{
  m_reliableLogin = m_buttonReliableLogin.GetCheck() > 0;
}

void WebConfigWServices::OnBnClickedCheckWSDLin()
{
  m_checkWSDLin = m_buttonCheckWSDLin.GetCheck() > 0;
}

void WebConfigWServices::OnBnClickedCheckWSDLout()
{
  m_checkWSDLout = m_buttonCheckWSDLout.GetCheck() > 0;
}

void WebConfigWServices::OnBnClickedFieldCheck()
{
  m_fieldCheck = m_buttonFieldCheck.GetCheck() > 0;
}

//////////////////////////////////////////////////////////////////////////
//
// LOGGING OVERRIDES
//
//////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////
//
// USING FIELDS EVENTS
//
//////////////////////////////////////////////////////////////////////////

void 
WebConfigWServices::OnBnClickedUseWsEncrypt()
{
  m_useEncLevel = m_buttonUseEncLevel.GetCheck() > 0;
  UpdateData(FALSE);
}

void 
WebConfigWServices::OnBnClickedUseWsencPw()
{
  m_useEncPassword = m_buttonUseEncPassword.GetCheck() > 0;
  UpdateData(FALSE);
}

void
WebConfigWServices::OnBnClickedUseReliable()
{
  m_useReliable = m_buttonUseReliable.GetCheck() > 0;
  UpdateData(FALSE);
}

void
WebConfigWServices::OnBnClickedUseReliableLogin()
{
  m_useReliableLogin = m_buttonUseReliableLogin.GetCheck() > 0;
  UpdateData(FALSE);
}

void
WebConfigWServices::OnBnClickedUseCheckWSDLin()
{
  m_useCheckWSDLIn = m_buttonUseCheckWSDLin.GetCheck() > 0;
  UpdateData(FALSE);
}

void
WebConfigWServices::OnBnClickedUseCheckWSDLout()
{
  m_useCheckWSDLOut = m_buttonUseCheckWSDLout.GetCheck() > 0;
  UpdateData(FALSE);
}

void
WebConfigWServices::OnBnClickedUseFieldCheck()
{
  m_useFieldCheck = m_buttonUseFieldCheck.GetCheck() > 0;
  UpdateData(FALSE);
}

