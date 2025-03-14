//////////////////////////////////////////////////////////////////////////
//
// File: WinFile.cpp
//
// Everything :-) you can do with a Microsoft MS-Windows file (and faster!)
// Author: W.E. Huisman
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
#include "WinFile.h"
#include "CrackURL.h"
#include "ConvertWideString.h"

#include <atlconv.h>
#include <fileapi.h>
#include <handleapi.h>
#include <shlwapi.h>
#include <commdlg.h>
#include <shlobj.h>
#include <winnt.h>
#include <shellapi.h>
#include <AclAPI.h>
#include <filesystem>
#include <io.h>

#ifdef _AFX
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#endif

//////////////////////////////////////////////////////////////////////////
//
// HERE IS THE MOST IMPORTANT MAGIC!!
// 
// Not part of the public interface
// Most optimized size for a modern OS to read disk blocks just once!
// PAGESIZE (128 * 1024)
//
// Or optimized for the size of a memory page of a X86 processor of 4K
//
static DWORD PAGESIZE = (128 * 1024);

//////////////////////////////////////////////////////////////////////////
//
// CTOR / DTOR
//

// Create empty
// All member initializations are done in the interface definition
WinFile::WinFile()
{
  InitializeCriticalSection(&m_fileaccess);
}

// CTOR from a filename
WinFile::WinFile(XString p_filename)
        :m_filename(p_filename)
{
  InitializeCriticalSection(&m_fileaccess);
}

// CTOR from another WinFile
WinFile::WinFile(WinFile& p_other)
{
  *this = p_other;
  InitializeCriticalSection(&m_fileaccess);
}

// DTOR
WinFile::~WinFile()
{
  // Try to close an opened file. Ignore the errors
  Close();
  // Just to be sure if the closing went wrong
  PageBufferFree();
  // Remove locking section
  DeleteCriticalSection(&m_fileaccess);
}

//////////////////////////////////////////////////////////////////////////
//
// General functions and operations
//
//////////////////////////////////////////////////////////////////////////

bool
WinFile::Open(DWORD       p_flags    /*= winfile_read             */
             ,FAttributes p_attribs  /*= FAttributes::attrib_none */
             ,Encoding    p_encoding /*= Encoding::UTF8           */)
{
  bool result = false;
  bool append = false;

  // Reset the error
  m_error = 0;

  // If the file was already opened, we do NOT do it again
  // keep the file and the openMode from the previous opened action
  // We also fail if we do NOT have a filename
  if(m_file)
  {
    m_error = ERROR_ALREADY_EXISTS;
    return result;
  }
  if(m_filename.IsEmpty())
  {
    m_error = ERROR_FILE_NOT_FOUND;
    return result;
  }
  // Check on uniqueness: you cannot request both
  if((p_flags & FFlag::open_random_access) && 
     (p_flags & FFlag::open_sequential))
  {
    m_error = ERROR_INVALID_FUNCTION;
    return result;
  }

  // All file modes for MS-Windows OS
  DWORD access = 0;
  DWORD sharem = 0;
  DWORD create = 0;
  DWORD attrib = FILE_ATTRIBUTE_NORMAL;

  // Get standard access rights
  access |= (p_flags & FFlag::open_read ) ? GENERIC_READ  : 0;
  access |= (p_flags & FFlag::open_write) ? GENERIC_WRITE : 0;

  // Get shared access rights
  sharem |= (p_flags & FFlag::open_shared_read)  ? FILE_SHARE_READ  : 0;
  sharem |= (p_flags & FFlag::open_shared_write) ? FILE_SHARE_WRITE : 0;

  // Get creation disposition
  switch (p_flags & fflag_filter_create)
  {
    case FFlag::open_and_create:  create = CREATE_ALWAYS;     break;  // Open a new file (removing old content)
    case FFlag::open_and_append:  append = true;                      // Open for appending to the end
                                  [[fallthrough]];
    case FFlag::open_allways:     create = OPEN_ALWAYS;       break;  // Open without remove existing content
    case FFlag::open_if_exists:   create = OPEN_EXISTING;     break;  // Only open if it exists
    case FFlag::open_no_overwrite:create = CREATE_NEW;        break;  // Fails if file exists
    case FFlag::open_truncate:    create = TRUNCATE_EXISTING; break;  // Truncate existing file and open
  }

  // Get the attributes
  attrib |= (p_flags & FFlag::open_random_access) ? FILE_FLAG_RANDOM_ACCESS   : 0;
  attrib |= (p_flags & FFlag::open_sequential)    ? FILE_FLAG_SEQUENTIAL_SCAN : 0;
  attrib |= p_attribs;

  // Getting the encoding in case of a text file
  if((p_flags & open_write) && (p_flags & open_trans_text))
  {
    m_encoding = p_encoding;
  }
  // Acquire a file access lock
  AutoCritSec lock(&m_fileaccess);

  // Try to create/open the file
  m_file = CreateFile((LPCTSTR)m_filename.GetString()
                     ,access
                     ,sharem
                     ,nullptr     // RIGHTS not implemented
                     ,create
                     ,attrib
                     ,nullptr);   // Template file not implemented

  if(m_file == INVALID_HANDLE_VALUE)
  {
    m_error = ::GetLastError();
    m_file  = nullptr;
  }
  else
  {
    // See if we must spool to the end
    if(append)
    {
      LARGE_INTEGER move{ 0, 0 };
      if(::SetFilePointerEx(m_file,move,&move,FILE_END) == 0)
      {
        m_error = ::GetLastError();
        return result;
      }
    }
    else if((p_flags & FFlag::open_write) && (p_flags & FFlag::open_trans_text))
    {
      WriteEncodingBOM();
    }
    // Keep the open flags
    m_openMode = p_flags;
    m_ungetch  = 0;
    result     = true;
  }
  return result;
}

// Flush and close the file to the filesystem
bool
WinFile::Close(bool p_flush /*= false*/)
{
  bool result = false;

  // Reset the error code
  m_error = 0;

  // If file was not opened: it is closed!
  if(!m_file)
  {
    return true;
  }

  // If it was a shared memory segment
  // the file handle was a mapped handle
  if(m_sharedMemory)
  {
    m_file = nullptr;

    // Flush first: ignore error state!
    FlushViewOfFile(m_sharedMemory,m_sharedSize);
    m_sharedSize = 0;
    // Unmap and close the shared memory segment
    if(::UnmapViewOfFile(m_sharedMemory) == 0)
    {
      m_error = ::GetLastError();
      return false;
    }
    return true;
  }

  // Acquire a file access lock
  AutoCritSec lock(&m_fileaccess);

  // In case of a text file: flush our buffers
  PageBufferFlush();

  // Try to flush and ignore eventually occurring errors
  if(p_flush)
  {
    ::FlushFileBuffers(m_file);
  }

  // If opened in lock mode: try to unlock the file first
  // Closing the handle will also do that, but will take some time
  // The delay can hinder your and other applications
  if(m_openMode & FFlag::open_lockmode)
  {
    UnLockFile();
  }

  // Close by closing the handle
  result = CloseHandle(m_file);
  m_file = nullptr;

  // Handle an error
  if(result == false)
  {
    m_error = ::GetLastError();
  }

  // Reset the open mode
  m_openMode = FFlag::no_mode;
  m_ungetch  = 0;
  m_encoding = Encoding::EN_ACP;

  PageBufferFree();
  return result;
}

// Only create the file (leaves the file unopened)
bool
WinFile::Create(FAttributes p_attribs /*=FAttributes::attrib_normal*/)
{
  m_error = 0;

  if(Open(winfile_write,p_attribs))
  {
    if(Close())
    {
      return true;
    }
  }
  m_error = ::GetLastError();
  return false;
}

// Recursively create the desired directory
bool
WinFile::CreateDirectory()
{
  // Reset the error code
  m_error = 0;

  // Make sure we have a filename
  if(m_filename.IsEmpty())
  {
    m_error = ERROR_NOT_FOUND;
    return false;
  }
  XString drive,directory,filename,extension;
  FilenameParts(m_filename,drive,directory,filename,extension);

  XString dirToBeOpened(drive);
  XString path(directory);
  XString part = GetBaseDirectory(path);
  while(!part.IsEmpty())
  {
    dirToBeOpened += _T("\\");
    dirToBeOpened += part;
    if (!::CreateDirectory(dirToBeOpened.GetString(),nullptr))
    {
      m_error = ::GetLastError();
      if(!(m_error == ERROR_ACCESS_DENIED && !path.IsEmpty()))
      {
        if(m_error != ERROR_ALREADY_EXISTS)
        {
          // Cannot create the directory
          return false;
        }
      }
    }
    part = GetBaseDirectory(path);
  }
  return true;
}

bool
WinFile::Exists()
{
  // If the file is opened it obviously exists
  if(m_file)
  {
    return true;
  }
  // If not filename given it obviously does not exists
  if(m_filename.IsEmpty())
  {
    return false;
  }
  // Ask the filesystem, regardless of read/write rights
  return _taccess(m_filename.GetString(),0) != -1;
}

// Check for access to the file or directory
bool
WinFile::CanAccess(bool p_write /*= false*/)
{
  // In case we already had an opened file: use the file mode
  if(m_file)
  {
    // See if file was opened in write-mode
    if(p_write)
    {
      return (m_openMode & FFlag::open_write) > 0;
    }
    // Existence and read-access both granted
    return true;
  }
  
  // Go take a look on the filesystem

  // Default check for read access
  DWORD mode = GENERIC_READ;
  // Also check for write access
  if(p_write)
  {
    mode |= GENERIC_WRITE;
  }
  HANDLE handle = CreateFile(m_filename,
                             mode,
                             FILE_SHARE_READ | FILE_SHARE_WRITE,  // share for reading and writing
                             NULL,                                // no security
                             OPEN_EXISTING,                       // existing file only, testing only
                             FILE_ATTRIBUTE_NORMAL | FILE_FLAG_BACKUP_SEMANTICS,
                             NULL);
  if(handle == INVALID_HANDLE_VALUE)
  {
    return false;
  }
  CloseHandle(handle);
  return true;
}

// Remove the file from the filesystem
bool
WinFile::DeleteFile()
{
  // Reset the error
  m_error = 0;

  if(m_file)
  {
    m_error = ERROR_OPEN_FILES;
    return false;
  }

  if(m_filename.IsEmpty())
  {
    m_error = ERROR_FILE_NOT_FOUND;
    return false;
  }

  // Call the MS-Windows SDK function
  if(::DeleteFile(m_filename.GetString()) == FALSE)
  {
    if(GetLastError() != ERROR_FILE_NOT_FOUND)
    {
      m_error = ::GetLastError();
      return false;
    }
  }
  return true;
}

// Delete the directory from the filesystem
// Returns the number of deleted items (files,directories)
unsigned
WinFile::DeleteDirectory(bool p_recursive /*= false*/)
{
  // Reset the error
  m_error = 0;

  if(m_file)
  {
    m_error = ERROR_OPEN_FILES;
    return 0;
  }

  if(m_filename.IsEmpty())
  {
    m_error = ERROR_FILE_NOT_FOUND;
    return 0;
  }

  std::filesystem::path path(m_filename.GetString());
  std::error_code error;
  uintmax_t removed = 0;

  if(p_recursive)
  {
    removed = std::filesystem::remove_all(path,error);
  }
  else
  {
    removed = std::filesystem::remove(path,error);
  }
  if(error)
  {
    m_error = error.value();
    return 0;
  }
  return (unsigned) removed;
}

// Delete the file by moving it to the trashcan
// by way of the Explorer SH(ell) function.
bool
WinFile::DeleteToTrashcan(bool p_show /*= false*/,bool p_confirm /*= false*/)
{
  // Reset the error
  m_error = 0;

  // Check if file still open
  // in that case we cannot delete it
  if(m_file)
  {
    m_error = ERROR_OPEN_FILES;
    return false;
  }

  // Check that we have a valid filename
  if(m_filename.IsEmpty() || _taccess(m_filename.GetString(),0) != 0)
  {
    m_error = ERROR_FILE_NOT_FOUND;
    return false;
  }

  // Copy the filename for extra 0's after the name
  // so it becomes a list of just one (1) filename
  TCHAR  filelist   [MAX_PATH + 2];
  memset(filelist,0,(MAX_PATH + 2) * sizeof(TCHAR));
  _tcscpy_s(filelist,MAX_PATH,m_filename.GetString());

  SHFILEOPSTRUCT file;
  memset(&file,0,sizeof(SHFILEOPSTRUCT));
  file.wFunc  = FO_DELETE;
  file.pFrom  = filelist;
  file.fFlags = FOF_FILESONLY | FOF_ALLOWUNDO;
  file.lpszProgressTitle = _T("Progress");

  // Add our options
  if(!p_show)
  {
    file.fFlags |= FOF_SILENT;
  }
  if(!p_confirm)
  {
    file.fFlags |= FOF_NOCONFIRMATION;
  }

  // Now go move to trashcan via the shell file operation
  if(SHFileOperation(&file))
  {
    m_error = GetLastError();
    return false;
  }
  return true;
}

// Make a copy of the file
bool
WinFile::CopyFile(XString p_destination,FCopy p_how /*= winfile_copy*/)
{
  // Reset the error
  m_error = 0;

  // Check if we have both names
  if(m_filename.IsEmpty() || p_destination.IsEmpty())
  {
    m_error = ERROR_FILE_NOT_FOUND;
    return false;
  }

  // Check if still open
  if(m_file)
  {
    m_error = ERROR_INVALID_FUNCTION;
    return false;
  }

  // Copy the file
  if(::CopyFileEx(m_filename.GetString(),p_destination.GetString(),nullptr,nullptr,FALSE,p_how) == 0)
  {
    m_error = ::GetLastError();
    return false;
  }
  return true;
}

// Move the file by making a copy and removing the original file
bool
WinFile::MoveFile(XString p_destination,FMove p_how /*= winfile_move*/)
{
  // Reset the error
  m_error = 0;

  // Check if we have both names
  if(m_filename.IsEmpty() || p_destination.IsEmpty())
  {
    m_error = ERROR_FILE_NOT_FOUND;
    return false;
  }

  // Check if still open
  if(m_file)
  {
    m_error = ERROR_INVALID_FUNCTION;
    return false;
  }

  // Copy the file
  if(::MoveFileEx(m_filename.GetString(),p_destination.GetString(),p_how) == 0)
  {
    m_error = ::GetLastError();
    return false;
  }

  // This is our new file position
  m_filename = p_destination;
  return true;
}

// Create a temporary file on the %TMP% directory
bool      
WinFile::CreateTempFileName(XString p_pattern,XString p_extension /*= ""*/)
{
  // Directory for GetTempFileName cannot be larger than (MAX_PATH-14)
  // See documentation on "docs.microsoft.com"
  TCHAR tempdirectory[MAX_PATH - 14];
  ::GetTempPath(MAX_PATH-14,tempdirectory);

  // Getting the temp filename
  TCHAR tempfilename[MAX_PATH + 1];
  UINT unique = 0; // Use auto numbering and creation of the temporary file!
  if(::GetTempFileName(tempdirectory,p_pattern.GetString(),unique,tempfilename) == 0)
  {
    m_error = ::GetLastError();
    return false;
  }

  // Beware: file now exists (empty) and has unique name
  m_filename = tempfilename;

  // Add the optional extension
  if(!p_extension.IsEmpty())
  {
    ::DeleteFile(m_filename.GetString());
    int pos = m_filename.ReverseFind('.');
    if(pos > 0)
    {
      m_filename = m_filename.Left(pos);
    }
    if(p_extension[0] != '.')
    {
      m_filename += '.';
    }
    m_filename += p_extension;
  }
  return true;
}

void* 
WinFile::OpenAsSharedMemory(XString  p_name
                           ,bool     p_local     /*= true*/
                           ,bool     p_trycreate /*= false*/
                           ,size_t   p_size      /*= 0*/)
{
  // Reset the error
  m_error = 0;

  // Check if still open
  if(m_file)
  {
    m_error = ERROR_INVALID_FUNCTION;
    return nullptr;
  }

  // Check that name does NOT have a '\' in it
  if(p_name.Find('\\') >= 0)
  {
    m_error = ERROR_INVALID_FUNCTION;
    return nullptr;
  }

  // Extend the name for RDP sessions
  p_name = (p_local ? _T("Local\\") : _T("Global\\")) + p_name;

  // Acquire a file access lock
  AutoCritSec lock(&m_fileaccess);

  // DO either a create of, or an open of an existing memory segment
  if(p_trycreate)
  {
    // Try to open in read/write mode
    if(!Open(winfile_write))
    {
      // If fails: m_error already set
      return nullptr;
    }
    // Check handle to be sure!!
    if(m_file == nullptr || m_file == INVALID_HANDLE_VALUE)
    {
      return nullptr;
    }

    // File is now opened: try to create a file mapping
    DWORD protect  = PAGE_READWRITE;
    DWORD sizeLow  = p_size & 0x0FFFFFFF;
    DWORD sizeHigh = ((unsigned __int64) p_size) >> 32;
    m_file = CreateFileMapping(m_file,nullptr,protect,sizeLow,sizeHigh,p_name.GetString());
    if(m_file == NULL)
    {
      m_error = ::GetLastError();
      return m_file;
    }
  }
  else
  {
    // Try to open for read-write access. Child processes can NOT inherit the handle
    m_file = OpenFileMapping(FILE_MAP_ALL_ACCESS,FALSE,p_name.GetString());
    if(m_file == NULL)
    {
      m_error = ::GetLastError();
      return m_file;
    }
  }
  m_sharedMemory = MapViewOfFile(m_file,FILE_MAP_ALL_ACCESS,0,0,(SIZE_T)p_size);
  if(m_sharedMemory)
  {
    // Success: we have a segment of this size
    m_sharedSize = p_size;
  }
  else
  {
    m_error = ::GetLastError();
    CloseHandle(m_file);
    m_file = NULL;
  }
  return m_sharedMemory;
}

// Grant full access on file or directory
bool
WinFile::GrantFullAccess()
{
  // Check if we have a filename to work on
  if(m_filename.IsEmpty())
  {
    return false;
  }

  bool result      = false;
  PSID SIDEveryone = nullptr;
  PACL acl         = nullptr;
  SID_IDENTIFIER_AUTHORITY SIDAuthWorld = SECURITY_WORLD_SID_AUTHORITY;
  EXPLICIT_ACCESS ea;

  __try 
  {
    // Create a SID for the Everyone group.
    if(AllocateAndInitializeSid(&SIDAuthWorld,1,SECURITY_WORLD_RID
                               ,0,0,0,0,0,0,0
                               ,&SIDEveryone) == 0)
    {
      // AllocateAndInitializeSid (Everyone) failed
      __leave;
    }
    // Set full access for Everyone.
    ZeroMemory(&ea,sizeof(EXPLICIT_ACCESS));
    ea.grfAccessPermissions = MAXIMUM_ALLOWED;
    ea.grfAccessMode        = SET_ACCESS;
    ea.grfInheritance       = SUB_CONTAINERS_AND_OBJECTS_INHERIT;
    ea.Trustee.TrusteeForm  = TRUSTEE_IS_SID;
    ea.Trustee.TrusteeType  = TRUSTEE_IS_WELL_KNOWN_GROUP;
    ea.Trustee.ptstrName    = (LPTSTR)SIDEveryone;

    if(ERROR_SUCCESS != SetEntriesInAcl(1,&ea,nullptr,&acl))
    {
      // SetEntriesInAcl failed
      __leave;
    }
    // Try to modify the object's DACL.
    if(ERROR_SUCCESS != SetNamedSecurityInfo((LPTSTR)m_filename.GetString() // name of the object
                                             ,SE_FILE_OBJECT                // type of object: file or directory
                                             ,DACL_SECURITY_INFORMATION     // change only the object's DACL
                                             ,nullptr,nullptr               // do not change owner or group
                                             ,acl                           // DACL specified
                                             ,nullptr))                     // do not change SACL
    {
      // SetNamedSecurityInfo failed to change the DACL Maximum Allowed Access
      __leave;
    }
    // Full access granted!
    result = true;
  }
  __finally 
  {
    if(SIDEveryone)
    {
      FreeSid(SIDEveryone);
    }
    if(acl)
    {
      LocalFree(acl);
    }
  }
  return result;
}

// BEWARE! Only use after a copy constructor or an assignment
void
WinFile::ForgetFile()
{
  // Acquire a file access lock
  AutoCritSec lock(&m_fileaccess);

  m_filename.Empty();
  m_file         = NULL;
  m_openMode     = FFlag::no_mode;
  m_sharedMemory = 0L;
  m_sharedSize   = 0L;
  m_error        = 0;
  m_ungetch      = 0;

  PageBufferFree();
}

//////////////////////////////////////////////////////////////////////////
//
// CONTENT OPERATIONS
//
//////////////////////////////////////////////////////////////////////////

// Read a string (possibly with CR/LF to newline translation)
bool
WinFile::Read(XString& p_string,uchar p_delim /*= '\n'*/)
{
  std::string result;
  bool unicodeSkip(false);
  int  last(0);

  // Reset the error
  m_error = 0;

  // See if file is open
  if(m_file == nullptr)
  {
    m_error = ERROR_FILE_NOT_FOUND;
    return false;
  }

  // Do NOT do this in binary mode: no backing page buffer!
  if(m_openMode & FFlag::open_trans_binary)
  {
    m_error = ERROR_INVALID_FUNCTION;
    return false;
  }

  bool crstate = false;

  while(true)
  {
    int ch(PageBufferRead());
    if(ch == EOF || ch == 0)
    {
      m_error = ::GetLastError();
      p_string = TranslateInputBuffer(result);
      return p_string.GetLength() ? true : false;
    }
    result += (uchar)ch;

    // Do the CR/LF to "\n" translation
    if(m_openMode & FFlag::open_trans_text)
    {
      if(ch == p_delim || ch == '\r')
      {
        unicodeSkip = (m_encoding == Encoding::LE_UTF16) ||
                      (m_encoding == Encoding::BE_UTF16);
      }
      if(ch == '\r')
      {
        int ext;
        crstate = true;
        switch(m_encoding)
        {
          case Encoding::LE_UTF16:  ext = PageBufferRead();
                                    result += (uchar) ext;
                                    if(ext)
                                    {
                                      crstate = false;
                                    }
                                    break;
          case Encoding::BE_UTF16:  if(result[result.size() - 2])
                                    {
                                      crstate = false;
                                    }
                                    break;
          case Encoding::UTF8:      last = 0; 
                                    [[fallthrough]];
          default:                  break;

        }
        continue;
      }
    }
    if(ch == p_delim && m_encoding == Encoding::LE_UTF16)
    {
      // Read in trailing zero for a newline in this encoding
      result += (uchar)(last = PageBufferRead());
    }
    if(crstate && ch == p_delim && !last)
    {
      if(unicodeSkip)
      {
        result[result.size() - 4] = result[result.size() - 2];
        result[result.size() - 3] = result[result.size() - 1];
      }
      else
      {
        result[result.size() - 2] = (uchar)ch;
      }
      result.erase(result.size() - 1 - (unicodeSkip ? 1 : 0));
      crstate = false;
    }

    // See if we are ready reading the string
    if(ch == p_delim && !last)
    {
      break;
    }
    if(unicodeSkip)
    {
      last = ch;
    }
  }
  p_string = TranslateInputBuffer(result);
  return true;
}

XString 
WinFile::TranslateInputBuffer(std::string& p_string)
{
  if(p_string.empty())
  {
    return _T("");
  }
#ifdef _UNICODE
  if(m_encoding == Encoding::UTF8)
  {
    return ExplodeString(p_string,CODEPAGE_UTF8);
  }
  else if(m_encoding == Encoding::BE_UTF16)
  {
    BlefuscuToLilliput(p_string);
  }
  if(m_encoding == Encoding::LE_UTF16||
     m_encoding == Encoding::BE_UTF16)
  {
    // We are already UTF-16
    XString result;
    int size = (int) p_string.size();
    LPTSTR resbuf = result.GetBufferSetLength(size + 1);
    memcpy(resbuf,p_string.c_str(),size);
    size /= sizeof(TCHAR);
    resbuf[size] = (TCHAR) 0;
    result.ReleaseBufferSetLength(size);
    return result;
  }
  // Last resort, create XString for current codepage
  // Mostly MS-Windows 1252 in Western Europe
  return ExplodeString(p_string,GetACP());
#else
  if(m_encoding == Encoding::UTF8)
  {
    // Convert UTF-8 -> UTF-16 -> MBCS
    int   clength = 0;
    // Getting the needed buffer length (in code points)
    clength = MultiByteToWideChar(CODEPAGE_UTF8,0,p_string.c_str(),-1,NULL,NULL);
    uchar* buffer = new uchar[clength * 2];
    // Doing the 'real' conversion
    clength = MultiByteToWideChar(CODEPAGE_UTF8,0,p_string.c_str(),-1,reinterpret_cast<LPWSTR>(buffer),clength);

    // Getting the needed length for MBCS
    clength = ::WideCharToMultiByte(GetACP(),0,(LPCWSTR) buffer,-1,NULL,NULL,NULL,NULL);
    XString result;
    LPTSTR strbuf = result.GetBufferSetLength(clength);
    // Doing the conversion to MBCS
    clength = ::WideCharToMultiByte(GetACP(),0,(LPCWSTR) buffer,-1,reinterpret_cast<LPSTR>(strbuf),clength,NULL,NULL);
    result.ReleaseBuffer();
    delete[] buffer;
    return result;
  }
  else if(m_encoding == Encoding::LE_UTF16 ||
          m_encoding == Encoding::BE_UTF16 )
  {
    if(m_encoding == Encoding::BE_UTF16)
    {
      BlefuscuToLilliput(p_string);
    }
    // Implode to MBCS
    XString result;
    int clength = 0;
    int blength = 0;
    // Zero delimit the input string for sure!
    p_string += '0';
    p_string += '0';

    // Size in UTF16 character code points
    clength = (int)p_string.size() / 2;

    // Getting the needed length for MBCS
    blength = ::WideCharToMultiByte(GetACP(),WC_COMPOSITECHECK,(LPCWSTR)p_string.c_str(),clength,NULL,NULL,NULL,NULL);
    char* buffer = new char[blength + 1];
    // Doing the conversion from UTF-16 to MBCS
    blength = WideCharToMultiByte(GetACP(),WC_COMPOSITECHECK,(LPCWSTR)p_string.c_str(),clength,buffer,blength,NULL,NULL);
    buffer[blength - 1] = 0;
    result = buffer;
    delete[] buffer;
    return result;
  }
  else if(m_encoding == Encoding::EN_ACP || m_encoding == (Encoding)GetACP())
  {
    // Simply the string in the current encoding
    return XString(p_string.c_str());
  }
  else
  {
    // We got some strange encoding which we try to convert to MBCS
    XString string(p_string.c_str());
    return DecodeStringFromTheWire(string,CodepageToCharset((int)m_encoding));
  }
#endif
}

// On input:  Convert Big-Endian (Blefuscu) to Little-Endian (Lilliput)
// On output: Convert Little-Endian (Lilliput) to Big-Endian (Blefuscu)
// MS-Windows = Intel architecture = Little-Endian (LE)
// Mac-OS     = Motorola architecture = Big-Endian (BE)
void
WinFile::BlefuscuToLilliput(std::string& p_gulliver)
{
  uchar ch1,ch2;
  for(size_t index = 0;index < p_gulliver.size();index += 2)
  {
    ch1 = p_gulliver[index];
    ch2 = p_gulliver[index + 1];
    p_gulliver[index    ] = ch2;
    p_gulliver[index + 1] = ch1;
  }
}

// Read a buffer directly from a binary file
bool
WinFile::Read(void* p_buffer,size_t p_bufsize,int& p_read)
{
  // Reset error and size
  m_error = 0;
  p_read  = 0;

  // See if file is open
  if(m_file == nullptr)
  {
    m_error = ERROR_FILE_NOT_FOUND;
    return false;
  }

  // Only in binary mode! Do not use the backing page buffer
  if(!(m_openMode & FFlag::open_trans_binary) || m_pageBuffer)
  {
    m_error = ERROR_INVALID_FUNCTION;
    return false;
  }

  // Acquire a file access lock
  AutoCritSec lock(&m_fileaccess);

  DWORD read = 0;
  if(::ReadFile(m_file,p_buffer,(DWORD)p_bufsize,&read,nullptr) == 0)
  {
    m_error = ::GetLastError();
    return false;
  }
  p_read = read;
  return true;
}

// Writing a string to the WinFile
bool
WinFile::Write(const XString& p_string)
{
  // Reset the error
  m_error = 0;

  // See if file is open
  if(m_file == nullptr)
  {
    m_error = ERROR_FILE_NOT_FOUND;
    return false;
  }

  // Do NOT do this in binary mode: no backing pagebuffer!
  if(m_openMode & FFlag::open_trans_binary)
  {
    m_error = ERROR_INVALID_FUNCTION;
    return false;
  }

  // Convert to text standard string (always 8 bits!!)
  bool doText = (m_openMode & FFlag::open_trans_text) > 0;
  std::string string = TranslateOutputBuffer(p_string);

  for(size_t index = 0;index < string.size();++index)
  {
    uchar ch = string[index];
    if(doText && ch == '\n')
    {
      if(!(m_encoding == Encoding::LE_UTF16 && string[index + 1]) &&
         !(m_encoding == Encoding::BE_UTF16 && string[index - 1]))
      {

        if(!PageBufferWrite('\r'))
        {
          break;
        }
        if(m_encoding == Encoding::BE_UTF16 ||
           m_encoding == Encoding::LE_UTF16)
        {
          if(!PageBufferWrite(0))
          {
            break;
          }
        }
      }
    }
    if(!PageBufferWrite(ch))
    {
      break;
    }
  }

  return true;
}

std::string 
WinFile::TranslateOutputBuffer(const XString& p_string)
{
  if(p_string.IsEmpty())
  {
    return std::string();
  }
  std::string result;

#ifdef _UNICODE
  if(m_encoding == Encoding::LE_UTF16 ||
     m_encoding == Encoding::BE_UTF16)
  {
    // We are already UTF16
    result.append(p_string.GetLength() * sizeof(TCHAR),(uchar) ' ');
    memcpy((void*) result.c_str(),p_string.GetString(),p_string.GetLength() * sizeof(TCHAR));

    if(m_encoding == Encoding::BE_UTF16)
    {
      BlefuscuToLilliput(result);
    }
  }
  else
  {
    // Simply implode the string to MBCS
    result = ImplodeString(p_string,(int)m_encoding);
  }
#else
  if(m_encoding == Encoding::UTF8)
  {
    int   clength = 0;
    int   blength = 0;

    // Getting the length to convert to UTF-16
    clength = MultiByteToWideChar(GetACP(),0,p_string.GetString(),-1,NULL,NULL);
    blength = clength * 2;
    uchar* buffer = new uchar[blength + 2];
    // Real conversion to UTF-16
    clength = MultiByteToWideChar(GetACP(),0,p_string.GetString(),-1,reinterpret_cast<LPWSTR>(buffer),blength);

    // Getting the length to convert back to UTF-8
    clength = WideCharToMultiByte(CODEPAGE_UTF8,0,(LPCWSTR) buffer,clength,NULL,0,NULL,NULL);
    result.append(clength,' ');
    // Real conversion to UTF-8
    WideCharToMultiByte(CODEPAGE_UTF8,0,(LPCWSTR) buffer,clength,reinterpret_cast<LPSTR>(const_cast<char*>(result.c_str())),clength,NULL,NULL);
    result.erase(clength - 1);
    delete[] buffer;
  }
  else if(m_encoding == Encoding::LE_UTF16 ||
          m_encoding == Encoding::BE_UTF16)
  {
    int length = 0;
    // Getting the needed length
    length = MultiByteToWideChar(GetACP(),0,p_string.GetString(),-1,NULL,NULL);
    length *= 2;
    uchar* buffer = new uchar[length + 1];
    // Doing the 'real' conversion
    length = MultiByteToWideChar(GetACP(),0,p_string.GetString(),-1,reinterpret_cast<LPWSTR>(buffer),length);
    length *= 2;
    result.append(length,' ');
    memcpy((void*)result.c_str(),buffer,length);
    result.erase(length - 2);
    delete[] buffer;

    // Invert Big-Endian string
    if(m_encoding == Encoding::BE_UTF16)
    {
      BlefuscuToLilliput(result);
    }
  }
  else if(m_encoding == Encoding::EN_ACP || m_encoding == (Encoding)GetACP())
  {
    // Simply the string
    result = p_string.GetString();
  }
  else
  {
    // We have some other string in an exotic encoding
    result = DecodeStringFromTheWire(p_string,CodepageToCharset((int)m_encoding));
  }
#endif
  return result;
}

// Writing a buffer block to a binary file
bool
WinFile::Write(void* p_buffer,size_t p_bufsize)
{
  // Reset the error
  m_error = 0;

  // See if file is open
  if (m_file == nullptr)
  {
    m_error = ERROR_FILE_NOT_FOUND;
    return false;
  }

  // See if we are opened in 'write' mode
  if(!(m_openMode & FFlag::open_write))
  {
    m_error = ERROR_INVALID_FUNCTION;
    return false;
  }

  // Acquire a file access lock
  AutoCritSec lock(&m_fileaccess);

  // Flush our page buffer (if any)
  if(m_pageBuffer)
  {
    PageBufferFlush();
  }

  DWORD written = 0;
  if(::WriteFile(m_file,p_buffer,(DWORD)p_bufsize,&written,nullptr) == 0)
  {
    m_error = ::GetLastError();
    return false;
  }
  if(written != p_bufsize)
  {
    m_error = ::GetLastError();
    return false;
  }
  return true;
}

bool
WinFile::Format(LPCTSTR p_format,...)
{
  va_list argList;
  va_start(argList,p_format);
  bool result = FormatV(p_format,argList);
  va_end(argList);
  return result;
}

bool
WinFile::FormatV(LPCTSTR p_format,va_list p_list)
{
  XString buffer;
  buffer.FormatV(p_format,p_list);
  return Write(buffer);
}

// Getting the current file position
// Returns the file position, or -1 for error
size_t
WinFile::Position() const
{
  // Reset the error
  m_error = 0;

  // See if file is open
  if (m_file == nullptr)
  {
    m_error = ERROR_FILE_NOT_FOUND;
    return (size_t) - 1;
  }

  // Read files should readjust the current position
  PageBufferAdjust();

  LARGE_INTEGER move = { 0, 0 };
  LARGE_INTEGER pos  = { 0, 0 };

  // Acquire a file access lock
  AutoCritSec lock(&m_fileaccess);

  if(::SetFilePointerEx(m_file, move, &pos, FILE_CURRENT) == 0)
  {
    m_error = ::GetLastError();
    return (size_t) - 1;
  }
  return (size_t) pos.QuadPart;
}

// Setting a new file position
size_t
WinFile::Position(FSeek p_how,LONGLONG p_position /*= 0*/)
{
  // Reset the error
  m_error = 0;

  // See if file is open
  if (m_file == nullptr)
  {
    m_error = ERROR_FILE_NOT_FOUND;
    return (size_t) - 1;
  }

  // Check if we where opened for random access, 
  // so we can move the file pointer
  if((m_openMode & FFlag::open_random_access) == 0)
  {
    m_error = ERROR_INVALID_FUNCTION;
    return (size_t) - 1;
  }

  if(m_openMode & FFlag::open_write)
  {
    // In case of a file open for write: flush out to the current position
    PageBufferFlush();
  }
  else
  {
    // In case of a file open for read: reset the file position to the buffer pointer
    PageBufferAdjust();
  }

  // Set the position
  LARGE_INTEGER move = { 0, 0 };
  LARGE_INTEGER pos  = { 0, 0 };
  move.QuadPart = p_position;

  // Figure out the method of moving
  DWORD method = 0;
  switch(p_how)
  {
    case FSeek::file_begin:   method = FILE_BEGIN;   break;
    case FSeek::file_current: method = FILE_CURRENT; break;
    case FSeek::file_end:     method = FILE_END;     break;
    default:                  m_error = ERROR_INVALID_FUNCTION;
                              return (size_t) - 1;
  }

  // Acquire a file access lock
  AutoCritSec lock(&m_fileaccess);

  // Perform the file move
  if(::SetFilePointerEx(m_file,move,&pos,method) == 0)
  {
    m_error = ::GetLastError();
    return (size_t) - 1;
  }
  PageBufferFree();

  // Return the new resulting file position
  // In case of current/end can be different from p_position!
  return (size_t) pos.QuadPart;
}

bool
WinFile::Flush(bool p_all /*= false*/)
{
  // Reset the error
  m_error = 0;

  // See if file is open
  if (m_file == nullptr)
  {
    m_error = ERROR_FILE_NOT_FOUND;
    return false;
  }

  // In case of a translate_text file: flush our buffers
  PageBufferFlush();

  // Acquire a file access lock
  AutoCritSec lock(&m_fileaccess);

  // Do the flush
  if (::FlushFileBuffers(m_file) == 0)
  {
    m_error = ::GetLastError();
    return false;
  }

  // Flush all buffers. No error return value!
  if(p_all)
  {
    _flushall();
  }
  return true;
}

// Getting a string much like "fgets(buffer*,size,FILE*)"
bool
WinFile::Gets(uchar* p_buffer,size_t p_size)
{
  // Reset the error state
  m_error = 0;

  // Do NOT do this in binary mode: no backing pagebuffer!
  if(m_openMode & FFlag::open_trans_binary)
  {
    m_error = ERROR_INVALID_FUNCTION;
    return false;
  }

  // make sure we will get something
  if(p_size == 0)
  {
    return false;
  }

  // Make space for the closing zero
  --p_size;

  // Make sure we have a read buffer
  *p_buffer++ = (uchar) Getch();

  // Scan forward in the pagebuffer for a newline
  char* ending = strchr((char*)m_pagePointer,'\n');
  if(ending && ((size_t)ending < (size_t)m_pageTop))
  {
    // Found a newline before the end of the buffer
    // So do an optimized buffer copy of the whole string
    size_t amount = (size_t)ending - (size_t)m_pagePointer + 1;
    if(amount <= p_size)
    {
      strncpy_s((char*)p_buffer,p_size,(char*)m_pagePointer,amount);
      // Take care of the text mode translation
      if((m_openMode & FFlag::open_trans_text) && (amount >= 2) && (p_buffer[amount - 2] == '\r'))
      {
        p_buffer[amount - 2] = '\n';
        p_buffer[amount - 1] = 0;
      }
      m_pagePointer += amount;
      p_buffer      += amount;
      *p_buffer      = 0;
      return true;
    }
  }
  // Conservative read each character
  // Allowing to read in a next buffer from the filesystem
  for(size_t index = 0; index < p_size; ++index)
  {
    int ch = Getch();
    if (ch == EOF)
    {
      if(index > 0)
      {
        break;
      }
      return false;
    }
    *p_buffer++ = (uchar)ch;
    if(ch == '\n')
    {
      break;
    }
  }
  *p_buffer = 0;
  return true;
}

bool
WinFile::Puts(uchar* p_buffer)
{
  // Reset the error state
  m_error = 0;

  // Do NOT do this in binary mode: no backing pagebuffer!
  if (m_openMode & FFlag::open_trans_binary)
  {
    m_error = ERROR_INVALID_FUNCTION;
    return false;
  }

  while(*p_buffer)
  {
    uchar ch = *p_buffer++;
    if(ch == '\n' && (m_openMode & FFlag::open_trans_text))
    {
      if(PageBufferWrite('\r') == false)
      {
        return false;
      }
    }
    if(PageBufferWrite(ch) == false)
    {
      return false;
    }
  }
  return true;
}

// Puts a single character into the file stream
bool
WinFile::Putch(uchar p_char)
{
  // Reset the error state
  m_error = 0;

  // Do NOT do this in binary mode: no backing pagebuffer!
  if(m_openMode & FFlag::open_trans_binary)
  {
    m_error = ERROR_INVALID_FUNCTION;
    return false;
  }
  // If we **DID** have an unget character, forget about it!
  m_ungetch = 0;

  // Writing a single character
  return PageBufferWrite(p_char);
}

// Get the next character/byte from the file stream
// or returns EOF at the end of the file
int
WinFile::Getch()
{
  // Reset the error state
  m_error = 0;

  // Do NOT do this in binary mode: no backing pagebuffer!
  if (m_openMode & FFlag::open_trans_binary)
  {
    m_error = ERROR_INVALID_FUNCTION;
    return false;
  }

  // If we **DID** have an unget character, return that one
  if(m_ungetch)
  {
     uchar ch(m_ungetch);
     m_ungetch = 0;
     return ch;
  }
  int ch = PageBufferRead();
  if(ch == '\r')
  {
    if(m_openMode & FFlag::open_trans_text)
    {
      ch = PageBufferRead();
    }
  }
  return ch;
}

// Push back one character in the file stream
bool
WinFile::Ungetch(uchar p_ch)
{
  // Reset the error state
  m_error = 0;

  if(m_ungetch)
  {
    m_error = ERROR_INVALID_FUNCTION;
    return false;
  }
  m_ungetch = p_ch;
  return true;
}

bool
WinFile::Lock(size_t p_begin,size_t p_length)
{
  // Reset the error
  m_error = 0;

  // See if file is open
  if(m_file == nullptr || ((m_openMode & FFlag::open_lockmode) == 0))
  {
    m_error = ERROR_FILE_NOT_FOUND;
    return false;
  }

#if defined _M_IX86
  DWORD beginLow   = p_begin;
  DWORD beginHigh  = 0;
  DWORD lengthLow  = p_length;
  DWORD lengthHigh = 0;
#else 
  DWORD beginLow   = p_begin   & 0x0FFFFFFFF;
  DWORD beginHigh  = p_begin  >> 32;
  DWORD lengthLow  = p_length  & 0x0FFFFFFFF;
  DWORD lengthHigh = p_length >> 32;
#endif

  // Acquire a file access lock
  AutoCritSec lock(&m_fileaccess);

  if(::LockFile(m_file,beginLow,beginHigh,lengthLow,lengthHigh) == 0)
  {
    m_error = ::GetLastError();
    return false;
  }
  return true;
}

bool
WinFile::UnLock(size_t p_begin,size_t p_length)
{
  // Reset the error
  m_error = 0;

  // See if file is open
  if(m_file == nullptr || ((m_openMode & FFlag::open_lockmode) == 0))
  {
    m_error = ERROR_FILE_NOT_FOUND;
    return false;
  }

#if defined _M_IX86
  DWORD beginLow   = p_begin;
  DWORD beginHigh  = 0;
  DWORD lengthLow  = p_length;
  DWORD lengthHigh = 0;
#else 
  DWORD beginLow   = p_begin   & 0x0FFFFFFFF;
  DWORD beginHigh  = p_begin  >> 32;
  DWORD lengthLow  = p_length  & 0x0FFFFFFFF;
  DWORD lengthHigh = p_length >> 32;
#endif

  // Acquire a file access lock
  AutoCritSec lock(&m_fileaccess);

  if(::UnlockFile(m_file,beginLow,beginHigh,lengthLow,lengthHigh) == 0)
  {
    m_error = ::GetLastError();
    return false;
  }
  return true;
}

bool
WinFile::UnLockFile()
{
  // Reset error
  m_error = 0;

  // See if file is open
  if(m_file == nullptr)
  {
    m_error = ERROR_FILE_NOT_FOUND;
    return false;
  }

  size_t size = GetFileSize();
  if(size != INVALID_FILE_SIZE)
  {
    return UnLock(0, size);
  }
  // m_error code already filled
  return false;
}

// Writing a Byte-Order-Mark for the file
bool
WinFile::WriteEncodingBOM()
{
  switch(m_encoding)
  {
    case Encoding::UTF8:     Putch(0xEF);
                             Putch(0xBB);
                             Putch(0xBF);
                             break;
    case Encoding::LE_UTF16: Putch(0xFF);
                             Putch(0xFE);
                             break;
    case Encoding::BE_UTF16: Putch(0xFE);
                             Putch(0xFF);
                             break;
    case Encoding::BE_UTF32: Putch(0);
                             Putch(0);
                             Putch(0xFE);
                             Putch(0xFF);
                             break;
    case Encoding::LE_UTF32: Putch(0xFF);
                             Putch(0xFE);
                             Putch(0);
                             Putch(0);
                             break;
    // Incompatible encodings on MS-Windows
    // Or code pages that does not define a BOM                                
    default:                 return false;
  }
  return true;
}

// Check for a Byte-Order-Mark (BOM)
// Returns the found type of BOM
// and the number of bytes to skip in your input
//
// Type of BOM found to defuse
// As found on: https://en.wikipedia.org/wiki/Byte_order_mark
//
BOMOpenResult 
WinFile::DefuseBOM(const uchar*  p_pointer
                  ,Encoding&     p_type
                  ,unsigned int& p_skip)
{
  // Preset nothing
  p_type = (Encoding) -1;
  p_skip = 0;

  // Get first four characters in the message as integers
  int c1 = * p_pointer;
  int c2 = *(p_pointer + 1);
  int c3 = *(p_pointer + 2);
  int c4 = *(p_pointer + 3);

  // Check if Big-Endian UTF-8 BOM
  if(c1 == 0xEF && c2 == 0xBB && c3 == 0xBF)
  {
    // Yes BE-BOM in UTF-8
    p_skip = 3;
    p_type = Encoding::UTF8;
    return BOMOpenResult::BOM;
  }
  // Check Little-Endian UTF-16/UTF32
  if(c1 == 0xFF && c2 == 0xFE)
  {
    if(c3 == 0x0 && c4 == 0x0)
    {
      p_skip = 4;
      p_type = Encoding::LE_UTF32;
      return BOMOpenResult::BOM;
    }
    p_skip = 2;
    p_type = Encoding::LE_UTF16;
    return BOMOpenResult::BOM;
  }
  // Check Big-Endian UTF-16
  if(c1 == 0xFE && c2 == 0xFF)
  {
    p_skip = 2;
    p_type = Encoding::BE_UTF16;
    return BOMOpenResult::BOM;
  }
  // Check Big-Endian UTF-32
  if(c1 == 0 && c2 == 0 && c3 == 0xFE && c4 == 0xFF)
  {
    p_skip = 4;
    p_type = Encoding::BE_UTF32;
    return BOMOpenResult::Incompatible;
  }
  // After BE32 check. The buffer is most likely empty
  if(c1 == 0)
  {
    return BOMOpenResult::NoString;
  }
  // Check for UTF-7 special case
  if(c1 == 0x2B && c2 == 0x2F && c3 == 0x76)
  {
    if(c4 == 0x38 || c4 == 39 || c4 == 0x2B || c4 == 0x2F)
    {
      // Application still has to process lower 2 bits 
      // of the 4th character. Re-spool to that char.
      p_skip = 3;
      p_type = Encoding::UTF7;
      return BOMOpenResult::Incompatible;
    }
  }
  // Check GB-18030
  if(c1 == 0x84 && c2 == 0x31 && c3 == 0x95 && c4 == 0x33)
  {
    p_skip = 4;
    p_type = Encoding::GB_18030;
    return BOMOpenResult::Incompatible;
  }
  // Check for UTF-1 special case
  if(c1 == 0xF7 && c2 == 0x64 && c3 == 0x4C)
  {
    p_skip = 3;
    return BOMOpenResult::Incompatible;
  }
  // Check for UTF-EBCDIC IBM set
  if(c1 == 0xDD && c2 == 0x73 && c3 == 0x66 && c4 == 0x73)
  {
    p_skip = 4;
    return BOMOpenResult::Incompatible;
  }
  // Check for CSCU 
  if(c1 == 0x0E && c2 == 0xFE && c3 == 0xFF)
  {
    p_skip = 3;
    return BOMOpenResult::Incompatible;
  }
  // Check for BOCU-1
  if(c1 == 0xFB && c2 == 0xEE && c3 == 0x28)
  {
    p_skip = 3;
    return BOMOpenResult::Incompatible;
  }
  // NOT A BOM !!
  return BOMOpenResult::NoEncoding;
}

//////////////////////////////////////////////////////////////////////////
//
// SETTERS
//
//////////////////////////////////////////////////////////////////////////

// Set the filename, but only if the file was not (yet) opened
bool
WinFile::SetFilename(XString p_filename)
{
  if(m_file == nullptr)
  {
    m_filename = p_filename;
    return true;
  }
  return false;
}

// Get a filename in a 'special' Windows folder
// p_folder parameter is one of the many CSIDL_* folder names
bool
WinFile::SetFilenameInFolder(int p_folder,XString p_filename)
{
  // Check if file was already opened
  if(m_file)
  {
    return false;
  }

  // Make sure we only get a folder (and not creating one)
  p_folder = p_folder & 0xFF;

  // Local variables
  IMalloc*      pShellMalloc = nullptr;   // A pointer to the shell's IMalloc interface
  IShellFolder* psfParent    = nullptr;   // A pointer to the parent folder object's IShellFolder interface.
  LPITEMIDLIST  pidlItem     = nullptr;   // The item's PIDL.
  LPITEMIDLIST  pidlRelative = nullptr;   // The item's PIDL relative to the parent folder.
  TCHAR         special[MAX_PATH];        // The path for Favorites.
  STRRET        strings;                  // The structure for strings returned from IShellFolder.
  bool          result = false;           // Result of the directory search

  special[0] = 0;
  HRESULT hres = SHGetMalloc(&pShellMalloc);
  if (FAILED(hres))
  {
    return result;
  }
  hres = SHGetSpecialFolderLocation(NULL,p_folder,&pidlItem);
  if (SUCCEEDED(hres))
  {
    hres = SHBindToParent(pidlItem,IID_IShellFolder,(void**)&psfParent,(LPCITEMIDLIST*)&pidlRelative);
    if (SUCCEEDED(hres))
    {
      // Retrieve the path
      memset(&strings, 0, sizeof(strings));
      hres = psfParent->GetDisplayNameOf(pidlRelative,SHGDN_NORMAL | SHGDN_FORPARSING,&strings);
      if (SUCCEEDED(hres))
      {
        if(StrRetToBuf(&strings,pidlItem,special,ARRAYSIZE(special)) != S_OK)
        {
          special[0] = 0;
          m_error = ERROR_FILE_NOT_FOUND;
        }
        else
        {
          result = true;
        }
      }
      else
      {
        m_error = ERROR_FILE_NOT_FOUND;
      }
      psfParent->Release();
    }
    // Clean up allocated memory
    if (pidlItem)
    {
      pShellMalloc->Free(pidlItem);
    }
  }
  pShellMalloc->Release();

  m_filename = special + XString(_T("\\")) + p_filename;
  return result;
}

bool
WinFile::SetFilenameFromResource(XString p_resource)
{
  m_filename = FileNameFromResourceName(p_resource);
  return !m_filename.IsEmpty();
}


// Set the file handle, but only if the file was not (yet) opened
bool
WinFile::SetFileHandle(HANDLE p_handle)
{
  // Acquire a file access lock
  AutoCritSec lock(&m_fileaccess);

  if(m_file == nullptr)
  {
    m_file = p_handle;
    return true;
  }
  return false;
}

bool
WinFile::SetFileAttribute(FAttributes p_attribute, bool p_set)
{
  // Reset the error code
  m_error = 0;

  if(m_file)
  {
    // Acquire a file access lock
    AutoCritSec lock(&m_fileaccess);

    BY_HANDLE_FILE_INFORMATION info;
    if (::GetFileInformationByHandle(m_file,&info))
    {
      if (p_set)
      {
        info.dwFileAttributes |= p_attribute;
      }
      else
      {
        info.dwFileAttributes &= ~p_attribute;
      }
      if(::SetFileInformationByHandle(m_file,FileBasicInfo,&info,sizeof(info)))
      {
        return true;
      }
    }
    m_error = ::GetLastError();
  }
  else if (!m_filename.IsEmpty())
  {
    DWORD attrib = ::GetFileAttributes(m_filename.GetString());
    if (attrib != INVALID_FILE_ATTRIBUTES)
    {
      if(p_set)
      {
        attrib |= p_attribute;
      }
      else
      {
        attrib &= ~p_attribute;
      }
      if(::SetFileAttributes(m_filename.GetString(), attrib))
      {
        return true;
      }
    }
    m_error = ::GetLastError();
  }
  else
  {
    // Nothing here: so no attributes to set
    m_error = ERROR_INVALID_FUNCTION;
  }
  return false;
}

bool
WinFile::SetHidden(bool p_hidden)
{
  return SetFileAttribute(FAttributes::attrib_hidden,p_hidden);
}

bool
WinFile::SetArchive(bool p_archive)
{
  return SetFileAttribute(FAttributes::attrib_archive,p_archive);
}

bool
WinFile::SetSystem(bool p_system)
{
  return SetFileAttribute(FAttributes::attrib_system,p_system);
}

bool
WinFile::SetNormal(bool p_normal)
{
  return SetFileAttribute(FAttributes::attrib_normal,p_normal);
}

bool
WinFile::SetReadOnly(bool p_readonly)
{
  return SetFileAttribute(FAttributes::attrib_readonly,p_readonly);
}

bool
WinFile::SetFileTimeCreated(FILETIME p_created)
{
  // Reset the error code
  m_error = 0;

  // See if file is open
  if(m_file)
  {
    // Acquire a file access lock
    AutoCritSec lock(&m_fileaccess);

    if(::SetFileTime(m_file,&p_created,nullptr,nullptr))
    {
      return true;
    }
    m_error = ::GetLastError();
  }
  else
  {
    m_error = ERROR_FILE_NOT_FOUND;
  }
  return false;
}

bool
WinFile::SetFileTimeModified(FILETIME p_modified)
{
  // Reset the error code
  m_error = 0;

  // See if file is open
  if(m_file)
  {
    // Acquire a file access lock
    AutoCritSec lock(&m_fileaccess);

    if(::SetFileTime(m_file,nullptr,nullptr,&p_modified))
    {
      return true;
    }
    m_error = ::GetLastError();
  }
  else
  {
    m_error = ERROR_FILE_NOT_FOUND;
  }
  return false;
}

bool
WinFile::SetFileTimeAccessed(FILETIME p_accessed)
{
  // Reset the error code
  m_error = 0;

  // See if file is open
  if(m_file)
  {
    // Acquire a file access lock
    AutoCritSec lock(&m_fileaccess);

    if(::SetFileTime(m_file,nullptr,&p_accessed,nullptr))
    {
      return true;
    }
    m_error = ::GetLastError();
  }
  else
  {
    m_error = ERROR_FILE_NOT_FOUND;
  }
  return false;
}

bool
WinFile::SetFileTimeCreated(SYSTEMTIME& p_created)
{
  FILETIME time;
  if(::SystemTimeToFileTime(&p_created,&time))
  {
    return SetFileTimeAccessed(time);
  }
  return false;
}

bool
WinFile::SetFileTimeModified(SYSTEMTIME& p_modified)
{
  FILETIME time;
  if(::SystemTimeToFileTime(&p_modified,&time))
  {
    return SetFileTimeModified(time);
  }
  return false;
}

bool
WinFile::SetFileTimeAccessed(SYSTEMTIME& p_accessed)
{
  FILETIME time;
  if(::SystemTimeToFileTime(&p_accessed,&time))
  {
    return SetFileTimeAccessed(time);
  }
  return false;
}

void
WinFile::SetEncoding(Encoding p_encoding)
{
  m_encoding = p_encoding;
}

// NOT thread safe: Must be set for the total process!
// And can only be set in multiples of 4K up to 1024K
bool
WinFile::SetBufferPageSize(DWORD p_pageSize)
{
  DWORD size = p_pageSize % (4 * 1024);
  if (size > 0 && size <= (1024 * 1024))
  {
    PAGESIZE = size;
    return true;
  }
  return false;
}

// Convert a FILETIME to a time_t value
time_t
WinFile::ConvertFileTimeToTimet(FILETIME p_time)
{
  LONGLONG time = ((LONGLONG)p_time.dwHighDateTime << 32);
  time += p_time.dwLowDateTime;
  time -= 116444736000000000L;
  time /= 10000000L;

  return time;
}

// Convert a time_t value to a FILETIME
FILETIME
WinFile::ConvertTimetToFileTime(time_t p_time)
{
  FILETIME time;
  LONGLONG ll = Int32x32To64(p_time,10000000) + 116444736000000000L;
  time.dwLowDateTime = (DWORD) ll;
  time.dwHighDateTime = ll >> 32;

  return time;
}

//////////////////////////////////////////////////////////////////////////
//
// GETTERS
//
//////////////////////////////////////////////////////////////////////////

// Are we open?
bool
WinFile::GetIsOpen() const
{
  return m_file != nullptr;
}

// Getting the name of the file
XString
WinFile::GetFilename()
{
  return m_filename;
}

// Getting the OS file handle
HANDLE
WinFile::GetFileHandle()
{
  return m_file;
}

// Getting the mode the file was opened in
DWORD
WinFile::GetOpenFlags()
{
  return m_openMode;
}

// Last encountered error code
int
WinFile::GetLastError()
{
  return m_error;
}

XString
WinFile::GetLastErrorString()
{
  // This will be the resulting message
  XString message;

  // This is the "errno" error number
  DWORD dwError = m_error;
  if(dwError == 0)
  {
    dwError = ::GetLastError();
  }
  // Buffer that gets the error message string
  HLOCAL  hlocal = NULL;   
  HMODULE hDll   = NULL;

  // Use the default system locale since we look for Windows messages.
  // Note: this MAKELANGID combination has 0 as value
  DWORD systemLocale = MAKELANGID(LANG_NEUTRAL,SUBLANG_NEUTRAL);

  // Get the error code's textual description
  BOOL fOk = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM     | 
                           FORMAT_MESSAGE_IGNORE_INSERTS  |
                           FORMAT_MESSAGE_ALLOCATE_BUFFER 
                          ,NULL               // DLL
                          ,dwError            // The error code
                          ,systemLocale       // Language
                          ,(PTSTR) &hlocal    // Buffer handle
                          ,0                  // Size if not buffer handle
                          ,NULL);             // Variable arguments
  // Non-generic networking error?
  if(!fOk) 
  {
    // Is it a network-related error?
    hDll = LoadLibraryEx(TEXT("netmsg.dll"),NULL,DONT_RESOLVE_DLL_REFERENCES);

    // Only if NETMSG was found and loaded
    if(hDll != NULL) 
    {
      fOk = FormatMessage(FORMAT_MESSAGE_FROM_HMODULE    | 
                          FORMAT_MESSAGE_IGNORE_INSERTS  |
                          FORMAT_MESSAGE_ALLOCATE_BUFFER 
                         ,hDll
                         ,dwError
                         ,systemLocale
                         ,(PTSTR) &hlocal
                         ,0
                         ,NULL);
    }
  }
  if(fOk && (hlocal != NULL)) 
  {
    // Getting the message from the buffer;
    message = (PCTSTR) LocalLock(hlocal);
    LocalFree(hlocal);
  }
  if(hDll)
  {
    FreeLibrary(hDll);
  }
  // Resulting 
  return message;
}

// Getting the size of the file or INVALID_FILE_SIZE
size_t
WinFile::GetFileSize() const
{
  // Reset the error
  m_error = 0;

  // Try by opened file
  if(m_file)
  {
    // Acquire a file access lock
    AutoCritSec lock(&m_fileaccess);

    LARGE_INTEGER size = { 0, 0 };
    if(::GetFileSizeEx(m_file,&size))
    {
      return (size_t)size.QuadPart;
    }
    m_error = ::GetLastError();
  }
  // Try by filename
  else if(!m_filename.IsEmpty())
  {
    WIN32_FILE_ATTRIBUTE_DATA data;
    memset(&data,0,sizeof(data));
    if(::GetFileAttributesEx(m_filename.GetString(),GetFileExInfoStandard,&data))
    {
      LARGE_INTEGER size = { 0,0 };
      size.LowPart  = data.nFileSizeLow;
      size.HighPart = data.nFileSizeHigh;

      return (size_t) size.QuadPart;
    }
    m_error = ::GetLastError();
  }
  else
  {
    // Nothing to do here
    m_error = ERROR_INVALID_FUNCTION;
  }
  return INVALID_FILE_SIZE;
}

// See if we are positioned at the end of the file
bool
WinFile::GetIsAtEnd() const
{
  // File is not opened, so NO
  if(!m_file)
  {
    return false;
  }

  // In case we are reading a buffered tekstfile
  // and we are not yet at the end of the current page buffer
  if(m_pageBuffer)
  {
    if(m_pagePointer != m_pageTop)
    {
      // Not yet read tot the end of the last buffer
      return false;
    }
  }

  // Getting the 'real' file size
  size_t size = GetFileSize();

  // Acquire a file access lock
  AutoCritSec lock(&m_fileaccess);

  // Getting the file position while reading pages, or reading binaries
  // Zero bytes removed from the current position, gets the position
  LARGE_INTEGER move = {0, 0};
  LARGE_INTEGER pos  = {0, 0};
  if(::SetFilePointerEx(m_file,move,&pos,FILE_CURRENT) == 0)
  {
    m_error = ::GetLastError();
    return true;
  }
  size_t cpos = (size_t) pos.QuadPart;

  // See if something went wrong
  if(size == (size_t)-1 || cpos == (size_t)-1)
  {
    return true;
  }

  // Not at the end, so NO
  if(cpos < size)
  {
    return false;
  }

  // At the end of the file AND at the end of the last buffer
  return true;
}

// If we have a memory segment, return the recorded size
// at the bottom of the memory range
size_t
WinFile::GetSharedMemorySize()
{
  if(m_sharedMemory)
  {
    return m_sharedSize;
  }
  return 0;
}

XString
WinFile::GetNamePercentEncoded()
{
  XString      encoded;
  bool         queryValue = false;
  static TCHAR unsafeString[]   = _T(" \"<>#{}|^~[]`");
  static TCHAR reservedString[] = _T("$&/;?@-!*()'");

  // Re-encode the string. 
  // Watch out: strange code ahead!
  for(int ind = 0;ind < m_filename.GetLength(); ++ind)
  {
    TCHAR ch = m_filename[ind];
    if(ch == '?')
    {
      queryValue = true;
    }
    if(ch == ' ' && queryValue)
    {
      encoded += '+';
    }
    else if(_tcsrchr(unsafeString,ch) ||                    // " \"<>#{}|\\^~[]`"     -> All strings
           (queryValue && _tcsrchr(reservedString,ch)) ||   // "$&+,./:;=?@-!*()'"    -> In query parameter value strings
           (ch < 0x20) ||                                   // 7BITS ASCII Control characters
           (ch > 0x7F) )                                    // Converted UTF-8 characters
    {
      TCHAR extra[3];
      _stprintf_s(extra,3,_T("%%%2.2X"),(int)ch);
      encoded += extra;
    }
    else
    {
      encoded += ch;
    }
  }
  return encoded;
}

XString
WinFile::GetFilenamePartDirectory()
{
  XString drive,directory,filename,extension;
  FilenameParts(m_filename,drive,directory,filename,extension);

  return directory;
}

XString    
WinFile::GetFilenamePartFilename()
{
  XString drive,directory,filename,extension;
  FilenameParts(m_filename,drive,directory,filename,extension);

  return filename + extension;
}

XString
WinFile::GetFilenamePartBasename()
{
  XString drive,directory,filename,extension;
  FilenameParts(m_filename,drive,directory,filename,extension);

  return filename;
}

XString    
WinFile::GetFilenamePartExtension()
{
  XString drive,directory,filename,extension;
  FilenameParts(m_filename,drive,directory,filename,extension);

  return extension;
}

XString 
WinFile::GetFilenamePartDrive()
{
  XString drive,directory,filename,extension;
  FilenameParts(m_filename,drive,directory,filename,extension);

  return drive;
}

FILETIME  
WinFile::GetFileTimeCreated()
{
  // New filetime
  FILETIME creation { 0,0 };
  FILETIME empty    { 0,0 };

  // Reset the error code
  m_error = 0;

  // See if file is open
  if(m_file)
  {
    // Acquire a file access lock
    AutoCritSec lock(&m_fileaccess);

    if(::GetFileTime(m_file,&creation,nullptr,nullptr))
    {
      // Use: FileTimeToSystemTime to break down!
      return creation;
    }
    m_error = ::GetLastError();
  }
  else
  {
    m_error = ERROR_FILE_NOT_FOUND;
  }
  return empty;
}

FILETIME  
WinFile::GetFileTimeModified()
{
  // New filetime
  FILETIME modification { 0, 0 };
  FILETIME empty        { 0, 0 };

  // Reset the error code
  m_error = 0;

  if(m_file)
  {
    // Acquire a file access lock
    AutoCritSec lock(&m_fileaccess);

    if(::GetFileTime(m_file,nullptr,nullptr,&modification))
    {
      // Use: FileTimeToSystemTime to break down!
      return modification;
    }
    m_error = ::GetLastError();
  }
  else
  {
    m_error = ERROR_FILE_NOT_FOUND;
  }
  return empty;

}

FILETIME  
WinFile::GetFileTimeAccessed()
{
  // New filetime
  FILETIME readtime { 0, 0 };
  FILETIME empty    { 0, 0 };

  // Reset the error code
  m_error = 0;

  if(m_file)
  {
    // Acquire a file access lock
    AutoCritSec lock(&m_fileaccess);

    if(::GetFileTime(m_file,nullptr,&readtime,nullptr))
    {
      // Use: FileTimeToSystemTime to break down!
      return readtime;
    }
    m_error = ::GetLastError();
  }
  else
  {
    m_error = ERROR_FILE_NOT_FOUND;
  }
  return empty;
}

bool
WinFile::GetFileTimeCreated(SYSTEMTIME& p_time)
{
  memset(&p_time,0,sizeof(SYSTEMTIME));
  FILETIME time = GetFileTimeCreated();
  if(!m_error)
  {
    ::FileTimeToSystemTime(&time,&p_time);
    return true;
  }
  return false;
}

bool
WinFile::GetFileTimeModified(SYSTEMTIME& p_time)
{
  memset(&p_time,0,sizeof(SYSTEMTIME));
  FILETIME time = GetFileTimeModified();
  if(!m_error)
  {
    ::FileTimeToSystemTime(&time,&p_time);
    return true;
  }
  return false;
}

bool
WinFile::GetFileTimeAccessed(SYSTEMTIME& p_time)
{
  memset(&p_time,0,sizeof(SYSTEMTIME));
  FILETIME time = GetFileTimeAccessed();
  if(!m_error)
  {
    ::FileTimeToSystemTime(&time,&p_time);
    return true;
  }
  return false;
}

// See if file is a opened as a shared memory segment
bool
WinFile::GetIsSharedMemory()
{
  return m_sharedMemory != nullptr;
}

bool      
WinFile::GetFileAttribute(FAttributes p_attribute)
{
  // Reset the error code
  m_error = 0;

  if(m_file)
  {
    // Acquire a file access lock
    AutoCritSec lock(&m_fileaccess);

    BY_HANDLE_FILE_INFORMATION info;
    if(::GetFileInformationByHandle(m_file,&info))
    {
      return (info.dwFileAttributes & p_attribute) > 0;
    }
    m_error = ::GetLastError();
  }
  else if(!m_filename.IsEmpty())
  {
    DWORD attrib = ::GetFileAttributes(m_filename.GetString());
    if(attrib != INVALID_FILE_ATTRIBUTES)
    {
      return (attrib & p_attribute) != 0;
    }
    m_error = ::GetLastError();
  }
  else
  {
    // Nothing here: so no attributes to retrieve
    m_error = ERROR_INVALID_FUNCTION;
  }
  return false;
}

// See if the file is a temporary file
// File will be kept in cache after close for a quick re-open action
bool
WinFile::GetIsTempFile()
{
  return GetFileAttribute(FAttributes::attrib_temporary);
}

// See if this is a hidden file
// Not normally visible in the MS-Windows Explorer
bool
WinFile::GetIsHidden()
{
  return GetFileAttribute(FAttributes::attrib_hidden);
}

// See if the file is marked for archiving in the next archiving run
bool
WinFile::GetIsArchive()
{
  return GetFileAttribute(FAttributes::attrib_archive);
}

// See if this file is marked a a special system file (not to be removed)
bool
WinFile::GetIsSystem()
{
  return GetFileAttribute(FAttributes::attrib_system);
}

bool
WinFile::GetIsNormal()
{
  return GetFileAttribute(FAttributes::attrib_normal);
}

// See if the file is marked as read-only
bool
WinFile::GetIsReadOnly()
{
  return GetFileAttribute(FAttributes::attrib_readonly);
}

// See if this is a directory
bool
WinFile::GetIsDirectory()
{
  return GetFileAttribute(FAttributes::attrib_directory);
}

Encoding
WinFile::GetEncoding()
{
  return m_encoding;
}

bool
WinFile::GetFoundBOM()
{
  return m_foundBOM;
}

bool
WinFile::SetFilenameByDialog(HWND    p_parent      // Parent window (if any)
                            ,bool    p_open        // true = Open/New, false = SaveAs
                            ,XString p_title       // Title of the dialog
                            ,XString p_defext      // Default extension
                            ,XString p_filename    // Default first file
                            ,int     p_flags       // Default flags
                            ,XString p_filter      // Filter for extensions
                            ,XString p_direct)     // Directory to start in
{
  // Reset error
  m_error = 0;
  
  // Open filename structure
  OPENFILENAME ofn;
  TCHAR   original[MAX_PATH + 1];
  TCHAR   filename[MAX_PATH + 1];
  XString filter;

  if(p_filter.IsEmpty())
  {
    filter = _T("All files (*.*)|*.*|");
  }
  else filter = p_filter;

  // Register original CWD (Current Working Directory)
  GetCurrentDirectory(MAX_PATH,original);
  if(!p_direct.IsEmpty())
  {
    // Change to starting directory // VISTA
    SetCurrentDirectory(p_direct.GetString());
  }

  // Check sizes for OPENFILENAME
  if(  filter.GetLength() > 1024) filter   = filter  .Left(1024);
  if(p_defext.GetLength() >  100) p_defext = p_defext.Left(100);
  if( p_title.GetLength() >  100) p_title  = p_title .Left(100);

  // Prepare the filename buffer: size for a new name!
  _tcsncpy_s(filename,MAX_PATH,p_filename.GetString(),MAX_PATH);

  // Prepare the filter
  filter += _T("||");
  for(int index = 0; index < filter.GetLength(); ++index)
  {
    if(filter[index] == _T('|')) filter.SetAt(index,0);
  }

  // Fill in the filename structure
  p_flags |= OFN_ENABLESIZING | OFN_LONGNAMES | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_EXPLORER;
  p_flags &= ~(OFN_NODEREFERENCELINKS | OFN_NOTESTFILECREATE);

  ofn.lStructSize       = sizeof(OPENFILENAME);
  ofn.hwndOwner         = p_parent;
  ofn.hInstance         = (HINSTANCE)GetWindowLongPtr(p_parent,GWLP_HINSTANCE);
  ofn.lpstrFile         = (LPTSTR)filename;
  ofn.lpstrDefExt       = (LPTSTR)p_defext.GetString();
  ofn.lpstrTitle        = (LPTSTR)p_title.GetString();
  ofn.lpstrFilter       = (LPTSTR)filter.GetString();
  ofn.Flags             = p_flags;
  ofn.nFilterIndex      = 1;    // Use lpstrFilter
  ofn.nMaxFile          = MAX_PATH;
  ofn.lpstrCustomFilter = nullptr; //(LPSTR) buf_filter;
  ofn.nMaxCustFilter    = 0;
  ofn.lpstrFileTitle    = nullptr;
  ofn.nMaxFileTitle     = 0;
  ofn.lpstrInitialDir   = (LPCTSTR)p_direct.GetString();
  ofn.nFileOffset       = 0;
  ofn.lCustData         = 0;
  ofn.lpfnHook          = nullptr;
  ofn.lpTemplateName    = nullptr;

  // Get the filename
  int res = IDCANCEL;
  if (p_open)
  {
    res = GetOpenFileName(&ofn);
  }
  else
  {
    res = GetSaveFileName(&ofn);
  }
  // Keep the file
  if(res == IDOK)
  {
    m_filename = filename;
  }
  else
  {
    m_error = ERROR_FILE_NOT_FOUND;
  }
  // Go back to the original directory
  SetCurrentDirectory(original);

  return true;
}

//////////////////////////////////////////////////////////////////////////
//
// OPERATORS
//
//////////////////////////////////////////////////////////////////////////

bool
WinFile::operator==(const WinFile& p_other)
{
  // Must be the same object in the OS!
  if(m_file == p_other.m_file)
  {
    return true;
  }
  // If one or both are opened: cannot be the same
  // even if it is an open handle to the same filesystem file
  // because they can be in different states
  if(m_file != nullptr || p_other.m_file != nullptr)
  {
    return false;
  }
  // If both names empty, than both are NIL and thus equal
  if(m_filename.IsEmpty() && p_other.m_filename.IsEmpty())
  {
    return true;
  }
  // If both names are equal
  // files are equal: non opened, pointing to the same file name
  return m_filename.CompareNoCase(p_other.m_filename) == 0;
}

bool
WinFile::operator!=(const WinFile& p_other)
{
  return !(*this == p_other);
}

// Assignment operator
// BEWARE: making a copy is error prone
// always re-sync your buffer before switching between objects
WinFile& 
WinFile::operator=(const WinFile& p_other)
{
  if(&p_other != this)
  {
    m_file          = p_other.m_file;
    m_filename      = p_other.m_filename;
    m_openMode      = p_other.m_openMode;
    m_error         = p_other.m_error;
    m_ungetch       = p_other.m_ungetch;
    m_sharedMemory  = p_other.m_sharedMemory;
    m_sharedSize    = p_other.m_sharedSize;

    if(p_other.m_pageBuffer)
    {
      PageBuffer();
      m_pagePointer = p_other.m_pagePointer;
      m_pageTop     = p_other.m_pageTop;
      memcpy(m_pageBuffer,p_other.m_pageBuffer,PAGESIZE);
    }
  }
  return *this;
}

XString
WinFile::LegalDirectoryName(XString p_name,bool p_extensionAllowed /*= true*/)
{
  // Check on non-empty
  if(p_name.IsEmpty())
  {
    return _T("NewDirectory");
  }

  XString name(p_name);
  const TCHAR  forbidden_all[] = _T("<>:\"/\\|?*");
  const TCHAR  forbidden_ext[] = _T("<>:\"/\\|?*.");
  const TCHAR* reserved[] =
  {
    _T("CON"), _T("PRN"), _T("AUX"), _T("NUL"),
    _T("COM1"),_T("COM2"),_T("COM3"),_T("COM4"),_T("COM5"),_T("COM6"),_T("COM7"),_T("COM8"),_T("COM9"),
    _T("LPT1"),_T("LPT2"),_T("LPT3"),_T("LPT4"),_T("LPT5"),_T("LPT6"),_T("LPT7"),_T("LPT8"),_T("LPT9")
  };

  // Use either one of these
  const TCHAR* forbidden = p_extensionAllowed ? forbidden_all : forbidden_ext;

  // Replace forbidden characters by an '_'
  for(int index = 0;index < name.GetLength();++index)
  {
    // Replace interpunction
    if(_tcsrchr(forbidden,(TCHAR)name.GetAt(index)) != nullptr)
    {
      name.SetAt(index,'_');
    }
    // Replace non-printable characters
    if(name.GetAt(index) < ' ')
    {
      name.SetAt(index,'=');
    }
  }

  // Construct uppercase name
  XString upper(name);
  upper.MakeUpper();

  // Scan for reserved names
  for(unsigned index = 0;index < (sizeof(reserved) / sizeof(const char*)); ++index)
  {
    // Direct transformation of the reserved name
    if(upper.Compare(reserved[index]) == 0)
    {
      name = _T("Directory_") + name;
      break;
    }
    // No extension allowed after a reserved name (e.g. "LPT3.txt")
    int pos = upper.Find(reserved[index]);
    if(pos == 0 && name.GetLength() > (int)_tcslen(reserved[index]))
    {
      pos += (int)_tcslen(reserved[index]);
      if(name[pos] == '.')
      {
        name.SetAt(pos,'_');
        break;
      }
    }
  }

  // Check on the ending on a space or dot
  XString ch = name.Right(1);
  if(ch == _T(".") || ch == _T(" "))
  {
    name = name.Left(name.GetLength() - 1);
  }

  // Legal directory name
  return name;
}

// Create a file name from an HTTP resource name
XString
WinFile::FileNameFromResourceName(XString p_resource)
{
  XString filename = CrackedURL::DecodeURLChars(p_resource);
  filename.Replace(_T('/'),_T('\\'));
  filename.Replace(_T("\\\\"),_T("\\"));
  return filename;
}

//////////////////////////////////////////////////////////////////////////
//
// PRIVATE OPERATIONS
//
//////////////////////////////////////////////////////////////////////////

void
WinFile::FilenameParts(XString  p_fullpath
                      ,XString& p_drive
                      ,XString& p_directory
                      ,XString& p_filename
                      ,XString& p_extension)
{
  TCHAR drive [_MAX_DRIVE + 1];
  TCHAR direct[_MAX_DIR   + 1];
  TCHAR fname [_MAX_FNAME + 1];
  TCHAR extens[_MAX_EXT   + 1];

  p_fullpath = StripFileProtocol(p_fullpath);
  _tsplitpath_s(p_fullpath.GetString(),drive,direct,fname,extens);
  p_drive     = drive;
  p_directory = direct;
  p_filename  = fname;
  p_extension = extens;
}

// Generic strip protocol from URL to form OS filenames
// "file:///c|/Program%20Files/Program%23name/file%25name.exe" =>
// "c:\Program Files\Program#name\file%name.exe"
XString
WinFile::StripFileProtocol(XString p_fileref)
{
  if(p_fileref.GetLength() > 8)
  {
    if(_tcsicmp(p_fileref.Left(8),_T("file:///")) == 0)
    {
      p_fileref = p_fileref.Mid(8);
    }
  }
  // Create a filename separator char name
  for(int index = 0; index < p_fileref.GetLength(); ++index)
  {
    if(p_fileref[index] == '/') p_fileref.SetAt(index,'\\');
    if(p_fileref[index] == '|') p_fileref.SetAt(index,':');
  }

  // Resolve the '%' chars in the filename
  ResolveSpecialChars(p_fileref);
  return p_fileref;
}

// Special optimized function to resolve %5C -> '\' in pathnames
// Returns number of chars replaced
int
WinFile::ResolveSpecialChars(XString& p_value)
{
  int total = 0;

  int pos = p_value.Find('%');
  while (pos >= 0)
  {
    ++total;
    int num = 0;
    XString hexstring = p_value.Mid(pos+1,2);
    hexstring.SetAt(0,(TCHAR)toupper(hexstring[0]));
    hexstring.SetAt(1,(TCHAR)toupper(hexstring[1]));

    if(isdigit(hexstring[0]))
    {
      num = hexstring[0] - '0';
    }
    else
    {
      num = hexstring[0] - 'A' + 10;
    }
    num *= 16;
    if (isdigit(hexstring[1]))
    {
      num += hexstring[1] - '0';
    }
    else
    {
      num += hexstring[1] - 'A' + 10;
    }
    p_value.SetAt(pos,(char)num);
    p_value  = p_value.Left(pos+1);
    p_value += p_value.Mid(pos+3);
    pos = p_value.Find('%');
  }
  return total;
}

// Encode a filename in special characters
int
WinFile::EncodeSpecialChars(XString& p_value)
{
  int total = 0;

  int pos = 0;
  while(pos < p_value.GetLength())
  {
    TCHAR ch = (TCHAR) p_value.GetAt(pos);
    if(!_istalnum(ch) && ch != '/')
    {
      // Encoding in 2 chars HEX
      XString hexstring;
      hexstring.Format(_T("%%%2.2X"),ch);

      // Take left and right of the special char
      XString left = p_value.Left(pos);
      XString right = p_value.Mid(pos + 1);

      // Create a new value with the encoded char in it
      p_value = left + hexstring + right;

      // Next position + 1 conversion done
      pos += 2;
      ++total;
    }
    // Look at next character
    ++pos;
  }

  return total;
}

// Getting the first (base) directory
XString
WinFile::GetBaseDirectory(XString& p_path)
{
  XString result;

  // Strip of an extra path separator
  while (p_path[0] == '\\')
  {
    p_path = p_path.Mid(1);
  }

  // Find first separator
  int pos = p_path.Find('\\');
  if (pos >= 0)
  {
    // Return first subdirectory
    result = p_path.Left(pos);
    p_path = p_path.Mid(pos);
  }
  else
  {
    // Last directory in the line
    result = p_path;
    p_path.Empty();
  }
  return result;
}

// Reduce file path name for RE-BASE of directories
// IN:  C:\direct1\direct2\direct3\..\..\direct4 
// OUT: C:\direct1\direct4
XString
WinFile::ReduceDirectoryPath(XString& path)
{
  TCHAR buffer[_MAX_PATH+1] = _T("");
  _tcsncpy_s(buffer,path.GetString(),_MAX_PATH);
  bool foundReduction = true;

  while(foundReduction)
  {
    // Drop out if we find nothing;
    foundReduction = false;

    LPTSTR pnt1 = buffer;

    while(*pnt1 && *pnt1!='\\' && *pnt1!='/') ++pnt1;
    if(!*pnt1++)
    {
      // Not one directory separator
      return path;
    }
    LPTSTR pnt3 = pnt1;
    while(*pnt1 && *pnt1!='\\' && *pnt1!='/') ++pnt1;
    if(!*pnt1++)
    {
      // Not a second directory separator
      return path;
    }
    LPTSTR pnt2 = pnt1;
    while(*pnt1 && *pnt1!='\\' && *pnt1!='/') ++pnt1;
    while(*pnt1)
    {
      ++pnt1;
      // IN:  C:\direct1\direct2\direct3\..\..\direct4\
      //         |       |       |
      //      pnt3    pnt2    pnt1
      if(_tcsncmp(pnt2,_T("..\\"),3)==0 || _tcsncmp(pnt2,_T("../"),3)==0)
      {
        // Space between pnt2 and pnt1 = \..\
        // IN:  C:\direct1\direct2\direct3\..\..\direct4\
        //                         |       |  |
        //                      pnt3    pnt2  pnt1

        // REDUCTION
        size_t len = _MAX_PATH - (pnt3 - buffer);
        _tcscpy_s(pnt3,len,pnt1);

        // At least one more loop
        foundReduction = true;
        // Return to the top!!
        break;
      }
      // Next level of directories
      pnt3 = pnt2;
      pnt2 = pnt1;
      while(*pnt1 && *pnt1!='\\' && *pnt1!='/') ++pnt1;
    }
  }
  return buffer;
}

// Makes a relative pathname from an absolute one
// Absolute: "file:///C:/aaa/bbb/ccc/ddd/eee/file.ext"
// Base    : "file:///C:/aaa/bbb/ccc/rrr/qqq/"
// output
// Relative: "../../ddd/eee/file.ext"
bool
WinFile::MakeRelativePathname(const XString& p_base
                             ,const XString& p_absolute
                             ,      XString& p_relative)
{
  p_relative = _T("");
  XString base     = StripFileProtocol(p_base);
  XString absolute = StripFileProtocol(p_absolute);

  // Special case: no filename
  if(absolute.IsEmpty())
  {
    return true;
  }
  // Special case: base is empty. Cannot make it relative
  // This is a programming error
  if(base.IsEmpty())
  {
    return false;
  }
  // Make all directory separators the same
  base.Replace('\\','/');
  absolute.Replace('\\','/');

  // Special case: only a filename, make it relative to the 'this' directory
  if(absolute.Find('/') < 0)
  {
    // Cannot use this in *.HHP projects!
    // p_relative = XString("./") + absolute;
    p_relative = absolute;
    return true;
  }
  // Special case: already a relative path
  if(absolute.GetAt(0) == '.')
  {
    p_relative = absolute;
    return true;
  }
  // Find the path-parts that are common to both names
  // We can eliminate these parts
  bool notCompatible = true;
  int  pos = absolute.Find('/');
  while(pos >= 0)
  {
    XString left_base = base.Left(pos);
    XString left_abso = absolute.Left(pos);
    if(left_base.CompareNoCase(left_abso))
    {
      // Stop here. Pathnames are different from here.
      break;
    }
    // Eliminate this part
    base = base.Mid(pos + 1);
    absolute = absolute.Mid(pos + 1);
    // Did at least one elimination
    notCompatible = false;
    // Find next position
    pos = absolute.Find('/');
  }
  if(notCompatible)
  {
    // Pathnames are not compatible. i.e. are on another filesystem
    // or on another protocol or ....
    return false;
  }
  // Find the path-parts that are different
  // We can substitute these with "../" parts.
  // This is what we have left:
  // Absolute: "ddd/eee/file.ext"
  // Base    : "rrr/qqq/"
  pos = base.Find('/');
  while(pos >= 0)
  {
    absolute = XString(_T("../")) + absolute;
    base = base.Mid(pos + 1);
    pos  = base.Find('/');
  }
  p_relative = absolute;

  // Relative path is complete.
  // Now warn for files outside the project
  if(p_relative.GetLength() > 1)
  {
    if(p_relative.Left(2) != _T(".."))
    {
      // Did not succeed to make a relative path
      return false;
    }
  }
  return true;
}

// Check for Unicode UTF-16 in the buffer
bool
WinFile::IsTextUnicodeUTF16(const uchar* p_pointer,size_t p_length)
{
  int result = IS_TEXT_UNICODE_UNICODE_MASK;
  return (bool)::IsTextUnicode(p_pointer,(int)p_length,&result);
}

// Check for Unicode UTF-8 in the buffer
//
// https://en.wikipedia.org/wiki/UTF-8
//   
//              Byte 1     Byte 2     Byte 3     Byte 4
// -----------|----------|----------|----------|----------|
// 1 Byte     | 0xxxxxxx |          |          |          |
// 2 Bytes    | 110xxxxx | 10xxxxxx |          |          |
// 3 Bytes    | 1110xxxx | 10xxxxxx | 10xxxxxx |          |
// 4 Bytes    | 11110xxx | 10xxxxxx | 10xxxxxx | 10xxxxxx |
//
bool
WinFile::IsTextUnicodeUTF8(const uchar* p_pointer,size_t p_length)
{
  int blocklen = 1;
  bool ascii = true;

  while(*p_pointer && p_length--)
  {
    unsigned char ch = *p_pointer++;

    if(ascii)
    {
      if(ch & 0x80)
      {
             if((ch >> 3) == 0x1E) blocklen = 3;
        else if((ch >> 4) == 0x0E) blocklen = 2;
        else if((ch >> 5) == 0x06) blocklen = 1;
        else
        {
          // No UTF-8 Escape code
          return false;
        }
        ascii = false;
      }
    }
    else
    {
      if((ch >> 6) == 0x2)
      {
        if(--blocklen == 0)
        {
          ascii = true;
        }
      }
      else
      {
        // Can only for 0x10xxxxxxx
        return false;
      }
    }
  }
  return true;
}

#ifdef _UNICODE
XString
WinFile::ExplodeString(const std::string& p_string,unsigned p_codepage)
{
  // Convert UTF-8 -> UTF-16
  // Convert MBCD  -> UTF-16
  // Getting the needed buffer space (in codepoints! Not bytes!!)
  int length = MultiByteToWideChar(p_codepage,0,p_string.c_str(),-1,NULL,NULL);
  uchar* buffer = new uchar[length * 2];
  // Doing the 'real' conversion
  MultiByteToWideChar(p_codepage,0,p_string.c_str(),-1,reinterpret_cast<LPWSTR>(buffer),length);
  XString result;
  LPCTSTR resbuf = result.GetBufferSetLength(length);
  memcpy((void*) resbuf,buffer,length * 2);
  result.ReleaseBuffer();
  delete[] buffer;
  return result;
}

std::string 
WinFile::ImplodeString(const XString& p_string,unsigned p_codepage)
{
  int clength = 0;
  int blength = 0;
  // Getting the length of the translation buffer first
  clength = ::WideCharToMultiByte(p_codepage,0,(LPCWSTR) p_string.GetString(),-1,NULL,0,NULL,NULL);
  char* buffer = new char[clength + 1];
  blength = ::WideCharToMultiByte(p_codepage,0,(LPCWSTR) p_string.GetString(),clength,(LPSTR) buffer,clength,NULL,NULL);
  buffer[clength] = 0;
  if(blength > 0 && blength < clength)
  {
    buffer[blength] = 0;
  }
  std::string result(buffer);
  delete[] buffer;
  return result;
}
#endif

//////////////////////////////////////////////////////////////////////////
// 
// STREAMING OPERATORS
//
//////////////////////////////////////////////////////////////////////////

WinFile& 
WinFile::operator<<(const TCHAR p_char)
{
  XString str(p_char);
  Write(str);
  return *this;
}

WinFile& 
WinFile::operator<<(const short p_num)
{
  if(m_openMode & open_trans_binary)
  {
    Write(static_cast<void*>(const_cast<short*>(&p_num)),sizeof(short));
  }
  else
  {
    XString buf;
    buf.Format(_T("%d"),(int) p_num);
    Write(buf);
  }
  return *this;
}

WinFile&
WinFile::operator<<(const int p_num)
{
  if(m_openMode & open_trans_binary)
  {
    Write(static_cast<void*>(const_cast<int*>(&p_num)),sizeof(int));
  }
  else
  {
    XString buf;
    buf.Format(_T("%d"),p_num);
    Write(buf);
  }
  return *this;
}

WinFile&
WinFile::operator<<(const unsigned p_num)
{
  if(m_openMode & open_trans_binary)
  {
    Write(static_cast<void*>(const_cast<unsigned*>(&p_num)),sizeof(unsigned));
  }
  else
  {
    XString buf;
    buf.Format(_T("%u"),p_num);
    Write(buf);
  }
  return *this;
}

WinFile&
WinFile::operator<<(const INT64 p_num)
{
  if(m_openMode & open_trans_binary)
  {
    Write(static_cast<void*>(const_cast<INT64*>(&p_num)),sizeof(INT64));
  }
  else
  {
    XString buf;
    buf.Format(_T("%I64d"),p_num);
    Write(buf);
  }
  return *this;
}


WinFile&
WinFile::operator<<(const float p_num)
{
  if(m_openMode & open_trans_binary)
  {
    Write(static_cast<void*>(const_cast<float*>(&p_num)),sizeof(short));
  }
  else
  {
    XString buf;
    buf.Format(_T("%G"),(double)p_num);
    Write(buf);
  }
  return *this;
}

WinFile&
WinFile::operator<<(const double p_num)
{
  if(m_openMode & open_trans_binary)
  {
    Write(static_cast<void*>(const_cast<double*>(&p_num)),sizeof(short));
  }
  else
  {
    XString buf;
    buf.Format(_T("%G"),p_num);
    Write(buf);
  }
  return *this;
}

WinFile&
WinFile::operator<<(const LPCTSTR p_string)
{
  XString string(p_string);
  *this << string;
  return *this;
}

WinFile&
WinFile::operator<<(const XString& p_string)
{
  if(m_openMode & open_trans_binary)
  {
    Write(static_cast<void*>(const_cast<PTCHAR>(p_string.GetString())),p_string.GetLength() * sizeof(TCHAR));
  }
  else
  {
    Write(p_string);
  }
  return *this;
}

WinFile&
WinFile::operator>>(TCHAR& p_char)
{
  int read = 0;
  Read(reinterpret_cast<void*>(&p_char),sizeof(TCHAR),read);
  return *this;
}

WinFile&
WinFile::operator>>(short& p_num)
{
  if(m_openMode & open_trans_binary)
  {
    int read = 0;
    Read(reinterpret_cast<void*>(&p_num),sizeof(short),read);
  }
  else
  {
    m_error = ERROR_INVALID_FUNCTION;
  }
  return *this;
}

WinFile&
WinFile::operator>>(int& p_num)
{
  if(m_openMode & open_trans_binary)
  {
    int read = 0;
    Read(reinterpret_cast<void*>(&p_num),sizeof(int),read);
  }
  else
  {
    m_error = ERROR_INVALID_FUNCTION;
  }
  return *this;
}

WinFile&
WinFile::operator>>(unsigned& p_num)
{
  if(m_openMode & open_trans_binary)
  {
    int read = 0;
    Read(reinterpret_cast<void*>(&p_num),sizeof(unsigned),read);
  }
  else
  {
    m_error = ERROR_INVALID_FUNCTION;
  }
  return *this;
}

WinFile&
WinFile::operator>>(INT64& p_num)
{
  if(m_openMode & open_trans_binary)
  {
    int read = 0;
    Read(reinterpret_cast<void*>(&p_num),sizeof(INT64),read);
  }
  else
  {
    m_error = ERROR_INVALID_FUNCTION;
  }
  return *this;
}

WinFile&
WinFile::operator>>(float& p_num)
{
  if(m_openMode & open_trans_binary)
  {
    int read = 0;
    Read(reinterpret_cast<void*>(&p_num),sizeof(float),read);
  }
  else
  {
    m_error = ERROR_INVALID_FUNCTION;
  }
  return *this;
}

WinFile&
WinFile::operator>>(double& p_num)
{
  if(m_openMode & open_trans_binary)
  {
    int read = 0;
    Read(reinterpret_cast<void*>(&p_num),sizeof(double),read);
  }
  else
  {
    m_error = ERROR_INVALID_FUNCTION;
  }
  return *this;
}

WinFile&
WinFile::operator>>(XString& p_string)
{
  if(m_openMode & open_trans_text)
  {
    Read(p_string);
  }
  else
  {
    m_error = ERROR_INVALID_FUNCTION;
  }
  return *this;
}

//////////////////////////////////////////////////////////////////////////
//
// PAGE BUFFER:
// This is the cache from the file system
//
//////////////////////////////////////////////////////////////////////////

// Create a page buffer if we do not have one
uchar*
WinFile::PageBuffer()
{
  if(m_pageBuffer == nullptr)
  {
    m_pageBuffer  = new uchar[(size_t)PAGESIZE + (size_t)1];
    m_pagePointer = m_pageBuffer;
    m_pageTop     = m_pageBuffer;  // Buffer is empty!
    m_pageBuffer[PAGESIZE] = 0;
  }
  return m_pagePointer;
}

// Free the page buffer again
void
WinFile::PageBufferFree()
{
  if(m_pageBuffer)
  {
    delete [] m_pageBuffer;
    m_pageBuffer = nullptr;
    m_pageTop    = nullptr;
  }
  m_pagePointer = nullptr;
}

// Reading one (1) character from the file buffer
// Prerequisite: the file must be opened
int
WinFile::PageBufferRead()
{
  bool scanBom = false;

  // Make sure we have a page buffer
  if(!m_pageBuffer)
  {
    PageBuffer();
    scanBom = true;
  }
  // If pagebuffer is empty 
  // or we are finished reading the buffer
  // read in a new buffer from the file system
  if(m_pagePointer == m_pageTop)
  {
    // Reset error state
    m_error = 0;

    // if reading AND writing, flush buffer back to disk
    if(m_openMode & FFlag::open_write)
    {
      // Acquire a file access lock
      AutoCritSec lock(&m_fileaccess);

      DWORD write = 0;
      DWORD size = (DWORD)((size_t)m_pagePointer - (size_t)m_pageBuffer);
      if (::WriteFile(m_file, m_pageBuffer, size, &write, nullptr) == 0)
      {
        m_error = ::GetLastError();
        return EOF;
      }
      m_pagePointer = m_pageBuffer;

      // if read-and-write: read in first page and reset FPOS
      if (PageBufferReadForeward(scanBom) == false)
      {
        return EOF;
      }
    }
    else
    {
      // OPTIMIZE READ FOREWARD!!
      // Read-in one buffer
      // Acquire a file access lock
      AutoCritSec lock(&m_fileaccess);

      DWORD size = 0;
      if(::ReadFile(m_file,m_pageBuffer,(DWORD)PAGESIZE,&size,nullptr) == 0)
      {
        m_error = ::GetLastError();
        return EOF;
      }
      // Read-nothing: legal end-of-file
      if(size == 0)
      {
        return EOF;
      }
      m_pagePointer = m_pageBuffer;
      m_pageTop     = (uchar*) ((size_t)m_pageBuffer + (size_t)size);

      if(size <= PAGESIZE)
      {
        m_pageBuffer[size] = 0;
      }

      // First buffer read in: scan for encoding
      if(scanBom)
      {
        ScanBomInFirstPageBuffer();
        if(m_pagePointer >= m_pageTop)
        {
          return EOF;
        }
      }
    }
  }
  return (int) *m_pagePointer++;
}

// Scan for a valid BYTE-ORDER-MARK: UTF-8 or UTF-16 Big Endian
void
WinFile::ScanBomInFirstPageBuffer()
{
  Encoding type;
  unsigned skip = 0;
  if(DefuseBOM(m_pagePointer,type,skip) == BOMOpenResult::BOM)
  {
    m_foundBOM     = true;
    m_encoding     = type;
    m_pagePointer += skip;
  }
}

// Writing one (1) character to the file buffer
// Prerequisite: the file must be opened
bool
WinFile::PageBufferWrite(uchar ch)
{
  // Make sure we have a page buffer
  if(!m_pageBuffer)
  {
    PageBuffer();

    if (PageBufferReadForeward(false) == false)
    {
      return false;
    }
  }

  // If we have a full page buffer: write it to the file system
  // afterwards we reset the file pointer so we can fill the buffer again
  if(m_pagePointer == m_pageTop)
  {
    // Acquire a file access lock
    AutoCritSec lock(&m_fileaccess);

    DWORD write = 0;
    DWORD size = (DWORD)((size_t)m_pagePointer - (size_t)m_pageBuffer);
    if (::WriteFile(m_file,m_pageBuffer,size,&write,nullptr) == 0)
    {
      m_error = ::GetLastError();
      return false;
    }
    m_pagePointer = m_pageBuffer;

    // if read-and-write: read in first page and reset FPOS
    if(PageBufferReadForeward(false) == false)
    {
      return false;
    }
  }
  // Now write our character for sure
  *m_pagePointer++ = ch;
  return true;
}

bool
WinFile::PageBufferReadForeward(bool p_scanBom)
{
  // if read-and-write: read in first page and reset FPOS
  if (m_openMode & FFlag::open_read)
  {
    // Acquire a file access lock
    AutoCritSec lock(&m_fileaccess);

    DWORD size = 0;
    if(::ReadFile(m_file,m_pageBuffer,PAGESIZE,&size,nullptr) == 0)
    {
      m_error = ::GetLastError();
      return false;
    }
    if(size)
    {
      m_pageTop = m_pageBuffer + size;
      if(PageBufferAdjust(true) == false)
      {
        return false;
      }
    }
    // First buffer read in: scan for encoding
    if(p_scanBom && (m_openMode & FFlag::open_trans_text))
    {
      ScanBomInFirstPageBuffer();
    }
    return true;
  }
  m_pageTop = m_pageBuffer + PAGESIZE;
  return true;
}

bool
WinFile::PageBufferFlush()
{
  // Check if we have a page buffer
  if(!m_pageBuffer)
  {
    return true;
  }
  // See if we have anything in the buffer
  if(m_pagePointer == m_pageBuffer)
  {
    return true;
  }

  // Only do this in write mode or read-write mode
  // IN read mode it is *NOT* an error
  if(!(m_openMode & FFlag::open_write))
  {
    return true;
  }

  // Acquire a file access lock
  AutoCritSec lock(&m_fileaccess);

  // Write the valid amount in the file buffer
  DWORD written = 0;
  DWORD write = (DWORD)((size_t)m_pagePointer - (size_t)m_pageBuffer);
  if (::WriteFile(m_file,m_pageBuffer,write,&written,nullptr) == 0)
  {
    m_error = ::GetLastError();
    return false;
  }
  // Resetting the page buffer to clean state: begin writing again
  m_pagePointer = m_pageBuffer;
  m_pageTop     = m_pageBuffer;

  return true;
}

// In case of reading, by reading a large buffer we actually read past the 
// logical point of reading by the class functions
// To gain the user's perspective, we set the file pointer back to the position
// the program has been reading to
bool
WinFile::PageBufferAdjust(bool p_beginning /*= false*/) const
{
  // Check if we have a page buffer
  if(!m_pageBuffer)
  {
    return true;
  }

  // See if we did not read to the actual position
  if((m_pagePointer == m_pageTop) && !p_beginning)
  {
    // **REAL** file position already in sync!
    return true;
  }

  // Calculate the distance of the file pointer to set back to the actual read position
  LONGLONG distance = 0L;
  if(p_beginning)
  {
    // Go back the complete buffer
    distance = (size_t)m_pageBuffer - (size_t)m_pageTop;
  }
  else
  {
    // Go back to the last read/write position
    distance = (size_t)m_pagePointer - (size_t)m_pageTop;
  }

  // Set the position
  LARGE_INTEGER move = { 0, 0 };
  LARGE_INTEGER pos  = { 0, 0 };
  move.QuadPart = distance;

  // Acquire a file access lock
  AutoCritSec lock(&m_fileaccess);

  // Perform the file move
  if(::SetFilePointerEx(m_file,move,&pos,FILE_CURRENT) == 0)
  {
    m_error = ::GetLastError();
    return false;
  }

  return true;
}
