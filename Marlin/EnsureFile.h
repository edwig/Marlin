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
  EnsureFile(XString p_filename);
 ~EnsureFile();

  // Re-setting the filename
  void     SetFilename(XString p_filename);
  // Setting a resource name (from HTTP)
  void     SetResourceName(XString p_resource);
  // Create/Open file
  int      OpenFile(FILE** p_file,char* p_mode);
  // Create/check existence of a directory
  int      CheckCreateDirectory();
  // Grant full access on file/directory
  bool     GrantFullAccess();

  // Special optimized function to resolve %5C -> '\' in pathnames
  int      ResolveSpecialChars(XString& p_value);
  // Encode a filename in special characters
  int      EncodeSpecialChars(XString& p_value);
  // Create a file name from an HTTP resource name
  XString  FileNameFromResourceName(XString p_resource);
  // Reduce file path name for RE-BASE of directories, removing \..\ parts
  XString  ReduceDirectoryPath(XString& path);
  // Makes a relative pathname from an absolute one
  bool     MakeRelativePathname(XString& p_base,XString& p_absolute,XString& p_relative);
  // Generic strip protocol from URL to form OS filenames
  XString  StripFileProtocol(XString fileref);
  // Generic find case-insensitive
  int      FindNoCase(XString line,XString part,int pos = 0);
  // Getting the first (base) directory
  XString  GetBaseDirectory(XString& p_path);

  // GETTING PARTS OF THE FILENAME
  XString  GetFilename();
  XString  FilenamePart  (XString fullpath);
  XString  ExtensionPart (XString fullpath);
  XString  DirectoryPart (XString fullpath);
  XString  RemoveBasePart(XString base,XString fullpath);
  void     FilenameParts (XString fullpath,XString& p_drive,XString& p_directory,XString& p_file,XString& p_extension);

private:
  XString m_filename;
};

inline void
EnsureFile::SetFilename(XString p_filename)
{
  m_filename = p_filename;
}

inline XString
EnsureFile::GetFilename()
{
  return m_filename;
}
