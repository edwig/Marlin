//////////////////////////////////////////////////////////////////////////
//
// File: WinFile.h
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
#pragma once
#include <windows.h>
#include <atlstr.h>
#include <string>
#include "Encoding.h"
#include "AutoCritical.h"

// MS-Windows codepage numbers for translation of text files
#define CODEPAGE_UTF8   65001

typedef unsigned char uchar;

// Flags for the open modes
typedef enum _fflag
{
  no_mode             = 0x00000   // No mode yet. Pristine object
  // Creating or opening
 ,open_and_create     = 0x00001   // Standard open in write mode  (create file and open, even if it exists)
 ,open_and_append     = 0x00002   // Standard open in append mode (create-or-open and set at EOF)
 ,open_allways        = 0x00003   // Open or create 
 ,open_if_exists      = 0x00004   // Standard open in read mode   (open if it exists, otherwise fails)
 ,open_no_overwrite   = 0x00005   // Open only if it does NOT exists. non-standard write to file
 ,open_truncate       = 0x00006   // Open and truncate the contents of the file
  // Read-write modes
 ,open_read           = 0x00010   // Open in read  mode (possibly blocking others)
 ,open_write          = 0x00020   // Open in write mode (possibly blocking others)
 ,open_shared_read    = 0x00040   // Open in read  mode, shared with other shared readers
 ,open_shared_write   = 0x00080   // Open in write mode, shared with other shared writers
  // Transformations
 ,open_trans_text     = 0x00100   // "\n" to "CR-LF" translations
 ,open_trans_binary   = 0x00200   // Binary access only, no string access
  // Special modes
 ,open_random_access  = 0x01000   // both read-and-write random access and file-repositioning
 ,open_sequential     = 0x02000   // sequential access in read or in write mode, but not both
 ,open_lockmode       = 0x04000   // can do a lock-file-position
  // Temporary file specials
 ,temp_persistent     = 0x10000   // temporary files stay in cache for quick re-opening
 ,temp_removeOnClose  = 0x20000   // temporary files will be removed after closing
}
FFlag;

// Predefined filters for the open flags
#define fflag_filter_create     0x00000F
#define fflag_filter_readwrite  0x0000F0
#define fflag_filter_transform  0x000F00
#define fflag_filter_special    0x00F000
#define fflag_filter_tempfile   0x0F0000

// Most used open modes
#define winfile_read   ((FFlag)(open_if_exists  | open_read  ))
#define winfile_write  ((FFlag)(open_and_create | open_write ))
#define winfile_append ((FFlag)(open_and_append | open_write | open_shared_write ))

typedef enum _fattrib
{
  attrib_none             = 0x00000000  // Nothing yet
 ,attrib_readonly         = 0x00000001  // FILE_ATTRIBUTE_READONLY
 ,attrib_hidden           = 0x00000002  // FILE_ATTRIBUTE_HIDDEN
 ,attrib_system           = 0x00000004  // FILE_ATTRIBUTE_SYSTEM
 ,attrib_directory        = 0x00000010  // FILE_ATTRIBUTE_DIRECTORY
 ,attrib_archive          = 0x00000020  // FILE_ATTRIBUTE_ARCHIVE
 ,attrib_normal           = 0x00000080  // FILE_ATTRIBUTE_NORMAL
 ,attrib_temporary        = 0x00000100  // FILE_ATTRIBUTE_TEMPORARY (File is kept in cache after close, for a re-open action)
 ,attrib_encrypted        = 0x00004000  // FILE_ATTRIBUTE_ENCRYPTED
 ,attrib_delete_on_close  = 0x04000000  // FILE_FLAG_DELETE_ON_CLOSE
 ,attrib_no_buffering     = 0x20000000  // FILE_FLAG_NO_BUFFERING (link your program with commobj !!)
 ,attrib_remotestorage    = 0x00100000  // FILE_FLAG_NO_RECALL (keep in remote storage)
 ,attrib_posix            = 0x01000000  // FILE_FLAG_POSIX_SEMANTICS (Case sensitive filenames !!)
 ,attrib_rdp_aware        = 0x00800000  // FILE_FLAG_SESSION_AWARE (Cannot be opened in session 0)
 ,attrib_write_through    = 0x80000000  // FILE_FLAG_WRITE_THROUGH
}
FAttributes;

// Flag for the seek operations in a file
typedef enum _fseek
{
  file_begin    = 1   // Set position relative to the beginning
 ,file_current  = 2   // Set position relative to the current position
 ,file_end      = 3   // Set at the end (position is ignored)
}
FSeek;

// Flag for copying files
typedef enum _copyFile
{
  copy_no_overwrite     = 0x00001  // Copy fails if the destination does already exist
 ,copy_allow_decryption = 0x00008  // Allow copy even destination directory does not support encryption
 ,copy_allow_symlink    = 0x00800  // If source is a symbolic link, the destination is also a symbolic link
 ,copy_no_buffering     = 0x01000  // Copy the file without using the system I/O cache
 ,copy_open_for_write   = 0x00004  // The source is also open for write operations during the copy
 ,copy_restartable      = 0x00002  // Aborted copies can be restarted (slow!)
}
FCopy;

// Flag for moving files
typedef enum _moveFile
{
  move_replace_existing = 0x001   // Moving file CAN overwrite an existing file
 ,move_copy_allowed     = 0x002   // Copying to another volume is allowed as result of the move
 ,move_delay_reboot     = 0x004   // File will be moved at next reboot before the paging file is made
 ,move_write_through    = 0x008   // Function returns until file is flushed to the file system
 ,move_not_trackable    = 0x020   // File move will fail if file will not be trackable on a FAT(32) system
}
FMove;

// Most used options for copying and moving a file
#define winfile_copy ((FCopy)(FCopy::copy_allow_decryption | FCopy::copy_allow_symlink))
#define winfile_move ((FMove)(FMove::move_replace_existing | FMove::move_copy_allowed))

//////////////////////////////////////////////////////////////////////////
//
// THE CLASS INTERFACE
//
//////////////////////////////////////////////////////////////////////////

class WinFile
{
public:
  // CTOR 
  WinFile();
  // CTOR from a filename
  WinFile(XString p_filename);
  // CTOR from another file pointer
  WinFile(WinFile& p_other);
  // DTOR
  ~WinFile();

  // OPERATIONS ON THE FILE SYSTEM
  bool      Open(DWORD p_flags = winfile_read,FAttributes p_attribs = FAttributes::attrib_none,Encoding p_encoding = Encoding::UTF8);
  bool      Close(bool p_flush = false);
  bool      Create(FAttributes p_attribs = FAttributes::attrib_normal);
  bool      CreateDirectory();
  bool      Exists();
  bool      CanAccess(bool p_write = false);
  bool      DeleteFile();
  unsigned  DeleteDirectory(bool p_recursive = false);
  bool      DeleteToTrashcan(bool p_show = false, bool p_confirm = false);
  bool      CopyFile(XString p_destination,FCopy p_how = winfile_copy);
  bool      MoveFile(XString p_destination,FMove p_how = winfile_move);
  bool      CreateTempFileName(XString p_pattern,XString p_extension = _T(""));
  bool      GrantFullAccess();
  void      ForgetFile(); // BEWARE!

  // OPERATIONS TO READ AND WRITE CONTENT
  bool      Read(XString& p_string,uchar p_delim = '\n');
  bool      Read(void* p_buffer,size_t p_bufsize,int& p_read);
  bool      Write(const XString& p_string);
  bool      Write(void* p_buffer,size_t p_bufsize);
  bool      Format(LPCTSTR p_format,...);
  bool      FormatV(LPCTSTR p_format,va_list p_list);
  size_t    Position() const;
  size_t    Position(FSeek p_how,LONGLONG p_position = 0);
  bool      Flush(bool p_all = false);
  bool      Lock  (size_t p_begin,size_t p_length);
  bool      UnLock(size_t p_begin,size_t p_length);
  bool      UnLockFile();
  // Classic STDIO.H like functions
  bool      Gets(uchar* p_buffer,size_t p_size);
  bool      Puts(uchar* p_buffer);
  int       Getch();
  bool      Putch(uchar p_char);
  bool      Ungetch(uchar p_ch);  // only works in conjunction with "Getch()"

  // SETTERS
  bool      SetFilename(XString p_filename);
  bool      SetFilenameInFolder(int p_folder,XString p_filename);  // Use CSIDL_* names !
  bool      SetFilenameFromResource(XString p_resource);
  bool      SetFileHandle(HANDLE p_handle);
  bool      SetFileAttribute(FAttributes p_attribute,bool p_set);
  bool      SetHidden(bool p_hidden);
  bool      SetArchive(bool p_archive);
  bool      SetSystem(bool p_system);
  bool      SetNormal(bool p_normal);
  bool      SetReadOnly(bool p_readonly);
  bool      SetFileTimeCreated (FILETIME p_created);
  bool      SetFileTimeModified(FILETIME p_modified);
  bool      SetFileTimeAccessed(FILETIME p_accessed);
  bool      SetFileTimeCreated (SYSTEMTIME& p_created);
  bool      SetFileTimeModified(SYSTEMTIME& p_modified);
  bool      SetFileTimeAccessed(SYSTEMTIME& p_accessed);
  void      SetEncoding(Encoding p_encoding);

  // GETTERS
  bool      GetIsOpen() const;
  XString   GetFilename();
  HANDLE    GetFileHandle();
  DWORD     GetOpenFlags();
  int       GetLastError();
  XString   GetLastErrorString();
  size_t    GetFileSize() const;
  bool      GetIsAtEnd() const;
  size_t    GetSharedMemorySize();
  XString   GetNamePercentEncoded();
  XString   GetFilenamePartDirectory();
  XString   GetFilenamePartFilename();
  XString   GetFilenamePartBasename();
  XString   GetFilenamePartExtension();
  XString   GetFilenamePartDrive();
  FILETIME  GetFileTimeCreated();
  FILETIME  GetFileTimeModified();
  FILETIME  GetFileTimeAccessed();
  bool      GetFileTimeCreated (SYSTEMTIME& p_time);
  bool      GetFileTimeModified(SYSTEMTIME& p_time);
  bool      GetFileTimeAccessed(SYSTEMTIME& p_time);
  bool      GetFileAttribute(FAttributes p_attribute);
  bool      GetIsTempFile();
  bool      GetIsSharedMemory();
  bool      GetIsHidden();
  bool      GetIsArchive();
  bool      GetIsSystem();
  bool      GetIsNormal();
  bool      GetIsReadOnly();
  bool      GetIsDirectory();
  Encoding  GetEncoding();
  bool      GetFoundBOM();

  // FUNCTIONS

  // Check for a Byte-Order-Mark (BOM)
  static BOMOpenResult  DefuseBOM(const uchar*  p_pointer     // First gotten string in the file
                                 ,Encoding&      p_type       // Return: type of BOM (if any)
                                 ,unsigned int& p_skip);      // Return: number of chars to skip
  // Check for Unicode UTF-16 in the buffer
  static bool IsTextUnicodeUTF16(const uchar* p_pointer,size_t p_length);
  // Check for Unicode UTF-8 in the buffer
  static bool IsTextUnicodeUTF8 (const uchar* p_pointer,size_t p_length);
  // Getting the filename from a dialog
  bool      SetFilenameByDialog(HWND     p_parent             // Parent HWND (if any)
                               ,bool     p_open               // true = Open/New, false = SaveAs
                               ,XString  p_title              // Title of the dialog
                               ,XString  p_defext   = _T("")  // Default extension
                               ,XString  p_filename = _T("")  // Default first file
                               ,int      p_flags    = 0       // Default flags
                               ,XString  p_filter   = _T("")  // Filter for extensions
                               ,XString  p_direct   = _T(""));// Directory to start in
  // Open file as a shared memory segment
  void*     OpenAsSharedMemory(XString  p_name                // Name of the shared memory segment to open
                              ,bool     p_local     = true    // Standard on your local session, otherwise global
                              ,bool     p_trycreate = false   // Create with m_filename if not exists
                              ,size_t   p_size      = 0);     // Size of memory if we create it
  XString   LegalDirectoryName(XString  p_name,bool p_extensionAllowed = true);
  // Create a file name from an HTTP resource name
  XString   FileNameFromResourceName(XString p_resource);
  // Reduce file path name for RE-BASE of directories, removing \..\ parts
  XString  ReduceDirectoryPath(XString& path);
  // Makes a relative pathname from an absolute one
  bool     MakeRelativePathname(const XString& p_base,const XString& p_absolute,XString& p_relative);

  // OPERATORS

  bool     operator==(const WinFile& p_other);
  bool     operator!=(const WinFile& p_other);
  WinFile& operator= (const WinFile& p_other);

  // STREAMING OPERATORS

  WinFile& operator<<(const TCHAR    p_char);
  WinFile& operator<<(const short    p_num);
  WinFile& operator<<(const int      p_num);
  WinFile& operator<<(const unsigned p_num);
  WinFile& operator<<(const INT64    p_num);
  WinFile& operator<<(const float    p_num);
  WinFile& operator<<(const double   p_num);
  WinFile& operator<<(const LPCTSTR  p_string);
  WinFile& operator<<(const XString& p_string);

  WinFile& operator>>(TCHAR&    p_char);
  WinFile& operator>>(short&    p_num);
  WinFile& operator>>(int&      p_num);
  WinFile& operator>>(unsigned& p_num);
  WinFile& operator>>(INT64&    p_num);
  WinFile& operator>>(float&    p_num);
  WinFile& operator>>(double&   p_num);
  WinFile& operator>>(XString&  p_string);

  // Handy for streaming an end-of-line as output
  // Does *NOT* do a flush as std::endl does!!
  static const TCHAR endl { '\n' };

  // Not thread safe: Must be set for the total process!
  // And can only be set in multiples of 4K up to 1024K
  static bool     SetBufferPageSize(DWORD p_pageSize);
  // Convert a FILETIME to a time_t value
  static time_t   ConvertFileTimeToTimet(FILETIME p_time);
  // Convert a time_t value to a FILETIME
  static FILETIME ConvertTimetToFileTime(time_t p_time);
  // Translating UTF-8 / UTF-16-LE and UTF-16-BE in reading and writing
  XString         TranslateInputBuffer(std::string& p_string);
  std::string     TranslateOutputBuffer(const XString& p_string);

private:
  // PRIVATE OPERATIONS
  void      FilenameParts(XString p_fullpath,XString& p_drive,XString& p_directory,XString& p_filename,XString& p_extension);
  XString   StripFileProtocol  (XString  p_fileref);
  int       ResolveSpecialChars(XString& p_value);
  int       EncodeSpecialChars(XString& p_value);
  XString   GetBaseDirectory   (XString& p_path);
  // Page buffer cache functions
  uchar*    PageBuffer();
  void      PageBufferFree();
  int       PageBufferRead();
  bool      PageBufferReadForeward(bool p_scanBom);
  bool      PageBufferWrite(uchar ch);
  bool      PageBufferFlush();
  bool      PageBufferAdjust(bool p_beginning = false) const;
  // Resolving UTF-8 and UTF-16 text files
  void      ScanBomInFirstPageBuffer();
  bool      WriteEncodingBOM();
  // Convert Big-Endian (Blefuscu) to Little-Endian (Lilliput)
  void        BlefuscuToLilliput(std::string& p_gulliver);
#ifdef _UNICODE
  XString     ExplodeString(const std::string& p_string,unsigned p_codepage);
  std::string ImplodeString(const     XString& p_string,unsigned p_codepage);
#endif
  // PRIVATE DATA

  // The file and open actions
  XString     m_filename;                           // Name of the file (if any)
  HANDLE      m_file         { nullptr };           // Handle to the OS file
  DWORD       m_openMode     { FFlag::no_mode   };  // How the file was opened
  Encoding    m_encoding     { Encoding::EN_ACP };  // Encoding found by BOM
  bool        m_foundBOM     { false };             // Found a BOM when opening file
  // Page buffer cache
  uchar*      m_pageBuffer   { nullptr };           // PB: Text mode page buffer
  uchar*      m_pagePointer  { nullptr };           // PP: Pointer in the page buffer
  uchar*      m_pageTop      { nullptr };           // PT: Pointer to the last position in the buffer
  // Current information
  mutable int m_error        { 0 };                 // Last encountered error (if any)
  uchar       m_ungetch      { 0 };                 // Last get character with "Getch"
  // Shared Memory
  void*       m_sharedMemory { nullptr };           // Pointer in memory (if any)
  size_t      m_sharedSize   { 0 };                 // Size of shared memory segment
  // One thread at the time!
  mutable CRITICAL_SECTION m_fileaccess;
};

//////////////////////////////////////////////////////////////////////////
//
// DOCUMENTATION
// =============
// Reading and writing uses the page buffer in a different way seen from
// the perspective of the current-file-position as seen by the OS.
//
// In case of a sequential read-from file
// optimized for a fast-read-forward
//
// BOF                   PB                 PB+PAGESIZE              EOF
// |                     |                  |                        |
// +---------------------+==================+------------------------+
//                                  ^       ^
//                                  |       |
//                                  PP      PT,FPOS
//
// BOF             = BEGIN-OF-FILE
// PB              = m_pageBuffer   : Beginning of memory buffer
// PP              = m_pagePointer  : Current read position
// PT              = m_pageTop      : End of valid buffer. if last page, maximum = EOF
// PB + PAGESIZE   = maximum buffer size
// FPOS            = Current File POSition according to the OS
// EOF             = END-OF-FILE
//
//
// In case of sequential write-to-file (optimized fast write-forward)
//         OR random-access with read-and-write (resets the file position and is slower)
//
// BOF                   PB                 PB+PAGESIZE              EOF
// |                     |                  |                        |
// +---------------------+==================+------------------------+
//                       ^        ^         ^ 
//                       |        |         |
//                    FPOS        PP        PT
//
// BOF             = BEGIN-OF-FILE
// PB              = m_pageBuffer     : Beginning of memory buffer
// PP              = m_pagePointer    : Current read/write position
// PT              = m_pageTop        : End of valid buffer. If last page, maximum = EOF
// PB + PAGESIZE   = maximum buffer size
// FPOS            = Current File POSition according to the OS
// EOF             = END-OF-FILE
//
