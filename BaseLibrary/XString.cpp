//////////////////////////////////////////////////////////////////////////
//
// XString
//
// Std Mfc eXtension (SMX)String is a string derived from std::string
// But does just about everything that MFC CString also does
// The string is derived from std::string with CString methods
// SMX = std::string with MFC eXtensions
//
// Created: 2014-2025 ir. W.E. Huisman
// MIT License
//
// Permission is hereby granted, free of charge, to any person obtaining a 
// copy of this software and associated documentation files(the "Software"), 
// to deal in the Software without restriction, including without limitation 
// the rights to use, copy, modify, merge, publish, distribute, sublicense, 
// and /or sell copies of the Software, and to permit persons to whom the 
// Software is furnished to do so, subject to the following conditions :
//
// The above copyright notice and this permission notice shall be included 
// in all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, 
// INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A 
// PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT 
// HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION 
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH 
// THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
//////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "BaseLibrary.h"
#include "ConvertWideString.h"
#include "XString.h"
#include <atlbase.h>
#include <WinNls.h>

// Base CTOR
XString::XString()
{
}

// CTOR from character pointer
XString::XString(LPCTSTR p_string)
        :stdstring(p_string)
{
}

// CTOR from a number of characters
XString::XString(TCHAR p_char,int p_count /* = 1*/)
{
  append(p_count,p_char);
}

// CTOR from other string
XString::XString(const XString& p_string)
{
  append(p_string);
}

// CTOR from other string
XString::XString(const stdstring& p_string)
{
  append(p_string);
}

#ifdef _UNICODE

// CTOR form a wchar_t stream
XString::XString(wchar_t* p_string)
{
  append(p_string);
}

// CTOR from a ANSI string
XString::XString(PCSTR p_string)
{
  int len = MultiByteToWideChar(CP_ACP,0,p_string,(int)strlen(p_string),nullptr,0);
  wchar_t* buffer = alloc_new wchar_t[len + 1];
  MultiByteToWideChar(CP_ACP,0,p_string,len,buffer,len);
  buffer[len] = 0;
  append(buffer);
  delete [] buffer;
}
#else

// CTOR from unsigned char stream
XString::XString(unsigned char* p_string)
{
  append((const char*)p_string);
}

// CTOR from Unicode string
XString::XString(PCWSTR p_string)
{
  int strlen = (int)wcslen(p_string);
  int len = WideCharToMultiByte(CP_ACP,0,p_string,strlen,nullptr,0,nullptr,nullptr);
  char* buffer = alloc_new char[len + 1];
  WideCharToMultiByte(CP_ACP,0,p_string,strlen,buffer,len,0,nullptr);
  buffer[len] = 0;
  append(buffer);
  delete [] buffer;
}

#endif

// Returns a BSTR constructed from the current object
BSTR 
XString::AllocSysString()
{
#ifdef _UNICODE
  wstring str = *this;
#else
  wstring str = StringToWString(this->c_str());
#endif
  BSTR bstrResult = ::SysAllocString(str.c_str());

  if(bstrResult == NULL)
  {
    throw std::bad_alloc();
  }
  return(bstrResult);
}

void 
XString::AnsiToOem()
{
#ifndef _UNICODE
  // Only works for MBCS, not for Unicode
  ::CharToOemBuff((LPCSTR)(c_str()),(LPSTR)(c_str()),static_cast<DWORD>(length()));
#endif
}

// Append a string, or n chars from a string
//
void 
XString::Append(LPCTSTR p_string,int p_length)
{
  stdstring str(p_string);
  append(str.substr(0,p_length));
}

// Append a formatted string
void 
XString::AppendFormat(LPCTSTR p_format,...)
{
  va_list argList;
  va_start(argList,p_format);

  AppendFormatV(p_format,argList);

  va_end(argList);
}

void 
XString::AppendFormat(UINT p_strID,...)
{
  va_list argList;
  va_start(argList,p_strID);

  AppendFormatV(p_strID,argList);

  va_end(argList);
}

void 
XString::AppendFormatV(LPCTSTR p_format,va_list p_list)
{
  // Getting a buffer of the correct length
  int len = _vsctprintf(p_format,p_list) + 1;
  TCHAR* buffer = alloc_new TCHAR[len];
  // Formatting the parameters
  _vstprintf_s(buffer,len,p_format,p_list);
  // Adding to the string
  append(buffer);

  delete[] buffer;
}

void 
XString::AppendFormatV(UINT p_strID,va_list p_list)
{
  // Getting the string
  XString str;
  if(str.LoadString(p_strID))
  {
    // Getting a buffer of the correct length
    int len = _vsctprintf(str.c_str(),p_list) + 1;
    TCHAR* buffer = alloc_new TCHAR[len];
    // Formatting the parameters
    _vstprintf_s(buffer,len,str.c_str(),p_list);
    // Adding to the string
    append(buffer);

    delete[] buffer;
  }
}

// Delete, returning new length
int 
XString::Delete(int p_index,int p_count)
{
  erase(p_index,p_count);
  return (int) length();
}

// Format a string
void 
XString::Format(LPCTSTR p_format,...)
{
  va_list argList;
  va_start(argList,p_format);

  FormatV(p_format,argList);

  va_end(argList);
}

void 
XString::Format(UINT p_strID,...)
{
  va_list argList;
  va_start(argList,p_strID);

  FormatV(p_strID,argList);

  va_end(argList);
}

#pragma warning(push)
#pragma warning(disable:4840)
void
XString::Format(XString p_format,...)
{
  va_list argList;
  va_start(argList,p_format);

  FormatV(p_format.c_str(),argList);

  va_end(argList);
}
#pragma warning(pop)

// Format a variable list
void 
XString::FormatV(LPCTSTR p_format,va_list p_list)
{
  // Getting a buffer of the correct length
  int len = _vsctprintf(p_format,p_list) + 1;
  TCHAR* buffer = alloc_new TCHAR[len];
  // Formatting the parameters
  _vstprintf_s(buffer,len,p_format,p_list);
  // Adding to the string
  *this = (LPCTSTR)buffer;

  delete[] buffer;
}

void 
XString::FormatV(UINT p_strID,va_list p_list)
{
  // Getting the string
  XString str;
  if(str.LoadString(p_strID))
  {
    // Getting a buffer of the correct length
    int len = _vsctprintf(str.c_str(),p_list) + 1;
    TCHAR* buffer = alloc_new TCHAR[len];
    // Formatting the parameters
    _vstprintf_s(buffer,len,str.c_str(),p_list);
    // Adding to the string
    *this = (LPCTSTR)buffer;

    delete[] buffer;
  }
}

// Format a message by system format instead of printf
void 
XString::FormatMessage(LPCTSTR p_format,...)
{
  va_list argList;
  va_start(argList,p_format);

  FormatMessageV(p_format,&argList);

  va_end(argList);
}

void 
XString::FormatMessage(UINT p_strID,...)
{
  va_list argList;
  va_start(argList,p_strID);

  FormatMessageV(p_strID,&argList);

  va_end(argList);
}

void 
XString::FormatMessageV(LPCTSTR p_format,va_list* p_list)
{
  // format message into temporary buffer pszTemp
  CHeapPtr<TCHAR,CLocalAllocator> pszTemp;

  // FormatMessage returns zero in case of failure or the number of characters
  // if it is success, but we may actually get 0 as a number of characters.
  // So to avoid this situation use SetLastError and GetLastErorr to determine
  // whether the function FormatMessage has failed.
  DWORD dwLastError = ::GetLastError();
  ::SetLastError(0);

  DWORD dwResult = ::FormatMessage(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ALLOCATE_BUFFER
                                  ,p_format,0,0,reinterpret_cast<LPTSTR>(&pszTemp),0,p_list);

  if((dwResult == 0) && (::GetLastError() != 0))
  {
    throw std::bad_alloc();
  }
  ::SetLastError(dwLastError);

  *this = (LPCTSTR)pszTemp;
}

void 
XString::FormatMessageV(UINT p_strID,va_list* p_list)
{
  XString format;
  
  if(format.LoadString(p_strID))
  {
    // format message into temporary buffer pszTemp
    CHeapPtr<TCHAR,CLocalAllocator> pszTemp;
    DWORD dwLastError = ::GetLastError();
    ::SetLastError(0);

    DWORD dwResult = ::FormatMessage(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ALLOCATE_BUFFER
                                     ,format.c_str(),0,0,reinterpret_cast<LPTSTR>(&pszTemp),0,p_list);

    if((dwResult == 0) && (::GetLastError() != 0))
    {
      throw std::bad_alloc();
    }
    ::SetLastError(dwLastError);

    *this = (LPCTSTR)pszTemp;
  }
}

// Getting one character of the string safely
int
XString::GetAt(int p_index) const
{
  if(p_index >= 0 && (int)p_index < size())
  {
    return at(p_index);
  }
  return 0;
}

// Getting buffer of at least p_length + 1 size
PTSTR 
XString::GetBufferSetLength(int p_length)
{
  if(p_length > size())
  {
    append(((size_t)p_length - size()),0);
  }
  else
  {
    resize(p_length);
  }
  return (PTSTR)c_str();
}

// Getting a shell environment variable
BOOL 
XString::GetEnvironmentVariable(LPCTSTR p_variable)
{
  BOOL  result = FALSE;
  ULONG length = ::GetEnvironmentVariable(p_variable,nullptr,0);

  if(length == 0)
  {
    Empty();
  }
  else
  {
    TCHAR* pszBuffer = alloc_new TCHAR[length + 1];
    ::GetEnvironmentVariable(p_variable,(LPTSTR)pszBuffer,length);
    *this = pszBuffer;
    delete [] pszBuffer;
    result = TRUE;
  }
  return result;
}

// Insert char or string
int 
XString::Insert(int p_index,LPCTSTR p_string)
{
  insert(p_index,p_string);
  return (int)length();
}

int 
XString::Insert(int p_index,TCHAR p_char)
{
  insert(p_index,1,p_char);
  return (int)length();
}

// Taking the left side of the string
XString 
XString::Left(int p_length) const
{
  if(p_length < 0)
  {
    p_length = 0;
  }

  stdstring copy(*this);
  const XString left(copy.substr(0,p_length).c_str());
  return left;
}

// Load a string from the resources
BOOL 
XString::LoadString(UINT p_strID)
{
  const ATLSTRINGRESOURCEIMAGE* resource = nullptr;
  HINSTANCE instance = _AtlBaseModule.GetHInstanceAt(0);

  for(int ind = 1; instance != nullptr && resource == nullptr; instance = _AtlBaseModule.GetHInstanceAt(ind++))
  {
    resource = AtlGetStringResourceImage(instance,p_strID,0);
    if(resource)
    {
      return LoadString(instance,p_strID,0);
    }
  }
  return FALSE;
}

BOOL 
XString::LoadString(HINSTANCE p_inst,UINT p_strID,WORD p_languageID)
{
  const ATLSTRINGRESOURCEIMAGE* image = AtlGetStringResourceImage(p_inst,p_strID,p_languageID);
  if(image == nullptr)
  {
    return(FALSE);
  }
  int logicSize = image->nLength + 1;
  int  realSize = logicSize * sizeof(WCHAR);
  WCHAR* temp = alloc_new WCHAR[logicSize + 1];
  memcpy_s(temp,realSize,(void*)image->achString,realSize);
  ((char*)temp)[realSize - 1] = 0;
  ((char*)temp)[realSize - 2] = 0;

#ifdef _UNICODE
  *this = temp;
#else
  *this = (LPCSTR)WStringToString(temp).GetString();
#endif

  delete [] temp;
  return(TRUE);
}

// Lock the buffer returning the string
// Does not exactly what CString does!!
LPCTSTR 
XString::LockBuffer()
{
  // Lock();
  return c_str();
}

XString&
XString::MakeLower()
{
  for(size_t ind = 0;ind < length();++ind)
  {
    replace(ind,1,1,(char)tolower(at(ind)));
  }
  return *this;
}

XString&
XString::MakeUpper()
{
  for(size_t ind = 0;ind < length(); ++ind)
  {
    replace(ind,1,1,(char)toupper(at(ind)));
  }
  return *this;
}

XString&
XString::MakeReverse()
{
  _tcsrev((TCHAR*)(c_str()));
  return *this;
}

// Releasing the buffer again
void 
XString::ReleaseBuffer(int p_newLength /*=-1*/)
{
  if(p_newLength < 0)
  {
    p_newLength = (int)_tcslen(c_str());
  }
  if(length() >= p_newLength)
  {
    erase(p_newLength);
    shrink_to_fit();
  }
}

void 
XString::ReleaseBufferSetLength(int p_newLength)
{
  if(p_newLength < 0 || p_newLength > (int)capacity())
  {
    throw std::bad_array_new_length();
  }
  else if(p_newLength > length())
  {
    resize(p_newLength,0);
  }
  else if(p_newLength < length())
  {
    erase(p_newLength);
    shrink_to_fit();
  }
}

// Remove all occurrences of char
int 
XString::Remove(TCHAR p_char)
{
  int count = 0;
  size_t pos = 0;
  while(pos < size())
  {
    if(at(pos) == p_char)
    {
      erase(pos,1);
      ++count;
    }
    else
    {
      ++pos;
    }
  }
  return count;
}

// Replace a string or a character

int XString::Replace(LPCTSTR p_old,LPCTSTR p_new)
{
  size_t count  = 0;
  size_t length = _tcslen(p_old);
  size_t incr   = _tcslen(p_new);
  size_t pos    = find(p_old);

  while(pos != string::npos)
  {
    replace(pos,length,p_new);
    pos = find(p_old,pos + incr);
    ++count;
  }
  return (int)count;
}

int 
XString::Replace(TCHAR p_old,TCHAR p_new)
{
  int count = 0;
  size_t pos = find(p_old);

  while(pos != stdstring::npos)
  {
    replace(pos,1,1,p_new);
    pos = find(p_old);
    ++count;
  }
  return count;
}

// Get substring from the right 
XString
XString::Right(int p_length) const
{
  int sz = (int) size();
  if(p_length > sz)
  {
    p_length = sz;
  }
  return XString(substr(sz - p_length,p_length).c_str());
}

// Set char at a position
void 
XString::SetAt(int p_index,TCHAR p_char)
{
  int sz = (int) size();
  if(p_index < 0 || p_index > sz)
  {
    throw std::bad_array_new_length();
  }
  if(p_index == sz)
  {
    push_back(p_char);
  }
  else
  {
    replace(p_index,1,1,p_char);
  }
}

// SetString interface
void 
XString::SetString(LPCTSTR p_string)
{
  if(p_string == nullptr)
  {
    throw std::exception("Null pointer string");
  }
  DWORD_PTR str   = (DWORD_PTR) p_string;
  DWORD_PTR begin = (DWORD_PTR) c_str();
  DWORD_PTR end   = (DWORD_PTR) c_str() + size();
  if(begin <= str && str <= end)
  {
    // It was a substring of the existing string
    erase(0,str - begin);
  }
  else
  {
    // Completely different string
    *this = p_string;
  }
}

void 
XString::SetString(LPCTSTR p_string,int p_length)
{
  if(p_string == nullptr)
  {
    throw std::exception("Null pointer string");
  }
  stdstring str(p_string);
  if(p_length < (int)str.size())
  {
    str.resize(p_length);
  }
  SetString(str.c_str());
}

// Set string from a COM BSTR
// Does something different than CString, because it does NOT 
// reduce the amount of string space, but copies the BSTR to the String
BSTR 
XString::SetSysString(BSTR* p_string)
{
#ifdef UNICODE
  int nLen = (int) _tcslen(*p_string);
  BOOL bSuccess = ::SysReAllocStringLen(p_string,NULL,nLen);
  if(bSuccess)
  {
    _tcsncpy_s(*p_string,nLen,c_str(),nLen);
  }
#else
  int nLen = ::MultiByteToWideChar(CP_ACP,0,c_str(),(DWORD)size(),NULL,NULL);
  BOOL bSuccess = ::SysReAllocStringLen(p_string,NULL,nLen);
  if(bSuccess)
  {
    ::MultiByteToWideChar(CP_ACP,0,c_str(),(DWORD)size(),*p_string,nLen);
  }
#endif
  else
  {
    throw StdException(_T("Bad XString allocation!"));
  }
  return(*p_string);
}

// Leftmost string not in argument
XString 
XString::SpanExcluding(LPCTSTR p_string) const
{
  if(p_string == nullptr)
  {
    throw StdException(_T("Null pointer string"));
  }
  return Left((int)_tcscspn(c_str(),p_string));
}

// Leftmost string in argument
XString 
XString::SpanIncluding(LPCTSTR p_string) const
{
  if(p_string == nullptr)
  {
    throw std::exception("Null pointer string");
  }
  return Left((int)_tcsspn(c_str(),p_string));
}

// Length of the string
int 
XString::StringLength(LPCTSTR p_string)
{
  if(p_string == nullptr)
  {
    return 0;
  }
  return (int)_tcslen(p_string);
}

// Return tokenized strings
XString 
XString::Tokenize(LPCTSTR p_tokens,int& p_curpos) const
{
  if(p_curpos < 0)
  {
    throw std::bad_alloc();
  }
  if((p_tokens == nullptr) || (*p_tokens == 0))
  {
    if(p_curpos < GetLength())
    {
      return (XString(c_str() + p_curpos));
    }
  }
  else
  {
    LPCTSTR pszPlace = c_str() + p_curpos;
    LPCTSTR pszEnd   = c_str() + size();
    if(pszPlace < pszEnd)
    {
      int nIncluding = (int)_tcsspn(pszPlace,p_tokens);

      if((pszPlace + nIncluding) < pszEnd)
      {
        pszPlace += nIncluding;
        int nExcluding = (int)_tcscspn(pszPlace,p_tokens);

        int iFrom  = p_curpos + nIncluding;
        int nUntil = nExcluding;
        p_curpos   = iFrom + nUntil + 1;

        return(Mid(iFrom,nUntil));
      }
    }
  }
  // return empty string, done tokenizing
  p_curpos = -1;

  return XString(_T(""));
}

XString& 
XString::TrimLeft(TCHAR p_char)
{
  if(!empty())
  {
    int count = 0;
    LPCTSTR str = c_str();
    while(*str && *str == p_char)
    {
      ++str;
      ++count;
    }

    erase(0,count);
  }
  return *this;
}

XString& 
XString::TrimLeft(LPCTSTR p_string)
{
  // if we're not trimming anything, we're not doing any work
  if((p_string == nullptr) || (*p_string == 0) || empty())
  {
    return(*this);
  }

  LPCTSTR psz = c_str();
  while((*psz != 0) && (_tcschr(p_string,*psz) != NULL)) ++psz;

  if(psz != c_str())
  {
    // fix up data and length
    int iFirst = int(psz - c_str());
    erase(0,iFirst);
  }
  return(*this);
}

XString& XString::TrimRight(TCHAR p_char)
{
  if(!empty())
  {
    size_t pos = size() - 1;
    while(pos != stdstring::npos)
    {
      if(at(pos) != p_char)
      {
        ++pos;
        break;
      }
      if(pos == 0)
      {
        break;
      }
      --pos;
    }
    erase(pos);
  }
  return *this;
}

XString& 
XString::TrimRight(LPCTSTR p_string)
{
  // if we're not trimming anything, we're not doing any work
  if((p_string == nullptr) || (*p_string == 0) || empty())
  {
    return(*this);
  }

  // Start at the ending of the string
  size_t pos = size() - 1;
  while(pos && (pos != stdstring::npos))
  {
    if(_tcschr(p_string,at(pos)) != nullptr)
    {
      --pos;
    }
    else
    {
      ++pos;
      break;
    }
  }

  // truncate at left-most matching character
  if(pos >= 0)
  {
  	erase(pos,string::npos);
  }
  return(*this);
}

XString XString::Mid(int p_index) const
{
  if(p_index < length())
  {
    return XString(substr(p_index).c_str());
  }
  return XString();
}

XString XString::Mid(int p_index,int p_length) const
{
  if(p_index < length())
  {
    return XString(substr(p_index,p_length).c_str());
  }
  return XString();
}

//////////////////////////////////////////////////////////////////////////
//
// OPERATORS
//
//////////////////////////////////////////////////////////////////////////

XString::operator LPTSTR() const
{
  return (LPTSTR)c_str();
}

XString::operator LPCTSTR() const
{
  return c_str();
}

XString
XString::operator+(const XString& p_extra) const
{
  XString total(c_str());
  total.append(p_extra);
  return total;
}

XString
XString::operator+(LPCTSTR p_extra) const
{
  XString string(c_str());
  string.append(p_extra);
  return string;
}

XString
XString::operator+(const TCHAR p_char) const
{
  XString string(c_str());
  string.append(1,p_char);
  return string;
}

XString&
XString::operator+=(XString& p_extra)
{
  append(p_extra.c_str());
  return *this;
}

XString&
XString::operator+=(const stdstring& p_string)
{
  append(p_string);
  return *this;
}

XString&
XString::operator+=(LPCTSTR p_extra)
{
  append(p_extra);
  return *this;
}

XString&
XString::operator=(const XString& p_extra)
{
  assign(p_extra);
  return *this;
}

XString&
XString::operator+=(const TCHAR p_char)
{
  append(1,p_char);
  return *this;
}

XString&
XString::operator=(LPCTSTR p_string)
{
  assign(p_string);
  return *this;
}

//////////////////////////////////////////////////////////////////////////
//
// Conversion methods
//
//////////////////////////////////////////////////////////////////////////

int
XString::AsInt()
{
  return _ttoi(c_str());
}

long
XString::AsLong()
{
  return _ttol(c_str());
}

unsigned
XString::AsUnsigned()
{
  return (unsigned)_ttoi(c_str());
}

INT64
XString::AsInt64()
{
  return _ttoi64(c_str());
}

UINT64
XString::AsUint64()
{
  TCHAR* endptr = nullptr;
  return _tcstoui64(c_str(),&endptr,10);
}

void
XString::SetNumber(int p_number,int p_radix /*= 10*/)
{
  TCHAR buffer[14];
  _itot_s(p_number,buffer,14,p_radix);
  *this = buffer;
}

void
XString::SetNumber(unsigned p_number)
{
  TCHAR buffer[14];
  _itot_s(p_number,buffer,14,10);
  *this = buffer;
}

void
XString::SetNumber(INT64 p_number)
{
  TCHAR buffer[21];
  _i64tot_s(p_number,buffer,21,10);
  *this = buffer;
}

void
XString::SetNumber(UINT64 p_number)
{
  TCHAR buffer[21];
  _ui64tot_s(p_number,buffer,21,10);
  *this = buffer;
}
