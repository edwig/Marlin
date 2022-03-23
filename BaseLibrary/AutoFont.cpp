//CAutoFont class implementation
#include "pch.h"
#include "BaseLibrary.h"
#include "AutoFont.h"

AutoFont::AutoFont()
{
	lf.lfHeight=-12;
	lf.lfWidth=0;
	lf.lfEscapement=0;
	lf.lfOrientation=0;
	lf.lfWeight=FW_NORMAL;
	lf.lfItalic=0;
	lf.lfUnderline=0;
	lf.lfStrikeOut=0;
	lf.lfCharSet=ANSI_CHARSET;
	lf.lfOutPrecision=OUT_DEFAULT_PRECIS;
	lf.lfClipPrecision=CLIP_DEFAULT_PRECIS;
	lf.lfQuality=PROOF_QUALITY;
	lf.lfPitchAndFamily=VARIABLE_PITCH | FF_ROMAN;
	strcpy_s(lf.lfFaceName,LF_FACESIZE,"Times New Roman");

	CreateFontIndirect(&lf);

	fontColor=0;
	hDC=NULL;
}

AutoFont::AutoFont(XString facename)
{
	lf.lfHeight=-12;
	lf.lfWidth=0;
	lf.lfEscapement=0;
	lf.lfOrientation=0;
	lf.lfWeight=FW_NORMAL;
	lf.lfItalic=0;
	lf.lfUnderline=0;
	lf.lfStrikeOut=0;
	lf.lfCharSet=ANSI_CHARSET;
	lf.lfOutPrecision=OUT_DEFAULT_PRECIS;
	lf.lfClipPrecision=CLIP_DEFAULT_PRECIS;
	lf.lfQuality=PROOF_QUALITY;
	lf.lfPitchAndFamily=VARIABLE_PITCH | FF_ROMAN;
	strcpy_s(lf.lfFaceName,LF_FACESIZE,(LPCTSTR)facename);

	CreateFontIndirect(&lf);

	fontColor=0;
	hDC=NULL;
}

AutoFont::AutoFont(LOGFONT& logfont)
{
	lf=logfont;
	CreateFontIndirect(&lf);

	fontColor=0;
	hDC=NULL;
}

AutoFont::AutoFont(HFONT font)
{
	HFONT hFont=(HFONT)font;

  GetObject((HANDLE)hFont, sizeof(LOGFONT),&lf);

	fontColor=0;
	hDC=NULL;
}

AutoFont::~AutoFont()
{
}

LONG AutoFont::SetHeight(LONG height)
{
	LONG l=lf.lfHeight;

	DeleteObject(m_font);
	lf.lfHeight=height;
	m_font = CreateFontIndirect(&lf);

	return l;
}

LONG AutoFont::SetHeightA(LONG height)
{
	LONG l=lf.lfHeight;

	DeleteObject(m_font);
  if(height > 0)
  {
    height = 0 - height;
  }
	lf.lfHeight = height;
	m_font = CreateFontIndirect(&lf);

	return l;
}

LONG AutoFont::SetWidth(LONG width)
{
	LONG l=lf.lfWidth;

	DeleteObject(m_font);
	lf.lfWidth=width;
	m_font = CreateFontIndirect(&lf);

	return l;
}

LONG AutoFont::SetEscapement(LONG esc)
{
	LONG l=lf.lfEscapement;

	DeleteObject(m_font);
	lf.lfEscapement=esc;
	m_font = CreateFontIndirect(&lf);

	return l;
}

LONG AutoFont::SetOrientation(LONG orientation)
{
	LONG l=lf.lfOrientation;

	DeleteObject(m_font);
	lf.lfOrientation=orientation;
	m_font = CreateFontIndirect(&lf);

	return l;
}

LONG AutoFont::SetWeight(LONG weight)
{
	LONG l=lf.lfWeight;

	DeleteObject(m_font);
	lf.lfWeight=weight;
	m_font = CreateFontIndirect(&lf);

	return l;
}

BYTE AutoFont::SetCharset(BYTE charset)
{
	BYTE b=lf.lfCharSet;

	DeleteObject(m_font);
	lf.lfCharSet=charset;
	m_font = CreateFontIndirect(&lf);

	return b;
}

BYTE AutoFont::SetOutPrecision(BYTE op)
{
	BYTE b=lf.lfOutPrecision;

	DeleteObject(m_font);
	lf.lfOutPrecision=op;
	m_font = CreateFontIndirect(&lf);

	return b;
}

BYTE AutoFont::SetClipPrecision(BYTE cp)
{
	BYTE b=lf.lfClipPrecision;

	DeleteObject(m_font);
	lf.lfClipPrecision=cp;
	m_font = CreateFontIndirect(&lf);

	return b;
}

BYTE AutoFont::SetQuality(BYTE qual)
{
	BYTE b=lf.lfQuality;

	DeleteObject(m_font);
	lf.lfQuality=qual;
	m_font = CreateFontIndirect(&lf);

	return b;
}

BYTE AutoFont::SetPitchAndFamily(BYTE paf)
{
	BYTE b=lf.lfPitchAndFamily;

	DeleteObject(m_font);
	lf.lfPitchAndFamily=paf;
	m_font = CreateFontIndirect(&lf);

	return b;
}

XString AutoFont::SetFaceName(XString facename)
{
	XString str=lf.lfFaceName;

	DeleteObject(m_font);
	strcpy_s(lf.lfFaceName,LF_FACESIZE, facename.GetString());
	m_font = CreateFontIndirect(&lf);

	return str;
}

// BEWARE: Not thread safe!
LPCTSTR 
AutoFont::SetFaceName(LPCTSTR facename)
{
  static char buffer[LF_FACESIZE + 1];
  strcpy_s(buffer,LF_FACESIZE,lf.lfFaceName);

	DeleteObject(m_font);
	strcpy_s(lf.lfFaceName,LF_FACESIZE,facename);
	m_font = CreateFontIndirect(&lf);

  return (LPCTSTR) &buffer;
}

BOOL AutoFont::SetBold(BOOL B)
{
	BOOL b;

	if (B)
		b=SetWeight(FW_BOLD);
	else
		b=SetWeight(FW_NORMAL);

	if (b >= FW_MEDIUM)
		return TRUE;
	else
		return FALSE;
}

BOOL AutoFont::SetItalic(BOOL i)
{
	BOOL b=(BOOL)lf.lfItalic;

	DeleteObject(m_font);
	lf.lfItalic=i;
	m_font = CreateFontIndirect(&lf);

	return b;
}

BOOL AutoFont::SetUnderline(BOOL u)
{
	BOOL b=(BOOL)lf.lfUnderline;

	DeleteObject(m_font);
	lf.lfUnderline=u;
	m_font = CreateFontIndirect(&lf);

	return b;
}

BOOL AutoFont::SetStrikeOut(BOOL s)
{
	BOOL b=(BOOL)lf.lfStrikeOut;

	DeleteObject(m_font);
	lf.lfStrikeOut=s;
	m_font = CreateFontIndirect(&lf);

	return b;
}

void AutoFont::SetLogFont(LOGFONT& logfont)
{
	lf=logfont;
	DeleteObject(m_font);
	m_font = CreateFontIndirect(&lf);
}

LONG AutoFont::GetHeight()
{
	return lf.lfHeight;
}

LONG AutoFont::GetWidth()
{
	return lf.lfWidth;
}

LONG AutoFont::GetEscapement()
{
	return lf.lfEscapement;
}

LONG AutoFont::GetOrientation()
{
	return lf.lfEscapement;
}

LONG AutoFont::GetWeight()
{
	return lf.lfWeight;
}

BYTE AutoFont::GetCharset()
{
	return lf.lfCharSet;
}

BYTE AutoFont::GetOutPrecision()
{
	return lf.lfOutPrecision;
}

BYTE AutoFont::GetClipPrecision()
{
	return lf.lfClipPrecision;
}

BYTE AutoFont::GetQuality()
{
	return lf.lfQuality;
}

BYTE AutoFont::GetPitchAndFamily()
{
	return lf.lfPitchAndFamily;
}

LPCTSTR AutoFont::GetFaceName()
{
	return lf.lfFaceName;
}

BOOL AutoFont::GetBold()
{
	return lf.lfWeight >= FW_MEDIUM ? TRUE : FALSE;
}

BOOL AutoFont::GetItalic()
{
	return (BOOL)lf.lfItalic;
}

BOOL AutoFont::GetUnderline()
{
	return (BOOL)lf.lfUnderline;
}

BOOL AutoFont::GetStrikeOut()
{
	return (BOOL)lf.lfStrikeOut;
}

XString AutoFont::ContractFont()
{
	XString str, color;

	str.Format("%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%s",
		lf.lfHeight,
		lf.lfWidth,
		lf.lfEscapement,
		lf.lfOrientation,
		lf.lfWeight,
		lf.lfItalic,
		lf.lfUnderline,
		lf.lfStrikeOut,
		lf.lfCharSet,
		lf.lfOutPrecision,
		lf.lfClipPrecision,
		lf.lfQuality,
		lf.lfPitchAndFamily,
		lf.lfFaceName);
	color.Format("%u", fontColor);
	str+=",";
	str+=color;

	return str;
}

void AutoFont::ExtractFont(XString& str)
{
	lf.lfHeight         = atol((LPCTSTR)GetToken(str, ","));
	lf.lfWidth          = atol((LPCTSTR)GetToken(str, ","));
	lf.lfEscapement     = atol((LPCTSTR)GetToken(str, ","));
	lf.lfOrientation    = atol((LPCTSTR)GetToken(str, ","));
	lf.lfWeight         = atol((LPCTSTR)GetToken(str, ","));
	lf.lfItalic         = atoi((LPCTSTR)GetToken(str, ","));
	lf.lfUnderline      = atoi((LPCTSTR)GetToken(str, ","));
	lf.lfStrikeOut      = atoi((LPCTSTR)GetToken(str, ","));
	lf.lfCharSet        = atoi((LPCTSTR)GetToken(str, ","));
	lf.lfOutPrecision   = atoi((LPCTSTR)GetToken(str, ","));
	lf.lfClipPrecision  = atoi((LPCTSTR)GetToken(str, ","));
	lf.lfQuality        = atoi((LPCTSTR)GetToken(str, ","));
	lf.lfPitchAndFamily = atoi((LPCTSTR)GetToken(str, ","));
	strcpy_s(lf.lfFaceName,LF_FACESIZE,(LPCTSTR)GetToken(str, ","));

	DeleteObject(m_font);
	m_font = CreateFontIndirect(&lf);

	fontColor=atol((LPCTSTR)str);
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

void AutoFont::GetFontFromDialog(HFONT* f,DWORD* color,HDC* pPrinterDC,HWND* pParentWnd)
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
	fontColor=color;
	if (hDC!=NULL)
		::SetTextColor(hDC, color);
}

COLORREF AutoFont::GetFontColor()
{
	return fontColor;
}

void AutoFont::SetDC(HDC dc)
{
	hDC=dc;
}