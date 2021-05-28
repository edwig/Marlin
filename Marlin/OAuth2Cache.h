#pragma once
#include <map>


typedef struct _oauthSession
{
  CString m_url;
  CString m_appID;
  CString m_appKey;
  CString m_bearerToken;
  CString m_retryToken;
  INT64   m_expires;
}
OAuthSession;


using AuthCache = std::map<int,OAuthSession*>;

class OAuth2Cache
{
public:
  OAuth2Cache();
 ~OAuth2Cache();

  // FUNCIONS

  // Create a credentials grant, returning a session ID
  int       CreateClientCredentialsGrant(CString p_url,CString p_appID,CString p_appKey);

  // GETTERS
  CString   GetBearerToken(int p_session,bool p_refresh = true);
  CString   GetBearerToken(CString p_url,bool p_refresh = true);
  bool      GetIsExpired(int p_session);
  INT64     GetExpires(int p_session);

private:
  AuthCache m_cache;
  INT64     m_defaultPeriod { 7 * 60 * CLOCKS_PER_SEC };
  int       m_nextSession   { 0 };
};


