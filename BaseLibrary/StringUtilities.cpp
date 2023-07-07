/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: StringUtilities.cpp
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
#include "StringUtilities.h"

XString AsString(int p_number,int p_radix /*=10*/)
{
  XString string;
  char* buffer = string.GetBufferSetLength(12);
  _itoa_s(p_number,buffer,12,p_radix);
  string.ReleaseBuffer();
  
  return string;
}

XString AsString(double p_number)
{
  XString string;
  string.Format("%f",p_number);
  return string;
}

int AsInteger(XString p_string)
{
  return atoi(p_string.GetBuffer());
}

double  AsDouble(XString p_string)
{
  return atof(p_string.GetBuffer());
}

bcd AsBcd(XString p_string)
{
  bcd num(p_string);
  return num;
}

int SplitString(XString p_string,std::vector<XString>& p_vector,char p_separator /*= ','*/,bool p_trim /*=false*/)
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
#define RETURNSTR "\r"
#define LINEFEEDSTR "\n"

  std::string tempStr(p_string.GetString());

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
int FindMatchingBracket(const CString& p_string,int p_bracketPos)
{
  char bracket = p_string[p_bracketPos];
  char match = char(0);
  bool reverse = false;

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
      char c = p_string[pos];
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
      char c = p_string[pos];
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
bool SplitArgument(int& p_pos,const CString& p_data,char p_splitter,CString& p_argument)
{
  int len = p_data.GetLength();
  if(p_pos >= len)
  {
    return false;
  }
  for(int pos = p_pos,nest = 0; pos < len; ++pos)
  {
    switch(char c = p_data[pos])
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
