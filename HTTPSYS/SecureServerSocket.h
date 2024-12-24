/////////////////////////////////////////////////////////////////////////////
// 
// SecureServerSocket
//
// Original idea:
// David Maw: https://www.codeproject.com/Articles/1000189/A-Working-TCP-Client-and-Server-With-SSL
// License:   https://www.codeproject.com/info/cpol10.aspx
//
#pragma once
#include <functional>
#include <wincrypt.h>
#pragma comment(lib, "crypt32.lib")
#include <wintrust.h>
#define SECURITY_WIN32
#include <security.h>
#pragma comment(lib, "secur32.lib")
#include "SocketStream.h"
#include "PlainSocket.h"
#include "http_private.h"
#include "CertificateInfo.h"

class Listener;

class SecureServerSocket : public PlainSocket
{
public:
	SecureServerSocket(SOCKET p_socket,HANDLE p_stopEvent);
  virtual ~SecureServerSocket(void);

  // Set up state for this connection
  HRESULT InitializeSSL(const void* p_buffer = nullptr,const int p_length = 0) override;

  // SocketStream interface
  int     RecvMsg    (LPVOID  p_buffer,const ULONG p_length) override;
  int     SendMsg    (LPCVOID p_buffer,const ULONG p_length) override;
	int     RecvPartial(LPVOID  p_buffer,const ULONG p_length) override;
	int     SendPartial(LPCVOID p_buffer,const ULONG p_length) override;
  int     RecvPartialOverlapped(LPVOID p_buffer,const ULONG p_length,LPOVERLAPPED p_overlapped) override;
  int     SendPartialOverlapped(LPVOID p_buffer,const ULONG p_length,LPOVERLAPPED p_overlapped) override;
	int     Disconnect(int p_how = SD_BOTH) override;
  bool    Close(void) override;

	static PSecurityFunctionTable SSPI(void);

  void    SetThumbprint(PTCHAR p_thumprint);
  void    SetCertificateStore(CString p_store);
  void    SetRequestClientCertificate(bool p_request);

  std::function<SECURITY_STATUS(PCCERT_CONTEXT & pCertContext, LPCTSTR p_certStore,PTCHAR p_thumbprint)> m_selectServerCert;
  std::function<bool(PCCERT_CONTEXT pCertContext, const bool trusted)> m_clientCertAcceptable;

  CertificateInfo*  GetClientCertificate() { return m_clientCertificate; };
  CertificateInfo*  GetServerCertificate() { return m_serverCertificate; };

  // Public: but only meant for the overlapping I/O routines !!
  void              RecvPartialOverlappedContinuation(LPOVERLAPPED p_overlapped);
private:
  static HRESULT    InitializeClass(void);
         HRESULT    LogSSLInitError(HRESULT hr);
         bool       SSPINegotiateLoop(void);
  SECURITY_STATUS   CreateCredentialsFromCertificate(PCredHandle phCreds, PCCERT_CONTEXT pCertContext);

  static PSecurityFunctionTable g_pSSPI;
	static CredHandle g_ServerCreds;
  static CString    g_ServerName;

  // Private data
	static const int  m_maxMsgSize   = 16000; // Arbitrary but less than 16384 limit, including MaxExtraSize
	static const int  m_maxExtraSize = 50;    // Also arbitrary, current header is 5 bytes, trailer 36
	CHAR              m_writeBuffer[m_maxMsgSize + m_maxExtraSize];       // Enough for a whole encrypted message
	CHAR              m_readBuffer[(m_maxMsgSize + m_maxExtraSize) * 2];  // Enough for two whole messages so we don't need to move data around in buffers
	DWORD             m_readBufferBytes { 0       };
	void*             m_readPointer     { nullptr };
	CtxtHandle        m_context;
	SecPkgContext_StreamSizes m_sizes;
  TCHAR             m_thumbprint[CERT_THUMBPRINT_SIZE + 1];
  CString           m_certificateStore;
  bool              m_requestClientCertificate { false };
  // SSL Certificate
  CertificateInfo*  m_serverCertificate { nullptr };
  CertificateInfo*  m_clientCertificate { nullptr };
  // Overlapping I/O
  LPOVERLAPPED      m_readOverlapped    { nullptr };
  OVERLAPPED        m_overReceiving     { 0 };
  LPVOID            m_originalBuffer    { nullptr };
  ULONG             m_originalLength    { 0 };
  BYTE*             m_overflowBuffer    { nullptr };
  ULONG             m_overflowSize      { 0 };
};
