//////////////////////////////////////////////////////////////////////////
//
// USER-SPACE IMPLEMENTTION OF HTTP.SYS
//
// 2018 (c) ir. W.E. Huisman
// License: MIT
//
//////////////////////////////////////////////////////////////////////////

#pragma once
#include <vector>

class UrlGroup;
class Logfile;

using UrlGroups = std::vector<UrlGroup*>;

class ServerSession
{
public:
  ServerSession();
 ~ServerSession();

 ULONG    AddUrlGroup   (UrlGroup* p_group);
 bool     RemoveUrlGroup(UrlGroup* p_group);
 void     SetSocketLogging(int p_logging);

 int      GetSocketLogging() { return m_socketLogging; };
 Logfile* GetLogfile()       { return m_logfile;       };

private:
  void    CreateLogfile();

  UrlGroups         m_groups;
  Logfile*          m_logfile       { nullptr };
  int               m_socketLogging { SOCK_LOGGING_OFF };
  CRITICAL_SECTION  m_lock;
};
