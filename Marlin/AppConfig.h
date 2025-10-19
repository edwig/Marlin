/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: AppConfig.h
//
// Marlin Server: Internet server/client
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
#include "XMLMessage.h"

class MarlinConfig;

// DEFAULTS for the application config file: "<PRODUCT_NAME>.Config"

#define SECTION_ROOTNAME    _T("Configuration")
#define SECTION_APPLICATION _T("Application")

// The application configuration file
// This is a special XML file with a special root node
//
class AppConfig : public XMLMessage
{
public:
  explicit AppConfig(const XString& p_fileName = _T(""),bool p_readMarlinConfig = true);
  virtual ~AppConfig();

  // Read the config from disk
  bool ReadConfig();
  // Write back the config to disk
  bool WriteConfig();

  // SETTERS

  bool SetSection  (const XString& p_section);
  bool SetParameter(const XString& p_section,const XString& p_parameter,const XString& p_value);
  bool SetParameter(const XString& p_section,const XString& p_parameter,int   p_value);
  bool SetParameter(const XString& p_section,const XString& p_parameter,bool  p_value);
  bool SetEncrypted(const XString& p_section,const XString& p_parameter,const XString& p_value);
  bool SetAttribute(const XString& p_section,const XString& p_parameter,const XString& p_attrib,int    p_value);
  bool SetAttribute(const XString& p_section,const XString& p_parameter,const XString& p_attrib,double p_value);
  bool SetAttribute(const XString& p_section,const XString& p_parameter,const XString& p_attrib,const XString& p_value);

  bool RemoveSection  (const XString& p_section);
  bool RemoveParameter(const XString& p_section,const XString& p_parameter);
  bool RemoveAttribute(const XString& p_section,const XString& p_parameter,const XString& p_attrib);

  // GETTERS

  bool    IsFilled() const    { return m_filled;   };
  bool    IsChanged() const   { return m_changed;  };
  XString GetFilename() const { return m_fileName; };
  XString GetParameterString (const XString& p_section,const XString& p_parameter,const XString& p_default) const;
  bool    GetParameterBoolean(const XString& p_section,const XString& p_parameter,bool    p_default) const;
  int     GetParameterInteger(const XString& p_section,const XString& p_parameter,int     p_default) const;
  XString GetEncryptedString (const XString& p_section,const XString& p_parameter,const XString& p_default) const;
  int     GetAttribute(const XString& p_section,const XString& p_parameter,const XString& p_attrib,int     p_default) const;
  double  GetAttribute(const XString& p_section,const XString& p_parameter,const XString& p_attrib,double  p_default) const;
  XString GetAttribute(const XString& p_section,const XString& p_parameter,const XString& p_attrib,const XString& p_default) const;
  // Is the config file writable
  bool    GetConfigWritable() const;
  // The config file as an absolute pathname
  XString GetConfigFilename() const;
  // Server URL is not stored in this form,but it is used in many places.
  XString GetServerURL() const;

  // DISCOVERY

  bool    HasSection  (const XString& p_section) const;
  bool    HasParameter(const XString& p_section,const XString& p_parameter) const;
  bool    HasAttribute(const XString& p_section,const XString& p_parameter,const XString& p_attribute) const;

private:
  // Find section with this name
  XMLElement*  FindSection(const XString& p_section) const;
  // Find parameter within a section
  XMLElement*  FindParameter(XMLElement* p_section,const XString& p_parameter) const;
// Check the root node for the correct config file
  virtual bool CheckRootNodeName() const;

  // Config filename (relative) if not 'Marlin.config'
  XString       m_fileName;
  // Status fields
  bool          m_changed { false };
  bool          m_filled  { false };
  // Backed up by this MarlinConfig
  MarlinConfig* m_config  { nullptr };
};
