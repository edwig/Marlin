/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: ServerAppletDlg.h
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
#pragma once
#include <atlimage.h>
#include <vector>

using std::vector;

typedef enum _serverStatus
{
  Server_stopped = 1
 ,Server_transit
 ,Server_running
}
ServerStatus;

// ServerAppletDlg dialog
class ServerAppletDlg : public CDialog
{
// Construction
public:
	ServerAppletDlg(CWnd* pParent = NULL);   // standard constructor
 ~ServerAppletDlg();

// Dialog Data
	enum { IDD = IDD_SERVERAPPLET };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
  void            GetConfigVariables();
  void            GetServerStatus();
  void            GetServerStatusLocally();
  bool            CheckConfiguration();
  void            SetInstallMenu(int p_service);
  void            PumpMessage();
  bool            AreYouSure(XString p_actie);
  void            GetWSStatus();
  void            IISRestart();
  void            IISStop();
  void            IISStart();

  XString         m_role;
  int             m_service;
  ServerStatus    m_serverStatus;
  XString         m_status;
  XString         m_logline;
  XString         m_password;
  bool            m_loginStatus;
  XString         m_connection;
  XString         m_url;
  XString         m_loginURL;

	HICON           m_hIcon;
  CButton         m_buttonConfig;
  CButton         m_buttonStart;
  CButton         m_buttonStop;
  CButton         m_buttonFunction1;
  CButton         m_buttonFunction2;
  CButton         m_buttonFunction3;
  CButton         m_buttonFunction4;
  CButton         m_buttonFunction5;
  CButton         m_buttonFunction6;
  CButton         m_buttonFunction7;
  CImage*         m_trafficLight;

	// Generated message map functions
	virtual BOOL    OnInitDialog();
	afx_msg void    OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void    OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
  afx_msg void OnBnClickedConfig();
  afx_msg void OnEnChangeStatus();
  afx_msg void OnBnClickedQuery();
  afx_msg void OnBnClickedStart();
  afx_msg void OnBnClickedStop();
  afx_msg void OnBnClickedBounce();
  afx_msg void OnEnChangeUser();
  afx_msg void OnBnClickedFunction1();
  afx_msg void OnBnClickedFunction2();
  afx_msg void OnBnClickedFunction3();
  afx_msg void OnBnClickedFunction4();
  afx_msg void OnBnClickedFunction5();
  afx_msg void OnBnClickedFunction6();
  afx_msg void OnBnClickedFunction7();
  afx_msg void OnBnClickedOk();
};
