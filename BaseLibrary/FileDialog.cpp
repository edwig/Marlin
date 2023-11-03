////////////////////////////////////////////////////////////////////////
//
// File: FileDialog.cpp
//
// Copyright (c) 1998-2022 ir. W.E. Huisman
// All rights reserved
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of 
// this software and associated documentation files (the "Software"), 
// to deal in the Software without restriction, including without limitation the rights 
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, 
// and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all copies 
// or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, 
// INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
// WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION 
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
// Version number: See SQLComponents.h
//
#include "pch.h"
#include "FileDialog.h"
#include <dlgs.h>

#pragma warning (disable:4312)

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

DocFileDialog::DocFileDialog(HWND    p_owner
                            ,bool    p_open        // true = open, false = SaveAs
                            ,XString p_title       // Title of the dialog
                            ,XString p_defext      // Default extension
                            ,XString p_filename    // Default first file
                            ,int     p_flags       // Default flags
                            ,XString p_filter      // Filter for extensions
                            ,XString p_direct)     
              :m_open(p_open)
{
  if(p_filter.IsEmpty())
  {
    p_filter = _T("Text files (*.txt)|*.txt|");
  }
  // Register original CWD (Current Working Directory)
  GetCurrentDirectory(MAX_PATH, m_original);
  if(!p_direct.IsEmpty())
  {
    // Change to starting directory
    SetCurrentDirectory(p_direct.GetString());
  }
  _tcsncpy_s(m_filter,  1024,   p_filter,  1024);
  _tcsncpy_s(m_filename,MAX_PATH,p_filename,MAX_PATH);
  _tcsncpy_s(m_defext,  100,    p_defext,  100);
  _tcsncpy_s(m_title,   100,    p_title,   100);
  FilterString(m_filter);

  // Fill in the filename structure
  p_flags |= OFN_ENABLESIZING | OFN_LONGNAMES | OFN_NOCHANGEDIR |  OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;
  p_flags &= ~(OFN_NODEREFERENCELINKS | OFN_NOLONGNAMES | OFN_NOTESTFILECREATE);

  m_ofn.lStructSize       = sizeof(OPENFILENAME);
  m_ofn.hwndOwner         = p_owner;
  m_ofn.hInstance         = (HINSTANCE) GetWindowLong(m_ofn.hwndOwner,GWLP_HINSTANCE);
  m_ofn.lpstrFile         = (LPTSTR) m_filename;
  m_ofn.lpstrDefExt       = (LPTSTR) m_defext;
  m_ofn.lpstrTitle        = (LPTSTR) m_title;
  m_ofn.lpstrFilter       = (LPTSTR) m_filter;
  m_ofn.Flags             = p_flags;
  m_ofn.nFilterIndex      = 1;    // Use lpstrFilter
  m_ofn.nMaxFile          = MAX_PATH;
  m_ofn.lpstrCustomFilter = NULL; //(LPSTR) buf_filter;
  m_ofn.nMaxCustFilter    = 0;
  m_ofn.lpstrFileTitle    = NULL;
  m_ofn.nMaxFileTitle     = 0;
  m_ofn.lpstrInitialDir   = NULL;
  m_ofn.nFileOffset       = 0;
  m_ofn.lCustData         = NULL;
  m_ofn.lpfnHook          = NULL;
  m_ofn.lpTemplateName    = NULL;
}

DocFileDialog::~DocFileDialog()
{
  // Go back to the original directory
  SetCurrentDirectory(m_original);
}

#pragma warning(disable:4702)
int 
DocFileDialog::DoModal()
{
  int res = IDCANCEL;
  try
  {
    if(m_open)
    {
      res = GetOpenFileName(&m_ofn);
    }
    else
    {
      res = GetSaveFileName(&m_ofn);
    }
  }
  catch(...)
  {
    ::MessageBox(NULL,_T("Cannot create a file dialog"),_T("ERROR"),MB_OK|MB_ICONHAND);
  }
  return res;
}

XString 
DocFileDialog::GetChosenFile()
{
  return (XString) m_ofn.lpstrFile;
}

void
DocFileDialog::FilterString(PTCHAR filter)
{
  PTCHAR pnt = filter;
  while(*pnt)
  {
    if(*pnt == '|')
    {
      *pnt = 0;
    }
    ++pnt;
  }
  *pnt++ = 0;
  *pnt++ = 0;
}
