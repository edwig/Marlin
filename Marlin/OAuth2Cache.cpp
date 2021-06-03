#include "stdafx.h"
#include "OAuth2Cache.h"
#include "HTTPMessage.h"
#include "HTTPClient.h"
#include "JSONMessage.h"

OAuth2Cache::OAuth2Cache()
{
}

OAuth2Cache::~OAuth2Cache()
{
}

// Create a credentials grant, returning a session ID
int
OAuth2Cache::CreateClientCredentialsGrant(CString p_url,CString p_appID,CString p_appKey)
{
  OAuthSession session;
  session.m_flow    = OAuthFlow::OA_CCR;
  session.m_url     = p_url;
  session.m_appID   = p_appID;
  session.m_appKey  = p_appKey;
  session.m_expires = 0L;

  m_cache.insert(std::make_pair(++m_nextSession,session));
  return m_nextSession;
}

CString
OAuth2Cache::GetBearerToken(int p_session,bool p_refresh /*= true*/)
{
  CString token;
  OAuthSession* session = FindSession(p_session);
  if(session)
  {
    if(GetIsExpired(p_session))
    {
      StartCientCredentialsGrant(session);
    }
    if(!GetIsExpired(p_session))
    {
      token = session->m_bearerToken;
    }
  }
  return token;
}

bool
OAuth2Cache::GetIsExpired(int p_session)
{
  bool expired = true;
  OAuthSession* session = FindSession(p_session);
  if(session)
  {
    INT64 now = clock();
    if(session->m_expires > now)
    {
      expired = false;
    }
  }
  return expired;
}

INT64
OAuth2Cache::GetExpires(int p_session)
{
  OAuthSession* session = FindSession(p_session);
  if(session)
  {
    return session->m_expires;
  }
  return 0L;
}

//////////////////////////////////////////////////////////////////////////
//
// PRIVATE
//
//////////////////////////////////////////////////////////////////////////

OAuthSession* 
OAuth2Cache::FindSession(int p_session)
{
  AuthCache::iterator it = m_cache.find(p_session);
  if(it != m_cache.end())
  {
    return &it->second;
  }
  return nullptr;
}

void
OAuth2Cache::StartCientCredentialsGrant(OAuthSession* p_session)
{
  // Reset the token
  p_session->m_bearerToken.Empty();
  p_session->m_expires = 0L;

  INT64 now = clock();

  // Getting a token from this URL
  HTTPMessage getToken(HTTPCommand::http_get,p_session->m_url);

  CString payload;

  getToken.SetBody(payload);
  HTTPClient client;
  if(client.Send(&getToken))
  {
    CString payload = getToken.GetBody();
    JSONMessage json;
    json.ParseMessage(payload);
    if(!json.GetErrorState())
    {
      JSONobject& object = json.GetValue().GetObject();
      for(auto& pair : object)
      {
        if(pair.m_name.CompareNoCase("token") == 0)
        {
          p_session->m_bearerToken = pair.m_value.GetString();
        }
        if(pair.m_name.CompareNoCase("Expires") == 0)
        {
          p_session->m_expires = now + (CLOCKS_PER_SEC * pair.m_value.GetNumberInt());
        }
      }
    }
  }
}
