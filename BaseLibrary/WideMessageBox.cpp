/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: WideMessageBox.cpp
//
// Copyright (c) 2006-2017 ir. W.E. Huisman
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
#include "WideMessageBox.h"
#include "ConvertWideString.h"
#include <VersionHelpers.h>

#ifndef MB_CANCELTRYCONTINUE
#define MB_CANCELTRYCONTINUE 0x006L
#endif

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// Measure the extents of a string of text in a specified font
// by drawing it on an off-screen DC and by doing so,
// getting the outer measures in a CRect.
static BOOL 
_WMB_CalcTextSize(const XString& strText, CSize& sizeText,HFONT font)
{
  BOOL result = FALSE;

  if(strText.IsEmpty() || font == NULL)
  {
    sizeText = CSize(0,0);
    return FALSE;
  }
  HDC dc = ::GetDC(NULL);
  HGDIOBJ pFontPrev = ::SelectObject(dc,font);

  CRect rectText = CRect(CPoint(0,0),sizeText);
  if(::DrawText(dc,strText,strText.GetLength(),&rectText,DT_CALCRECT|DT_LEFT|DT_NOPREFIX|DT_TOP|DT_WORDBREAK) != 0)
  {
    sizeText.cx = (int)(rectText.right  * 1.04);
    sizeText.cy = (int)(rectText.bottom * 1.04);
    ::SelectObject(dc,pFontPrev);
    result = TRUE;
  }
  else
  {
    sizeText = CSize(0,0);
  }
  // Reset the font
  ::SelectObject(dc,pFontPrev);
  return result;
}

// Convert pixels to DLU's
// See the MSDN documentation at "GetDialogBaseUnits"
static CPoint 
_WMB_Pix2Dlu(int pixX, int pixY)
{
  CPoint baseXY(::GetDialogBaseUnits());
  CPoint dluXY;
  dluXY.x = ::MulDiv(pixX, 4, baseXY.x);
  dluXY.y = ::MulDiv(pixY, 8, baseXY.y);
  return dluXY;
}

typedef HRESULT (WINAPI* TDIF)(const TASKDIALOGCONFIG*,int*,int*,BOOL*);
static TDIF TaskDialogIndirectFunc = NULL;
static HMODULE common = NULL;

// Loaded status van TaskDialog
// 0 -> Nog niet getest
// 1 -> Getest, niet aanwezig
// 2 -> Getest, geladen
static int loaded = 0;

// Decrement the counter of the DLL, so it can get unloaded
static void
UnloadCommonControls()
{
  if(common)
  {
    ::FreeLibrary(common);
  }
};

// Find out if we have a common-controls with a Vista compatible "TaskDialog"
// Reasons to do it in this indirect way:
// 1: Using TaskDialog/TaskDialogIndirect directly would break the link-fase under XP
// 2: Which common-controls gets loaded depends on the current manifest file
// So simply asking for the version of the Windows-OS will **NOT** work here
static bool
LoadCommonControls()
{
  if(loaded == 2)
  {
    return true;
  }
  if(loaded == 1)
  {
    return false;
  }
  // Check version of your operating system
  if(::IsWindowsVistaOrGreater())
  {
    HMODULE mod = ::LoadLibrary("comctl32.dll");
    if(mod)
    {
      atexit(UnloadCommonControls);
      TaskDialogIndirectFunc = (TDIF)GetProcAddress(mod,"TaskDialogIndirect");
    }
    if(TaskDialogIndirectFunc)
    {
      loaded = 2;
      return true;
    }
  }
  loaded = 1;
  return false;
}

HRESULT __stdcall
WideMessageBoxCallback(HWND hwnd, UINT msg, WPARAM /*wParam*/, LPARAM /*lParam*/, LONG_PTR /*lpRefData*/)
{
  if(msg == TDN_CREATED)
  {
    ::SetForegroundWindow(hwnd);
  }
  if(msg == TDN_BUTTON_CLICKED)
  {
    // Button clicked, so close
    return S_OK;
  }
  // Return S_FALSE, or the TaksDialog will close!
  return S_FALSE;
}

// WideMessageBox: The replacement of ::MessageBox on MS-Vista
// Does auto-sizing in width on basis of the p_message text
int 
WideMessageBox(HWND        p_hwnd
              ,const char* p_message
              ,const char* p_title
              ,int         p_buttons /* = MB_OK */)
{
  if(LoadCommonControls() == false)
  {
    // Still no TaskDialog function available
    // or foreground function not provided in TaskDialog
    return ::MessageBox(p_hwnd,p_message,p_title,p_buttons);
  }
  TASKDIALOGCONFIG config;
  int pressed = 0;

  // Prepare taskdialog config structure
  memset(&config,0,sizeof(TASKDIALOGCONFIG));
  config.cbSize  = sizeof(TASKDIALOGCONFIG);

  // Parent message box to parent window
  config.hwndParent = p_hwnd;

  // Title of the box
  CComBSTR title = CT2CW(p_title);
  config.pszWindowTitle = title;

  // Determine which buttons to use
  switch(p_buttons & 0x0f)
  {
    case MB_OK:                 config.dwCommonButtons = TDCBF_OK_BUTTON;
                                break;
    case MB_YESNO:              config.dwCommonButtons = TDCBF_YES_BUTTON | TDCBF_NO_BUTTON;
                                break;
    case MB_YESNOCANCEL:        config.dwCommonButtons = TDCBF_YES_BUTTON | TDCBF_NO_BUTTON | TDCBF_CANCEL_BUTTON;
                                break;
    case MB_RETRYCANCEL:        config.dwCommonButtons = TDCBF_RETRY_BUTTON | TDCBF_CANCEL_BUTTON;
                                break;
    case MB_ABORTRETRYIGNORE:   // Fall through
    case MB_CANCELTRYCONTINUE:  // New Microsoft policy: Not a good user dialog exchange
                                // Don't do this. We don't support it here.
                                return ::MessageBox(p_hwnd,p_message,p_title,p_buttons);
  }
  // Determine which main icon to use
  switch(p_buttons & 0x0f0)
  {
    //   MB_ICONHAND:
    case MB_ICONERROR:        config.pszMainIcon = TD_ERROR_ICON;
                              break;
    //   MB_ICONEXCLAMATION
    case MB_ICONWARNING:      config.pszMainIcon = TD_WARNING_ICON;
                              break;
    //   MB_ICONASTERISK:
    case MB_ICONINFORMATION:  // Fall through
    case MB_ICONQUESTION:     // New Microsoft policy. Question icons are for help
                              config.pszMainIcon = TD_INFORMATION_ICON;
                              break;
    // MAKEINTRESOURCEW(value) are listed here
    // -4 TD_SHIELD_ICON    
    // -5 Shield icon       + blue band
    // -6 Update icon       + yellow band  
    // -7 Protection shield + red band
    // -8 Green shield      + green band
    // -9 Admin shield      + brown band
  }
  // Getting a standard Vista message font
  NONCLIENTMETRICS ncm;
  ncm.cbSize = sizeof(NONCLIENTMETRICS);
  ::SystemParametersInfo(SPI_GETNONCLIENTMETRICS, 0, &ncm, 0);

  if(_tcscmp(ncm.lfMenuFont.lfFaceName,"Segoe UI") == 0)
  {
    ncm.lfMenuFont.lfQuality = 5;
  }
  ncm.lfMenuFont.lfWeight = FW_NORMAL;
  ncm.lfMenuFont.lfItalic = 0;
  HFONT font = ::CreateFontIndirect(&ncm.lfMenuFont);

  // Size of the text to be displayed
  XString text(p_message);

  // Support and correct all kind of mistakes from programmers
  text.Replace("\n\r","\n");
  text.Replace("\r\n","\n");
  text.Replace("\n","\r\n");

  // Calculate size of the text
  CSize sizeText(4000,2000);
  if(_WMB_CalcTextSize(text,sizeText,font))
  {
    // Left / Right margin in the dialog
    sizeText.cx += 20; 
    if(config.pszMainIcon)
    {
      // Main icon takes this amount of space
      sizeText.cx += 48;
    }
    // POLICY: WideMessageBox is at maximum 90% of the screen's width
    int maxWidth = GetSystemMetrics(SM_CXSCREEN) * 90 / 100;
    if(sizeText.cx > maxWidth)
    {
      sizeText.cx = maxWidth;
    }
    // Convert pixels to dialog-units (DLU's)
    CPoint sizeDLU = _WMB_Pix2Dlu(sizeText.cx,sizeText.cy);
    config.cxWidth = sizeDLU.x;
  }
  else
  {
    TRACE("WideMessageBox: Cannot get an off-screen measure of the text: %s",text.GetString());
    return ::MessageBox(p_hwnd,p_message,p_title,p_buttons);
  }
  std::wstring mess = StringToWString(text);
  config.pszContent = mess.c_str();

  // Set callback for server programs
  if(p_buttons & MB_SETFOREGROUND)
  {
    config.pfCallback     = (PFTASKDIALOGCALLBACK) WideMessageBoxCallback;
    config.lpCallbackData = NULL;
  }

  // Do the task dialog box as a replacement of MessageBox
  HRESULT result = (*TaskDialogIndirectFunc)(&config    // How to config it
                                            ,&pressed   // Result of the box
                                            ,NULL       // No radio buttons used
                                            ,NULL);     // No check box result
  switch(result)
  {
    case S_OK:           break;
    case E_OUTOFMEMORY:  ::MessageBox(p_hwnd,"Out of memory","Error",MB_OK|MB_ICONERROR);
                         pressed = 0;
                         break;
    case E_INVALIDARG:   ::MessageBox(p_hwnd,"Wrong argument to TaskDialog","Error",MB_OK|MB_ICONERROR);
                         pressed = 0;
                         break;
    case E_FAIL:         ::MessageBox(p_hwnd,"Failure in TaskDialog","Error",MB_OK|MB_ICONERROR);
                         pressed = 0;
                         break;
    default:             ::MessageBox(p_hwnd,"Unknown error in TaskDialog","Error",MB_OK|MB_ICONERROR);
                         pressed = 0;
                         break;
  }
  return pressed;
}
