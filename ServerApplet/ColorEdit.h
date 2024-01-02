/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: ColorEdit.h
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

/////////////////////////////////////////////////////////////////////////////
// ColorEdit window

// Some default character sets from the WinGDI
#define   CHINESE   GB2312_CHARSET
#define   ENGLISH   DEFAULT_CHARSET
// Number of DLU's the inner eye is within the eyebrow
#define   EDIT_INNER_EYE  5
#define   EDIT_EYE_WEIGHT 2
// Default font name
#define   EDIT_DEFAULT_FONT "Arial"
// Special measures
#define   EDIT_PIXEL_SHIFT  4
// Empty field
#define   EDIT_EMPTYFIELD "<EMPTY>"


// Macro to catch FONT scaling
#ifndef EDIT_NOSCALING
#define WS(val)    MulDiv((val##),CWindowDC(0).GetDeviceCaps(LOGPIXELSY),96)
#else
#define WS(val)   (val##)
#endif

class ColorEdit : public CEdit
{
// Construction
public:
  ColorEdit();
  virtual ~ColorEdit();

// Operations
public:
  void SetTextColor(COLORREF p_colorText);
  void SetTextColorEmpty(COLORREF p_colorText);
  void SetFontSize(int p_size );
  void SetFontStyle(bool p_bold,bool p_italic = false,bool p_underLine = false);
  void SetBkColor(COLORREF p_colorBackground);
  void SetBkColorEmpty(COLORREF p_colorEmptyBackground);
  void SetPasswordEyeColor(COLORREF p_colorPasswordEye);
  void SetFontName(XString  p_fontName,BYTE language = ENGLISH);
  void SetPassword(bool p_password);
  void SetEmpty(bool p_empty,XString p_text = _T(""));
  void SetDoPixelShift(bool p_shift);

  void SetBorderColor        (COLORREF p_inner,COLORREF p_outer);
  void SetBorderColorDisabled(COLORREF p_inner,COLORREF p_outer);
  void SetBorderColorFocus   (COLORREF p_inner,COLORREF p_outer);
  void SetBorderColorNonFocus(COLORREF p_inner,COLORREF p_outer);

  bool    GetIsEmpty();
  XString GetEmptyText();

public:
  afx_msg void    OnPaint();
  afx_msg BOOL    OnKillfocus();
  afx_msg void    OnSetfocus();
  afx_msg void    OnMouseMove  (UINT   nFlags, CPoint point);
  afx_msg LRESULT OnMouseHover (WPARAM wParam, LPARAM lParam);
  afx_msg LRESULT OnMouseLeave (WPARAM wParam, LPARAM lParam);
  afx_msg void    OnLButtonDown(UINT   nFlags, CPoint point);
  afx_msg void    OnLButtonUp  (UINT   nFlags, CPoint point);

  afx_msg HBRUSH  CtlColor(CDC* pDC, UINT nCtlColor);
  afx_msg void    OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);

  DECLARE_MESSAGE_MAP()
private:
  void     DrawEditFrame();
  void     DrawPasswordEye();
  void     ResetFont();

  int      m_fontSize;
  COLORREF m_colorText;
  COLORREF m_colorTextEmpty;
  COLORREF m_colorBackground;
  COLORREF m_colorBackgroundEmpty;
  COLORREF m_colorPasswordEye;
  bool     m_italic;
  bool     m_bold;
  bool     m_underLine;
  bool     m_focus;
  bool     m_over;
  XString  m_fontName;
  BYTE     m_language;
  CBrush   m_bkBrush;
  CBrush   m_bkEmptyBrush;
  CFont*   m_pFont;
  bool     m_isPassword;
  bool     m_empty;
  XString  m_emptyText;
  bool     m_pixelShift;

  int      m_colorBorderInn;          // Normal border color inside
  int      m_colorBorderOut;          // Normal border color outside
  int      m_colorBorderDisabledInn;  // Disabled border color inside
  int      m_colorBorderDisabledOut;  // Disabled border color outside
  int      m_colorBorderFocusInn;     // Border color mouse-over-and-focus inside
  int      m_colorBorderFocusOut;     // Border color mouse-over-and-focus outside
  int      m_colorBorderNonFocusInn;  // Border color mouse-over-and-not-focus inside
  int      m_colorBorderNonFocusOut;  // Border color mouse-over-and-not-focus outside
};

inline void 
ColorEdit::SetPassword(bool p_password)
{
  m_isPassword = p_password;
  ResetFont();
}

inline bool 
ColorEdit::GetIsEmpty()
{
  return m_empty;
}

inline XString 
ColorEdit::GetEmptyText()
{
  return m_emptyText;
}
