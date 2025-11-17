/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: XMLRestriction.cpp
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
#include "pch.h"
#include "XMLRestriction.h"
#include "XMLTemporal.h"
#include "CrackURL.h"
#include <stdint.h>
#include <regex>

XMLRestriction::XMLRestriction(const XString& p_name)
               :m_name(p_name)
{
}

// SETTING RESTRICTIONS

void
XMLRestriction::AddEnumeration(const XString& p_enum,const XString& p_displayValue /*=""*/)
{
  if(!HasEnumeration(p_enum))
  {
    m_enums.insert(std::make_pair(p_enum,p_displayValue));
  }
}

void
XMLRestriction::AddMaxExclusive(const XString& p_max)
{ 
  m_maxExclusive        = p_max; 
  m_maxExclusiveDouble  = p_max.GetString();
  m_maxExclusiveInteger = _ttoi64(p_max);
}

void
XMLRestriction::AddMaxInclusive(const XString& p_max)
{ 
  m_maxInclusive        = p_max; 
  m_maxInclusiveDouble  = p_max;
  m_maxInclusiveInteger = _ttoi64(p_max);
}

void
XMLRestriction::AddMinExclusive(const XString& p_max)
{ 
  m_minExclusive        = p_max; 
  m_minExclusiveDouble  = p_max;
  m_minExclusiveInteger = _ttoi64(p_max);
}

void
XMLRestriction::AddMinInclusive(const XString& p_max)
{ 
  m_minInclusive        = p_max; 
  m_minInclusiveDouble  = p_max;
  m_minInclusiveInteger = _ttoi64(p_max);
}

void
XMLRestriction::AddMinOccurs(const XString& p_min)
{
  m_minOccurs = (unsigned) _ttoll(p_min);
  if(m_minOccurs > m_maxOccurs)
  {
    m_maxOccurs = m_minOccurs;
  }
}

void
XMLRestriction::AddMaxOccurs(const XString& p_max)
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
XMLRestriction::HasEnumeration(const XString& p_enum)
{
  return m_enums.find(p_enum) != m_enums.end();
}

XString 
XMLRestriction::GiveDisplayValue(const XString& p_enum)
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
XMLRestriction::PrintEnumRestriction(const XString& p_name)
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
XMLRestriction::PrintIntegerRestriction(const XString& p_name,int p_value)
{
  XString restriction;
  restriction.Format(_T("<!--%s: %d-->"),p_name.GetString(),p_value);
  return restriction;
}

// <!--pattern: ab*-->
XString
XMLRestriction::PrintStringRestriction(const XString& p_name,const XString& p_value)
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
XMLRestriction::PrintRestriction(const XString& p_name)
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
XMLRestriction::HandleWhitespace(XmlDataType p_type,XString& p_value)
{
  if(m_whiteSpace == 0)
  {
    // If "whiteSpace" attribute is not given
    if(p_type == XmlDataType::XDT_String)
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
XMLRestrictions::FindRestriction(const XString& p_name)
{
  AllRestrictions::iterator it = m_restrictions.find(p_name);
  if(it != m_restrictions.end())
  {
    return &(it->second);
  }
  return nullptr;
}

XMLRestriction* 
XMLRestrictions::AddRestriction(const XString& p_name)
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
XMLRestrictions::AddEnumeration(const XString& p_name,const XString& p_enum,const XString& p_displayValue /*=""*/)
{
  XMLRestriction* res = FindRestriction(p_name);
  if(res)
  {
    res->AddEnumeration(p_enum,p_displayValue);
  }
}

bool            
XMLRestrictions::HasEnumeration(const XString& p_name,const XString& p_enum)
{
  XMLRestriction* res = FindRestriction(p_name);
  if(res)
  {
    return res->HasEnumeration(p_enum);
  }
  return false;
}

XString
XMLRestrictions::GiveDisplayValue(const XString& p_name,const XString& /*p_enum*/)
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
XMLRestriction::CheckRangeFloat(const XString& p_value)
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
XMLRestriction::CheckRangeDecimal(const XString& p_value)
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
XMLRestriction::CheckRangeTime(const XString& p_time)
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
XMLRestriction::CheckRangeDate(const XString& p_date)
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
XMLRestriction::CheckRangeStamp(const XString& p_timestamp)
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
XMLRestriction::CheckRangeDuration(const XString& p_duration)
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
XMLRestriction::CheckRangeGregYM(const XString& p_yearmonth)
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
XMLRestriction::CheckRangeGregMD(const XString& p_monthday)
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
XMLRestriction::CheckAnyURI(const XString& p_value)
{
  XString result;
  CrackedURL url(p_value);
  if(!url.Valid())
  {
    result  = _T("Not a valid URI: ");
    result += p_value;
  }
  return result;
}

// Integer of an arbitrary length
XString 
XMLRestriction::CheckInteger(const XString& p_value)
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
      return XString(_T("Not an integer, but: ")) + p_value;
    }
  }
  return CheckRangeDecimal(p_value);
}

XString
XMLRestriction::CheckBoolean(const XString& p_value)
{
  if(!p_value.IsEmpty())
  {
    XString value(p_value);
    value.Trim();
    if(value.CompareNoCase(_T("true")) &&
       value.CompareNoCase(_T("false")) &&
       value.CompareNoCase(_T("1")) &&
       value.CompareNoCase(_T("0")))
    {
      XString details(_T("Not a boolean, but: "));
      details += value;
      return details;
    }
  }
  return _T("");
}

XString
XMLRestriction::CheckBase64(const XString& p_value)
{
  int len = p_value.GetLength();
  for(int ind = 0; ind < len; ++ind)
  {
    _TUCHAR ch = p_value[ind];
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
XMLRestriction::CheckNumber(const XString& p_value,bool p_specials)
{
  XString result;
  XString value(p_value);
  value.TrimLeft('-');
  value.TrimLeft('+');

  if(p_specials)
  {
    if(value == _T("INF") || value == _T("NaN"))
    {
      return result;
    }
  }
  for(int ind = 0; ind < value.GetLength(); ++ind)
  {
    _TUCHAR ch = (_TUCHAR) value.GetAt(ind);
    if(!isspace(ch) && !isdigit(ch) && ch != '.' && ch != '+' && ch != '-' && toupper(ch) != 'E')
    {
      result  = _T("Not a number: ");
      result += value;
      return result;
    }
  }
  return CheckRangeFloat(value);
}

XString
XMLRestriction::CheckDatePart(const XString& p_value)
{
  XString result;
  int dateYear,dateMonth,dateDay;

  int num = _stscanf_s(p_value,_T("%d-%d-%d"),&dateYear,&dateMonth,&dateDay);
  if(num != 3)
  {
    result  = _T("Not a date: ");
    result += p_value;
    return result;
  }
  else
  {
    if(dateYear  < 0 || dateYear  > 9999 ||
       dateMonth < 1 || dateMonth > 12   ||
       dateDay   < 1 || dateDay   > 31    )
    {
      result  = _T("Date out of range: ");
      result += p_value;
      return result;
    }
  }
  return CheckRangeDate(p_value);
}

XString
XMLRestriction::CheckTimeZone(const XString& p_value)
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
      result = XString(_T("Not a timezone hour:min but: ")) + p_value;
    }
    else
    {
      if(hours > 14 || minutes >= 60)
      {
        result = XString(_T("Timezone out of range: ") )+ p_value;
      }
    }
  }
  else
  {
    int num = _stscanf_s(p_value,_T("%d"),&hours);
    if(num != 1)
    {
      result = XString(_T("Not a timezone hour but: ")) + p_value;
    }
    else
    {
      if(hours > 14)
      {
        result = XString(_T("Timezone out of range: ")) + p_value;
      }
    }
  }
  return result;
}

// YYYY-MM-DD[[+/-]nn[:nn]]
// YYYY-MM-DD[Znn[:nn]]
XString
XMLRestriction::CheckDate(const XString& p_value)
{
  XString result;
  int dash1 = p_value.Find('-');
  int dash2 = p_value.Find('-',dash1 + 1);

  // Must have a date part
  if(dash1 < 0 || dash2 < 0)
  {
    return XString(_T("Not a date: ")) + p_value;
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
    result = XString(_T("Invalid date: ")) + p_value;
  }
  return result;
}

// YYYY-MM-DDThh:mm:ss
XString
XMLRestriction::CheckStampPart(const XString& p_value)
{
  XString result;
  int dateYear,dateMonth,dateDay;
  int timeHour,timeMin,timeSec;

  int num = _stscanf_s(p_value,_T("%d-%d-%dT%d:%d:%d")
                      ,&dateYear,&dateMonth,&dateDay
                      ,&timeHour,&timeMin,  &timeSec);
  if(num != 6)
  {
    result = XString(_T("Not a dateTime: ")) + p_value;
    return result;
  }
  else if(dateYear  < 0 || dateYear  > 9999 ||
          dateMonth < 1 || dateMonth >   12 ||
          dateDay   < 1 || dateDay   >   31 ||
          timeHour  < 0 || timeHour  >   23 ||
          timeMin   < 0 || timeMin   >   59 ||
          timeSec   < 0 || timeSec   >   60  )
  {
    result = XString(_T("DateTime out of range: ")) + p_value;
    return result;
  }
  return CheckRangeStamp(p_value);
}

// YYYY-MM-DDThh:mm:ss[Znn[:nn]]
// YYYY-MM-DDThh:mm:ss[[+/-]nn[:nn]]
XString
XMLRestriction::CheckDateTime(const XString& p_value,bool p_explicit)
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
      result = XString(_T("dayTimeStamp missing an explicit timezone: ")) + p_value;
    }
    else
    {
      result = CheckStampPart(p_value);
    }
  }
  return result;
}

XString
XMLRestriction::CheckTimePart(const XString& p_value)
{
  XString result;
  int hour = 0;
  int min  = 0;
  int sec  = 0;

  int num = _stscanf_s(p_value,_T("%d:%d:%d"),&hour,&min,&sec);
  if(num != 3)
  {
    result = XString(_T("Not a time: ")) + p_value;
    return result;
  }
  else if(hour < 0 || hour > 23 ||
          min  < 0 || min  > 59 ||
          sec  < 0 || sec  > 60)
  {
    result = XString(_T("time out of range: ")) + p_value;
    return result;
  }
  return CheckRangeTime(p_value);
}

// HH:MM::SS[Znn[:nn]]
// HH:MM::SS[[+/-]nn[:nn]]
XString
XMLRestriction::CheckTime(const XString& p_value)
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
XMLRestriction::CheckGregDay(const XString& p_value)
{
  XString value(p_value);
  value.Trim();
  value.Trim('-'); // Up to 2 chars may appear
  int num = _ttoi(value);
  XString result;

  if(num < 1 || num > 31)
  {
    result = XString(_T("Not a Gregorian day in month: ")) + value;
    return result;
  }
  return CheckRangeDecimal(value);
}

XString
XMLRestriction::CheckGregMonth(const XString& p_value)
{
  XString value(p_value);
  value.Trim();
  value.Trim('-'); // Up to 2 chars may appear
  int num = _ttoi(value);
  XString result;

  if(num < 1 || num > 12)
  {
    result = XString(_T("Not a Gregorian month in year: ")) + value;
    return result;
  }
  return CheckRangeDecimal(value);
}

XString
XMLRestriction::CheckGregYear(const XString& p_value)
{
  XString value(p_value);
  value.Trim();
  int num = _ttoi(value);
  XString result;

  if(num < 01 || num > 9999)
  {
    result = XString(_T("Not a Gregorian XML year: ")) + value;
    return result;
  }
  return CheckRangeDecimal(value);
}

XString
XMLRestriction::CheckGregMD(const XString& p_value)
{
  // Try to convert to GregorianMD at least once
  XMLGregorianMD greg(p_value);
  return CheckRangeGregMD(p_value);
}

XString
XMLRestriction::CheckGregYM(const XString& p_value)
{
  // Try to convert to GregorianYM at least once
  XMLGregorianYM greg(p_value);
  // Check ranges
  return CheckRangeGregYM(p_value);
}

XString
XMLRestriction::CheckHexBin(const XString& p_value)
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
XMLRestriction::CheckLong(const XString& p_value)
{
  XString result = CheckInteger(p_value);
  if(result.IsEmpty())
  {
    INT64 val = _ttoi64(p_value);
    if(val < INT32_MIN || INT32_MAX < val)
    {
      result = XString(_T("Long int out of range: ")) + p_value;
      return result;
    }
  }
  return CheckRangeDecimal(p_value);
}

XString
XMLRestriction::CheckShort(const XString& p_value)
{
  XString result = CheckInteger(p_value);
  if(result.IsEmpty())
  {
    INT64 val = _ttoi64(p_value);
    if(val < INT16_MIN || INT16_MAX < val)
    {
      result = XString(_T("Short out of range: ")) + p_value;
      return result;
    }
  }
  return CheckRangeDecimal(p_value);
}

XString
XMLRestriction::CheckByte(const XString& p_value)
{
  XString result = CheckInteger(p_value);
  if(result.IsEmpty())
  {
    INT64 val = _ttoi64(p_value);
    if(val < INT8_MIN || INT8_MAX < val)
    {
      result = XString(_T("Byte out of range: ")) + p_value;
      return result;
    }
  }
  return CheckRangeDecimal(p_value);
}

XString
XMLRestriction::CheckNNegInt(const XString& p_value)
{
  XString result = CheckInteger(p_value);
  if(result.IsEmpty())
  {
    INT64 val = _ttoi64(p_value);
    if(val < 0 || INT32_MAX < val)
    {
      result = XString(_T("nonNegativeInteger out of range: ")) + p_value;
      return result;
    }
  }
  return CheckRangeDecimal(p_value);
}

XString
XMLRestriction::CheckPosInt(const XString& p_value)
{
  XString result = CheckInteger(p_value);
  if(result.IsEmpty())
  {
    INT64 val = _ttoi64(p_value);
    if(val <= 0 || INT32_MAX < val)
    {
      result = XString(_T("positiveInteger out of range: ")) + p_value;
      return result;
    }
  }
  return CheckRangeDecimal(p_value);
}

XString
XMLRestriction::CheckUnsLong(const XString& p_value)
{
  XString result = CheckInteger(p_value);
  if(result.IsEmpty())
  {
    INT64 val = _ttoi64(p_value);
    if(val < 0 || UINT32_MAX < val)
    {
      result = XString(_T("unsignedLong / unsignedInt out of range: ")) + p_value;
      return result;
    }
  }
  return CheckRangeDecimal(p_value);
}

XString
XMLRestriction::CheckUnsShort(const XString& p_value)
{
  XString result = CheckInteger(p_value);
  if(result.IsEmpty())
  {
    INT64 val = _ttoi64(p_value);
    if(val < 0 || UINT16_MAX < val)
    {
      result = XString(_T("unsignedShort out of range: ")) + p_value;
      return result;
    }
  }
  return CheckRangeDecimal(p_value);
}

XString
XMLRestriction::CheckUnsByte(const XString& p_value)
{
  XString result = CheckInteger(p_value);
  if(result.IsEmpty())
  {
    INT64 val = _ttoi64(p_value);
    if(val < 0 || UINT8_MAX < val)
    {
      result = XString(_T("unsignedByte out of range: ")) + p_value;
      return result;
    }
  }
  return CheckRangeDecimal(p_value);
}

XString
XMLRestriction::CheckNonPosInt(const XString& p_value)
{
  XString result = CheckInteger(p_value);
  if(result.IsEmpty())
  {
    INT64 val = _ttoi64(p_value);
    if(val < INT32_MIN || 0 < val)
    {
      result = XString(_T("nonPositiveInteger out of range: ")) + p_value;
      return result;
    }
  }
  return CheckRangeDecimal(p_value);
}

XString
XMLRestriction::CheckNegInt(const XString& p_value)
{
  XString result = CheckInteger(p_value);
  if(result.IsEmpty())
  {
    INT64 val = _ttoi64(p_value);
    if(val < INT32_MIN || 0 <= val)
    {
      result = XString(_T("negativeInteger out of range: ")) + p_value;
      return result;
    }
  }
  return CheckRangeDecimal(p_value);
}

XString   
XMLRestriction::CheckNormal(const XString& p_value)
{
  XString result;

  for(int ind = 0;ind < p_value.GetLength(); ++ind)
  {
    _TUCHAR ch = (_TUCHAR) p_value.GetAt(ind);
    if(ch == '\r' || ch == '\n' || ch == '\t')
    {
      result = XString(_T("normalizedString contains red space: ")) + p_value;
      return result;
    }
  }
  return CheckRangeDecimal(p_value);
}

XString
XMLRestriction::CheckToken(const XString& p_value)
{
  XString result = CheckNormal(p_value);

  if(result.IsEmpty())
  {
    if(p_value.Left(1) == _T(" ") || p_value.Right(1) == _T(" "))
    {
      result = XString(_T("token cannot start or end with a space: ")) + p_value;
    }
    else if(p_value.Find(_T("  ")) >= 0)
    {
      result = XString(_T("token cannot contain separators larger than a space: ")) + p_value;
    }
  }
  return result;
}

XString   
XMLRestriction::CheckNMTOKEN(const XString& p_value)
{
  XString result = CheckToken(p_value);

  if(result.IsEmpty())
  {
    for(int ind = 0;ind < p_value.GetLength(); ++ind)
    {
      _TUCHAR ch = (_TUCHAR) p_value.GetAt(ind);
      if(!isalnum(ch) && ch != ':' && ch != '-' && ch != '.' && ch != '_' && ch < 128)
      {
        result = XString(_T("NMTOKEN with illegal characters: ")) + p_value;
      }
    }
  }
  return result;
}

XString   
XMLRestriction::CheckName(const XString& p_value)
{
  XString result = CheckNMTOKEN(p_value);
  if(result.IsEmpty())
  {
    _TUCHAR ch = (_TUCHAR) p_value.GetAt(0);
    if(ch != ':' && !isalpha(ch) && ch != '_' && ch < 128)
    {
      result = XString(_T("Name should begin with a name-start-character: ")) + p_value;
    }
  }
  return result;
}

XString   
XMLRestriction::CheckNCName(const XString& p_value)
{
  XString result = CheckName(p_value);
  if(result.IsEmpty())
  {
    if(p_value.Find(':') >= 0)
    {
      result = XString(_T("NCName cannot contain a colon: ")) + p_value;
    }
  }
  return result;
}

// Check "name:name" -> Qualified name
XString   
XMLRestriction::CheckQName(const XString& p_value)
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
    result = XString(_T("QName cannot have more than one colon: ")) + p_value;
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
XMLRestriction::CheckNMTOKENS(const XString& p_value)
{
  XString result;

  if(p_value.Find(_T("  ")) >= 0)
  {
    return XString(_T("NMTOKENS contains seperators larger than a space: ")) + p_value;
  }

  XString value(p_value);
  value.Trim();
  int pos = value.Find(' ');

  while(pos > 0)
  {
    XString token = value.Left(pos);
    value = value.Mid(pos + 1);

    result = CheckNMTOKEN(token);
    if(!result.IsEmpty())
    {
      result = XString(_T("NMTOKENS: ")) + result;
      return result;
    }
    // Next token
    pos = value.Find(' ');
  }
  return CheckNMTOKEN(value);
}

XString
XMLRestriction::CheckNames(const XString& p_value)
{
  XString result;

  if(p_value.Find(_T("  ")) >= 0)
  {
    return XString(_T("ENTITIES/IDREFS contains seperators larger than a space: ")) + p_value;
  }

  XString value(p_value);
  value.Trim();
  int pos = value.Find(' ');

  while(pos > 0)
  {
    XString token = value.Left(pos);
    value = value.Mid(pos + 1);

    result = CheckName(token);
    if(!result.IsEmpty())
    {
      result = XString(_T("ENTITIES/IDREFS: ")) + result;
      return result;
    }
    // Next token
    pos = value.Find(' ');
  }
  return CheckName(value);
}

// [-] P nY[nM[nD]]  [time part]
// [-] P nM[nD]      [time part]
// [-] P nD          [time part]
// [-] P T nH[nM[nS[.S]]]
// [-] P T nM[nS[.S]]
// [-] P T nS[.S]
XString
XMLRestriction::CheckDuration(const XString& p_value,int& p_type)
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
                return XString(_T("Illegal field markers in duration: ")) + p_value;
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
    return XString(_T("duration has incompatible field values: ")) + p_value;
  }
  return CheckRangeDuration(p_value);
}

bool
XMLRestriction::ScanDurationValue(const XString& p_duration
                                 ,int&     p_value
                                 ,int&     p_fraction
                                 ,TCHAR&   p_marker
                                 ,bool&    p_didTime)
{
  XString duration(p_duration);

  // Reset values
  p_value  = 0;
  p_marker = 0;
  bool found = false;

  // Check for empty string
  if(duration.IsEmpty())
  {
    return false;
  }

  // Scan for beginning of time part
  if(duration.GetAt(0) == 'T')
  {
    p_didTime = true;
    duration  = duration.Mid(1);
  }

  // Scan a number
  while(isdigit(duration.GetAt(0)))
  {
    found = true;
    p_value *= 10;
    p_value += duration.GetAt(0) - '0';
    duration = duration.Mid(1);
  }

  if(duration.GetAt(0) == '.')
  {
    duration = duration.Mid(1);

    int frac = 9;
    while(isdigit(duration.GetAt(0)))
    {
      --frac;
      p_fraction *= 10;
      p_fraction += duration.GetAt(0) - '0';
      duration    = duration.Mid(1);
    }
    p_fraction *= (int) pow(10,frac);
  }

  // Scan a marker
  if(isalpha(duration.GetAt(0)))
  {
    p_marker = (TCHAR) duration.GetAt(0);
    duration = duration.Mid(1);
  }

  // True if both found, and fraction only found for seconds
  return (p_fraction && p_marker == 'S') ||
         (p_fraction == 0 && found && p_marker > 0);
}

XString
XMLRestriction::CheckDuration(const XString& p_value)
{
  int p_type = 0;
  return CheckDuration(p_value,p_type);
}

XString
XMLRestriction::CheckYearMonth(const XString& p_value)
{
  int p_type = 0;
  XString result = CheckDuration(p_value,p_type);
  if(result.IsEmpty() && (p_type < 1 || 3 < p_type))
  {
    result = XString(_T("yearMonthDuration out of bounds: ")) + p_value;
    return result;
  }
  return CheckRangeDuration(p_value);
}

XString
XMLRestriction::CheckDaySecond(const XString& p_value)
{
  int p_type = 0;
  XString result = CheckDuration(p_value,p_type);
  if(result.IsEmpty() && (p_type < 4 || 13 < p_type))
  {
    result = XString(_T("dayTimeDuration out of bounds: ")) + p_value;
    return result;
  }
  return CheckRangeDuration(p_value);
}

XString
XMLRestriction::CheckDatatype(XmlDataType p_type,const XString& p_value)
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
    switch((XmlDataType)((int)p_type & XDT_MaskTypes))
    {
      case XmlDataType::XDT_AnyURI:            result = CheckAnyURI   (p_value);       break;
      case XmlDataType::XDT_Base64Binary:      result = CheckBase64   (p_value);       break;
      case XmlDataType::XDT_Boolean:           result = CheckBoolean  (p_value);       break;
      case XmlDataType::XDT_Date:              result = CheckDate     (p_value);       break;
      case XmlDataType::XDT_Integer:           result = CheckInteger  (p_value);       break;
      case XmlDataType::XDT_Decimal:           result = CheckNumber   (p_value,false); break;
      case XmlDataType::XDT_Double:            result = CheckNumber   (p_value,true);  break;
      case XmlDataType::XDT_DateTime:          result = CheckDateTime (p_value,false); break;
      case XmlDataType::XDT_DateTimeStamp:     result = CheckDateTime (p_value,true);  break;
      case XmlDataType::XDT_Float:             result = CheckNumber   (p_value,true);  break;
      case XmlDataType::XDT_Duration:          result = CheckDuration (p_value);       break;
      case XmlDataType::XDT_DayTimeDuration:   result = CheckDaySecond(p_value);       break;
      case XmlDataType::XDT_YearMonthDuration: result = CheckYearMonth(p_value);       break;
      case XmlDataType::XDT_GregDay:           result = CheckGregDay  (p_value);       break;
      case XmlDataType::XDT_GregMonth:         result = CheckGregMonth(p_value);       break;
      case XmlDataType::XDT_GregYear:          result = CheckGregYear (p_value);       break;
      case XmlDataType::XDT_GregMonthDay:      result = CheckGregMD   (p_value);       break;
      case XmlDataType::XDT_GregYearMonth:     result = CheckGregYM   (p_value);       break;
      case XmlDataType::XDT_HexBinary:         result = CheckHexBin   (p_value);       break;
      case XmlDataType::XDT_Long:              result = CheckLong     (p_value);       break;
      case XmlDataType::XDT_Int:               result = CheckLong     (p_value);       break;
      case XmlDataType::XDT_Short:             result = CheckShort    (p_value);       break;
      case XmlDataType::XDT_NonNegativeInteger:result = CheckNNegInt  (p_value);       break;
      case XmlDataType::XDT_PositiveInteger:   result = CheckPosInt   (p_value);       break;
      case XmlDataType::XDT_UnsignedLong:      result = CheckUnsLong  (p_value);       break;
      case XmlDataType::XDT_UnsignedInt:       result = CheckUnsLong  (p_value);       break;
      case XmlDataType::XDT_UnsignedShort:     result = CheckUnsShort (p_value);       break;
      case XmlDataType::XDT_UnsignedByte:      result = CheckUnsByte  (p_value);       break;
      case XmlDataType::XDT_NonPositiveInteger:result = CheckNonPosInt(p_value);       break;
      case XmlDataType::XDT_NegativeInteger:   result = CheckNegInt   (p_value);       break;
      case XmlDataType::XDT_Time:              result = CheckTime     (p_value);       break;
      case XmlDataType::XDT_Token:             result = CheckToken    (p_value);       break;
      case XmlDataType::XDT_NMTOKEN:           result = CheckNMTOKEN  (p_value);       break;
      case XmlDataType::XDT_Name:              result = CheckName     (p_value);       break;
      case XmlDataType::XDT_ENTITY:            result = CheckName     (p_value);       break;
      case XmlDataType::XDT_ID:                result = CheckName     (p_value);       break;
      case XmlDataType::XDT_IDREF:             result = CheckName     (p_value);       break;
      case XmlDataType::XDT_QName:             result = CheckQName    (p_value);       break;
      case XmlDataType::XDT_NMTOKENS:          result = CheckNMTOKENS (p_value);       break;
      case XmlDataType::XDT_ENTITIES:          result = CheckNames    (p_value);       break;
      case XmlDataType::XDT_IDREFS:            result = CheckNames    (p_value);       break;
      default:                                 break;
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
XMLRestriction::CheckTotalDigits(const XString& p_value)
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
XMLRestriction::CheckFractionDigits(const XString& p_value)
{
  XString error;
  int pos = p_value.Find('.');
  if(pos >= 0)
  {
    // Take fraction part
    XString value = p_value.Mid(pos + 1);
   
    int count = 0;
    for(int ind = 0; ind < value.GetLength(); ++ind)
    {
      if(isdigit(value.GetAt(pos)))
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
XMLRestriction::CheckRestriction(XmlDataType p_type,const XString& p_value)
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
  if(p_type == XmlDataType::XDT_NOTATION)
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
