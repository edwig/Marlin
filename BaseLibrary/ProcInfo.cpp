/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: ProcInfo.cpp
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
#include "pch.h"
#include "ProcInfo.h"
#include <fstream>
#include <Tlhelp32.h>
#include <Lmcons.h> // defines UNLEN
#include <VersionHelpers.h>

#pragma comment(lib, "version.lib")

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

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
  explicit CDllLoader(XString const& name)
  {
    m_hDll = LoadLibrary(name); 
  }

  ~CDllLoader()
  { 
    if(m_hDll)
    {
      FreeLibrary(m_hDll);
    }
  }

  operator HINSTANCE() const
  {
    return m_hDll;
  }

  operator bool () const
  { 
    return m_hDll != 0;
  }

  bool operator ! () const 
  { 
    return m_hDll == 0;
  }

  template <class T>
  void GetProcAddress(XString const& name, T& funcptr)
  {
    funcptr = (T) ::GetProcAddress(m_hDll,CT2CA(name.GetString()));
  }

private:
  HINSTANCE m_hDll { NULL };
};

//=============================================================================

XString 
StToString(SYSTEMTIME const& st)
{
  TCHAR buff[25];
  _stprintf_s(buff,25,_T("%02d-%02d-%02d %04d:%02d:%02d"), 
            st.wDay, st.wMonth, st.wYear,
            st.wHour,st.wMinute,st.wSecond);
  return XString(buff);
}

XString 
FtToString(FILETIME const& ft)
{
  SYSTEMTIME st;
  FileTimeToSystemTime(&ft, &st);
  return StToString(st);
}

XString 
MakeWidth(XString const& input, int length)
{
  XString result = input;
  while(result.GetLength() < length)
  {
    result += _T(" ");
  }
  return result;
}

XString 
AttribToString(DWORD dwAttrib)
{
  XString result;
  #define ATTRIB_CASE(arg) if((dwAttrib & arg) == arg) result += #arg _T(" ")
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
         ,m_version(0)
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
  for(auto& loadModule : m_modules)
  {
    delete loadModule;
  }
}

#pragma warning (disable: 4996)

bool 
ProcInfo::IsWin10AnniversaryOrHigher()
{
  return getRealOSVersion().dwBuildNumber >= 14393;
}

RTL_OSVERSIONINFOW 
ProcInfo::getRealOSVersion()
{
  HMODULE hMod = ::GetModuleHandle(_T("ntdll.dll"));
  if(hMod)
  {
    // BEWARE: GetProcAddress is always in MBCS encoding
    RtlGetVersionPtr fxPtr = (RtlGetVersionPtr)::GetProcAddress(hMod,"RtlGetVersion");
    if(fxPtr != nullptr)
    {
      RTL_OSVERSIONINFOW rovi = { 0 };
      rovi.dwOSVersionInfoSize = sizeof(rovi);
      if(STATUS_SUCCESS == fxPtr(&rovi))
      {
        return rovi;
      }
    }
  }
  RTL_OSVERSIONINFOW rovi = { 0 };
  return rovi;
}


void 
ProcInfo::GetSystemType()
{
  m_platform.Empty();

  if(IsWindows10OrGreater())
  {
    m_platform = _T("Windows 10 or greater.");
  }
  else if(IsWin10AnniversaryOrHigher())
  {
    m_platform = _T("Windows 10 Anniversary or greater.");
  }
  else if(IsWindows8Point1OrGreater())
  {
    m_platform = _T("Windows 8.1 or greater.");
  }
  else if(IsWindows8OrGreater())
  {
    m_platform = _T("Windows 8.0 or greater.");
  }
  else if(IsWindows7SP1OrGreater())
  {
    m_platform = _T("Windows 7 SP1 or greater.");
  }
  else if(IsWindows7OrGreater())
  {
    m_platform = _T("Windows 7 or greater.");
  }
  else if(IsWindowsVistaSP2OrGreater())
  {
    m_platform = _T("Windows Vista SP2 or greater.");
  }
  else if(IsWindowsVistaSP1OrGreater())
  {
    m_platform = _T("Windows Vista SP1 or greater.");
  }
  else if(IsWindowsVistaOrGreater())
  {
    m_platform = _T("Windows Vista or greater.");
  }
  else if(IsWindowsXPSP3OrGreater())
  {
    m_platform = _T("Windows XP SP3 or greater)");
  }
  else if(IsWindowsXPSP2OrGreater())
  {
    m_platform = _T("Windows XP SP2 or greater");
  }
  else if(IsWindowsXPSP1OrGreater())
  {
    m_platform = _T("Windows XP SP1 or greater");
  }
  else if(IsWindowsXPOrGreater())
  {
    m_platform = _T("Windows XP or greater");
  }

  // Check for Terminal Server versions
  if(IsWindowsServer())
  {
    m_platform += _T(" Server.");
  }
  if(IsActiveSessionCountLimited())
  {
    m_platform += _T(" (Limited sessions)");
  }
}

//=============================================================================

void 
ProcInfo::GetSystemInfo()
{
  PTCHAR buffer = new TCHAR[MAX_ENVSPACE];
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
  if(ExpandEnvironmentStrings(_T("%PATH%"),buffer,MAX_ENVSPACE - 2))
  {
    m_pathspec = buffer;
  }

  // Free the buffer
  delete [] buffer;
}

//=============================================================================

void 
ProcInfo::GetLocaleInfo()
{
  TCHAR szBuffer[50];

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
  GetNumberFormat(LOCALE_SYSTEM_DEFAULT, 0, _T("-1234.56"), 0, szBuffer, 15);
  m_system_number = szBuffer;

  //	Get user locale info
  GetDateFormat(LOCALE_USER_DEFAULT, 0, &st, 0, szBuffer, 50);
  m_user_date = szBuffer;
  GetTimeFormat(LOCALE_USER_DEFAULT, 0, &st, 0, szBuffer, 50);
  m_user_time = szBuffer;
  GetNumberFormat(LOCALE_USER_DEFAULT, 0, _T("-1234.56"), 0, szBuffer, 15);
  m_user_number = szBuffer;
}

//=============================================================================

void 
ProcInfo::GetApplicInfo()
{
  TCHAR szBuffer[4096];

  //	Get application path
  GetModuleFileName(GetModuleHandle(0), szBuffer, MAX_PATH);
  m_application = szBuffer;

  //	Get command line
  m_commandline = GetCommandLine();
}

//=============================================================================

void 
ProcInfo::GetModuleInfo()
{
  if(m_use_psapi)			//	Windows NT 4.0 enumerator code
  {
    //  Load the required dll
    CDllLoader psapi(_T("psapi.dll"));
  
    //  Get address of enum process
    EnumProcessModulesProc EnumProcessModules;
    psapi.GetProcAddress(_T("EnumProcessModules"), EnumProcessModules);

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
    CDllLoader toolhelp(_T("kernel32.dll"));

    //	Get function addresses
    CreateToolhelp32SnapshotProc	ctsp;
    Module32FirstProc							m32f;
    Module32NextProc							m32n;
    toolhelp.GetProcAddress(_T("CreateToolhelp32Snapshot"), ctsp);
    toolhelp.GetProcAddress(_T("Module32First"), m32f);
    toolhelp.GetProcAddress(_T("Module32Next"),  m32n);

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

XString
ProcInfo::ReadLangString(LPVOID pVI
                        ,PTCHAR pszFormatString
                        ,DWORD  dwLang
                        ,PTCHAR pszStringName)
{
  TCHAR  szLangString[MAX_PATH];
  PTCHAR pszString;
  UINT len;

  _stprintf_s(szLangString,MAX_PATH,pszFormatString,dwLang,pszStringName);
  if(VerQueryValue(pVI,szLangString,(LPVOID*)&pszString,&len))
  {
    return pszString;
  }
  return _T("");
}

ProcModule* 
ProcInfo::LoadModule(HMODULE hModule)
{
  Module* loadModule = new Module();

  //	Format hModule, which is the load address 
  TCHAR buff[40];

#if defined _M_IX86
  _stprintf_s(buff,20,_T("0x%08X"),(unsigned int)hModule);
#endif
#if defined _M_X64
  _stprintf_s(buff,40,_T("0x%08I64X"),(__int64)hModule);
#endif
  
  loadModule->m_load_address = buff;

  //  Get absolute file name
  TCHAR filename[MAX_PATH + 1];
  if(!GetModuleFileName(hModule,filename,MAX_PATH))
  {
    return loadModule;
  }
  loadModule->m_full_path = filename;

  //  Get file information
  HANDLE hFind = FindFirstFile(filename, &loadModule->m_file_info);
  if(hFind != INVALID_HANDLE_VALUE) 
  {
    FindClose(hFind);   
  }
  loadModule->m_file_name = loadModule->m_file_info.cFileName;
  loadModule->m_full_path = filename;
  loadModule->m_file_path = loadModule->m_full_path.Left(
                            loadModule->m_full_path.GetLength() - 
                            loadModule->m_file_name.GetLength() - 1);

  //  Get version information size
  DWORD dwUselessParam;
  DWORD dwVersionSize = GetFileVersionInfoSize((PTCHAR) loadModule->m_full_path.GetString(), &dwUselessParam);

  //  Load version info
  if(dwVersionSize > 0)
  {
    //  Allocate and retrieve block
    LPVOID pVI = new BYTE[dwVersionSize];
    if(GetFileVersionInfo((PTCHAR)loadModule->m_full_path.GetString(),0,dwVersionSize, pVI))
    {
      UINT len;

      //  Copy fixed info block
      VS_FIXEDFILEINFO* pffi;
      if(VerQueryValue(pVI, _T("\\"), (LPVOID*) &pffi, &len))
      {
        memcpy(&loadModule->m_version_info, pffi, sizeof(loadModule->m_version_info));
      }

      //  Read version strings
      LPBYTE lpvi;
      if(VerQueryValue(pVI,_T("\\VarFileInfo\\Translation"), (LPVOID*) &lpvi, &len))
      {
        //  Determine language-specific key
        DWORD dwLCP = lpvi[2] + (lpvi[3] << 8) + (lpvi[0] << 16) + (lpvi[1] << 24);
        TCHAR szLangFormat[] = _T("\\StringFileInfo\\%08X\\%s");

        //  Read strings
        loadModule->m_company_name       = ReadLangString(pVI, szLangFormat, dwLCP, _T("CompanyName"));
        loadModule->m_file_description   = ReadLangString(pVI, szLangFormat, dwLCP, _T("FileDescription"));
        loadModule->m_fileversion        = ReadLangString(pVI, szLangFormat, dwLCP, _T("FileVersion"));
        loadModule->m_internal_name      = ReadLangString(pVI, szLangFormat, dwLCP, _T("InternalName"));
        loadModule->m_legal_copyright    = ReadLangString(pVI, szLangFormat, dwLCP, _T("LegalCopyright"));
        loadModule->m_original_filename  = ReadLangString(pVI, szLangFormat, dwLCP, _T("OriginalFileName"));
        loadModule->m_product_name       = ReadLangString(pVI, szLangFormat, dwLCP, _T("ProductName"));
        loadModule->m_product_version    = ReadLangString(pVI, szLangFormat, dwLCP, _T("ProductVersion"));
        loadModule->m_trademarks         = ReadLangString(pVI, szLangFormat, dwLCP, _T("LegalTrademarks"));
        loadModule->m_private_build      = ReadLangString(pVI, szLangFormat, dwLCP, _T("PrivateBuild"));
        loadModule->m_special_build      = ReadLangString(pVI, szLangFormat, dwLCP, _T("SpecialBuild"));
        loadModule->m_comments           = ReadLangString(pVI, szLangFormat, dwLCP, _T("Comments"));
      }
    }
    //  Cleanup
    delete [] (BYTE*) pVI;
  }
  
  return loadModule;
}


//=============================================================================

void ProcInfo::WriteToFile(XString const& p_filename) const
{
  WinFile file(p_filename);
  if(!file.Open(winfile_write | open_trans_text))
  {
    return;
  }
  XString line(_T("=================================================================\n"));

  //	Write info header
  file.Write(line);
  file.Write(_T("= Application information\n"));
  file.Write(line);
  file.Format(_T("Application      : %s\n"),m_application.GetString());
  file.Format(_T("Current date/time: %s\n"),m_datetime.GetString());
  file.Format(_T("Command line     : %s\n"),m_commandline.GetString());
  file.Write (_T("\n"));

  //	Write platform info
  file.Write(line);
  file.Write (_T("= Platform information\n"));
  file.Write(line);
  file.Format(_T("Computer name    : %s\n"),m_computer.GetString());
  file.Format(_T("Current user     : %s\n"),m_username.GetString());
  file.Format(_T("Windows version  : %s\n"),m_platform.GetString());
  file.Format(_T("Windows directory: %s\n"),m_windows_path.GetString());
  file.Format(_T("Current directory: %s\n"),m_current_path.GetString());
  file.Format(_T("%%PATH%%           : %s\n"),m_pathspec.GetString());
  file.Write (_T("\n"));

  //	Write locale info
  file.Write(line);
  file.Write (_T("= Locale information\n"));
  file.Write(line);
  file.Write (_T("Value type         System locale        User locale\n"));
  file.Write (_T("------------------ -------------------- --------------------------\n"));
  file.Format(_T("Number             %-20s %-20s\n"),m_system_number.GetString(),m_user_number.GetString());
  file.Format(_T("Date               %-20s %-20s\n"),m_system_date.GetString(),  m_user_date.GetString());
  file.Format(_T("Time               %-20s %-20s\n"),m_system_time.GetString(),  m_user_time.GetString());
  file.Format(_T("\n"));

  //	Write modules
  for(auto& mod : m_modules)
  {
    file.Write(line);
    file.Format(_T("= Module information for: %s\n"),mod->m_file_name.GetString());
    file.Write(line);
    file.Format(_T("File name        : %s\n"),mod->m_file_name.GetString());
    file.Format(_T("File path        : %s\n"),mod->m_full_path.GetString());
    file.Format(_T("File size        : %lu\n"),mod->m_file_info.nFileSizeLow);
    file.Format(_T("File created     : %s\n"),FtToString(mod->m_file_info.ftCreationTime).GetString());
    file.Format(_T("File modified    : %s\n"),FtToString(mod->m_file_info.ftLastWriteTime).GetString());
    file.Format(_T("File attributes  : %s\n"),AttribToString(mod->m_file_info.dwFileAttributes).GetString());
    file.Format(_T("Load address     : %s\n"),mod->m_load_address.GetString());
    file.Format(_T("Company name     : %s\n"),mod->m_company_name.GetString());
    file.Format(_T("File description : %s\n"),mod->m_file_description.GetString());
    file.Format(_T("File version     : %s\n"),mod->m_fileversion.GetString());
    file.Format(_T("Internal name    : %s\n"),mod->m_internal_name.GetString());
    file.Format(_T("Legal copyright  : %s\n"),mod->m_legal_copyright.GetString());
    file.Format(_T("Original filename: %s\n"),mod->m_original_filename.GetString());
    file.Format(_T("Product name     : %s\n"),mod->m_product_name.GetString());
    file.Format(_T("Product version  : %s\n"),mod->m_product_version.GetString());
    file.Format(_T("\n"));
  }
  file.Close();
}

