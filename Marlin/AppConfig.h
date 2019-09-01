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
#define DEFAULT_SPINCOUNT     2000
#define MINIMUM_SPINCOUNT     1000
#define MAXIMUM_SPINCOUNT   100000
#define DEFAULT_TIMEOUTCONN   5000
#define MINIMUM_TIMEOUTCONN   3000
#define MAXIMUM_TIMEOUTCONN  60000
#define DEFAULT_RETRYCOUNT       3
#define MAXIMUM_RETRYCOUNT      10
#define DEFAULT_CONNTIMEOUT  10000
#define MINIMUM_CONNTIMEOUT   5000
#define MAXIMUM_CONNTIMEOUT  30000

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
  // Should the client do logging
  bool     GetClientLogging();
  // Should the server do extra logging
  bool     GetServerLogging();
  // Is the config file writable
  bool     GetConfigWritable();
  // The config file as an absolute pathname
  CString  GetConfigFilename();
  // Get the webroot
  CString  GetWebRoot();
  // Get the base directory
  CString  GetBaseDirectory();
  // Get the CriticalSection's spin count
  DWORD    GetSpinCount();
  // Get timeout Connect
  DWORD    GetTimeoutConnect();
  // Get run-as status
  bool     GetRunAsService();
  // Connection pulse timeout
  int      GetConnectionTimeout();
  // Lifetime of a connection in seconds
  int      GetLifetimeConnection();

  // SETTERS

  void  SetName(CString p_name);
  void  SetInstance(int p_instance);
  void  SetServer(CString p_server);
  void  SetBaseURL(CString p_baseURL);
  void  SetServerSecure(bool p_secure);
  void  SetWebRoot(CString p_webroot);
  void  SetBaseDirectory(CString p_baseDirectory);
  void  SetClientLog(CString p_logfile);
  void  SetServerLog(CString p_logfile);
  void  SetUserAgent(CString p_agent);
  void  SetServerPort(unsigned p_port);
  void  SetClientPort(unsigned p_port);
  void  SetClientLogging(bool p_logging);
  void  SetServerLogging(bool p_logging);
  void  SetClientStartType(int p_startType);
  void  SetSpinCount(DWORD p_spinCount);
  void  SetPortOffsets(bool p_offsets);
  void  SetPushInterface(bool p_push);
  void  SetTimeoutConnect(DWORD p_timeout);
  void  SetRunAsService(bool p_service);
  void  SetCheckIncoming(bool p_check);
  void  SetCheckOutgoing(bool p_check);
  void  SetConnectionTimeout(int p_timeout);
  void  SetLifetimeConnection(int p_seconds);

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
  CString  m_name;
  int      m_instance;
  CString  m_server;
  bool     m_secure;
  unsigned m_serverPort;
  CString  m_baseUrl;
  CString  m_serverLog;
  bool     m_serverLogging;
  CString  m_baseDirectory;
  CString  m_webroot;
  CString  m_agent;
  DWORD    m_spinCount;
  DWORD    m_timeoutConnect;
  bool     m_runAsService;
  int      m_connectionTimeout;
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

inline bool
AppConfig::GetServerLogging()
{
  return m_serverLogging;
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
AppConfig::SetServerLogging(bool p_logging)
{
  m_serverLogging = p_logging;
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

inline DWORD
AppConfig::GetSpinCount()
{
  return m_spinCount;
}

inline void
AppConfig::SetSpinCount(DWORD p_spinCount)
{
  m_spinCount = p_spinCount;
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

inline DWORD
AppConfig::GetTimeoutConnect()
{
  return m_timeoutConnect;
}

inline void
AppConfig::SetTimeoutConnect(DWORD p_timeout)
{
  m_timeoutConnect = p_timeout;
}

inline bool
AppConfig::GetRunAsService()
{
  return m_runAsService;
}

inline void
AppConfig::SetRunAsService(bool p_service)
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

inline CString
AppConfig::GetBaseDirectory()
{
  return m_baseDirectory;
}

inline void
AppConfig::SetBaseDirectory(CString p_baseDirectory)
{
  m_baseDirectory = p_baseDirectory;
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
