/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: XMLTemporal.cpp
//
// Copyright (c) 2014-2024 ir. W.E. Huisman
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
#include "BaseLibrary.h"
#include "XMLTemporal.h"
#include <time.h>

#ifdef _AFX
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#endif

// Number of days at the beginning of the month
// 365 days at the end of the year
//
int g_daysInTheMonth[14] =
{
   0    // January
  ,31   // February
  ,59   // March
  ,90   // April
  ,120  // May
  ,151  // June
  ,181  // July
  ,212  // August
  ,243  // September
  ,273  // October
  ,304  // November
  ,334  // December
  ,365  // 1 Year
  ,396  // February next year
};

//////////////////////////////////////////////////////////////////////////
//
// BASE CLASS XMLTemporal
//
//////////////////////////////////////////////////////////////////////////

// XTOR Base class
XMLTemporal::XMLTemporal(XString p_value)
            :m_string(p_value)
{
}

// XML Temporal parsing
bool
XMLTemporal::ParseXMLDate(const XString& p_string,XMLTimestamp& p_moment)
{
  int ja[4] = {0,0,0,0};
  int ma[2] = {0,0};
  int da[2] = {0,0};
  int uu[2] = {0,0};
  int mi[2] = {0,0};
  int se[2] = {0,0};
  int fraction = 0;
  int UTCuu[2] = {0,0};
  int UTCmi[2] = {0,0};

  // changed char to unsigned int for 64 bit implementation
  TCHAR sep3,sep4,sep5,sep6,sep7,sep8;

  int n = _stscanf_s(p_string
                    ,_T("%1d%1d%1d%1d-%1d%1d-%1d%1d%c%1d%1d%c%1d%1d%c%1d%1d%c%d%c%1d%1d%c%1d%1d")
                    ,&ja[0],&ja[1],&ja[2],&ja[3]
                    ,&ma[0],&ma[1]
                    ,&da[0],&da[1]
                    ,&sep3,(unsigned int) sizeof(TCHAR)
                    ,&uu[0],&uu[1]
                    ,&sep4,(unsigned int) sizeof(TCHAR)
                    ,&mi[0],&mi[1]
                    ,&sep5,(unsigned int) sizeof(TCHAR)
                    ,&se[0],&se[1]
                    ,&sep6,(unsigned int) sizeof(TCHAR)
                    ,&fraction
                    ,&sep7,(unsigned int) sizeof(TCHAR)
                    ,&UTCuu[0],&UTCuu[1]
                    ,&sep8,(unsigned int) sizeof(TCHAR)
                    ,&UTCmi[0],&UTCmi[1]);

  int jaar    = ja[0] * 1000 + ja[1] * 100 + ja[2] * 10 + ja[3];
  int maand   = ma[0] * 10 + ma[1];
  int dag     = da[0] * 10 + da[1];
  int uur     = uu[0] * 10 + uu[1];
  int minuut  = mi[0] * 10 + mi[1];
  int seconde = se[0] * 10 + se[1];
  int UTCuurBuffer = UTCuu[0] * 10 + UTCuu[1];
  int UTCminBuffer = UTCmi[0] * 10 + UTCmi[1];

  if(n >= 8 && jaar >= 0 && jaar <= 9999 && maand >= 0 && maand <= 12 && dag >= 0 && dag <= 31)
  {
    bool valid = false;
    bool UTC    = false;
    bool offset = false;
    int  offsetminuten = 0;

    switch(n)
    {
    case 8:   // Has "YYYY-MM-DD" pattern
              valid = true;
              break;
    case 9:   // Has "YYYY-MM-DDZ" pattern
              UTC = (sep3 == 'Z');
              break;
    case 14:  // UTCuurBuffer in uur
              // UTCminBuffer in minuut
              if((sep3 == '-' || sep3 == '+') && sep4 == ':' && minuut >= 0 && minuut <= 59)
              {
                // Has "YYYY-MM-DD[+-]HH:MM" pattern
                offsetminuten = uur * 60 + minuut;
                if(offsetminuten >= 0 && offsetminuten <= 840)
                {
                  if(sep3 == '+')
                  {
                    offsetminuten = -offsetminuten;
                  }
                  uur    = 0;
                  minuut = 0;
                  offset = true;
                }
              }
              break;
    case 17:  // Has "YYYY-MM-DDTHH:MM:SS" pattern
              valid = sep3 == 'T' && sep4 == ':' && sep5 == ':';
              break;
    case 18:  // Has "YYYY-MM-DDTHH:MM:SSZ" pattern
              UTC   = sep3 == 'T' && sep4 == ':' && sep5 == ':' && sep6 == 'Z';
              break;
    case 19:  // Has "YYYY-MM-DDTHH:MM:SS.fraction" pattern
              valid = sep3 == 'T' && sep4 == ':' && sep5 == ':' && sep6 == '.';
              break;
    case 20:  // Has "YYYY-MM-DDTHH:MM:SS.fractionZ" pattern
              UTC   = sep3 == 'T' && sep4 == ':' && sep5 == ':' && sep6 == '.' && sep7 == 'Z';
              break;
    case 22:  // UTCuurBuffer in fraction
              // UTCminBuffer in UTCuurBuffer
              if(sep3 == 'T' && sep4 == ':'  && sep5 == ':' && 
                (sep6 == '+' || sep6 == '-') && sep7 == ':' && 
                UTCuurBuffer >= 0 && 
                UTCuurBuffer <= 59)
              {
                // Has "YYYY-MM-DDTHH:MM:SS[+-]HH:MM" pattern
                offsetminuten = fraction * 60 + UTCuurBuffer;
                if(offsetminuten >= 0 && offsetminuten <= 840)
                {
                  if(sep6 == '-')
                  {
                    offsetminuten = -offsetminuten;
                  }
                  offset = true;
                }
                fraction = 0;
              }
              break;
    case 25: if(sep3 == 'T' && sep4 == ':'  && sep5 == ':' && sep6 == '.' && 
               (sep7 == '+' || sep7 == '-') && sep8 == ':' && 
               UTCminBuffer >= 0 && 
               UTCminBuffer <= 59)
             {
               // Has "YYYY-MM-DDTHH:MM:SS.fraction[+-]HH:MM" pattern
               offsetminuten = UTCuurBuffer * 60 + UTCminBuffer;
               if(offsetminuten >= 0 && offsetminuten <= 840)
               {
                 if(sep7 == '-')
                 {
                   offsetminuten = -offsetminuten;
                 }
                 offset = true;
               }
             }
             break;
    }
    if((valid || offset || UTC) && 
      ((uur     >= 0 && uur     <= 23 && 
        minuut  >= 0 && minuut  <= 59 && 
        seconde >= 0 && seconde <= 59) || 
        (uur == 24 && minuut == 0 && seconde == 0 && fraction == 0)))
    {
      bool plusdag = uur == 24;
      if(plusdag) uur = 0;

      p_moment.SetTimestamp(jaar,maand,dag,uur,minuut,seconde);

      if(plusdag)
      {
        p_moment = p_moment.AddDays(1);
      }
      if(offset)
      {
        p_moment = p_moment.AddMinutes(offsetminuten);
        UTC = true;
      }
      if(UTC)
      {
        TIME_ZONE_INFORMATION tziCurrent;
        ::ZeroMemory(&tziCurrent,sizeof(tziCurrent));
        if(::GetTimeZoneInformation(&tziCurrent) != TIME_ZONE_ID_INVALID)
        {
          p_moment = p_moment.AddMinutes(-tziCurrent.Bias);
        }
      }
      // Store the fraction (if any)
      p_moment.SetFraction(fraction);
      return true;
    }
  }
  return false;
}

//////////////////////////////////////////////////////////////////////////
//
// XMLTime
//
//////////////////////////////////////////////////////////////////////////

// XTOR Time
XMLTime::XMLTime(XString p_value)
        :XMLTemporal(p_value)
{
  ParseTime(p_value);
}

void 
XMLTime::ParseTime(XString p_value)
{
  // Copy and trim the string
  XString string(p_value);
  string.Trim();

  if(string.IsEmpty())
  {
    // No time string, do not parse
    return;
  }

  if(ParseXMLTime(p_value))
  {
    return;
  }

  // Support different separators 
  string.Replace(_T(" "),_T(":"));
  string.Replace(_T("-"),_T(":"));
  string.Replace(_T("."),_T(":"));
  string.Replace(_T("/"),_T(":"));

  // Clear the result
  m_theTime.m_hour = -1;
  m_theTime.m_minute = 0;
  m_theTime.m_second = 0;

  //  Parse the string
  int n = _stscanf_s(string,_T("%d:%d:%d")
                    ,&m_theTime.m_hour
                    ,&m_theTime.m_minute
                    ,&m_theTime.m_second);
  if(n < 2)
  {
    throw StdException(_T("Wrong format for conversion to time. Must be in the format: hh:mm[:ss]"));
  }

  // Check for correct values
  if(m_theTime.m_hour < 0 || m_theTime.m_hour > 23)
  {
    throw StdException(_T("Incorrect time format: the value of the hour must be between 0 and 23 (inclusive)"));
  }

  if(m_theTime.m_minute < 0 || m_theTime.m_minute > 59)
  {
    throw StdException(_T("Incorrect time format: the value of minutes must be between 0 and 59 (inclusive)"));
  }

  if(m_theTime.m_second < 0 || m_theTime.m_second > 59)
  {
    throw StdException(_T("Incorrect time format: the value of the seconds must be between 0 and 59 (inclusive)"));
  }
  SetTime();
}

// XML Time support
bool
XMLTime::ParseXMLTime(const XString& p_string)
{
  unsigned int uu[2] = {0,0};
  unsigned int mi[2] = {0,0};
  unsigned int se[2] = {0,0};
  unsigned int fraction = 0;
  unsigned int UTCuu[2] = {0,0};
  unsigned int UTCmi[2] = {0,0};

  //  Parse the string
  // changed char to unsigned int for 64 bit implementation
  TCHAR sep1,sep2,sep3,sep4,sep5;
  int n = _stscanf_s(p_string,_T("%1u%1u%c%1u%1u%c%1u%1u%c%u%c%1u%1u%c%1u%1u"),
                     &uu[0],&uu[1],
                     &sep1,(unsigned int) sizeof(TCHAR),
                     &mi[0],&mi[1],
                     &sep2,(unsigned int) sizeof(TCHAR),
                     &se[0],&se[1],
                     &sep3,(unsigned int) sizeof(TCHAR),
                     &fraction,
                     &sep4,(unsigned int) sizeof(TCHAR),
                     &UTCuu[0],&UTCuu[1],
                     &sep5,(unsigned int) sizeof(TCHAR),
                     &UTCmi[0],&UTCmi[1]);

  int uurBuffer = uu[0] * 10 + uu[1];
  int minBuffer = mi[0] * 10 + mi[1];
  int secBuffer = se[0] * 10 + se[1];
  int UTCuurBuffer = UTCuu[0] * 10 + UTCuu[1];
  int UTCminBuffer = UTCmi[0] * 10 + UTCmi[1];

  if(n >= 8 && sep1 == ':' && sep2 == ':' && 
    ((uurBuffer >=  0 && uurBuffer <= 23 && 
      minBuffer >=  0 && minBuffer <= 59 && 
      secBuffer >=  0 && secBuffer <= 59) ||
     (uurBuffer == 24 && minBuffer == 0 && secBuffer == 0)))
  {
    SetTime(uurBuffer,minBuffer,secBuffer);
    bool UTC = false;

    switch(n)
    {
      case 8:     // Has "HH:MM:SS" pattern
                  return true;
                  break;
      case 9:     // Has "HH:MM:SSZ" pattern for UTC time
                  UTC = sep3 == 'Z';
                  break;
      case 10:    if(sep3 == '.')
                  {
                    // Has "HH:MM:SS.fraction" pattern
                    return true;
                  }
                  break;
      case 11:    // Has "HH:MM:SS.fractionZ" pattern for UTC time
                  UTC = sep3 == '.' && sep4 == 'Z';
                  break;
      case 13:    // UTCuurBuffer in fraction
                  // UTCminBuffer in UTCuurBuffer
                  //
                  if((sep3 == '-' || sep3 == '+') && sep4 == ':' && 
                     UTCuurBuffer >= 0 && 
                     UTCuurBuffer <= 59)
                  {
                    // Has "HH:MM:SS[+-]UU:MM" pattern
                    int minutes = fraction * 60 + UTCuurBuffer;
                    if(minutes >= 0 && minutes <= 840)
                    {
                      if(sep3 == '+')
                      {
                        minutes = -minutes;
                      }
                      m_value += (INT64)minutes * ONE_MINUTE;
                      Normalise();
                      UTC = true;
                    }
                  }
                  break;
      case 16:    if(sep3 == '.' && (sep4 == '-' || sep4 == '+') && sep5 == ':' && 
                     UTCminBuffer >= 0 && UTCminBuffer <= 59)
                  {
                    // Has "HH:MM:SS.fraction[+-]UU:MM" pattern
                    int minutes = UTCuurBuffer * 60 + UTCminBuffer;
                    if(minutes >= 0 && minutes <= 840)
                    {
                      if(sep4 == '+')
                      {
                        minutes = -minutes;
                      }
                      m_value += (int64)minutes * ONE_MINUTE;
                      Normalise();
                      UTC = true;
                    }
                  }
                  break;
    }
    if(UTC)
    {
      TIME_ZONE_INFORMATION tziCurrent;
      ::ZeroMemory(&tziCurrent,sizeof(tziCurrent));
      if(::GetTimeZoneInformation(&tziCurrent) != TIME_ZONE_ID_INVALID)
      {
        m_value += (INT64)-tziCurrent.Bias * ONE_MINUTE;
        Normalise();
      }
      return true;
    }
  }
  return false;
}

// Setting the time
// Storage value is the total number of seconds on a day
void
XMLTime::SetTime(int p_hour,int p_minute,int p_second)
{
  m_theTime.m_hour = p_hour;
  m_theTime.m_minute = p_minute;
  m_theTime.m_second = p_second;
  SetTime();
}

// Private SetTime: Calculate the number of seconds from the storage
void
XMLTime::SetTime()
{
  m_value = 0;
  if(m_theTime.m_hour   >= 0 && m_theTime.m_hour   < 24 &&
     m_theTime.m_minute >= 0 && m_theTime.m_minute < 60 &&
     m_theTime.m_second >= 0 && m_theTime.m_second < 60)
  {
    m_value = ((INT64)m_theTime.m_hour   * ONE_HOUR  ) + 
              ((INT64)m_theTime.m_minute * ONE_MINUTE) +
               (INT64)m_theTime.m_second;
  }
  else
  {
    // Tell it to the outside world
    XString error;
    error.Format(_T("Incorrect time %02d:%02d:%02d"),m_theTime.m_hour,m_theTime.m_minute,m_theTime.m_second);
    // Reset the time to NULL
    // Then throw the error
    throw StdException(error);
  }
}

// Normalise the number of hours, minutes and seconds       
//
void
XMLTime::Normalise()
{
  if(m_value >= ONE_DAY)
  {
    m_value %= ONE_DAY;
  }
  if(m_value < 0)
  {
    m_value = ONE_DAY - (-m_value % ONE_DAY);
  }
  m_theTime.m_hour   = (int) m_value / ONE_HOUR;
  long rest          = (int) m_value % ONE_HOUR;
  m_theTime.m_minute =       rest    / ONE_MINUTE;
  m_theTime.m_second =       rest    % ONE_MINUTE;
}

//////////////////////////////////////////////////////////////////////////
//
// XMLDate
//
//////////////////////////////////////////////////////////////////////////

XMLDate::XMLDate()
        :XMLTemporal(_T(""))
{
}

XMLDate::XMLDate(XString p_value)
        :XMLTemporal(p_value)
{
  ParseDate(p_value);
}

bool
XMLDate::ParseDate(XString p_value)
{
  XString datum(p_value);
  bool    success = false;

  // Remove spaces at both ends
  datum.Trim();

  if(datum.IsEmpty())
  {
    return true;
  }

  XMLTimestamp mom;
  if(ParseXMLDate(datum,mom))
  {
    SetDate(mom.Year(),mom.Month(),mom.Day());
    success = true;
  }

  if(!success)
  {
    int jaar = 0;
    int maand = 0;
    int dag = 0;
    if(!ParseDate(datum,&jaar,&maand,&dag))
    {
      // Wrong formatted date
      throw StdException(_T("Date has a wrong format"));
    }
    // Special case: Pronto does this
    // 00-00-00 -> wordt vandaag
    if(dag == 0 && maand == 0)
    {
      Today();
      success = true;
    }
    else
    {
      // Set the MJD on the year/month/day values
      // and thus performs check on domain values and correctness
      success = SetDate(jaar,maand,dag);
    }
  }
  return success;
}

bool
XMLDate::ParseDate(const XString& p_datum,int* p_jaar,int* p_maand,int* p_dag)
{
  XString datum(p_datum);

  // Minimum European of American format
  datum.Replace('-',' ');
  datum.Replace('/',' ');
  datum.Replace('.',' ');

  int num = _stscanf_s(datum,_T("%d %d %d"),p_jaar,p_maand,p_dag);

  // Need at x-y-z pattern 
  bool result = (num == 3);

  // Optional century correction
  if(*p_jaar < 100 && *p_jaar > 0)
  {
    *p_jaar += *p_jaar <= 50 ? 2000 : 1900;
  }
  return result;
}

// Today's date
void
XMLDate::Today()
{
  _tzset();
  __time64_t ltime;
  _time64(&ltime);
  struct tm now;
  _localtime64_s(&now,&ltime);

  // Return as SQLDate object
  SetDate(now.tm_year + 1900,now.tm_mon + 1,now.tm_mday);
}

// SQLDate::SetDate
// Set the date year-month-day in a date instance
bool
XMLDate::SetDate(int p_year,int p_month,int p_day)
{
  m_date.m_year  = (short)p_year;
  m_date.m_month = (char) p_month;
  m_date.m_day   = (char) p_day;
  return SetMJD();
}

// Calculate the DateValue m_mjd for a date
bool
XMLDate::SetMJD()
{
  long year  = m_date.m_year;
  long month = m_date.m_month;
  long day   = m_date.m_day;

  // Validate year, month and day by the ODBC definition
  if (year  > 0 && year  < 10000 &&
      month > 0 && month < 13    &&
      day   > 0 && day   < 32)
  {
    // Check on the Gregorian definition of months
    // Checks on leap years and gets the days-in-the-month
    bool leapyear = ((year & 3) == 0) &&
                    ((year % 100) != 0 || (year % 400) == 0);

    long daysInMonth = g_daysInTheMonth[month] - g_daysInTheMonth[month-1] +
                       ((leapyear && day == 29 && month == 2) ? 1 : 0);

    // Finish validating the date
    if (day <= daysInMonth)
    {
      // Calculate Astronomical Modified Julian Day Number (MJD)
      // Method P.D-Smith: Practical Astronomy
      // Page 9: Paragraph 4: Julian day numbers.
      // See also:
      // Robin.M. Green: Spherical Astronomy, page 250 and next.
      if(month < 3)
      {
        month += 12;
        --year;
      }
      long gregorianB = 0;
      long factorC, factorD;
      if(year > 1582)
      {
        long gregorianA = 0;
        gregorianA = year / 100;
        gregorianB = 2 - gregorianA + (gregorianA / 4);
      }
      factorC = (long)(365.25  * (double) year);
      factorD = (long)(30.6001 * (double)((size_t)month + 1));
      // The correction factor (Modified JD) 
      // Falls on 16 November 1858 12:00 hours (noon), 
      // so subtract 679006 (17 November 1858 00:00:00 hour)
      m_value = (INT64)gregorianB + (INT64)factorC + (INT64)factorD + (INT64)day - (INT64)679006;
      return true;
    }
  }
  // Preset NULL value
  m_value = 0;
  return false;
}

//////////////////////////////////////////////////////////////////////////
//
// XMLTimestamp
//
//////////////////////////////////////////////////////////////////////////

XMLTimestamp::XMLTimestamp()
             :XMLTemporal(_T(""))
{
}

XMLTimestamp::XMLTimestamp(XString p_value)
             :XMLTemporal(p_value)
{
  ParseMoment(p_value);
}

XMLTimestamp::XMLTimestamp(INT64 p_value)
             :XMLTemporal(_T(""))
{
  m_value = p_value;
  Normalise();
}

void
XMLTimestamp::ParseMoment(XString p_value)
{
  XString string(p_value);

  // Trim spaces from string
  string.Trim();
  // Test for emptiness
  if(string.IsEmpty())
  {
    return;
  }

  // Check for a XML string validation
  if(ParseXMLDate(string,*this))
  {
    RecalculateValue();
    return;
  }

  // See if we have a date AND a time
  int pos1 = string.Find(' ');
  int pos2 = string.Find('T');
  if(pos1 > 0 || pos2 > 0)
  {
    // Position comes from 'T' (leftover from XML string)
    // Beware: yy-mm-dd is different than dd-mm-yyyy
    if(pos1 < 0)
    {
      pos1 = pos2;
    }
    XString dateString = string.Left(pos1);
    XString timeString = string.Mid(pos1 + 1);

    XMLDate date(dateString);
    XMLTime time(timeString);
    SetTimestamp(date.Year(),date.Month(),date.Day(),
                 time.Hour(),time.Minute(),time.Second());
    return;
  }
  // See if we have a time only
  if(string.Find(':') > 0)
  {
    // It is a time string for today
    XMLDate date;
    date.Today();
    XMLTime time(string);
    SetTimestamp(date.Year(),date.Month(),date.Day(),
                 time.Hour(),time.Minute(),time.Second());
    return;
  }
  // It's a date only
  XMLDate date(string);
  XMLTime time(_T(""));
  SetTimestamp(date.Year(),date.Month(),date.Day(),
               time.Hour(),time.Minute(),time.Second());

}

// Set (change) the time
void
XMLTimestamp::SetTimestamp(int p_year,int p_month, int p_day
                          ,int p_hour,int p_minute,int p_second
                          ,int p_fraction /*=0*/)
{
  m_timestamp.m_year   = (short)p_year;
  m_timestamp.m_month  = (char) p_month;
  m_timestamp.m_day    = (char) p_day;
  m_timestamp.m_hour   = (char) p_hour;
  m_timestamp.m_minute = (char) p_minute;
  m_timestamp.m_second = (char) p_second;
  m_fraction = p_fraction;

  RecalculateValue();
}

void
XMLTimestamp::RecalculateValue()
{
  // Validate timestmap first;
  Validate();

  int year  = m_timestamp.m_year;
  int month = m_timestamp.m_month;
  int day   = m_timestamp.m_day;

  // Check on Gregorian definition of months
  // Check on leapyear and days in the month
  bool leapYear = ((year & 3) == 0) &&
                  ((year % 100) != 0 || (year % 400) == 0);

  int  daysInMonth = g_daysInTheMonth[month] - g_daysInTheMonth[month-1] +
                     ((leapYear && day == 29 && month == 2) ? 1 : 0);

  if (day <= 0 || day > daysInMonth)
  {
    m_value = 0;
    XString error;
    error.Format(_T("Day of the month must be between 1 and %d inclusive."),daysInMonth);
    throw StdException(error);
  }
  // Calculate the Astronomical Julian Day Number (JD)
  // Method P.D-Smith: Practical Astronomy
  // Page 9: Paragraph 4: Julian day numbers.
  // See also: Robin M. Green: Spherical Astronomy, page 250
  if(month < 3)
  {
    month += 12;
    --year;
  }
  long gregorianB = 0;
  long factorC, factorD;
  if(year > 1582)
  {
    long gregorianA = year / 100;
    gregorianB = 2 - gregorianA + (gregorianA / 4);
  }
  factorC = (long) (365.25  * (double)year);
  factorD = (long) (30.6001 * (double)((size_t)month + 1));
  // The correction factor (Modified JD) 
  // Falls on 16 November 1858 12:00 hours (noon), 
  // so subtract 679006 (17 November 1858 00:00:00 hour)
  m_value  = (INT64)gregorianB + factorC + factorD + day - (INT64)679006;
  m_value *= ONE_DAY;
  m_value += (INT64)m_timestamp.m_hour   * ONE_HOUR   +
             (INT64)m_timestamp.m_minute * ONE_MINUTE +
             (INT64)m_timestamp.m_second;
}

void
XMLTimestamp::Normalise()
{
  long gregorianB = 0;
  long factorC = 0;
  long factorD = 0;
  long factorE = 0;
  long factorG = 0;

  // Calculate Civil Day from the Modified Julian Day Number (MJD)
  // Method P.D-Smith: Practical Astronomy
  // Page 11: Paragraph 5: Converting Julian day number to the calendar date
  // See also Robin M. Green: Spherical Astronomy, page 250 and next

  // Correction factor is MJD (2,400,000.5) + 0.5 (17 Nov 1858 instead of 16 Nov 12:00 hours)
  double JD = (double)((m_value / ONE_DAY) + 2400001);
  if(JD > 2299160)
  {
    long gregorianA = (long) ((JD - 1867216.25) / 36524.25);
    gregorianB = (long) (JD + 1 + gregorianA - (gregorianA / 4));
  }
  else
  {
    gregorianB = (long) JD;
  }
  factorC = gregorianB + 1524;
  factorD = (long) (((double)factorC - 122.1) / 365.25);
  factorE = (long)  ((double)factorD * 365.25);
  factorG = (long) (((double)((size_t)factorC - (size_t)factorE)) / 30.6001);
  m_timestamp.m_day   = (char)   (factorC - factorE - (int)((double)factorG * 30.6001));
  m_timestamp.m_month = (char)  ((factorG > 13) ? factorG - 13 : factorG - 1);
  m_timestamp.m_year  = (short) ((m_timestamp.m_month > 2) ? factorD - 4716 : factorD - 4715);

  long seconden        =         m_value  % ONE_DAY;
  m_timestamp.m_hour   = (char) (seconden / ONE_HOUR);
  int rest             =         seconden % ONE_HOUR;
  m_timestamp.m_minute = (char) (rest     / ONE_MINUTE);
  m_timestamp.m_second = (char) (rest     % ONE_MINUTE);
  // Fraction not set from normalized value
  m_fraction = 0;

  // Validate our results
  Validate();
}

void
XMLTimestamp::Validate()
{
  // Validate by ODBC definition 
  // Timestamp >= { ts '1-1-1 0:0:0'}
  // Timestamp <= { ts '9999-31-12 23:59:61' }

  if(m_timestamp.m_year <= 0 || m_timestamp.m_year >= 10000)
  {
    m_value = 0;
    throw StdException(_T("Year must be between 1 and 9999 inclusive."));
  }
  if(m_timestamp.m_month <= 0 || m_timestamp.m_month >= 13)
  {
    m_value = 0;
    throw StdException(_T("Month must be between 1 and 12 inclusive."));
  }
  if(m_timestamp.m_hour < 0 || m_timestamp.m_hour >= 24)
  {
    m_value = 0;
    throw StdException(_T("Hour must be between 0 and 23 inclusive."));
  }
  if(m_timestamp.m_minute < 0 || m_timestamp.m_minute >= 60)
  {
    m_value = 0;
    throw StdException(_T("Minute must be between 0 and 59 inclusive."));
  }
  if(m_timestamp.m_second < 0 || m_timestamp.m_second >= 62)
  {
    m_value = 0;
    throw StdException(_T("Number of seconds must be between 0 and 61 inclusive."));
  }
  if(m_fraction < 0 || m_fraction > NANOSECONDS_PER_SEC)
  {
    m_value = 0;
    throw StdException(_T("Fraction of seconds must be between 0 and 999,999,999"));
  }
}

XMLTimestamp
XMLTimestamp::AddDays(int p_number) const
{
  if(p_number)
  {
    return XMLTimestamp((INT64)(m_value + ((INT64)p_number * ONE_DAY)));
  }
  return *this;
}

XMLTimestamp
XMLTimestamp::AddMinutes(int p_number) const
{
  if(p_number)
  {
    return XMLTimestamp((INT64)(m_value + (INT64)p_number * ONE_MINUTE));
  }
  return *this;
}

XString 
XMLTimestamp::AsString()
{
  XString string;
  string.Format(_T("%04d-%02d-%02dT%02d:%02d:%02d")
               ,m_timestamp.m_year
               ,m_timestamp.m_month
               ,m_timestamp.m_day
               ,m_timestamp.m_hour
               ,m_timestamp.m_minute
               ,m_timestamp.m_second);
  if(m_fraction)
  {
    string.AppendFormat(_T(".%d"),m_fraction);
    string.TrimRight(_T("0"));
  }
  return string;
}

void
XMLTimestamp::SetCurrentTimestamp(bool p_fraction)
{
  // Getting the current date-and-time
  SYSTEMTIME sys;
  ::GetLocalTime(&sys);

  long nanoseconds = 0;

  if(p_fraction)
  {
    // Getting high-resolution time in 100-nanosecond resolution
    FILETIME ftime;
    ::GetSystemTimeAsFileTime(&ftime);
    nanoseconds = (ftime.dwLowDateTime % 10000000);
  }
  // Construct timestamp
  SetTimestamp(sys.wYear,sys.wMonth, sys.wDay
              ,sys.wHour,sys.wMinute,sys.wSecond
              ,100 * nanoseconds);
}

void
XMLTimestamp::SetSystemTimestamp(bool p_fraction)
{
  // Getting the current date-and-time
  SYSTEMTIME sys;
  ::GetSystemTime(&sys);

  long nanoseconds = 0;

  if(p_fraction)
  {
    // Getting high-resolution time in 100-nanosecond resolution
    FILETIME ftime;
    ::GetSystemTimeAsFileTime(&ftime);
    nanoseconds = (ftime.dwLowDateTime % 10000000);
  }
  // Construct timestamp
  SetTimestamp(sys.wYear,sys.wMonth, sys.wDay
              ,sys.wHour,sys.wMinute,sys.wSecond
              ,100 * nanoseconds);

}

//////////////////////////////////////////////////////////////////////////
//
// XMLDuration
//
//////////////////////////////////////////////////////////////////////////

XMLDuration::XMLDuration(XString p_value)
            :XMLTemporal(p_value)
{
  ParseDuration(p_value);
}

// Parse an interval from a XML duration string
// a la: http://www.w3.org/TR/2012/REC-xmlschema11-2-20120405/datatypes.html#duration
bool
XMLDuration::ParseDuration(XString p_duration)
{
  bool  negative    = false;
  bool  didTime     = false;
  int   value       = 0;
  int   fraction    = 0;
  TCHAR marker      = 0;
  TCHAR firstMarker = 0;
  TCHAR lastMarker  = 0;
  SQLINTERVAL type;

  // Set the current interval to NULL
  m_value = 0;

  // Parse the negative sign
  p_duration.Trim();
  if(p_duration.Left(1) == _T("-"))
  {
    negative = true;
    p_duration = p_duration.Mid(1);
  }

  // Must see a 'P' for period
  if(p_duration.Left(1) != _T("P"))
  {
    return false; // Leave interval at NULL
  }
  p_duration = p_duration.Mid(1);

  // Scan year/month/day/hour/min/second/fraction values
  while(ScanDurationValue(p_duration,value,fraction,marker,didTime))
  {
    switch(marker)
    {
      case 'Y': m_interval.intval.year_month.year = value;
                break;
      case 'D': m_interval.intval.day_second.day  = value;
                break;
      case 'H': if(!didTime)
                {
                  throw StdException(_T("Illegal duriation period (hours without a 'T')"));
                }
                m_interval.intval.day_second.hour = value;
                break;
      case 'M': if(didTime)
                {
                  m_interval.intval.day_second.minute = value;
                  marker = 'm'; // minutes!
                }
                else
                {
                  m_interval.intval.year_month.month = value;
                }
                break;
      case 'S': if(!didTime)
                {
                  throw StdException(_T("Illegal duration period (seconds without a 'T')"));
                }
                m_interval.intval.day_second.second   = value;
                m_interval.intval.day_second.fraction = fraction;
                break;
      default:  // Illegal string, leave interval at NULL
                return false;
    }
    // Getting first/last marker
    lastMarker = marker;
    if(firstMarker == 0)
    {
      firstMarker = marker;
    }
  }

  // Finding the interval type
       if(firstMarker == 'Y' && lastMarker == 'Y') type = SQL_IS_YEAR;
  else if(firstMarker == 'M' && lastMarker == 'M') type = SQL_IS_MONTH;
  else if(firstMarker == 'D' && lastMarker == 'D') type = SQL_IS_DAY;
  else if(firstMarker == 'H' && lastMarker == 'H') type = SQL_IS_HOUR;
  else if(firstMarker == 'm' && lastMarker == 'm') type = SQL_IS_MINUTE;
  else if(firstMarker == 'S' && lastMarker == 'S') type = SQL_IS_SECOND;
  else if(firstMarker == 'Y' && lastMarker == 'M') type = SQL_IS_YEAR_TO_MONTH;
  else if(firstMarker == 'D' && lastMarker == 'H') type = SQL_IS_DAY_TO_HOUR;
  else if(firstMarker == 'D' && lastMarker == 'm') type = SQL_IS_DAY_TO_MINUTE;
  else if(firstMarker == 'D' && lastMarker == 'S') type = SQL_IS_DAY_TO_SECOND;
  else if(firstMarker == 'H' && lastMarker == 'm') type = SQL_IS_HOUR_TO_MINUTE;
  else if(firstMarker == 'H' && lastMarker == 'S') type = SQL_IS_HOUR_TO_SECOND;
  else if(firstMarker == 'm' && lastMarker == 'S') type = SQL_IS_MINUTE_TO_SECOND;
  else
  {
    // Beware: XML duration has combinations that are NOT compatible
    // with the SQL definition of an interval, like Month-to-Day
    XString error;
    error.Format(_T("XML duration period not compatible with SQL (%c to %c)"),firstMarker,lastMarker);
    throw StdException(error);
  }

  // Found everything: wrap up
  m_interval.interval_type = type;
  if(negative)
  {
    m_interval.interval_sign = 1;
  }
  Normalise();
  RecalculateValue();

  return true;
}

bool
XMLDuration::ScanDurationValue(XString& p_duration
                              ,int&     p_value
                              ,int&     p_fraction
                              ,TCHAR&   p_marker
                              ,bool&    p_didTime)
{
  // Reset values
  p_value  = 0;
  p_marker = 0;
  bool found = false;

  // Check for empty string
  if(p_duration.IsEmpty())
  {
    return false;
  }

  // Scan for beginning of time part
  if(p_duration.GetAt(0) == 'T')
  {
    p_didTime  = true;
    p_duration = p_duration.Mid(1);
  }

  // Scan a number
  while(isdigit(p_duration.GetAt(0)))
  {
    found = true;
    p_value *= 10;
    p_value += p_duration.GetAt(0) - '0';
    p_duration = p_duration.Mid(1);
  }

  if(p_duration.GetAt(0) == '.')
  {
    p_duration = p_duration.Mid(1);

    int frac = 9;
    while(isdigit(p_duration.GetAt(0)))
    {
      --frac;
      p_fraction *= 10;
      p_fraction += p_duration.GetAt(0) - '0';
      p_duration  = p_duration.Mid(1);
    }
    p_fraction *= (int) pow(10,frac);
  }

  // Scan a marker
  if(isalpha(p_duration.GetAt(0)))
  {
    p_marker = p_duration.GetAt(0);
    p_duration = p_duration.Mid(1);
  }

  // True if both found, and fraction only found for seconds
  return (p_fraction && p_marker == 'S') ||
         (p_fraction == 0 && found && p_marker > 0);
}

// Normalise the field values
void 
XMLDuration::Normalise()
{
  switch(m_interval.interval_type)
  {
    case SQL_IS_YEAR:             break; // Nothing to do
    case SQL_IS_MONTH:            break; // Nothing to do
    case SQL_IS_DAY:              break; // Nothing to do
    case SQL_IS_HOUR:             break; // Nothing to do
    case SQL_IS_MINUTE:           break; // Nothing to do
    case SQL_IS_SECOND:           if(m_interval.intval.day_second.fraction > NANOSECONDS_PER_SEC)
                                  {
                                    m_interval.intval.day_second.second   += m_interval.intval.day_second.fraction / NANOSECONDS_PER_SEC;
                                    m_interval.intval.day_second.fraction %= NANOSECONDS_PER_SEC;
                                  }
                                  break;
    case SQL_IS_YEAR_TO_MONTH:    if(m_interval.intval.year_month.month > 12)
                                  {
                                    m_interval.intval.year_month.year  += m_interval.intval.year_month.month / 12;
                                    m_interval.intval.year_month.month %= 12;
                                  }
                                  break;
    case SQL_IS_DAY_TO_HOUR:      if(m_interval.intval.day_second.hour > 24)
                                  {
                                    m_interval.intval.day_second.day  += m_interval.intval.day_second.hour / 24;
                                    m_interval.intval.day_second.hour %= 24;
                                  }
                                  break;
    case SQL_IS_DAY_TO_MINUTE:    if(m_interval.intval.day_second.minute > 60)
                                  {
                                    m_interval.intval.day_second.hour   += m_interval.intval.day_second.minute / 60;
                                    m_interval.intval.day_second.minute %= 60;
                                  }
                                  if(m_interval.intval.day_second.hour > 24)
                                  {
                                    m_interval.intval.day_second.day  += m_interval.intval.day_second.hour / 24;
                                    m_interval.intval.day_second.hour %= 24;
                                  }
                                  break;
    case SQL_IS_DAY_TO_SECOND:    if(m_interval.intval.day_second.fraction > NANOSECONDS_PER_SEC)
                                  {
                                    m_interval.intval.day_second.second   += m_interval.intval.day_second.fraction / NANOSECONDS_PER_SEC;
                                    m_interval.intval.day_second.fraction %= NANOSECONDS_PER_SEC;
                                  }
                                  if(m_interval.intval.day_second.second > 60)
                                  {
                                    m_interval.intval.day_second.minute += m_interval.intval.day_second.second / 60;
                                    m_interval.intval.day_second.second %= 60;
                                  }
                                  if(m_interval.intval.day_second.minute > 60)
                                  {
                                    m_interval.intval.day_second.hour   += m_interval.intval.day_second.minute / 60;
                                    m_interval.intval.day_second.minute %= 60;
                                  }
                                  if(m_interval.intval.day_second.hour > 24)
                                  {
                                    m_interval.intval.day_second.day  += m_interval.intval.day_second.hour / 24;
                                    m_interval.intval.day_second.hour %= 24;
                                  }
                                  break;
    case SQL_IS_HOUR_TO_MINUTE:   if(m_interval.intval.day_second.minute > 60)
                                  {
                                    m_interval.intval.day_second.hour   += m_interval.intval.day_second.minute / 60;
                                    m_interval.intval.day_second.minute %= 60;
                                  }
                                  break;
    case SQL_IS_HOUR_TO_SECOND:   if(m_interval.intval.day_second.fraction > NANOSECONDS_PER_SEC)
                                  {
                                    m_interval.intval.day_second.second   += m_interval.intval.day_second.fraction / NANOSECONDS_PER_SEC;
                                    m_interval.intval.day_second.fraction %= NANOSECONDS_PER_SEC;
                                  }
                                  if(m_interval.intval.day_second.second > 60)
                                  {
                                    m_interval.intval.day_second.minute += m_interval.intval.day_second.second / 60;
                                    m_interval.intval.day_second.second %= 60;
                                  }
                                  if(m_interval.intval.day_second.minute > 60)
                                  {
                                    m_interval.intval.day_second.hour   += m_interval.intval.day_second.minute / 60;
                                    m_interval.intval.day_second.minute %= 60;
                                  }
                                  break;
    case SQL_IS_MINUTE_TO_SECOND: if(m_interval.intval.day_second.fraction > NANOSECONDS_PER_SEC)
                                  {
                                    m_interval.intval.day_second.second   += m_interval.intval.day_second.fraction / NANOSECONDS_PER_SEC;
                                    m_interval.intval.day_second.fraction %= NANOSECONDS_PER_SEC;
                                  }
                                  if(m_interval.intval.day_second.second > 60)
                                  {
                                    m_interval.intval.day_second.minute += m_interval.intval.day_second.second / 60;
                                    m_interval.intval.day_second.second %= 60;
                                  }
                                  break;
    default:                      break;
  }
}

// Recalculate the total interval value
void 
XMLDuration::RecalculateValue()
{
  if(m_interval.interval_type == SQL_IS_YEAR  ||
     m_interval.interval_type == SQL_IS_MONTH ||
     m_interval.interval_type == SQL_IS_YEAR_TO_MONTH)
  {
    m_value = (INT64)m_interval.intval.year_month.year * 24 +
              (INT64)m_interval.intval.year_month.month;

  }
  else if(m_interval.interval_type > 0 &&
          m_interval.interval_type <= SQL_IS_MINUTE_TO_SECOND)
  {
    m_value = (INT64)m_interval.intval.day_second.day    * NANOSECONDS_PER_SEC * ONE_DAY  +
              (INT64)m_interval.intval.day_second.hour   * NANOSECONDS_PER_SEC * ONE_HOUR +
              (INT64)m_interval.intval.day_second.minute * NANOSECONDS_PER_SEC * ONE_MINUTE +
              (INT64)m_interval.intval.day_second.second * NANOSECONDS_PER_SEC +
              (INT64)m_interval.intval.day_second.fraction;
  }
  else
  {
    m_value = 0L;
  }
  if(m_interval.interval_sign)
  {
    m_value *= -1L;
  }
}

//////////////////////////////////////////////////////////////////////////
//
// XMLGregorianMD
//
//////////////////////////////////////////////////////////////////////////

XMLGregorianMD::XMLGregorianMD(XString p_value)
               :XMLTemporal(p_value)
{
  ParseGregorianMD(p_value);
}

void
XMLGregorianMD::ParseGregorianMD(XString p_value)
{
  int month = 0;
  int day   = 0;
  bool negative = false;

  p_value.Trim();
  if(p_value.GetAt(0) == '-')
  {
    negative = true;
    p_value = p_value.Mid(1);
  }
  int num = _stscanf_s(p_value,_T("%d-%d"),&month,&day);
  if(num != 2)
  {
    XString result = _T("Not a Gregorian month-day value: ") + p_value;
    throw StdException(result);
  }
  else if(month < 1 || month > 12 ||
          day   < 1 || day   > 31)
  {
    XString result = _T("Gregorian month-day overflow: ") + p_value;
    throw StdException(result);
  }
  m_value = (INT64)month * 31 + (INT64)day;
  if(negative)
  {
    m_value = - m_value;
  }
}

//////////////////////////////////////////////////////////////////////////
//
// XMLGregorianYM
//
//////////////////////////////////////////////////////////////////////////

XMLGregorianYM::XMLGregorianYM(XString p_value)
               :XMLTemporal(p_value)
{
  ParseGregorianYM(p_value);
}

void
XMLGregorianYM::ParseGregorianYM(XString p_value)
{
  p_value.Trim();
  XString result;
  int year  = 0;
  int month = 0;
  bool negative = false;

  if(p_value.GetAt(0) == '-')
  {
    negative = true;
    p_value = p_value.Mid(1);
  }

  int num = _stscanf_s(p_value,_T("%d-%d"),&year,&month);
  if(num != 2)
  {
    result = _T("Not a Gregorian year-month value: ") + p_value;
    throw StdException(result);
  }
  else if(year  < 0 || year  > 9999 ||
          month < 1 || month >   12  )
  {
    result = _T("Gregorian year-month overflow: ") + p_value;
    throw StdException(result);
  }
  m_value = (INT64)year * 12 + (INT64)month;
  if(negative)
  {
    m_value = - m_value;
  }
}
