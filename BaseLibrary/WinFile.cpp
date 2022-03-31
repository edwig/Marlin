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
#include <shellapi.h>
#include <iostream>
#include <fileapi.h>
#include <handleapi.h>
#include <shlwapi.h>
#include <shlobj.h>
#include <winnt.h>
#include <commdlg.h>
#include <AclAPI.h>
#include <filesystem>
#include <algorithm>
#include <io.h>

// Do not complain about enum classes
#pragma warning (disable: 26812)

//////////////////////////////////////////////////////////////////////////
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
}

// CTOR from a filename
WinFile::WinFile(XString p_filename)
        :m_filename(p_filename)
{
}

// CTOR from another file pointer
WinFile::WinFile(WinFile& p_other)
{
  *this = p_other;
}

// DTOR
WinFile::~WinFile()
{
  // Try to close an opened file. Ignore the errors
  Close();
  // Just to be sure if the closing went wrong
  PageBufferFree();
}

//////////////////////////////////////////////////////////////////////////
//
// General functions and operations
//
//////////////////////////////////////////////////////////////////////////

bool
WinFile::Open(DWORD p_flags /*= winfile_read*/,FAttributes p_attribs /*= FAttributes::attrib_none*/)
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

  // Try to create/open the file
  m_file = CreateFile((LPCSTR)m_filename.GetString()
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
    dirToBeOpened += "\\";
    dirToBeOpened += part;
    if (!::CreateDirectory(dirToBeOpened.GetString(),nullptr))
    {
      m_error = ::GetLastError();
      if(m_error != ERROR_ALREADY_EXISTS)
      {
        // Cannot create the directory
        return false;
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
  return _access(m_filename.GetString(),0) != -1;
}

// Check for access to the file
bool
WinFile::CanAccess(bool p_write /*= false*/)
{
  // In case we already had an opened file: use the file modes
  if(m_file)
  {
    // See if file was opened in write-mode
    if(p_write)
    {
      return (m_openMode & FFlag::open_read);
    }
    // Existence and read-access both granted
    return true;
  }
  
  // Go take a look on the filesystem

  // Default check for read access
  int mode = 4;

  // Also check for write access
  if(p_write)
  {
    mode |= 2;
  }
  // Use low-level POSIX access function.
  // Other MS-Windows functions call this one, so use it directly
  if(_access(m_filename.GetString(),mode) != -1)
  {
    return true;
  }
  return false;
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
  if(m_filename.IsEmpty() || _access(m_filename.GetString(),0) != 0)
  {
    m_error = ERROR_FILE_NOT_FOUND;
    return false;
  }

  // Copy the filename for extra 0's after the name
  // so it becomes a list of just one (1) filename
  char   filelist  [MAX_PATH + 2];
  memset(filelist,0,MAX_PATH + 2);
  strcpy_s(filelist,MAX_PATH,m_filename.GetString());

  SHFILEOPSTRUCT file;
  memset(&file,0,sizeof(SHFILEOPSTRUCT));
  file.wFunc  = FO_DELETE;
  file.pFrom  = filelist;
  file.fFlags = FOF_FILESONLY | FOF_ALLOWUNDO;
  file.lpszProgressTitle = "Progress";

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
  char tempdirectory[MAX_PATH - 14];
  ::GetTempPath(MAX_PATH-14,tempdirectory);

  // Getting the temp filename
  char tempfilename[MAX_PATH + 1];
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
WinFile::OpenAsSharedMemory(XString   p_name
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
  p_name = (p_local ? "Local\\" : "Global\\") + p_name;

  // DO either a create of, or an open of an existing memory segment
  if(p_trycreate)
  {
    // Try to open in read/write mode
    if(!Open(winfile_write))
    {
      // If fails: m_error already set
      return nullptr;
    }

    // File is now opened: try to create a file mapping
    DWORD protect  = PAGE_READWRITE;
    DWORD sizeLow  = p_size & 0x0FFFFFFF;
    DWORD sizeHigh = p_size >> 32;
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
    if(ERROR_SUCCESS != SetNamedSecurityInfo((LPSTR)m_filename.GetString()     // name of the object
                                             ,SE_FILE_OBJECT               // type of object: file or directory
                                             ,DACL_SECURITY_INFORMATION    // change only the object's DACL
                                             ,nullptr,nullptr              // do not change owner or group
                                             ,acl                          // DACL specified
                                             ,nullptr))                    // do not change SACL
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

// Read a XString (possibly with CR/LF to newline translation)
bool
WinFile::Read(XString& p_string)
{
  XString result;

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

  bool crstate = false;

  while(true)
  {
    char ch = PageBufferRead();
    if(ch == EOF)
    {
      m_error = ::GetLastError();
      p_string = result;
      return false;
    }
    result += ch;

    // Do the CR/LF to "\n" translation
    if(m_openMode & FFlag::open_trans_text)
    {
      if(ch == '\r')
      {
        crstate = true;
        continue;
      }
      if(crstate && ch == '\n')
      {
        result.SetAt(result.GetLength() - 2,ch);
        result = result.Left(result.GetLength()-1);
      }
    }
    crstate = false;

    // See if we are ready reading the XString
    if(ch == '\n')
    {
      break;
    }
  }
  p_string = result;
  return true;
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

  DWORD read = 0;
  if(::ReadFile(m_file,p_buffer,(DWORD)p_bufsize,&read,nullptr) == 0)
  {
    m_error = ::GetLastError();
    return false;
  }
  p_read = read;
  return true;
}

// Writing a XString to the WinFile
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

  bool doText = (m_openMode & FFlag::open_trans_text) > 0;

  for(int index = 0;index < p_string.GetLength();++index)
  {
    int ch = p_string.GetAt(index);
    if(doText && ch == '\n')
    {
      if(PageBufferWrite('\r') == false)
      {
        break;
      }
    }
    if(PageBufferWrite(ch) == false)
    {
      break;
    }
  }

  return true;
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

  // Not in conjunction with the STDIO like functions
  // Only in binary mode! Do not use the backing page buffer
  if (!(m_openMode & FFlag::open_trans_binary) || m_pageBuffer)
  {
    m_error = ERROR_INVALID_FUNCTION;
    return false;
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
WinFile::Format(LPCSTR p_format, ...)
{
  va_list argList;
  va_start(argList, p_format);
  bool result = FormatV(p_format, argList);
  va_end(argList);
  return result;
}

bool
WinFile::FormatV(LPCSTR p_format, va_list p_list)
{
  // Getting a buffer of the correct length
  int len = _vscprintf(p_format, p_list) + 1;
  char* buffer = new char[len];
  // Formatting the parameters
  vsprintf_s(buffer, len, p_format, p_list);
  // Adding to the string
  bool result = Write(buffer, len - 1);
  delete[] buffer;
  return result;
}

// Getting the current file position
// Returns the file position, or -1 for error
size_t
WinFile::Position()
{
  // Reset the error
  m_error = 0;

  // See if file is open
  if (m_file == nullptr)
  {
    m_error = ERROR_FILE_NOT_FOUND;
    return -1;
  }

  // Read files should readjust the current position
  PageBufferAdjust();

  LARGE_INTEGER move = { 0, 0 };
  LARGE_INTEGER pos  = { 0, 0 };

  if(::SetFilePointerEx(m_file, move, &pos, FILE_CURRENT) == 0)
  {
    m_error = ::GetLastError();
    return -1;
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
    return -1;
  }

  // Check if we where opened for random access, 
  // so we can move the file pointer
  if((m_openMode & FFlag::open_random_access) == 0)
  {
    m_error = ERROR_INVALID_FUNCTION;
    return -1;
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
                              return -1;
  }

  // Perform the file move
  if(::SetFilePointerEx(m_file,move,&pos,method) == 0)
  {
    m_error = ::GetLastError();
    return -1;
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

// Getting a XString much like "fgets(buffer*,size,FILE*)"
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
  *p_buffer++ = Getch();

  // Scan forward in the pagebuffer for a newline
  char* ending = strchr((char*)m_pagePointer,'\n');
  if(ending && ((size_t)ending < (size_t)m_pageTop))
  {
    // Found a newline before the end of the buffer
    // So do an optimized buffer copy of the whole XString
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
    int ch = *p_buffer++;
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
     int ch(m_ungetch);
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

// Check for a Byte-Order-Mark (BOM)
// Returns the found type of BOM
// and the number of bytes to skip in your input
BOMOpenResult 
WinFile::DefuseBOM(const uchar*  p_pointer
                 ,BOMType&      p_type
                 ,unsigned int& p_skip)
{
  // Preset nothing
  p_type = BOMType::BT_NO_BOM;
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
    p_type = BOMType::BT_BE_UTF8;
    return BOMOpenResult::BOR_Bom;
  }
  // Check UTF-8 BOM in other Endian 
  if(c1 == 0xBB || c1 == 0xBF)
  {
    // UTF-8 but incompatible. Might work yet!!
    p_skip = 3;
    p_type = BOMType::BT_LE_UTF8;
    return BOMOpenResult::BOR_OpenedIncompatible;
  }
  // Check Big-Endian UTF-16
  if(c1 == 0xFE && c2 == 0xFF)
  {
    p_skip = 2;
    p_type = BOMType::BT_BE_UTF16;
    return BOMOpenResult::BOR_Bom;
  }
  // Check Little-Endian UTF-16/UTF32
  if(c1 == 0xFF && c2 == 0xFE)
  {
    if(c3 == 0x0 && c4 == 0x0)
    {
      p_skip = 4;
      p_type = BOMType::BT_LE_UTF32;
      return BOMOpenResult::BOR_OpenedIncompatible;
    }
    p_skip = 2;
    p_type = BOMType::BT_LE_UTF16;
    return BOMOpenResult::BOR_OpenedIncompatible;
  }
  // Check Big-Endian UTF-32
  if(c1 == 0 && c2 == 0 && c3 == 0xFE && c4 == 0xFF)
  {
    p_skip = 4;
    p_type = BOMType::BT_BE_UTF32;
    return BOMOpenResult::BOR_OpenedIncompatible;
  }
  // Check for UTF-7 special case
  if(c1 == 0x2B && c2 == 0x2F && c3 == 0x76)
  {
    if(c4 == 0x38 || c4 == 39 || c4 == 0x2B || c4 == 0x2F)
    {
      // Application still has to process lower 2 bits 
      // of the 4th character. Re-spool to that char.
      p_skip = 3;
      p_type = BOMType::BT_BE_UTF7;
      return BOMOpenResult::BOR_OpenedIncompatible;
    }
  }
  // Check for UTF-1 special case
  if(c1 == 0xF7 && c2 == 0x64 && c3 == 0x4C)
  {
    p_skip = 3;
    p_type = BOMType::BT_BE_UTF1;
    return BOMOpenResult::BOR_OpenedIncompatible;
  }
  // Check for UTF-EBCDIC IBM set
  if(c1 == 0xDD && c2 == 0x73 && c3 == 0x66 && c4 == 0x73)
  {
    p_skip = 4;
    p_type = BOMType::BT_UTF_EBCDIC;
    return BOMOpenResult::BOR_OpenedIncompatible;
  }
  // Check for CSCU 
  if(c1 == 0x0E && c2 == 0xFE && c3 == 0xFF)
  {
    p_skip = 3;
    p_type = BOMType::BT_BE_CSCU;
    return BOMOpenResult::BOR_OpenedIncompatible;
  }
  // Check for BOCU-1
  if(c1 == 0xFB && c2 == 0xEE && c3 == 0x28)
  {
    p_skip = 3;
    p_type = BOMType::BT_BOCU_1;
    return BOMOpenResult::BOR_OpenedIncompatible;
  }
  // Check GB-18030
  if(c1 == 0x84 && c2 == 0x31 && c3 == 0x95 && c4 == 0x33)
  {
    p_skip = 4;
    p_type = BOMType::BT_GB_18030;
    return BOMOpenResult::BOR_OpenedIncompatible;
  }
  // NOT A BOM !!
  return BOMOpenResult::BOR_NoBom;
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
  CHAR          special[MAX_PATH];        // The path for Favorites.
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

  m_filename = special + XString("\\") + p_filename;
  return result;
}

// Set the file handle, but only if the file was not (yet) opened
bool
WinFile::SetFileHandle(HANDLE p_handle)
{
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
  // Buffer that gets the error message XString
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
WinFile::GetFileSize()
{
  // Reset the error
  m_error = 0;

  // Try by opened file
  if(m_file)
  {
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
  XString     encoded;
  bool        queryValue = false;
  static char unsafeString[]   = " \"<>#{}|^~[]`";
  static char reservedString[] = "$&/;?@-!*()'";

  // Re-encode the XString. 
  // Watch out: strange code ahead!
  for(int ind = 0;ind < m_filename.GetLength(); ++ind)
  {
    unsigned char ch = m_filename[ind];
    if(ch == '?')
    {
      queryValue = true;
    }
    if(ch == ' ' && queryValue)
    {
      encoded += '+';
    }
    else if(strchr(unsafeString,ch) ||                    // " \"<>#{}|\\^~[]`"     -> All strings
           (queryValue && strchr(reservedString,ch)) ||   // "$&+,./:;=?@-!*()'"    -> In query parameter value strings
           (ch < 0x20) ||                                 // 7BITS ASCII Control characters
           (ch > 0x7F) )                                  // Converted UTF-8 characters
    {
      char extra[3];
      sprintf_s(extra,3,"%%%2.2X",(int)ch);
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

bool
WinFile::SetFilenameByDialog(HWND   p_parent      // Parent window (if any)
                            ,bool   p_open        // true = Open/New, false = SaveAs
                            ,XString p_title       // Title of the dialog
                            ,XString p_defext      // Default extension
                            ,XString p_filename    // Default first file
                            ,int    p_flags       // Default flags
                            ,XString p_filter      // Filter for extensions
                            ,XString p_direct)     // Directory to start in
{
  // Reset error
  m_error = 0;
  
  // Open filename structure
  OPENFILENAME ofn;
  char    original[MAX_PATH + 1];
  char    filename[MAX_PATH + 1];
  XString  filter;

  if (p_filter.IsEmpty())
  {
    filter = "All files (*.*)|*.*|";
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
  strncpy_s(filename,MAX_PATH,p_filename.GetString(),MAX_PATH);

  // Prepare the filter
  filter += "||";
  for(int index = 0; index < filter.GetLength(); ++index)
  {
    if(filter[index] == '|')
    {
      filter.SetAt(index,0);
    }
  }

  // Fill in the filename structure
  p_flags |= OFN_ENABLESIZING | OFN_LONGNAMES | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_EXPLORER;
  p_flags &= ~(OFN_NODEREFERENCELINKS | OFN_NOTESTFILECREATE);

  ofn.lStructSize       = sizeof(OPENFILENAME);
  ofn.hwndOwner         = p_parent;
  ofn.hInstance         = (HINSTANCE)GetWindowLongPtr(p_parent,GWLP_HINSTANCE);
  ofn.lpstrFile         = (LPSTR)filename;
  ofn.lpstrDefExt       = (LPSTR)p_defext.GetString();
  ofn.lpstrTitle        = (LPSTR)p_title.GetString();
  ofn.lpstrFilter       = (LPSTR)filter.GetString();
  ofn.Flags             = p_flags;
  ofn.nFilterIndex      = 1;    // Use lpstrFilter
  ofn.nMaxFile          = MAX_PATH;
  ofn.lpstrCustomFilter = nullptr; //(LPSTR) buf_filter;
  ofn.nMaxCustFilter    = 0;
  ofn.lpstrFileTitle    = nullptr;
  ofn.nMaxFileTitle     = 0;
  ofn.lpstrInitialDir   = (LPCSTR)p_direct.GetString();
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
  return _stricmp(m_filename.GetString(), p_other.m_filename.GetString()) == 0;
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
    return "NewDirectory";
  }

  XString name(p_name);
  const char  forbidden_all[] = "<>:\"/\\|?*";
  const char  forbidden_ext[] = "<>:\"/\\|?*.";
  const char* reserved[] =
  {
    "CON", "PRN", "AUX", "NUL",
    "COM1","COM2","COM3","COM4","COM5","COM6","COM7","COM8","COM9",
    "LPT1","LPT2","LPT3","LPT4","LPT5","LPT6","LPT7","LPT8","LPT9"
  };

  // Use either one of these
  const char* forbidden = p_extensionAllowed ? forbidden_all : forbidden_ext;

  // Replace forbidden characters by an '_'
  for(int index = 0;index < name.GetLength();++index)
  {
    // Replace interpunction
    if(strchr(forbidden,name.GetAt(index)) != nullptr)
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
    if(strcmp(reserved[index],upper.GetString()) == 0)
    {
      name = "Directory_" + name;
      break;
    }
    // No extension allowed after a reserved name (e.g. "LPT3.txt")
    int pos = upper.Find(reserved[index]);
    if(pos == 0 && name.GetLength() > (int)strlen(reserved[index]))
    {
      pos += (int)strlen(reserved[index]);
      if(name[pos] == '.')
      {
        name.SetAt(pos,'_');
        break;
      }
    }
  }

  // Check on the ending on a space or dot
  XString ending = name.Right(1);
  if(ending[0] == '.' || ending[0] == ' ')
  {
    name = name.Left(name.GetLength()-1);
  }

  // Legal directory name
  return name;
}

//////////////////////////////////////////////////////////////////////////
//
// PRIVATE OPERATIONS
//
//////////////////////////////////////////////////////////////////////////

void
WinFile::FilenameParts(XString p_fullpath,XString& p_drive,XString& p_directory,XString& p_filename,XString& p_extension)
{
  char drive [_MAX_DRIVE + 1];
  char direct[_MAX_DIR   + 1];
  char fname [_MAX_FNAME + 1];
  char extens[_MAX_EXT   + 1];

  p_fullpath = StripFileProtocol(p_fullpath);
  _splitpath_s(p_fullpath.GetString(),drive,direct,fname,extens);
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
    if(_stricmp(p_fileref.Mid(0,8).GetString(),"file:///") == 0)
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
    hexstring.SetAt(0,toupper(hexstring[0]));
    hexstring.SetAt(1,toupper(hexstring[1]));

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
    p_value = p_value.Mid(0,pos+1) + p_value.Mid(pos+3);
    pos = p_value.Find('%');
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
    result = p_path.Mid(0,pos);
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
  // Make sure we have a page buffer
  if(!m_pageBuffer)
  {
    PageBuffer();
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
      DWORD write = 0;
      DWORD size = (DWORD)((size_t)m_pagePointer - (size_t)m_pageBuffer);
      if (::WriteFile(m_file, m_pageBuffer, size, &write, nullptr) == 0)
      {
        m_error = ::GetLastError();
        return false;
      }
      m_pagePointer = m_pageBuffer;

      // if read-and-write: read in first page and reset FPOS
      if (PageBufferReadForeward() == false)
      {
        return false;
      }
    }
    else
    {
      // OPTIMIZE READ FOREWARD!!
      // Read-in one buffer
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
    }
  }
  return (int)(*m_pagePointer++);
}

// Writing one (1) character to the file buffer
// Prerequisite: the file must be opened
bool
WinFile::PageBufferWrite(int ch)
{
  // Make sure we have a page buffer
  if(!m_pageBuffer)
  {
    PageBuffer();

    if (PageBufferReadForeward() == false)
    {
      return false;
    }
  }

  // If we have a full page buffer: write it to the file system
  // afterwards we reset the file pointer so we can fill the buffer again
  if(m_pagePointer == m_pageTop)
  {
    DWORD write = 0;
    DWORD size = (DWORD)((size_t)m_pagePointer - (size_t)m_pageBuffer);
    if (::WriteFile(m_file,m_pageBuffer,size,&write,nullptr) == 0)
    {
      m_error = ::GetLastError();
      return false;
    }
    m_pagePointer = m_pageBuffer;

    // if read-and-write: read in first page and reset FPOS
    if(PageBufferReadForeward() == false)
    {
      return false;
    }
  }
  // Now write our character for sure
  *m_pagePointer++ = (uchar)ch;
  return true;
}

bool
WinFile::PageBufferReadForeward()
{
  // if read-and-write: read in first page and reset FPOS
  if (m_openMode & FFlag::open_read)
  {
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
WinFile::PageBufferAdjust(bool p_beginning /*= false*/)
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

  // Perform the file move
  if(::SetFilePointerEx(m_file,move,&pos,FILE_CURRENT) == 0)
  {
    m_error = ::GetLastError();
    return false;
  }

  return true;
}
