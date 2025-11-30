//////////////////////////////////////////////////////////////////////////
//
// File: XString.h
//
// Std Mfc eXtension String is a string derived from std::string
// But does just about everything that MFC CString also does
// 
// Created: 2014-2025 ir. W.E. Huisman MSC
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

#pragma once
#include <wtypes.h>
#include <string>
#include <tchar.h>

using std::string;
using std::wstring;

#ifdef _UNICODE
#define stdstring wstring
#pragma message("XString is now defined as std::wstring")
#else
#define stdstring string
#pragma message("XString is now defined as std::string")
#endif

#pragma warning(disable: 4239)

class XString : public stdstring
{
public:
  // Empty CTOR
  XString();
  // CTOR from character pointer
  XString(LPCTSTR p_string);
  // CTOR from a number of characters
  XString(TCHAR p_char,int p_count = 1);
  // CTOR from other string
  XString(const XString& p_string);
  // CTOR from other string
  XString(const stdstring& p_string);
#ifdef _UNICODE
  // CTOR form a wchar_t stream
  XString(wchar_t* p_string);
  // CTOR from a ANSI string
  XString(PCSTR p_string);
#else
// CTOR from unsigned char stream
  XString(unsigned char* p_string);
  // CTOR from unicode string
  XString(PCWSTR p_string);
#endif

  // Convert String to BSTR. Free it with "SysFreeString"
  BSTR        AllocSysString();
  // Append a string, or n chars from a string
  void        Append(LPCTSTR p_string);
  void        Append(LPCTSTR p_string,int p_length);
  // Append a single character
  void        AppendChar(TCHAR p_char);
  // Append a formatted string
  void        AppendFormat(LPCTSTR p_format,...);
  void        AppendFormat(UINT    p_strID ,...);
  // Append a formatted variable list
  void        AppendFormatV(LPCTSTR p_format,va_list p_list);
  void        AppendFormatV(UINT    p_strID, va_list p_list);

  // ANSI/OEM Conversions
  void        AnsiToOem();
  void        OemToAnsi();

  // Collate
  int         Collate(LPCTSTR p_string);
  int         CollateNoCase(LPCTSTR p_string);
  // Compare
  int         Compare(LPCTSTR p_string) const;
  int         CompareNoCase(LPCTSTR p_string) const;
  // Construct. MFC does this, but it's unclear/undocumented why!
  // void    Construct(String* p_string);

  // Delete, returning new length
  int         Delete(int p_index,int p_count);
  // Make empty
  void        Empty();
  // Find position or -1 for not found
  int         Find(TCHAR   p_char,  int p_start = 0) const;
  int         Find(LPCTSTR p_string,int p_start = 0) const;
  // Find on of the chars or -1 for not found
  int         FindOneOf(LPCTSTR p_string) const;
  // Format a string
  void        Format(LPCTSTR p_format,...);
  void        Format(UINT    p_strID ,...);
  void        Format(XString p_format,...);
  // Format a variable list
  void        FormatV(LPCTSTR p_format,va_list p_list);
  void        FormatV(UINT    p_strID, va_list p_list);
  // Format a message by system format instead of printf
  void        FormatMessage(LPCTSTR p_format,...);
  void        FormatMessage(UINT    p_strID, ...);
  // Format a message by system format from variable list
  void        FormatMessageV(LPCTSTR p_format,va_list* p_list);
  void        FormatMessageV(UINT    p_strID, va_list* p_list);
  // Free extra space (shrinking the string)
  void        FreeExtra();
  // Getting a shell environment variable
  BOOL        GetEnvironmentVariable(LPCTSTR p_variable);
  // Getting string length
  int         GetLength() const;
  // Getting a char from the string
  int         GetAt(int p_index) const;
  // Length of allocated string capacity
  unsigned    GetAllocLength() const;
  // Getting buffer of at least p_length + 1 size
  PTSTR       GetBufferSetLength(int p_length);
  // Releasing the buffer again
  void        ReleaseBuffer(int p_newLength = -1);
  void        ReleaseBufferSetLength(int p_newLength);
  // Getting the string
  LPCTSTR     GetString() const;
  // Insert char or string
  int         Insert(int p_index,LPCTSTR p_string);
  int         Insert(int p_index,TCHAR   p_char);
  // See if string is empty
  bool        IsEmpty() const;
  // Taking the left side of the string
  XString     Left(int p_length) const;
  // Locking the buffer
  LPCTSTR     LockBuffer();
  // Load a string from the resources
  BOOL        LoadString(UINT p_strID);
  BOOL        LoadString(HINSTANCE p_inst,UINT p_strID);
  BOOL        LoadString(HINSTANCE p_inst,UINT p_strID,WORD p_languageID);
  // Make lower/upper case or reverse
  XString&    MakeLower();
  XString&    MakeReverse();
  XString&    MakeUpper();
  // Take substring out of the middle
  XString     Mid(int p_index) const;
  XString     Mid(int p_index,int p_length) const;
  // Preallocate a string of specified size
  void        Preallocate(int p_length);
  // Remove all occurrences of char
  int         Remove(TCHAR p_char);
  // Replace a string or a character
  int         Replace(LPCTSTR p_old,LPCTSTR p_new);
  int         Replace(TCHAR   p_old,TCHAR   p_new);
  // Find last occurrence of a char
  int         ReverseFind(TCHAR p_char) const;
  // Get substring from the right 
  XString     Right(int p_length) const;
  // Set char at a position
  void        SetAt(int p_index,TCHAR p_char);
  // SetString interface
  void        SetString(LPCTSTR p_string);
  void        SetString(LPCTSTR p_string,int p_length);
  // Set string from a COM BSTR
  BSTR        SetSysString(BSTR* p_string);
  // Leftmost string (not) in argument
  XString     SpanExcluding(LPCTSTR p_string) const;
  XString     SpanIncluding(LPCTSTR p_string) const;
  // Length of the string
  static int  StringLength (LPCTSTR p_string);
  // Return tokenized strings
  XString     Tokenize(LPCTSTR p_tokens,int& p_curpos) const;
  // Trim the string
  XString&    Trim();
  XString&    Trim(TCHAR  p_char);
  XString&    Trim(LPCTSTR p_string);
  XString&    TrimLeft();
  XString&    TrimLeft(TCHAR  p_char);
  XString&    TrimLeft(LPCTSTR p_string);
  XString&    TrimRight();
  XString&    TrimRight(TCHAR  p_char);
  XString&    TrimRight(LPCTSTR p_string);
  // Truncate the string
  void        Truncate(int p_length);

  // OPERATORS

  operator  LPTSTR()  const;
  operator  LPCTSTR() const;
  XString   operator+ (const XString& p_extra) const;
  XString   operator+ (LPCTSTR p_extra) const;
  XString   operator+ (const TCHAR p_char) const;
  XString&  operator+=(XString& p_extra);
  XString&  operator+=(const stdstring& p_string);
  XString&  operator+=(LPCTSTR p_extra);
  XString&  operator+=(const TCHAR p_char);
  XString&  operator =(const XString& p_extra);
  XString&  operator =(LPCTSTR p_string);
  // Extra conversion methods
  int       AsInt();
  long      AsLong();
  unsigned  AsUnsigned();
  INT64     AsInt64();
  UINT64    AsUint64();

  void      SetNumber(int      p_number,int p_radix = 10);
  void      SetNumber(unsigned p_number);
  void      SetNumber(INT64    p_number);
  void      SetNumber(UINT64   p_number);

private:
  // No extra private data
};

// Add _T("some string") + XString-object
inline XString operator+(LPCTSTR lhs,const XString& rhs)
{
  XString result(lhs);
  result += rhs;
  return result;
}

//////////////////////////////////////////////////////////////////////////
//
// INLINING FOR PERFORMANCE
// All methods consisting of one line of code are inlined
//
//////////////////////////////////////////////////////////////////////////

inline void XString::Append(LPCTSTR p_string)
{
  append(p_string);
}

inline void XString::AppendChar(TCHAR p_char)
{
  push_back(p_char);
}

inline int XString::Collate(LPCTSTR p_string)
{
  return _tcscoll(c_str(),p_string);
}

inline int XString::CollateNoCase(LPCTSTR p_string)
{
  return _tcsicoll(c_str(),p_string);
}

inline int XString::Compare(LPCTSTR p_string) const
{
  return _tcscmp(c_str(),p_string);
}

inline int XString::CompareNoCase(LPCTSTR p_string) const
{
  return _tcsicmp(c_str(),p_string);
}

inline void XString::Empty()
{
  clear();
}

inline int XString::Find(TCHAR p_char,int p_start /*= 0*/) const
{
  return (int) find(p_char,p_start);
}

inline int XString::Find(LPCTSTR p_string,int p_start /*= 0*/) const
{
  return (int) find(p_string,p_start);
}

inline int XString::FindOneOf(LPCTSTR p_string) const
{
  // Find on of the chars or -1 for not found
  return (int) find_first_of(p_string,0);
}

inline void XString::FreeExtra()
{
  shrink_to_fit();
}

inline int XString::GetLength() const
{
  return (int)length();
}

inline LPCTSTR XString::GetString() const
{
  return c_str();
}

inline unsigned  XString::GetAllocLength() const
{
  return (unsigned)capacity();
}

inline bool XString::IsEmpty() const
{
  return empty();
}

inline BOOL XString::LoadString(HINSTANCE p_inst,UINT p_strID)
{
  return LoadString(p_inst,p_strID,0);
}

inline void XString::OemToAnsi()
{
#ifndef _UNICODE
  // Only works for MBCS, not for Unicode
  ::OemToCharBuff(c_str(),(LPSTR)c_str(),static_cast<DWORD>(length()));
#endif
}

inline void XString::Preallocate(int p_length)
{
  reserve(p_length);
}

inline int XString::ReverseFind(TCHAR p_char) const
{
  return (int) find_last_of(p_char);
}

inline XString& XString::Trim()
{
  return TrimLeft().TrimRight();
}

inline XString& XString::Trim(TCHAR p_char)
{
  return TrimLeft(p_char).TrimRight(p_char);
}

inline XString& XString::Trim(LPCTSTR p_string)
{
  return TrimLeft(p_string).TrimRight(p_string);
}

inline XString& XString::TrimLeft()
{
  return TrimLeft(' ');
}

inline XString& XString::TrimRight()
{
  return TrimRight(' ');
}

inline void XString::Truncate(int p_length)
{
  erase(p_length);
}
