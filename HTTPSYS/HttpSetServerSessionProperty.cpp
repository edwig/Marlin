//////////////////////////////////////////////////////////////////////////
//
// USER-SPACE IMPLEMENTTION OF HTTP.SYS
//
// 2018 (c) ir. W.E. Huisman
// License: MIT
//
//////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "http_private.h"
#include "ServerSession.h"
#include "RequestQueue.h"
#include "OpaqueHandles.h"
#include <ConvertWideString.h>
#include <string>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

using std::wstring;

// Forward declarations of status functions
ULONG SetServerState         (ServerSession* p_session,PVOID p_info,ULONG p_length);
ULONG SetServerTimeouts      (ServerSession* p_session,PVOID p_info,ULONG p_length);
ULONG SetServerAuthentication(ServerSession* p_session,PVOID p_info,ULONG p_length);

HTTPAPI_LINKAGE
ULONG WINAPI
HttpSetServerSessionProperty(IN HTTP_SERVER_SESSION_ID  ServerSessionId
                            ,IN HTTP_SERVER_PROPERTY    Property
                            ,_In_reads_bytes_(PropertyInformationLength) PVOID PropertyInformation
                            ,IN ULONG                   PropertyInformationLength)
{
  // See if we have minimal parameters
  if(PropertyInformation == nullptr)
  {
    return ERROR_INVALID_PARAMETER;
  }
  // Grab our session
  ServerSession* session = g_handles.GetSessionFromOpaqueHandle(ServerSessionId);
  if(session == nullptr)
  {
    return ERROR_INVALID_PARAMETER;
  }

  switch(Property)
  {
    case HttpServerStateProperty:                   return SetServerState   (session,PropertyInformation,PropertyInformationLength);
    case HttpServerTimeoutsProperty:                return SetServerTimeouts(session,PropertyInformation,PropertyInformationLength);
    case HttpServerQosProperty:                     // Fall through
    case HttpServerLoggingProperty:                 return ERROR_INVALID_PARAMETER;
    case HttpServerAuthenticationProperty:          // Fall through
    case HttpServerExtendedAuthenticationProperty:  return SetServerAuthentication(session,PropertyInformation,PropertyInformationLength);
    case HttpServerChannelBindProperty:             // Fall through
    default:                                        return ERROR_INVALID_PARAMETER;
  }

  return NO_ERROR;
}

ULONG 
SetServerState(ServerSession* p_session, PVOID p_info, ULONG p_length)
{
  if(p_length < sizeof(HTTP_STATE_INFO))
  {
    return ERROR_MORE_DATA;
  }
  PHTTP_STATE_INFO info = reinterpret_cast<PHTTP_STATE_INFO>(p_info);
  if(info->Flags.Present == 1)
  {
    p_session->SetEnabledState(info->State);
    return NO_ERROR;
  }
  return ERROR_INVALID_PARAMETER;
}

ULONG 
SetServerTimeouts(ServerSession* p_session, PVOID p_info, ULONG p_length)
{
  if(p_length < sizeof(HTTP_TIMEOUT_LIMIT_INFO))
  {
    return ERROR_MORE_DATA;
  }

  PHTTP_TIMEOUT_LIMIT_INFO info = reinterpret_cast<PHTTP_TIMEOUT_LIMIT_INFO>(p_info);
  if(info->Flags.Present == 1)
  {
    p_session->SetTimeoutEntityBody    (info->EntityBody);
    p_session->SetTimeoutDrainBody     (info->DrainEntityBody);
    p_session->SetTimeoutRequestQueue  (info->RequestQueue);
    p_session->SetTimeoutIdleConnection(info->IdleConnection);
    p_session->SetTimeoutHeaderWait    (info->HeaderWait);
    p_session->SetTimeoutMinSendRate   (info->MinSendRate);
    return NO_ERROR;
  }
  return ERROR_INVALID_PARAMETER;
}

ULONG 
SetServerAuthentication(ServerSession* p_session,PVOID p_info,ULONG p_length)
{
  if(p_length < sizeof(HTTP_SERVER_AUTHENTICATION_INFO))
  {
    return ERROR_MORE_DATA;
  }

  // Get info pointer and see if it is valid
  PHTTP_SERVER_AUTHENTICATION_INFO info = reinterpret_cast<PHTTP_SERVER_AUTHENTICATION_INFO>(p_info);
  if(info->Flags.Present != 1)
  {
    return ERROR_INVALID_PARAMETER;
  }

  ULONG   scheme    = info->AuthSchemes;
  bool    ntlmcache = !(info->DisableNTLMCredentialCaching);
  CString domain;
  CString realm;
  wstring domainWide;
  wstring realmWide;

  // Check if one of these authentication schemes (or multiple) are given
  if((scheme & HTTP_AUTH_ENABLE_BASIC)     == 0 &&
     (scheme & HTTP_AUTH_ENABLE_DIGEST)    == 0 &&
     (scheme & HTTP_AUTH_ENABLE_NTLM)      == 0 &&
     (scheme & HTTP_AUTH_ENABLE_NEGOTIATE) == 0 &&
     (scheme & HTTP_AUTH_ENABLE_KERBEROS)  == 0 &&
     (scheme != 0)) 
  {
    // Other than ZERO (none) or one of the scheme's above
    return ERROR_INVALID_PARAMETER;
  }

  // Getting domain and realm
  if(scheme & HTTP_AUTH_ENABLE_DIGEST)
  {
#ifdef _UNICODE
    domain = info->DigestParams.DomainName;
    realm  = info->DigestParams.Realm;
#else
    bool foundBom;
    TryConvertWideString((const BYTE*)info->DigestParams.DomainName,info->DigestParams.DomainNameLength,_T(""),domain,foundBom);
    TryConvertWideString((const BYTE*)info->DigestParams.Realm,     info->DigestParams.RealmLength,     _T(""),realm, foundBom);
#endif
    domainWide = info->DigestParams.DomainName;
    realmWide  = info->DigestParams.Realm;
  }

  if(scheme & HTTP_AUTH_ENABLE_BASIC)
  {
#ifdef _UNICODE
    realm = info->BasicParams.Realm;
#else
    bool foundBom;
    TryConvertWideString((const BYTE*)info->DigestParams.Realm,info->DigestParams.RealmLength,_T(""),realm,foundBom);
#endif
    realmWide = info->BasicParams.Realm;
  }

  // Let the session and URL groups know
  p_session->SetAuthentication(scheme,domain,realm,domainWide,realmWide,ntlmcache);
  return NO_ERROR;
}

