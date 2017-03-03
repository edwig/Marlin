/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: XMLTemporal.h
//
// Marlin Server: Internet server/client
// 
// Copyright (c) 2017 ir. W.E. Huisman
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
#pragma once
#include <sqltypes.h>

#define ONE_MINUTE (60)        // One minute in seconds
#define ONE_HOUR   (60*60)     // One hour in seconds
#define ONE_DAY    (24*60*60)  // One day in seconds
#define NANOSECONDS_PER_SEC 1000

// Storage of the time in hours,minutes & seconds
struct TimeStorage
{
  int m_hour;
  int m_minute;
  int m_second;
};

// Date in memory is a struct
typedef struct _Date
{
  short m_year;   // 0 - 9999 jaren
  char  m_month;  // 1 - 12   maanden
  char  m_day;    // 1 - 31   dagen
}
DateStorage;

struct StampStorage
{
  short m_year;      // 1 - 9999  year
  char  m_month;     // 1 - 12    months
  char  m_day;       // 1 - 31    days
  char  m_hour;      // 0 - 23    hour
  char  m_minute;    // 0 - 59    minute
  char  m_second;    // 0 - 59    seconds
};

class XMLTimestamp;

//////////////////////////////////////////////////////////////////////////
// GENERAL BASE CLASS

class XMLTemporal
{
public:
  XMLTemporal(CString p_value);

  CString GetString() { return m_string; };
  INT64   GetValue()  { return m_value;  };

  // XML Temporal parsing
  bool    ParseXMLDate(const CString& p_string,XMLTimestamp& p_moment);

protected:
  CString m_string;
  INT64   m_value     { NULL };
};

//////////////////////////////////////////////////////////////////////////
// TIME

class XMLTime : public XMLTemporal
{
public:
  XMLTime(CString p_value);
  int Hour()   { return m_theTime.m_hour;   };
  int Minute() { return m_theTime.m_minute; };
  int Second() { return m_theTime.m_second; };
private:
  void ParseTime(CString p_value);
  bool ParseXMLTime(const CString& p_string);
  void SetTime(int p_hour,int p_minute,int p_second);
  void SetTime();
  void Normalise();

  TimeStorage m_theTime;
};

//////////////////////////////////////////////////////////////////////////
// DATE

class XMLDate : public XMLTemporal
{
public:
  XMLDate();
  XMLDate(CString p_value);
  int  Year()  { return m_date.m_year;  };
  int  Month() { return m_date.m_month; };
  int  Day()   { return m_date.m_day;   };
  void Today();
private:
  bool ParseDate(CString p_value);
  bool ParseDate(const CString& p_datum,int* p_jaar,int* p_maand,int* p_dag);
  bool SetDate(int p_year,int p_month,int p_day);
  bool SetMJD();

  DateStorage m_date;
};

//////////////////////////////////////////////////////////////////////////
// TIMESTAMP

class XMLTimestamp : public XMLTemporal
{
public:
  XMLTimestamp();
  XMLTimestamp(CString p_value);
  XMLTimestamp(INT64   p_value);
  void SetTimestamp(int p_year,int p_month,int p_day
                   ,int p_hour,int p_minute,int p_second
                   ,int p_fraction = 0);
  void SetFraction(int p_fraction) { m_fraction = p_fraction; };
  XMLTimestamp AddDays   (int p_number) const;
  XMLTimestamp AddMinutes(int p_number) const;

  int  Year()  { return m_timestamp.m_year;  };
  int  Month() { return m_timestamp.m_month; };
  int  Day()   { return m_timestamp.m_day;   };
private:
  void ParseMoment(CString p_value);
  void RecalculateValue();
  void Normalise();
  void Validate();

  StampStorage m_timestamp;
  int          m_fraction;
};

//////////////////////////////////////////////////////////////////////////
// XMLDuration

class XMLDuration : public XMLTemporal
{
public:
  XMLDuration(CString p_value);
private:
  bool ParseDuration(CString p_value);
  // Parsing/scanning one value of a XML duration string
  bool ScanDurationValue(CString& p_duraction,int& p_value,int& p_fraction,char& p_marker,bool& p_didTime);
  void Normalise();
  void RecalculateValue();

  // The one and only interval
  SQL_INTERVAL_STRUCT m_interval;
};


//////////////////////////////////////////////////////////////////////////
// XMLGregorianMD

class XMLGregorianMD : public XMLTemporal
{
public:
  XMLGregorianMD(CString p_value);
private:
  void ParseGregorianMD(CString p_value);
};

//////////////////////////////////////////////////////////////////////////
// XMLGregorianYM

class XMLGregorianYM: public XMLTemporal
{
public:
  XMLGregorianYM(CString p_value);
private:
  void ParseGregorianYM(CString p_value);
};

