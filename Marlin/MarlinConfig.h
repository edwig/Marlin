/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: MarlinConfig.h
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
#include "XMLMessage.h"
#include <map>
#include <list>

#define MARLINCONFIG_WACHTWOORD "W!e@b#.$C%o^n&f*i(g"

class MarlinConfig : public XMLMessage
{
public:
  MarlinConfig();
  MarlinConfig(CString p_filename);
  virtual ~MarlinConfig();

  // FILE METHODS

  bool   ReadConfig(CString p_filename);
  bool   WriteConfig();
  static CString GetExePath();
  static CString GetExeModule();
  static CString GetSiteConfig(CString p_prefixURL);
  static CString GetURLConfig(CString p_url);

  // SETTERS

  bool SetSection  (CString p_section);
  bool SetParameter(CString p_section,CString p_parameter,CString p_value);
  bool SetParameter(CString p_section,CString p_parameter,int     p_value);
  bool SetParameter(CString p_section,CString p_parameter,bool    p_value);
  bool SetEncrypted(CString p_section,CString p_parameter,CString p_value);
  bool SetAttribute(CString p_section,CString p_parameter,CString p_attrib,int     p_value);
  bool SetAttribute(CString p_section,CString p_parameter,CString p_attrib,double  p_value);
  bool SetAttribute(CString p_section,CString p_parameter,CString p_attrib,CString p_value);

  bool RemoveSection  (CString p_section);
  bool RemoveParameter(CString p_section,CString p_parameter);
  bool RemoveAttribute(CString p_section,CString p_parameter,CString p_attrib);

  // GETTERS

  bool    IsFilled() { return m_filled; };
  CString GetFilename();
  CString GetParameterString (CString p_section,CString p_parameter,CString p_default);
  bool    GetParameterBoolean(CString p_section,CString p_parameter,bool    p_default);
  int     GetParameterInteger(CString p_section,CString p_parameter,int     p_default);
  CString GetEncryptedString (CString p_section,CString p_parameter,CString p_default);
  int     GetAttribute(CString p_section,CString p_parameter,CString p_attrib,int     p_default);
  double  GetAttribute(CString p_section,CString p_parameter,CString p_attrib,double  p_default);
  CString GetAttribute(CString p_section,CString p_parameter,CString p_attrib,CString p_default);

  // DISCOVERY

  bool    HasSection  (CString p_section);
  bool    HasParameter(CString p_section,CString p_parameter);
  bool    HasAttribute(CString p_section,CString p_parameter,CString p_attribute);
  bool    IsChanged();
  void    ForgetChanges(); // Thread with care!

private:
  // Find section with this name
  XMLElement*   FindSection(CString p_section);
  // Find parameter within a section
  XMLElement*   FindParameter(XMLElement* p_section,CString p_parameter);
  // Find an attribute within a parameter
  XMLAttribute* FindAttribute(XMLElement* p_parameter,CString p_attribute);
  // Remove a parameter
  void          RemoveParameter(XMLElement* p_section,CString p_parameter);
  // Remove an attribute
  void          RemoveAttribute(XMLElement* p_parameter,CString p_attribute);

  // Filename of read config file
  CString   m_fileName;
  // Something was changed, so save the file
  bool      m_changed { false };
  // Correctly read from disk
  bool      m_filled  { false };
};
