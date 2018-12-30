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
#include "UrlGroup.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static ULONG SetUrlGroupAuthentication(UrlGroup* p_group,PHTTP_SERVER_AUTHENTICATION_INFO p_info);
static ULONG SetUrlGroupServerBinding (UrlGroup* p_group,PHTTP_BINDING_INFO       p_info);
static ULONG SetUrlGroupEnabledState  (UrlGroup* p_group,PHTTP_STATE_INFO         p_info);
static ULONG SetUrlGroupTimeouts      (UrlGroup* p_group,PHTTP_TIMEOUT_LIMIT_INFO p_info);

ULONG WINAPI
HttpSetUrlGroupProperty(IN HTTP_URL_GROUP_ID    UrlGroupId
                       ,IN HTTP_SERVER_PROPERTY Property
                       ,_In_reads_bytes_(PropertyInformationLength) PVOID PropertyInformation
                       ,IN ULONG                PropertyInformationLength)
{
  // Check that these are provided
  if(PropertyInformation == 0 || PropertyInformationLength == 0)
  {
    return ERROR_INVALID_PARAMETER;
  }

  // Find the URL group
  UrlGroup* group = GetUrlGroupFromHandle(UrlGroupId);
  if(group == nullptr)
  {
    return ERROR_INVALID_PARAMETER;
  }

  // Do the properties
  if(Property == HttpServerAuthenticationProperty ||
     Property == HttpServerExtendedAuthenticationProperty)
  {
    PHTTP_SERVER_AUTHENTICATION_INFO info = reinterpret_cast<PHTTP_SERVER_AUTHENTICATION_INFO>(PropertyInformation);
    if(PropertyInformationLength == sizeof(HTTP_SERVER_AUTHENTICATION_INFO))
    {
      return SetUrlGroupAuthentication(group,info);
    }
  }
  else if(Property == HttpServerBindingProperty)
  {
    PHTTP_BINDING_INFO info = (PHTTP_BINDING_INFO)PropertyInformation;
    if(PropertyInformationLength == sizeof(HTTP_BINDING_INFO))
    {
      return SetUrlGroupServerBinding(group, info);
    }
  }
  else if(Property == HttpServerStateProperty)
  {
    PHTTP_STATE_INFO info = (PHTTP_STATE_INFO)PropertyInformation;
    if (PropertyInformationLength == sizeof(HTTP_STATE_INFO))
    {
      return SetUrlGroupEnabledState(group,info);
    }
  }
  else if(Property == HttpServerTimeoutsProperty)
  {
    PHTTP_TIMEOUT_LIMIT_INFO info = (PHTTP_TIMEOUT_LIMIT_INFO)PropertyInformation;
    if(PropertyInformationLength == sizeof(HTTP_TIMEOUT_LIMIT_INFO))
    {
      return SetUrlGroupTimeouts(group,info);
    }
  }
  return ERROR_INVALID_PARAMETER;
}

ULONG 
SetUrlGroupAuthentication(UrlGroup* p_group,PHTTP_SERVER_AUTHENTICATION_INFO p_info)
{
  // Cannot be called: URL's already bound to this group.
  if(p_group->GetNumberOfUrls())
  {
    return ERROR_ALREADY_EXISTS;
  }

  if(p_info->Flags.Present == 1)
  {
    USES_CONVERSION;

    ULONG scheme = p_info->AuthSchemes;
    bool ntlmcache = !(p_info->DisableNTLMCredentialCaching);
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
      domain = W2A(p_info->DigestParams.DomainName);
      domainWide = p_info->DigestParams.DomainName;
      realm  = W2A(p_info->DigestParams.Realm);
      realmWide  = p_info->DigestParams.Realm;
    }
    if(scheme & HTTP_AUTH_ENABLE_BASIC)
    {
      realm = W2A(p_info->BasicParams.Realm);
      realmWide = p_info->BasicParams.Realm;
    }

    // Let the group know
    p_group->SetAuthentication(scheme,domain,realm,ntlmcache);
    p_group->SetAuthenticationWide(domainWide,realmWide);
  }
  else
  {
    // Reset the authentication of the URL group
    wstring empty;
    p_group->SetAuthentication(0,"","",false);
    p_group->SetAuthenticationWide(empty, empty);
  }
  return NO_ERROR;
}

ULONG 
SetUrlGroupServerBinding(UrlGroup* p_group,PHTTP_BINDING_INFO p_info)
{
  // Find out what to do by the Flags.Present parameter
  if(p_info->Flags.Present == 1)
  {
    p_group->SetRequestQueue(p_info->RequestQueueHandle);
  }
  else
  {
    p_group->SetRequestQueue(nullptr);
  }
  return NO_ERROR;
}

ULONG 
SetUrlGroupEnabledState(UrlGroup* p_group, PHTTP_STATE_INFO p_info)
{
  if(p_info->Flags.Present == 1)
  {
    p_group->SetEnabledState(p_info->State);
  }
  else
  {
    p_group->SetEnabledState(HttpEnabledStateActive);
  }
  return NO_ERROR;
}

ULONG 
SetUrlGroupTimeouts(UrlGroup* p_group, PHTTP_TIMEOUT_LIMIT_INFO p_info)
{
  if(p_info->Flags.Present == 1)
  {
    // Copy new timings
    p_group->SetTimeoutEntityBody    (p_info->EntityBody);
    p_group->SetTimeoutDrainBody     (p_info->DrainEntityBody);
    p_group->SetTimeoutRequestQueue  (p_info->RequestQueue);
    p_group->SetTimeoutIdleConnection(p_info->IdleConnection);
    p_group->SetTimeoutHeaderWait    (p_info->HeaderWait);
    p_group->SetTimeoutMinSendRate   (p_info->MinSendRate);
  }
  else
  {
    // Reset all timings to defaults;
    p_group->SetTimeoutEntityBody    (URL_TIMEOUT_ENTITY_BODY);
    p_group->SetTimeoutDrainBody     (URL_TIMEOUT_DRAIN_BODY);
    p_group->SetTimeoutRequestQueue  (URL_TIMEOUT_REQUEST_QUEUE);
    p_group->SetTimeoutIdleConnection(URL_TIMEOUT_IDLE_CONNECTION);
    p_group->SetTimeoutHeaderWait    (URL_TIMEOUT_HEADER_WAIT);
    p_group->SetTimeoutMinSendRate   (URL_DEFAULT_MIN_SEND_RATE);
  }
  return NO_ERROR;
}
