/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: XMLRestriction.h
//
// Copyright (c) 2014-2021 ir. W.E. Huisman
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
#ifndef __XMLRESTRICTION__
#define __XMLRESTRICTION__
#include <map>
#include "bcd.h"
#include "XMLMessage.h"

using XmlEnums = std::map<CString,CString>;

class XMLRestriction
{
public:
  XMLRestriction(CString p_name);
  CString PrintRestriction(CString p_name);
  CString CheckRestriction(XmlDataType p_type,CString p_value);
  CString CheckDatatype   (XmlDataType p_type,CString p_value);
  CString HandleWhitespace(XmlDataType p_type,CString p_value);

  // Set restrictions
  void    AddEnumeration(CString p_enum,CString p_displayValue = "");
  void    AddBaseType(CString p_type)     { m_baseType       = p_type;   };
  void    AddLength(int p_length)         { m_length         = p_length; };
  void    AddMinLength(int p_length)      { m_minLength      = p_length; };
  void    AddMaxLength(int p_length)      { m_maxLength      = p_length; };
  void    AddTotalDigits(int p_digits)    { m_totalDigits    = p_digits; };
  void    AddFractionDigits(int p_digits) { m_fractionDigits = p_digits; };
  void    AddPattern(CString p_pattern)   { m_pattern        = p_pattern;};
  void    AddWhitespace(int p_white)      { m_whiteSpace     = p_white;  };
  void    AddMaxExclusive(CString p_max);
  void    AddMaxInclusive(CString p_max);
  void    AddMinExclusive(CString p_max);
  void    AddMinInclusive(CString p_max);

  // Get restrictions
  bool    HasEnumeration  (CString p_enum);
  CString GiveDisplayValue(CString p_enum);
  CString HasBaseType()                   { return m_baseType;       };
  int     HasLength()                     { return m_length;         };
  int     HasMaxLength()                  { return m_maxLength;      };
  int     HasMinLength()                  { return m_minLength;      };
  int     HasTotalDigits()                { return m_totalDigits;    };
  int     HasFractionDigits()             { return m_fractionDigits; };
  CString HasMaxExclusive()               { return m_maxExclusive;   };
  CString HasMaxInclusive()               { return m_maxInclusive;   };
  CString HasMinExclusive()               { return m_minExclusive;   };
  CString HasMinInclusive()               { return m_minInclusive;   };
  CString HasPattern()                    { return m_pattern;        };
  int     HasWhitespace()                 { return m_whiteSpace;     };

  // GETTERS
  CString   GetName()                     { return m_name;           };
  XmlEnums& GetEnumerations()             { return m_enums;          };
private:
  CString   PrintEnumRestriction   (CString p_name);
  CString   PrintIntegerRestriction(CString p_name,int p_value);
  CString   PrintStringRestriction (CString p_name,CString p_value);
  CString   PrintSpaceRestriction();

  // Checking the restrictions
  CString   CheckAnyURI   (CString p_value);
  CString   CheckInteger  (CString p_value);
  CString   CheckBoolean  (CString p_value);
  CString   CheckDate     (CString p_value);
  CString   CheckBase64   (CString p_value);
  CString   CheckNumber   (CString p_value,bool p_specials);
  CString   CheckDateTime (CString p_value,bool p_explicit);
  CString   CheckDatePart (CString p_value);
  CString   CheckDuration (CString p_value);
  CString   CheckYearMonth(CString p_value);
  CString   CheckDaySecond(CString p_value);
  CString   CheckTimePart (CString p_value); 
  CString   CheckTime     (CString p_value);
  CString   CheckTimeZone (CString p_value);
  CString   CheckStampPart(CString p_value);
  CString   CheckGregDay  (CString p_value);
  CString   CheckGregMonth(CString p_value);
  CString   CheckGregYear (CString p_value);
  CString   CheckGregMD   (CString p_value);
  CString   CheckGregYM   (CString p_value);
  CString   CheckHexBin   (CString p_value);
  CString   CheckLong     (CString p_value);
  CString   CheckShort    (CString p_value);
  CString   CheckByte     (CString p_value);
  CString   CheckNNegInt  (CString p_value);
  CString   CheckPosInt   (CString p_value);
  CString   CheckUnsLong  (CString p_value);
  CString   CheckUnsShort (CString p_value);
  CString   CheckUnsByte  (CString p_value);
  CString   CheckNonPosInt(CString p_value);
  CString   CheckNegInt   (CString p_value);
  CString   CheckNormal   (CString p_value);
  CString   CheckToken    (CString p_value);
  CString   CheckNMTOKEN  (CString p_value);
  CString   CheckName     (CString p_value);
  CString   CheckNCName   (CString p_value);
  CString   CheckQName    (CString p_value);
  CString   CheckNMTOKENS (CString p_value);
  CString   CheckNames    (CString p_value);
  CString   CheckDuration (CString p_value,int& p_type);
  bool      ScanDurationValue  (CString& p_duration,int& p_value,int& p_fraction,char& p_marker,bool& p_didTime);
  // Check max decimal/fraction digits
  CString   CheckTotalDigits   (CString p_value);
  CString   CheckFractionDigits(CString p_value);
  // Check ranges max/min exclusive/inclusive
  CString   CheckRangeFloat    (CString p_value);
  CString   CheckRangeDecimal  (CString p_value);
  CString   CheckRangeStamp    (CString p_timestamp);
  CString   CheckRangeDate     (CString p_date);
  CString   CheckRangeTime     (CString p_time);
  CString   CheckRangeDuration (CString p_duration);
  CString   CheckRangeGregYM   (CString p_yearmonth);
  CString   CheckRangeGregMD   (CString p_monthday);

  CString   m_name;                     // Name of the restriction
  CString   m_baseType;                 // Base XSD type of the restriction
  XmlEnums  m_enums;                    // All <enumeration>'s
  int       m_length         { 0   };   // Exact length of the field
  int       m_maxLength      { 0   };   // Max allowed length of the field
  int       m_minLength      { 0   };   // Min allowed length of the field
  int       m_totalDigits    { 0   };   // Exact number of digits allowed
  int       m_fractionDigits { 0   };   // Number of decimal places
  int       m_whiteSpace     { 0   };   // 1=preserve, 2=replace, 3=collapse
  CString   m_pattern;                  // Pattern for pattern matching
  CString   m_maxExclusive;             // Max value up-to     this value
  CString   m_maxInclusive;             // Max value including this value
  CString   m_minExclusive;             // Min value down-to   this value
  CString   m_minInclusive;             // Min value including this value
  // Redundant optimalisation
  bcd       m_maxExclusiveDouble;
  bcd       m_maxInclusiveDouble;
  bcd       m_minExclusiveDouble;
  bcd       m_minInclusiveDouble;
  INT64     m_maxExclusiveInteger { 0   };
  INT64     m_maxInclusiveInteger { 0   };
  INT64     m_minExclusiveInteger { 0   };
  INT64     m_minInclusiveInteger { 0   };
};

using AllRestrictions = std::map<CString,XMLRestriction>;

class XMLRestrictions
{
public:
  XMLRestriction* FindRestriction (CString p_name);
  XMLRestriction* AddRestriction  (CString p_name);
  void            AddEnumeration  (CString p_name,CString p_enum,CString p_displayValue = "");
  bool            HasEnumeration  (CString p_name,CString p_enum);
  CString         GiveDisplayValue(CString p_name,CString p_enum);

private:
  AllRestrictions m_restrictions;
};

#endif
