/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: XMLRestriction.cpp
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
#include "XMLRestriction.h"
#include "XMLTemporal.h"
#include "CrackURL.h"
#include <stdint.h>
#include <regex>

#ifdef _AFX
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#endif

XMLRestriction::XMLRestriction(XString p_name)
               :m_name(p_name)
{
}

// SETTING RESTRICTIONS

void
XMLRestriction::AddEnumeration(XString p_enum,XString p_displayValue /*=""*/)
{
  if(!HasEnumeration(p_enum))
  {
    m_enums.insert(std::make_pair(p_enum,p_displayValue));
  }
}

void
XMLRestriction::AddMaxExclusive(XString p_max) 
{ 
  m_maxExclusive        = p_max; 
  m_maxExclusiveDouble  = p_max.GetString();
  m_maxExclusiveInteger = _ttoi64(p_max);
}

void
XMLRestriction::AddMaxInclusive(XString p_max)
{ 
  m_maxInclusive        = p_max; 
  m_maxInclusiveDouble  = p_max;
  m_maxInclusiveInteger = _ttoi64(p_max);
}

void
XMLRestriction::AddMinExclusive(XString p_max)
{ 
  m_minExclusive        = p_max; 
  m_minExclusiveDouble  = p_max;
  m_minExclusiveInteger = _ttoi64(p_max);
}

void
XMLRestriction::AddMinInclusive(XString p_max)
{ 
  m_minInclusive        = p_max; 
  m_minInclusiveDouble  = p_max;
  m_minInclusiveInteger = _ttoi64(p_max);
}

void
XMLRestriction::AddMinOccurs(XString p_min)
{
  m_minOccurs = (unsigned) _ttoll(p_min);
  if(m_minOccurs > m_maxOccurs)
  {
    m_maxOccurs = m_minOccurs;
  }
}

void
XMLRestriction::AddMaxOccurs(XString p_max)
{
  if(p_max.Compare(_T("unbounded")) == 0)
  {
    m_maxOccurs = UINT_MAX;
    return;
  }
  m_maxOccurs = (unsigned) _ttoll(p_max);
  if(m_maxOccurs < m_minOccurs)
  {
    m_minOccurs = m_maxOccurs;
  }
}

bool 
XMLRestriction::HasEnumeration(XString p_enum)
{
  return m_enums.find(p_enum) != m_enums.end();
}

XString 
XMLRestriction::GiveDisplayValue(XString p_enum)
{
  XmlEnums::iterator it = m_enums.find(p_enum);
  if(it != m_enums.end())
  {
    return it->second;
  }
  return _T("");
}

// Print restriction as XML comment node
// <!--datatype has values: M (='Male customer') / F (='Female customer') / U (='Unknown')-->
// <!--datatype has values: M / F / U-->
XString
XMLRestriction::PrintEnumRestriction(XString p_name)
{
  bool print = false;
  XString restriction;
  restriction.Format(_T("<!--%s has values: "),p_name.GetString());

  for(auto& enumvalue : m_enums)
  {
    if(print) restriction += _T(" / ");
    restriction += enumvalue.first;
    if(!enumvalue.second.IsEmpty())
    {
      restriction += _T(" (='");
      restriction += enumvalue.second;
      restriction += _T("')");
    }
    print = true;
  }

  restriction += _T("-->");
  return restriction;
}

// <!--maxLength: 20-->
XString
XMLRestriction::PrintIntegerRestriction(XString p_name,int p_value)
{
  XString restriction;
  restriction.Format(_T("<!--%s: %d-->"),p_name.GetString(),p_value);
  return restriction;
}

// <!--pattern: ab*-->
XString
XMLRestriction::PrintStringRestriction(XString p_name,XString p_value)
{
  XString restriction;
  restriction.Format(_T("<!--%s: %s-->"),p_name.GetString(),p_value.GetString());
  return restriction;
}

// <!--whitespace: collapse-->
XString
XMLRestriction::PrintSpaceRestriction()
{
  XString value;

  switch(m_whiteSpace)
  {
    case 1: value = _T("preserve"); break;
    case 2: value = _T("replace");  break;
    case 3: value = _T("collapse"); break;
  }
  return PrintStringRestriction(_T("whitespace"),value);
}

XString
XMLRestriction::PrintRestriction(XString p_name)
{
  XString restriction;

  // Print base type
  if(!m_baseType.IsEmpty()) restriction += PrintStringRestriction(_T("base"),m_baseType);
  // All integer restrictions
  if(m_length)         restriction += PrintIntegerRestriction(_T("length (EXACT)"),m_length);
  if(m_maxLength)      restriction += PrintIntegerRestriction(_T("maxLength"),     m_maxLength);
  if(m_minLength)      restriction += PrintIntegerRestriction(_T("minLength"),     m_minLength);
  if(m_totalDigits)    restriction += PrintIntegerRestriction(_T("totalDigits"),   m_totalDigits);
  if(m_fractionDigits) restriction += PrintIntegerRestriction(_T("fractionDigits"),m_fractionDigits);
  // All string restrictions
  if(!m_maxExclusive.IsEmpty()) restriction += PrintStringRestriction(_T("maxExclusive"),m_maxExclusive);
  if(!m_maxInclusive.IsEmpty()) restriction += PrintStringRestriction(_T("maxInclusive"),m_maxInclusive);
  if(!m_minExclusive.IsEmpty()) restriction += PrintStringRestriction(_T("minExclusive"),m_minExclusive);
  if(!m_minInclusive.IsEmpty()) restriction += PrintStringRestriction(_T("minInclusive"),m_minInclusive);
  if(!m_pattern     .IsEmpty()) restriction += PrintStringRestriction(_T("pattern"),     m_pattern);
  // Whitespace handling
  if(m_whiteSpace) 
  {
    restriction += PrintSpaceRestriction();
  }
  // Print enumerations
  if(!m_enums.empty())
  {
    restriction += PrintEnumRestriction(p_name);
  }
  return restriction;
}

// If whitespace not given and it's not a string -> Collapse
// If whitespace not given and it's a string -> preserve
// Otherwise: use whitespace
XString 
XMLRestriction::HandleWhitespace(XmlDataType p_type,XString p_value)
{
  if(m_whiteSpace == 0)
  {
    // If "whiteSpace" attribute is not given
    if(p_type == XDT_String)
    {
      return p_value;
    }
    else
    {
      // Collapse
      p_value.Replace('\t',' ');
      p_value.Replace('\r',' ');
      p_value.Replace('\n',' ');
      p_value.Replace(_T("  "),_T(" "));
      p_value = p_value.Trim();
    }
  }
  else
  {
    // In case we have a whiteSpace attribute
    // Preserve = 1 -> Do nothing
    if(m_whiteSpace >= 2)
    {
      // Replace and collapse
      p_value.Replace('\t',' ');
      p_value.Replace('\r',' ');
      p_value.Replace('\n',' ');
    }
    // Full collapse
    if(m_whiteSpace == 3)
    {
      p_value.Replace(_T("  "),_T(" "));
      p_value = p_value.Trim();
    }
  }
  return p_value;
}

//////////////////////////////////////////////////////////////////////////
//
//  RESTRICTIONS FOR A CLASS OF MESSAGES. I.E. a WSDL registration file
//
//////////////////////////////////////////////////////////////////////////

XMLRestriction* 
XMLRestrictions::FindRestriction(XString p_name)
{
  AllRestrictions::iterator it = m_restrictions.find(p_name);
  if(it != m_restrictions.end())
  {
    return &(it->second);
  }
  return nullptr;
}

XMLRestriction* 
XMLRestrictions::AddRestriction(XString p_name)
{
  XMLRestriction* res = FindRestriction(p_name);
  if(res == nullptr)
  {
    // Insert
    XMLRestriction rest(p_name);
    m_restrictions.insert(std::make_pair(p_name,rest));
    // Re-Find
    res = FindRestriction(p_name);
  }
  return res;
}

void
XMLRestrictions::AddEnumeration(XString p_name,XString p_enum,XString p_displayValue /*=""*/)
{
  XMLRestriction* res = FindRestriction(p_name);
  if(res)
  {
    res->AddEnumeration(p_enum,p_displayValue);
  }
}

bool            
XMLRestrictions::HasEnumeration(XString p_name,XString p_enum)
{
  XMLRestriction* res = FindRestriction(p_name);
  if(res)
  {
    return res->HasEnumeration(p_enum);
  }
  return false;
}

XString
XMLRestrictions::GiveDisplayValue(XString p_name,XString /*p_enum*/)
{
  XMLRestriction* res = FindRestriction(p_name);
  if(res)
  {
    return res->GiveDisplayValue(p_name);
  }
  return _T("");
}

//////////////////////////////////////////////////////////////////////////
//
// CHECKING A RESTRICTION
//
//////////////////////////////////////////////////////////////////////////

// Check ranges max/min exclusive/inclusive
XString   
XMLRestriction::CheckRangeFloat(XString p_value)
{
  double d = _ttof(p_value);
  if(errno == ERANGE)
  {
    return _T("Floating point overflow");
  }
  bcd value(d);

  if(!m_minInclusive.IsEmpty())
  {
    if(value < m_minInclusiveDouble)
    {
      return _T("Value too small. < minInclusive");
    }
  }
  if(!m_minExclusive.IsEmpty())
  {
    if(value <= m_minExclusiveDouble)
    {
      return _T("Value too small. <= minExclusive");
    }
  }
  if(!m_maxInclusive.IsEmpty())
  {
    if(value > m_maxInclusiveDouble)
    {
      return _T("Value too big. > maxInclusive");
    }
  }
  if(!m_maxExclusive.IsEmpty())
  {
    if(value >= m_maxExclusiveDouble)
    {
      return _T("Value too big. >= maxExclusive");
    }
  }
  return _T("");
}

XString   
XMLRestriction::CheckRangeDecimal(XString p_value)
{
  _set_errno(0);
  INT64 value = _ttoi64(p_value.GetString());
  if(errno == ERANGE)
  {
    return _T("Numeric overflow");
  }

  if(!m_minInclusive.IsEmpty())
  {
    if(value < m_minInclusiveInteger)
    {
      return _T("Value too small. < minInclusive");
    }
  }
  if(!m_minExclusive.IsEmpty())
  {
    if(value <= m_minExclusiveInteger)
    {
      return _T("Value too small. <= minExclusive");
    }
  }
  if(!m_maxInclusive.IsEmpty())
  {
    if(value > m_maxInclusiveInteger)
    {
      return _T("Value too big. > maxInclusive");
    }
  }
  if(!m_maxExclusive.IsEmpty())
  {
    if(value >= m_maxExclusiveInteger)
    {
      return _T("Value too big. >= maxExclusive");
    }
  }
  return _T("");
}

XString
XMLRestriction::CheckRangeTime(XString p_time)
{
  if(!m_minInclusive.IsEmpty())
  {
    INT64 min = XMLTime(m_minInclusive).GetValue();
    INT64 val = XMLTime(p_time).GetValue();
    if(val < min) return _T("Value too small. < minInclusive");
  }
  if(!m_minExclusive.IsEmpty())
  {
    INT64 min = XMLTime(m_minExclusive).GetValue();
    INT64 val = XMLTime(p_time).GetValue();
    if(val <= min) return _T("Value too small. <= minExclusive");
  }
  if(!m_maxInclusive.IsEmpty())
  {
    INT64 max = XMLTime(m_maxInclusive).GetValue();
    INT64 val = XMLTime(p_time).GetValue();
    if(val > max) return _T("Value too big. > maxInclusive");
  }
  if(!m_maxExclusive.IsEmpty())
  {
    INT64 max = XMLTime(m_maxExclusive).GetValue();
    INT64 val = XMLTime(p_time).GetValue();
    if(val > max) return _T("Value too big. >= maxExclusive");
  }
  return _T("");
}

XString
XMLRestriction::CheckRangeDate(XString p_date)
{
  if(!m_minInclusive.IsEmpty())
  {
    INT64 min = XMLDate(m_minInclusive).GetValue();
    INT64 val = XMLDate(p_date).GetValue();
    if(val < min) return _T("Value too small. < minInclusive");
  }
  if(!m_minExclusive.IsEmpty())
  {
    INT64 min = XMLDate(m_minExclusive).GetValue();
    INT64 val = XMLDate(p_date).GetValue();
    if(val <= min) return _T("Value too small. <= minExclusive");
  }
  if(!m_maxInclusive.IsEmpty())
  {
    INT64 max = XMLDate(m_maxInclusive).GetValue();
    INT64 val = XMLDate(p_date).GetValue();
    if(val > max) return _T("Value too big. > maxInclusive");
  }
  if(!m_maxExclusive.IsEmpty())
  {
    INT64 max = XMLDate(m_maxExclusive).GetValue();
    INT64 val = XMLDate(p_date).GetValue();
    if(val > max) return _T("Value too big. >= maxExclusive");
  }
  return _T("");
}

XString   
XMLRestriction::CheckRangeStamp(XString p_timestamp)
{
  if(!m_minInclusive.IsEmpty())
  {
    INT64 min = XMLTimestamp(m_minInclusive).GetValue();
    INT64 val = XMLTimestamp(p_timestamp).GetValue();
    if(val < min) return _T("Value too small. < minInclusive");
  }
  if(!m_minExclusive.IsEmpty())
  {
    INT64 min = XMLTimestamp(m_minExclusive).GetValue();
    INT64 val = XMLTimestamp(p_timestamp).GetValue();
    if(val <= min) return _T("Value too small. <= minExclusive");
  }
  if(!m_maxInclusive.IsEmpty())
  {
    INT64 max = XMLTimestamp(m_maxInclusive).GetValue();
    INT64 val = XMLTimestamp(p_timestamp).GetValue();
    if(val > max) return _T("Value too big. > maxInclusive");
  }
  if(!m_maxExclusive.IsEmpty())
  {
    INT64 max = XMLTimestamp(m_maxExclusive).GetValue();
    INT64 val = XMLTimestamp(p_timestamp).GetValue();
    if(val > max) return _T("Value too big. >= maxExclusive");
  }
  return _T("");
}

XString
XMLRestriction::CheckRangeDuration(XString p_duration)
{
  if(!m_minInclusive.IsEmpty())
  {
    INT64 min = XMLDuration(m_minInclusive).GetValue();
    INT64 val = XMLDuration(p_duration).GetValue();
    if(val < min) return _T("Value too small. < minInclusive");
  }
  if(!m_minExclusive.IsEmpty())
  {
    INT64 min = XMLDuration(m_minExclusive).GetValue();
    INT64 val = XMLDuration(p_duration).GetValue();
    if(val <= min) return _T("Value too small. <= minExclusive");
  }
  if(!m_maxInclusive.IsEmpty())
  {
    INT64 max = XMLDuration(m_maxInclusive).GetValue();
    INT64 val = XMLDuration(p_duration).GetValue();
    if(val > max) return _T("Value too big. > maxInclusive");
  }
  if(!m_maxExclusive.IsEmpty())
  {
    INT64 max = XMLDuration(m_maxExclusive).GetValue();
    INT64 val = XMLDuration(p_duration).GetValue();
    if(val > max) return _T("Value too big. >= maxExclusive");
  }
  return _T("");
}

XString   
XMLRestriction::CheckRangeGregYM(XString p_yearmonth)
{
  if(!m_minInclusive.IsEmpty())
  {
    INT64 min = XMLGregorianYM(m_minInclusive).GetValue();
    INT64 val = XMLGregorianYM(p_yearmonth).GetValue();
    if(val < min) return _T("Value too small. < minInclusive");
  }
  if(!m_minExclusive.IsEmpty())
  {
    INT64 min = XMLGregorianYM(m_minExclusive).GetValue();
    INT64 val = XMLGregorianYM(p_yearmonth).GetValue();
    if(val <= min) return _T("Value too small. <= minExclusive");
  }
  if(!m_maxInclusive.IsEmpty())
  {
    INT64 max = XMLGregorianYM(m_maxInclusive).GetValue();
    INT64 val = XMLGregorianYM(p_yearmonth).GetValue();
    if(val > max) return _T("Value too big. > maxInclusive");
  }
  if(!m_maxExclusive.IsEmpty())
  {
    INT64 max = XMLGregorianYM(m_maxExclusive).GetValue();
    INT64 val = XMLGregorianYM(p_yearmonth).GetValue();
    if(val >= max) return _T("Value too big. >= maxExclusive");
  }
  return _T("");
}

XString
XMLRestriction::CheckRangeGregMD(XString p_monthday)
{
  if(!m_minInclusive.IsEmpty())
  {
    INT64 min = XMLGregorianMD(m_minInclusive).GetValue();
    INT64 val = XMLGregorianMD(p_monthday).GetValue();
    if(val < min) return _T("Value too small. < minInclusive");
  }
  if(!m_minExclusive.IsEmpty())
  {
    INT64 min = XMLGregorianMD(m_minExclusive).GetValue();
    INT64 val = XMLGregorianMD(p_monthday).GetValue();
    if(val <= min) return _T("Value too small. <= minExclusive");
  }
  if(!m_maxInclusive.IsEmpty())
  {
    INT64 max = XMLGregorianMD(m_maxInclusive).GetValue();
    INT64 val = XMLGregorianMD(p_monthday).GetValue();
    if(val > max) return _T("Value too big. > maxInclusive");
  }
  if(!m_maxExclusive.IsEmpty())
  {
    INT64 max = XMLGregorianMD(m_maxExclusive).GetValue();
    INT64 val = XMLGregorianMD(p_monthday).GetValue();
    if(val >= max) return _T("Value too big. >= maxExclusive");
  }
  return _T("");
}

XString
XMLRestriction::CheckAnyURI(XString p_value)
{
  XString result;
  CrackedURL url(p_value);
  if(!url.Valid())
  {
    result = _T("Not a valid URI: ") + p_value;
  }
  return result;
}

// Integer of an arbitrary length
XString 
XMLRestriction::CheckInteger(XString p_value)
{
  XString value(p_value);
  value.Trim();
  _TUCHAR ch = (_TUCHAR) value.GetAt(0);
  if(ch == '+' || ch == '-')
  {
    value = value.Mid(1);
  }

  for(int ind = 0;ind < value.GetLength(); ++ind)
  {
    if(!isdigit(value.GetAt(ind)))
    {
      return _T("Not an integer, but: ") + p_value;
    }
  }
  return CheckRangeDecimal(p_value);
}

XString
XMLRestriction::CheckBoolean(XString p_value)
{
  p_value.Trim();
  if(!p_value.IsEmpty())
  {
    if(p_value.CompareNoCase(_T("true")) &&
       p_value.CompareNoCase(_T("false")) &&
       p_value.CompareNoCase(_T("1")) &&
       p_value.CompareNoCase(_T("0")))
    {
      XString details(_T("Not a boolean, but: "));
      details += p_value;
      return details;
    }
  }
  return _T("");
}

XString
XMLRestriction::CheckBase64(XString p_value)
{
  int len = p_value.GetLength();
  for(int ind = 0; ind < len; ++ind)
  {
    _TUCHAR ch = (_TUCHAR) p_value.GetAt(ind);
    switch(ch)
    {
      case  9:      // Tab
      case 10:      // LF 
      case 13:      // CR
      case 32:      // Space
      case _T('/'): // Slash
      case _T('+'): // Plus
                    continue;
      case _T('='): if(ind == len - 1 ||
                      (ind == len - 2 && p_value.GetAt(ind + 1) == _T('=')))
                    {
                      return _T("");
                    }
                    break;
      default:      if(_istalnum(ch))
                    {
                      continue;
                    }
    }
    return _T("Not a base64Binary value");
  }
  return _T("");
}

// Number can be: nnnnn[.nnnnnn][[+|-]{E|e}nnn]
XString
XMLRestriction::CheckNumber(XString p_value,bool p_specials)
{
  XString result;
  p_value.TrimLeft('-');
  p_value.TrimLeft('+');

  if(p_specials)
  {
    if(p_value == _T("INF") || p_value == _T("NaN"))
    {
      return result;
    }
  }
  for(int ind = 0; ind < p_value.GetLength(); ++ind)
  {
    _TUCHAR ch = (_TUCHAR) p_value.GetAt(ind);
    if(!isspace(ch) && !isdigit(ch) && ch != '.' && ch != '+' && ch != '-' && toupper(ch) != 'E')
    {
      result  = _T("Not a number: ");
      result += p_value;
      return result;
    }
  }
  return CheckRangeFloat(p_value);
}

XString
XMLRestriction::CheckDatePart(XString p_value)
{
  XString result;
  int dateYear,dateMonth,dateDay;

  p_value.Trim();
  int num = _stscanf_s(p_value,_T("%d-%d-%d"),&dateYear,&dateMonth,&dateDay);
  if(num != 3)
  {
    result = _T("Not a date: ");
    result += p_value;
    return result;
  }
  else
  {
    if(dateYear  < 0 || dateYear  > 9999 ||
       dateMonth < 1 || dateMonth > 12   ||
       dateDay   < 1 || dateDay   > 31    )
    {
      result = _T("Date out of range: ") + p_value;
      return result;
    }
  }
  return CheckRangeDate(p_value);
}

XString
XMLRestriction::CheckTimeZone(XString p_value)
{
  XString result;
  int pos = p_value.Find(':');
  int hours = 0;
  int minutes = 0;

  if(pos > 0)
  {
    int num = _stscanf_s(p_value,_T("%d:%d"),&hours,&minutes);
    if(num != 2)
    {
      result = _T("Not a timezone hour:min but: ") + p_value;
    }
    else
    {
      if(hours > 14 || minutes >= 60)
      {
        result = _T("Timezone out of range: ") + p_value;
      }
    }
  }
  else
  {
    int num = _stscanf_s(p_value,_T("%d"),&hours);
    if(num != 1)
    {
      result = _T("Not a timezone hour but: ") + p_value;
    }
    else
    {
      if(hours > 14)
      {
        result = _T("Timezone out of range: ") + p_value;
      }
    }
  }
  return result;
}

// YYYY-MM-DD[[+/-]nn[:nn]]
// YYYY-MM-DD[Znn[:nn]]
XString
XMLRestriction::CheckDate(XString p_value)
{
  XString result;
  int dash1 = p_value.Find('-');
  int dash2 = p_value.Find('-',dash1 + 1);

  // Must have a date part
  if(dash1 < 0 || dash2 < 0)
  {
    return _T("Not a date: ") + p_value;
  }

  int tz1 = p_value.Find('Z',dash2 + 1);
  int tz2 = p_value.Find('+',dash2 + 1);
  int tz3 = p_value.Find('-',dash2 + 1);

  if(tz1 < 0 && tz2 < 0 && tz3 < 0)
  {
    result += CheckDatePart(p_value);
  }
  else if(tz1 > 0) 
  {
    result += CheckDatePart(p_value.Left(tz1));
    result += CheckTimeZone(p_value.Mid(tz1 + 1));
  }
  else if(tz2 > 0 || tz3 > 0)
  {
    tz1 = max(tz2,tz3);
    result += CheckDatePart(p_value.Left(tz1));
    result += CheckTimeZone(p_value.Mid(tz1 + 1));
  }
  else
  {
    result = _T("Invalid date: ") + p_value;
  }
  return result;
}

// YYYY-MM-DDThh:mm:ss
XString
XMLRestriction::CheckStampPart(XString p_value)
{
  XString result;
  int dateYear,dateMonth,dateDay;
  int timeHour,timeMin,timeSec;

  p_value.Trim();
  int num = _stscanf_s(p_value,_T("%d-%d-%dT%d:%d:%d")
                      ,&dateYear,&dateMonth,&dateDay
                      ,&timeHour,&timeMin,  &timeSec);
  if(num != 6)
  {
    result = _T("Not a dateTime: ") + p_value;
    return result;
  }
  else if(dateYear  < 0 || dateYear  > 9999 ||
          dateMonth < 1 || dateMonth >   12 ||
          dateDay   < 1 || dateDay   >   31 ||
          timeHour  < 0 || timeHour  >   23 ||
          timeMin   < 0 || timeMin   >   59 ||
          timeSec   < 0 || timeSec   >   60  )
  {
    result = _T("DateTime out of range: ") + p_value;
    return result;
  }
  return CheckRangeStamp(p_value);
}

// YYYY-MM-DDThh:mm:ss[Znn[:nn]]
// YYYY-MM-DDThh:mm:ss[[+/-]nn[:nn]]
XString
XMLRestriction::CheckDateTime(XString p_value,bool p_explicit)
{
  XString result;
  int pos1 = p_value.Find('Z');
  int pos2 = p_value.Find('+');
  int pos3 = p_value.Find('-');

  if(pos1 > 0)
  {
    result += CheckStampPart(p_value.Left(pos1));
    result += CheckTimeZone (p_value.Mid(pos1 + 1));
  }
  else if(pos2 > 0 || pos3 > 0)
  {
    pos1 = max(pos2,pos3);
    result += CheckStampPart(p_value.Left(pos1));
    result += CheckTimeZone (p_value.Mid(pos1 + 1));
  }
  else
  {
    if(p_explicit)
    {
      result = _T("dayTimeStamp missing an explicit timezone: ") + p_value;
    }
    else
    {
      result = CheckStampPart(p_value);
    }
  }
  return result;
}

XString
XMLRestriction::CheckTimePart(XString p_value)
{
  XString result;
  int hour = 0;
  int min  = 0;
  int sec  = 0;

  int num = _stscanf_s(p_value,_T("%d:%d:%d"),&hour,&min,&sec);
  if(num != 3)
  {
    result = _T("Not a time: ") + p_value;
    return result;
  }
  else if(hour < 0 || hour > 23 ||
          min  < 0 || min  > 59 ||
          sec  < 0 || sec  > 60)
  {
    result = _T("time out of range: ") + p_value;
    return result;
  }
  return CheckRangeTime(p_value);
}

// HH:MM::SS[Znn[:nn]]
// HH:MM::SS[[+/-]nn[:nn]]
XString
XMLRestriction::CheckTime(XString p_value)
{
  XString result;
  int pos1 = p_value.Find('Z');
  int pos2 = p_value.Find('+');
  int pos3 = p_value.Find('-');

  if(pos1 > 0)
  {
    result += CheckTimePart(p_value.Left(pos1));
    result += CheckTimeZone(p_value.Mid(pos1 + 1));
  }
  else if(pos2 > 0 || pos3 > 0)
  {
    pos1 = max(pos2,pos3);
    result += CheckTimePart(p_value.Left(pos1));
    result += CheckTimeZone(p_value.Mid(pos1 + 1));
  }
  else
  {
    result = CheckTimePart(p_value);
  }
  return result;
}

XString
XMLRestriction::CheckGregDay(XString p_value)
{
  p_value.Trim();
  p_value.Trim('-'); // Up to 2 chars may appear
  int num = _ttoi(p_value);
  XString result;

  if(num < 1 || num > 31)
  {
    result = _T("Not a Gregorian day in month: ") + p_value;
    return result;
  }
  return CheckRangeDecimal(p_value);
}

XString
XMLRestriction::CheckGregMonth(XString p_value)
{
  p_value.Trim();
  p_value.Trim('-'); // Up to 2 chars may appear
  int num = _ttoi(p_value);
  XString result;

  if(num < 1 || num > 12)
  {
    result = _T("Not a Gregorian month in year: ") + p_value;
    return result;
  }
  return CheckRangeDecimal(p_value);
}

XString
XMLRestriction::CheckGregYear(XString p_value)
{
  p_value.Trim();
  int num = _ttoi(p_value);
  XString result;

  if(num < 01 || num > 9999)
  {
    result = _T("Not a Gregorian XML year: ") + p_value;
    return result;
  }
  return CheckRangeDecimal(p_value);
}

XString
XMLRestriction::CheckGregMD(XString p_value)
{
  // Try to convert to GregorianMD at least once
  XMLGregorianMD greg(p_value);
  return CheckRangeGregMD(p_value);
}

XString
XMLRestriction::CheckGregYM(XString p_value)
{
  // Try to convert to GregorianYM at least once
  XMLGregorianYM greg(p_value);
  // Check ranges
  return CheckRangeGregYM(p_value);
}

XString
XMLRestriction::CheckHexBin(XString p_value)
{
  for(int ind = 0; ind < p_value.GetLength(); ++ind)
  {
    _TUCHAR ch = (_TUCHAR) p_value.GetAt(ind);
    if(!isspace(ch) && !isxdigit(ch))
    {
      return _T("Not a base64Binary field");
    }
  }
  return _T("");
}

XString
XMLRestriction::CheckLong(XString p_value)
{
  XString result = CheckInteger(p_value);
  if(result.IsEmpty())
  {
    INT64 val = _ttoi64(p_value);
    if(val < INT32_MIN || INT32_MAX < val)
    {
      result = _T("Long int out of range: ") + p_value;
      return result;
    }
  }
  return CheckRangeDecimal(p_value);
}

XString
XMLRestriction::CheckShort(XString p_value)
{
  XString result = CheckInteger(p_value);
  if(result.IsEmpty())
  {
    INT64 val = _ttoi64(p_value);
    if(val < INT16_MIN || INT16_MAX < val)
    {
      result = _T("Short out of range: ") + p_value;
      return result;
    }
  }
  return CheckRangeDecimal(p_value);
}

XString
XMLRestriction::CheckByte(XString p_value)
{
  XString result = CheckInteger(p_value);
  if(result.IsEmpty())
  {
    INT64 val = _ttoi64(p_value);
    if(val < INT8_MIN || INT8_MAX < val)
    {
      result = _T("Byte out of range: ") + p_value;
      return result;
    }
  }
  return CheckRangeDecimal(p_value);
}

XString
XMLRestriction::CheckNNegInt(XString p_value)
{
  XString result = CheckInteger(p_value);
  if(result.IsEmpty())
  {
    INT64 val = _ttoi64(p_value);
    if(val < 0 || INT32_MAX < val)
    {
      result = _T("nonNegativeInteger out of range: ") + p_value;
      return result;
    }
  }
  return CheckRangeDecimal(p_value);
}

XString
XMLRestriction::CheckPosInt(XString p_value)
{
  XString result = CheckInteger(p_value);
  if(result.IsEmpty())
  {
    INT64 val = _ttoi64(p_value);
    if(val <= 0 || INT32_MAX < val)
    {
      result = _T("positiveInteger out of range: ") + p_value;
      return result;
    }
  }
  return CheckRangeDecimal(p_value);
}

XString
XMLRestriction::CheckUnsLong(XString p_value)
{
  XString result = CheckInteger(p_value);
  if(result.IsEmpty())
  {
    INT64 val = _ttoi64(p_value);
    if(val < 0 || UINT32_MAX < val)
    {
      result = _T("unsignedLong / unsignedInt out of range: ") + p_value;
      return result;
    }
  }
  return CheckRangeDecimal(p_value);
}

XString
XMLRestriction::CheckUnsShort(XString p_value)
{
  XString result = CheckInteger(p_value);
  if(result.IsEmpty())
  {
    INT64 val = _ttoi64(p_value);
    if(val < 0 || UINT16_MAX < val)
    {
      result = _T("unsignedShort out of range: ") + p_value;
      return result;
    }
  }
  return CheckRangeDecimal(p_value);
}

XString
XMLRestriction::CheckUnsByte(XString p_value)
{
  XString result = CheckInteger(p_value);
  if(result.IsEmpty())
  {
    INT64 val = _ttoi64(p_value);
    if(val < 0 || UINT8_MAX < val)
    {
      result = _T("unsignedByte out of range: ") + p_value;
      return result;
    }
  }
  return CheckRangeDecimal(p_value);
}

XString
XMLRestriction::CheckNonPosInt(XString p_value)
{
  XString result = CheckInteger(p_value);
  if(result.IsEmpty())
  {
    INT64 val = _ttoi64(p_value);
    if(val < INT32_MIN || 0 < val)
    {
      result = _T("nonPositiveInteger out of range: ") + p_value;
      return result;
    }
  }
  return CheckRangeDecimal(p_value);
}

XString
XMLRestriction::CheckNegInt(XString p_value)
{
  XString result = CheckInteger(p_value);
  if(result.IsEmpty())
  {
    INT64 val = _ttoi64(p_value);
    if(val < INT32_MIN || 0 <= val)
    {
      result = _T("negativeInteger out of range: ") + p_value;
      return result;
    }
  }
  return CheckRangeDecimal(p_value);
}

XString   
XMLRestriction::CheckNormal(XString p_value)
{
  XString result;

  for(int ind = 0;ind < p_value.GetLength(); ++ind)
  {
    _TUCHAR ch = (_TUCHAR) p_value.GetAt(ind);
    if(ch == '\r' || ch == '\n' || ch == '\t')
    {
      result = _T("normalizedString contains red space: ") + p_value;
      return result;
    }
  }
  return CheckRangeDecimal(p_value);
}

XString
XMLRestriction::CheckToken(XString p_value)
{
  XString result = CheckNormal(p_value);

  if(result.IsEmpty())
  {
    if(p_value.Left(1) == _T(" ") || p_value.Right(1) == _T(" "))
    {
      result = _T("token cannot start or end with a space: ") + p_value;
    }
    else if(p_value.Find(_T("  ")) >= 0)
    {
      result = _T("token cannot contain separators larger than a space: ") + p_value;
    }
  }
  return result;
}

XString   
XMLRestriction::CheckNMTOKEN(XString p_value)
{
  XString result = CheckToken(p_value);

  if(result.IsEmpty())
  {
    for(int ind = 0;ind < p_value.GetLength(); ++ind)
    {
      _TUCHAR ch = (_TUCHAR) p_value.GetAt(ind);
      if(!isalnum(ch) && ch != ':' && ch != '-' && ch != '.' && ch != '_' && ch < 128)
      {
        result = _T("NMTOKEN with illegal characters: ") + p_value;
      }
    }
  }
  return result;
}

XString   
XMLRestriction::CheckName(XString p_value)
{
  XString result = CheckNMTOKEN(p_value);
  if(result.IsEmpty())
  {
    _TUCHAR ch = (_TUCHAR) p_value.GetAt(0);
    if(ch != ':' && !isalpha(ch) && ch != '_' && ch < 128)
    {
      result = _T("Name should begin with a name-start-character: ") + p_value;
    }
  }
  return result;
}

XString   
XMLRestriction::CheckNCName(XString p_value)
{
  XString result = CheckName(p_value);
  if(result.IsEmpty())
  {
    if(p_value.Find(':') >= 0)
    {
      result = _T("NCName cannot contain a colon: ") + p_value;
    }
  }
  return result;
}

// Check "name:name" -> Qualified name
XString   
XMLRestriction::CheckQName(XString p_value)
{
  XString result;
  int pos = p_value.Find(':');

  if(pos < 0)
  {
    return CheckNCName(p_value);
  }
  int pos2 = p_value.Find(':',pos + 1);
  if(pos2 > pos)
  {
    result = _T("QName cannot have more than one colon: ") + p_value;
  }
  else
  {
    XString first  = p_value.Left(pos);
    XString second = p_value.Mid(pos + 1);

    result = CheckNCName(first);
    if(result.IsEmpty())
    {
      result = CheckNCName(second);
    }
  }
  return result;
}

XString   
XMLRestriction::CheckNMTOKENS(XString p_value)
{
  XString result;

  if(p_value.Find(_T("  ")) >= 0)
  {
    return _T("NMTOKENS contains seperators larger than a space: ") + p_value;
  }

  p_value.Trim();
  int pos = p_value.Find(' ');

  while(pos > 0)
  {
    XString token = p_value.Left(pos);
    p_value = p_value.Mid(pos + 1);

    result = CheckNMTOKEN(token);
    if(!result.IsEmpty())
    {
      result = _T("NMTOKENS: ") + result;
      return result;
    }
    // Next token
    pos = p_value.Find(' ');
  }
  return CheckNMTOKEN(p_value);
}

XString
XMLRestriction::CheckNames(XString p_value)
{
  XString result;

  if(p_value.Find(_T("  ")) >= 0)
  {
    return _T("ENTITIES/IDREFS contains seperators larger than a space: ") + p_value;
  }

  p_value.Trim();
  int pos = p_value.Find(' ');

  while(pos > 0)
  {
    XString token = p_value.Left(pos);
    p_value = p_value.Mid(pos + 1);

    result = CheckName(token);
    if(!result.IsEmpty())
    {
      result = _T("ENTITIES/IDREFS: ") + result;
      return result;
    }
    // Next token
    pos = p_value.Find(' ');
  }
  return CheckName(p_value);
}

// [-] P nY[nM[nD]]  [time part]
// [-] P nM[nD]      [time part]
// [-] P nD          [time part]
// [-] P T nH[nM[nS[.S]]]
// [-] P T nM[nS[.S]]
// [-] P T nS[.S]
XString
XMLRestriction::CheckDuration(XString p_value,int& p_type)
{
  bool  didTime     = false;
  int   value       = 0;
  int   fraction    = 0;
  TCHAR marker      = 0;
  TCHAR firstMarker = 0;
  TCHAR lastMarker  = 0;
  XString duration(p_value);

  // Reset type
  p_type = 0;

  // Parse the negative sign
  duration.Trim();
  if(duration.Left(1) == _T("-"))
  {
    duration = duration.Mid(1);
  }

  // Must see a 'P' for period
  if(duration.Left(1) != _T("P"))
  {
    return XString(); // Leave interval at NULL
  }
  duration = duration.Mid(1);

  // Scan year/month/day/hour/min/second/fraction values
  while(ScanDurationValue(duration,value,fraction,marker,didTime))
  {
    switch(marker)
    {
      case 'Y': break;
      case 'D': break;
      case 'H': break;
      case 'M': if(didTime)
                {
                  marker = 'm'; // minutes!
                }
                break;
      case 'S': break;
      default:  // Illegal string, leave interval at NULL
                return _T("Illegal field markers in duration: ") + p_value;
    }
    // Getting first/last marker
    lastMarker = marker;
    if(firstMarker == 0)
    {
      firstMarker = marker;
    }
  }

  // Finding the interval type
       if(firstMarker == 'Y' && lastMarker == 'Y') p_type = 1;
  else if(firstMarker == 'M' && lastMarker == 'M') p_type = 2;
  else if(firstMarker == 'Y' && lastMarker == 'M') p_type = 3;
  else if(firstMarker == 'D' && lastMarker == 'D') p_type = 4;
  else if(firstMarker == 'H' && lastMarker == 'H') p_type = 5;
  else if(firstMarker == 'm' && lastMarker == 'm') p_type = 6;
  else if(firstMarker == 'S' && lastMarker == 'S') p_type = 7;
  else if(firstMarker == 'D' && lastMarker == 'H') p_type = 8;
  else if(firstMarker == 'D' && lastMarker == 'm') p_type = 9;
  else if(firstMarker == 'D' && lastMarker == 'S') p_type = 10;
  else if(firstMarker == 'H' && lastMarker == 'm') p_type = 11;
  else if(firstMarker == 'H' && lastMarker == 'S') p_type = 12;
  else if(firstMarker == 'm' && lastMarker == 'S') p_type = 13;
  else
  {
    // Beware: XML duration has combinations that are NOT compatible
    // with the SQL definition of an interval, like Month-to-Day
    return _T("duration has incompatible field values: ") + p_value;
  }
  return CheckRangeDuration(p_value);
}

bool
XMLRestriction::ScanDurationValue(XString& p_duration
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
    p_marker   = (TCHAR) p_duration.GetAt(0);
    p_duration = p_duration.Mid(1);
  }

  // True if both found, and fraction only found for seconds
  return (p_fraction && p_marker == 'S') ||
         (p_fraction == 0 && found && p_marker > 0);
}

XString
XMLRestriction::CheckDuration(XString p_value)
{
  int p_type = 0;
  return CheckDuration(p_value,p_type);
}

XString
XMLRestriction::CheckYearMonth(XString p_value)
{
  int p_type = 0;
  XString result = CheckDuration(p_value,p_type);
  if(result.IsEmpty() && (p_type < 1 || 3 < p_type))
  {
    result = _T("yearMonthDuration out of bounds: ") + p_value;
    return result;
  }
  return CheckRangeDuration(p_value);
}

XString
XMLRestriction::CheckDaySecond(XString p_value)
{
  int p_type = 0;
  XString result = CheckDuration(p_value,p_type);
  if(result.IsEmpty() && (p_type < 4 || 13 < p_type))
  {
    result = _T("dayTimeDuration out of bounds: ") + p_value;
    return result;
  }
  return CheckRangeDuration(p_value);
}

XString
XMLRestriction::CheckDatatype(XmlDataType p_type,XString p_value)
{
  XString result;

  // Empty value, nothing to check
  if(p_value.IsEmpty())
  {
    return result;
  }

  try
  {
    // Checking only base datatypes
    // String and CDATA are never checked!
    switch(p_type & XDT_MaskTypes)
    {
      case XDT_AnyURI:            result = CheckAnyURI   (p_value);       break;
      case XDT_Base64Binary:      result = CheckBase64   (p_value);       break;
      case XDT_Boolean:           result = CheckBoolean  (p_value);       break;
      case XDT_Date:              result = CheckDate     (p_value);       break;
      case XDT_Integer:           result = CheckInteger  (p_value);       break;
      case XDT_Decimal:           result = CheckNumber   (p_value,false); break;
      case XDT_Double:            result = CheckNumber   (p_value,true);  break;
      case XDT_DateTime:          result = CheckDateTime (p_value,false); break;
      case XDT_DateTimeStamp:     result = CheckDateTime (p_value,true);  break;
      case XDT_Float:             result = CheckNumber   (p_value,true);  break;
      case XDT_Duration:          result = CheckDuration (p_value);       break;
      case XDT_DayTimeDuration:   result = CheckDaySecond(p_value);       break;
      case XDT_YearMonthDuration: result = CheckYearMonth(p_value);       break;
      case XDT_GregDay:           result = CheckGregDay  (p_value);       break;
      case XDT_GregMonth:         result = CheckGregMonth(p_value);       break;
      case XDT_GregYear:          result = CheckGregYear (p_value);       break;
      case XDT_GregMonthDay:      result = CheckGregMD   (p_value);       break;
      case XDT_GregYearMonth:     result = CheckGregYM   (p_value);       break;
      case XDT_HexBinary:         result = CheckHexBin   (p_value);       break;
      case XDT_Long:              result = CheckLong     (p_value);       break;
      case XDT_Int:               result = CheckLong     (p_value);       break;
      case XDT_Short:             result = CheckShort    (p_value);       break;
      case XDT_NonNegativeInteger:result = CheckNNegInt  (p_value);       break;
      case XDT_PositiveInteger:   result = CheckPosInt   (p_value);       break;
      case XDT_UnsignedLong:      result = CheckUnsLong  (p_value);       break;
      case XDT_UnsignedInt:       result = CheckUnsLong  (p_value);       break;
      case XDT_UnsignedShort:     result = CheckUnsShort (p_value);       break;
      case XDT_UnsignedByte:      result = CheckUnsByte  (p_value);       break;
      case XDT_NonPositiveInteger:result = CheckNonPosInt(p_value);       break;
      case XDT_NegativeInteger:   result = CheckNegInt   (p_value);       break;
      case XDT_Time:              result = CheckTime     (p_value);       break;
      case XDT_Token:             result = CheckToken    (p_value);       break;
      case XDT_NMTOKEN:           result = CheckNMTOKEN  (p_value);       break;
      case XDT_Name:              result = CheckName     (p_value);       break;
      case XDT_ENTITY:            result = CheckName     (p_value);       break;
      case XDT_ID:                result = CheckName     (p_value);       break;
      case XDT_IDREF:             result = CheckName     (p_value);       break;
      case XDT_QName:             result = CheckQName    (p_value);       break;
      case XDT_NMTOKENS:          result = CheckNMTOKENS (p_value);       break;
      case XDT_ENTITIES:          result = CheckNames    (p_value);       break;
      case XDT_IDREFS:            result = CheckNames    (p_value);       break;
      default:                    break;
    }
  }
  catch(StdException& er)
  {
    ReThrowSafeException(er);
    // Primary datatype conversion went wrong
    result = er.GetErrorMessage();
  }
  return result;
}

XString   
XMLRestriction::CheckTotalDigits(XString p_value)
{
  XString error;
  XString value(p_value);
  int pos = p_value.Find('.');
  if(pos > 0)
  {
    value = p_value.Left(pos);
  }
  int count = 0;
  for(int ind = 0; ind < value.GetLength(); ++ind)
  {
    if(isdigit(value.GetAt(pos))) ++count;
  }
  if(count > m_totalDigits)
  {
    error.Format(_T("Number overflow totalDigits %d/%d"),count,m_totalDigits);
  }
  return error;
}

XString
XMLRestriction::CheckFractionDigits(XString p_value)
{
  XString error;
  int pos = p_value.Find('.');
  if(pos >= 0)
  {
    // Take fraction part
    p_value = p_value.Mid(pos + 1);
   
    int count = 0;
    for(int ind = 0; ind < p_value.GetLength(); ++ind)
    {
      if(isdigit(p_value.GetAt(pos)))
      {
        ++count;
      }
      else
      {
        // Stop counting at the 'E' or 'e' sign of the exponent
        break;
      }
    }
    if(count > m_fractionDigits)
    {
      error.Format(_T("Number precision overflow fractionDigits %d/%d"),count,m_fractionDigits);
    }
  }
  return error;
}

XString 
XMLRestriction::CheckRestriction(XmlDataType p_type,XString p_value)
{
  XString result;

  // Checking other restrictions
  if(m_length)
  {
    // Exact length required
    if(p_value.GetLength() != m_length)
    {
      result.Format(_T("Field length not exactly: %d"),m_length);
    }
  }
  if(m_maxLength)
  {
    // Maximum size of the field
    if(p_value.GetLength() > m_maxLength)
    {
      result.Format(_T("Field too long. Longer than: %d"),m_maxLength);
    }
  }
  if(m_minLength)
  {
    // Minimum size of the field
    if(p_value.GetLength() < m_minLength)
    {
      result.Format(_T("Field is too short. Shorter than: %d"),m_minLength);
    }
  }
  if(m_totalDigits)
  {
    result = CheckTotalDigits(p_value);
    if(!result.IsEmpty())
    {
      return result;
    }
  }
  if(m_fractionDigits)
  {
    result = CheckFractionDigits(p_value);
    if(!result.IsEmpty())
    {
      return result;
    }
  }

  // Notation datatype must have a enumerator list
  if(p_type == XDT_NOTATION)
  {
    if(m_enums.empty())
    {
      result = _T("NOTATION must declare an enumerator list of QNames");
      return result;
    }
    for(const auto& value : m_enums)
    {
      result = CheckQName(value.first);
      if(!result.IsEmpty())
      {
        return result;
      }
    }
  }

  // Pattern matching, if any
  if(!m_pattern.IsEmpty())
  {
#ifdef _UNICODE
    std::wstring str(p_value);
    std::wregex  reg(m_pattern);
#else
    std::string str(p_value);
    std::regex  reg(m_pattern);
#endif
    if(std::regex_match(str,reg) == false)
    {
      result.Format(_T("Field value [%s] does not match the pattern: %s"),p_value.GetString(),m_pattern.GetString());
      return result;
    }
  }
  
  // If no enumerations, then we are done
  if(m_enums.empty())
  {
    return result;
  }
  // See if the value is one of the stated enum values
  for(const auto& value : m_enums)
  {
    if(p_value.CompareNoCase(value.first) == 0)
    {
      return result;
    }
  }
  result.Format(_T("Field value [%s] is not in the list of allowed enumeration values."),p_value.GetString());
  return result;
}
