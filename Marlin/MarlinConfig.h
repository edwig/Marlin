/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: MarlinConfig.h
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
#include <map>
#include <list>

#define MARLINCONFIG_WACHTWOORD _T("W!e@b#.$C%o^n&f*i(g")

#define DEFAULT_NAME        "<DTAP Name>"   // DTAP = "Development", "Test", "Acceptance", "Production"
#define DEFAULT_SERVER      "mymachine"
#define DEFAULT_URL         "/myserver/"
#define DEFAULT_SERVERLOG   "C:\\WWW\\Serverlog.txt"
#define DEFAULT_WEBROOT     "C:\\WWW\\"
#define DEFAULT_INSTANCE         1          // Machine instance
#define MAXIMUM_INSTANCE       100          // No more than 100 instances on 1 machine
#define DEFAULT_SERVERPORT     443          // Default HTTPS port of the server
#define RUNAS_STANDALONE         0          // Running as a one-time standalone program
#define RUNAS_NTSERVICE          1          // Running as an integrated Windows-NT service
#define RUNAS_IISAPPPOOL         2          // Running as an IIS application pool

class MarlinConfig : public XMLMessage
{
public:
  MarlinConfig();
  explicit MarlinConfig(XString p_filename);
  virtual ~MarlinConfig();

  // FILE METHODS

  bool   ReadConfig(XString p_filename);
  bool   WriteConfig();
  static XString GetExePath();
  static XString GetExeModule();
  static XString GetSiteConfig(XString p_prefixURL);
  static XString GetURLConfig(XString p_url);

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

  bool    IsFilled() { return m_filled; };
  XString GetFilename();
  XString GetParameterString (XString p_section,XString p_parameter,XString p_default);
  bool    GetParameterBoolean(XString p_section,XString p_parameter,bool    p_default);
  int     GetParameterInteger(XString p_section,XString p_parameter,int     p_default);
  XString GetEncryptedString (XString p_section,XString p_parameter,XString p_default);
  int     GetAttribute(XString p_section,XString p_parameter,XString p_attrib,int     p_default);
  double  GetAttribute(XString p_section,XString p_parameter,XString p_attrib,double  p_default);
  XString GetAttribute(XString p_section,XString p_parameter,XString p_attrib,XString p_default);

  // DISCOVERY

  bool    HasSection  (XString p_section);
  bool    HasParameter(XString p_section,XString p_parameter);
  bool    HasAttribute(XString p_section,XString p_parameter,XString p_attribute);
  bool    IsChanged();
  void    ForgetChanges(); // Thread with care!
  bool    GetConfigWritable();

private:
  // Find section with this name
  XMLElement*   FindSection(XString p_section);
  // Find parameter within a section
  XMLElement*   FindParameter(XMLElement* p_section,XString p_parameter);
  // Find an attribute within a parameter
  XMLAttribute* FindAttribute(XMLElement* p_parameter,XString p_attribute);
  // Remove a parameter
  void          RemoveParameter(XMLElement* p_section,XString p_parameter);
  // Remove an attribute
  void          RemoveAttribute(XMLElement* p_parameter,XString p_attribute);

  // Filename of read config file
  XString   m_fileName;
  // Something was changed, so save the file
  bool      m_changed { false };
  // Correctly read from disk
  bool      m_filled  { false };
};
