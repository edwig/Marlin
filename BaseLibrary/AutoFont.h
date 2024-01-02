/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: AutoFont.h
//
// BaseLibrary: Indispensable general objects and functions
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
#include <wingdi.h>

class AutoFont 
{
public:
	AutoFont();                             // Default Constructor
	explicit AutoFont(XString facename);    // Font name constructor
	explicit AutoFont(LOGFONT& logfont);    // LogFont constructor
	explicit AutoFont(HFONT font);          // Constructs font based on existing font
	~AutoFont();                            // Destructor

	LONG      SetHeight(LONG height);
	LONG      SetHeightA(LONG height);
	LONG      SetWidth(LONG width);
	LONG      SetEscapement(LONG esc);
	LONG      SetOrientation(LONG orientation);
	LONG      SetWeight(LONG weight);
	BYTE      SetCharset(BYTE charset);
	BYTE      SetOutPrecision(BYTE op);
	BYTE      SetClipPrecision(BYTE cp);
	BYTE      SetQuality(BYTE qual);
	BYTE      SetPitchAndFamily(BYTE paf);
	XString   SetFaceName(XString facename);
	LPCTSTR   SetFaceName(const TCHAR* facename);
	BOOL      SetBold(BOOL B);
	BOOL      SetItalic(BOOL i);
	BOOL      SetUnderline(BOOL u);
	BOOL      SetStrikeOut(BOOL s);
	void      SetLogFont(const LOGFONT& logfont);
	void      SetFontColor(COLORREF color);
	void      SetDC(HDC dc);

	LONG      GetHeight();
	LONG      GetWidth();
	LONG      GetEscapement();
	LONG      GetOrientation();
	LONG      GetWeight();
	BYTE      GetCharset();
	BYTE      GetOutPrecision();
	BYTE      GetClipPrecision();
	BYTE      GetQuality();
	BYTE      GetPitchAndFamily();
	LPCTSTR   GetFaceName();
	BOOL      GetBold();
	BOOL      GetItalic();
	BOOL      GetUnderline();
	BOOL      GetStrikeOut();
	COLORREF  GetFontColor();
	// GetLogFont is a member of CFont, and is used as normal.

	// These two functions are good for registry...
	XString ContractFont();             //Places font info into single string
	void    ExtractFont(XString& str);  //Parses single string font info.

	// This function allows seamless use of the CFontDialog object
	// f: CFont or CAutoFont object to initialize the dlg with.
	// color: initial color shown in font color dialog
	// pPrinterDC: points to a printer DC if needed
	// pParentWnd: parent window of dialog, if needed
	//
	// The new font is returned through f, the new color through color
	void GetFontFromDialog(HFONT* f=NULL,DWORD *color=0,HDC* pPrinterDC=NULL,HWND *pParentWnd=NULL);

private:
  // Used in Expand function
  XString   GetToken(XString& str,const TCHAR* c);

  HFONT     m_font { NULL };
#ifdef UNICODE
  LOGFONTW  m_lf;
#else
	LOGFONTA  m_lf;                   // Stores this fonts LogFont for quick retrieval
#endif
	COLORREF  m_fontColor;
	HDC       m_hDC;
};
