//////////////////////////////////////////////////////////////////////////
//
// USER-SPACE IMPLEMENTTION OF HTTP.SYS
//
// 2018 (c) ir. W.E. Huisman
// License: MIT
//
//////////////////////////////////////////////////////////////////////////

#pragma once
#include <map>
#include <string>

// Test to see if it is still a UrlGroup object
#define HTTP_URLGROUP_IDENT 0x0000EDED0000EDED

class ServerSession;
class RequestQueue;

// Mappings of URLS in the URL group
using URLNames = std::multimap<CString,URL>;
using std::wstring;

class UrlGroup
{
public:
  UrlGroup(ServerSession* p_session);
 ~UrlGroup();

  // FUNCTIONS
  ULONG AddUrlPrefix(CString pFullyQualifiedUrl,HTTP_URL_CONTEXT UrlContext);
  ULONG DelUrlPrefix(CString pFullyQualifiedUrl,ULONG p_flags);
  ULONG NumberOfPorts (USHORT p_port);
  URL*  FindLongestURL(USHORT p_port,CString p_abspath,int& p_length);

  // SETTERS
  void SetRequestQueue(HANDLE p_requestQueue);
  void SetEnabledState(HTTP_ENABLED_STATE p_state);
  void SetTimeoutEntityBody    (USHORT p_timeout) { m_timeoutEntityBody      = p_timeout; };
  void SetTimeoutDrainBody     (USHORT p_timeout) { m_timeoutDrainEntityBody = p_timeout; };
  void SetTimeoutRequestQueue  (USHORT p_timeout) { m_timeoutRequestQueue    = p_timeout; };
  void SetTimeoutIdleConnection(USHORT p_timeout) { m_timeoutIdleConnection  = p_timeout; };
  void SetTimeoutHeaderWait    (USHORT p_timeout) { m_timeoutHeaderWait      = p_timeout; };
  void SetTimeoutMinSendRate   (ULONG  p_rate)    { m_timeoutMinSendRate     = p_rate;    };
  void SetAuthentication(ULONG p_scheme,CString p_domain,CString p_realm,bool p_caching);
  void SetAuthenticationWide(wstring p_domain,wstring p_realm);

  // GETTERS
  ULONGLONG           GetIdent()                    { return m_ident;                   };
  ServerSession*      GetServerSession()            { return m_session;                 };
  RequestQueue*       GetRequestQueue()             { return m_queue;                   };
  LONG                GetNumberOfUrls()             { return (LONG)m_urls.size();       };
  HTTP_ENABLED_STATE  GetEnabledState()             { return m_state;                   };
  USHORT              GetTimeoutEntityBody()        { return m_timeoutEntityBody;       };
  USHORT              GetTimeoutDrainEntityBody()   { return m_timeoutDrainEntityBody;  };
  USHORT              GetTimeoutRequestQueue()      { return m_timeoutRequestQueue;     };
  USHORT              GetTimeoutIdleConnection()    { return m_timeoutIdleConnection;   };
  USHORT              GetTimeoutHeaderWait()        { return m_timeoutHeaderWait;       };
  ULONG               GetTimeoutMinSendRate()       { return m_timeoutMinSendRate;      };
  ULONG               GetAuthenticationScheme()     { return m_scheme;                  };
  CString             GetAuthenticationDomain()     { return m_domain;                  };
  CString             GetAuthenticationRealm()      { return m_realm;                   };
  bool                GetAuthenticationCaching()    { return m_ntlmCaching;             };
  wstring             GetAuthenticationDomainWide() { return m_domainWide;              };
  wstring             GetAuthenticationRealmWide()  { return m_realmWide;               };

private:
  int                 SegmentedCompare(const char* p_left,const char* p_right);
  bool                UrlIsRegistered(CString p_prefix);
  bool                GetURLSettings(URL& p_url);

  // Primary identity
  ULONGLONG          m_ident { HTTP_URLGROUP_IDENT };
  ServerSession*     m_session;
  RequestQueue*      m_queue { NULL };
  HTTP_ENABLED_STATE m_state;
  // Timeouts
  USHORT             m_timeoutEntityBody        { URL_TIMEOUT_ENTITY_BODY     };
  USHORT             m_timeoutDrainEntityBody   { URL_TIMEOUT_DRAIN_BODY      };
  USHORT             m_timeoutRequestQueue      { URL_TIMEOUT_REQUEST_QUEUE   };
  USHORT             m_timeoutIdleConnection    { URL_TIMEOUT_IDLE_CONNECTION };
  USHORT             m_timeoutHeaderWait        { URL_TIMEOUT_HEADER_WAIT     };
  ULONG              m_timeoutMinSendRate       { URL_DEFAULT_MIN_SEND_RATE   };
  // URLS to service!
  URLNames           m_urls;
  // Authentication type of the URL group
  ULONG              m_scheme { 0 };
  CString            m_domain;
  CString            m_realm;
  wstring            m_domainWide;
  wstring            m_realmWide;
  bool               m_ntlmCaching { true };
  // Locking
  CRITICAL_SECTION   m_lock;
};

inline UrlGroup*
GetUrlGroupFromHandle(HTTP_URL_GROUP_ID p_handle)
{
  try
  {
    UrlGroup* group = reinterpret_cast<UrlGroup*>(p_handle);
    if(group && group->GetIdent() == HTTP_URLGROUP_IDENT)
    {
      return group;
    }
  }
  catch(...)
  {
    // Error in application: Not a Request handle
  }
  return nullptr;
}
