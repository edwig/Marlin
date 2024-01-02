/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: BrowseForDirectory.cpp
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
#include "stdafx.h"
#include "BrowseForDirectory.h"
#include <direct.h>
#include <SHLOBJ.H>

#ifndef BIF_NEWDIALOGSTYLE
#define BIF_NEWDIALOGSTYLE 0x0040
#endif

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#pragma warning (disable: 4302)
#pragma warning (disable: 4311)

LPITEMIDLIST PathToPidl(XString const& path)
{
  LPITEMIDLIST  pidl = NULL;
  LPSHELLFOLDER pDesktopFolder;
  wchar_t       olePath[MAX_PATH + 1];
  ULONG         chEaten      = 0;
  ULONG         dwAttributes = 0;

  if (SUCCEEDED(SHGetDesktopFolder(&pDesktopFolder)))
  {
#ifdef UNICODE
    wcsncpy_s(olePath,path.GetString(),MAX_PATH);
#else
    MultiByteToWideChar(CP_ACP
                       ,MB_PRECOMPOSED
                       ,path
                       ,-1
                       ,(LPWSTR)olePath
                       ,MAX_PATH);
#endif
    HRESULT hr;
    hr = pDesktopFolder->ParseDisplayName(NULL
                                         ,NULL
                                         ,(LPWSTR)olePath
                                         ,&chEaten
                                         ,&pidl
                                         ,&dwAttributes);
    if (FAILED(hr))
    {
      return 0;
    }
    pDesktopFolder->Release();
  }
  return pidl;
}

//=============================================================================

XString PidlToPath(LPITEMIDLIST pidl, bool deletePidl = false)
{
  //  Convert the pidl to a path
 TCHAR szPath[MAX_PATH] = _T("");
  BOOL cvtResult = SHGetPathFromIDList(pidl,szPath);
  if(!cvtResult) 
  {
    //throw some error?
  }
  //  Store in string
  XString result = szPath;

  //  Free the result pidl
  if(deletePidl)
  {
    CoTaskMemFree(pidl);
  }
  //  Done
  return result;
}

//=============================================================================

BrowseForDirectory::BrowseForDirectory()
{
  // Getting the original directory CWD (Current Working Directory)
  m_hwnd = NULL;
  m_originalDir[0] = 0;
  _getcwd(m_originalDir,MAX_PATH);
}

//=============================================================================

BrowseForDirectory::~BrowseForDirectory()
{
  // Go back to the original directory
  _chdir((LPCSTR) m_originalDir);
}

//=============================================================================

bool BrowseForDirectory::Browse(HWND            hwndParent, 
                                XString const&  title, 
                                XString const&  initdir, 
                                XString const&  rootdir, 
                                bool            showFiles,
                                bool            showStatus)
{
  hwndParent = AfxGetApp()->GetMainWnd()->GetSafeHwnd();
  //  Store initial settings
  m_root = rootdir;
  m_init = initdir;

  //  Init the struct
  BROWSEINFO bi;
  memset(&bi, 0, sizeof(bi));

  //  Dialog parent
  bi.hwndOwner = hwndParent;

  //  Allow root to be set
  if(rootdir != "") 
  {
    bi.pidlRoot = PathToPidl(m_root);
  }

  //  Crashed in some libraries. So disabled. Did not use it
  //   bi.lpfn = CallbackProcS;
  //   bi.lParam = (LONG)this;

  //  Buffer in which the display name is returned
  TCHAR szDisplayName[MAX_PATH];
  bi.pszDisplayName = szDisplayName;

  //  Title
  bi.lpszTitle = title;

  //  Misc options
  if(showFiles)   bi.ulFlags |= BIF_BROWSEINCLUDEFILES;
  if(showStatus)  bi.ulFlags |= BIF_STATUSTEXT;

  bi.ulFlags |= BIF_EDITBOX | BIF_VALIDATE | BIF_NEWDIALOGSTYLE;

  //  Show the dialog
  LPITEMIDLIST pidlResult = SHBrowseForFolder(&bi);

  //  Free the root path pidl
  if(bi.pidlRoot) CoTaskMemFree((LPITEMIDLIST)bi.pidlRoot);
   
  //  Check result
  if(pidlResult == 0) return false;

  //  Store the resulting info
  m_disp = szDisplayName;
  m_path = PidlToPath(pidlResult, true);

  //  Done
  return true;
}

//=============================================================================
void BrowseForDirectory::EnableOk(bool bEnable)
{
  //  Should only be called when called from within OnSelChange
  if(m_hwnd == NULL || IsWindow(m_hwnd) == false)
  {
    MessageBox(NULL,_T("Call from invalid context"),_T("Map dialoog"),MB_OK|MB_ICONERROR);
    return;
  }
  //  Set the ok button state
  ::SendMessage(m_hwnd, BFFM_ENABLEOK, 0, bEnable);
}

//=============================================================================
void BrowseForDirectory::SetSelection(XString const& path)
{
  //  Should only be called when called from within OnSelChange
  if(m_hwnd == NULL || IsWindow(m_hwnd) == false)
  {
    MessageBox(NULL,_T("Call from invalid context"),_T("Directory browse dialog"),MB_OK|MB_ICONERROR);
    return;
  }
  //  Set the current path in the tree
  ::SendMessage(m_hwnd, BFFM_SETSELECTION, TRUE, (long)(LPCTSTR)path );  
}

//=============================================================================
void 
BrowseForDirectory::SetStatusText(XString const& text)
{
  //  Should only be called when called from within OnSelChange
  if(m_hwnd == 0 || IsWindow(m_hwnd) == false)
  {
    MessageBox(NULL,_T("Call from invalid context"),_T("Directory browse dialog"),MB_OK|MB_ICONERROR);
    return;
  }
  //  Set the current path in the tree
  ::SendMessage(m_hwnd,BFFM_SETSTATUSTEXT,0,(long)(LPCTSTR)text);  
}

//=============================================================================

void BrowseForDirectory::OnInitialized()
{
  //  Only meant for derived classes
}

//=============================================================================
void BrowseForDirectory::OnSelChange(XString const&)
{
  //  Only meant for derived classes
}

//=============================================================================
int BrowseForDirectory::CallbackProc(HWND hwnd,UINT uMsg,LPARAM lParam)
{
  try
  {
    m_hwnd = hwnd;
    switch(uMsg)
    {
      case BFFM_INITIALIZED:    if(m_init != "") SetSelection(m_init);
                                OnInitialized();
                                break;
      case BFFM_SELCHANGED:     OnSelChange(PidlToPath((LPITEMIDLIST)lParam));
                                break;
      case BFFM_VALIDATEFAILED: MessageBox(NULL,_T("Directory path name is invalid. Correct to an existing directory."),_T("Directory browse dialog"),MB_OK|MB_ICONERROR);
                                break;
    }
    m_hwnd = 0;
  }
  catch(CException& er)
  {
    m_hwnd = 0;
    UNREFERENCED_PARAMETER(er);
  }
  return 0;
}


//=============================================================================

int CALLBACK BrowseForDirectory::CallbackProcS(HWND hwnd,UINT uMsg,LPARAM lParam,LPARAM lpData)
{
  return ((BrowseForDirectory*)lpData)->CallbackProc(hwnd, uMsg, lParam);
}
