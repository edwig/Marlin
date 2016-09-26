#pragma once
#include "XMLMessage.h"
#include <map>


typedef struct _iisSite
{
  CString m_name;
  CString m_binding;
  CString m_protocol;
  int     m_port;
  bool    m_secure;
  CString m_path;
}
IISSite;


using IISSites = std::map<CString,IISSite>;

class WebConfigIIS
{
public:
  WebConfigIIS(CString p_application = "");
 ~WebConfigIIS();
  
  // Read total config
  bool ReadConfig();

  // GETTERS
  
  // The fysical logfile path
  CString     GetLogfilePath() { return m_logpath; };
  // We do the detailed logging
  bool        GetDoLogging()   { return m_logging; };

private:
  // Read one config file
  bool        ReadConfig(CString p_configFile);
  // Replace environment variables in a string
  static bool ReplaceEnvironVars(CString& p_string);
  // Site registration
  IISSite*    GetSite(CString p_site);
  // Reading of the internal structures of a config file
  void        ReadLogPath(XMLMessage& p_msg);
  void        ReadSites  (XMLMessage& p_msg);

  // For specific web application, or juist defaults
  CString   m_application;

  IISSites  m_sites;
  CString   m_logpath;
  bool      m_logging { false };
};
