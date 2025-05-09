/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: WideMessageBox.h
//
// Copyright (c) 2014-2025 ir. W.E. Huisman
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

// ===================== Task Dialog =========================
// Cannot include <commctrl.h> : Would not build on XP
// FOR USE IN VISUALSTUDIO 2010
// #ifndef __XTPTASKAPI_H__

#ifdef _WIN32
#include <pshpack1.h>
#include <commctrl.h>
#endif

typedef HRESULT (CALLBACK *PFTASKDIALOGCALLBACK)(__in HWND hwnd, __in UINT msg, __in WPARAM wParam, __in LPARAM lParam, __in LONG_PTR lpRefData);

#if (NTDDI_VERSION < NTDDI_VISTA)
enum _TASKDIALOG_FLAGS
{
  TDF_ENABLE_HYPERLINKS               = 0x0001,
  TDF_USE_HICON_MAIN                  = 0x0002,
  TDF_USE_HICON_FOOTER                = 0x0004,
  TDF_ALLOW_DIALOG_CANCELLATION       = 0x0008,
  TDF_USE_COMMAND_LINKS               = 0x0010,
  TDF_USE_COMMAND_LINKS_NO_ICON       = 0x0020,
  TDF_EXPAND_FOOTER_AREA              = 0x0040,
  TDF_EXPANDED_BY_DEFAULT             = 0x0080,
  TDF_VERIFICATION_FLAG_CHECKED       = 0x0100,
  TDF_SHOW_PROGRESS_BAR               = 0x0200,
  TDF_SHOW_MARQUEE_PROGRESS_BAR       = 0x0400,
  TDF_CALLBACK_TIMER                  = 0x0800,
  TDF_POSITION_RELATIVE_TO_WINDOW     = 0x1000,
  TDF_RTL_LAYOUT                      = 0x2000,
  TDF_NO_DEFAULT_RADIO_BUTTON         = 0x4000,
  TDF_CAN_BE_MINIMIZED                = 0x8000
};
typedef int TASKDIALOG_FLAGS;                         // Note: _TASKDIALOG_FLAGS is an int

typedef enum _TASKDIALOG_MESSAGES
{
  TDM_NAVIGATE_PAGE                   = WM_USER+101,
  TDM_CLICK_BUTTON                    = WM_USER+102, // wParam = Button ID
  TDM_SET_MARQUEE_PROGRESS_BAR        = WM_USER+103, // wParam = 0 (nonMarque) wParam != 0 (Marquee)
  TDM_SET_PROGRESS_BAR_STATE          = WM_USER+104, // wParam = new progress state
  TDM_SET_PROGRESS_BAR_RANGE          = WM_USER+105, // lParam = MAKELPARAM(nMinRange, nMaxRange)
  TDM_SET_PROGRESS_BAR_POS            = WM_USER+106, // wParam = new position
  TDM_SET_PROGRESS_BAR_MARQUEE        = WM_USER+107, // wParam = 0 (stop marquee), wParam != 0 (start marquee), lparam = speed (milliseconds between repaints)
  TDM_SET_ELEMENT_TEXT                = WM_USER+108, // wParam = element (TASKDIALOG_ELEMENTS), lParam = new element text (LPCWSTR)
  TDM_CLICK_RADIO_BUTTON              = WM_USER+110, // wParam = Radio Button ID
  TDM_ENABLE_BUTTON                   = WM_USER+111, // lParam = 0 (disable), lParam != 0 (enable), wParam = Button ID
  TDM_ENABLE_RADIO_BUTTON             = WM_USER+112, // lParam = 0 (disable), lParam != 0 (enable), wParam = Radio Button ID
  TDM_CLICK_VERIFICATION              = WM_USER+113, // wParam = 0 (unchecked), 1 (checked), lParam = 1 (set key focus)
  TDM_UPDATE_ELEMENT_TEXT             = WM_USER+114, // wParam = element (TASKDIALOG_ELEMENTS), lParam = new element text (LPCWSTR)
  TDM_SET_BUTTON_ELEVATION_REQUIRED_STATE = WM_USER+115, // wParam = Button ID, lParam = 0 (elevation not required), lParam != 0 (elevation required)
  TDM_UPDATE_ICON                     = WM_USER+116  // wParam = icon element (TASKDIALOG_ICON_ELEMENTS), lParam = new icon (hIcon if TDF_USE_HICON_* was set, PCWSTR otherwise)
}
TASKDIALOG_MESSAGES;

typedef enum _TASKDIALOG_NOTIFICATIONS
{
  TDN_CREATED                         = 0,
  TDN_NAVIGATED                       = 1,
  TDN_BUTTON_CLICKED                  = 2,            // wParam = Button ID
  TDN_HYPERLINK_CLICKED               = 3,            // lParam = (LPCWSTR)pszHREF
  TDN_TIMER                           = 4,            // wParam = Milliseconds since dialog created or timer reset
  TDN_DESTROYED                       = 5,
  TDN_RADIO_BUTTON_CLICKED            = 6,            // wParam = Radio Button ID
  TDN_DIALOG_CONSTRUCTED              = 7,
  TDN_VERIFICATION_CLICKED            = 8,             // wParam = 1 if checkbox checked, 0 if not, lParam is unused and always 0
  TDN_HELP                            = 9,
  TDN_EXPANDO_BUTTON_CLICKED          = 10            // wParam = 0 (dialog is now collapsed), wParam != 0 (dialog is now expanded)
}
TASKDIALOG_NOTIFICATIONS;

typedef struct _TASKDIALOG_BUTTON
{
  int     nButtonID;
  PCWSTR  pszButtonText;
}
TASKDIALOG_BUTTON;

typedef enum _TASKDIALOG_ELEMENTS
{
  TDE_CONTENT,
  TDE_EXPANDED_INFORMATION,
  TDE_FOOTER,
  TDE_MAIN_INSTRUCTION
}
TASKDIALOG_ELEMENTS;

typedef enum _TASKDIALOG_ICON_ELEMENTS
{
  TDIE_ICON_MAIN,
  TDIE_ICON_FOOTER
}
TASKDIALOG_ICON_ELEMENTS;

#define TD_WARNING_ICON         MAKEINTRESOURCEW(-1)
#define TD_ERROR_ICON           MAKEINTRESOURCEW(-2)
#define TD_INFORMATION_ICON     MAKEINTRESOURCEW(-3)
#define TD_SHIELD_ICON          MAKEINTRESOURCEW(-4)

enum _TASKDIALOG_COMMON_BUTTON_FLAGS
{
  TDCBF_OK_BUTTON            = 0x0001, // selected control return value IDOK
  TDCBF_YES_BUTTON           = 0x0002, // selected control return value IDYES
  TDCBF_NO_BUTTON            = 0x0004, // selected control return value IDNO
  TDCBF_CANCEL_BUTTON        = 0x0008, // selected control return value IDCANCEL
  TDCBF_RETRY_BUTTON         = 0x0010, // selected control return value IDRETRY
  TDCBF_CLOSE_BUTTON         = 0x0020  // selected control return value IDCLOSE
};
typedef int TASKDIALOG_COMMON_BUTTON_FLAGS;           // Note: _TASKDIALOG_COMMON_BUTTON_FLAGS is an int

typedef struct _TASKDIALOGCONFIG
{
  UINT        cbSize;
  HWND        hwndParent;
  HINSTANCE   hInstance;                              // used for MAKEINTRESOURCE() strings
  TASKDIALOG_FLAGS                dwFlags;            // TASKDIALOG_FLAGS (TDF_XXX) flags
  TASKDIALOG_COMMON_BUTTON_FLAGS  dwCommonButtons;    // TASKDIALOG_COMMON_BUTTON (TDCBF_XXX) flags
  PCWSTR      pszWindowTitle;                         // string or MAKEINTRESOURCE()
  union
  {
    HICON     hMainIcon;
    PCWSTR    pszMainIcon;
  } 
  DUMMYUNIONNAME;
  PCWSTR      pszMainInstruction;
  PCWSTR      pszContent;
  UINT        cButtons;
  const TASKDIALOG_BUTTON  *pButtons;
  int         nDefaultButton;
  UINT        cRadioButtons;
  const TASKDIALOG_BUTTON  *pRadioButtons;
  int         nDefaultRadioButton;
  PCWSTR      pszVerificationText;
  PCWSTR      pszExpandedInformation;
  PCWSTR      pszExpandedControlText;
  PCWSTR      pszCollapsedControlText;
  union
  {
    HICON     hFooterIcon;
    PCWSTR    pszFooterIcon;
  } 
  DUMMYUNIONNAME2;
  PCWSTR      pszFooter;
  PFTASKDIALOGCALLBACK pfCallback;
  LONG_PTR    lpCallbackData;
  UINT        cxWidth;                                // width of the Task Dialog's client area in DLU's. If 0, Task Dialog will calculate the ideal width.
} 
TASKDIALOGCONFIG;

WINCOMMCTRLAPI HRESULT WINAPI TaskDialogIndirect(__in const TASKDIALOGCONFIG *pTaskConfig, __out_opt int *pnButton, __out_opt int *pnRadioButton, __out_opt BOOL *pfVerificationFlagChecked);
WINCOMMCTRLAPI HRESULT WINAPI TaskDialog(__in_opt HWND hwndParent, __in_opt HINSTANCE hInstance, __in_opt PCWSTR pszWindowTitle, __in_opt PCWSTR pszMainInstruction, __in_opt PCWSTR pszContent, TASKDIALOG_COMMON_BUTTON_FLAGS dwCommonButtons, __in_opt PCWSTR pszIcon, __out_opt int *pnButton);
#endif

#ifdef _WIN32
#include <poppack.h>
#endif

// ==================== End TaskDialog =======================
// #endif

// MessageBox replacement for MS-Vista
// Supports width auto-sizing on base of the message
int 
WideMessageBox(HWND    p_hwnd
              ,LPCTSTR p_message
              ,LPCTSTR p_title
              ,int     p_buttons = MB_OK);

