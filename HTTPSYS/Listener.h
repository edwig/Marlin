//////////////////////////////////////////////////////////////////////////
//
// USER-SPACE IMPLEMENTTION OF HTTP.SYS
//
// 2018 (c) ir. W.E. Huisman
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
  PTCHAR    GetThumbprint()               { return m_thumbprint;               };
  CString   GetCertificateStore()         { return m_certificateStore;         };
  bool      GetRequestClientCertificate() { return m_requestClientCertificate; };

  void      SetSendTimeoutSeconds(int p_timeout) { m_sendTimeoutSeconds = p_timeout; };
  void      SetRecvTimeoutSeconds(int p_timeout) { m_recvTimeoutSeconds = p_timeout; };
  int       GetSendTimeoutSeconds() { return m_sendTimeoutSeconds; };
  int       GetRecvTimeoutSeconds() { return m_recvTimeoutSeconds; };

  std::function<SECURITY_STATUS(PCCERT_CONTEXT & pCertContext, LPCTSTR p_certSTore,PTCHAR p_thumbprint)> m_selectServerCert;
  std::function<bool(PCCERT_CONTEXT pCertContext, const bool trusted)>                  m_clientCertAcceptable;

  int       m_workerThreadCount;
  CEvent    m_stopEvent;
private:
  static UINT __cdecl Worker(void *);
  static UINT __cdecl ListenerWorker(LPVOID);
  void                Listen(void);

  // Primary objects of the server
  RequestQueue*     m_queue;          // We are running for this queue
  int               m_port   { INTERNET_DEFAULT_HTTP_PORT };
  bool              m_secure { false };
  // Listener data
  SOCKET            m_listenSockets[FD_SETSIZE];
  HANDLE            m_hSocketEvents[FD_SETSIZE];
  int               m_numListenSockets;
  CCriticalSection  m_workerThreadLock;
  CWinThread*       m_listenerThread;
  // Timeouts
  int               m_sendTimeoutSeconds { 30 };  // Send timeout in seconds
  int               m_recvTimeoutSeconds { 30 };  // Receive timeout in seconds
  // Secure channel information
  TCHAR             m_thumbprint[CERT_THUMBPRINT_SIZE + 1]; // SSL/TLS certificate used for HTTPS
  CString           m_certificateStore;
  bool              m_requestClientCertificate { false };
};


