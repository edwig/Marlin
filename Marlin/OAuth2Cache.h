/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: OAuth2Cache.h
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
#include <map>

enum class OAuthFlow
{
  OA_IMPLICIT = 1 // OAuth2 Implicit Credentials grant
 ,OA_CLIENT       // OAuth2 Client Credentials Grant  (client-id and client-key)
 ,OA_ROWNER       // OAuth2 Resource Owner Credentials Grant (username/password)
 ,OA_AUTCODE      // OAuth2 Authorization Code Credentials Grant
 ,OA_ASSERT       // OAuth2 Assertion Credentials Grant
};

// Well known token servers
constexpr const char* token_server_ms("https://login.microsoftonline.com/%s/oauth2/v2.0/token");

// Well known scopes
constexpr const char* scope_ms_graph("https://graph.microsoft.com/.default");

// Bearer tokens will be re-gotten after % of the expiration time
const int token_validity_time = 90;  // Refresh after 90 percent of time has expired

typedef struct _oauthSession
{
  OAuthFlow m_flow;           // Type of authorization flow
  CString   m_url;            // URL of the token server
  CString   m_appID;          // Client-id of the application
  CString   m_appKey;         // Client-secret of the application
  CString   m_username;       // For Resource-owners only!
  CString   m_password;       // For Resource-owners only!
  CString   m_scope;          // Scope of the grant
  CString   m_bearerToken;    // Last returned "Bearer" token
  CString   m_retryToken;     // Retry token (if any)
  INT64     m_expires;        // Moment the token expires
}
OAuthSession;

class HTTPClient;
class LogAnalysis;
using AuthCache = std::map<int,OAuthSession>;

class OAuth2Cache
{
public:
  OAuth2Cache();
 ~OAuth2Cache();

  // FUNCIONS

  // Create a token server URL from  a template and a tenant
  CString   CreateTokenURL(CString p_template,CString p_tenant);
  // Create a credentials grant, returning a session ID
  int       CreateClientCredentialsGrant(CString p_url,CString p_appID,CString p_appKey,CString p_scope);
  // Create a resource owner grant, returning a session ID
  int       CreateResourceOwnerCredentialsGrant(CString p_url,CString p_appID,CString p_appKey,CString p_scope,CString p_username,CString p_password);
  // Ending a session, removing from the cache
  bool      EndSession(int p_session);

  // GETTERS
  CString   GetBearerToken(int p_session,bool p_refresh = true);
  bool      GetIsExpired(int p_session);
  INT64     GetExpires(int p_session);
  INT64     GetDefaultExpirationPeriod();
  int       GetHasSession(CString p_appID,CString p_appKey);

  // SETTERS
  void      SetExpired(int p_session);
  void      SetAnalysisLog(LogAnalysis* p_logfile);
  void      SetDefaultExpirationPeriod(INT64 p_default);
  void      SetDevelopment(bool p_dev = true);

private:
  OAuthSession* FindSession(int p_session);
  void          StartCredentialsGrant(OAuthSession* p_session);
  CString       CreateTokenRequest(OAuthSession* p_session);
  HTTPClient*   GetClient();

  AuthCache     m_cache;                       // All cached authentications
  HTTPClient*   m_client        { nullptr };   // To send to the token server
  LogAnalysis*  m_logfile       { nullptr };   // Optional logfile
  INT64         m_defaultPeriod { 60 * 60 };   // Token valid for 1 hour
  int           m_nextSession   { 0 };         // Next session number to register
  bool          m_development   { false };     // Used in a development environment
  // Locking of the session state
  CRITICAL_SECTION m_lock;
};
