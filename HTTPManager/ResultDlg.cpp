/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: ResultDlg.cpp
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
#include "HTTPManager.h"
#include "ResultDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// ResultDlg dialog

IMPLEMENT_DYNAMIC(ResultDlg, CDialogEx)

ResultDlg::ResultDlg(CWnd* pParent,CString p_result)
          :CDialogEx(ResultDlg::IDD, pParent)
          ,m_result(p_result)
{
}

ResultDlg::~ResultDlg()
{
}

void ResultDlg::DoDataExchange(CDataExchange* pDX)
{
  CDialogEx::DoDataExchange(pDX);
  DDX_Text(pDX,IDC_RESULT,m_result);
}

BEGIN_MESSAGE_MAP(ResultDlg, CDialogEx)
  ON_EN_CHANGE(IDC_RESULT, &ResultDlg::OnEnChangeResult)
END_MESSAGE_MAP()

BOOL
ResultDlg::OnInitDialog()
{
  CDialogEx::OnInitDialog();

  LOGFONT  lgFont;

  lgFont.lfCharSet        = DEFAULT_CHARSET;
  lgFont.lfClipPrecision  = 0;
  lgFont.lfEscapement     = 0;
  strcpy_s(lgFont.lfFaceName,32,"Courier new");
  lgFont.lfHeight         = 100;
  lgFont.lfItalic         = 0;
  lgFont.lfOrientation    = 0;
  lgFont.lfOutPrecision   = 0;
  lgFont.lfPitchAndFamily = 2;
  lgFont.lfQuality        = 0;
  lgFont.lfStrikeOut      = 0;
  lgFont.lfUnderline      = 0;
  lgFont.lfWidth          = 0;
  lgFont.lfWeight         = FW_NORMAL;

  if(m_font.m_hObject)
  {
    m_font.DeleteObject();
  }
  m_font.CreatePointFontIndirect(&lgFont);

  CEdit* edit = (CEdit*) GetDlgItem(IDC_RESULT);
  edit->SetFont(&m_font);

  return TRUE;
}

// ResultDlg message handlers


void 
ResultDlg::OnEnChangeResult()
{
  UpdateData();
}
