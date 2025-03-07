//////////////////////////////////////////////////////////////////////////
//
// USER-SPACE IMPLEMENTTION OF HTTP.SYS
//
// 2018 - 2024 (c) ir. W.E. Huisman
// License: MIT
//
//////////////////////////////////////////////////////////////////////////

#pragma once
#include <afxmt.h>
#include <functional>
#include <wincrypt.h>

// Errors from the Socket listener
enum ErrorType
{
  NoError,
  UnknownError,
  SocketInuse,
  SocketUnusable
};

class URL;
class UrlGroup;
class Request;
class RequestQueue;
class SocketStream;

class Listener
{
public:
  Listener(RequestQueue* p_queue,int p_port,URL* p_url,USHORT p_timeout);
 ~Listener();

public:
  // Initialize the listener
  ErrorType Initialize(int p_tcpListenPort);

  void      StartListener();
  void      StopListener();

  USHORT    GetPort()       { return m_port;   };
  bool      GetSecureMode() { return m_secure; };
  BYTE*     GetThumbprint()               { return m_thumbprint;               };
  XString   GetCertificateStore()         { return m_certificateStore;         };
  bool      GetRequestClientCertificate() { return m_requestClientCertificate; };

  void      SetSendTimeoutSeconds(int p_timeout) { m_sendTimeoutSeconds = p_timeout; };
  void      SetRecvTimeoutSeconds(int p_timeout) { m_recvTimeoutSeconds = p_timeout; };
  int       GetSendTimeoutSeconds() { return m_sendTimeoutSeconds; };
  int       GetRecvTimeoutSeconds() { return m_recvTimeoutSeconds; };

  std::function<SECURITY_STATUS(PCCERT_CONTEXT & pCertContext, LPCTSTR p_certSTore,BYTE* p_thumbprint)> m_selectServerCert;
  std::function<bool(PCCERT_CONTEXT pCertContext, const bool trusted)> m_clientCertAcceptable;

  static unsigned __stdcall ListenerWorker(void* p_param);
  static unsigned __stdcall Worker(void* p_param);

  ULONGLONG m_workerThreadCount;
  CEvent    m_stopEvent;
private:
  void              Listen(void);

  // Primary objects of the server
  RequestQueue*     m_queue;          // We are running for this queue
  int               m_port   { INTERNET_DEFAULT_HTTP_PORT };
  bool              m_secure { false };
  // Listener data
  SOCKET            m_listenSockets[FD_SETSIZE];
  HANDLE            m_hSocketEvents[FD_SETSIZE];
  int               m_numListenSockets;
  CCriticalSection  m_workerThreadLock;
  HANDLE            m_listenerThread { NULL };
  // Timeouts
  int               m_sendTimeoutSeconds { 30 };  // Send timeout in seconds
  int               m_recvTimeoutSeconds { 30 };  // Receive timeout in seconds
  // Secure channel information
  BYTE              m_thumbprint[CERT_THUMBPRINT_SIZE + 1]; // SSL/TLS certificate used for HTTPS
  XString           m_certificateStore;
  bool              m_requestClientCertificate { false };
};


