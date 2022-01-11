/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: EnsureFile.h
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
#pragma once

class EnsureFile
{
public:
  EnsureFile();
  EnsureFile(CString p_filename);
 ~EnsureFile();

  // Re-setting the filename
  void     SetFilename(CString p_filename);
  // Setting a resource name (from HTTP)
  void     SetResourceName(CString p_resource);
  // Create/Open file
  int      OpenFile(FILE** p_file,char* p_mode);
  // Create/check existence of a directory
  int      CheckCreateDirectory();
  // Grant full access on file/directory
  bool     GrantFullAccess();

  // Special optimized function to resolve %5C -> '\' in pathnames
  int      ResolveSpecialChars(CString& p_value);
  // Encode a filename in special characters
  int      EncodeSpecialChars(CString& p_value);
  // Create a file name from an HTTP resource name
  CString  FileNameFromResourceName(CString p_resource);
  // Reduce file path name for RE-BASE of directories, removing \..\ parts
  CString  ReduceDirectoryPath(CString& path);
  // Makes a relative pathname from an absolute one
  bool     MakeRelativePathname(CString& p_base,CString& p_absolute,CString& p_relative);
  // Generic strip protocol from URL to form OS filenames
  CString  StripFileProtocol(CString fileref);
  // Generic find case-insensitive
  int      FindNoCase(CString line,CString part,int pos = 0);
  // Getting the first (base) directory
  CString  GetBaseDirectory(CString& p_path);

  // GETTING PARTS OF THE FILENAME
  CString  GetFilename();
  CString  FilenamePart  (CString fullpath);
  CString  ExtensionPart (CString fullpath);
  CString  DirectoryPart (CString fullpath);
  CString  RemoveBasePart(CString base,CString fullpath);
  void     FilenameParts (CString fullpath,CString& p_drive,CString& p_directory,CString& p_file,CString& p_extension);

private:
  CString m_filename;
};

inline void
EnsureFile::SetFilename(CString p_filename)
{
  m_filename = p_filename;
}

inline CString
EnsureFile::GetFilename()
{
  return m_filename;
}
