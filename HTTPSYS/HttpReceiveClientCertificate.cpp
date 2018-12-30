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
#include "SecureServerSocket.h"
#include "RequestQueue.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

ULONG WINAPI
HttpReceiveClientCertificate(IN HANDLE              RequestQueueHandle
                            ,IN HTTP_CONNECTION_ID  ConnectionId
                            ,IN ULONG               Flags
                            ,_Out_writes_bytes_to_(SslClientCertInfoSize, *BytesReceived) PHTTP_SSL_CLIENT_CERT_INFO SslClientCertInfo
                            ,IN ULONG               SslClientCertInfoSize
                            ,_Out_opt_ PULONG       BytesReceived
                            ,IN LPOVERLAPPED        Overlapped OPTIONAL)
{
  // Overlapped I/O not implemented yet
  if(Overlapped)
  {
    return ERROR_IMPLEMENTATION_LIMIT;
  }

  // Need pointers and bytes
  if(SslClientCertInfo == nullptr || SslClientCertInfoSize == 0 || BytesReceived == nullptr)
  {
    return ERROR_INVALID_PARAMETER;
  }

  // Check the flags parameter. Must be ZERO (client certificate) or the SSL token
  if(Flags != 0 && Flags != HTTP_RECEIVE_SECURE_CHANNEL_TOKEN)
  {
    return ERROR_INVALID_PARAMETER;
  }

  // Get our queue
  // Finding our request queue
  RequestQueue* queue = GetRequestQueueFromHandle(RequestQueueHandle);
  if(queue == nullptr)
  {
    return ERROR_INVALID_PARAMETER;
  }

  // Get our server socket
  SecureServerSocket* socket = reinterpret_cast<SecureServerSocket*>(ConnectionId);
  if(socket == nullptr)
  {
    return ERROR_INVALID_PARAMETER;
  }

  // Decide on which certificate to return
  CertificateInfo* info = Flags ? socket->GetServerCertificate() : socket->GetClientCertificate();
  if(info == nullptr)
  {
    return ERROR_INVALID_PARAMETER;
  }

  // Getting our blob or ERROR_MORE_DATA
  return info->GetSSLCertificateInfoBlob(SslClientCertInfo,SslClientCertInfoSize,BytesReceived);
}
