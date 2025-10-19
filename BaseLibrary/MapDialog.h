/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: MapDialog.h
//
// BaseLibrary: Indispensable general objects and functions
// 
// Created: 2014-2025 ir. W.E. Huisman
// MIT License
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

class MapDialog 
{
public:

   MapDialog();
   virtual ~MapDialog();

  bool Browse(HWND            hwndParent, 
              XString const&  title, 
              XString const&  initdir    = _T(""),
              XString const&  rootdir    = _T(""),
              bool            showFiles  = false,
              bool            showStatus = false);

  XString const& GetPath() const { return m_path; }
protected:
  //  These functions can only be called from
  //  within the OnSelChange function, so it
  //  is reasonable to keep them as protected.
  void EnableOk(bool bEnable);
  void SetSelection(const XString& path);
  void SetStatusText(const XString& text);

private:

  virtual void OnInitialized();
  virtual void OnSelChange(const XString& path);
          int  CallbackProc(HWND hwnd,UINT uMsg,LPARAM lParam);
  static int CALLBACK CallbackProcS(HWND hwnd,UINT uMsg,LPARAM lParam,LPARAM lpData);

  HWND      m_hwnd;
  TCHAR     m_originalDir[MAX_PATH+1];
  XString   m_disp;
  XString   m_path;
  XString   m_root;
  XString   m_init;
};
