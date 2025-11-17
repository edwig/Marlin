/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: AppConfig.cpp
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
#include "stdafx.h"
#include "Marlin.h"
#include "AppConfig.h"
#include "MarlinConfig.h"
#include <winhttp.h>
#include <Crypto.h>
#include <io.h>

// XTOR
// In essence there are two practical use cases
// 1) Create a new config file separate from the MarlinConfig (2 files)
//    use case is when we have multiple servers in the same directory
//    Simply do this: m_config = new AppConfig();
// 2) Create a new config file IN the Marlin.config file (1 file)
//    use case is when we have only one server in the directory
//    Setup is: m_config = new AppConfig(_T("Marlin.config"),false);
//
AppConfig::AppConfig(const XString& p_fileName /*= ""*/,bool p_readMarlinConfig /*= true*/)
          :m_fileName(p_fileName)
{
  if(p_readMarlinConfig)
  {
    m_config = alloc_new MarlinConfig();
  }
}

// DTOR: Optionally drop the marlin.config file
//
AppConfig::~AppConfig()
{
  if(m_config)
  {
    delete m_config;
  }
}

//////////////////////////////////////////////////////////////////////////
//
// SETTERS FOR SECTIONS, PARAMETERS and ATTRIBUTES
//
//////////////////////////////////////////////////////////////////////////

bool
AppConfig::SetSection(const XString& p_section)
{
  // If it is not the application config, and we have a MarlinConfig
  // Set it in the general Marlin.config file
  if(m_config && p_section.Compare(SECTION_APPLICATION) != 0)
  {
    return m_config->SetSection(p_section);
  }
  // We are the Marlin.config file, or it is the 'Application' section
  if(FindElement(p_section))
  {
    // Section already exists, nothing to do
    return true;
  }
  if(AddElement(nullptr,p_section,_T(""),XmlDataType::XDT_Complex) == nullptr)
  {
    return false;
  }
  return (m_changed = true);
}

// Add a parameter to a section
// To succeed, the section must exist already
bool
AppConfig::SetParameter(const XString& p_section,const XString& p_parameter,const XString& p_value)
{
  // If it is not the application config, and we have a MarlinConfig
  // Set it in the general Marlin.config file
  if(m_config && p_section.Compare(SECTION_APPLICATION) != 0)
  {
    if(m_config->HasSection(p_section))
    {
      return m_config->SetParameter(p_section,p_parameter,p_value);
    }
    return false;
  }
  // We are the Marlin.config file, or it is the 'Application' section
  // But first make sure the section does exists
  XMLElement* section = FindElement(p_section);
  if(section == nullptr)
  {
    return false;
  }
  XMLElement* elem = FindElement(section,p_parameter);
  if(elem)
  {
    if(elem->GetValue().Compare(p_value) == 0)
    {
      // Value is not changed
      return true;
    }
    // Element did exist, but the value is different
    // update the value to the new value
    elem->SetValue(p_value);
  }
  else
  {
    if(AddElement(section,p_parameter,p_value) == nullptr)
    {
      return false;
    }
  }
  return (m_changed = true);
}

bool
AppConfig::SetParameter(const XString& p_section,const XString& p_parameter,int p_value)
{
  XString value;
  value.Format(_T("%d"),p_value);
  return SetParameter(p_section,p_parameter,value);
}

bool
AppConfig::SetParameter(const XString& p_section,const XString& p_parameter,bool p_value)
{
  XString value = p_value ? _T("true") : _T("false");
  return SetParameter(p_section,p_parameter,value);
}

bool
AppConfig::SetEncrypted(const XString& p_section,const XString& p_parameter,const XString& p_value)
{
  Crypto crypt;
  XString encrypted;
  if(!p_value.IsEmpty())
  {
    XString reverse(p_value);
    reverse.MakeReverse();
    XString value = reverse + _T(":") + p_value;
    encrypted = crypt.Encryption(value,MARLINCONFIG_WACHTWOORD);
  }
  return SetParameter(p_section,p_parameter,encrypted);
}

// Add a attribute to a section parameter
// To succeed, the section and the parameter must exist already
bool
AppConfig::SetAttribute(const XString& p_section,const XString& p_parameter,const XString& p_attrib,const XString& p_value)
{
  // If it is not the application config, and we have a MarlinConfig
  // Set it in the general Marlin.config file
  if(m_config && p_section.Compare(SECTION_APPLICATION) != 0)
  {
    if(m_config->HasSection(p_section) && m_config->HasParameter(p_section,p_parameter))
    {
      return m_config->SetAttribute(p_section,p_parameter,p_attrib,p_value);
    }
    return false;
  }

  // We are the Marlin.config file, or it is the 'Application' section
  // But first make sure the parameter does exists
  XMLElement* section = FindElement(p_section);
  if(section == nullptr)
  {
    return false;
  }
  XMLElement* param = FindElement(section,p_parameter);
  if(param == nullptr)
  {
    return false;
  }
  if(XMLMessage::SetAttribute(param,p_attrib,p_value) == nullptr)
  {
    return false;
  }
  return (m_changed = true);
}

bool
AppConfig::SetAttribute(const XString& p_section,const XString& p_parameter,const XString& p_attrib,int p_value)
{
  XString value;
  value.Format(_T("%d"),p_value);
  return SetAttribute(p_section,p_parameter,p_attrib,value);
}

bool
AppConfig::SetAttribute(const XString& p_section,const XString& p_parameter,const XString& p_attrib,double p_value)
{
  XString value;
  value.Format(_T("%G"),p_value);
  return SetAttribute(p_section,p_parameter,p_attrib,value);
}

//////////////////////////////////////////////////////////////////////////
//
// REMOVE SECTION, PARAMETER AND ATTRIBUTE
//
//////////////////////////////////////////////////////////////////////////

bool
AppConfig::RemoveSection(const XString& p_section)
{
  // If it is not the application config, and we have a MarlinConfig
  // Set it in the general Marlin.config file
  if(m_config && p_section.Compare(SECTION_APPLICATION) != 0)
  {
    return m_config->RemoveSection(p_section);
  }
  // We are the Marlin.config file, or it is the 'Application' section
  XMLElement* section = FindElement(p_section);
  if(section == nullptr)
  {
    return false;
  }
  if(!DeleteElement(m_root,section))
  {
    return false;
  }
  return (m_changed = true);
}

bool
AppConfig::RemoveParameter(const XString& p_section,const XString& p_parameter)
{
  // If it is not the application config, and we have a MarlinConfig
  // Set it in the general Marlin.config file
  if(m_config && p_section.Compare(SECTION_APPLICATION) != 0)
  {
    return m_config->RemoveParameter(p_section,p_parameter);
  }
  // We are the Marlin.config file, or it is the 'Application' section
  XMLElement* section = FindElement(p_section);
  if(section == nullptr)
  {
    return false;
  }
  XMLElement* param = FindElement(section,p_parameter);
  if(param)
  {
    if(!DeleteElement(section,param))
    {
      return false;
    }
  }
  return (m_changed = true);
}

bool
AppConfig::RemoveAttribute(const XString& p_section,const XString& p_parameter,const XString& p_attrib)
{
  // If it is not the application config, and we have a MarlinConfig
  // Set it in the general Marlin.config file
  if(m_config && p_section.Compare(SECTION_APPLICATION) != 0)
  {
    return m_config->RemoveAttribute(p_section,p_parameter,p_attrib);
  }
  // We are the Marlin.config file, or it is the 'Application' section
  XMLElement* section = FindElement(p_section);
  if(section == nullptr)
  {
    return false;
  }
  XMLElement* param = FindElement(section,p_parameter);
  if(param)
  {
    if(!DeleteAttribute(param,p_attrib))
    {
      return false;
    }
  }
  return (m_changed = true);
}

//////////////////////////////////////////////////////////////////////////
//
// GETTERS
//
//////////////////////////////////////////////////////////////////////////


XString
AppConfig::GetParameterString(const XString& p_section,const XString& p_parameter,const XString& p_default) const
{
  // Search first in our own config
  XMLElement* section = FindElement(p_section);
  if(section)
  {
    XMLElement* element = FindElement(section,p_parameter);
    if(element)
    {
      return element->GetValue();
    }
  }
  // Second guess: search in the MarlinConfig
  if(m_config)
  {
    if(m_config->HasParameter(p_section,p_parameter))
    {
      return m_config->GetParameterString(p_section,p_parameter,p_default);
    }
  }
  // Nope: Return the default value
  return p_default;
}

int
AppConfig::GetParameterInteger(const XString& p_section,const XString& p_parameter,int p_default) const
{
  if(!HasParameter(p_section,p_parameter))
  {
    return p_default;
  }
  XString param = GetParameterString(p_section,p_parameter,_T(""));
  return _ttoi(param);
}

bool
AppConfig::GetParameterBoolean(const XString& p_section,const XString& p_parameter,bool p_default) const
{
  if(!HasParameter(p_section,p_parameter))
  {
    return p_default;
  }
  XString param = GetParameterString(p_section,p_parameter,_T(""));
  if(param.IsEmpty())
  {
    return p_default;
  }
  if(param.CompareNoCase(_T("true")) == 0)
  {
    return true;
  }
  if(param.CompareNoCase(_T("false")) == 0)
  {
    return false;
  }
  // Simply 0 or 1 (Greater than zero)
  return (_ttoi(param) > 0);
}

XString
AppConfig::GetEncryptedString(const XString& p_section,const XString& p_parameter,const XString& p_default) const
{
  // Check if it exists first
  if(!HasParameter(p_section,p_parameter))
  {
    return p_default;
  }

  Crypto crypt;
  XString decrypted;

  try
  {
    XString encrypted = GetParameterString(p_section,p_parameter,_T(""));
    if(!encrypted.IsEmpty())
    {
      decrypted = crypt.Decryption(encrypted,MARLINCONFIG_WACHTWOORD);
      int pos = decrypted.Find(':');
      if(pos > 0)
      {
        XString reversed = decrypted.Left(pos);
        decrypted = decrypted.Mid(pos + 1);
        reversed.MakeReverse();
        if(reversed != decrypted)
        {
          // preempt invalid results
          decrypted.Empty();
        }
      }
      else
      {
        // preempt invalid results
        decrypted.Empty();
      }
      return decrypted;
    }
  }
  catch(StdException& er)
  {
    UNREFERENCED_PARAMETER(er);
  }
  return p_default;
}

XString
AppConfig::GetAttribute(const XString& p_section,const XString& p_parameter,const XString& p_attrib,const XString& p_default) const
{
  XMLElement* section = FindElement(p_section);
  if(section)
  {
    XMLElement* param = FindElement(section,p_parameter);
    if(param)
    {
      return XMLMessage::GetAttribute(param,p_attrib);
    }
  }
  // Not found: Search our marlin.config
  if(m_config)
  {
    return m_config->GetAttribute(p_section,p_parameter,p_attrib,p_default);
  }
  return p_default;
}

int
AppConfig::GetAttribute(const XString& p_section,const XString& p_parameter,const XString& p_attrib,int p_default) const
{
  XMLElement* section = FindElement(p_section);
  if(section)
  {
    XMLElement* param = FindElement(section,p_parameter);
    if(param)
    {
      return _ttoi(XMLMessage::GetAttribute(param,p_attrib));
    }
  }
  // Not found: Search our marlin.config
  if(m_config)
  {
    return m_config->GetAttribute(p_section,p_parameter,p_attrib,p_default);
  }
  return p_default;
}

double
AppConfig::GetAttribute(const XString& p_section,const XString& p_parameter,const XString& p_attrib,double p_default) const
{
  XMLElement* section = FindElement(p_section);
  if(section)
  {
    XMLElement* param = FindElement(section,p_parameter);
    if(param)
    {
      return _ttof(XMLMessage::GetAttribute(param,p_attrib));
    }
  }
  // Not found: Search our marlin.config
  if(m_config)
  {
    return m_config->GetAttribute(p_section,p_parameter,p_attrib,p_default);
  }
  return p_default;
}

// Server URL is not stored in this form
// But  it is used in many places.
XString
AppConfig::GetServerURL() const
{
  XString serverURL;
  XString section(SECTION_APPLICATION);
  
  int serverPort  = GetParameterInteger(section,_T("ServerPort"),INTERNET_DEFAULT_HTTPS_PORT);
  XString server  = GetParameterString (section,_T("Server"),    _T(""));
  XString baseUrl = GetParameterString (section,_T("BaseURL"),   _T("/"));

  if(serverPort == INTERNET_DEFAULT_HTTP_PORT)
  {
    serverURL.Format(_T("http://%s%s"),server.GetString(),baseUrl.GetString());
  }
  else if(serverPort == INTERNET_DEFAULT_HTTPS_PORT)
  {
    serverURL.Format(_T("https://%s%s"),server.GetString(),baseUrl.GetString());
  }
  else
  {
    serverURL.Format(_T("http://%s:%d%s"),server.GetString(),serverPort,baseUrl.GetString());
  }
  return serverURL;
}

//////////////////////////////////////////////////////////////////////////
//
// DISCOVERY
//
//////////////////////////////////////////////////////////////////////////

bool
AppConfig::HasSection(const XString& p_section) const
{
  if(FindSection(p_section))
  {
    return true;
  }
  if(m_config)
  {
    return m_config->HasSection(p_section);
  }
  return false;
}

bool
AppConfig::HasParameter(const XString& p_section,const XString& p_parameter) const
{
  XMLElement* section = FindSection(p_section);
  if(section != nullptr)
  {
    return FindParameter(section,p_parameter) != nullptr;
  }
  if(m_config)
  {
    return m_config->HasParameter(p_section,p_parameter);
  }
  return false;
}

bool
AppConfig::HasAttribute(const XString& p_section,const XString& p_parameter,const XString& p_attribute) const
{
  XMLElement* section = FindElement(p_section);
  if(section)
  {
    XMLElement* param = FindElement(section,p_parameter);
    if(param)
    {
      if(FindAttribute(param,p_attribute))
      {
        return true;
      }
    }
  }
  if(m_config)
  {
    return m_config->HasAttribute(p_section,p_parameter,p_attribute);
  } 
  return false;
}

//////////////////////////////////////////////////////////////////////////
//
// READING AND WRITING THE CONFIG FILE
//
//////////////////////////////////////////////////////////////////////////

// Is the config file writable
bool
AppConfig::GetConfigWritable() const
{
  XString fileNaam = GetConfigFilename();
  if (_taccess(fileNaam, 00) == -1)
  {
    // File does not exist. Assume that we can create it
    // for instance in the ServerApplet config dialog
    return true;
  }
  if(_taccess(fileNaam, 06) == 0)
  {
    if(m_config)
    {
      if(m_config->GetConfigWritable() == false)
      {
        return false;
      }
    }
    // Read AND write access tot the file
    return true;
  }
  return false;
}

// The config file as an absolute pathname
XString
AppConfig::GetConfigFilename() const
{
  // Special configured file from the constructor
  if(!m_fileName.IsEmpty())
  {
    return MarlinConfig::GetExePath() + m_fileName;
  }
  // This is our config file
  return MarlinConfig::GetExePath() + PRODUCT_NAME + _T(".Config");
}

// Read the config from disk
bool
AppConfig::ReadConfig()
{
  // This is our config file
  XString fileName = GetConfigFilename();

  if(XMLMessage::LoadFile(fileName) == false)
  {
    return false;
  }

  // Check if it is our config file
  if(!CheckRootNodeName())
  {
    return false;
  }

  if(XMLMessage::GetInternalError() != XmlError::XE_NoError)
  {
    return false;
  }
  return true;
}

// Write back the config to disk
bool
AppConfig::WriteConfig()
{
  if(m_config)
  {
    m_config->WriteConfig();
  }

  // Be sure we have the correct root node
  if(GetRoot()->GetName().IsEmpty())
  {
    SetRootNodeName(SECTION_ROOTNAME);
  }

  // Writing to file is the result
  return SaveFile(GetConfigFilename());
}

//////////////////////////////////////////////////////////////////////////
//
// PROTECTED METHODS
//
//////////////////////////////////////////////////////////////////////////

// Find section with this name
XMLElement*
AppConfig::FindSection(const XString& p_section) const
{
  return FindElement(p_section);
}

// Find parameter within a section
XMLElement*
AppConfig::FindParameter(XMLElement* p_section,const XString& p_parameter) const
{
  return FindElement(p_section,p_parameter);
}

// In the base class this is always ok
// Used to just read the base members.
// E.g. in the ServerMain startup methods
bool
AppConfig::CheckRootNodeName() const
{
  XMLElement* root = XMLMessage::GetRoot();
  return root->GetName().CompareNoCase(SECTION_ROOTNAME) == 0;
}
