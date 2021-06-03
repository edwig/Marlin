#pragma once
#include <map>

enum class OAuthFlow
{
  OA_CCR = 1  // OAuth2 Client Credentials Grant
};

typedef struct _oauthSession
{
  OAuthFlow m_flow;           // Type of authorization flow
  CString   m_url;            // URL of the token server
  CString   m_appID;          // Client-id of the application
  CString   m_appKey;         // Client-secret of the application
  CString   m_bearerToken;    // Last returned "Bearer" token
  CString   m_retryToken;     // Retry token (if any)
  INT64     m_expires;        // Moment the token expires
}
OAuthSession;


using AuthCache = std::map<int,OAuthSession>;

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
  INT64     GetDefaultExpirationPeriod()                { return m_defaultPeriod;      }

  // SETTERS
  void      SetDefaultExpirationPeriod(INT64 p_default) { m_defaultPeriod = p_default; }

private:
  OAuthSession* FindSession(int p_session);
  void          StartCientCredentialsGrant(OAuthSession* p_session);

  AuthCache m_cache;
  INT64     m_defaultPeriod { 7 * 60 * CLOCKS_PER_SEC };
  int       m_nextSession   { 0 };
};


