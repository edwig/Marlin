/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: XMLRestriction.h
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
#pragma once
#include <map>
#include "XMLDataType.h"

using XmlEnums = std::map<XString,XString>;

class XMLRestriction
{
public:
  explicit XMLRestriction(XString p_name);
  XString PrintRestriction(XString p_name);
  XString CheckRestriction(XmlDataType p_type,XString p_value);
  XString CheckDatatype   (XmlDataType p_type,XString p_value);
  XString HandleWhitespace(XmlDataType p_type,XString p_value);

  // Set restrictions
  void    AddEnumeration(XString p_enum,XString p_displayValue = _T(""));
  void    AddBaseType(XString p_type)     { m_baseType       = p_type;   }
  void    AddLength(int p_length)         { m_length         = p_length; }
  void    AddMinLength(int p_length)      { m_minLength      = p_length; }
  void    AddMaxLength(int p_length)      { m_maxLength      = p_length; }
  void    AddTotalDigits(int p_digits)    { m_totalDigits    = p_digits; }
  void    AddFractionDigits(int p_digits) { m_fractionDigits = p_digits; }
  void    AddPattern(XString p_pattern)   { m_pattern        = p_pattern;}
  void    AddWhitespace(int p_white)      { m_whiteSpace     = p_white;  }
  void    AddMaxExclusive(XString p_max);
  void    AddMaxInclusive(XString p_max);
  void    AddMinExclusive(XString p_max);
  void    AddMinInclusive(XString p_max);
  void    AddMinOccurs(XString p_min);
  void    AddMaxOccurs(XString p_max);

  // Get restrictions
  bool    HasEnumeration  (XString p_enum);
  XString GiveDisplayValue(XString p_enum);
  XString HasBaseType()                   { return m_baseType;       }
  int     HasLength()                     { return m_length;         }
  int     HasMaxLength()                  { return m_maxLength;      }
  int     HasMinLength()                  { return m_minLength;      }
  int     HasTotalDigits()                { return m_totalDigits;    }
  int     HasFractionDigits()             { return m_fractionDigits; }
  XString HasMaxExclusive()               { return m_maxExclusive;   }
  XString HasMaxInclusive()               { return m_maxInclusive;   }
  XString HasMinExclusive()               { return m_minExclusive;   }
  XString HasMinInclusive()               { return m_minInclusive;   }
  XString HasPattern()                    { return m_pattern;        }
  int     HasWhitespace()                 { return m_whiteSpace;     }
 unsigned HasMinOccurs()                  { return m_minOccurs;      }
 unsigned HasMaxOccurs()                  { return m_maxOccurs;      }

  // GETTERS
  XString   GetName()                     { return m_name;           };
  XmlEnums& GetEnumerations()             { return m_enums;          };
private:
  XString   PrintEnumRestriction   (XString p_name);
  XString   PrintIntegerRestriction(XString p_name,int p_value);
  XString   PrintStringRestriction (XString p_name,XString p_value);
  XString   PrintSpaceRestriction();

  // Checking the restrictions
  XString   CheckAnyURI   (XString p_value);
  XString   CheckInteger  (XString p_value);
  XString   CheckBoolean  (XString p_value);
  XString   CheckDate     (XString p_value);
  XString   CheckBase64   (XString p_value);
  XString   CheckNumber   (XString p_value,bool p_specials);
  XString   CheckDateTime (XString p_value,bool p_explicit);
  XString   CheckDatePart (XString p_value);
  XString   CheckDuration (XString p_value);
  XString   CheckYearMonth(XString p_value);
  XString   CheckDaySecond(XString p_value);
  XString   CheckTimePart (XString p_value); 
  XString   CheckTime     (XString p_value);
  XString   CheckTimeZone (XString p_value);
  XString   CheckStampPart(XString p_value);
  XString   CheckGregDay  (XString p_value);
  XString   CheckGregMonth(XString p_value);
  XString   CheckGregYear (XString p_value);
  XString   CheckGregMD   (XString p_value);
  XString   CheckGregYM   (XString p_value);
  XString   CheckHexBin   (XString p_value);
  XString   CheckLong     (XString p_value);
  XString   CheckShort    (XString p_value);
  XString   CheckByte     (XString p_value);
  XString   CheckNNegInt  (XString p_value);
  XString   CheckPosInt   (XString p_value);
  XString   CheckUnsLong  (XString p_value);
  XString   CheckUnsShort (XString p_value);
  XString   CheckUnsByte  (XString p_value);
  XString   CheckNonPosInt(XString p_value);
  XString   CheckNegInt   (XString p_value);
  XString   CheckNormal   (XString p_value);
  XString   CheckToken    (XString p_value);
  XString   CheckNMTOKEN  (XString p_value);
  XString   CheckName     (XString p_value);
  XString   CheckNCName   (XString p_value);
  XString   CheckQName    (XString p_value);
  XString   CheckNMTOKENS (XString p_value);
  XString   CheckNames    (XString p_value);
  XString   CheckDuration (XString p_value,int& p_type);
  bool      ScanDurationValue  (XString& p_duration,int& p_value,int& p_fraction,TCHAR& p_marker,bool& p_didTime);
  // Check max decimal/fraction digits
  XString   CheckTotalDigits   (XString p_value);
  XString   CheckFractionDigits(XString p_value);
  // Check ranges max/min exclusive/inclusive
  XString   CheckRangeFloat    (XString p_value);
  XString   CheckRangeDecimal  (XString p_value);
  XString   CheckRangeStamp    (XString p_timestamp);
  XString   CheckRangeDate     (XString p_date);
  XString   CheckRangeTime     (XString p_time);
  XString   CheckRangeDuration (XString p_duration);
  XString   CheckRangeGregYM   (XString p_yearmonth);
  XString   CheckRangeGregMD   (XString p_monthday);

  XString   m_name;                     // Name of the restriction
  XString   m_baseType;                 // Base XSD type of the restriction
  XmlEnums  m_enums;                    // All <enumeration>'s
  int       m_length         { 0   };   // Exact length of the field
  int       m_maxLength      { 0   };   // Max allowed length of the field
  int       m_minLength      { 0   };   // Min allowed length of the field
  int       m_totalDigits    { 0   };   // Exact number of digits allowed
  int       m_fractionDigits { 0   };   // Number of decimal places
  int       m_whiteSpace     { 0   };   // 1=preserve, 2=replace, 3=collapse
  unsigned  m_minOccurs      { 1   };   // Minimum number of child elements
  unsigned  m_maxOccurs      { 1   };   // Maximum number of child elements
  XString   m_pattern;                  // Pattern for pattern matching
  XString   m_maxExclusive;             // Max value up-to     this value
  XString   m_maxInclusive;             // Max value including this value
  XString   m_minExclusive;             // Min value down-to   this value
  XString   m_minInclusive;             // Min value including this value
  // Redundant optimalization
  bcd       m_maxExclusiveDouble;
  bcd       m_maxInclusiveDouble;
  bcd       m_minExclusiveDouble;
  bcd       m_minInclusiveDouble;
  INT64     m_maxExclusiveInteger { 0   };
  INT64     m_maxInclusiveInteger { 0   };
  INT64     m_minExclusiveInteger { 0   };
  INT64     m_minInclusiveInteger { 0   };
};

using AllRestrictions = std::map<XString,XMLRestriction>;

class XMLRestrictions
{
public:
  XMLRestriction* FindRestriction (XString p_name);
  XMLRestriction* AddRestriction  (XString p_name);
  void            AddEnumeration  (XString p_name,XString p_enum,XString p_displayValue = _T(""));
  bool            HasEnumeration  (XString p_name,XString p_enum);
  XString         GiveDisplayValue(XString p_name,XString p_enum);

private:
  AllRestrictions m_restrictions;
};
