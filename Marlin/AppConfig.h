#pragma once

#pragma once
#include "XMLMessage.h"

// DEFAULTS for the application config file: "<PRODUCT_NAME>.Config"

#define DEFAULT_NAME        "<DTAP Name>"   // DTAP = "Development", "Test", "Acceptance", "Production"
#define DEFAULT_SERVER      "localhost"
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
  AppConfig(CString p_rootname);
  virtual ~AppConfig();

  // Read the config from disk
  bool ReadConfig();
  // Write back the config to disk
  bool WriteConfig();

  // GETTERS

  // The name of the server
  CString  GetName();
  // Getting the role of the server
  CString  GetRole();
  // Getting the host server
  CString  GetServer();
  // Server URL is not stored in this form
  CString  GetServerURL();
  // Getting secure server or not
  bool     GetServerSecure();
  // Base URL of the server itself
  CString  GetBaseURL();
  // Server port of the server itself
  int      GetServerPort();
  // The instance number
  int      GetInstance();
  // The server logfile
  CString  GetServerLogfile();
  // Should the server do extra logging
  int      GetServerLoglevel();
  // Is the config file writable
  bool     GetConfigWritable();
  // The config file as an absolute pathname
  CString  GetConfigFilename();
  // Get the webroot
  CString  GetWebRoot();
  // Get run-as status
  int      GetRunAsService();

  // SETTERS

  void  SetName(CString p_name);
  void  SetRole(CString p_role);
  void  SetInstance(int p_instance);
  void  SetServer(CString p_server);
  void  SetBaseURL(CString p_baseURL);
  void  SetServerSecure(bool p_secure);
  void  SetWebRoot(CString p_webroot);
  void  SetServerLog(CString p_logfile);
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
  virtual bool AddParameter(CString& p_param,CString& p_value);
  // Adding extra elements to the file
  virtual void WriteConfigElements();

  // Clean before write
  virtual void Clean();

  CString m_rootname;

private:
  CString  m_name;               // Name of the server or IIS application-pool name
  CString  m_role;               // Server / Client / Server&Client
  int      m_instance;           // Between 1 and 100
  CString  m_server;             // Server host name
  bool     m_secure;             // HTTP or HTTPS
  unsigned m_serverPort;         // Server input port number
  CString  m_baseUrl;            // Base URL only
  CString  m_serverLog;          // Server logfile path name
  int      m_serverLoglevel;     // See HTTPLoglevel.h
  CString  m_webroot;
  int      m_runAsService;       // Start method RUNAS_*
};

inline CString
AppConfig::GetName()
{
  return m_name;
}

inline void
AppConfig::SetName(CString p_name)
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

inline CString
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
AppConfig::SetServer(CString p_server)
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
AppConfig::SetWebRoot(CString p_webroot)
{
  m_webroot = p_webroot;
}

inline CString
AppConfig::GetWebRoot()
{
  return m_webroot;
}

inline CString
AppConfig::GetServerLogfile()
{
  return m_serverLog;
}

inline void
AppConfig::SetServerLog(CString p_logfile)
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
AppConfig::SetBaseURL(CString p_baseURL)
{
  m_baseUrl = p_baseURL;
}

inline CString
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
AppConfig::SetRole(CString p_role)
{
  m_role = p_role;
}

inline CString
AppConfig::GetRole()
{
  return m_role;
}
