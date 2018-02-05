/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: MapDialoog.cpp
//
// Marlin Component: Internet server/client
// 
// Copyright (c) 2015-2018 ir. W.E. Huisman
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
#include "StdAfx.h"
#include "MapDialoog.h"
#include <direct.h>
#include <SHLOBJ.H>

#ifndef BIF_NEWDIALOGSTYLE
#define BIF_NEWDIALOGSTYLE 0x0040
#endif


LPITEMIDLIST PathToPidl(CString const& path)
{
  LPITEMIDLIST  pidl = NULL;
  LPSHELLFOLDER pDesktopFolder;
  OLECHAR       olePath[MAX_PATH];
  ULONG         chEaten;
  ULONG         dwAttributes;
  HRESULT       hr;

  if (SUCCEEDED(SHGetDesktopFolder(&pDesktopFolder)))
  {
    MultiByteToWideChar(CP_ACP
                       ,MB_PRECOMPOSED
                       ,path
                       ,-1
                       ,olePath
                       ,MAX_PATH);

    hr = pDesktopFolder->ParseDisplayName(NULL
                                         ,NULL
                                         ,olePath
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

CString PidlToPath(LPITEMIDLIST pidl, bool deletePidl = false)
{
  //  Convert the pidl to a path
  char szPath[MAX_PATH] = "";
  BOOL cvtResult = SHGetPathFromIDList(pidl, szPath);
  if(!cvtResult) 
  {
    throw CString("Cannot translate a PIDL to a pathname");
  }
  //  Store in string
  CString result = szPath;

  //  Free the result pidl
  if(deletePidl)
  {
    CoTaskMemFree(pidl);
  }
  //  Done
  return result;
}

//=============================================================================

MapDialoog::MapDialoog()
{
  // Register original CWD (Current Working Directory)
  m_originalDir[0] = 0;
  _getcwd(m_originalDir,MAX_PATH);
}

//=============================================================================

MapDialoog::~MapDialoog()
{
  // Go back to the original directory
  _chdir((LPCSTR) m_originalDir);
}

//=============================================================================

bool MapDialoog::Browse(HWND            hwndParent, 
                        CString const&  title, 
                        CString const&  initdir, 
                        CString const&  rootdir, 
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

  //  We want to get callbacks
  bi.lpfn = CallbackProcS;
  bi.lParam = (LPARAM)this;

  //  Buffer in which the display name is returned
  char szDisplayName[MAX_PATH];
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
void MapDialoog::EnableOk(bool bEnable)
{
  //  Should only be called when called from within OnSelChange
  if(m_hwnd == 0 || IsWindow(m_hwnd) == false)
  {
    MessageBox(NULL,"Call from invalid context","Map dialoog",MB_OK|MB_ICONERROR);
  }
  //  Set the ok button state
  ::SendMessage(m_hwnd, BFFM_ENABLEOK, 0, bEnable);
}

//=============================================================================
void MapDialoog::SetSelection(CString const& path)
{
  //  Should only be called when called from within OnSelChange
  if(m_hwnd == 0 || IsWindow(m_hwnd) == false)
  {
    MessageBox(NULL,"Call from invalid context","Map dialoog",MB_OK|MB_ICONERROR);
  }
  //  Set the current path in the tree
  ::SendMessage(m_hwnd, BFFM_SETSELECTION, TRUE, (LPARAM)(char const*)path);  
}

//=============================================================================
void MapDialoog::SetStatusText(CString const& text)
{
  //  Should only be called when called from within OnSelChange
  if(m_hwnd == 0 || IsWindow(m_hwnd) == false)
  {
    MessageBox(NULL,"Call from invalid context","Map dialoog",MB_OK|MB_ICONERROR);
  }
  //  Set the current path in the tree
  ::SendMessage(m_hwnd, BFFM_SETSTATUSTEXT, 0, (LPARAM)(char const*)text);  
}

//=============================================================================

void MapDialoog::OnInitialized()
{
  //  Only meant for derived classes
}

//=============================================================================
void MapDialoog::OnSelChange(CString const&)
{
  //  Only meant for derived classes
}

//=============================================================================
int MapDialoog::CallbackProc(    
    HWND hwnd, 
    UINT uMsg, 
    LPARAM lParam)
{
  try
  {
    m_hwnd = hwnd;
    switch(uMsg)
    {
    case BFFM_INITIALIZED:
      if(m_init != "") SetSelection(m_init);
      OnInitialized();
      break;
    case BFFM_SELCHANGED:
      OnSelChange(PidlToPath((LPITEMIDLIST)lParam));
      break;
    case BFFM_VALIDATEFAILED:
      MessageBox(NULL,"The pathname entered is incorrect. Enter a correct pathname.","Map dialog",MB_OK|MB_ICONERROR);
      break;
    }
    m_hwnd = 0;
  }
  catch(...)
  {
    m_hwnd = 0;
  }
  return 0;
}


//=============================================================================

int CALLBACK MapDialoog::CallbackProcS(
    HWND hwnd, 
    UINT uMsg, 
    LPARAM lParam, 
    LPARAM lpData
    )
{
  return ((MapDialoog*)lpData)->CallbackProc(hwnd, uMsg, lParam);
}


//=============================================================================
