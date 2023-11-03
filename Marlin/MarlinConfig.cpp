/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: MarlinConfig.cpp
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
#include "Stdafx.h"
#include "MarlinConfig.h"
#include "Crypto.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

MarlinConfig::MarlinConfig()
{
  ReadConfig(_T("Marlin.config"));
}

MarlinConfig::MarlinConfig(XString p_filename)
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
MarlinConfig::GetSiteConfig(XString p_prefixURL)
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
MarlinConfig::GetURLConfig(XString p_url)
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

bool
MarlinConfig::ReadConfig(XString p_filename)
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
MarlinConfig::FindSection(XString p_section)
{
  return FindElement(p_section);
}

// Find parameter within a section
XMLElement*   
MarlinConfig::FindParameter(XMLElement* p_section,XString p_parameter)
{
  return FindElement(p_section,p_parameter);
}

// Remove a parameter
void         
MarlinConfig::RemoveParameter(XMLElement* p_section,XString p_parameter)
{
  DeleteElement(p_section,p_parameter);
}

// Find an attribute within a parameter
XMLAttribute* 
MarlinConfig::FindAttribute(XMLElement* p_parameter,XString p_attribute)
{
  return XMLMessage::FindAttribute(p_parameter,p_attribute);
}

// SETTERS

bool 
MarlinConfig::SetSection(XString p_section)
{
  if(FindElement(p_section))
  {
    // Section already exists
    return false;
  }
  if(AddElement(NULL,p_section,XDT_Complex,_T("")))
  {
    m_changed = true;
  }
  return true;
}

bool 
MarlinConfig::SetParameter(XString p_section,XString p_parameter,XString p_value)
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
    if(AddElement(section,p_parameter,XDT_String,p_value))
    {
      m_changed = true;
      return true;
    }
  }
  return false;
}

bool 
MarlinConfig::SetParameter(XString p_section,XString p_parameter,int p_value)
{
  XString value;
  if(p_value)
  {
    value.Format(_T("%d"),p_value);
  }
  return SetParameter(p_section,p_parameter,value);
}

bool
MarlinConfig::SetParameter(XString p_section,XString p_parameter,bool p_value)
{
  XString value = p_value ? _T("true") : _T("false");
  return SetParameter(p_section,p_parameter,value);
}

bool 
MarlinConfig::SetEncrypted(XString p_section,XString p_parameter,XString p_value)
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
MarlinConfig::SetAttribute(XString p_section,XString p_parameter,XString p_attrib,XString p_value)
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
MarlinConfig::SetAttribute(XString p_section,XString p_parameter,XString p_attrib,int p_value)
{
  XString value;
  value.Format(_T("%d"),p_value);
  return SetAttribute(p_section,p_parameter,p_attrib,value);
}

bool 
MarlinConfig::SetAttribute(XString p_section,XString p_parameter,XString p_attrib,double p_value)
{
  XString value;
  value.Format(_T("%G"),p_value);
  return SetAttribute(p_section,p_parameter,p_attrib,value);
}

// Remove an attribute
void
MarlinConfig::RemoveAttribute(XMLElement* p_parameter,XString p_attribute)
{
  // to be implemented
  if(DeleteAttribute(p_parameter,p_attribute))
  {
    m_changed = true;
  }
}

bool 
MarlinConfig::RemoveSection(XString p_section)
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
MarlinConfig::RemoveParameter(XString p_section,XString p_parameter)
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
MarlinConfig::RemoveAttribute(XString p_section,XString p_parameter,XString p_attrib)
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
MarlinConfig::GetParameterString(XString p_section,XString p_parameter,XString p_default)
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
MarlinConfig::GetParameterInteger(XString p_section,XString p_parameter,int p_default)
{
  XString param = GetParameterString(p_section,p_parameter,_T(""));
  if(param.IsEmpty())
  {
    return p_default;
  }
  return _ttoi(param);
}

bool
MarlinConfig::GetParameterBoolean(XString p_section,XString p_parameter,bool p_default)
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
MarlinConfig::GetEncryptedString (XString p_section,XString p_parameter,XString p_default)
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
MarlinConfig::GetAttribute(XString p_section,XString p_parameter,XString p_attrib,XString p_default)
{
  XString val = p_default;

  XMLElement* section = FindElement(p_section);
  if(section)
  {
    XMLElement* param = FindElement(section,p_parameter);
    if(param)
    {
      return XMLMessage::GetAttribute(param,p_attrib);
    }
  }
  return val;
}

int
MarlinConfig::GetAttribute(XString p_section,XString p_parameter,XString p_attrib,int p_default)
{
  int val = p_default;

  XMLElement* section = FindElement(p_section);
  if(section)
  {
    XMLElement* param = FindElement(section,p_parameter);
    if(param)
    {
      return _ttoi(XMLMessage::GetAttribute(param,p_attrib));
    }
  }
  return val;
}

double
MarlinConfig::GetAttribute(XString p_section,XString p_parameter,XString p_attrib,double p_default)
{
  double val = p_default;

  XMLElement* section = FindElement(p_section);
  if(section)
  {
    XMLElement* param = FindElement(section,p_parameter);
    if(param)
    {
      return _ttof(XMLMessage::GetAttribute(param,p_attrib));
    }
  }
  return val;
}

// DISCOVERY

bool
MarlinConfig::HasSection(XString p_section)
{
  return FindSection(p_section) != nullptr;
}

bool    
MarlinConfig::HasParameter(XString p_section,XString p_parameter)
{
  XMLElement* section = FindSection(p_section);
  if(section != nullptr)
  {
    return FindParameter(section,p_parameter) != nullptr;
  }
  return false;
}

bool
MarlinConfig::HasAttribute(XString p_section,XString p_parameter,XString p_attribute)
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
MarlinConfig::IsChanged()
{
  return m_changed;
}

XString
MarlinConfig::GetFilename()
{
  return m_fileName;
}

