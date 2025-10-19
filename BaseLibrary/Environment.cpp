/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: Environment.cpp
//
// Created: 2014-2025 ir. W.E. Huisman
// MIT License
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
#include "Environment.h"

#define BUFFER_LINE 4096

// Getting the <modulename>.env file
// Name of the file must be the same as the executable
//
XString GetEnvironmentFile()
{
  TCHAR buffer[_MAX_PATH];

  GetModuleFileName(GetModuleHandle(NULL), buffer, _MAX_PATH);
  XString loadModule(buffer);
  if(loadModule.Right(4).CompareNoCase(_T(".exe")) == 0)
  {
    loadModule = loadModule.Left(loadModule.GetLength() - 4);
    loadModule += _T(".env");
    return loadModule;
  }
  return _T("");
}

// Get the general file for the whole directory
//
XString GetGeneralFile(XString p_file)
{
  int pos = p_file.ReverseFind('\\');
  if(pos >= 0)
  {
    return p_file.Left(pos + 1) + _T("Environment.env");
  }
  return _T("");
}

// Remove newline characters from the line buffer
//
void RemoveNewline(XString& buffer)
{
  for(int len = 0;len < buffer.GetLength();++len)
  {
    TCHAR ch = (TCHAR) buffer.GetAt(len);
    if(ch == '\n' || ch == '\r')
    {
      buffer.Truncate(len);
      return;
    }
  }
}

// Skip lines that begin with a comment character
//
bool SkipComment(XString& p_buffer)
{
  if(p_buffer.GetAt(0) == _T(';') || p_buffer.GetAt(0) == _T('#'))
  {
    return true;
  }
  return false;
}

// variable = value   ->   variable=value
// variable + value   ->   variable=<oldvalue>value
// variable < value   ->   variable=value<oldvalue>
// variable > value   ->   variable=<oldvalue>value
// variable !         ->   Delete variable from the environment
//
void ProcessVariable(XString& p_string)
{
  // Finding the delimiter character (operator)
  int pos = p_string.FindOneOf(_T("+=<>!"));
  if(pos < 0)
  {
    return;
  }
  // Finding the variable and the value
  XString variable = p_string.Left(pos);
  XString value    = p_string.Mid(pos + 1);
  variable = variable.Trim();
  value    = value.Trim();

  // Get size of the variable value
  PTCHAR envvar = nullptr;
  int size = GetEnvironmentVariable(variable,envvar,0);
  envvar = alloc_new TCHAR[size+1];
  GetEnvironmentVariable(variable,envvar,size);

  // Perform the operator calculation
  switch(p_string.GetAt(pos))
  {
    case '=': break;
    case '!': value.Empty(); 
              break;
    case '+': // fall through
    case '>': value = XString(envvar) + value; 
              break;
    case '<': value = value  + envvar;
              break;
  }

  // Setting the variable to the new value
  SetEnvironmentVariable(variable,value);

  delete [] envvar;
}

// Process the environment file for the executable
//
void DoProcessEnvironment()
{
  // Find name for this executable first
  XString filename = GetEnvironmentFile();
  WinFile file(filename);

  if(!file.Open(winfile_read))
  {
    file.SetFilename(GetGeneralFile(filename));
    file.Open(winfile_read);

  }
  // Process if found
  if(file.GetIsOpen())
  {
    XString line;
    while(file.Read(line))
    {
      RemoveNewline(line);
      if(SkipComment(line))
      {
        continue;
      }
      ProcessVariable(line);
    }
    file.Close();
  }
}
