/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: ColorEdit.cpp
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
#include "ColorEdit.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// ColorEdit

ColorEdit::ColorEdit() 
          :m_colorBackground(RGB(152,188,231))
          ,m_bkBrush(m_colorBackground)
          ,m_bkEmptyBrush(m_colorBackground)
{
  m_pFont             = new CFont;
  m_fontSize          = 110;         // 11 punts font
  m_colorText         = RGB(0,0,0);  // Black
  m_colorTextEmpty    = RGB(0x7F,0x7F,0x7F); // gray
  m_colorPasswordEye  = RGB(0x7f,0x7f,0x7F); // gray
  m_italic            = false;
  m_bold              = false;
  m_focus             = false;
  m_over              = false;
  m_underLine         = false;
  m_fontName          = EDIT_DEFAULT_FONT;
  m_language          = ENGLISH;
  m_empty             = true;
  m_isPassword        = false;
  m_pixelShift        = false;

  m_colorBorderInn         = RGB( 81, 81,233);
  m_colorBorderOut         = RGB( 81, 81,233);
  m_colorBorderDisabledInn = RGB(255,255,255);
  m_colorBorderDisabledOut = RGB(133,133,133);
  m_colorBorderFocusInn    = RGB(255,  0,  0);  // hover and focus
  m_colorBorderFocusOut    = RGB(255,  0,  0);
  m_colorBorderNonFocusInn = RGB(  0,255,  0);  // Hover over - no focus
  m_colorBorderNonFocusOut = RGB(  0,255,  0);
  m_colorBackgroundEmpty   = RGB(152,188,231);
}

ColorEdit::~ColorEdit()
{
  if(m_pFont)
  {
    delete m_pFont;
    m_pFont = NULL;
  }
}

BEGIN_MESSAGE_MAP(ColorEdit, CEdit)
  ON_WM_PAINT()
  ON_WM_MOUSEMOVE()
  ON_WM_LBUTTONUP()
  ON_WM_LBUTTONDOWN()
  ON_MESSAGE(WM_MOUSEHOVER,           OnMouseHover)
  ON_MESSAGE(WM_MOUSELEAVE,           OnMouseLeave)
  ON_CONTROL_REFLECT_EX(EN_KILLFOCUS, OnKillfocus)
  ON_CONTROL_REFLECT   (EN_SETFOCUS,  OnSetfocus)
  ON_WM_CTLCOLOR_REFLECT()
  ON_WM_CHAR()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// ColorEdit message handlers
void 
ColorEdit::SetTextColor(COLORREF p_colorText)
{
  m_colorText = p_colorText;
}

void 
ColorEdit::SetTextColorEmpty(COLORREF p_colorText)
{
  m_colorTextEmpty = p_colorText;
}

void 
ColorEdit::SetFontSize(int p_size)
{
  m_fontSize = p_size;
  ResetFont();
}

void 
ColorEdit::SetFontStyle(bool p_bold
                       ,bool p_italic
                       ,bool p_underLine)
{
  m_bold       = p_bold;
  m_italic     = p_italic;
  m_underLine  = p_underLine;
  ResetFont();
}

void 
ColorEdit::SetBkColor(COLORREF p_colorBackground)
{
  m_colorBackground = p_colorBackground;
  if( m_bkBrush.m_hObject )
  {
    m_bkBrush.DeleteObject();
  }	
  m_bkBrush.CreateSolidBrush(m_colorBackground);
}

void
ColorEdit::SetBkColorEmpty(COLORREF p_colorEmptyBackground)
{
  m_colorBackgroundEmpty = p_colorEmptyBackground;
  if(m_bkEmptyBrush.m_hObject)
  {
    m_bkEmptyBrush.DeleteObject();
  }
  m_bkEmptyBrush.CreateSolidBrush(m_colorBackgroundEmpty);
}

void 
ColorEdit::SetPasswordEyeColor(COLORREF p_colorPasswordEye)
{
  m_colorPasswordEye = p_colorPasswordEye;
  DrawPasswordEye();
}

void 
ColorEdit::SetFontName(XString p_fontName,BYTE p_language /* = ENGLISH */)
{
  m_fontName = p_fontName;
  m_language = p_language;
  ResetFont();
}

void 
ColorEdit::SetBorderColor(COLORREF p_inner,COLORREF p_outer)
{
  m_colorBorderInn = p_inner;
  m_colorBorderOut = p_outer;
  DrawEditFrame();
}

void 
ColorEdit::SetBorderColorDisabled(COLORREF p_inner,COLORREF p_outer)
{
  m_colorBorderDisabledInn = p_inner;
  m_colorBorderDisabledOut = p_outer;
  DrawEditFrame();
}

void 
ColorEdit::SetBorderColorFocus(COLORREF p_inner,COLORREF p_outer)
{
  m_colorBorderFocusInn = p_inner;
  m_colorBorderFocusOut = p_outer;
  DrawEditFrame();
}

void 
ColorEdit::SetBorderColorNonFocus(COLORREF p_inner,COLORREF p_outer)
{
  m_colorBorderNonFocusInn = p_inner;
  m_colorBorderNonFocusOut = p_outer;
  DrawEditFrame();
}

void
ColorEdit::SetDoPixelShift(bool p_shift)
{
  m_pixelShift = p_shift;
}

void
ColorEdit::SetEmpty(bool p_empty, XString p_text)
{
  if(p_empty && p_text.IsEmpty())
  {
    p_text = EDIT_EMPTYFIELD;
  }
  m_empty     = p_empty;
  m_emptyText = p_text;
  SetWindowText(p_text);
}

void 
ColorEdit::DrawEditFrame()
{
  CRect rect;
  CDC* dc=this->GetDC();
  this->GetClientRect(&rect);

  if(m_pixelShift)
  {
    // Do the pixel shift. 
    rect.top  -= EDIT_PIXEL_SHIFT;

    // Hierdoor valt er een gat tussen de bovenkant van de edit box en de tekst.
    // Deze wordt opgevuld met de huidige achtergrondkleur
    COLORREF back = m_empty ? m_colorBackgroundEmpty : m_colorBackground;
    CRect clearme(rect);
    clearme.InflateRect(-1,-1);
    clearme.bottom = clearme.top + EDIT_PIXEL_SHIFT;
    dc->FillSolidRect(clearme,back);
  }
  else
  {
    // Inner border is drawn outside the client area
    rect.InflateRect(1,1);
  }

  if(!this->IsWindowEnabled())
  {
    dc->Draw3dRect(rect,m_colorBorderDisabledInn,m_colorBorderDisabledInn);
    rect.InflateRect(1,1);
    dc->Draw3dRect(rect,m_colorBorderDisabledOut,m_colorBorderDisabledOut);
    ReleaseDC(dc);
    return;
  }
  if(m_over)
  {
    if(m_focus)
    {
      dc->Draw3dRect(rect,m_colorBorderFocusInn,m_colorBorderFocusInn);
      rect.InflateRect(1,1);
      dc->Draw3dRect(rect,m_colorBorderFocusOut,m_colorBorderFocusOut);
    }
    else
    {
      dc->Draw3dRect( rect,m_colorBorderNonFocusInn,m_colorBorderNonFocusInn);
      rect.InflateRect(1,1);
      dc->Draw3dRect( rect,m_colorBorderNonFocusOut,m_colorBorderNonFocusOut);
    }
  }
  else // !m_over
  {
    dc->Draw3dRect(rect,m_colorBorderInn,m_colorBorderInn);
    rect.InflateRect(1,1);
    dc->Draw3dRect(rect,m_colorBorderOut,m_colorBorderOut);
  }
  ReleaseDC(dc);

  DrawPasswordEye();
}

void
ColorEdit::DrawPasswordEye()
{
  // Only draw a password eye if we are a password field
  if(!m_isPassword)
  {
    return;
  }

  // Getting the rectangle of the input field
  CRect rect;
  CDC* dc = this->GetDC();
  this->GetClientRect(&rect);

  // Bounding rectangle of the eye
  int top    = rect.Height() / 8;
  int left   = rect.right - rect.Height() + top;
  int bottom = rect.Height() - top;
  int right  = rect.right    - top;

  // Start/end of the eyebrow
  int startx = right;
  int starty = (bottom - top) / 2;
  int endx   = left;
  int endy   = starty;

  // Take a colored pen
  CPen pen;
  pen.CreatePen(PS_SOLID,EDIT_EYE_WEIGHT,m_colorPasswordEye);
  CPen* oldpen = dc->SelectObject(&pen);

  // Draw the eyebrow
  dc->Arc(left,top,right,bottom,startx,starty,endx,endy);

  // Size of the eye circle
  int inner = WS(EDIT_INNER_EYE);
  left   += inner;
  right  -= inner;
  top    += inner;
  bottom -= inner;

  // Draw Circle
  dc->Ellipse(left,top,right,bottom);

  // Done, restore pen and DC
  dc->SelectObject(oldpen);
  ReleaseDC(dc);
}

void 
ColorEdit::ResetFont()
{
  LOGFONT  lgFont;

  lgFont.lfCharSet        = m_language;
  lgFont.lfClipPrecision  = 0;
  lgFont.lfEscapement     = 0;
  strcpy_s(lgFont.lfFaceName,32,m_fontName);
  lgFont.lfHeight         = m_fontSize;
  lgFont.lfItalic         = m_italic;
  lgFont.lfOrientation    = 0;
  lgFont.lfOutPrecision   = 0;
  lgFont.lfPitchAndFamily = 2;
  lgFont.lfQuality        = 0;
  lgFont.lfStrikeOut      = 0;
  lgFont.lfUnderline      = m_underLine;
  lgFont.lfWidth          = 0;
  if(m_bold )
  {
    lgFont.lfWeight = FW_BOLD;
  }
  else
  {
    lgFont.lfWeight = FW_MEDIUM;
  }
  if(m_pFont->m_hObject)
  {
    m_pFont->DeleteObject();
  }
  m_pFont->CreatePointFontIndirect(&lgFont);
  SetFont(m_pFont);
}

void 
ColorEdit::OnMouseMove(UINT nFlags, CPoint point) 
{
  TRACKMOUSEEVENT mouseEvent;
  
  mouseEvent.cbSize      = sizeof(TRACKMOUSEEVENT);
  mouseEvent.dwFlags     = TME_HOVER | TME_LEAVE;
  mouseEvent.dwHoverTime = 1;
  mouseEvent.hwndTrack   = m_hWnd;

  _TrackMouseEvent(&mouseEvent);

  CEdit::OnMouseMove(nFlags, point);
}

BOOL
ColorEdit::OnKillfocus() 
{
  m_focus = true;
  DrawEditFrame();

  // Sta toe dat Dialog ook KILLFOCUS afhandelt.
  return FALSE;
}

void 
ColorEdit::OnSetfocus() 
{
  m_focus = true;
  DrawEditFrame();
  if(m_empty)
  {
    SetSel(0,0,TRUE);
  }
}

LRESULT 
ColorEdit::OnMouseHover(WPARAM wParam,LPARAM lParam)
{
  m_over = true;
  DrawEditFrame();
  return 0;
}

LRESULT 
ColorEdit::OnMouseLeave(WPARAM wParam,LPARAM lParam)
{
  m_over = false;
  DrawEditFrame();
  return 0;
}

void
ColorEdit::OnLButtonDown(UINT nFlags,CPoint point)
{
  CRect rcItem;
  this->GetClientRect(&rcItem);

  // Only if the mouse is above the password eye do we reveal the password
  if(m_isPassword && point.y <= rcItem.bottom && point.x >= (rcItem.right - rcItem.Height()))
  {
    SendMessage(EM_SETPASSWORDCHAR,0,0);
  }
  CWnd::OnLButtonDown(nFlags,point);
}

void
ColorEdit::OnLButtonUp(UINT nFlags,CPoint point)
{
  CRect rcItem;
  this->GetClientRect(&rcItem);

  if(m_isPassword)
  {
    // Re-hide the password
    SendMessage(EM_SETPASSWORDCHAR,'o',0);
    Invalidate();
  }
  CWnd::OnLButtonUp(nFlags,point);
}

void
ColorEdit::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags)
{
  if(nChar && m_empty)
  {
    if(m_isPassword)
    {
      // Hide empty text and start a password
      SetEmpty(false,"");
      SendMessage(EM_SETPASSWORDCHAR,'o',0);
      DrawEditFrame();
    }
    else
    {
      XString text;
      CEdit::GetWindowText(text);
      if(text.Compare(EDIT_EMPTYFIELD) == 0)
      {
        CEdit::SetWindowText("");
      }
      SetEmpty(false,"");
    }
  }
  CEdit::OnChar(nChar,nRepCnt,nFlags);
  DrawPasswordEye();
}

// Return a non-NULL brush if the parent's handler should not be called
HBRUSH 
ColorEdit::CtlColor(CDC* pDC, UINT nCtlColor) 
{
  DrawEditFrame();
  // Change attributes of the DC here
  pDC->SetBkMode(TRANSPARENT);
  if(m_empty)
  {
    pDC->SetTextColor(m_colorTextEmpty); 
    pDC->SetBkColor  (m_colorBackgroundEmpty);
    return m_bkEmptyBrush;
  }
  else
  {
    pDC->SetTextColor(m_colorText);
    pDC->SetBkColor  (m_colorBackground);
    return m_bkBrush;
  }
}

void
ColorEdit::OnPaint()
{
  CWnd::OnPaint();
  DrawEditFrame();
  DrawPasswordEye();
}
