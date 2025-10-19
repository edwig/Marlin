/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: XMLRestriction.h
//
// Created: 2014-2025 ir. W.E. Huisman
// MIT License
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
  explicit XMLRestriction(const XString& p_name);
  XString PrintRestriction(const XString& p_name);
  XString CheckRestriction(XmlDataType p_type,const XString& p_value);
  XString CheckDatatype   (XmlDataType p_type,const XString& p_value);
  XString HandleWhitespace(XmlDataType p_type,      XString& p_value);

  // Set restrictions
  void    AddEnumeration(const XString& p_enum,const XString& p_displayValue = _T(""));
  void    AddBaseType(const XString& p_type)     { m_baseType       = p_type;   }
  void    AddLength(int p_length)                { m_length         = p_length; }
  void    AddMinLength(int p_length)             { m_minLength      = p_length; }
  void    AddMaxLength(int p_length)             { m_maxLength      = p_length; }
  void    AddTotalDigits(int p_digits)           { m_totalDigits    = p_digits; }
  void    AddFractionDigits(int p_digits)        { m_fractionDigits = p_digits; }
  void    AddPattern(const XString& p_pattern)   { m_pattern        = p_pattern;}
  void    AddWhitespace(int p_white)             { m_whiteSpace     = p_white;  }
  void    AddMaxExclusive(const XString& p_max);
  void    AddMaxInclusive(const XString& p_max);
  void    AddMinExclusive(const XString& p_max);
  void    AddMinInclusive(const XString& p_max);
  void    AddMinOccurs(const XString& p_min);
  void    AddMaxOccurs(const XString& p_max);

  // Get restrictions
  bool    HasEnumeration  (const XString& p_enum);
  XString GiveDisplayValue(const XString& p_enum);
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
  XString   PrintEnumRestriction   (const XString& p_name);
  XString   PrintIntegerRestriction(const XString& p_name,int p_value);
  XString   PrintStringRestriction (const XString& p_name,const XString& p_value);
  XString   PrintSpaceRestriction();

  // Checking the restrictions
  XString   CheckAnyURI   (const XString& p_value);
  XString   CheckInteger  (const XString& p_value);
  XString   CheckBoolean  (const XString& p_value);
  XString   CheckDate     (const XString& p_value);
  XString   CheckBase64   (const XString& p_value);
  XString   CheckNumber   (const XString& p_value,bool p_specials);
  XString   CheckDateTime (const XString& p_value,bool p_explicit);
  XString   CheckDatePart (const XString& p_value);
  XString   CheckDuration (const XString& p_value);
  XString   CheckYearMonth(const XString& p_value);
  XString   CheckDaySecond(const XString& p_value);
  XString   CheckTimePart (const XString& p_value);
  XString   CheckTime     (const XString& p_value);
  XString   CheckTimeZone (const XString& p_value);
  XString   CheckStampPart(const XString& p_value);
  XString   CheckGregDay  (const XString& p_value);
  XString   CheckGregMonth(const XString& p_value);
  XString   CheckGregYear (const XString& p_value);
  XString   CheckGregMD   (const XString& p_value);
  XString   CheckGregYM   (const XString& p_value);
  XString   CheckHexBin   (const XString& p_value);
  XString   CheckLong     (const XString& p_value);
  XString   CheckShort    (const XString& p_value);
  XString   CheckByte     (const XString& p_value);
  XString   CheckNNegInt  (const XString& p_value);
  XString   CheckPosInt   (const XString& p_value);
  XString   CheckUnsLong  (const XString& p_value);
  XString   CheckUnsShort (const XString& p_value);
  XString   CheckUnsByte  (const XString& p_value);
  XString   CheckNonPosInt(const XString& p_value);
  XString   CheckNegInt   (const XString& p_value);
  XString   CheckNormal   (const XString& p_value);
  XString   CheckToken    (const XString& p_value);
  XString   CheckNMTOKEN  (const XString& p_value);
  XString   CheckName     (const XString& p_value);
  XString   CheckNCName   (const XString& p_value);
  XString   CheckQName    (const XString& p_value);
  XString   CheckNMTOKENS (const XString& p_value);
  XString   CheckNames    (const XString& p_value);
  XString   CheckDuration (const XString& p_value,int& p_type);
  bool      ScanDurationValue  (const XString& p_duration,int& p_value,int& p_fraction,TCHAR& p_marker,bool& p_didTime);
  // Check max decimal/fraction digits
  XString   CheckTotalDigits   (const XString& p_value);
  XString   CheckFractionDigits(const XString& p_value);
  // Check ranges max/min exclusive/inclusive
  XString   CheckRangeFloat    (const XString& p_value);
  XString   CheckRangeDecimal  (const XString& p_value);
  XString   CheckRangeStamp    (const XString& p_timestamp);
  XString   CheckRangeDate     (const XString& p_date);
  XString   CheckRangeTime     (const XString& p_time);
  XString   CheckRangeDuration (const XString& p_duration);
  XString   CheckRangeGregYM   (const XString& p_yearmonth);
  XString   CheckRangeGregMD   (const XString& p_monthday);

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
  XMLRestriction* FindRestriction (const XString& p_name);
  XMLRestriction* AddRestriction  (const XString& p_name);
  void            AddEnumeration  (const XString& p_name,const XString& p_enum,const XString& p_displayValue = _T(""));
  bool            HasEnumeration  (const XString& p_name,const XString& p_enum);
  XString         GiveDisplayValue(const XString& p_name,const XString& p_enum);

private:
  AllRestrictions m_restrictions;
};
