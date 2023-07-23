/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: Environment.cpp
//
// Copyright (c) 2015 ir. W.E. Huisman
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
#include "Environment.h"

#define BUFFER_LINE 4096

// Getting the <modulename>.env file
// Name of the file must be the same as the executable
//
XString GetEnvironmentFile()
{
  char buffer[_MAX_PATH];

  GetModuleFileName(GetModuleHandle(NULL), buffer, _MAX_PATH);
  XString module(buffer);
  if(module.Right(4).CompareNoCase(".exe") == 0)
  {
    module = module.Left(module.GetLength() - 4);
    module += ".env";
    return module;
  }
  return "";
}

// Get the general file for the whole directory
//
XString GetGeneralFile(XString p_file)
{
  int pos = p_file.ReverseFind('\\');
  if(pos >= 0)
  {
    return p_file.Left(pos + 1) + "Environment.env";
  }
  return "";
}

// Remove newline characters from the line buffer
//
void RemoveNewline(char* buffer)
{
  int len = (int) strlen(buffer);
  while(len-- > 0)
  {
    if(buffer[len] == '\n' || buffer[len] == '\r')
    {
      buffer[len] = 0;
    }
  }
}

// Skip lines that begin with a comment character
//
bool SkipComment(const char* buffer)
{
  if(buffer[0] == ';' || buffer[0] == '#')
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
void ProcessVariable(char* buffer)
{
  // Finding the delimiter character (operator)
  int pos = (int) strcspn(buffer,"+=<>!");
  if(pos == (int) strlen(buffer))
  {
    return;
  }
  // Finding the variable and the value
  XString line(buffer);
  XString variable = line.Left(pos);
  XString value    = line.Mid(pos + 1);
  variable = variable.Trim();
  value    = value.Trim();

  // Get size of the variable value
  char* envvar = nullptr;
  int size = GetEnvironmentVariable(variable,envvar,0);
  envvar = new char[size+1];
  GetEnvironmentVariable(variable,envvar,size);

  // Perform the operator calculation
  switch(buffer[pos])
  {
    case '=': break;
    case '!': value.Empty(); 
              break;
    case '+': // fall through
    case '>': value = envvar + value; 
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
  FILE*   file = nullptr;
  fopen_s(&file,filename,"r");

  if(file == nullptr)
  {
    // fallback to general "environment.env" file
    filename = GetGeneralFile(filename);
    fopen_s(&file,filename,"r");
  }

  // Process if found
  if(file)
  {
    char buffer[BUFFER_LINE + 1];
    while(fgets(buffer,BUFFER_LINE,file))
    {
      RemoveNewline(buffer);
      if(SkipComment(buffer))
      {
        continue;
      }
      ProcessVariable(buffer);
    }
    fclose(file);
  }
}
