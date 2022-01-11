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
  ReadConfig("Marlin.config");
}

MarlinConfig::MarlinConfig(CString p_filename)
{
  if(p_filename.IsEmpty())
  {
    ReadConfig("Marlin.config");
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

static char g_staticAddress;

/* static */ CString
MarlinConfig::GetExeModule()
{
  char buffer[_MAX_PATH + 1];

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
  return CString(buffer);
}

/* static */ CString
MarlinConfig::GetExePath()
{
  CString assembly = GetExeModule();

  int slashPosition = assembly.ReverseFind('\\');
  if(slashPosition == 0)
  {
    return "";
  }
  return assembly.Left(slashPosition + 1);
}

// Find the name of a URL site specific Marlin.config file
// Thus we can set overrides on a site basis
// Will produce something like 'Site-<port>-url-without-slashes.config'

/* static */ CString
MarlinConfig::GetSiteConfig(CString p_prefixURL)
{
  CString pathName = MarlinConfig::GetExePath();

  CString name(p_prefixURL);
  int pos = name.Find("//");
  if(pos)
  {
    name = "Site" + name.Mid(pos + 2);
    name.Replace(':','-');
//  name.Replace('+','-');    // Strong can appear in the filename
    name.Replace('*','!');    // Weak appears as '!' in the filename
    name.Replace('.','_');
    name.Replace('/','-');
    name.Replace('\\','-');
    name.Replace("--","-");
    name.TrimRight('-');
    name += ".config";

    return pathName + name;
  }
  return "";
}

/* static */ CString
MarlinConfig::GetURLConfig(CString p_url)
{
  CString pathName = MarlinConfig::GetExePath();

  CString url(p_url);
  int pos = url.Find("//");
  if(pos)
  {
    // Knock of the protocol
    url = "URL" + url.Mid(pos+2);
    // Knock of the query/anchor sequence
    pos = url.Find("?");
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
    url.Replace("\\","/");
    // Remove special chars
    url.Replace(':','-');
    url.Replace(']','-');
    url.Replace('[','-');
    url.Replace('.','_');
    url.Replace('/','-');
    url.Replace('\\','-');
    url.Replace("--","-");
    url.TrimRight('-');

    // Create config file name
    url+= ".config";
    return pathName + url;
  }
  return "";
}

bool
MarlinConfig::ReadConfig(CString p_filename)
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
    CString rootName = m_root->GetName();
    if(rootName.CompareNoCase("Configuration") == 0)
    {
      // A Configuration file
      return m_filled = true;
    }
  }
  Reset();
  SetRootNodeName("Configuration");
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
    m_fileName = GetExePath() + "Marlin.config";
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
MarlinConfig::FindSection(CString p_section)
{
  return FindElement(p_section);
}

// Find parameter within a section
XMLElement*   
MarlinConfig::FindParameter(XMLElement* p_section,CString p_parameter)
{
  return FindElement(p_section,p_parameter);
}

// Remove a parameter
void         
MarlinConfig::RemoveParameter(XMLElement* p_section,CString p_parameter)
{
  DeleteElement(p_section,p_parameter);
}

// Find an attribute within a parameter
XMLAttribute* 
MarlinConfig::FindAttribute(XMLElement* p_parameter,CString p_attribute)
{
  return XMLMessage::FindAttribute(p_parameter,p_attribute);
}

// SETTERS

bool 
MarlinConfig::SetSection(CString p_section)
{
  if(FindElement(p_section))
  {
    // Section already exists
    return false;
  }
  if(AddElement(NULL,p_section,XDT_Complex,""))
  {
    m_changed = true;
  }
  return true;
}

bool 
MarlinConfig::SetParameter(CString p_section,CString p_parameter,CString p_value)
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
MarlinConfig::SetParameter(CString p_section,CString p_parameter,int p_value)
{
  CString value;
  if(p_value)
  {
    value.Format("%d",p_value);
  }
  return SetParameter(p_section,p_parameter,value);
}

bool
MarlinConfig::SetParameter(CString p_section,CString p_parameter,bool p_value)
{
  CString value = p_value ? "true" : "false";
  return SetParameter(p_section,p_parameter,value);
}

bool 
MarlinConfig::SetEncrypted(CString p_section,CString p_parameter,CString p_value)
{
  Crypto crypt;
  CString encrypted;
  if(!p_value.IsEmpty())
  {
    CString reverse(p_value);
    reverse.MakeReverse();
    CString value = reverse + ":" + p_value;
    encrypted = crypt.Encryption(value,MARLINCONFIG_WACHTWOORD);
  }
  return SetParameter(p_section,p_parameter,encrypted);
}

bool 
MarlinConfig::SetAttribute(CString p_section,CString p_parameter,CString p_attrib,CString p_value)
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
MarlinConfig::SetAttribute(CString p_section,CString p_parameter,CString p_attrib,int p_value)
{
  CString value;
  value.Format("%d",p_value);
  return SetAttribute(p_section,p_parameter,p_attrib,value);
}

bool 
MarlinConfig::SetAttribute(CString p_section,CString p_parameter,CString p_attrib,double p_value)
{
  CString value;
  value.Format("%G",p_value);
  return SetAttribute(p_section,p_parameter,p_attrib,value);
}

// Remove an attribute
void
MarlinConfig::RemoveAttribute(XMLElement* p_parameter,CString p_attribute)
{
  // to be implemented
  if(DeleteAttribute(p_parameter,p_attribute))
  {
    m_changed = true;
  }
}

bool 
MarlinConfig::RemoveSection(CString p_section)
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
MarlinConfig::RemoveParameter(CString p_section,CString p_parameter)
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
MarlinConfig::RemoveAttribute(CString p_section,CString p_parameter,CString p_attrib)
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

CString 
MarlinConfig::GetParameterString(CString p_section,CString p_parameter,CString p_default)
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
MarlinConfig::GetParameterInteger(CString p_section,CString p_parameter,int p_default)
{
  CString param = GetParameterString(p_section,p_parameter,"");
  if(param.IsEmpty())
  {
    return p_default;
  }
  return atoi(param);
}

bool
MarlinConfig::GetParameterBoolean(CString p_section,CString p_parameter,bool p_default)
{
  CString param = GetParameterString(p_section,p_parameter,"");
  if(param.IsEmpty())
  {
    return p_default;
  }
  if(param.CompareNoCase("true") == 0)
  {
    return true;
  }
  if(param.CompareNoCase("false") == 0)
  {
    return false;
  }
  // Simply 0 or 1
  return (atoi(param) > 0);
}

CString 
MarlinConfig::GetEncryptedString (CString p_section,CString p_parameter,CString p_default)
{
  Crypto crypt;
  CString decrypted;

  try
  {
    CString encrypted = GetParameterString(p_section,p_parameter,"");
    if(!encrypted.IsEmpty())
    {
      decrypted = crypt.Decryption(encrypted,MARLINCONFIG_WACHTWOORD);
      int pos = decrypted.Find(':');
      if(pos > 0)
      {
        CString reversed = decrypted.Left(pos);
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

CString 
MarlinConfig::GetAttribute(CString p_section,CString p_parameter,CString p_attrib,CString p_default)
{
  CString val = p_default;

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
MarlinConfig::GetAttribute(CString p_section,CString p_parameter,CString p_attrib,int p_default)
{
  int val = p_default;

  XMLElement* section = FindElement(p_section);
  if(section)
  {
    XMLElement* param = FindElement(section,p_parameter);
    if(param)
    {
      return atoi(XMLMessage::GetAttribute(param,p_attrib));
    }
  }
  return val;
}

double
MarlinConfig::GetAttribute(CString p_section,CString p_parameter,CString p_attrib,double p_default)
{
  double val = p_default;

  XMLElement* section = FindElement(p_section);
  if(section)
  {
    XMLElement* param = FindElement(section,p_parameter);
    if(param)
    {
      return atof(XMLMessage::GetAttribute(param,p_attrib));
    }
  }
  return val;
}

// DISCOVERY

bool
MarlinConfig::HasSection(CString p_section)
{
  return FindSection(p_section) != nullptr;
}

bool    
MarlinConfig::HasParameter(CString p_section,CString p_parameter)
{
  XMLElement* section = FindSection(p_section);
  if(section != nullptr)
  {
    return FindParameter(section,p_parameter) != nullptr;
  }
  return false;
}

bool
MarlinConfig::HasAttribute(CString p_section,CString p_parameter,CString p_attribute)
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

CString
MarlinConfig::GetFilename()
{
  return m_fileName;
}

