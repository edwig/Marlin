/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: BrowseForDirectory.h
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
#pragma once

class BrowseForDirectory 
{
public:

   BrowseForDirectory();
   virtual ~BrowseForDirectory();

  bool Browse(HWND            hwndParent, 
              CString const&  title, 
              CString const&  initdir    = "",
              CString const&  rootdir    = "",
              bool            showFiles  = false,
              bool            showStatus = false);

  CString const& GetPath() const { return m_path; }
protected:
  //  These functions can only be called from
  //  within the OnSelChange function, so it
  //  is reasonable to keep them as protected.
  void EnableOk(bool bEnable);
  void SetSelection(CString const& path);
  void SetStatusText(CString const& text);

private:

  virtual void OnInitialized();
  virtual void OnSelChange(CString const& path);
         int          CallbackProc (HWND hwnd,UINT uMsg,LPARAM lParam);
  static int CALLBACK CallbackProcS(HWND hwnd,UINT uMsg,LPARAM lParam,LPARAM lpData);

  HWND      m_hwnd;
  char      m_originalDir[MAX_PATH+1];
  CString   m_disp;
  CString   m_path;
  CString   m_root;
  CString   m_init;
};
