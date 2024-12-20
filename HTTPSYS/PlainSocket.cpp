/////////////////////////////////////////////////////////////////////////////
// 
// PlainSocket
//
// Original idea:
// David Maw: https://www.codeproject.com/Articles/1000189/A-Working-TCP-Client-and-Server-With-SSL
// License:   https://www.codeproject.com/info/cpol10.aspx
//
#include "stdafx.h"
#include <process.h>
#include <stdlib.h>
#include <WS2tcpip.h>
#include <MSTcpIP.h>
#include "PlainSocket.h"
#include "Logging.h"
#include <LogAnalysis.h>
#include <mswsock.h>
#include <intsafe.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// Constructor for an active connection, socket created later
PlainSocket::PlainSocket(HANDLE p_stopEvent)
            :m_stopEvent(p_stopEvent)
{
  Initialize();
}

// Constructor for an already connected socket
PlainSocket::PlainSocket(SOCKET p_socket,HANDLE p_stopEvent)
            :m_actualSocket(p_socket)
            ,m_stopEvent(p_stopEvent)
{
  Initialize();
}

// Destructor
PlainSocket::~PlainSocket()
{
  Close();

  // Close all pending events
  if(m_read_event)
  {
    WSACloseEvent(m_read_event);
    m_read_event = nullptr;
  }
  if(m_write_event)
  {
    WSACloseEvent(m_write_event);
    m_write_event = nullptr;
  }
  if(m_extraStop)
  {
    CloseHandle(m_extraStop);
    m_extraStop = NULL;
  }
}

//////////////////////////////////////////////////////////////////////////
//
// PUBLIC MEMBERS
//
//////////////////////////////////////////////////////////////////////////

void
PlainSocket::Initialize()
{
  // Re-Set to sane values
	m_lastError     = 0;
	m_recvInitiated = false;

  // Getting the events and check for invalid values
  if(!m_read_event)
  {
    m_read_event = WSACreateEvent();  
  }
	WSAResetEvent(m_read_event);
  if(!m_write_event)
  {
    m_write_event = WSACreateEvent(); 
  }
	WSAResetEvent(m_write_event);
	
  if(m_read_event == WSA_INVALID_EVENT || m_write_event == WSA_INVALID_EVENT)
  {
    throw _T("WSACreateEvent failed");
  }

  // Set socket options for TCP/IP
  int rc = true;
	setsockopt(m_actualSocket,IPPROTO_TCP,TCP_NODELAY,(char *)&rc,sizeof(int));

  m_initDone = true;
}

// Find connection type (AF_INET (IPv4) or AF_INET6 (IPv6))
int
PlainSocket::FindConnectType(LPCTSTR p_host,LPCTSTR p_portname)
{
#ifdef _UNICODE
  ADDRINFOW  hints;
  ADDRINFOW* result;
#else
  ADDRINFOA  hints;
  ADDRINFOA* result;
#endif
  memset(&hints,0,sizeof(ADDRINFO));
  int type = AF_INET;

  // Request streaming type socket in TCP/IP protocol
  hints.ai_family   = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;

  DWORD retval = GetAddrInfo(p_host,p_portname,&hints,&result);
  if(retval == 0)
  {
    if((result->ai_family == AF_INET6) ||
       (result->ai_family == AF_INET))
    {
      type = result->ai_family;
    }
    FreeAddrInfo(result);
  }
  else
  {
    // MESS_INETTYPE 
    LogError(_T("Cannot determine if internet is of type IP4 or IP6"));
  }
  // Assume it's IP4 as a default
  return type;
}

// Connect as a client to a server actively
bool 
PlainSocket::Connect(LPCTSTR p_hostName, USHORT p_portNumber)
{
	SOCKADDR_STORAGE localAddr  = {0};
	SOCKADDR_STORAGE remoteAddr = {0};
	DWORD sizeLocalAddr  = sizeof(localAddr);
	DWORD sizeRemoteAddr = sizeof(remoteAddr);
	TCHAR portName[10]   = {0};
  BOOL    bSuccess     = FALSE;
	timeval timeout      = {0};
  int     result       = 0;

  if(m_initDone == false)
  {
    Initialize();
  }

  // Convert port number and find IPv4 or IPv6
  _itot_s(p_portNumber, portName, _countof(portName), 10);
  int type = FindConnectType(p_hostName,portName);

  // Create the actual physical socket
	m_actualSocket = socket(type, SOCK_STREAM, 0);
	if (m_actualSocket == INVALID_SOCKET)
  {
		LogError(_T("Socket failed with error: %d\n"), WSAGetLastError());
		return false;
	}

  // Find timeout for the connection
  timeout.tv_sec = GetConnTimeoutSeconds();
	CTime Now = CTime::GetCurrentTime();

  USES_CONVERSION;
  std::wstring hostname = T2CW(p_hostName);
  std::wstring portname = T2CW(portName);

	// Note that WSAConnectByName requires Vista or Server 2008
  // Note that WSAConnectByNameA is deprecated, so we convert to the Unicode counterpart.
	bSuccess = WSAConnectByNameW(m_actualSocket
                              ,(LPWSTR)hostname.c_str()
                              ,(LPWSTR)portname.c_str()
                              ,&sizeLocalAddr
                              ,(SOCKADDR*)&localAddr
                              ,&sizeRemoteAddr
                              ,(SOCKADDR*)&remoteAddr
                              ,&timeout
                              ,nullptr);

	CTimeSpan HowLong = CTime::GetCurrentTime() - Now;
	if (!bSuccess)
  {
		m_lastError = WSAGetLastError();
		LogError(_T("**** WSAConnectByName Error %d connecting to \"%s\" (%s)"),
				     m_lastError,
				     p_hostName, 
				     portName);
		closesocket(m_actualSocket);
		return false;       
	}

  DebugMsg(_T("Connection made in %ld second(s)"),HowLong.GetTotalSeconds());

  // Activate previously set options
	result = setsockopt(m_actualSocket, SOL_SOCKET,SO_UPDATE_CONNECT_CONTEXT, NULL, 0);
	if (result == SOCKET_ERROR)
  {
		m_lastError = WSAGetLastError();
		LogError(_T("setsockopt for SO_UPDATE_CONNECT_CONTEXT failed with error: %d"), m_lastError);
		closesocket(m_actualSocket);
		return false;       
	}

  result = true;
	// At this point we have a connection, so set up keep-alive so we can detect if the host disconnects
	// This code is commented out because it does not seen to be helpful
  if(m_useKeepalive)
  {
    result = ActivateKeepalive();
  }

  // We are now a client side socket
  m_active = true;

  // Remember what we connected on
  m_hostName   = p_hostName;
  m_portNumber = p_portNumber;

	return result;
}

bool PlainSocket::ActivateKeepalive()
{
  BOOL so_keepalive = m_useKeepalive;
  int iResult = setsockopt(m_actualSocket, SOL_SOCKET, SO_KEEPALIVE, (const char *)&so_keepalive, sizeof(so_keepalive));
	if (iResult == SOCKET_ERROR)
  {
		m_lastError = WSAGetLastError();
		LogError(_T("Setsockopt for SO_KEEPALIVE failed with error: %d\n"),m_lastError);
		closesocket(m_actualSocket);
    m_actualSocket = NULL;
		return false;       
	}

  // Now set keep alive timings, if activated
  if(m_useKeepalive)
  {
    DWORD dwBytes = 0;
    tcp_keepalive setting = { 0 };

    setting.onoff             = 1;
    setting.keepalivetime     = m_keepaliveTime     * CLOCKS_PER_SEC;     // Keep Alive in x milli seconds
    setting.keepaliveinterval = m_keepaliveInterval * CLOCKS_PER_SEC;     // Resend if No-Reply
    if (WSAIoctl(m_actualSocket
                ,SIO_KEEPALIVE_VALS
                ,&setting
    	          ,sizeof(setting)
                ,nullptr          // Result buffer sReturned
                ,0                // Size of result buffer
                ,&dwBytes         // Total bytes
                ,nullptr          // Pointer to OVERLAPPED
                ,nullptr) != 0)   // Completion routine
    {
      m_lastError = WSAGetLastError() ;
      LogError(_T("WSAIoctl to set keep-alive failed with error: %d\n"), m_lastError);
      closesocket(m_actualSocket);
      m_actualSocket = NULL;
      return false;       
    }
  }
  return true;
}

// Set up SSL/TLS state for this connection:
// NEVER USED ON PLAIN SOCKETS! Only on derived classes!!
HRESULT 
PlainSocket::InitializeSSL(const void* /*p_buffer*/ /*= nullptr*/,const int /*p_length*/ /*= 0*/)
{
  return SOCKET_ERROR;
}

void PlainSocket::SetConnTimeoutSeconds(int p_newTimeoutSeconds)
{
  if(p_newTimeoutSeconds == INFINITE)
  {
    p_newTimeoutSeconds = MAXINT;
  }
  if(p_newTimeoutSeconds > 0)
  {
    m_connTimeoutSeconds = p_newTimeoutSeconds;
  }
}

void PlainSocket::SetRecvTimeoutSeconds(int NewRecvTimeoutSeconds)
{
  if(NewRecvTimeoutSeconds == INFINITE)
  {
    NewRecvTimeoutSeconds = MAXINT;
  }
	if (NewRecvTimeoutSeconds>0)
	{
		m_recvTimeoutSeconds = NewRecvTimeoutSeconds;
		m_recvEndTime = CTime::GetCurrentTime() + CTimeSpan(0,0,0,m_recvTimeoutSeconds);
	}
}

void 
PlainSocket::SetSendTimeoutSeconds(int NewSendTimeoutSeconds)
{
  if(NewSendTimeoutSeconds == INFINITE)
  {
    NewSendTimeoutSeconds = MAXINT;
  }
	if (NewSendTimeoutSeconds>0)
	{
		m_sendTimeoutSeconds = NewSendTimeoutSeconds;
		m_sendEndTime = CTime::GetCurrentTime() + CTimeSpan(0,0,0,m_sendTimeoutSeconds);
	}
}

bool 
PlainSocket::SetUseKeepAlive(bool p_keepalive)
{
  m_useKeepalive = p_keepalive;
  if(m_initDone)
  {
    return ActivateKeepalive();
  }
  return true;
}

bool  
PlainSocket::SetKeepaliveTime(int p_time)
{
  if(p_time > MINIMUM_KEEPALIVE)
  {
    m_keepaliveTime = p_time;
    if(m_initDone)
    {
      return ActivateKeepalive();
    }
    return true;
  }
  return false;
}

bool  
PlainSocket::SetKeepaliveInterval(int p_interval)
{
  if(p_interval > MINIMUM_KEEPALIVE)
  {
    m_keepaliveInterval = p_interval;
    if(m_initDone)
    {
      return ActivateKeepalive();
    }
    return true;
  }
  return false;
}

DWORD PlainSocket::GetLastError()
{
	return m_lastError; 
}

// Shutdown one of the sides of the socket, or both
int 
PlainSocket::Disconnect(int p_how)
{
  if(m_actualSocket)
  {
    if(p_how == SD_BOTH)
    {
      Close();
      return NO_ERROR;
    }
    if(p_how == SD_RECEIVE && m_readOverlapped->OffsetHigh)
    {
      CancelThreadpoolIo(m_threadPoolIO);
    }
    if(p_how == SD_SEND && m_sendOverlapped->OffsetHigh)
    {
      CancelThreadpoolIo(m_threadPoolIO);
    }
    // Only shutdown one side
    return ::shutdown(m_actualSocket,p_how);
  }
  return NO_ERROR;
}

bool 
PlainSocket::Close(void)
{
  // In case of an outstanding threadpool IO, cancel that
  if(m_threadPoolIO)
  {
    CloseThreadpoolIo (m_threadPoolIO);
    m_threadPoolIO = nullptr;
  }

  // Possibly notify a waiting application for HttpWaitForDisconnect
  if(m_extraStop)
  {
    SetEvent(m_extraStop);
  }

  // If initialized: close the underlying system socket
  if(m_initDone)
  {
    // Shutdown both sides of the socket
    if(shutdown(m_actualSocket,SD_BOTH) != 0)
    {
		  m_lastError = ::WSAGetLastError();
	  }
    // And then close it directly
    if(closesocket(m_actualSocket))
    {
      // Log the error
      m_lastError = WSAGetLastError();
      return false;
    }
    m_actualSocket = NULL;
    m_initDone = false;
  }
  return true;
}

// Waiting for this socket to disconnect
ULONG   
PlainSocket::RegisterForDisconnect()
{
  if(m_extraStop)
  {
    return ERROR_ALREADY_EXISTS;
  }
  m_extraStop = ::CreateEvent(nullptr,FALSE,FALSE,nullptr);
  DWORD result = WaitForSingleObject(m_extraStop,INFINITE);
  
  // Ready with stop event
  CloseHandle(m_extraStop);
  m_extraStop = NULL;

  // Result
  return result == WAIT_OBJECT_0 ? NO_ERROR : ERROR_CONNECTION_ABORTED;
}

//////////////////////////////////////////////////////////////////////////
//
// RECEIVING AND SENDING DATA
//
//////////////////////////////////////////////////////////////////////////

// Receives up to Len bytes of data and returns the amount received - or SOCKET_ERROR if it times out
int 
PlainSocket::RecvPartial(LPVOID p_buffer, const ULONG p_length)
{
	WSABUF    buffer;
	WSAEVENT  hEvents[2] = { m_stopEvent,m_read_event };
  DWORD			bytes_read = 0;
  DWORD     msg_flags  = 0;
	int       received   = 0;

	if (m_recvInitiated)
	{
		// Special case, the previous read timed out, so we are trying again, maybe it completed in the meantime
		received      = SOCKET_ERROR;
		m_lastError   = WSA_IO_PENDING;
		m_recvEndTime = 0;
	}
	else
	{
		// Normal case, the last read completed normally, now we're reading again

		// Setup the buffers array
		buffer.buf = static_cast<char*>(p_buffer);
		buffer.len = p_length;
	
		// Create the overlapped I/O event and structures
		memset(&m_os, 0, sizeof(OVERLAPPED));
		m_os.hEvent = hEvents[1];
		WSAResetEvent(m_os.hEvent);
		m_recvInitiated = true;
		received = WSARecv(m_actualSocket, &buffer, 1, &bytes_read, &msg_flags, &m_os, NULL); // Start an asynchronous read
		m_lastError = WSAGetLastError();
	}

	// If the timer has been invalidated, restart it
  if(m_recvEndTime == 0)
  {
    m_recvEndTime = CTime::GetCurrentTime() + CTimeSpan(0,0,0,m_recvTimeoutSeconds);
  }
	// Now wait for the I/O to complete if necessary, and see what happened
	bool IOCompleted = false;

	if ((received == SOCKET_ERROR) && (m_lastError == WSA_IO_PENDING))  // Read in progress, normal case
	{
		CTimeSpan TimeLeft = m_recvEndTime - CTime::GetCurrentTime();
		DWORD dwWait, milliSecondsLeft = (DWORD)TimeLeft.GetTotalSeconds()*1000;
    if(milliSecondsLeft <= 5)
    {
      dwWait = WAIT_TIMEOUT;
    }
		else
		{
			dwWait = WaitForMultipleObjects(2, hEvents, false, milliSecondsLeft);
      if(dwWait == WAIT_OBJECT_0 + 1) // The read event 
      {
        IOCompleted = true;
      }
		}
	}
  else if(!received) // if received is zero, the read was completed immediately
  {
    IOCompleted = true;
  }
	if(IOCompleted)
	{
		m_recvInitiated = false;
		if(WSAGetOverlappedResult(m_actualSocket,&m_os,&bytes_read,true,&msg_flags) && (bytes_read > 0))
		{
      if(!InSecureMode())
      {
        DebugMsg(_T(" "));
        DebugMsg(_T("Received message has %d bytes"),bytes_read);
        PrintHexDump(bytes_read,p_buffer);
      }

			m_lastError = 0;
      if(bytes_read == p_length) // We got what was requested
      {
        m_recvEndTime = 0; // Restart the timer on the next read
      }
			return bytes_read; // Normal case, we read some bytes, it's all good
		}
		else
		{	
      // A bad thing happened
			int error = WSAGetLastError();
      if(error == 0) // The socket was closed
      {
        return 0;
      }
      else if(m_lastError == 0)
      {
        m_lastError = error;
      }
		}
	}
	return SOCKET_ERROR;
}

void CALLBACK PlainSocketOverlappedResult(PTP_CALLBACK_INSTANCE pInstance
                                         ,PVOID                 pvContext
                                         ,PVOID                 pOverlapped
                                         ,ULONG                 IoResult
                                         ,ULONG_PTR             NumberOfBytesTransferred
                                         ,PTP_IO                pIo)
{
  PlainSocket* socket = reinterpret_cast<PlainSocket*>(pvContext);
  if(socket)
  {
    if(((LPOVERLAPPED)pOverlapped)->Offset == SD_SEND)
    {
      socket->SendingOverlapped(IoResult,(DWORD)NumberOfBytesTransferred,0);
    }
    else // SD_RECEIVE
    {
      socket->ReceiveOverlapped(IoResult,(DWORD)NumberOfBytesTransferred,0);
    }
    // Ready
    ((LPOVERLAPPED)pOverlapped)->OffsetHigh = 0;
  }
}

// Receives up to   p_length bytes of data with an OVERLAPPED callback
int   
PlainSocket::RecvPartialOverlapped(LPVOID p_buffer, const ULONG p_length,LPOVERLAPPED p_overlapped)
{
	WSABUF    buffer;
  DWORD			bytes_read = 0;
  DWORD     msg_flags  = 0;
	int       received   = 0;

  // Not using timeout on overlapped I/O
  // The intended use is to NOT wait and return immediately
  m_recvEndTime = 0;

	if(m_recvInitiated)
	{
		// Overlapped I/O not yet ready
		m_lastError   = WSA_IO_PENDING;
    return SOCKET_ERROR;
  }

  // Now initiating a read action
  m_recvInitiated = true;

  // Overlapped I/O from caller
  m_readOverlapped = p_overlapped;

  // We will be reading into this external buffer
  m_receiveBuffer = p_buffer;

  // Normal case, the last read completed normally, now we're reading again
	// Setup the buffers array
	buffer.buf = static_cast<char*>(p_buffer);
	buffer.len = p_length;
	
  // Make sure we have a threadpool IO record for reading
  if(m_threadPoolIO == nullptr)
  {
    m_threadPoolIO = CreateThreadpoolIo((HANDLE)m_actualSocket,PlainSocketOverlappedResult,this,nullptr);
    if(m_threadPoolIO == nullptr)
    {
      return SOCKET_ERROR;
    }
  }
  StartThreadpoolIo(m_threadPoolIO);

	// Create the overlapped I/O event and structures
	memset(&m_overReading,0,sizeof(WSAOVERLAPPED));
  m_overReading.Offset = SD_RECEIVE;
  m_overReading.OffsetHigh = 1;
	received    = WSARecv(m_actualSocket,&buffer,1,&bytes_read,&msg_flags,&m_overReading,nullptr);
	m_lastError = WSAGetLastError();

  if(m_lastError == 0 || m_lastError == WSA_IO_PENDING)  // Read in progress, normal case
	{
    TRACE("PlainSocket: WSA_IO_PENDING Succeeded!\n");
    return NO_ERROR;
	}
  TRACE("PlainSocket: FAILED IOCP!\n");

  // Cancel again
  CancelThreadpoolIo(m_threadPoolIO);

  // See if already received in-sync
  if(received > 0)
  {
    ReceiveOverlapped(m_lastError,bytes_read,msg_flags);
    return received;
  }
  return SOCKET_ERROR;
}

void
PlainSocket::ReceiveOverlapped(DWORD dwError,DWORD cbTransferred,DWORD dwFlags)
{
  // Reset the receive
  m_recvInitiated = false;

  if(!InSecureMode())
  {
    DebugMsg(_T(" "));
    DebugMsg(_T("Received message has %d bytes"),cbTransferred);
    PrintHexDump(cbTransferred,m_receiveBuffer);
  }
  if(m_readOverlapped)
  {
    PFN_SOCKET_COMPLETION completion = reinterpret_cast<PFN_SOCKET_COMPLETION>(m_readOverlapped->hEvent);
    m_readOverlapped->Internal     = dwError;
    m_readOverlapped->InternalHigh = cbTransferred;
    (*completion)(m_readOverlapped);
  }
}

// Receives exactly Len bytes of data and returns the amount received - or SOCKET_ERROR if it times out
int PlainSocket::RecvMsg(LPVOID p_buffer, const ULONG p_length)
{
  ULONG bytes_received       = 0;
	ULONG total_bytes_received = 0; 

	m_recvEndTime = 0; // Tell RecvPartial to restart the timer

	while (total_bytes_received < p_length)
	{
		bytes_received = PlainSocket::RecvPartial((char*)p_buffer+total_bytes_received, p_length-total_bytes_received);
    if(bytes_received == SOCKET_ERROR)
    {
      return SOCKET_ERROR;
    }
    else if(bytes_received == 0)
    {
      break; // socket is closed, no data left to receive
    }
    else
    {
      total_bytes_received += bytes_received;
    }
	} // loop
	return (total_bytes_received);
}

// sends a message, or part of one
int PlainSocket::SendPartial(LPCVOID p_buffer, const ULONG p_length)
{
	WSAOVERLAPPED os;
	WSABUF buffer;
	DWORD bytes_sent = 0;

	// Setup the buffer array
	buffer.buf = (char *)p_buffer;
	buffer.len = p_length;


  if(!InSecureMode())
  {
    DebugMsg(_T(" "));
    DebugMsg(_T("Send message has %d bytes"),p_length);
    PrintHexDump(p_length,p_buffer);
  }

	// Reset the timer if it has been invalidated 
  if(m_sendEndTime == 0)
  {
    m_sendEndTime = CTime::GetCurrentTime() + CTimeSpan(0,0,0,m_sendTimeoutSeconds);
  }
	m_lastError = 0;

	// Create the overlapped I/O event and structures
	memset(&os, 0, sizeof(OVERLAPPED));
	os.hEvent = m_write_event;
	WSAResetEvent(m_read_event);
	int received = WSASend(m_actualSocket, &buffer, 1, &bytes_sent, 0, &os, NULL);
	m_lastError  = WSAGetLastError();

	// Now wait for the I/O to complete if necessary, and see what happened
	bool IOCompleted = false;

	if ((received == SOCKET_ERROR) && (m_lastError == WSA_IO_PENDING))  // Write in progress
	{
		WSAEVENT hEvents[2] = { m_stopEvent,m_write_event };
	  DWORD dwWait;
		CTimeSpan TimeLeft = m_sendEndTime - CTime::GetCurrentTime();
		dwWait = WaitForMultipleObjects(2, hEvents, false, (DWORD)TimeLeft.GetTotalSeconds()*1000);
    if(dwWait == WAIT_OBJECT_0 + 1) // The write event
    {
      IOCompleted = true;
    }
	}
  else if(!received) // if rc is zero, the write was completed immediately, which is common
  {
    IOCompleted = true;
  }

	if (IOCompleted)
	{
		DWORD msg_flags = 0;
		if (WSAGetOverlappedResult(m_actualSocket, &os, &bytes_sent, true, &msg_flags))
		{
      if(bytes_sent == p_length) // Everything that was requested was sent
      {
        m_sendEndTime = 0;  // Invalidate the timer so it is set next time through
      }
			return bytes_sent;
		}
	}
	return SOCKET_ERROR;
}

// Sends up to p_length bytes of data with an OVERLAPPED callback
int
PlainSocket::SendPartialOverlapped(LPVOID p_buffer,const ULONG p_length,LPOVERLAPPED p_overlapped)
{
	WSABUF    buffer;
  DWORD			bytes_sent = 0;
  DWORD     msg_flags  = 0;
	int       written    = 0;

  // Not using timeout on overlapped I/O
  // The intended use is to NOT wait and return immediately
  m_sendEndTime = 0;

	if(m_sendInitiated)
	{
		// Overlapped I/O not yet ready
		m_lastError   = WSA_IO_PENDING;
    return SOCKET_ERROR;
  }

  // Now initiating a read action
  m_sendInitiated = true;

  // Overlapped I/O from caller
  m_sendOverlapped = p_overlapped;

  // We will be reading into this external buffer
  m_sendingBuffer = p_buffer;

  // Normal case, the last sent completed normally, now we're sending again
	// Setup the buffers array
	buffer.buf = static_cast<char*>(p_buffer);
	buffer.len = p_length;
	
  // Make sure we have a threadpool IO record for reading
  if(m_threadPoolIO == nullptr)
  {
    m_threadPoolIO = CreateThreadpoolIo((HANDLE)m_actualSocket,PlainSocketOverlappedResult,this,nullptr);
    if(m_threadPoolIO == nullptr)
    {
      return SOCKET_ERROR;
    }
  }
  StartThreadpoolIo(m_threadPoolIO);

	// Create the overlapped I/O event and structures
	memset(&m_overSending,0,sizeof(WSAOVERLAPPED));
  m_overSending.Offset = SD_SEND;
  m_overSending.OffsetHigh = 1;
	written     = WSASend(m_actualSocket,&buffer,1,&bytes_sent,msg_flags,&m_overSending,nullptr);
	m_lastError = WSAGetLastError();

  if(m_lastError == 0 || m_lastError == WSA_IO_PENDING)  // write  in progress, normal case
	{
    TRACE("PlainSocket: WSA_IO_PENDING Succeeded!\n");
    return NO_ERROR;
	}
  TRACE("PlainSocket: FAILED IOCP!\n");

  // Cancel again
  CancelThreadpoolIo(m_threadPoolIO);

  // See if already received in-sync
  if(written > 0)
  {
    SendingOverlapped(m_lastError,bytes_sent,msg_flags);
    return written;
  }
  return SOCKET_ERROR;
}

void
PlainSocket::SendingOverlapped(DWORD dwError,DWORD cbTransferred,DWORD dwFlags)
{
  // Reset the receive
  m_sendInitiated = false;

  if(!InSecureMode())
  {
    DebugMsg(_T(" "));
    DebugMsg(_T("Sending message has %d bytes"),cbTransferred);
    PrintHexDump(cbTransferred,m_sendingBuffer);
  }
  if(m_sendOverlapped)
  {
    PFN_SOCKET_COMPLETION completion = reinterpret_cast<PFN_SOCKET_COMPLETION>(m_sendOverlapped->hEvent);
    m_sendOverlapped->Internal     = dwError;
    m_sendOverlapped->InternalHigh = cbTransferred;
    (*completion)(m_sendOverlapped);
  }
}

// sends all the data or returns a timeout
//
int
PlainSocket::SendMsg(LPCVOID p_buffer, const ULONG p_length)
{
	ULONG	bytes_sent       = 0;
	ULONG total_bytes_sent = 0;

  // Do we have something to do?
  if(p_length == 0)
  {
    return 0;
  }

	m_sendEndTime = 0; // Invalidate the timer so SendPartial can reset it.

	while (total_bytes_sent < p_length)
	{
		bytes_sent = PlainSocket::SendPartial((char*)p_buffer + total_bytes_sent, p_length - total_bytes_sent);
    if((bytes_sent == SOCKET_ERROR))
    {
      return SOCKET_ERROR;
    }
    else if(bytes_sent == 0)
    {
      if(total_bytes_sent == 0)
      {
        return SOCKET_ERROR;
      }
      else
      {
        break; // socket is closed, no chance of sending more
      }
    }
    else
    {
      total_bytes_sent += bytes_sent;
    }
	}; // loop
	return (total_bytes_sent);
}

// Test if the socket is (still) readable
bool
PlainSocket::IsReadible(bool& p_readible)
{
  timeval timeout = {0, 0};
  fd_set  fds;
  FD_ZERO(&fds);
  FD_SET(m_actualSocket,&fds);
  int status = select(0,&fds,nullptr,nullptr,&timeout);
  if(status == SOCKET_ERROR)
  {
    return false;
  }
  else
  {
    p_readible = !(status == 0);
    return true;
  }
}
