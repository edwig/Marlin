/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: ServiceQuality.h
//
// Marlin Server: Internet server/client
// 
// Copyright (c) 2014-2022 ir. W.E. Huisman
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

class QualityOption
{
public:
  QualityOption() = default;

  XString m_field;
  int     m_percent { 100 };
  // Extra option extension.
  // Beware: Accomodates only one (1) extension!
  XString m_extension;
  XString m_value;
};

using QOptionMap = std::map<int,QualityOption>;

//////////////////////////////////////////////////////////////////////////
//
// Parsing the following HTTP headers:
// Accept               type/subtype;q=1;level=n, */*
// Accept-charset       charset;q=1, *
// Accept-encoding      encoding;q=0.5, *;q=0
// Accept-language      ln, en-gb;q=0.8 en-us;q=0.9, *
//
class ServiceQuality
{
public:
  explicit ServiceQuality(XString p_header);
 ~ServiceQuality() = default;

  // Request options from the HTTP header in order of preference. Starts with 0.
  QualityOption* GetOptionByPreference(int p_preference);
  XString        GetStringByPreference(int p_preference);
  // Find if a option is acceptable
  int            GetPreferenceByName(XString p_name);

private:
  // Parsing the incoming header
  void ParseHeader(XString p_header);
  void ParseOption(XString p_option);

  QOptionMap m_options;
};
