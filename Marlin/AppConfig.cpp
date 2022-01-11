/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: AppConfig.cpp
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
#include "stdafx.h"
#include "Marlin.h"
#include "AppConfig.h"
#include "MarlinServer.h"
#include "MarlinConfig.h"
#include <winhttp.h>
#include <io.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

AppConfig::AppConfig(CString p_rootname)
          :m_rootname(p_rootname)
{
  m_name               = DEFAULT_NAME;
  m_instance           = DEFAULT_INSTANCE;
  m_server             = DEFAULT_SERVER;
  m_secure             = false;
  m_serverPort         = INTERNET_DEFAULT_HTTP_PORT;
  m_baseUrl            = DEFAULT_URL;
  m_serverLog          = DEFAULT_SERVERLOG;
  m_serverLoglevel      = false;
  m_webroot            = DEFAULT_WEBROOT;
  m_runAsService       = false;
}

AppConfig::~AppConfig()
{
}

// Server URL is not stored in this form
// But  it is used in many places.
CString
AppConfig::GetServerURL()
{
  CString MDAServerURL;

  if (m_serverPort == INTERNET_DEFAULT_HTTP_PORT)
  {
    MDAServerURL.Format("http://%s%s", m_server.GetString(), m_baseUrl.GetString());
  }
  else if (m_serverPort == INTERNET_DEFAULT_HTTPS_PORT)
  {
    MDAServerURL.Format("https://%s%s", m_server.GetString(), m_baseUrl.GetString());
  }
  else
  {
    MDAServerURL.Format("http://%s:%d%s", m_server.GetString(), m_serverPort, m_baseUrl.GetString());
  }
  return MDAServerURL;
}

// Is the config file writable
bool
AppConfig::GetConfigWritable()
{
  CString fileNaam = GetConfigFilename();
  if (_access(fileNaam, 00) == -1)
  {
    // File does not exist. Assume that we can create it
    // for instance in the ServerApplet config dialog
    return true;
  }
  if (_access(fileNaam, 06) == 0)
  {
    // Read AND write access tot the file
    return true;
  }
  return false;
}

// The config file as an absolute pathname
CString
AppConfig::GetConfigFilename()
{
  // This is our config file
  return MarlinConfig::GetExePath() + PRODUCT_NAME + ".Config";
}

// Read the config from disk
bool
AppConfig::ReadConfig()
{
  // This is our config file
  CString fileName = GetConfigFilename();

  if(XMLMessage::LoadFile(fileName) == false)
  {
    return false;
  }

  // Check if it is our config file
  if(!CheckRootNodeName())
  {
    return false;
  }

  XMLElement* node = GetElementFirstChild(m_root);
  while (node)
  {
    CString param = node->GetName();
    CString value = node->GetValue();

    // Remember
    AddParameter(param,value);

    // Next node
    node = GetElementSibling(node);
  }

  // Post reading checks, see if everything OK
  return CheckConsistency();
}

// Write back the config to disk
bool
AppConfig::WriteConfig()
{
  // Post settings checks
  CheckConsistency();

  // Set the root node name, possibly not done yet
  m_root->SetName(m_rootname);

  // Rebuild the XML structure
  Clean();

  SetElement("Name",          m_name);
  SetElement("Role",          m_role);
  SetElement("Instance",      m_instance);
  SetElement("Server",        m_server);
  SetElement("Secure",        m_secure);
  SetElement("ServerPort",    (int)m_serverPort);
  SetElement("BaseURL",       m_baseUrl);
  SetElement("ServerLog", m_serverLog);
  SetElement("ServerLogging", m_serverLoglevel);
  SetElement("WebRoot",       m_webroot);
  SetElement("RunAsService",  m_runAsService);

  WriteConfigElements();

  // Writing to file is the result
  return SaveFile(GetConfigFilename());
}

// In the base class this is always ok
// Used to just read the base members.
// E.g. in the ServerMain startup methods
bool
AppConfig::CheckRootNodeName()
{
  return true;
}

// Adding extra elements to the file
// Used in derived classes only
void 
AppConfig::WriteConfigElements()
{
}

//////////////////////////////////////////////////////////////////////////
//
// PROTECTED METHODS
//
//////////////////////////////////////////////////////////////////////////

// Remember a parameter
bool
AppConfig::AddParameter(CString& p_param,CString& p_value)
{
       if(p_param.Compare("Name")          == 0) m_name           = p_value;
  else if(p_param.Compare("Role")          == 0) m_role           = p_value;
  else if(p_param.Compare("Instance")      == 0) m_instance       = atoi(p_value);
  else if(p_param.Compare("Server")        == 0) m_server         = p_value;
  else if(p_param.Compare("Secure")        == 0) m_secure         = (p_value.CompareNoCase("true") == 0);
  else if(p_param.Compare("ServerPort")    == 0) m_serverPort     = atoi(p_value);
  else if(p_param.Compare("BaseURL")       == 0) m_baseUrl        = p_value;
  else if(p_param.Compare("ServerLog")     == 0) m_serverLog      = p_value;
  else if(p_param.Compare("ServerLogging") == 0) m_serverLoglevel = atoi(p_value);
  else if(p_param.Compare("WebRoot")       == 0) m_webroot        = p_value;
  else if(p_param.Compare("RunAsService")  == 0) m_runAsService   = atoi(p_value);
  else return false;

  return true;
}

// Check for consistency of members
bool
AppConfig::CheckConsistency()
{
  // Name
  if(m_name.IsEmpty())
  {
    m_name = DEFAULT_NAME;
  }

  // Instance number
  if(m_instance <=  0)              m_instance = DEFAULT_INSTANCE;
  if(m_instance > MAXIMUM_INSTANCE) m_instance = MAXIMUM_INSTANCE;

  // Server name
  if(m_server.IsEmpty())
  {
    m_server = DEFAULT_SERVER;
  }

  // CHeck the BaseURL
  if(m_baseUrl.IsEmpty())
  {
    m_baseUrl = PRODUCT_SITE;
  }

  // Port numbers back to default?
  if(m_serverPort != INTERNET_DEFAULT_HTTP_PORT  && 
     m_serverPort != INTERNET_DEFAULT_HTTPS_PORT && 
     m_serverPort < 1025)
  {
    m_serverPort = DEFAULT_SERVERPORT;
  }

  // Log files
  if(m_serverLog.IsEmpty())
  {
    m_serverLog = DEFAULT_SERVERLOG;
  }

  // Check webroot
  if(m_webroot.IsEmpty())
  {
    m_webroot = DEFAULT_WEBROOT;
  }
  else
  {
    // Must end on a directory separator 
    if(m_webroot.Right(1) != "\\" && m_webroot.Right(1) != "/")
    {
      m_webroot += "\\";
    }
  }

  // Check for the base URL 
  // It needs to begin and end with an '/' marker
  if(m_baseUrl.GetLength() > 1)
  {
    if(m_baseUrl.Left(1) != "/")
    {
      m_baseUrl = "/" + m_baseUrl;
    }
    if(m_baseUrl.Right(1) != "/")
    {
      m_baseUrl += "/";
    }
  }

  return true;
}

// Clean the body to put something different in it
// The SOAP Fault or the body encryption
void
AppConfig::Clean()
{
  XMLElement* child = nullptr;
  do
  {
    child = GetElementFirstChild(m_root);
    if (child)
    {
      DeleteElement(m_root, child);
    }
  } while (child);
}

