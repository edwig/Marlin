//////////////////////////////////////////////////////////////////////////
//
// USER-SPACE IMPLEMENTTION OF HTTP.SYS
//
// 2018 - 2024 (c) ir. W.E. Huisman
// License: MIT
//
//////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "http_private.h"

HTTPAPI_LINKAGE
ULONG WINAPI HttpDeleteServiceConfiguration(_Reserved_ IN HANDLE        ServiceHandle
                                           ,IN HTTP_SERVICE_CONFIG_ID   ConfigId
                                           ,_In_reads_bytes_(ConfigInformationLength) IN PVOID /*pConfigInformation*/
                                           ,IN ULONG                    /*ConfigInformationLength*/
                                           ,_Reserved_ IN LPOVERLAPPED  pOverlapped)
{
  // Service handle should be zero: Not used by Microsoft either
  // Overlapped handle is not supported by this driver
  if(ServiceHandle || pOverlapped)
  {
    return ERROR_INVALID_PARAMETER;
  }

  // See what we need to do
  switch(ConfigId)
  {
    case HttpServiceConfigIPListenList:
    case HttpServiceConfigSSLCertInfo:
    case HttpServiceConfigUrlAclInfo:
    case HttpServiceConfigTimeout:
    case HttpServiceConfigCache:        break;
    default: return ERROR_INVALID_PARAMETER;
  }
  return NO_ERROR;
}
