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
  explicit AppConfig(XString p_fileName = _T(""),bool p_readMarlinConfig = true);
  virtual ~AppConfig();

  // Read the config from disk
  bool ReadConfig();
  // Write back the config to disk
  bool WriteConfig();

  // SETTERS

  bool SetSection  (XString p_section);
  bool SetParameter(XString p_section,XString p_parameter,XString p_value);
  bool SetParameter(XString p_section,XString p_parameter,int     p_value);
  bool SetParameter(XString p_section,XString p_parameter,bool    p_value);
  bool SetEncrypted(XString p_section,XString p_parameter,XString p_value);
  bool SetAttribute(XString p_section,XString p_parameter,XString p_attrib,int     p_value);
  bool SetAttribute(XString p_section,XString p_parameter,XString p_attrib,double  p_value);
  bool SetAttribute(XString p_section,XString p_parameter,XString p_attrib,XString p_value);

  bool RemoveSection  (XString p_section);
  bool RemoveParameter(XString p_section,XString p_parameter);
  bool RemoveAttribute(XString p_section,XString p_parameter,XString p_attrib);

  // GETTERS

  bool    IsFilled()    { return m_filled;   };
  bool    IsChanged()   { return m_changed;  };
  XString GetFilename() { return m_fileName; };
  XString GetParameterString (XString p_section,XString p_parameter,XString p_default);
  bool    GetParameterBoolean(XString p_section,XString p_parameter,bool    p_default);
  int     GetParameterInteger(XString p_section,XString p_parameter,int     p_default);
  XString GetEncryptedString (XString p_section,XString p_parameter,XString p_default);
  int     GetAttribute(XString p_section,XString p_parameter,XString p_attrib,int     p_default);
  double  GetAttribute(XString p_section,XString p_parameter,XString p_attrib,double  p_default);
  XString GetAttribute(XString p_section,XString p_parameter,XString p_attrib,XString p_default);
  // Is the config file writable
  bool    GetConfigWritable();
  // The config file as an absolute pathname
  XString GetConfigFilename();
  // Server URL is not stored in this form,but it is used in many places.
  XString GetServerURL();

  // DISCOVERY

  bool    HasSection  (XString p_section);
  bool    HasParameter(XString p_section,XString p_parameter);
  bool    HasAttribute(XString p_section,XString p_parameter,XString p_attribute);

private:
  // Find section with this name
  XMLElement*  FindSection(XString p_section);
  // Find parameter within a section
  XMLElement*  FindParameter(XMLElement* p_section,XString p_parameter);
// Check the root node for the correct config file
  virtual bool CheckRootNodeName();

  // Config filename (relative) if not 'Marlin.config'
  XString       m_fileName;
  // Status fields
  bool          m_changed { false };
  bool          m_filled  { false };
  // Backed up by this MarlinConfig
  MarlinConfig* m_config  { nullptr };

//   XString  m_name;               // Name of the server or IIS application-pool name
//   XString  m_role;               // Server / Client / Server&Client
//   int      m_instance;           // Between 1 and 100
//   XString  m_server;             // Server host name
//   bool     m_secure;             // HTTP or HTTPS
//   unsigned m_serverPort;         // Server input port number
//   XString  m_baseUrl;            // Base URL only
//   XString  m_serverLog;          // Server logfile path name
//   int      m_serverLoglevel;     // See HTTPLoglevel.h
//   XString  m_webroot;
//   int      m_runAsService;       // Start method RUNAS_*
};
