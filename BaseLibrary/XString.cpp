//////////////////////////////////////////////////////////////////////////
//
// SMX_String
//
// Std Mfc eXtension (SMX)String is a string derived from std::string
// But does just about everything that MFC XString also does
// The string is derived from std::string with XString methods
// SMX = std::string with MFC eXtensions
//
// Copyright (c) 2016-2025 ir. W.E. Huisman MSC
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

// Are we using MFC (AFX)
#ifdef _AFX
// If we are using the BaseLibrary within a MFC project
// The string definition is purely the MFC XString class
typedef CString XString;
#pragma message("XString is now defined as MFC::CString")
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#else
#ifdef __ATLSTR_H__
#pragma message("XString is now defined as ATL::CString")
#else
#pragma message("XString is now defined as std::string::MSX_String")
#endif

SMX_String::SMX_String()
{
}

// CTOR from character pointer
SMX_String::SMX_String(LPCTSTR p_string)
           :stdstring(p_string)
{
}

// CTOR from a number of characters
SMX_String::SMX_String(TCHAR p_char,int p_count /* = 1*/)
{
  append(p_count,p_char);
}

// CTOR from other string
SMX_String::SMX_String(const SMX_String& p_string)
{
  append(p_string.c_str());
}

// CTOR from std::string
SMX_String::SMX_String(const stdstring& p_string)
           :stdstring(p_string)
{
}

// CTOR from a ANSI string
SMX_String::SMX_String(PCSTR p_string)
{
#ifdef _UNICODE
  int len = MultiByteToWideChar(CP_ACP,0,p_string,(int)strlen(p_string),nullptr,0);
  wchar_t* buffer = new wchar_t[len + 1];
  MultiByteToWideChar(CP_ACP,0,p_string,(int)strlen(p_string),buffer,len);
  append(buffer);
  delete [] buffer;
#else
  append(p_string);
#endif
}

BSTR 
SMX_String::AllocSysString()
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
SMX_String::AnsiToOem()
{
#ifndef _UNICODE
  // Only works for MBCS, not for Unicode
  ::CharToOemBuff(reinterpret_cast<LPCSTR>(c_str()),reinterpret_cast<LPSTR>(c_str()),reinterpret_cast<DWORD>(length()));
#endif
}

// Append a string, or n chars from a string
//
void 
SMX_String::Append(LPCTSTR p_string,int p_length)
{
  stdstring str(p_string);
  append(str.substr(0,p_length));
}

// Append a formatted string
void 
SMX_String::AppendFormat(LPCTSTR p_format,...)
{
  va_list argList;
  va_start(argList,p_format);

  AppendFormatV(p_format,argList);

  va_end(argList);
}

void 
SMX_String::AppendFormat(UINT p_strID,...)
{
  va_list argList;
  va_start(argList,p_strID);

  AppendFormatV(p_strID,argList);

  va_end(argList);
}

void 
SMX_String::AppendFormatV(LPCTSTR p_format,va_list p_list)
{
  // Getting a buffer of the correct length
  int len = _vsctprintf(p_format,p_list) + 1;
  TCHAR* buffer = new TCHAR[len];
  // Formatting the parameters
  _vstprintf_s(buffer,len,p_format,p_list);
  // Adding to the string
  append(buffer);

  delete[] buffer;
}

void 
SMX_String::AppendFormatV(UINT p_strID,va_list p_list)
{
  // Getting the string
  SMX_String str;
  if(str.LoadString(p_strID))
  {
    // Getting a buffer of the correct length
    int len = _vsctprintf(str.c_str(),p_list) + 1;
    TCHAR* buffer = new TCHAR[len];
    // Formatting the parameters
    _vstprintf_s(buffer,len,str.c_str(),p_list);
    // Adding to the string
    append(buffer);

    delete[] buffer;
  }
}

// Delete, returning new length
int 
SMX_String::Delete(int p_index,int p_count)
{
  erase(p_index,p_count);
  return (int) length();
}

// Format a string
void 
SMX_String::Format(LPCTSTR p_format,...)
{
  va_list argList;
  va_start(argList,p_format);

  FormatV(p_format,argList);

  va_end(argList);
}

void 
SMX_String::Format(UINT p_strID,...)
{
  va_list argList;
  va_start(argList,p_strID);

  FormatV(p_strID,argList);

  va_end(argList);
}

void
SMX_String::Format(SMX_String p_format,...)
{
  LPCTSTR format = p_format.c_str();
  va_list argList;
  va_start(argList,format);

  FormatV(p_format.c_str(),argList);

  va_end(argList);
}

// Format a variable list
void 
SMX_String::FormatV(LPCTSTR p_format,va_list p_list)
{
  // Getting a buffer of the correct length
  int len = _vsctprintf(p_format,p_list) + 1;
  TCHAR* buffer = new TCHAR[len];
  // Formatting the parameters
  _vstprintf_s(buffer,len,p_format,p_list);
  // Adding to the string
  *this = (LPCTSTR)buffer;

  delete[] buffer;
}

void 
SMX_String::FormatV(UINT p_strID,va_list p_list)
{
  // Getting the string
  SMX_String str;
  if(str.LoadString(p_strID))
  {
    // Getting a buffer of the correct length
    int len = _vsctprintf(str.c_str(),p_list) + 1;
    TCHAR* buffer = new TCHAR[len];
    // Formatting the parameters
    _vstprintf_s(buffer,len,str.c_str(),p_list);
    // Adding to the string
    *this = (LPCTSTR)buffer;

    delete[] buffer;
  }
}

// Format a message by system format instead of printf
void 
SMX_String::FormatMessage(LPCTSTR p_format,...)
{
  va_list argList;
  va_start(argList,p_format);

  FormatMessageV(p_format,&argList);

  va_end(argList);
}

void 
SMX_String::FormatMessage(UINT p_strID,...)
{
  va_list argList;
  va_start(argList,p_strID);

  FormatMessageV(p_strID,&argList);

  va_end(argList);
}

void 
SMX_String::FormatMessageV(LPCTSTR p_format,va_list* p_list)
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
SMX_String::FormatMessageV(UINT p_strID,va_list* p_list)
{
  SMX_String format;
  
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

// Getting buffer of at least p_length + 1 size
PTSTR 
SMX_String::GetBufferSetLength(int p_length)
{
  reserve(p_length);
  return (PTSTR)c_str();
}

// Getting a shell environment variable
BOOL 
SMX_String::GetEnvironmentVariable(LPCTSTR p_variable)
{
  BOOL  result = FALSE;
  ULONG length = ::GetEnvironmentVariable(p_variable,NULL,0);

  if(length == 0)
  {
    Empty();
  }
  else
  {
    TCHAR* pszBuffer = new TCHAR[length + 1];
    ::GetEnvironmentVariable(p_variable,(LPTSTR)pszBuffer,length);
    *this = pszBuffer;
    delete [] pszBuffer;
    result = TRUE;
  }
  return result;
}

// Insert char or string
int 
SMX_String::Insert(int p_index,LPCTSTR p_string)
{
  insert(p_index,p_string);
  return (int)length();
}

int 
SMX_String::Insert(int p_index,TCHAR p_char)
{
  insert(p_index,1,p_char);
  return (int)length();
}

// Taking the left side of the string
SMX_String 
SMX_String::Left(int p_length) const
{
  if(p_length < 0)
  {
    p_length = 0;
  }

  stdstring copy(*this);
  const SMX_String left(copy.substr(0,p_length));
  return left;
}

// Load a string from the resources
BOOL 
SMX_String::LoadString(UINT p_strID)
{
  const ATLSTRINGRESOURCEIMAGE* resource = nullptr;
  HINSTANCE instance = _AtlBaseModule.GetHInstanceAt(0);

  for(int ind = 1; instance != NULL && resource == nullptr; instance = _AtlBaseModule.GetHInstanceAt(ind++))
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
SMX_String::LoadString(HINSTANCE p_inst,UINT p_strID,WORD p_languageID)
{
  const ATLSTRINGRESOURCEIMAGE* image = AtlGetStringResourceImage(p_inst,p_strID,p_languageID);
  if(image == nullptr)
  {
    return(FALSE);
  }
  int logicSize = image->nLength + 1;
  int  realSize = logicSize * sizeof(WCHAR);
  WCHAR* temp = new WCHAR[logicSize + 1];
  memcpy_s(temp,realSize,(void*)image->achString,realSize);
  ((char*)temp)[realSize - 1] = 0;
  ((char*)temp)[realSize - 2] = 0;

#ifdef _UNICODE
  *this = temp;
#else
  *this = (LPCSTR) WStringToString(temp).GetString();
#endif

  delete [] temp;
  return(TRUE);
}

// Lock the buffer returning the string
// Does not exactly what XString does!!
PCTSTR 
SMX_String::LockBuffer()
{
  // Lock();
  return c_str();
}

void 
SMX_String::MakeLower()
{
  for(size_t ind = 0;ind < length();++ind)
  {
    replace(ind,1,1,(char)tolower(at(ind)));
  }
}

void 
SMX_String::MakeUpper()
{
  for(size_t ind = 0;ind < length(); ++ind)
  {
    replace(ind,1,1,(char)toupper(at(ind)));
  }
}

// Releasing the buffer again
void 
SMX_String::ReleaseBuffer(int p_newLength /*=-1*/)
{
  if(p_newLength < 0)
  {
    shrink_to_fit();
  }
  else
  {
    resize(p_newLength,0);
  }
}

void 
SMX_String::ReleaseBufferSetLength(int p_newLength)
{
  if(p_newLength < 0 || p_newLength > (int)capacity())
  {
    throw std::bad_array_new_length();
  }
  else
  {
    resize(p_newLength,0);
  }
}

// Remove all occurrences of char
int 
SMX_String::Remove(TCHAR p_char)
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

int SMX_String::Replace(PCTSTR p_old,PCTSTR p_new)
{
  int    count  = 0;
  size_t length = _tcslen(p_old);
  size_t pos    = find(p_old);

  while(pos != string::npos)
  {
    replace(pos,length,p_new);
    pos = find(p_old);
    ++count;
  }
  return count;
}

int 
SMX_String::Replace(TCHAR p_old,TCHAR p_new)
{
  int count = 0;
  size_t pos = find(p_old);

  while(pos != string::npos)
  {
    replace(pos,1,1,p_new);
    pos = find(p_old);
    ++count;
  }
  return count;
}

// Get substring from the right 
SMX_String
SMX_String::Right(int p_length) const
{
  int sz = (int) size();
  if(p_length > sz)
  {
    p_length = sz;
  }
  return SMX_String(substr(sz - p_length,p_length));
}

// Set char at a position
void 
SMX_String::SetAt(int p_index,TCHAR p_char)
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
SMX_String::SetString(PCTSTR p_string)
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
SMX_String::SetString(PCTSTR p_string,int p_length)
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
// Does something different than XString, because it does NOT 
// reduce the amount of string space, but copies the BSTR to the String
BSTR 
SMX_String::SetSysString(BSTR* p_string)
{
#ifdef _UNICODE
  _tcscpy_s(*p_string,GetLength(),c_str());
#else
  int nLen = ::MultiByteToWideChar(CP_ACP,0,c_str(),(DWORD)size(),NULL,NULL);
  BOOL bSuccess = ::SysReAllocStringLen(p_string,NULL,nLen);
  if(bSuccess)
  {
    ::MultiByteToWideChar(CP_ACP,0,c_str(),(DWORD)size(),*p_string,nLen);
  }
  else
  {
    throw StdException(_T("Bad SMX_String allocation!"));
  }
#endif
  return(*p_string);
}

// Leftmost string not in argument
SMX_String 
SMX_String::SpanExcluding(PCTSTR p_string)
{
  if(p_string == nullptr)
  {
    throw StdException(_T("Null pointer string"));
  }
  return Left((int)_tcscspn(c_str(),p_string));
}

// Leftmost string in argument
SMX_String 
SMX_String::SpanIncluding(PCTSTR p_string)
{
  if(p_string == nullptr)
  {
    throw std::exception("Null pointer string");
  }
  return Left((int)_tcsspn(c_str(),p_string));
}

// Length of the string
int 
SMX_String::StringLength(PCTSTR p_string)
{
  if(p_string == nullptr)
  {
    return 0;
  }
  return (int)_tcslen(p_string);
}

// Return tokenized strings
SMX_String 
SMX_String::Tokenize(PCTSTR p_tokens,int& p_curpos) const
{
  if(p_curpos < 0)
  {
    throw std::bad_alloc();
  }
  if((p_tokens == nullptr) || (*p_tokens == 0))
  {
    if(p_curpos < GetLength())
    {
      return (SMX_String(c_str() + p_curpos));
    }
  }
  else
  {
    PCTSTR pszPlace = c_str() + p_curpos;
    PCTSTR pszEnd   = c_str() + size();
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

  return SMX_String(_T(""));
}

SMX_String& 
SMX_String::TrimLeft(TCHAR p_char)
{
  int count = 0;
  PCTSTR str = c_str();
  while(*str && *str == p_char)
  {
    ++str;
    ++count;
  }

  erase(0,count);
  return *this;
}

SMX_String& 
SMX_String::TrimLeft(PCTSTR p_string)
{
  // if we're not trimming anything, we're not doing any work
  if((p_string == nullptr) || (*p_string == 0))
  {
    return(*this);
  }

  PCTSTR psz = c_str();
  while((*psz != 0) && (_tcschr(p_string,*psz) != NULL)) ++psz;

  if(psz != c_str())
  {
    // fix up data and length
    int iFirst = int(psz - c_str());
    erase(0,iFirst);
  }
  return(*this);
}

SMX_String& SMX_String::TrimRight(TCHAR p_char)
{
  if(!empty())
  {
    size_t pos = size() - 1;
    while(pos != string::npos)
    {
      if(at(pos) != p_char)
      {
        ++pos;
        break;
      }
      --pos;
    }
    erase(pos);
  }
  return *this;
}

SMX_String& 
SMX_String::TrimRight(PCTSTR p_string)
{
  // if we're not trimming anything, we're not doing any work
  if((p_string == nullptr) || (*p_string == 0))
  {
    return(*this);
  }

  // Start at the ending of the string
  size_t pos = size() - 1;
  while(pos != string::npos)
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
  erase(pos,string::npos);
  return(*this);
}

//////////////////////////////////////////////////////////////////////////
//
// OPERATORS
//
//////////////////////////////////////////////////////////////////////////

SMX_String::operator LPTSTR() const
{
  return (LPTSTR)c_str();
}

SMX_String::operator LPCTSTR() const
{
  return c_str();
}

SMX_String
SMX_String::operator+(const SMX_String& p_extra) const
{
  SMX_String string(c_str());
  string.append(p_extra.c_str());
  return string;
}

SMX_String
SMX_String::operator+(LPCTSTR p_extra) const
{
  SMX_String string(c_str());
  string.append(p_extra);
  return string;
}

SMX_String 
SMX_String::operator+ (const TCHAR p_char) const
{
  SMX_String string(c_str());
  string.append(1,p_char);
  return string;
}

SMX_String
SMX_String::operator+=(SMX_String& p_extra)
{
  append(p_extra.c_str());
  return *this;
}

SMX_String
SMX_String::operator+=(stdstring& p_string)
{
  append(p_string);
  return *this;
}

SMX_String
SMX_String::operator+=(LPCTSTR p_extra)
{
  append(p_extra);
  return *this;
}

SMX_String&
SMX_String::operator=(const SMX_String& p_extra)
{
  assign(p_extra);
  return *this;
}

SMX_String
SMX_String::operator+=(const TCHAR p_char)
{
  append(1,p_char);
  return *this;
}

SMX_String&
SMX_String::operator=(LPCTSTR p_string)
{
  assign(p_string);
  return *this;
}


//////////////////////////////////////////////////////////////////////////
//
// StringData counterparts
//
//////////////////////////////////////////////////////////////////////////

// void String::AddRef()
// {
//   _InterlockedIncrement(&m_references);
// }
// 
// void String::Release()
// {
//   if(_InterlockedDecrement(&m_references) <= 0)
//   {
//     delete this;
//   }
// }
// 
// void String::Lock()
// {
//   // Locked buffers can't be shared
//   --m_references;
//   if(m_references == 0)
//   {
//     m_references = -1;
//   }
// 
// }
// 
// void String::Unlock()
// {
//   if(IsLocked())
//   {
//     ++m_references;
//     if(m_references == 0)
//     {
//       m_references = 1;
//     }
//   }
// }

#endif