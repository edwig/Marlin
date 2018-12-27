//////////////////////////////////////////////////////////////////////////
//
// USER-SPACE IMPLEMENTTION OF HTTP.SYS
//
// 2018 (c) ir. W.E. Huisman
// License: MIT
//
//////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include <afxwin.h>
#include "http_private.h"
#include "URL.h"
#include "Listener.h"
#include "Request.h"
#include "RequestQueue.h"
#include "UrlGroup.h"
#include "SSLUtilities.h"
#include "Logging.h"

#include <process.h>
#include <strsafe.h>
#include <atlconv.h>
#include <WS2tcpip.h>
#include "Logging.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// Listener object, listens for connections on one thread, and initiates a worker
// thread each time a client connects. Listens on both IPv4 and IPv6 addresses
Listener::Listener(RequestQueue* p_queue,int p_port,URL* p_url,USHORT p_timeout)
         :m_queue(p_queue)
         ,m_port(p_port)
         ,m_sendTimeoutSeconds(p_timeout)
         ,m_recvTimeoutSeconds(p_timeout)
	       ,m_stopEvent(FALSE,TRUE)
         ,m_workerThreadCount(0)
         ,m_listenerThread(NULL)
         ,m_numListenSockets(0)
{
	for (int i = 0; i<FD_SETSIZE; i++)
	{
		m_listenSockets[i] = INVALID_SOCKET;
		m_hSocketEvents[i] = nullptr;
	}
  m_thumbprint[0] = 0;
  m_selectServerCert     = SelectServerCert;
  m_clientCertAcceptable = ClientCertAcceptable;

  // Copy the secure channel settings from the URL provided
  m_secure = p_url->m_secure;
  if(m_secure)
  {
    m_certificateStore         = p_url->m_certStoreName;
    m_requestClientCertificate = p_url->m_requestClientCert;
    memcpy_s(m_thumbprint,CERT_THUMBPRINT_SIZE,p_url->m_thumbprint,CERT_THUMBPRINT_SIZE);
  }
}

Listener::~Listener(void)
{
  StopListener();
	for(int i = 0;i < FD_SETSIZE;++i)
	{
    if(m_listenSockets[i] != INVALID_SOCKET)
    {
      closesocket(m_listenSockets[i]);
      m_listenSockets[i] = INVALID_SOCKET;
    }
    if(m_hSocketEvents[i])
    {
      CloseHandle(m_hSocketEvents[i]);
      m_hSocketEvents[i] = nullptr;
    }
	}
}

// This is the individual worker process, all it does is start, change its name to something useful,
// then call the Lambda function passed in via the BeginListening method
UINT __cdecl Listener::Worker(void* p_argument)
{
	Request*  request = reinterpret_cast<Request*>(p_argument);
  Listener* listener = request->GetListener();

	SetThreadName("Request worker");

  // Doing our work
  InterlockedIncrement(&listener->m_workerThreadCount);
  request->ReceiveRequest();
  InterlockedDecrement(&listener->m_workerThreadCount);

  return 0;
}

// Worker process for connection listening
UINT __cdecl Listener::ListenerWorker(LPVOID p_param)
{
  // See _beginthread call for parameter definition
	Listener* listener = reinterpret_cast<Listener *>(p_param); 

	SetThreadName("HTTPListener");

  listener->Listen();
	return 0;
}

// Initialize the listener, set up the socket to listen on, or return an error
ErrorType 
Listener::Initialize(int p_tcpListenPort)
{
	CString portText;
	portText.Format("%i",p_tcpListenPort);

	// Get list of addresses to listen on
	ADDRINFOT Hints, *AddrInfo, *AI;
	memset(&Hints, 0, sizeof (Hints));
	Hints.ai_family   = PF_UNSPEC;    // Meaning IP4 or IP6
	Hints.ai_socktype = SOCK_STREAM;  // Streaming sockets only
	Hints.ai_flags    = AI_NUMERICHOST | AI_PASSIVE;
	if (GetAddrInfo(nullptr,portText,&Hints,&AddrInfo) != 0)
	{
    LogError("Error getaddressinfo: %d",GetLastError());
		return UnknownError;
	}

	// Create one or more passive sockets to listen on
	int i;
	for (i = 0, AI = AddrInfo; AI != nullptr; AI = AI->ai_next)
	{
		// Did we receive more addresses than we can handle?  Highly unlikely, but check anyway.
		if (i == FD_SETSIZE) break;

		// Only support PF_INET and PF_INET6.  If something else, skip to next address.
		if ((AI->ai_family != AF_INET) && (AI->ai_family != AF_INET6)) continue;

		m_hSocketEvents[i] = CreateEvent(nullptr,   // no security attributes
			                               true,		  // manual reset event
			                               false,		  // not signaled
			                               nullptr);	// no name

    // Check that we got the event
    if(!(m_hSocketEvents[i]))
    {
      return UnknownError;
    }

    // Create listen socket
		m_listenSockets[i] = WSASocketW(AI->ai_family,SOCK_STREAM,0,NULL,0,WSA_FLAG_OVERLAPPED);
    if(m_listenSockets[i] == INVALID_SOCKET)
    {
      return SocketUnusable;
    }

    // Bind listen socket to this address
		int rc = bind(m_listenSockets[i], AI->ai_addr, (int)AI->ai_addrlen);
		if(rc)
		{
      if(WSAGetLastError() == WSAEADDRINUSE)
      {
        return SocketInuse;
      }
      else
      {
        return SocketUnusable;
      }
		}

    // Listen a-sync on this socket
    if(listen(m_listenSockets[i],10))
    {
      return SocketUnusable;
    }
    // Accept the FD_ACCEPT-ing event of the socket
    if(WSAEventSelect(m_listenSockets[i],m_hSocketEvents[i],FD_ACCEPT))
    {
      return SocketUnusable;
    }
		i++;
	}
  // Register number of simultaneous sockets.
	m_numListenSockets = i;

	return NoError;
}

// Start listening for connections, if a timeout is specified keep listening until then
void Listener::StartListener()
{
	m_listenerThread = AfxBeginThread(ListenerWorker,this);
}

// Stop listening, tells the listener thread it can stop, then waits for it to terminate
void Listener::StopListener(void)
{
	m_stopEvent.SetEvent();
	if (m_listenerThread)
	{
		WaitForSingleObject(m_listenerThread->m_hThread, INFINITE); // Will auto delete
	}
	m_listenerThread = nullptr;
}

// Listen for connections until the "stop" event is caused, this is invoked on
// its own thread
void Listener::Listen(void)
{
	HANDLE events[FD_SETSIZE+1];
	SOCKET readSocket = NULL;
	DWORD  wait       = 0;

  m_workerThreadCount = 0;

	DebugMsg("Start Listener::Listen method");

	events[0] = m_stopEvent;

 	// Add the events for each socket type (two at most, one for IPv4, one for IPv6)
	for (int i=0; i<m_numListenSockets; i++)
	{
		events[i+1] = m_hSocketEvents[i];
	}

	// Loop until there is a problem or the shutdown event is caused
	while (true)
	{
		wait = WaitForMultipleObjects(m_numListenSockets+1,events,false,INFINITE);
		if(wait == WAIT_OBJECT_0)
		{
			DebugMsg("Listener::Listen received a stop event");
			break;
		}
		int iMyIndex = wait-1;

		WSAResetEvent(m_hSocketEvents[iMyIndex]);
		readSocket = accept(m_listenSockets[iMyIndex], 0, 0);
		if (readSocket == INVALID_SOCKET)
		{
			LogError("Accept: readSocket == INVALID_SOCKET");
			break;
		}

		// A request to open a socket has been received, begin a thread to handle that connection.
    // Secure connections can take long to establish because of the handshaking, 
    // so we offload it to an extra thread to do the job.
		DebugMsg("Starting request worker");
    Request* request = new Request(m_queue,this,readSocket,events[0]);
    AfxBeginThread(Worker,request);
  }
	// There has been a problem, wait for all the worker threads to terminate
	Sleep(100);
	m_workerThreadLock.Lock();
	while (m_workerThreadCount)
	{
		m_workerThreadLock.Unlock();
		Sleep(100);
		DebugMsg("Waiting for all workers to terminate: worker thread count = %i", m_workerThreadCount);
		m_workerThreadLock.Lock();
	};

  if((readSocket != NULL) && (readSocket != INVALID_SOCKET))
  {
    closesocket(readSocket);
  }
	DebugMsg("End Listen method");
}
