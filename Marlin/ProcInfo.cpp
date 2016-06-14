/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: ProcInfo.cpp
//
// Marlin Server: Internet server/client
// 
// Copyright (c) 2016 ir. W.E. Huisman
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
#include "ProcInfo.h"
#include <fstream>
#include <Tlhelp32.h>
#include <Lmcons.h> // defines UNLEN

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#pragma comment(lib, "version.lib")

//=============================================================================

//	PSAPI.DLL on WinNT 4.0
typedef BOOL (WINAPI *EnumProcessModulesProc)(HANDLE hProcess, HMODULE * lphModule, DWORD cb, LPDWORD lpcbNeeded);

//	KERNEL32.DLL on Win9x/W2K/XP/higher
typedef HANDLE (WINAPI *CreateToolhelp32SnapshotProc)(DWORD dwFlags, DWORD th32ProcessID);

//=============================================================================

typedef BOOL (WINAPI *Process32FirstProc)(HANDLE hSnapshot, LPPROCESSENTRY32 lppe);
typedef BOOL (WINAPI *Process32NextProc) (HANDLE hSnapshot, LPPROCESSENTRY32 lppe);
typedef BOOL (WINAPI *Module32FirstProc) (HANDLE hSnapshot, LPMODULEENTRY32  lpme);
typedef BOOL (WINAPI *Module32NextProc)  (HANDLE hSnapshot, LPMODULEENTRY32  lpme);

//=============================================================================

class CDllLoader
{
public:
  CDllLoader(CString const& name) : m_hDll (0)
    { m_hDll = LoadLibrary(name); }

  ~CDllLoader()
    { if(m_hDll) FreeLibrary(m_hDll); }

  operator HINSTANCE() const
    { return m_hDll; }

  operator bool () const
    { return m_hDll != 0; }

  bool operator ! () const 
    { return m_hDll == 0; }

  template <class T>
  void GetProcAddress(CString const& name, T& funcptr)
    { funcptr = (T) ::GetProcAddress(m_hDll, name); }

private:
  HINSTANCE m_hDll;
};

//=============================================================================

CString StToString(SYSTEMTIME const& st)
{
  char buff[25];
  sprintf_s(buff,25,"%02d:%02d:%02d %02d/%02d/%02d", 
            st.wHour, st.wMinute, st.wSecond, 
            st.wDay, st.wMonth, st.wYear);

  return buff;
}


//=============================================================================

CString FtToString(FILETIME const& ft)
{
  SYSTEMTIME st;
  FileTimeToSystemTime(&ft, &st);
  return StToString(st);
}


//=============================================================================

CString MakeWidth(CString const& input, int length)
{
  CString result = input;
  while(result.GetLength() < length)
  {
    result += " ";
  }
  return result;
}


//=============================================================================

CString AttribToString(DWORD dwAttrib)
{
  CString result;
  #define ATTRIB_CASE(arg) if((dwAttrib & arg) == arg) result += #arg " "
  ATTRIB_CASE(FILE_ATTRIBUTE_ARCHIVE);
  ATTRIB_CASE(FILE_ATTRIBUTE_COMPRESSED);
  ATTRIB_CASE(FILE_ATTRIBUTE_DIRECTORY);
  ATTRIB_CASE(FILE_ATTRIBUTE_ENCRYPTED);
  ATTRIB_CASE(FILE_ATTRIBUTE_HIDDEN);
  ATTRIB_CASE(FILE_ATTRIBUTE_NORMAL);
  ATTRIB_CASE(FILE_ATTRIBUTE_NOT_CONTENT_INDEXED);
  ATTRIB_CASE(FILE_ATTRIBUTE_OFFLINE);
  ATTRIB_CASE(FILE_ATTRIBUTE_READONLY);
  ATTRIB_CASE(FILE_ATTRIBUTE_REPARSE_POINT);
  ATTRIB_CASE(FILE_ATTRIBUTE_SPARSE_FILE);
  ATTRIB_CASE(FILE_ATTRIBUTE_SYSTEM);
  ATTRIB_CASE(FILE_ATTRIBUTE_TEMPORARY); 
  #undef ATTRIB_CASE
  return result;
}


//=============================================================================

ProcInfo::ProcInfo()
         :m_use_psapi(false)
{
  //	Store current date and time
  SYSTEMTIME st;
  GetLocalTime(&st);
  m_datetime = StToString(st);

  //	Gather system information
  GetSystemType();
  GetSystemInfo();
  GetLocaleInfo();
  GetApplicInfo();
  GetModuleInfo();
}

ProcInfo::~ProcInfo()
{
  for(auto& module : m_modules)
  {
    delete module;
  }
}

//=============================================================================

#pragma warning (disable:4996)
// Shut op about IsWindows* macros from VersionHelpers.h
#pragma warning (disable: 28159)

void ProcInfo::GetSystemType()
{
  OSVERSIONINFOEX ove;

  //  Get regular info first
  ove.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
  GetVersionEx((LPOSVERSIONINFO)&ove);

  //  For NT 5 and later, we can gather more info
  bool isNT5 = ove.dwPlatformId == VER_PLATFORM_WIN32_NT && ove.dwMajorVersion >= 5;
  bool WS    = ove.wProductType == VER_NT_WORKSTATION;

  if(isNT5)
  {
    ove.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    if(!GetVersionEx((LPOSVERSIONINFO)&ove))
    {
      return;
    }
  }
  //	Describe platform
  if(ove.dwPlatformId == VER_PLATFORM_WIN32s)
  {
    switch(ove.dwMinorVersion)
    {
      case 51:		m_platform = "Windows NT 3.51";		break;
      default:		m_platform = "Unknown NT 3.xx";		break;
    }
  }
  else if(ove.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
  {
    m_use_psapi = false;
    switch(ove.dwMinorVersion)
    {
      case 0:			m_platform = "Windows 95";				break;
      case 10:		m_platform = "Windows 98";				break;
      case 90:		m_platform = "Windows Me";				break;
      default:		m_platform = "Unknown 9x";				break;
    }
  }
  else if(ove.dwPlatformId == VER_PLATFORM_WIN32_NT)
  {
    m_use_psapi = false;
    switch(ove.dwMajorVersion)
    {
      case 4:     m_platform = "Windows NT 4.0";
                  break;
      case 5:     switch(ove.dwMinorVersion)
                  {
                    case 0:			m_platform = "Windows 2000";		break;
                    case 1:			m_platform = "Windows XP";			break;
                    case 2:			m_platform = "Windows 2003";		break;
                    default:		m_platform = "Unknown NT";			
                                m_use_psapi = true;		
                                break;
                  }
                  break;
      case 6:			switch(ove.dwMinorVersion)
                  {
                    case 0: m_platform = WS ? "Windows Server 2008"    : "Windows Vista";
                            break;
                    case 1: m_platform = WS ? "Windows Server 2008 R2" : "Windows-7";
                            break;
                    case 2: m_platform = WS ? "Windows Server 2012"    : "Windows-8";
                            break;
                    case 3: m_platform = WS ? "Windows Server 2012 R2" : "Windows-8.1";
                            break;
                    case 4: m_platform = WS ? "Windows Server 2016"    : "Windows 10";
                            break;
                    default:m_platform = "Yet unknown MS-Windows version";
                            break;
                  }
                  break;
      default:    m_platform = "Yet unknown MS-Windows version";
                  break;
    }
  }
  //	Add version designation
  if(strlen(ove.szCSDVersion))
  {
    m_platform += " ";
    m_platform += ove.szCSDVersion;
  }
  //	Add actual version number
  char szVersion[25];
  sprintf_s(szVersion, 25, "%d.%d.%d", 
          ove.dwMajorVersion, 
          ove.dwMinorVersion, 
          ove.dwBuildNumber);
  m_platform += " (";
  m_platform += szVersion;
  m_platform += ")";
}
  

//=============================================================================

void ProcInfo::GetSystemInfo()
{
  char* buffer = new char[MAX_ENVSPACE];
  DWORD dwLength;

  //	Determine computer name
  dwLength = MAX_COMPUTERNAME_LENGTH + 1;
  if(GetComputerName(buffer, &dwLength))
  {
    m_computer = buffer;
  }

  //	Determine username
  dwLength = UNLEN + 1;
  if(GetUserName(buffer, &dwLength))
  {
    m_username = buffer;
  }

  //	Windows directory
  if(GetWindowsDirectory(buffer, MAX_PATH))
  {
    m_windows_path = buffer;
  }

  //	Current directory
  if(GetCurrentDirectory(MAX_PATH,buffer))
  {
    m_current_path = buffer;
  }

  //	Determine path (buffersize - 2 trailing '0')!!
  if(ExpandEnvironmentStrings("%PATH%",buffer,MAX_ENVSPACE - 2))
  {
    m_pathspec = buffer;
  }

  // Free the buffer
  delete [] buffer;
}

//=============================================================================

void ProcInfo::GetLocaleInfo()
{
  char szBuffer[50];

  //	Define date/time
  SYSTEMTIME st;
  st.wDay			= 1;
  st.wMonth		= 2;
  st.wYear		= 2003;
  st.wHour		= 4;
  st.wMinute  = 5;
  st.wSecond  = 6;

  //	Not used
  st.wDayOfWeek    = 0;
  st.wMilliseconds = 0;

  //	Get system locale info
  GetDateFormat(LOCALE_SYSTEM_DEFAULT, 0, &st, 0, szBuffer, 50);
  m_system_date = szBuffer;
  GetTimeFormat(LOCALE_SYSTEM_DEFAULT, 0, &st, 0, szBuffer, 50);
  m_system_time = szBuffer;
  GetNumberFormat(LOCALE_SYSTEM_DEFAULT, 0, "-1234.56", 0, szBuffer, 15);
  m_system_number = szBuffer;

  //	Get user locale info
  GetDateFormat(LOCALE_USER_DEFAULT, 0, &st, 0, szBuffer, 50);
  m_user_date = szBuffer;
  GetTimeFormat(LOCALE_USER_DEFAULT, 0, &st, 0, szBuffer, 50);
  m_user_time = szBuffer;
  GetNumberFormat(LOCALE_USER_DEFAULT, 0, "-1234.56", 0, szBuffer, 15);
  m_user_number = szBuffer;
}


//=============================================================================

void ProcInfo::GetApplicInfo()
{
  char szBuffer[4096];

  //	Get application path
  GetModuleFileName(GetModuleHandle(0), szBuffer, MAX_PATH);
  m_application = szBuffer;

  //	Get commandline
  m_commandline = GetCommandLine();
}


//=============================================================================

void ProcInfo::GetModuleInfo()
{
  if(m_use_psapi)			//	Windows NT 4.0 enumerator code
  {
    //  Load the required dll
    CDllLoader psapi("psapi.dll");
  
    //  Get address of enum proc
    EnumProcessModulesProc EnumProcessModules;
    psapi.GetProcAddress("EnumProcessModules", EnumProcessModules);

    //  Enumerate modules
    HMODULE hModules[500];
    DWORD dwModules;
    if(EnumProcessModules(GetCurrentProcess(), hModules, 500, &dwModules))
    {
      int num = dwModules / sizeof(HMODULE);
      for(int n = 0; n < num; ++n)
      {
        m_modules.push_back(LoadModule(hModules[n]));
      }
    }
  }
  else	//	Windows 9x/2000/XP/2003 enumerator code
  { 
    //	Load the required dll
    CDllLoader toolhelp("kernel32.dll");

    //	Get function addresses
    CreateToolhelp32SnapshotProc	ctsp;
    Module32FirstProc							m32f;
    Module32NextProc							m32n;
    toolhelp.GetProcAddress("CreateToolhelp32Snapshot", ctsp);
    toolhelp.GetProcAddress("Module32First", m32f);
    toolhelp.GetProcAddress("Module32Next",  m32n);

    //	Create the snapshot
    HANDLE hSnap = ctsp(TH32CS_SNAPMODULE, 0);
    if(hSnap != INVALID_HANDLE_VALUE)
    {
      MODULEENTRY32 me;
      me.dwSize = sizeof(me);
      if(m32n(hSnap, &me))
      {
        do
        {
          m_modules.push_back(LoadModule(me.hModule));
        } 
        while(m32n(hSnap, &me));
      }
      //	Done; cleanup snapshot
      CloseHandle(hSnap);
    }
  }
}

//=============================================================================

CString
ProcInfo::ReadLangString(LPVOID      pVI
                        ,char const* pszFormatString
                        ,DWORD       dwLang
                        ,char const* pszStringName)
{
  char szLangString[MAX_PATH];
  char const* pszString;
  UINT len;

  sprintf_s(szLangString,MAX_PATH,pszFormatString,dwLang,pszStringName);
  if(VerQueryValue(pVI,szLangString,(LPVOID*)&pszString,&len))
  {
    return pszString;
  }
  return "";
}

ProcModule* 
ProcInfo::LoadModule(HMODULE hModule)
{
  Module* module = new Module();

  //	Format hModule, which is the load address 
  char buff[40];

#if defined _M_IX86
  sprintf_s(buff,20, "0x%08X", (unsigned int)hModule);
#endif
#if defined _M_X64
sprintf_s(buff,40, "0x%08I64X", (__int64)hModule);
#endif
  
  module->m_load_address = buff;

  //  Get absolute file name
  char filename[MAX_PATH + 1];
  if(!GetModuleFileName(hModule, filename, MAX_PATH))
  {
    return module;
  }
  module->m_full_path = filename;

  //  Get file information
  HANDLE hFind = FindFirstFile(filename, &module->m_file_info);
  if(hFind != INVALID_HANDLE_VALUE) 
  {
    FindClose(hFind);   
  }
  module->m_file_name = module->m_file_info.cFileName;
  module->m_full_path = filename;
  module->m_file_path = module->m_full_path.Left(
                        module->m_full_path.GetLength() - 
                        module->m_file_name.GetLength() - 1);

  //  Get version information size
  DWORD dwUselessParam;
  DWORD dwVersionSize = GetFileVersionInfoSize((char*) (char const*) module->m_full_path, &dwUselessParam);

  //  Load version info
  if(dwVersionSize > 0)
  {
    //  Allocate and retrieve block
    LPVOID pVI = new BYTE[dwVersionSize];
    if(GetFileVersionInfo((char*) (char const*) module->m_full_path, 0, dwVersionSize, pVI))
    {
      UINT len;

      //  Copy fixed info block
      VS_FIXEDFILEINFO* pffi;
      if(VerQueryValue(pVI, "\\", (LPVOID*) &pffi, &len))
      {
        memcpy(&module->m_version_info, pffi, sizeof(module->m_version_info));
      }

      //  Read version strings
      LPBYTE lpvi;
      if(VerQueryValue(pVI, "\\VarFileInfo\\Translation", (LPVOID*) &lpvi, &len))
      {
        //  Determine language-specific key
        DWORD dwLCP = lpvi[2] + (lpvi[3] << 8) + (lpvi[0] << 16) + (lpvi[1] << 24);
        char szLangFormat[] = "\\StringFileInfo\\%08X\\%s";

        //  Read strings
        module->m_company_name       = ReadLangString(pVI, szLangFormat, dwLCP, "CompanyName");
        module->m_file_description   = ReadLangString(pVI, szLangFormat, dwLCP, "FileDescription");
        module->m_fileversion        = ReadLangString(pVI, szLangFormat, dwLCP, "FileVersion");
        module->m_internal_name      = ReadLangString(pVI, szLangFormat, dwLCP, "InternalName");
        module->m_legal_copyright    = ReadLangString(pVI, szLangFormat, dwLCP, "LegalCopyright");
        module->m_original_filename  = ReadLangString(pVI, szLangFormat, dwLCP, "OriginalFileName");
        module->m_product_name       = ReadLangString(pVI, szLangFormat, dwLCP, "ProductName");
        module->m_product_version    = ReadLangString(pVI, szLangFormat, dwLCP, "ProductVersion");
        module->m_trademarks         = ReadLangString(pVI, szLangFormat, dwLCP, "LegalTrademarks");
        module->m_private_build      = ReadLangString(pVI, szLangFormat, dwLCP, "PrivateBuild");
        module->m_special_build      = ReadLangString(pVI, szLangFormat, dwLCP, "SpecialBuild");
        module->m_comments           = ReadLangString(pVI, szLangFormat, dwLCP, "Comments");
      }
    }
    //  Cleanup
    delete [] (BYTE*) pVI;
  }
  
  return module;
}


//=============================================================================
/*
void ProcInfo::WriteToFile(CString const& Filename) const
{
  std::ofstream of(Filename);

  //	Write info header
  of << "=================================================================\n";
  of << "  Application information\n";
  of << "=================================================================\n";
  of << "Current date/time: " << m_datetime << "\n";
  of << "Application:       " << m_application << "\n";
  of << "Command line:      " << m_commandline << "\n";
  of << "\n\n";

  //	Write platform info
  of << "=================================================================\n";
  of << "  Platform information\n";
  of << "=================================================================\n";
  of << "Computer name:     " << m_computer << "\n";
  of << "Current user:      " << m_username << "\n";
  of << "Windows version:   " << m_platform << "\n";
  of << "Windows directory: " << m_windows_path << "\n";
  of << "Current directory: " << m_current_path << "\n";
  of << "%PATH%:            " << m_pathspec << "\n";
  of << "\n\n";

  //	Write locale info
  of << "=================================================================\n";
  of << "  Locale information\n";
  of << "=================================================================\n";
  of << "Value type         System locale       User locale\n";
  of << "-----------------------------------------------------------------\n";
  of << "Number            " << MakeWidth(m_system_number, 20) << m_user_number << "\n";
  of << "Date              " << MakeWidth(m_system_date, 20)   << m_user_date   << "\n";
  of << "Time              " << MakeWidth(m_system_time, 20)   << m_user_time   << "\n";
  of << "\n\n";

  //	Write modules
  ModuleList::const_iterator ii, ie;
  ii = m_modules.begin();
  ie = m_modules.end();
  for(; ii != ie; ++ii)
  {
    Module const& mod = *ii;
    of << "=================================================================\n";
    of << "  Module information for " << mod.m_file_name << "\n";
    of << "=================================================================\n";
    of << "Load address:      " << mod.m_load_address << "\n";
    of << "File name:         " << mod.m_file_name << "\n";
    of << "File path:         " << mod.m_full_path << "\n";
    of << "File size:         " << mod.m_file_info.nFileSizeLow << "\n";
    of << "File created:      " << FtToString(mod.m_file_info.ftCreationTime) << "\n";
    of << "File modified:     " << FtToString(mod.m_file_info.ftLastWriteTime) << "\n";
    of << "File attributes:   " << AttribToString(mod.m_file_info.dwFileAttributes) << "\n";
    of << "Company name:      " << mod.m_company_name << "\n";
    of << "File description:  " << mod.m_file_description << "\n";
    of << "File version:      " << mod.m_fileversion << "\n";
    of << "Internal name:     " << mod.m_internal_name << "\n";
    of << "Legal copyright:   " << mod.m_legal_copyright << "\n";
    of << "Original filename: " << mod.m_original_filename << "\n";
    of << "Product name:      " << mod.m_product_name << "\n";
    of << "Product version:   " << mod.m_product_version << "\n";
    of << "\n\n";
  }
}

*/
//=============================================================================
