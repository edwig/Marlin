/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: XMLRestriction.cpp
//
// Marlin Server: Internet server/client
// 
// Copyright (c) 2016 ir. W.E. Huisman
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
#include "stdafx.h"
#include "XMLRestriction.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// Do not warn about formatting CStrings
#pragma warning(disable:6284)

XMLRestriction::XMLRestriction(CString p_name)
               :m_name(p_name)
{
}

void
XMLRestriction::AddEnumeration(CString p_enum,CString p_displayValue /*=""*/)
{
  if(!HasEnumeration(p_enum))
  {
    m_enums.insert(std::make_pair(p_enum,p_displayValue));
  }
}

bool 
XMLRestriction::HasEnumeration(CString p_enum)
{
  return m_enums.find(p_enum) != m_enums.end();
}

CString 
XMLRestriction::GiveDisplayValue(CString p_enum)
{
  XmlEnums::iterator it = m_enums.find(p_enum);
  if(it != m_enums.end())
  {
    return it->second;
  }
  return "";
}

// Print restriction as XML comment node
// <!--datatype has values: M (='Male customer') / F (='Female customer') / U (='Unknown')-->
// <!--datatype has values: M / F / U-->
CString
XMLRestriction::PrintEnumRestriction(CString p_name)
{
  bool print = false;
  CString restriction;
  restriction.Format("<!--%s has values: ",p_name);

  for(auto& enumvalue : m_enums)
  {
    if(print) restriction += " / ";
    restriction += enumvalue.first;
    if(!enumvalue.second.IsEmpty())
    {
      restriction += " (='";
      restriction += enumvalue.second;
      restriction += "')";
    }
    print = true;
  }

  restriction += "-->";
  return restriction;
}

// <!--maxLength: 20-->
CString
XMLRestriction::PrintIntegerRestriction(CString p_name,int p_value)
{
  CString restriction;
  restriction.Format("<!--%s: %d-->",p_name,p_value);
  return restriction;
}

// <!--pattern: ab*-->
CString
XMLRestriction::PrintStringRestriction(CString p_name,CString p_value)
{
  CString restriction;
  restriction.Format("<!--%s: %s-->",p_name,p_value);
  return restriction;
}

// <!--whitespace: collapse-->
CString
XMLRestriction::PrintSpaceRestriction()
{
  CString value;

  switch(m_whiteSpace)
  {
    case 1: value = "preserve"; break;
    case 2: value = "replace";  break;
    case 3: value = "collapse"; break;
  }
  return PrintStringRestriction("whitespace",value);
}

CString
XMLRestriction::PrintRestriction(CString p_name)
{
  CString restriction;

  // Print base type
  if(!m_baseType.IsEmpty()) restriction += PrintStringRestriction("base",m_baseType);
  // All integer restrictions
  if(m_length)         restriction += PrintIntegerRestriction("length (EXACT)",m_length);
  if(m_maxLength)      restriction += PrintIntegerRestriction("maxLength",     m_maxLength);
  if(m_minLength)      restriction += PrintIntegerRestriction("minLength",     m_minLength);
  if(m_totalDigits)    restriction += PrintIntegerRestriction("totalDigits",   m_totalDigits);
  if(m_fractionDigits) restriction += PrintIntegerRestriction("fractionDigits",m_fractionDigits);
  // All string restrictions
  if(!m_maxExclusive.IsEmpty()) restriction += PrintStringRestriction("maxExclusive",m_maxExclusive);
  if(!m_maxInclusive.IsEmpty()) restriction += PrintStringRestriction("maxInclusive",m_maxInclusive);
  if(!m_minExclusive.IsEmpty()) restriction += PrintStringRestriction("minExclusive",m_minExclusive);
  if(!m_minInclusive.IsEmpty()) restriction += PrintStringRestriction("minInclusive",m_minInclusive);
  if(!m_pattern     .IsEmpty()) restriction += PrintStringRestriction("pattern",     m_pattern);
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

//////////////////////////////////////////////////////////////////////////
//
//  RESTRICTIONS FOR A CLASS OF MESSAGES. I.E. a WSDL registration file
//
//////////////////////////////////////////////////////////////////////////

XMLRestriction* 
XMLRestrictions::FindRestriction(CString p_name)
{
  AllRestrictions::iterator it = m_restrictions.find(p_name);
  if(it != m_restrictions.end())
  {
    return &(it->second);
  }
  return nullptr;
}

XMLRestriction* 
XMLRestrictions::AddRestriction(CString p_name)
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
XMLRestrictions::AddEnumeration(CString p_name,CString p_enum,CString p_displayValue /*=""*/)
{
  XMLRestriction* res = FindRestriction(p_name);
  if(res)
  {
    res->AddEnumeration(p_enum,p_displayValue);
  }
}

bool            
XMLRestrictions::HasEnumeration(CString p_name,CString p_enum)
{
  XMLRestriction* res = FindRestriction(p_name);
  if(res)
  {
    return res->HasEnumeration(p_enum);
  }
  return false;
}

CString
XMLRestrictions::GiveDisplayValue(CString p_name,CString p_enum)
{
  XMLRestriction* res = FindRestriction(p_name);
  if(res)
  {
    return res->GiveDisplayValue(p_name);
  }
  return "";
}

//////////////////////////////////////////////////////////////////////////
//
// CHECKING A RESTRICTION
//
//////////////////////////////////////////////////////////////////////////

CString 
XMLRestriction::CheckInteger(CString p_value)
{
  for(int ind = 0;ind < p_value.GetLength(); ++ind)
  {
    int ch = p_value.GetAt(ind);
    if(!isspace(ch) && !isdigit(ch) && ch != '-' && ch != '+')
    {
      CString result("Not an integer, but: ");
      result += p_value;
      return result;
    }
  }
  return "";
}

CString
XMLRestriction::CheckBoolean(CString p_value)
{
  p_value.Trim();
  if(!p_value.IsEmpty())
  {
    if(p_value.CompareNoCase("true") &&
       p_value.CompareNoCase("false") &&
       p_value.CompareNoCase("1") &&
       p_value.CompareNoCase("0"))
    {
      CString details("Not a boolean, but: ");
      details += p_value;
      return details;
    }
  }
  return "";
}

CString
XMLRestriction::CheckBase64(CString p_value)
{
  for(int ind = 0; ind < p_value.GetLength(); ++ind)
  {
    int ch = p_value.GetAt(ind);
    if(!isspace(ch) && !isxdigit(ch))
    {
      return "Not a base64 field";
    }
  }
  return "";
}

CString
XMLRestriction::CheckDouble(CString p_value)
{
  CString result;

  for(int ind = 0; ind < p_value.GetLength(); ++ind)
  {
    int ch = p_value.GetAt(ind);
    if(!isspace(ch) && !isdigit(ch) && ch != '+' || ch != '-' && toupper(ch) != 'E')
    {
      result  = "Not a number: ";
      result += p_value;
    }
  }
  return result;
}

// "YYYY-MM-DDThh:mm:ss"
CString
XMLRestriction::CheckDateTime(CString p_value)
{
  CString result;
  int dateYear,dateMonth,dateDay;
  int timeHour,timeMin,timeSec;

  p_value.Trim();
  int num = sscanf_s(p_value,"%d-%d-%dT%d:%d:%d"
                    ,&dateYear,&dateMonth,&dateDay
                    ,&timeHour,&timeMin,  &timeSec);
  if(num != 6)
  {
    result  = "Not a dateTime: ";
    result += p_value;
  }
  return result;
}

CString
XMLRestriction::CheckDatatype(XmlDataType p_type,CString p_value)
{
  CString result;

  // Checking only base datatypes
  switch(p_type)
  {
    case XDT_Integer:   result = CheckInteger (p_value); break;
    case XDT_Boolean:   result = CheckBoolean (p_value); break;
    case XDT_Double:    result = CheckDouble  (p_value); break;
    case XDT_Base64:    result = CheckBase64  (p_value); break;
    case XDT_DateTime:  result = CheckDateTime(p_value); break;
  }
  return result;
}

CString 
XMLRestriction::CheckRestriction(XmlDataType /*p_type*/,CString p_value)
{
  CString result;

  // Checking other restrictions
  if(m_length)
  {
    // Exact length required
    if(p_value.GetLength() != m_length)
    {
      result.Format("Field length not exactly: %d",m_length);
    }
  }
  if(m_maxLength)
  {
    // Maximum size of the field
    if(p_value.GetLength() > m_maxLength)
    {
      result.Format("Field too long. Longer than: %d",m_maxLength);
    }
  }
  if(m_minLength)
  {
    // Minimum size of the field
    if(p_value.GetLength() < m_minLength)
    {
      result.Format("Field is too short. Shorter than: %d",m_minLength);
    }
  }
  if(m_enums.empty())
  {
    return result;
  }
  // See if the value is one of the stated enum values
  for(auto& value : m_enums)
  {
    if(p_value.CompareNoCase(value.first) == 0)
    {
      return result;
    }
  }
  result.Format("Field value [%s] is not in the list of allowed enumeration values.",p_value);
  return result;

  // YET TO BE IMPLEMENTED
  // totalDigits
  // fractionDigits
  // maxExclusive
  // maxInclusive
  // minExclusive
  // minInclusive
}
