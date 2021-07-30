/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: ProcInfo.h
//
// Marlin Server: Internet server/client
// 
// Copyright (c) 2014-2021 ir. W.E. Huisman
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

  CString					 m_file_name;
  CString					 m_file_path;
  CString					 m_full_path;
  
  CString					 m_load_address;

  CString					 m_company_name;
  CString					 m_file_description;
  CString					 m_fileversion;
  CString					 m_internal_name;
  CString					 m_legal_copyright;
  CString					 m_original_filename;
  CString					 m_product_name;
  CString				   m_product_version;

  CString          m_trademarks;
  CString          m_private_build;
  CString          m_special_build;
  CString          m_comments;

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

  void WriteToFile(CString const& Filename) const;

  using ModuleList = std::vector<Module*>;

  CString			m_datetime;
  CString			m_application;
  CString			m_commandline;

  int         m_version;
  CString			m_platform;
  CString			m_computer;
  CString			m_username;
  CString			m_pathspec;

  CString			m_windows_path;
  CString			m_current_path;

  CString			m_system_number;
  CString			m_system_date;
  CString			m_system_time;
  CString			m_user_number;
  CString			m_user_date;
  CString			m_user_time;

  ModuleList  m_modules;
private:
  static
  CString             ReadLangString(LPVOID      pVI
                                    ,char const* pszFormatString
                                    ,DWORD       dwLang
                                    ,char const* pszStringName);
  bool                isWin10AnniversaryOrHigher();
  RTL_OSVERSIONINFOW  getRealOSVersion();
  bool				        m_use_psapi;

};
