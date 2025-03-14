/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: HTTPTime.cpp
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
#include "HTTPTime.h"

#ifdef _AFX
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#endif

const TCHAR* weekday_short[7] =
{
  _T("Sun")
 ,_T("Mon")
 ,_T("Tue")
 ,_T("Wed")
 ,_T("Thu")
 ,_T("Fri")
 ,_T("Sat")
};

const TCHAR* weekday_long[7] =
{
   _T("Sunday")
  ,_T("Monday")
  ,_T("Tuesday")
  ,_T("Wednesday")
  ,_T("Thursday")
  ,_T("Friday")
  ,_T("Saturday")
};

const TCHAR* month[12] =
{
  _T("Jan")
 ,_T("Feb")
 ,_T("Mar")
 ,_T("Apr")
 ,_T("May")
 ,_T("Jun")
 ,_T("Jul")
 ,_T("Aug")
 ,_T("Sep")
 ,_T("Oct")
 ,_T("Nov")
 ,_T("Dec")
};

// Print HTTP time in RFC 1123 format (Preferred standard)
// as in "Tue, 8 Dec 2015 21:26:32 GMT"
bool
HTTPTimeFromSystemTime(const SYSTEMTIME* p_systemtime,XString& p_time)
{
  // Check that we have a system time
  if(!p_systemtime ||
      p_systemtime->wDayOfWeek < 0 ||
      p_systemtime->wDayOfWeek > 6 ||
      p_systemtime->wMonth     < 1 ||
      p_systemtime->wMonth     > 12 )
  {
    SetLastError(ERROR_INVALID_PARAMETER);
    return false;
  }

  p_time.Format(_T("%s, %02d %s %04d %2.2d:%2.2d:%2.2d GMT")
                ,weekday_short[p_systemtime->wDayOfWeek]
                ,p_systemtime->wDay
                ,month[p_systemtime->wMonth - 1]
                ,p_systemtime->wYear
                ,p_systemtime->wHour
                ,p_systemtime->wMinute
                ,p_systemtime->wSecond);

  SetLastError(ERROR_SUCCESS);
  return true;
}

// Check year loosely for RFC850/1036 implementations
static void 
CheckYearImplementation(SYSTEMTIME* p_systemtime)
{
  if(p_systemtime->wYear < 100 && p_systemtime->wYear >= 0)
  {
    if(p_systemtime->wYear < 50)
    {
      p_systemtime->wYear += 2000;
    }
    else
    {
      p_systemtime->wYear += 1900;
    }
  }
}

// Convert an loosely conforming RFC2616 time such as 
// 'Sun, 04-Dec-2016 12:02:37 GMT' to the proper systemtime

// Excerpt from RFC 2616:
// 
// HTTP applications have historically allowed three different formats
// for the representation of date / time stamps :
//
// Sun, 06 Nov 1994 08:49:37 GMT      ; RFC 822,updated by RFC 1123
// Sunday, 06 - Nov - 94 08:49:37 GMT ; RFC 850,obsoleted by RFC 1036
// Sun Nov  6 08:49:37 1994           ; ANSI C's asctime() format
//
// The first format is preferred as an Internet standard and represents
// a fixed - length subset of that defined by RFC 1123[8](an update to
// RFC 822[9]).The second format is in common use,but is based on the
// obsolete RFC 850[12] date format and lacks a four - digit year.
// HTTP / 1.1 clients and servers that parse the date value MUST accept
// all three formats(for compatibility with HTTP / 1.0),though they MUST
// only generate the RFC 1123 format for representing HTTP - date values
// in header fields.See section 19.3 for further information
//
bool
HTTPTimeToSystemTime(const XString p_time,SYSTEMTIME* p_systemtime)
{
  unsigned index  = 0;
  unsigned length = p_time.GetLength();
  bool     ansic  = false;

  // Check if we have something to do
  if(p_time.IsEmpty() || !p_systemtime)
  {
    SetLastError(ERROR_INVALID_PARAMETER);
    return false;
  }
  SetLastError(ERROR_SUCCESS);

  // Reset systemtime
  memset(p_systemtime,0,sizeof(SYSTEMTIME));

  // Skip whitespace
  while(index < length && isspace(p_time.GetAt(index)))
  {
    ++index;
  }
  // If empty string, use default system time
  // Various implementations e.g. MS-Windows do this strange behavior
  // by getting the current system time
  if(index >= length)
  {
    GetSystemTime(p_systemtime);
    return true;
  }
  // Default day is not-scanned
  p_systemtime->wDayOfWeek = 7;
  // Now scan for the day of the week
  for(int ind = 0; ind < 7; ++ind)
  {
    if((p_time.Mid(index,3).CompareNoCase(weekday_short[ind]) == 0) ||
       (p_time.Mid(index,(int)_tcslen(weekday_long[ind])).CompareNoCase(weekday_long[ind]) == 0))
    {
      p_systemtime->wDayOfWeek = (WORD) ind;
      while(index < length && isalpha(p_time.GetAt(index))) ++index;
      break;
    }
  }

  // No weekday found
  if(p_systemtime->wDayOfWeek > 6) 
  {
    SetLastError(ERROR_INVALID_PARAMETER);
    memset(p_systemtime,0,sizeof(SYSTEMTIME));
    return false;
  }

  // Skip spaces and separators
  while(index < length  && 
        !isdigit(p_time.GetAt(index)) && 
        !isalpha(p_time.GetAt(index)))
  {
    ++index;
  }

  if(isalpha(p_time.GetAt(index)))
  {
    // ANSI C month name first
    ansic = true;
    for(int ind = 0; ind < 12; ++ind)
    {
      if(p_time.Mid(index,3).CompareNoCase(month[ind]) == 0)
      {
        p_systemtime->wMonth = (WORD)(ind + 1);
        while(index < length && isalpha(p_time.GetAt(index))) ++index;
        // Skip spaces and separators
        while(index < length  &&
              !isdigit(p_time.GetAt(index)) &&
              !isalpha(p_time.GetAt(index)))
        {
          ++index;
        }
        break;
      }
    }
  }
  
  // Scan day-of-the-month
  p_systemtime->wDay = (WORD) _ttoi(&p_time.GetString()[index]);
  while(index < length && isdigit(p_time.GetAt(index))) ++index;

  // Skip spaces and separators
  while(index < length  &&
        !isdigit(p_time.GetAt(index)) &&
        !isalpha(p_time.GetAt(index)))
  {
    ++index;
  }

  if(isalpha(p_time.GetAt(index)))
  {
    // Find the month name
    for(int ind = 0; ind < 12; ++ind)
    {
      if(p_time.Mid(index,3).CompareNoCase(month[ind]) == 0)
      {
        p_systemtime->wMonth = (WORD) (ind + 1);
        break;
      }
    }
  }
  // Check that we found a month name
  if(p_systemtime->wMonth == 0) 
  {
    SetLastError(ERROR_INVALID_PARAMETER);
    memset(p_systemtime,0,sizeof(SYSTEMTIME));
    return false;
  }

  if(!ansic)
  {
    // Skip to the year digit
    while(index < length && !isdigit(p_time.GetAt(index))) ++index;
    if(index >= length)
    {
      SetLastError(ERROR_INVALID_PARAMETER);
      memset(p_systemtime,0,sizeof(SYSTEMTIME));
      return false;
    }
    p_systemtime->wYear = (WORD) _ttoi(&p_time.GetString()[index]);
    while(index < length && isdigit(p_time.GetAt(index))) ++index;
  }

  // After this point we have the year
  // RFC's state to be robust. Missing hour or seconds are 
  // not leading to an error (Except for the ansic implementation)

  // Hours
  while(index < length && !isdigit(p_time.GetAt(index))) ++index;
  if(index >= length) 
  {
    CheckYearImplementation(p_systemtime);
    return true;
  }
  p_systemtime->wHour = (WORD) _ttoi(&p_time.GetString()[index]);
  while(index < length && isdigit(p_time.GetAt(index))) ++index;

  // Minutes
  while(index < length && !isdigit(p_time.GetAt(index))) ++index;
  if(index >= length) 
  {
    CheckYearImplementation(p_systemtime);
    return true;
  }
  p_systemtime->wMinute = (WORD) _ttoi(&p_time.GetString()[index]);
  while(index < length && isdigit(p_time.GetAt(index))) ++index;

  // Seconds
  while(index < length && !isdigit(p_time.GetAt(index))) ++index;
  if(index >= length) 
  {
    CheckYearImplementation(p_systemtime);
    return true;
  }
  p_systemtime->wSecond = (WORD) _ttoi(&p_time.GetString()[index]);
  while(index < length && isdigit(p_time.GetAt(index))) ++index;

  // Still ANSI C Year to do?
  if(ansic)
  {
    while(index < length && !isdigit(p_time.GetAt(index))) ++index;
    if(index >= length) 
    {
      SetLastError(ERROR_INVALID_PARAMETER);
      memset(p_systemtime,0,sizeof(SYSTEMTIME));
      return false;
    }
    p_systemtime->wYear = (WORD) _ttoi(&p_time.GetString()[index]);
  }
  CheckYearImplementation(p_systemtime);
  return true;
}

// Print HTTP time in RFC 1123 format (Preferred standard)
// as in "Tue, 8 Dec 2015 21:26:32 GMT"
XString
HTTPGetSystemTime()
{
  XString    time;
  SYSTEMTIME systemtime;
  GetSystemTime(&systemtime);

  time.Format(_T("%s, %02d %s %04d %2.2d:%2.2d:%2.2d GMT")
             ,weekday_short[systemtime.wDayOfWeek]
             ,systemtime.wDay
             ,month[systemtime.wMonth - 1]
             ,systemtime.wYear
             ,systemtime.wHour
             ,systemtime.wMinute
             ,systemtime.wSecond);

  return time;
}

const double SecondsPer100ns = 100. * 1.E-9;

void AddSecondsToSystemTime(SYSTEMTIME* p_timeIn,SYSTEMTIME* p_timeOut, double p_seconds)
{
  union 
  {
    ULARGE_INTEGER li;
    FILETIME       ft;
  };

  // Convert timeIn to filetime
  SystemTimeToFileTime(p_timeIn,&ft);

  // Add in the seconds
  li.QuadPart += (ULONGLONG) (p_seconds / SecondsPer100ns);

  // Convert back to systemtime
  FileTimeToSystemTime(&ft,p_timeOut);
}
