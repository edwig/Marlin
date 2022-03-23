/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: OAuth2Cache.cpp
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
#include "stdafx.h"
#include "OAuth2Cache.h"
#include "HTTPMessage.h"
#include "HTTPClient.h"
#include "JSONMessage.h"
#include "AutoCritical.h"
#include "LogAnalysis.h"
#include <sys\timeb.h>

//////////////////////////////////////////////////////////////////////////
//
// Basic Manual for the OAuth2 cache.
//
// 1) See to it that you have a HTTPClient at hand. e.g. 'HTTPClient client;'
// 2) Create an OAuth2Cache nearby. e.g. 'OAuth2Cache cache;'
// 3) Create an OAuth2 session. e.g. 'int session = cache.CreateClientCredentialsGrant(...);'
// 4) Connect both the cache and the session to the HTTPClient. 'client.SetOAuth2Cache(&cache);'
// 5) and 'clilent.SetOAuth2Session(session);'
// 6) Your good to go. Send something: 'client.Send(myMessage);'
// 
// HTTPClient client;
// OAuht2Cache cache;
// int session = cache.CreateClientCredentialsGrant("https://app.org.com/token","12345...","A78WE...","FooBar");
// client.SetOAuth2Cache(&cache);
// client.SetOAuth2Session(session);
// client.Send(...)
//
// And of course, as always, check for errors, session == 0 etc :-)
//

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

OAuth2Cache::OAuth2Cache()
{
  InitializeCriticalSection(&m_lock);
}

OAuth2Cache::~OAuth2Cache()
{
  if(m_client)
  {
    delete m_client;
    m_client = nullptr;
  }
  DeleteCriticalSection(&m_lock);
}

// Create a token server URL from  a template and a tenant
XString
OAuth2Cache::CreateTokenURL(XString p_template,XString p_tenant)
{
  XString url;
  url.Format(p_template,p_tenant.GetString());
  return url;
}

// Create a credentials grant, returning a session ID
int
OAuth2Cache::CreateClientCredentialsGrant(XString p_url
                                         ,XString p_appID
                                         ,XString p_appKey
                                         ,XString p_scope)
{
  OAuthSession session;
  session.m_flow    = OAuthFlow::OA_CLIENT;
  session.m_url     = p_url;
  session.m_appID   = p_appID;
  session.m_appKey  = p_appKey;
  session.m_expires = 0L;
  session.m_scope   = CrackedURL::EncodeURLChars(p_scope,true);

  AutoCritSec lock(&m_lock);
  m_cache.insert(std::make_pair(++m_nextSession,session));
  return m_nextSession;
}

// Create a resource owner grant, returning a session ID
int
OAuth2Cache::CreateResourceOwnerCredentialsGrant(XString p_url
                                                ,XString p_appID
                                                ,XString p_appKey
                                                ,XString p_scope
                                                ,XString p_username
                                                ,XString p_password)
{
  OAuthSession session;
  session.m_flow     = OAuthFlow::OA_ROWNER;
  session.m_url      = p_url;
  session.m_appID    = p_appID;
  session.m_appKey   = p_appKey;
  session.m_expires  = 0L;
  session.m_scope    = CrackedURL::EncodeURLChars(p_scope);
  session.m_username = p_username;
  session.m_password = p_password;

  AutoCritSec lock(&m_lock);
  m_cache.insert(std::make_pair(++m_nextSession,session));
  return m_nextSession;
}

// Ending a session, removing from the cache
bool
OAuth2Cache::EndSession(int p_session)
{
  AutoCritSec lock(&m_lock);
  AuthCache::iterator it = m_cache.find(p_session);
  if(it != m_cache.end())
  {
    m_cache.erase(it);
    return true;
  }
  return false;
}

XString
OAuth2Cache::GetBearerToken(int p_session,bool p_refresh /*= false*/)
{
  AutoCritSec lock(&m_lock);

  XString token;
  OAuthSession* session = FindSession(p_session);
  if(session)
  {
    if(GetIsExpired(p_session) || p_refresh)
    {
      StartCredentialsGrant(session);
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
  AutoCritSec lock(&m_lock);

  bool expired = true;
  OAuthSession* session = FindSession(p_session);
  if(session)
  {
    __timeb64 now;
    _ftime64_s(&now);

    if(session->m_expires > now.time)
    {
      expired = false;
    }
  }
  return expired;
}

INT64
OAuth2Cache::GetExpires(int p_session)
{
  AutoCritSec lock(&m_lock);

  OAuthSession* session = FindSession(p_session);
  if(session)
  {
    return session->m_expires;
  }
  return 0L;
}

INT64
OAuth2Cache::GetDefaultExpirationPeriod() 
{
  return m_defaultPeriod;
}

void
OAuth2Cache::SetDefaultExpirationPeriod(INT64 p_default)
{ 
  m_defaultPeriod = p_default;
}

void
OAuth2Cache::SetAnalysisLog(LogAnalysis* p_logfile)
{
  m_logfile = p_logfile;
  if(m_client)
  {
    m_client->SetLogging(p_logfile);
  }
}

// Forces to get a new Bearer token
void
OAuth2Cache::SetExpired(int p_session)
{
  OAuthSession* session = FindSession(p_session);
  if(session)
  {
    session->m_expires = 0;
  }
}

void
OAuth2Cache::SetDevelopment(bool p_dev /*= true*/)
{
  m_development = p_dev;
}

// Slow lookup of a session
int
OAuth2Cache::GetHasSession(XString p_appID,XString p_appKey)
{
  for(auto& ses : m_cache)
  {
    if(ses.second.m_appID == p_appID && ses.second.m_appKey == p_appKey)
    {
      return ses.first;
    }
  }
  return 0;
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

HTTPClient*
OAuth2Cache::GetClient()
{
  if(!m_client)
  {
    m_client = new HTTPClient;
    if(m_logfile)
    {
      m_client->SetLogging(m_logfile);
    }
    if(m_development)
    {
      // Test environments are normally lax with certificates
      // So be prepared to deal with not completely right ones!
      m_client->SetRelaxOptions(SECURITY_FLAG_IGNORE_CERT_CN_INVALID   |
                                SECURITY_FLAG_IGNORE_CERT_DATE_INVALID | 
                                SECURITY_FLAG_IGNORE_UNKNOWN_CA        | 
                                SECURITY_FLAG_IGNORE_CERT_WRONG_USAGE  );
    }
  }
  return m_client;
}

void
OAuth2Cache::StartCredentialsGrant(OAuthSession* p_session)
{
  bool valid = false;
  XString typeFound;

  // Reset the token
  p_session->m_bearerToken.Empty();
  p_session->m_expires = 0L;

  // Getting the current time
  __timeb64 now;
  _ftime64_s(&now);

  // Getting a token from this URL with a POST from this message
  HTTPMessage getToken(HTTPCommand::http_post,p_session->m_url);
  getToken.SetContentType("application/x-www-form-urlencoded");
  getToken.AddHeader("Accept","application/json");
  getToken.SetUser    (p_session->m_appID);
  getToken.SetPassword(p_session->m_appKey);
  XString payload = CreateTokenRequest(p_session);
  getToken.SetBody(payload);
  
  // Send through extra HTTPClient
  HTTPClient* client = GetClient();
  client->SetPreEmptiveAuthorization(WINHTTP_AUTH_SCHEME_BASIC);

  if(client->Send(&getToken))
  {
    JSONMessage json(&getToken);
    if(!json.GetErrorState() && (json.GetStatus() == HTTP_STATUS_OK))
    {
      JSONobject& object = json.GetValue().GetObject();
      for(auto& pair : object)
      {
        if(pair.m_name.CompareNoCase("token_type") == 0)
        {
          typeFound = pair.m_value.GetString();
        }
        if(pair.m_name.CompareNoCase("access_token") == 0)
        {
          // Remember our token
          p_session->m_bearerToken = pair.m_value.GetString();
        }
        if(pair.m_name.CompareNoCase("expires_in") == 0)
        {
          // Expires after a percentage of the given time, so we get a new token in time!
          p_session->m_expires = now.time + (pair.m_value.GetNumberInt() * token_validity_time / 100);
        }
      }

      // Check if we have everything
      if(typeFound.CompareNoCase("bearer") == 0 && p_session->m_bearerToken.GetLength() > 0 )
      {
        valid = true;

        // Token expiration not given, use the default
        if(p_session->m_expires == 0)
        {
          p_session->m_expires = m_defaultPeriod;
        }

        if(m_logfile)
        {
          m_logfile->AnalysisLog(__FUNCTION__,LogType::LOG_INFO,true,"Received OAuth2 Bearer token from: %s",p_session->m_url.GetString());
        }
      }
    }
  }

  // In case of an error: reset everything!!
  if(!valid)
  {
    p_session->m_bearerToken.Empty();
    p_session->m_expires = 0;

    if(m_logfile)
    {
      BYTE*  response = nullptr;
      unsigned length = 0;
      m_client->GetResponse(response,length);

      m_logfile->AnalysisLog(__FUNCTION__,LogType::LOG_ERROR,true,"Invalid response from token server. HTTP [%d]",m_client->GetStatus());
      m_logfile->AnalysisLog(__FUNCTION__,LogType::LOG_ERROR,false,(char*)response);
    }
  }
}

XString
OAuth2Cache::CreateTokenRequest(OAuthSession* p_session)
{
  XString request;
  request.Format("client_id=%s",           p_session->m_appID.GetString());
  request.AppendFormat("&scope=%s",        p_session->m_scope.GetString());
  request.AppendFormat("&client_secret=%s",p_session->m_appKey.GetString());
  request += "&grant_type=";
  switch (p_session->m_flow)
  {
    case OAuthFlow::OA_CLIENT:  request += "client_credentials"; 
                                break;
    case OAuthFlow::OA_ROWNER:  request += "password";           
                                request.AppendFormat("&username=%s",p_session->m_username.GetString());
                                request.AppendFormat("&password=%s",p_session->m_password.GetString());
                                break;
    default:                    // Not implemented flows
                                request.Empty();
                                break;
  }
  return request;
}
