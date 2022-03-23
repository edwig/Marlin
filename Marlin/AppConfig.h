/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: AppConfig.h
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

#pragma once
#include "XMLMessage.h"

// DEFAULTS for the application config file: "<PRODUCT_NAME>.Config"

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

class AppConfig : public XMLMessage
{
public:
  AppConfig(XString p_rootname);
  virtual ~AppConfig();

  // Read the config from disk
  bool ReadConfig();
  // Write back the config to disk
  bool WriteConfig();

  // GETTERS

  // The name of the server
  XString  GetName();
  // Getting the role of the server
  XString  GetRole();
  // Getting the host server
  XString  GetServer();
  // Server URL is not stored in this form
  XString  GetServerURL();
  // Getting secure server or not
  bool     GetServerSecure();
  // Base URL of the server itself
  XString  GetBaseURL();
  // Server port of the server itself
  int      GetServerPort();
  // The instance number
  int      GetInstance();
  // The server logfile
  XString  GetServerLogfile();
  // Should the server do extra logging
  int      GetServerLoglevel();
  // Is the config file writable
  bool     GetConfigWritable();
  // The config file as an absolute pathname
  XString  GetConfigFilename();
  // Get the webroot
  XString  GetWebRoot();
  // Get run-as status
  int      GetRunAsService();

  // SETTERS

  void  SetName(XString p_name);
  void  SetRole(XString p_role);
  void  SetInstance(int p_instance);
  void  SetServer(XString p_server);
  void  SetBaseURL(XString p_baseURL);
  void  SetServerSecure(bool p_secure);
  void  SetWebRoot(XString p_webroot);
  void  SetServerLog(XString p_logfile);
  void  SetServerPort(unsigned p_port);
  void  SetServerLoglevel(int p_loglevel);
  void  SetRunAsService(int p_service);

protected:
  // OVERRIDES FOR YOUR APPLICATION

  // Check the root node for the correct config file
  virtual bool CheckRootNodeName();
  // Check for consistency of members
  virtual bool CheckConsistency();
  // Remember a parameter
  virtual bool AddParameter(XString& p_param,XString& p_value);
  // Adding extra elements to the file
  virtual void WriteConfigElements();

  // Clean before write
  virtual void Clean();

  XString m_rootname;

private:
  XString  m_name;               // Name of the server or IIS application-pool name
  XString  m_role;               // Server / Client / Server&Client
  int      m_instance;           // Between 1 and 100
  XString  m_server;             // Server host name
  bool     m_secure;             // HTTP or HTTPS
  unsigned m_serverPort;         // Server input port number
  XString  m_baseUrl;            // Base URL only
  XString  m_serverLog;          // Server logfile path name
  int      m_serverLoglevel;     // See HTTPLoglevel.h
  XString  m_webroot;
  int      m_runAsService;       // Start method RUNAS_*
};

inline XString
AppConfig::GetName()
{
  return m_name;
}

inline void
AppConfig::SetName(XString p_name)
{
  m_name = p_name;
}

inline int
AppConfig::GetInstance()
{
  return m_instance;
}

inline void
AppConfig::SetInstance(int p_instance)
{
  m_instance = p_instance;
}

inline XString
AppConfig::GetServer()
{
  return m_server;
}

inline int
AppConfig::GetServerLoglevel()
{
  return m_serverLoglevel;
}

inline void
AppConfig::SetServer(XString p_server)
{
  m_server = p_server;
}

inline void
AppConfig::SetServerPort(unsigned p_port)
{
  m_serverPort = p_port;
}

inline void
AppConfig::SetServerLoglevel(int p_loglevel)
{
  m_serverLoglevel = p_loglevel;
}

inline void
AppConfig::SetWebRoot(XString p_webroot)
{
  m_webroot = p_webroot;
}

inline XString
AppConfig::GetWebRoot()
{
  return m_webroot;
}

inline XString
AppConfig::GetServerLogfile()
{
  return m_serverLog;
}

inline void
AppConfig::SetServerLog(XString p_logfile)
{
  m_serverLog = p_logfile;
}

inline int
AppConfig::GetRunAsService()
{
  return m_runAsService;
}

inline void
AppConfig::SetRunAsService(int p_service)
{
  m_runAsService = p_service;
}

inline void
AppConfig::SetServerSecure(bool p_secure)
{
  m_secure = p_secure;
}

inline void
AppConfig::SetBaseURL(XString p_baseURL)
{
  m_baseUrl = p_baseURL;
}

inline XString
AppConfig::GetBaseURL()
{
  return m_baseUrl;
}

inline int
AppConfig::GetServerPort()
{
  return m_serverPort;
}

inline bool
AppConfig::GetServerSecure()
{
  return m_secure;
}

inline void
AppConfig::SetRole(XString p_role)
{
  m_role = p_role;
}

inline XString
AppConfig::GetRole()
{
  return m_role;
}
