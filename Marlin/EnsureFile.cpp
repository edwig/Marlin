/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: EnsureFile.cpp
//
// Marlin Server: Internet server/client
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
#include "Stdafx.h"
#include "EnsureFile.h"
#include "CrackURL.h"
#include <AclAPI.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

EnsureFile::EnsureFile()
{
}

EnsureFile::EnsureFile(XString p_filename)
           :m_filename(p_filename)
{
}

EnsureFile::~EnsureFile()
{
}

// Setting a resource name (from HTTP)
void     
EnsureFile::SetResourceName(XString p_resource)
{
  m_filename = FileNameFromResourceName(p_resource);
}

// Create/Open file
int 
EnsureFile::OpenFile(FILE** p_file,char* p_mode)
{
  // Make sure we have a filename
  if(m_filename.IsEmpty())
  {
    return ERROR_NOT_FOUND;
  }
  // Reset file pointer
  *p_file = nullptr;

  // Split into parts
  XString drive,directory,filename,extension;
  FilenameParts(m_filename,drive,directory,filename,extension);

  XString dirToBeOpened(drive);

  XString path(directory);
  XString part = GetBaseDirectory(path);
  while(part.GetLength())
  {
    dirToBeOpened += "\\" + part;
    if(!CreateDirectory(dirToBeOpened,NULL))
    {
      DWORD error = GetLastError();
      if(error != ERROR_ALREADY_EXISTS)
      {
        // Cannot create the directory
        return error;
      }
    }
    part = GetBaseDirectory(path);
  }
  // Directory now ensured
  XString pathToBeOpened = dirToBeOpened + "\\"  + filename + extension;

  return fopen_s(p_file,pathToBeOpened,p_mode);
}

// Create/check existence of a directory
int      
EnsureFile::CheckCreateDirectory()
{
  // Make sure we have a filename
  if(m_filename.IsEmpty())
  {
    return ERROR_NOT_FOUND;
  }
  XString drive,directory,filename,extension;
  FilenameParts(m_filename,drive,directory,filename,extension);

  XString dirToBeOpened(drive);

  XString path(directory);
  XString part = GetBaseDirectory(path);
  while(part.GetLength())
  {
    dirToBeOpened += "\\" + part;
    if(!CreateDirectory(dirToBeOpened,NULL))
    {
      DWORD error = GetLastError();
      if(error != ERROR_ALREADY_EXISTS)
      {
        // Cannot create the directory
        return error;
      }
    }
    part = GetBaseDirectory(path);
  }
  return 0;
}

// Special optimized function to resolve %5C -> '\' in pathnames
// Returns number of chars replaced
int
EnsureFile::ResolveSpecialChars(XString& p_value)
{
  int total = 0;

  int pos = p_value.Find('%');
  while(pos >= 0)
  {
    ++total;
    int num = 0;
    XString hexstring = p_value.Mid(pos + 1,2);
    hexstring.MakeUpper();
    if(isdigit(hexstring.GetAt(0)))
    {
      num = hexstring.GetAt(0) - '0';
    }
    else
    {
      num = hexstring.GetAt(0) - 'A' + 10;
    }
    num *= 16;
    if(isdigit(hexstring.GetAt(1)))
    {
      num += hexstring.GetAt(1) - '0';
    }
    else
    {
      num += hexstring.GetAt(1) - 'A' + 10;
    }
    p_value.SetAt(pos,(char)num);
    p_value = p_value.Left(pos + 1) + p_value.Mid(pos + 3);
    pos = p_value.Find('%');
  }
  return total;
}

// Encode a filename in special characters
int
EnsureFile::EncodeSpecialChars(XString& p_value)
{
  int total = 0;

  int pos = 0;
  while (pos < p_value.GetLength())
  {
    int ch = p_value.GetAt(pos);
    if(!isalnum(ch) && ch != '/')
    {
      // Encoding in 2 chars HEX
      XString hexstring;
      hexstring.Format("%%%2.2X",ch);

      // Take left and right of the special char
      XString left  = p_value.Left(pos);
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

// Create a file name from an HTTP resource name
XString
EnsureFile::FileNameFromResourceName(XString p_resource)
{
  XString filename = CrackedURL::DecodeURLChars(p_resource);
  filename.Replace('/','\\');
  filename.Replace("\\\\","\\");
  return filename;
}

// Reduce file path name for RE-BASE of directories
// IN:  C:\direct1\direct2\direct3\..\..\direct4 
// OUT: C:\direct1\direct4
XString
EnsureFile::ReduceDirectoryPath(XString& path)
{
  char buffer[_MAX_PATH+1] = "";
  strncpy_s(buffer,path.GetString(),_MAX_PATH);
  bool foundReduction = true;

  while(foundReduction)
  {
    // Drop out if we find nothing;
    foundReduction = false;

    char* pnt1 = buffer;
    char* pnt2 = pnt1;
    char* pnt3 = pnt1;

    while(*pnt1 && *pnt1!='\\' && *pnt1!='/') ++pnt1;
    if(!*pnt1++)
    {
      // Not one directory separator
      return path;
    }
    pnt3 = pnt1;
    while(*pnt1 && *pnt1!='\\' && *pnt1!='/') ++pnt1;
    if(!*pnt1++)
    {
      // Not a second directory separator
      return path;
    }
    pnt2 = pnt1;
    while(*pnt1 && *pnt1!='\\' && *pnt1!='/') ++pnt1;
    while(*pnt1)
    {
      ++pnt1;
      // IN:  C:\direct1\direct2\direct3\..\..\direct4\
      //         |       |       |
      //      pnt3    pnt2    pnt1
      if(strncmp(pnt2,"..\\",3)==0 || strncmp(pnt2,"../",3)==0)
      {
        // Space between pnt2 and pnt1 = \..\
        // IN:  C:\direct1\direct2\direct3\..\..\direct4\
        //                         |       |  |
        //                      pnt3    pnt2  pnt1

        // REDUCTION
        size_t len = _MAX_PATH - (pnt3 - buffer);
        strcpy_s(pnt3,len,pnt1);

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
EnsureFile::MakeRelativePathname(XString& p_base
                                ,XString& p_absolute
                                ,XString& p_relative)
{
  p_relative = "";
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
  // Make all directory seperators the same
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
    absolute = XString("../") + absolute;
    base = base.Mid(pos + 1);
    pos  = base.Find('/');
  }
  p_relative = absolute;

  // Relative path is complete.
  // Now warn for files outside the project
  if(p_relative.GetLength() > 1)
  {
    if(p_relative.Left(2) != "..")
    {
      // Did not succeed to make a relative path
      return false;
    }
  }
  return true;
}

// Generic strip protocol from URL to form OS filenames
// "file:///c|/Program%20Files/Program%23name/file%25name.exe" =>
// "c:\Program Files\Program#name\file%name.exe"
XString
EnsureFile::StripFileProtocol(XString fileref)
{
  if(fileref.GetLength() > 8)
  {
    if(fileref.Left(8).CompareNoCase("file:///") == 0)
    {
      fileref = fileref.Mid(8);
    }
  }
  // Create a filename separator char name
  fileref.Replace('/','\\');
  fileref.Replace('|',':');
  // Resolve the '%' chars in the filename
  ResolveSpecialChars(fileref);
  return fileref;
}

XString
EnsureFile::FilenamePart(XString fullpath)
{
  char drive [_MAX_DRIVE + 1];
  char direct[_MAX_DIR   + 1];
  char fname [_MAX_FNAME + 1];
  char extens[_MAX_EXT   + 1];

  fullpath = StripFileProtocol(fullpath);
  _splitpath_s(fullpath.GetString(),drive,direct,fname,extens);
  XString filename = XString(fname) + XString(extens);
  return filename;
}

XString
EnsureFile::ExtensionPart(XString fullpath)
{
  char drive [_MAX_DRIVE + 1];
  char direct[_MAX_DIR   + 1];
  char fname [_MAX_FNAME + 1];
  char extens[_MAX_EXT   + 1];

  fullpath = StripFileProtocol(fullpath);
  _splitpath_s(fullpath.GetString(),drive,direct,fname,extens);
  return XString(extens);
}

XString
EnsureFile::DirectoryPart(XString fullpath)
{
  char drive [_MAX_DRIVE + 1];
  char direct[_MAX_DIR   + 1];
  char fname [_MAX_FNAME + 1];
  char extens[_MAX_EXT   + 1];

  fullpath = StripFileProtocol(fullpath);
  _splitpath_s(fullpath.GetString(),drive,direct,fname,extens);
  XString directory = XString(drive) + XString(direct);
  return directory;
}

XString
EnsureFile::RemoveBasePart(XString base,XString fullpath)
{
  fullpath = StripFileProtocol(fullpath);
  fullpath.Replace('/','\\');
  if(FindNoCase(fullpath,base,0) == 0)
  {
    return fullpath.Mid(base.GetLength());
  }
  return fullpath;
}

// Generic find case-insensitive
int
EnsureFile::FindNoCase(XString line,XString part,int pos/*=0*/)
{
  line.MakeLower();
  part.MakeLower();
  return line.Find(part,pos);
}
// Getting the first (base) directory
XString  
EnsureFile::GetBaseDirectory(XString& p_path)
{
  XString result;

  // Strip of an extra path separator
  while(p_path.GetAt(0) == '\\')
  {
    p_path = p_path.Mid(1);
  }

  // Find first separator
  int pos = p_path.Find('\\');
  if(pos >= 0)
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

void     
EnsureFile::FilenameParts(XString fullpath,XString& p_drive,XString& p_directory,XString& p_filename,XString& p_extension)
{
  char drive [_MAX_DRIVE + 1];
  char direct[_MAX_DIR   + 1];
  char fname [_MAX_FNAME + 1];
  char extens[_MAX_EXT   + 1];

  fullpath = StripFileProtocol(fullpath);
  _splitpath_s(fullpath.GetString(),drive,direct,fname,extens);
  p_drive     = XString(drive);
  p_directory = XString(direct);
  p_filename  = XString(fname);
  p_extension = XString(extens);
}

// Grant full access on file or directory
bool
EnsureFile::GrantFullAccess()
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
    ZeroMemory(&ea, sizeof(EXPLICIT_ACCESS));
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
    if(ERROR_SUCCESS != SetNamedSecurityInfo((LPSTR)m_filename.GetString() // name of the object
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
