//////////////////////////////////////////////////////////////////////////
//
// File: SMX_String.h
//
// Std Mfc eXtension String is a string derived from std::string
// But does just about everything that MFC XString also does
// 
// Copyright (c) 2014-2025 ir. W.E. Huisman MSC
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

using std::string;
using std::wstring;

// Are we using MFC (AFX)
#ifdef _AFX
// If we are using the BaseLibrary within a MFC project
// The string definition is purely the MFC XString class
typedef CString XString;
#pragma message("XString is now defined as MFC::CString")
#else
#ifdef __ATLSTR_H__
#pragma message("XString is now defined as ATL::CString")
#else
#pragma message("XString is now defined as std::string::MSX_String")
#endif

#ifdef _UNICODE
#define stdstring std::wstring
#else
#define stdstring std:string
#endif

#pragma warning(disable: 4239)

class SMX_String : public stdstring
{
public:
  // Empty CTOR
  SMX_String();
  // CTOR from character pointer
  SMX_String(LPCTSTR p_string);
  // CTOR from a number of characters
  SMX_String(TCHAR p_char,int p_count = 1);
  // CTOR from other string
  SMX_String(const SMX_String& p_string);
  // CTOR from std::string
  SMX_String(const stdstring& p_string);
  // CTOR from a ANSI string
  SMX_String(PCSTR p_string);

  // Convert String to BSTR. Free it with "SysFreeString"
  BSTR        AllocSysString();
  // Append a string, or n chars from a string
  void        Append(LPCTSTR p_string);
  void        Append(LPCTSTR p_string,int p_length);
  // Append a single character
  void        AppendChar(TCHAR p_char);
  // Append a formatted string
  void        AppendFormat(LPCTSTR p_format,...);
  void        AppendFormat(UINT   p_strID ,...);
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
  void        Format(UINT   p_strID ,...);
  void        Format(SMX_String p_format,...);
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
  PCTSTR      GetString() const;
  // Insert char or string
  int         Insert(int p_index,LPCTSTR p_string);
  int         Insert(int p_index,TCHAR   p_char);
  // See if string is empty
  bool        IsEmpty() const;
  // Taking the left side of the string
  SMX_String  Left(int p_length) const;
  // Locking the buffer
  PCTSTR      LockBuffer();
  // Load a string from the resources
  BOOL        LoadString(UINT p_strID);
  BOOL        LoadString(HINSTANCE p_inst,UINT p_strID);
  BOOL        LoadString(HINSTANCE p_inst,UINT p_strID,WORD p_languageID);
  // Make lower/upper case or reverse
  void        MakeLower();
  void        MakeReverse();
  void        MakeUpper();
  // Take substring out of the middle
  SMX_String  Mid(int p_index) const;
  SMX_String  Mid(int p_index,int p_length) const;
  // Preallocate a string of specified size
  void        Preallocate(int p_length);
  // Remove all occurrences of char
  int         Remove(TCHAR p_char);
  // Replace a string or a character
  int         Replace(PCTSTR p_old,PCTSTR p_new);
  int         Replace(TCHAR p_old,TCHAR p_new);
  // Find last occurrence of a char
  int         ReverseFind(TCHAR p_char) const;
  // Get substring from the right 
  SMX_String  Right(int p_length) const;
  // Set char at a position
  void        SetAt(int p_index,TCHAR p_char);
  // SetString interface
  void        SetString(PCTSTR p_string);
  void        SetString(PCTSTR p_string,int p_length);
  // Set string from a COM BSTR
  BSTR        SetSysString(BSTR* p_string);
  // Leftmost string (not) in argument
  SMX_String  SpanExcluding(PCTSTR p_string);
  SMX_String  SpanIncluding(PCTSTR p_string);
  // Length of the string
 static int   StringLength (PCTSTR p_string);
  // Return tokenized strings
  SMX_String  Tokenize(PCTSTR p_tokens,int& p_curpos) const;
  // Trim the string
  SMX_String& Trim();
  SMX_String& Trim(TCHAR  p_char);
  SMX_String& Trim(PCTSTR p_string);
  SMX_String& TrimLeft();
  SMX_String& TrimLeft(TCHAR  p_char);
  SMX_String& TrimLeft(PCTSTR p_string);
  SMX_String& TrimRight();
  SMX_String& TrimRight(TCHAR  p_char);
  SMX_String& TrimRight(PCTSTR p_string);
  // Truncate the string
  void        Truncate(int p_length);

  // OPERATORS

  operator LPTSTR() const;
  operator LPCTSTR() const;
  SMX_String   operator+ (const SMX_String& p_extra) const;
  SMX_String   operator+ (LPCTSTR p_extra) const;
  SMX_String   operator+ (const TCHAR p_char) const;
  SMX_String   operator+=(SMX_String& p_extra);
  SMX_String   operator+=(stdstring& p_string);
  SMX_String   operator+=(LPCTSTR p_extra);
  SMX_String   operator+=(const TCHAR p_char);
  SMX_String&  operator =(const SMX_String& p_extra);
  SMX_String&  operator =(LPCTSTR p_string);

private:
//   In CString these are in StringData 
//   We do not use it here, as we do not use the same locking scheme
//   in a std::string derived class
//
//   void      AddRef();
//   bool      IsLocked();
//   bool      IsShared();
//   void      Release();
//   void      Lock();
//   void      Unlock();
//   void      UnlockBuffer();
// 
//   // Extra data
//   long      m_references;
};

//////////////////////////////////////////////////////////////////////////
//
// INLINING FOR PERFORMANCE
// All methods consisting of one line of code are inline
//
//////////////////////////////////////////////////////////////////////////

inline void SMX_String::Append(LPCTSTR p_string)
{
  append(p_string);
}

inline void SMX_String::AppendChar(TCHAR p_char)
{
  push_back(p_char);
}

inline int SMX_String::Collate(LPCTSTR p_string)
{
  return _tcscoll(c_str(),p_string);
}

inline int SMX_String::CollateNoCase(LPCTSTR p_string)
{
  return _tcsicoll(c_str(),p_string);
}

inline int SMX_String::Compare(LPCTSTR p_string) const
{
  return _tcscmp(c_str(),p_string);
}

inline int SMX_String::CompareNoCase(LPCTSTR p_string) const
{
  return _tcsicmp(c_str(),p_string);
}

inline void SMX_String::Empty()
{
  clear();
}

inline int SMX_String::Find(TCHAR p_char,int p_start /*= 0*/) const
{
  return (int) find(p_char,p_start);
}

inline int SMX_String::Find(LPCTSTR p_string,int p_start /*= 0*/) const
{
  return (int) find(p_string,p_start);
}

inline int SMX_String::FindOneOf(LPCTSTR p_string) const
{
  // Find on of the chars or -1 for not found
  return (int) find_first_of(p_string,0);
}

inline void SMX_String::FreeExtra()
{
  shrink_to_fit();
}

inline int SMX_String::GetLength() const
{
  return (int)length();
}

inline PCTSTR SMX_String::GetString() const
{
  return c_str();
}

inline int SMX_String::GetAt(int p_index) const
{
  return at(p_index);
}

inline unsigned  SMX_String::GetAllocLength() const
{
  return (unsigned)capacity();
}

inline bool SMX_String::IsEmpty() const
{
  return empty();
}

inline BOOL SMX_String::LoadString(HINSTANCE p_inst,UINT p_strID)
{
  return LoadString(p_inst,p_strID,0);
}

inline void SMX_String::MakeReverse()
{
  _tcsrev((TCHAR*)(c_str()));
}

inline SMX_String SMX_String::Mid(int p_index) const
{
  return SMX_String(substr(p_index));
}

inline SMX_String SMX_String::Mid(int p_index,int p_length) const
{
  return SMX_String(substr(p_index,p_length));
}

inline void SMX_String::OemToAnsi()
{
#ifndef _UNICODE
  // Only works for MBCS, not for Unicode
  ::OemToCharBuff(c_str(),reinterpret_cast<LPTSTR>(c_str()),reinterpret_cast<DWORD>(length()));
#endif
}

inline void SMX_String::Preallocate(int p_length)
{
  reserve(p_length);
}

inline int SMX_String::ReverseFind(TCHAR p_char) const
{
  return (int) find_last_of(p_char);
}

inline SMX_String& SMX_String::Trim()
{
  return TrimLeft().TrimRight();
}

inline SMX_String& SMX_String::Trim(TCHAR p_char)
{
  return TrimLeft(p_char).TrimRight(p_char);
}

inline SMX_String& SMX_String::Trim(PCTSTR p_string)
{
  return TrimLeft(p_string).TrimRight(p_string);
}

inline SMX_String& SMX_String::TrimLeft()
{
  return TrimLeft(' ');
}

inline SMX_String& SMX_String::TrimRight()
{
  return TrimRight(' ');
}

inline void SMX_String::Truncate(int p_length)
{
  erase(p_length);
}

// Use XString for brevity
typedef SMX_String XString;

//////////////////////////////////////////////////////////////////////////
//
// StringData counterparts
//
//////////////////////////////////////////////////////////////////////////

// inline bool String::IsLocked()
// {
//   return m_references < 0;
// }
// 
// inline bool String::IsShared()
// {
//   return m_references > 0;
// }
//
// inline void String::UnlockBuffer()
// {
//   // Unlock();
// }

#endif  // #ifdef _AFX
