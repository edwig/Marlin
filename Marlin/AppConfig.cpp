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
#include "MarlinServer.h"
#include "MarlinConfig.h"
#include <winhttp.h>
#include <io.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

AppConfig::AppConfig(XString p_rootname)
          :m_rootname  (p_rootname)
	  ,m_name      (DEFAULT_NAME)
	  ,m_instance  (DEFAULT_INSTANCE)
	  ,m_server    (DEFAULT_SERVER)
	  ,m_secure    (false)
	  ,m_serverPort(INTERNET_DEFAULT_HTTPS_PORT)
	  ,m_baseUrl   (DEFAULT_URL)
	  ,m_webroot   (DEFAULT_WEBROOT)
	  ,m_serverLog (DEFAULT_SERVERLOG)
	  ,m_serverLoglevel(false)
	  ,m_runAsService(false)
{
}

AppConfig::~AppConfig()
{
}

// Server URL is not stored in this form
// But  it is used in many places.
XString
AppConfig::GetServerURL()
{
  XString MDAServerURL;

  if (m_serverPort == INTERNET_DEFAULT_HTTP_PORT)
  {
    MDAServerURL.Format(_T("http://%s%s"), m_server.GetString(), m_baseUrl.GetString());
  }
  else if (m_serverPort == INTERNET_DEFAULT_HTTPS_PORT)
  {
    MDAServerURL.Format(_T("https://%s%s"), m_server.GetString(), m_baseUrl.GetString());
  }
  else
  {
    MDAServerURL.Format(_T("http://%s:%d%s"), m_server.GetString(), m_serverPort, m_baseUrl.GetString());
  }
  return MDAServerURL;
}

// Is the config file writable
bool
AppConfig::GetConfigWritable()
{
  XString fileNaam = GetConfigFilename();
  if (_taccess(fileNaam, 00) == -1)
  {
    // File does not exist. Assume that we can create it
    // for instance in the ServerApplet config dialog
    return true;
  }
  if (_taccess(fileNaam, 06) == 0)
  {
    // Read AND write access tot the file
    return true;
  }
  return false;
}

// The config file as an absolute pathname
XString
AppConfig::GetConfigFilename()
{
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

  XMLElement* node = GetElementFirstChild(m_root);
  while (node)
  {
    XString param = node->GetName();
    XString value = node->GetValue();

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

  SetElement(_T("Name"),          m_name);
  SetElement(_T("Role"),          m_role);
  SetElement(_T("Instance"),      m_instance);
  SetElement(_T("Server"),        m_server);
  SetElement(_T("Secure"),        m_secure);
  SetElement(_T("ServerPort"),    (int)m_serverPort);
  SetElement(_T("BaseURL"),       m_baseUrl);
  SetElement(_T("ServerLog"),     m_serverLog);
  SetElement(_T("ServerLogging"), m_serverLoglevel);
  SetElement(_T("WebRoot"),       m_webroot);
  SetElement(_T("RunAsService"),  m_runAsService);

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
AppConfig::AddParameter(XString& p_param,XString& p_value)
{
       if(p_param.Compare(_T("Name"))          == 0) m_name           = p_value;
  else if(p_param.Compare(_T("Role"))          == 0) m_role           = p_value;
  else if(p_param.Compare(_T("Instance"))      == 0) m_instance       = _ttoi(p_value);
  else if(p_param.Compare(_T("Server"))        == 0) m_server         = p_value;
  else if(p_param.Compare(_T("Secure"))        == 0) m_secure         = (p_value.CompareNoCase(_T("true")) == 0);
  else if(p_param.Compare(_T("ServerPort"))    == 0) m_serverPort     = _ttoi(p_value);
  else if(p_param.Compare(_T("BaseURL"))       == 0) m_baseUrl        = p_value;
  else if(p_param.Compare(_T("ServerLog"))     == 0) m_serverLog      = p_value;
  else if(p_param.Compare(_T("ServerLogging")) == 0) m_serverLoglevel = _ttoi(p_value);
  else if(p_param.Compare(_T("WebRoot"))       == 0) m_webroot        = p_value;
  else if(p_param.Compare(_T("RunAsService"))  == 0) m_runAsService   = _ttoi(p_value);
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
    if(m_webroot.Right(1) != _T("\\") && m_webroot.Right(1) != _T("/"))
    {
      m_webroot += _T("\\");
    }
  }

  // Check for the base URL 
  // It needs to begin and end with an '/' marker
  if(m_baseUrl.GetLength() > 1)
  {
    if(m_baseUrl.Left(1) != _T("/"))
    {
      m_baseUrl = _T("/") + m_baseUrl;
    }
    if(m_baseUrl.Right(1) != _T("/"))
    {
      m_baseUrl += _T("/");
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

