/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: ServiceQuality.cpp
//
// Marlin Server: Internet server/client
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
#include "ServiceQuality.h"

#ifdef _AFX
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#endif

ServiceQuality::ServiceQuality(XString p_header)
{
  ParseHeader(p_header);
}

QualityOption* 
ServiceQuality::GetOptionByPreference(int p_preference)
{
  int number = -1;
  
  QOptionMap::iterator it = m_options.end();
  while(true)
  {
    if(number == p_preference)
    {
      return &(it->second);
    }
    ++number;
    if(--it == m_options.end())
    {
      break;
    }
  }
  return nullptr;
}

XString
ServiceQuality::GetStringByPreference(int p_preference)
{
  QualityOption* option = GetOptionByPreference(p_preference);
  if(option)
  {
    return option->m_field;
  }
  return _T("");
}

// Find if a option is acceptable
int
ServiceQuality::GetPreferenceByName(XString p_name)
{
  for(auto& opt : m_options)
  {
    if(opt.second.m_field.CompareNoCase(p_name) == 0)
    {
      return opt.first;
    }
  }
  return 0;
}

//////////////////////////////////////////////////////////////////////////
//
//  PRIVATE
//
//////////////////////////////////////////////////////////////////////////


// Example for a 'Accept' header
// Accept: text/*;q=0.3, text/html;q=0.7, text/html;level=1, text/html;level=2;q=0.4, */*;q=0.5

// Parsing the incoming header
void 
ServiceQuality::ParseHeader(XString p_header)
{
  while(p_header.GetLength())
  {
    int pos = p_header.Find(',');
    if(pos > 0)
    {
      XString option = p_header.Left(pos).Trim();
      p_header = p_header.Mid(pos + 1).Trim();
      ParseOption(option);
    }
    else
    {
      ParseOption(p_header);
      return;
    }
  }
}

void 
ServiceQuality::ParseOption(XString p_option)
{
  QualityOption option;
  XString parameters;
  int percent(100);

  // Parse field
  int pos = p_option.Find(';');
  if (pos > 0)
  {
    parameters = p_option.Mid(pos + 1).Trim();
    option.m_field = p_option.Left(pos).Trim();
  }
  else
  {
    option.m_field = p_option.Trim();
  }
  // Parse parameters
  while(parameters.GetLength())
  {
    XString param;
    XString value;
    pos = parameters.Find(';');
    if(pos > 0)
    {
      param = parameters.Left(pos).Trim();
      parameters = parameters.Mid(pos + 1).Trim();
    }
    else
    {
      param = parameters.Trim();
      parameters.Empty();
    }
    pos = param.Find(_T("="));
    if(pos >= 0)
    {
      value = param.Mid(pos + 1);
      param = param.Left(pos);
    }
    // Check if param = "q"
    if(param.CompareNoCase(_T("q")) == 0)
    {
      percent = (int)(100 * _ttof(value));
      option.m_percent = percent;
    }
    else
    {
      option.m_extension = param;
      option.m_value = value;
    }
  }
  // Add to options
  m_options.insert(std::make_pair(percent,option));
}
