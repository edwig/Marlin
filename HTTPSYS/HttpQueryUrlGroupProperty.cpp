//////////////////////////////////////////////////////////////////////////
//
// USER-SPACE IMPLEMENTTION OF HTTP.SYS
//
// 2018 - 2024 (c) ir. W.E. Huisman
// License: MIT
//
//////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "http_private.h"
#include "UrlGroup.h"
#include "OpaqueHandles.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

ULONG QueryUrlGroupAuthentication(UrlGroup* p_group,PVOID p_info,ULONG p_length,PULONG p_returned);
ULONG QueryUrlGroupTimeoutInfo   (UrlGroup* p_group,PVOID p_info,ULONG p_length,PULONG p_returned);
ULONG QueryUrlGroupStateInfo     (UrlGroup* p_group,PVOID p_info,ULONG p_length,PULONG p_returned);

HTTPAPI_LINKAGE
_Success_(return == NO_ERROR)
ULONG WINAPI HttpQueryUrlGroupProperty(IN HTTP_URL_GROUP_ID     UrlGroupId
                                      ,IN HTTP_SERVER_PROPERTY  Property
                                      ,_Out_writes_bytes_to_opt_(PropertyInformationLength, *ReturnLength) PVOID PropertyInformation
                                      ,IN ULONG                 PropertyInformationLength
                                      ,_Out_opt_ PULONG         ReturnLength)
{
  // Find the URL group
  UrlGroup* group = g_handles.GetUrGroupFromOpaqueHandle(UrlGroupId);
  if (group == nullptr)
  {
    return ERROR_INVALID_PARAMETER;
  }

  switch (Property)
  {
    case HttpServerAuthenticationProperty: return QueryUrlGroupAuthentication(group,PropertyInformation,PropertyInformationLength,ReturnLength);
    case HttpServerTimeoutsProperty:       return QueryUrlGroupTimeoutInfo   (group,PropertyInformation,PropertyInformationLength,ReturnLength);
    case HttpServerStateProperty:          return QueryUrlGroupStateInfo     (group,PropertyInformation,PropertyInformationLength,ReturnLength);
    case HttpServerChannelBindProperty:    // Fall through. To be implemented?
    case HttpServerQosProperty:            // Fall through. To be implemented?
    default: return ERROR_INVALID_PARAMETER;
  }
}

ULONG 
QueryUrlGroupAuthentication(UrlGroup* p_group,PVOID p_info,ULONG p_length,PULONG p_returned)
{
  // Check if the provided buffer is large enough
  if(p_length < sizeof(HTTP_SERVER_AUTHENTICATION_INFO))
  {
    *p_returned = sizeof(HTTP_SERVER_AUTHENTICATION_INFO);
    return ERROR_MORE_DATA;
  }

  // Provide the info
  PHTTP_SERVER_AUTHENTICATION_INFO auth = (PHTTP_SERVER_AUTHENTICATION_INFO)p_info;
  auth->Flags.Present         = 1;      // Properties are present
  auth->AuthSchemes           = p_group->GetAuthenticationScheme();
  auth->ReceiveMutualAuth     = true;   // Presently not used
  auth->ReceiveContextHandle  = false;  // Presently not done
  auth->DisableNTLMCredentialCaching = ! p_group->GetAuthenticationCaching();
  auth->ExFlags               = 0;      // Presently not used

  auth->DigestParams.DomainName       = (PWSTR) p_group->GetAuthenticationDomainWide().c_str();
  auth->DigestParams.DomainNameLength = (USHORT)p_group->GetAuthenticationDomainWide().size();
  auth->DigestParams.Realm            = (PWSTR) p_group->GetAuthenticationRealmWide().c_str();
  auth->DigestParams.RealmLength      = (USHORT)p_group->GetAuthenticationRealmWide().size();
  auth->BasicParams.Realm             = (PWSTR) p_group->GetAuthenticationRealmWide().c_str();
  auth->BasicParams.RealmLength       = (USHORT)p_group->GetAuthenticationRealmWide().size();

  return NO_ERROR;
}

ULONG 
QueryUrlGroupTimeoutInfo(UrlGroup* p_group,PVOID p_info,ULONG p_length,PULONG p_returned)
{
  if(p_length < sizeof(HTTP_TIMEOUT_LIMIT_INFO))
  {
    *p_returned = sizeof(HTTP_TIMEOUT_LIMIT_INFO);
    return ERROR_MORE_DATA;
  }

  // Getting the timeout info
  PHTTP_TIMEOUT_LIMIT_INFO info = (PHTTP_TIMEOUT_LIMIT_INFO)p_info;
  info->EntityBody      = p_group->GetTimeoutEntityBody();
  info->DrainEntityBody = p_group->GetTimeoutDrainEntityBody();
  info->RequestQueue    = p_group->GetTimeoutRequestQueue();
  info->IdleConnection  = p_group->GetTimeoutIdleConnection();
  info->HeaderWait      = p_group->GetTimeoutHeaderWait();
  info->MinSendRate     = p_group->GetTimeoutMinSendRate();

  return NO_ERROR;
}

ULONG 
QueryUrlGroupStateInfo(UrlGroup* p_group,PVOID p_info,ULONG p_length,PULONG p_returned)
{
  if(p_length < sizeof(HTTP_STATE_INFO))
  {
    *p_returned = sizeof(HTTP_STATE_INFO);
    return ERROR_MORE_DATA;
  }

  PHTTP_STATE_INFO info = (PHTTP_STATE_INFO)p_info;
  info->Flags.Present = 1;
  info->State         = p_group->GetEnabledState();

  return NO_ERROR;
}

