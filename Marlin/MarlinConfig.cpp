/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: MarlinConfig.cpp
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
#include "pch.h"
#include "MarlinConfig.h"
#include "Crypto.h"
#include <io.h>

MarlinConfig::MarlinConfig()
{
  ReadConfig(_T("Marlin.config"));
}

MarlinConfig::MarlinConfig(const XString& p_filename)
{
  if(p_filename.IsEmpty())
  {
    ReadConfig(_T("Marlin.config"));
  }
  else
  {
    ReadConfig(p_filename);
  }
}

MarlinConfig::~MarlinConfig()
{
  WriteConfig();
}

static const TCHAR g_staticAddress;

/* static */ XString
MarlinConfig::GetExeModule()
{
  TCHAR buffer[_MAX_PATH + 1];

  // Getting the module handle, if any
  // If it fails, the process names will be retrieved
  // Thus we get the *.DLL handle in IIS instead of a
  // %systemdrive\system32\inetsrv\w3wp.exe path
  HMODULE hmodule = NULL;
  GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | 
                    GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT
                   ,static_cast<LPCTSTR>(&g_staticAddress)
                   ,&hmodule);

  // Retrieve the path
  GetModuleFileName(hmodule,buffer,_MAX_PATH);
  return XString(buffer);
}

/* static */ XString
MarlinConfig::GetExePath()
{
  XString assembly = GetExeModule();

  int slashPosition = assembly.ReverseFind('\\');
  if(slashPosition == 0)
  {
    return _T("");
  }
  return assembly.Left(slashPosition + 1);
}

// Find the name of a URL site specific Marlin.config file
// Thus we can set overrides on a site basis
// Will produce something like 'Site-<port>-url-without-slashes.config'

/* static */ XString
MarlinConfig::GetSiteConfig(const XString& p_prefixURL)
{
  XString pathName = MarlinConfig::GetExePath();

  XString name(p_prefixURL);
  int pos = name.Find(_T("//"));
  if(pos)
  {
    name = _T("Site") + name.Mid(pos + 2);
    name.Replace(':','-');
//  name.Replace('+','-');    // Strong can appear in the filename
    name.Replace('*','!');    // Weak appears as '!' in the filename
    name.Replace('.','_');
    name.Replace('/','-');
    name.Replace('\\','-');
    name.Replace(_T("--"),_T("-"));
    name.TrimRight('-');
    name += _T(".config");

    return pathName + name;
  }
  return _T("");
}

/* static */ XString
MarlinConfig::GetURLConfig(const XString& p_url)
{
  XString pathName = MarlinConfig::GetExePath();

  XString url(p_url);
  int pos = url.Find(_T("//"));
  if(pos)
  {
    // Knock of the protocol
    url = _T("URL") + url.Mid(pos+2);
    // Knock of the query/anchor sequence
    pos = url.Find(_T("?"));
    if(pos > 0)
    {
      url = url.Left(pos);
    }
    // Knock of the resource, we just want the URL
    pos = url.ReverseFind('/');
    if(pos > 0)
    {
      url = url.Left(pos);
    }
    // For all you dumbs
    url.Replace(_T("\\"),_T("/"));
    // Remove special chars
    url.Replace(':','-');
    url.Replace(']','-');
    url.Replace('[','-');
    url.Replace('.','_');
    url.Replace('/','-');
    url.Replace('\\','-');
    url.Replace(_T("--"),_T("-"));
    url.TrimRight('-');

    // Create config file name
    url+= _T(".config");
    return pathName + url;
  }
  return _T("");
}

// Is the config file writable
bool
MarlinConfig::GetConfigWritable() const
{
  if(_taccess(m_fileName,00) == -1)
  {
    // File does not exist. Assume that we can create it
    // for instance in the ServerApplet config dialog
    return true;
  }
  if(_taccess(m_fileName,06) == 0)
  {
    // Read AND write access tot the file
    return true;
  }
  return false;
}

bool
MarlinConfig::ReadConfig(const XString& p_filename)
{
  // File to open  
  // Remember where we read it
  m_fileName = p_filename;
  
  // If it's not a UNC path and not a drive or a relative path
  // assume it's in our working directory
  if(p_filename.GetAt(0) != '\\' &&
     p_filename.GetAt(1) != ':'  &&
     p_filename.GetAt(0) != '.'  )
  {
    m_fileName = GetExePath() + p_filename;
  }

  if(XMLMessage::LoadFile(m_fileName))
  {
    XString rootName = m_root->GetName();
    if(rootName.CompareNoCase(_T("Configuration")) == 0)
    {
      // A Configuration file
      return m_filled = true;
    }
  }
  Reset();
  SetRootNodeName(_T("Configuration"));
  return false;
}

bool
MarlinConfig::WriteConfig()
{
  if(!m_changed)
  {
    return true;
  }
  // Check for new file
  if(m_fileName.IsEmpty())
  {
    m_fileName = GetExePath() + _T("Marlin.config");
  }

  return XMLMessage::SaveFile(m_fileName);
}

// In case we want to make in-memory changes that may never
// be written back to disk
void
MarlinConfig::ForgetChanges()
{
  m_changed = false;
}

// Find section with this name
XMLElement* 
MarlinConfig::FindSection(const XString& p_section) const
{
  return FindElement(p_section);
}

// Find parameter within a section
XMLElement*   
MarlinConfig::FindParameter(XMLElement* p_section,const XString& p_parameter) const
{
  return FindElement(p_section,p_parameter);
}

// Remove a parameter
void         
MarlinConfig::RemoveParameter(XMLElement* p_section,const XString& p_parameter)
{
  DeleteElement(p_section,p_parameter);
}

// Find an attribute within a parameter
XMLAttribute* 
MarlinConfig::FindAttribute(XMLElement* p_parameter,const XString& p_attribute) const
{
  return XMLMessage::FindAttribute(p_parameter,p_attribute);
}

// SETTERS

bool 
MarlinConfig::SetSection(const XString& p_section)
{
  if(FindElement(p_section))
  {
    // Section already exists
    return false;
  }
  if(AddElement(NULL,p_section,_T(""),XmlDataType::XDT_Complex))
  {
    m_changed = true;
  }
  return true;
}

// Add a parameter to a section
// To succeed, the section must exist already
bool 
MarlinConfig::SetParameter(const XString& p_section,const XString& p_parameter,const XString& p_value)
{
  XMLElement* section = FindElement(p_section);

  if(section)
  {
    XMLElement* elem = FindElement(section,p_parameter);
    if(elem)
    {
      if(elem->GetValue().Compare(p_value))
      {
        elem->SetValue(p_value);
        m_changed = true;
      }
      return false;
    }
    if(AddElement(section,p_parameter,p_value))
    {
      m_changed = true;
      return true;
    }
  }
  return false;
}

bool 
MarlinConfig::SetParameter(const XString& p_section,const XString& p_parameter,int p_value)
{
  XString value;
  if(p_value)
  {
    value.Format(_T("%d"),p_value);
  }
  return SetParameter(p_section,p_parameter,value);
}

bool
MarlinConfig::SetParameter(const XString& p_section,const XString& p_parameter,bool p_value)
{
  XString value = p_value ? _T("true") : _T("false");
  return SetParameter(p_section,p_parameter,value);
}

bool 
MarlinConfig::SetEncrypted(const XString& p_section,const XString& p_parameter,const XString& p_value)
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

bool 
MarlinConfig::SetAttribute(const XString& p_section,const XString& p_parameter,const XString& p_attrib,const XString& p_value)
{
  XMLElement* section = FindElement(p_section);
  if(section)
  {
    XMLElement* param = FindElement(section,p_parameter);
    if(param)
    {
      if(XMLMessage::SetAttribute(param,p_attrib,p_value))
      {
        m_changed = true;
        return true;
      }
    }
  }
  return false;
}

bool 
MarlinConfig::SetAttribute(const XString& p_section,const XString& p_parameter,const XString& p_attrib,int p_value)
{
  XString value;
  value.Format(_T("%d"),p_value);
  return SetAttribute(p_section,p_parameter,p_attrib,value);
}

bool 
MarlinConfig::SetAttribute(const XString& p_section,const XString& p_parameter,const XString& p_attrib,double p_value)
{
  XString value;
  value.Format(_T("%G"),p_value);
  return SetAttribute(p_section,p_parameter,p_attrib,value);
}

// Remove an attribute
void
MarlinConfig::RemoveAttribute(XMLElement* p_parameter,const XString& p_attribute)
{
  // to be implemented
  if(DeleteAttribute(p_parameter,p_attribute))
  {
    m_changed = true;
  }
}

bool 
MarlinConfig::RemoveSection(const XString& p_section)
{
  XMLElement* section = FindElement(p_section);
  if(section)
  {
    if(DeleteElement(m_root,section))
    {
      return m_changed = true;
    }
  }
  // Not found
  return false;
}

bool 
MarlinConfig::RemoveParameter(const XString& p_section,const XString& p_parameter)
{
  XMLElement* section = FindElement(p_section);
  if(section)
  {
    XMLElement* param = FindElement(section,p_parameter);
    if(param)
    {
      if(DeleteElement(section,param))
      {
        return m_changed = true;
      }
    }
  }
  return false;
}

bool 
MarlinConfig::RemoveAttribute(const XString& p_section,const XString& p_parameter,const XString& p_attrib)
{
  XMLElement* section = FindElement(p_section);
  if(section)
  {
    XMLElement* param = FindElement(section,p_parameter);
    if(param)
    {
      if(DeleteAttribute(param,p_attrib))
      {
        return m_changed = true;
      }
    }
  }
  return false;
}

//////////////////////////////////////////////////////////////////////////
//
// GETTERS
//
//////////////////////////////////////////////////////////////////////////

XString 
MarlinConfig::GetParameterString(const XString& p_section,const XString& p_parameter,const XString& p_default) const
{
  XMLElement* section = FindElement(p_section);
  if(section)
  {
    XMLElement* element = FindElement(section,p_parameter);
    if(element)
    {
      return element->GetValue();
    }
  }
  return p_default;
}

int
MarlinConfig::GetParameterInteger(const XString& p_section,const XString& p_parameter,int p_default) const
{
  XString param = GetParameterString(p_section,p_parameter,_T(""));
  if(param.IsEmpty())
  {
    return p_default;
  }
  return _ttoi(param);
}

bool
MarlinConfig::GetParameterBoolean(const XString& p_section,const XString& p_parameter,bool p_default) const
{
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
  // Simply 0 or 1
  return (_ttoi(param) > 0);
}

XString 
MarlinConfig::GetEncryptedString(const XString& p_section,const XString& p_parameter,const XString& p_default) const
{
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
          // pre-empt invalid results
          decrypted.Empty();
        }
      }
      else
      {
        // pre-empt invalid results
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
MarlinConfig::GetAttribute(const XString& p_section,const XString& p_parameter,const XString& p_attrib,const XString& p_default) const
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
  return p_default;
}

int
MarlinConfig::GetAttribute(const XString& p_section,const XString& p_parameter,const XString& p_attrib,int p_default) const
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
  return p_default;
}

double
MarlinConfig::GetAttribute(const XString& p_section,const XString& p_parameter,const XString& p_attrib,double p_default) const
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
  return p_default;
}

// DISCOVERY

bool
MarlinConfig::HasSection(const XString& p_section) const
{
  return FindSection(p_section) != nullptr;
}

bool    
MarlinConfig::HasParameter(const XString& p_section,const XString& p_parameter) const
{
  XMLElement* section = FindSection(p_section);
  if(section != nullptr)
  {
    return FindParameter(section,p_parameter) != nullptr;
  }
  return false;
}

bool
MarlinConfig::HasAttribute(const XString& p_section,const XString& p_parameter,const XString& p_attribute) const
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
  return false;
}

bool
MarlinConfig::IsChanged() const
{
  return m_changed;
}

XString
MarlinConfig::GetFilename() const
{
  return m_fileName;
}

