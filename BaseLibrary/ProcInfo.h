/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: ProcInfo.h
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
#pragma once
#include <windows.h>
#include <vector>

// Maximum size of the environment variables on MS-Windows
const int MAX_ENVSPACE = (32 * 1024);

//	Module information
class ProcModule 
{
public:

  ProcModule()
  {
    memset(&m_file_info, 0, sizeof(m_file_info));
    memset(&m_version_info, 0, sizeof(m_version_info));
  }

  XString          m_file_name;
  XString          m_file_path;
  XString          m_full_path;
  
  XString          m_load_address;

  XString          m_company_name;
  XString          m_file_description;
  XString          m_fileversion;
  XString          m_internal_name;
  XString	         m_legal_copyright;
  XString          m_original_filename;
  XString          m_product_name;
  XString          m_product_version;

  XString          m_trademarks;
  XString          m_private_build;
  XString          m_special_build;
  XString          m_comments;

  WIN32_FIND_DATA	 m_file_info;
  VS_FIXEDFILEINFO m_version_info;
};

class  ProcInfo 
{
public:
  #define STATUS_SUCCESS (0x00000000)

  typedef ProcModule Module;
  typedef LONG NTSTATUS, * PNTSTATUS;
  typedef NTSTATUS(WINAPI* RtlGetVersionPtr)(PRTL_OSVERSIONINFOW);

  //	Helper functions
  static Module* LoadModule(HMODULE);
  
  //  Construction
  ProcInfo();
 ~ProcInfo();

  //	Main discovery functions
  void GetSystemType();			//	System version
  void GetSystemInfo();			//	System settings
  void GetLocaleInfo();			//	Locale settings
  void GetApplicInfo();			//	Application information
  void GetModuleInfo();			//	Loaded modules

  void WriteToFile(XString const& Filename) const;

  using ModuleList = std::vector<Module*>;

  XString			m_datetime;
  XString			m_application;
  XString			m_commandline;

  int         m_version;
  XString			m_platform;
  XString			m_computer;
  XString			m_username;
  XString			m_pathspec;

  XString			m_windows_path;
  XString			m_current_path;

  XString			m_system_number;
  XString			m_system_date;
  XString			m_system_time;
  XString			m_user_number;
  XString			m_user_date;
  XString			m_user_time;

  ModuleList  m_modules;
private:
  static
  XString             ReadLangString(LPVOID      pVI
                                    ,char const* pszFormatString
                                    ,DWORD       dwLang
                                    ,char const* pszStringName);
  bool                IsWin10AnniversaryOrHigher();
  RTL_OSVERSIONINFOW  getRealOSVersion();
  bool				        m_use_psapi;

};
