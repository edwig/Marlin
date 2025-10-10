/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: StringUtilities.cpp
//
// BaseLibrary: Indispensable general objects and functions
// 
// Copyright (c) 2014-2025 ir. W.E. Huisman
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
#include "StringUtilities.h"

#ifdef _AFX
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#endif

XString AsString(int p_number,int p_radix /*=10*/)
{
  XString string;
  PTCHAR buffer = string.GetBufferSetLength(12);
  _itot_s(p_number,buffer,12,p_radix);
  string.ReleaseBuffer();
  
  return string;
}

XString AsString(double p_number)
{
  XString string;
  string.Format(_T("%f"),p_number);
  return string;
}

int AsInteger(XString p_string)
{
  return _ttoi(p_string.GetString());
}

double  AsDouble(XString p_string)
{
  return _ttof(p_string.GetString());
}

bcd AsBcd(XString p_string)
{
  bcd num(p_string.GetString());
  return num;
}

int SplitString(XString p_string,std::vector<XString>& p_vector,TCHAR p_separator /*= ','*/,bool p_trim /*=false*/)
{
  p_vector.clear();
  int pos = p_string.Find(p_separator);
  while(pos >= 0)
  {
    XString first = p_string.Left(pos);
    p_string = p_string.Mid(pos + 1);
    if(p_trim)
    {
      first = first.Trim();
    }
    p_vector.push_back(first);
    // Find next string
    pos = p_string.Find(p_separator);
  }
  if(p_trim)
  {
    p_string = p_string.Trim();
  }
  if(p_string.GetLength())
  {
    p_vector.push_back(p_string);
  }
  return (int)p_vector.size();
}

void NormalizeLineEndings(XString& p_string)
{
#define RETURNSTR   _T("\r")
#define LINEFEEDSTR _T("\n")

#ifdef _UNICODE
  std::wstring tempStr(p_string.GetString());
#else
  std::string tempStr(p_string.GetString());
#endif

  int lfpos = -1;
  while(true)
  {
    int crpos = (int)tempStr.find(RETURNSTR,++lfpos);
        lfpos = (int)tempStr.find(LINEFEEDSTR,lfpos);

    if(crpos >= 0)
    {
      if(lfpos == crpos + 1) //CRLF
      {
        continue;
      }
      else if(crpos < lfpos) //CR
      {
        lfpos = ++crpos;
        tempStr.insert(lfpos,LINEFEEDSTR);
        continue;
      }
    }
    if(lfpos >= 0)           //LF
    {
      tempStr.insert(lfpos++,RETURNSTR);
      continue;
    }
    break;
  }
  p_string = tempStr.c_str();
}

// Find the position of the matching bracket
// starting at the bracket in the parameters bracketPos
//
int FindMatchingBracket(const XString& p_string,int p_bracketPos)
{
  TCHAR bracket = p_string[p_bracketPos];
  TCHAR   match = 0;
  bool  reverse = false;

  switch(bracket)
  {
    case '(':   match = ')';    break;
    case '{':   match = '}';    break;
    case '[':   match = ']';    break;
    case '<':   match = '>';    break;
    case ')':   reverse = true;
                 match = '(';
                break;
    case '}':   reverse = true;
                match = '{';
                break;
    case ']':   reverse = true;
                match = '[';
                break;
    case '>':   reverse = true;
                match = '<';
                break;
    default:    // Nothing to match
                return -1;
  }

  if(reverse)
  {
    for(int pos = p_bracketPos - 1,nest = 1; pos >= 0; --pos)
    {
      TCHAR c = p_string[pos];
      if(c == bracket)
      {
        ++nest;
      }
      else if(c == match)
      {
        if(--nest == 0)
        {
          return pos;
        }
      }
    }
  }
  else
  {
    for(int pos = p_bracketPos + 1,nest = 1,len = p_string.GetLength(); pos < len; ++pos)
    {
      TCHAR c = p_string[pos];
      if(c == bracket)
      {
        ++nest;
      }
      else if(c == match)
      {
        if(--nest == 0)
        {
          return pos;
        }
      }
    }
  }
  return -1;
}

// Split arguments with p_splitter not within brackets
// p_pos must be 0 initially
bool SplitArgument(int& p_pos,const XString& p_data,TCHAR p_splitter,XString& p_argument)
{
  int len = p_data.GetLength();
  if(p_pos >= len)
  {
    return false;
  }
  for(int pos = p_pos,nest = 0; pos < len; ++pos)
  {
    switch(TCHAR c = p_data[pos])
    {
      case '(': ++nest;
                break;
      case ')': if(--nest < 0)
                {
                  pos = len;
                }
                break;
      default:  if(nest == 0 && c == p_splitter)
                {
                  p_argument = p_data.Mid(p_pos,pos - p_pos).Trim();
                  p_pos = pos + 1;
                  return true;
                }
    }
  }
  p_argument = p_data.Mid(p_pos).Trim();
  p_pos = len;
  return true;
}

XString GetStringFromClipboard(HWND p_wnd /*=NULL*/)
{
#ifdef _UNICODE
  UINT format = CF_UNICODETEXT;
#else
  UINT format = CF_TEXT;
#endif

  XString string;
  if(OpenClipboard(p_wnd))
  {
    HANDLE glob = GetClipboardData(format);
    if(glob)
    {
      LPCTSTR text = (LPCTSTR) GlobalLock(glob);
      string = text;
      GlobalUnlock(glob);

    }
    CloseClipboard();
  }
  return string;
}

bool PutStringToClipboard(XString p_string,HWND p_wnd /*=NULL*/,bool p_append /*=false*/)
{
  bool result = false;
#ifdef _UNICODE
  UINT format = CF_UNICODETEXT;
#else
  UINT format = CF_TEXT;
#endif

  if(OpenClipboard(p_wnd))
  {
    // Put the text in a global GMEM_MOVABLE memory handle
    size_t size = ((size_t) p_string.GetLength() + 1) * sizeof(TCHAR);
    HGLOBAL memory = GlobalAlloc(GHND | GMEM_MOVEABLE | GMEM_ZEROINIT,size);
    if(memory)
    {
      void* data = GlobalLock(memory);
      if(data)
      {
        size /= sizeof(TCHAR);
        _tcsncpy_s((LPTSTR)data,size,(LPCTSTR)p_string.GetString(),size);
      }
      else
      {
        GlobalFree(memory);
        return false;
      }

      // Set the text on the clipboard
      // and transfer ownership of the memory segment
      if(!p_append)
      {
        EmptyClipboard();
      }
      SetClipboardData(format,memory);
      GlobalUnlock(memory);
      result = true;
    }
    CloseClipboard();
  }
  return result;
}


// Count the number of characters in a string
int
CountOfChars(XString p_string,TCHAR p_char)
{
  int count = 0;
  for(int index = 0;index < p_string.GetLength();++index)
  {
    if(p_string.GetAt(index) == p_char)
    {
      ++count;
    }
  }
  return count;
}
