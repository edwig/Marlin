/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: AutoFont.cpp
//
// BaseLibrary: Indispensable general objects and functions
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
#include "pch.h"
#include "BaseLibrary.h"
#include "AutoFont.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

AutoFont::AutoFont()
{
	m_lf.lfHeight         = -12;
	m_lf.lfWidth          = 0;
	m_lf.lfEscapement     = 0;
	m_lf.lfOrientation    = 0;
	m_lf.lfWeight         = FW_NORMAL;
	m_lf.lfItalic         = 0;
	m_lf.lfUnderline      = 0;
	m_lf.lfStrikeOut      = 0;
	m_lf.lfCharSet        = ANSI_CHARSET;
	m_lf.lfOutPrecision   = OUT_DEFAULT_PRECIS;
	m_lf.lfClipPrecision  = CLIP_DEFAULT_PRECIS;
	m_lf.lfQuality        = PROOF_QUALITY;
	m_lf.lfPitchAndFamily = VARIABLE_PITCH | FF_ROMAN;
	strcpy_s(m_lf.lfFaceName,LF_FACESIZE,"Times New Roman");

	CreateFontIndirect(&m_lf);

	m_fontColor = 0;
	m_hDC       = NULL;
}

AutoFont::AutoFont(XString facename)
{
	m_lf.lfHeight         = -12;
	m_lf.lfWidth          = 0;
	m_lf.lfEscapement     = 0;
	m_lf.lfOrientation    = 0;
	m_lf.lfWeight         = FW_NORMAL;
	m_lf.lfItalic         = 0;
	m_lf.lfUnderline      = 0;
	m_lf.lfStrikeOut      = 0;
	m_lf.lfCharSet        = ANSI_CHARSET;
	m_lf.lfOutPrecision   = OUT_DEFAULT_PRECIS;
	m_lf.lfClipPrecision  = CLIP_DEFAULT_PRECIS;
	m_lf.lfQuality        = PROOF_QUALITY;
	m_lf.lfPitchAndFamily = VARIABLE_PITCH | FF_ROMAN;
	strcpy_s(m_lf.lfFaceName,LF_FACESIZE,reinterpret_cast<LPCTSTR>(facename.GetString()));

	CreateFontIndirect(&m_lf);

	m_fontColor = 0;
	m_hDC       = NULL;
}

AutoFont::AutoFont(LOGFONT& logfont)
         :m_lf(logfont)
{
	CreateFontIndirect(&m_lf);

	m_fontColor = 0;
	m_hDC       = NULL;
}

AutoFont::AutoFont(HFONT font)
{
	HFONT hFont = reinterpret_cast<HFONT>(font);
  GetObject(reinterpret_cast<HANDLE>(hFont), sizeof(LOGFONT),&m_lf);

	m_fontColor = 0;
	m_hDC = NULL;
}

AutoFont::~AutoFont()
{
}

LONG AutoFont::SetHeight(LONG height)
{
	LONG l = m_lf.lfHeight;

	DeleteObject(m_font);
	m_lf.lfHeight = height;
	m_font = CreateFontIndirect(&m_lf);

	return l;
}

LONG AutoFont::SetHeightA(LONG height)
{
	LONG l = m_lf.lfHeight;

	DeleteObject(m_font);
  if(height > 0)
  {
    height = 0 - height;
  }
	m_lf.lfHeight = height;
	m_font = CreateFontIndirect(&m_lf);

	return l;
}

LONG AutoFont::SetWidth(LONG width)
{
	LONG l = m_lf.lfWidth;

	DeleteObject(m_font);
	m_lf.lfWidth = width;
	m_font = CreateFontIndirect(&m_lf);

	return l;
}

LONG AutoFont::SetEscapement(LONG esc)
{
	LONG l = m_lf.lfEscapement;

	DeleteObject(m_font);
	m_lf.lfEscapement = esc;
	m_font = CreateFontIndirect(&m_lf);

	return l;
}

LONG AutoFont::SetOrientation(LONG orientation)
{
	LONG l = m_lf.lfOrientation;

	DeleteObject(m_font);
	m_lf.lfOrientation = orientation;
	m_font = CreateFontIndirect(&m_lf);

	return l;
}

LONG AutoFont::SetWeight(LONG weight)
{
	LONG l = m_lf.lfWeight;

	DeleteObject(m_font);
	m_lf.lfWeight = weight;
	m_font = CreateFontIndirect(&m_lf);

	return l;
}

BYTE AutoFont::SetCharset(BYTE charset)
{
	BYTE b = m_lf.lfCharSet;

	DeleteObject(m_font);
	m_lf.lfCharSet = charset;
	m_font = CreateFontIndirect(&m_lf);

	return b;
}

BYTE AutoFont::SetOutPrecision(BYTE op)
{
	BYTE b = m_lf.lfOutPrecision;

	DeleteObject(m_font);
	m_lf.lfOutPrecision=op;
	m_font = CreateFontIndirect(&m_lf);

	return b;
}

BYTE AutoFont::SetClipPrecision(BYTE cp)
{
	BYTE b = m_lf.lfClipPrecision;

	DeleteObject(m_font);
	m_lf.lfClipPrecision = cp;
	m_font = CreateFontIndirect(&m_lf);

	return b;
}

BYTE AutoFont::SetQuality(BYTE qual)
{
	BYTE b = m_lf.lfQuality;

	DeleteObject(m_font);
	m_lf.lfQuality = qual;
	m_font = CreateFontIndirect(&m_lf);

	return b;
}

BYTE AutoFont::SetPitchAndFamily(BYTE paf)
{
	BYTE b = m_lf.lfPitchAndFamily;

	DeleteObject(m_font);
	m_lf.lfPitchAndFamily = paf;
	m_font = CreateFontIndirect(&m_lf);

	return b;
}

XString AutoFont::SetFaceName(XString facename)
{
	XString str = m_lf.lfFaceName;

	DeleteObject(m_font);
	strcpy_s(m_lf.lfFaceName,LF_FACESIZE, facename.GetString());
	m_font = CreateFontIndirect(&m_lf);

	return str;
}

// BEWARE: Not thread safe!
LPCTSTR 
AutoFont::SetFaceName(LPCTSTR facename)
{
  static char buffer[LF_FACESIZE + 1];
  strcpy_s(buffer,LF_FACESIZE,m_lf.lfFaceName);

	DeleteObject(m_font);
	strcpy_s(m_lf.lfFaceName,LF_FACESIZE,facename);
	m_font = CreateFontIndirect(&m_lf);

  return (LPCTSTR) &buffer;
}

BOOL AutoFont::SetBold(BOOL B)
{
	BOOL b;

  if(B)
  {
    b = SetWeight(FW_BOLD);
  }
  else
  {
    b = SetWeight(FW_NORMAL);
  }
  return (b >= FW_MEDIUM);
}

BOOL AutoFont::SetItalic(BOOL i)
{
	BOOL b = (BOOL)m_lf.lfItalic;

	DeleteObject(m_font);
	m_lf.lfItalic = (BYTE)i;
	m_font = CreateFontIndirect(&m_lf);

	return b;
}

BOOL AutoFont::SetUnderline(BOOL u)
{
	BOOL b=(BOOL)m_lf.lfUnderline;

	DeleteObject(m_font);
	m_lf.lfUnderline = (BYTE) u;
	m_font = CreateFontIndirect(&m_lf);

	return b;
}

BOOL AutoFont::SetStrikeOut(BOOL s)
{
	BOOL b=(BOOL)m_lf.lfStrikeOut;

	DeleteObject(m_font);
	m_lf.lfStrikeOut = (BYTE) s;
	m_font = CreateFontIndirect(&m_lf);

	return b;
}

void AutoFont::SetLogFont(const LOGFONT& logfont)
{
	m_lf=logfont;
	DeleteObject(m_font);
	m_font = CreateFontIndirect(&m_lf);
}

LONG AutoFont::GetHeight()
{
	return m_lf.lfHeight;
}

LONG AutoFont::GetWidth()
{
	return m_lf.lfWidth;
}

LONG AutoFont::GetEscapement()
{
	return m_lf.lfEscapement;
}

LONG AutoFont::GetOrientation()
{
	return m_lf.lfEscapement;
}

LONG AutoFont::GetWeight()
{
	return m_lf.lfWeight;
}

BYTE AutoFont::GetCharset()
{
	return m_lf.lfCharSet;
}

BYTE AutoFont::GetOutPrecision()
{
	return m_lf.lfOutPrecision;
}

BYTE AutoFont::GetClipPrecision()
{
	return m_lf.lfClipPrecision;
}

BYTE AutoFont::GetQuality()
{
	return m_lf.lfQuality;
}

BYTE AutoFont::GetPitchAndFamily()
{
	return m_lf.lfPitchAndFamily;
}

LPCTSTR AutoFont::GetFaceName()
{
	return m_lf.lfFaceName;
}

BOOL AutoFont::GetBold()
{
	return m_lf.lfWeight >= FW_MEDIUM ? TRUE : FALSE;
}

BOOL AutoFont::GetItalic()
{
	return (BOOL)m_lf.lfItalic;
}

BOOL AutoFont::GetUnderline()
{
	return (BOOL)m_lf.lfUnderline;
}

BOOL AutoFont::GetStrikeOut()
{
	return (BOOL)m_lf.lfStrikeOut;
}

XString AutoFont::ContractFont()
{
	XString str, color;

  str.Format("%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%s",
            m_lf.lfHeight,
            m_lf.lfWidth,
            m_lf.lfEscapement,
            m_lf.lfOrientation,
            m_lf.lfWeight,
            m_lf.lfItalic,
            m_lf.lfUnderline,
            m_lf.lfStrikeOut,
            m_lf.lfCharSet,
            m_lf.lfOutPrecision,
            m_lf.lfClipPrecision,
            m_lf.lfQuality,
            m_lf.lfPitchAndFamily,
            m_lf.lfFaceName);
  color.Format("%ul",m_fontColor);
  str += ",";
  str += color;

	return str;
}

void AutoFont::ExtractFont(XString& str)
{
	m_lf.lfHeight         =        atol(GetToken(str, ","));
	m_lf.lfWidth          =        atol(GetToken(str, ","));
	m_lf.lfEscapement     =        atol(GetToken(str, ","));
	m_lf.lfOrientation    =        atol(GetToken(str, ","));
	m_lf.lfWeight         =        atol(GetToken(str, ","));
	m_lf.lfItalic         = (BYTE) atoi(GetToken(str, ","));
	m_lf.lfUnderline      = (BYTE) atoi(GetToken(str, ","));
	m_lf.lfStrikeOut      = (BYTE) atoi(GetToken(str, ","));
	m_lf.lfCharSet        = (BYTE) atoi(GetToken(str, ","));
	m_lf.lfOutPrecision   = (BYTE) atoi(GetToken(str, ","));
	m_lf.lfClipPrecision  = (BYTE) atoi(GetToken(str, ","));
	m_lf.lfQuality        = (BYTE) atoi(GetToken(str, ","));
	m_lf.lfPitchAndFamily = (BYTE) atoi(GetToken(str, ","));
	strcpy_s(m_lf.lfFaceName,LF_FACESIZE,GetToken(str, ","));

	DeleteObject(m_font);
	m_font = CreateFontIndirect(&m_lf);

	m_fontColor = atol(str);
}

XString AutoFont::GetToken(XString& str, LPCTSTR c)
{
	int pos;
	XString token;

	pos=str.Find(c);
	token=str.Left(pos);
	str=str.Mid(pos+1);

	return token;
}

void AutoFont::GetFontFromDialog(HFONT* /*f*/,DWORD* /*color*/,HDC* /*pPrinterDC*/,HWND* /*pParentWnd*/)
{
// 	LOGFONT tlf;
//   if (f == NULL)
//   {
//     tlf = lf;
//   }
//   else
//   {
//     GetObject((HANDLE)f, sizeof(LOGFONT), &lf);
//   }
// 	CFontDialog dlg(&tlf, CF_EFFECTS | CF_SCREENFONTS,
// 		pPrinterDC, pParentWnd);
// 	dlg.m_cf.rgbColors=fontColor;
// 	
// 	if (dlg.DoModal()==IDOK)
// 	{
// 		dlg.GetCurrentFont(&lf);
// 		DeleteObject(m_font);
// 		m_font = CreateFontIndirect(&lf);
//     if (f)
//     {
//       *f = m_font;
//     }
// 		color = &dlg.m_cf.rgbColors;
// 		SetFontColor(dlg.m_cf.rgbColors);
// 	}
}

void AutoFont::SetFontColor(COLORREF color)
{
	m_fontColor=color;
  if(m_hDC != NULL)
  {
    ::SetTextColor(m_hDC,color);
  }
}

COLORREF AutoFont::GetFontColor()
{
	return m_fontColor;
}

void AutoFont::SetDC(HDC dc)
{
	m_hDC = dc;
}