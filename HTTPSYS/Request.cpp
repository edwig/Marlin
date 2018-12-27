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
#include "Request.h"
#include "RequestQueue.h"
#include "UrlGroup.h"
#include "Listener.h"
#include "Logging.h"
#include "PlainSocket.h"
#include "SecureServerSocket.h"
#include "SSLUtilities.h"
#include "CodeBase64.h"
#include <wininet.h>
#include <mswsock.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

Request::Request(RequestQueue* p_queue
                ,Listener*     p_listener
                ,SOCKET        p_socket
                ,HANDLE        p_stopEvent)
        :m_queue(p_queue)
        ,m_listener(p_listener)
        ,m_handshakeDone(false)
        ,m_secure(false)
        ,m_url(nullptr)
{
  // HTTP_REQUEST_V1 && V2
  ZeroMemory(&m_request,sizeof(HTTP_REQUEST_V2));
  // Reset the context handle for authentication
  ZeroMemory(&m_context,sizeof(CtxtHandle));

  // Setting other members
  m_status        = RQ_CREATED;
  m_bytesRead     = 0L;
  m_contentLength = 0L;
  m_bytesRead     = 0L;
  m_bytesWritten  = 0L;
  m_timestamp     = 0L;
  m_token         = NULL;

  // Create a socket, conforming to the security mode
  SetSocket(p_listener,p_socket,p_stopEvent);

  // Setting OUR identity!!
  // This is WHY we implemented HTTP.SYS in user space!
  m_request.ConnectionId = (HTTP_CONNECTION_ID)m_socket;
  m_request.RequestId    = (HTTP_REQUEST_ID)   this;
}

Request::~Request()
{
  CloseRequest();
  Reset();
}

// Starting of a request.
// Receive the general HTTP line and all headers lines
// And put the request in the RequestQueue for our program 
void
Request::ReceiveRequest()
{
  // Try to establish a SSL/TLS connection (if so necessary)
  try
  {
    InitiateSSL();
  }
  catch (int error)
  {
    UNREFERENCED_PARAMETER(error);
    ReplyClientError();
    return;
  }

  bool looping = false;

  // Receive the headers and an initial body part
  // Loop for authentication challenging if needed
  do
  {
    try
    {
      ReceiveHeaders();
      if(CheckAuthentication())
      {
        looping = true;
        DrainRequest();
        ReplyClientError(HTTP_STATUS_DENIED, "Not authenticated");
        ResetRequestV1();
      }
      else
      {
        m_queue->AddIncomingRequest(this);
        looping = false;
      }
    }
    catch(int error)
    {
      if(error == HTTP_STATUS_SERVICE_UNAVAIL)
      {
        ReplyServerError(error,"Service temporarily unavailable");
      }
      else if(error == ERROR_HANDLE_EOF)
      {
        // Channel closed
        m_queue->RemoveRequest(this);
        return;
      }
      else
      {
        LogError("Illegal HTTP client call. Error: %d", error);
        ReplyClientError();
        delete this;
      }
    }
  }
  while(looping);
}

// Close the request when we're done with this request
// Just close the socket at our end with a shutdown
// Do not wait for the client but close the socket altogether right away
//
void
Request::CloseRequest()
{
  if(m_socket)
  {
    if(m_socket->Close() == false)
    {
      // Log the error
      int error = WSAGetLastError();
      LogError("Error shutdown connection: %s Error: %d",m_request.pRawUrl,error);
    }
    delete m_socket;
    m_socket = nullptr;
  }

  // Free the addresses here
  if(m_request.Address.pLocalAddress)
  {
    free((void*)m_request.Address.pLocalAddress);
    m_request.Address.pLocalAddress = nullptr;
  }
  if(m_request.Address.pRemoteAddress)
  {
    free((void*)m_request.Address.pRemoteAddress);
    m_request.Address.pRemoteAddress = nullptr;
  }
}

// Drain our request, so we can get busy with a new one 
// Done for the sake of an authentication round trip request
void
Request::DrainRequest()
{
  // We have no business here if we are not a keep-alive connection
  // In that case we just dis-regard the incoming buffers.
  if(m_keepAlive == false)
  {
    return;
  }

  // Calculate the amount we must read from the stream to reach the next 
  // HTTP header position of the next command
  ULONGLONG readin = m_contentLength;
  if(m_bytesRead < m_contentLength)
  {
    readin -= m_bytesRead;
  }

  // drain the incoming request body
  if(readin > 0)
  {
    // Temporary read buffer
    unsigned char* drain_buffer = new unsigned char[MESSAGE_BUFFER_LENGTH + 1];

    // Loop until the end of the content (the body)
    while(readin > 0)
    {
      ULONG read = 0L;
      if(ReceiveBuffer(drain_buffer,MESSAGE_BUFFER_LENGTH,&read,false) > 0)
      {
        if(read <= readin)
        {
          readin -= read;
        }
        else readin = 0;
      }
      else break;
    }
    delete [] drain_buffer;
  }
}

// If it was a keep-alive connection, reset to receive new request
// on the same socket channel. Remove from servicing queue first!!
bool
Request::RestartConnection()
{
  // No keep-alive stated by the request. We will throw this object away!
  if(!m_keepAlive)
  {
    return false;
  }

  // Application might not have read all of our payload body content
  if((m_status == RQ_READING) && (m_bytesRead < m_contentLength))
  {
    DrainRequest();
  }

  // Prepare for the next request
  ResetRequestV1();

  // Do **NOT** reset the HTTP_REQUEST_V2, we want the retain the authentication!!

  // Remove from servicing queue and restart a worker
  m_queue->ResetToServicing(this);
  m_status = RQ_CREATED;

  // Start new thread, like the listener would do
  AfxBeginThread(m_listener->Worker,this);
  return true;
}

// Reading the body of the HTTP call
int
Request::ReceiveBuffer(PVOID p_buffer,ULONG p_size,PULONG p_bytes,bool p_all)
{
  bool  reading   = true;
  ULONG totalRead = 0L;
  int   result    = 0;


  // Initial buffer left?
  if(m_bufferPosition < m_initialLength)
  {
    return CopyInitialBuffer(p_buffer,p_size,p_bytes);
  }

  // Restrict the number of bytes to read if a keep-alive situation
  if(m_status == RQ_READING && m_keepAlive &&
     m_contentLength && (m_bytesRead < m_contentLength) &&
     m_contentLength - m_bytesRead < p_size)
  {
    p_size = (ULONG) m_contentLength;
  }

  // Reading loop
  while (reading)
  {
    // Did we read the entire body already?
    if(m_contentLength && m_bytesRead >= m_contentLength)
    {
      result = NO_ERROR;
      break;
    }

    // Try to get a new buffer
    ULONG bytes = 0;
    result = ReadBuffer(p_buffer,p_size,&bytes);
    if(result)
    {
      result = ERROR_HANDLE_EOF;
      break;
    }

    // Remember how much we read
    totalRead += bytes;

    // One time read action
    if(p_all == false)
    {
      break;
    }

    // Calculate next position in buffer for next read action
    if((ULONG)bytes < p_size)
    {
      p_buffer = (PVOID)((ULONGLONG)p_buffer + (ULONGLONG)bytes);
      p_size  -= bytes;
    }
    else reading = false;
  }

  // Return number of bytes
  if(p_bytes)
  {
    *p_bytes = totalRead;
  }
  return result;
}

// Preset a receive chunk from the applications buffer
// Only done within the HttpReceiveHttpRequest if we also want a first buffer
void
Request::ReceiveChunk(PVOID p_buffer,ULONG p_size)
{
  USHORT count = m_request.EntityChunkCount;

  m_request.pEntityChunks = (PHTTP_DATA_CHUNK) realloc(m_request.pEntityChunks,(count + 1) * sizeof(PHTTP_DATA_CHUNK));
  m_request.pEntityChunks[count].DataChunkType           = HttpDataChunkFromMemory;
  m_request.pEntityChunks[count].FromMemory.pBuffer      = (PHTTP_DATA_CHUNK) p_buffer;
  m_request.pEntityChunks[count].FromMemory.BufferLength = p_size;

  ++m_request.EntityChunkCount;
}

//////////////////////////////////////////////////////////////////////////
//
// SENDING A RESPONSE
//
//////////////////////////////////////////////////////////////////////////

// Initially send the response to the client
// - Beginning with the HTTP status line
// - All response headers
// - Empty separator line between headers and body
// - Any entity chunks given in the first response as first body part
//
int
Request::SendResponse(PHTTP_RESPONSE p_response,PULONG p_bytes)
{
  // Drain any body not read in by the application
  DrainRequest();

  // Start answering the request
  m_status = RQ_ANSWERING;

  // Reset bytes written
  m_bytesWritten = 0;

  // Create the response line
  CString buffer;
  AddResponseLine(buffer,p_response);

  // Add all response headers
  AddAllKnownResponseHeaders  (buffer,p_response->Headers.KnownHeaders);
  AddAllUnknownResponseHeaders(buffer,p_response->Headers.pUnknownHeaders
                                     ,p_response->Headers.UnknownHeaderCount);
  
  // Line between headers and body
  buffer += "\r\n";

  // Perform one write of all header lines in one go!
  int result = WriteBuffer((PVOID)buffer.GetString(),buffer.GetLength(),p_bytes);
  if(result == NO_ERROR)
  {
    // Reset byte pointers
    m_bytesWritten  = 0;
    m_contentLength = 0;
    // Set status to writing the body
    m_status = RQ_WRITING;

    // Grab the content length of the body
    if(p_response->Headers.KnownHeaders[HttpHeaderContentLength].pRawValue)
    {
      m_contentLength = atoi(p_response->Headers.KnownHeaders[HttpHeaderContentLength].pRawValue);
    }

    // Also send our body right away, if any chunks given
    if(p_response->EntityChunkCount)
    {
      ULONG bytes = 0;
      int  result = SendEntityChunks(p_response->pEntityChunks,p_response->EntityChunkCount,&bytes);
      if (result == NO_ERROR)
      {
        *p_bytes += bytes;
      }
    }
  }
  // Ready
  return result;
}

// Sending a group of entity chunks. Used for both:
// - HttpSendHttpResponse
// - HttpSendResponseEntityBody
//
int
Request::SendEntityChunks(PHTTP_DATA_CHUNK p_chunks,int p_count,PULONG p_bytes)
{
  for(int index = 0; index < p_count; ++index)
  {
    int result = SendEntityChunk(&p_chunks[index],p_bytes);
    if (result != NO_ERROR)
    {
      return result;
    }
  }
  return NO_ERROR;
}

// See if our response is already completed (all chunks sent)
bool
Request::GetResponseComplete()
{
  if(m_contentLength == 0 || m_bytesWritten >= m_contentLength)
  {
    return true;
  }
  return false;
}

//////////////////////////////////////////////////////////////////////////
//
// PRIVATE
//
//////////////////////////////////////////////////////////////////////////

// Totally resetting the object, freeing all memory
// Normally only done when destructing the object
void
Request::Reset()
{
  // HTTP_REQUEST_V1
  // Headers and other structures
  ResetRequestV1();

  // HTTP_REQUEST_V2
  // Can have 1 (authentication) or 2 (SSL) records
  ResetRequestV2();

  // Free the headers line
  FreeInitialBuffer();

  // Primary authentication token
  if(m_token)
  {
    CloseHandle(m_token);
    m_token = NULL;
  }
}

// Reset the object so that we may receive a new request on the same socket
void
Request::ResetRequestV1()
{
  // Reset parameters of HTTP_REQUEST_V1
  m_request.Flags = 0;

  // ConnectionId / RequestId are untouched: linking us to the program

  m_request.UrlContext            = 0L;
  m_request.Version.MajorVersion  = 0;
  m_request.Version.MinorVersion  = 0;
  m_request.Verb                  = HttpVerbUnparsed;
  m_request.BytesReceived         = 0;
  m_request.RawConnectionId       = 0;

  // VERB & URL
  if(m_request.pUnknownVerb)
  {
    free((void*)m_request.pUnknownVerb);
    m_request.pUnknownVerb = nullptr;
    m_request.UnknownVerbLength = 0;
  }
  if(m_request.pRawUrl)
  {
    free((void*)m_request.pRawUrl);
    m_request.pRawUrl = nullptr;
    m_request.RawUrlLength = 0;
  }
  if(m_request.CookedUrl.pFullUrl)
  {
    free((void*)m_request.CookedUrl.pFullUrl);
    m_request.CookedUrl.pFullUrl          = nullptr;
    m_request.CookedUrl.pHost             = nullptr;
    m_request.CookedUrl.pAbsPath          = nullptr;
    m_request.CookedUrl.pQueryString      = nullptr;
    m_request.CookedUrl.FullUrlLength     = 0;
    m_request.CookedUrl.HostLength        = 0;
    m_request.CookedUrl.AbsPathLength     = 0;
    m_request.CookedUrl.QueryStringLength = 0;
  }

  // Unknown headers/trailers
  if(m_request.Headers.pUnknownHeaders)
  {
    for(int ind = 0;ind < m_request.Headers.UnknownHeaderCount; ++ind)
    {
      free((void*)m_request.Headers.pUnknownHeaders[ind].pName);
      free((void*)m_request.Headers.pUnknownHeaders[ind].pRawValue);
      m_request.Headers.pUnknownHeaders[ind].NameLength = 0;
      m_request.Headers.pUnknownHeaders[ind].RawValueLength = 0;
    }
    free(m_request.Headers.pUnknownHeaders);
    m_request.Headers.pUnknownHeaders = nullptr;
    m_request.Headers.UnknownHeaderCount = 0;
  }
  if(m_request.Headers.pTrailers)
  {
    for(int ind = 0;ind < m_request.Headers.TrailerCount;++ind)
    {
      free((void*)m_request.Headers.pTrailers[ind].pName);
      free((void*)m_request.Headers.pTrailers[ind].pRawValue);
      m_request.Headers.pTrailers[ind].NameLength = 0;
      m_request.Headers.pTrailers[ind].RawValueLength = 0;
    }
    free(m_request.Headers.pTrailers);
    m_request.Headers.pTrailers = nullptr;
    m_request.Headers.TrailerCount = 0;
  }

  // Known headers
  for(int ind = 0;ind < HttpHeaderRequestMaximum; ++ind)
  {
    if(m_request.Headers.KnownHeaders[ind].pRawValue)
    {
      free((void*)m_request.Headers.KnownHeaders[ind].pRawValue);
      m_request.Headers.KnownHeaders[ind].pRawValue = nullptr;
      m_request.Headers.KnownHeaders[ind].RawValueLength = 0;
    }
  }

  // Entity chunks
  if(m_request.pEntityChunks)
  {
    free(m_request.pEntityChunks);
    m_request.pEntityChunks = nullptr;
    m_request.EntityChunkCount = 0;
    // Data buffers are allocated by the calling program!!
    // They are never free-ed here!
  }

  // SSL Info and Certificate
  if(m_request.pSslInfo)
  {
    free((void*)m_request.pSslInfo->pServerCertIssuer);
    m_request.pSslInfo->pServerCertIssuer = nullptr;
    m_request.pSslInfo->ServerCertIssuerSize = 0;

    free((void*)m_request.pSslInfo->pServerCertSubject);
    m_request.pSslInfo->pServerCertSubject = nullptr;
    m_request.pSslInfo->ServerCertSubjectSize = 0;

    if(m_request.pSslInfo->pClientCertInfo)
    {
      free((void*)m_request.pSslInfo->pClientCertInfo->pCertEncoded);
      m_request.pSslInfo->pClientCertInfo->pCertEncoded = nullptr;
      m_request.pSslInfo->pClientCertInfo->CertEncodedSize = 0;
    }
    free(m_request.pSslInfo);
    m_request.pSslInfo = nullptr;
  }

  // Also reset general parameters
  m_status        = RQ_CREATED;
  m_bytesRead     = 0L;
  m_bytesWritten  = 0L;
  m_contentLength = 0L;
  m_keepAlive     = false;
  m_url           = nullptr;
}

// Reset HTTP_REQUEST_V2
// Can have 1 (authentication) or 2 (SSL) records
void
Request::ResetRequestV2()
{
  if(m_request.pRequestInfo)
  {
    for(int index = 0;index < m_request.RequestInfoCount; ++index)
    {
      if(m_request.pRequestInfo[index].InfoType == HttpRequestInfoTypeAuth)
      {
        free(m_request.pRequestInfo[index].pInfo);
      }
    }
    free(m_request.pRequestInfo);
  }
  m_request.pRequestInfo = nullptr;
  m_request.RequestInfoCount = 0;
}

// Create a socket for this request
void
Request::SetSocket(Listener* p_listener,SOCKET p_socket,HANDLE p_stop)
{
  // Create PlainSocket (normal) or SecureServerSocket (SSL/TLS)
  if(p_listener->GetSecureMode())
  {
    m_secure = true;
    m_socket = new SecureServerSocket(p_socket,p_stop);
  }
  else
  {
    m_socket = new PlainSocket(p_socket,p_stop);
  }

  // Prepare socket for this request (addresses and timings)
  // Done for both PlainSocket and SecureServerSocket alike
  SetAddresses(p_socket);
  SetTimings();

  // Now set parameters for SSL/TLS mode
  if(m_secure)
  {
    SecureServerSocket* secure = reinterpret_cast<SecureServerSocket*>(m_socket);

    // Setting for this secure socket
    secure->m_selectServerCert        = p_listener->m_selectServerCert;
    secure->SetThumbprint              (p_listener->GetThumbprint());
    secure->SetCertificateStore        (p_listener->GetCertificateStore());
    secure->SetRequestClientCertificate(p_listener->GetRequestClientCertificate());
    if(p_listener->GetRequestClientCertificate())
    {
      secure->m_clientCertAcceptable = p_listener->m_clientCertAcceptable;
    }
  }
}

// Grabbing from our socket the addresses of both local and remote side
void
Request::SetAddresses(SOCKET p_socket)
{
  // Address structure at least big enough for IPv6
  // WinSock documentation on MSDN says: at least 1 paragraph of memory extra!!
  sockaddr_in6 sockaddr;
  int namelen = sizeof(sockaddr_in6) + 16;

  // Get local socket information
  if(getsockname(p_socket,(PSOCKADDR)&sockaddr,&namelen))
  {
    int error = WSAGetLastError();
    LogError("Cannot get local address name for connection: %s",m_request.pRawUrl);
  }
  // Keep the address info
  m_request.Address.pLocalAddress = (PSOCKADDR) calloc(1,namelen);
  memcpy(m_request.Address.pLocalAddress,&sockaddr,namelen);
  // And keep the port number
  m_port = ntohs(sockaddr.sin6_port);

  // Get remote address information
  if(getpeername(p_socket,(PSOCKADDR)&sockaddr,&namelen))
  {
    int error = WSAGetLastError();
    LogError("Cannot get remote address name for connection: %s",m_request.pRawUrl);
  }
  // Keep as the remote side of the channel
  m_request.Address.pRemoteAddress = (PSOCKADDR)calloc(1,namelen);
  memcpy(m_request.Address.pRemoteAddress,&sockaddr,namelen);
}

// Set timeout settings for reading and writing
void
Request::SetTimings()
{
  PlainSocket* sock = reinterpret_cast<PlainSocket*>(m_socket);
  int timeout = (int)m_listener->GetRecvTimeoutSeconds();

  // If not set, use default from http.h
  if(timeout <= 0)
  {
    timeout = URL_TIMEOUT_DRAIN_BODY;
  }
  else if(timeout < HTTP_MINIMUM_TIMEOUT)
  {
    timeout = HTTP_MINIMUM_TIMEOUT;
  }

  // Use first timing for both timeouts of the socket
  sock->SetRecvTimeoutSeconds(timeout);
  sock->SetSendTimeoutSeconds(timeout);
}

// Do the handshake for a SSL/TLS connection
// For a keep-alive connection only done for the first request!
void
Request::InitiateSSL()
{
  if(m_secure)
  {
    if(!m_handshakeDone)
    {
      // Go do the SSL handshake
      SecureServerSocket* secure = reinterpret_cast<SecureServerSocket*>(m_socket);
      if(secure->InitializeSSL() != S_OK)
      {
        throw ERROR_INTERNET_SECURITY_CHANNEL_ERROR;
      }
    }
    m_handshakeDone = true;
  }
}

// Receive the first part of the message, being:
// Grab an initial buffer from the socket, and split off
// - The HTTP action line
// - All headers
// - Stop at the empty line under the headers
// - Leave the first part of the body in the initial buffers
//
void
Request::ReceiveHeaders()
{
  // Setup initial buffers
  ReadInitialMessage();

  // Getting the HTTP protocol line
  CString line = ReadTextLine();
  ReceiveHTTPLine(line);

  // Reading all request headers
  while (true)
  {
    line = ReadTextLine();
    if (line.GetLength())
    {
      ProcessHeader(line);
    }
    else break;
  }

  // Finding our site context
  FindUrlContext();

  // Finding the content length of the body (if any)
  FindContentLength();

  // Correct URL with host header
  CorrectFullURL();

  // Checking for keep-alive round trips
  FindKeepAlive();

  // Reset number of bytes read. We will now go read the body
  m_bytesRead = 0;
}

// Grab the first available message part just under the optimal buffer length
// so we can parse off the initial headers of the message
void
Request::ReadInitialMessage()
{
  FreeInitialBuffer();
  m_initialBuffer = (char*) malloc(MESSAGE_BUFFER_LENGTH + 1);

  int length = m_socket->RecvPartial(m_initialBuffer,MESSAGE_BUFFER_LENGTH);
  if (length > 0)
  {
    m_initialBuffer[length] = 0;
    m_initialLength = length;
    m_bytesRead    += length;
    return;
  }
  throw (int)ERROR_HANDLE_EOF;
}

// Find the next line in the initial buffers up to the "\r\n"
// Mark that the HTTP IETF RFC clearly states that all header
// lines must end in a <CR><LF> sequence!!
CString
Request::ReadTextLine()
{
  char* begin = &m_initialBuffer[m_bufferPosition];
  char* end = strchr(begin,'\r');

  if(end && end[1] == '\n')
  {
    int len = (int)((ULONGLONG)end - (ULONGLONG)begin);
    CString line;
    char* buffer = line.GetBufferSetLength(len);
    strncpy_s(buffer,len+1,begin,len);
    line.ReleaseBufferSetLength(len);

    m_bufferPosition += len + 2;
    return line;
  }
  throw ERROR_HTTP_INVALID_HEADER;
}

// Process the essential first line beginning with the HTTP verb
// VERB /absolute/url/of/the/request  HTTP/1.1
void
Request::ReceiveHTTPLine(CString p_line)
{
  USES_CONVERSION;

  char* verb = p_line.GetBuffer();
  char* url  = strchr(p_line.GetBuffer(),' ');
  if(url)
  {
    *url++ = 0;
    char* protocol = strrchr(url,' ');
    if(protocol)
    {
      *protocol++ = 0;

      // Finding the parts of the line
      FindVerb(verb);
      FindURL(url);
      FindProtocol(protocol);
      return;
    }
  }
  // Not a valid HTTP protocol line
  throw ERROR_HTTP_INVALID_HEADER;
}

// The line contains a HTTP header
// either do one of two things:
// - Store it as a 'known-header'
// - Store it as a 'unknown-header'
// All storing actions require string memory allocations!!
void
Request::ProcessHeader(CString p_line)
{
  // Remove end-of-line markers.
  int len = p_line.GetLength();
  if (len > 2)
  {
    p_line.TrimRight("\r\n");
  }

  // Find separating colon
  int colon = p_line.Find(':');
  if(colon)
  {
    CString header = p_line.Left(colon);
    CString value = p_line.Mid(colon + 1);
    value.Trim();

    int knwonHeader = FindKnownHeader(header);
    if (knwonHeader >= 0)
    {
      m_request.Headers.KnownHeaders[knwonHeader].pRawValue      = _strdup(value.GetString());
      m_request.Headers.KnownHeaders[knwonHeader].RawValueLength = (USHORT)value.GetLength();
    }
    else
    {
      m_request.Headers.pUnknownHeaders = (PHTTP_UNKNOWN_HEADER) realloc(m_request.Headers.pUnknownHeaders, (1 + m_request.Headers.UnknownHeaderCount) * sizeof(HTTP_UNKNOWN_HEADER));

      m_request.Headers.pUnknownHeaders[m_request.Headers.UnknownHeaderCount].pName          = _strdup(header.GetString());
      m_request.Headers.pUnknownHeaders[m_request.Headers.UnknownHeaderCount].NameLength     = (USHORT)header.GetLength();
      m_request.Headers.pUnknownHeaders[m_request.Headers.UnknownHeaderCount].pRawValue      = _strdup(value.GetString());
      m_request.Headers.pUnknownHeaders[m_request.Headers.UnknownHeaderCount].RawValueLength = (USHORT)value.GetLength();

      m_request.Headers.UnknownHeaderCount++;
    }
    return;
  }
  // No colon found: no valid HTTP header line
  throw ERROR_HTTP_INVALID_HEADER;
}

// Find our absolute URL path without the query part
// and hunt in the RequestQueues URL groups for a context number
void
Request::FindUrlContext()
{
  // Find absolute path in the raw URL
  CString abspath(m_request.pRawUrl);
  int posquery = abspath.Find('?');
  if(posquery >= 0)
  {
    abspath = abspath.Left(posquery);
  }

  // See if it is a site
  int pos = abspath.ReverseFind('/');
  if(pos)
  {
    int point = abspath.Find('.', pos);
    if (point < 0)
    {
      abspath += '/';
    }
  }
  
  // Find first matching URL and copy the opaque context ID
  URL* url = m_queue->FindLongestURL(m_port,abspath);
  if(url)
  {
    m_url = url;
    m_request.UrlContext = url->m_context;
    return;
  }
  // Weird: no context found
  // throw HTTP_STATUS_NOT_FOUND;
}

// Finding the content length header
// Also works for empty headers (converts to zero)
void
Request::FindContentLength()
{
  if(m_request.Headers.KnownHeaders[HttpHeaderContentLength].pRawValue)
  {
    m_contentLength = _atoi64(m_request.Headers.KnownHeaders[HttpHeaderContentLength].pRawValue);
    if(m_contentLength)
    {
      m_request.Flags = HTTP_REQUEST_FLAG_MORE_ENTITY_BODY_EXISTS;
    }
  }
}

// In case we just have an URL without a host: add the host
void
Request::CorrectFullURL()
{
  CString host;
  if(m_request.Headers.KnownHeaders[HttpHeaderHost].pRawValue)
  {
    host = m_request.Headers.KnownHeaders[HttpHeaderHost].pRawValue;

    CString abspath(m_request.CookedUrl.pAbsPath);
    if(abspath.Find("//") < 0)
    {
      CString newpath;
      newpath.Format("http://%s%s",host,abspath);

      if(m_request.CookedUrl.pFullUrl)
      {
        free((PVOID)m_request.CookedUrl.pFullUrl);
        m_request.CookedUrl.pFullUrl = nullptr;
      }
      if (m_request.pRawUrl)
      {
        free((PVOID)m_request.pRawUrl);
        m_request.pRawUrl = nullptr;
      }

      FindURL((char*)newpath.GetString());
    }
  }
}

// At various stages: we may now forget the initial buffer
// In case we just read all of the body
// or in case we reset and destroy the request.
void
Request::FreeInitialBuffer()
{
  if(m_initialBuffer)
  {
    free(m_initialBuffer);
  }
  m_initialBuffer  = nullptr;
  m_initialLength  = 0;
  m_bufferPosition = 0;
}

// These are all of the 'known' verbs that the HTTPSYS
// nows about. All other verbs have the status 'unknown'
// and are stored by there full name
//
static const char* all_verbs[] = 
{
  "",
  "",
  "",
  "OPTIONS",
  "GET",
  "HEAD",
  "POST",
  "PUT",
  "DELETE",
  "TRACE",
  "CONNECT",
  "TRACK",
  "MOVE",
  "COPY",
  "PROPFIND",
  "PROPPATCH",
  "MKCOL",
  "LOCK",
  "UNLOCK",
  "SEARCH"
};

// Finding the VERB in the all_verbs array.
// In case we do not find the verb, we store a string duplicate
void
Request::FindVerb(char* p_verb)
{
  // Find a known-verb and store the status
  for(int ind = HttpVerbOPTIONS; ind < HttpVerbMaximum; ++ind)
  {
    if(_stricmp(p_verb,all_verbs[ind]) == 0)
    {
      m_request.Verb = (HTTP_VERB)ind;
      return;
    }
  }

  // Store unknown VERB as word
  m_request.pUnknownVerb      = _strdup(p_verb);
  m_request.UnknownVerbLength = (USHORT) strlen(p_verb);
}

// Finding and storing an URL in the request structure
// 1) As a string duplicate
// 2) As a Unicode string duplicate
// 3) As a 'cooked' pointer set to the unicode duplicate
//
void
Request::FindURL(char* p_url)
{
  USES_CONVERSION;

  // skip whitespace
  CString full(p_url);
  full = full.Trim();

  // Copy the raw URL
  m_request.pRawUrl      = _strdup(full.GetString());
  m_request.RawUrlLength = (USHORT) strlen(p_url);
  // FULL URL
  wchar_t* copy  = _wcsdup(A2CW(full.GetString()));
  m_request.CookedUrl.pFullUrl = copy;
  m_request.CookedUrl.FullUrlLength = (USHORT) (wcslen(m_request.CookedUrl.pFullUrl) * sizeof(wchar_t));

  // Cook the URL
  int posHost  = full.Find("//");
  int posPort  = full.Find(':', posHost + 1);
  int posPath  = full.Find('/', posHost > 0 ? posHost + 2 : 0);
  int posQuery = full.Find('?');

  // Find pointers and lengths
  if(posHost >= 0)
  {
    m_request.CookedUrl.pHost = &copy[posHost + 2];
    if(posPort > 0)
    {
      m_request.CookedUrl.HostLength = (USHORT) (posPort - posHost + 1) * sizeof(wchar_t);
    }
    else
    {
      m_request.CookedUrl.HostLength = (USHORT)(posPath - posHost + 1) * sizeof(wchar_t);
    }
  }
  if (posPath >= 0)
  {
    m_request.CookedUrl.pAbsPath = &copy[posPath];
    if (posQuery > 0)
    {
      m_request.CookedUrl.AbsPathLength = (USHORT)(posQuery - posPath + 1) * sizeof(wchar_t);
    }
    else
    {
      m_request.CookedUrl.AbsPathLength = (USHORT) wcslen(m_request.CookedUrl.pAbsPath);
    }
  }
  if (posQuery > 0)
  {
    m_request.CookedUrl.pQueryString = &copy[posQuery];
    m_request.CookedUrl.QueryStringLength = (USHORT) wcslen(m_request.CookedUrl.pQueryString);
  }
}

// Finding the HTTP protocol and version numbers at the end of the
// HTTP command line (Beginning with the HTTP verb)
void
Request::FindProtocol(char* p_protocol)
{
  // skip whitespace
  while(isspace(*p_protocol)) ++p_protocol;

  if(_strnicmp(p_protocol, "http/", 5) == 0)
  {
    char* major = &p_protocol[5];
    char* minor = strchr(p_protocol, '.');
    if(minor)
    {
      m_request.Version.MajorVersion = atoi(major);
      m_request.Version.MinorVersion = atoi(++minor);
      return;
    }
  }
  // No "HTTP/x.y" found
  throw ERROR_HTTP_INVALID_HEADER;
}

// Known headers for a HTTP call from client to server
//
static const char* all_headers[] =
{
  "Cache-Control"         //  HttpHeaderCacheControl          = 0,    // general-header [section 4.5]
 ,"Connection"            //  HttpHeaderConnection            = 1,    // general-header [section 4.5]
 ,"Date"                  //  HttpHeaderDate                  = 2,    // general-header [section 4.5]
 ,"Keep-Alive"            //  HttpHeaderKeepAlive             = 3,    // general-header [not in rfc]
 ,"Pragma"                //  HttpHeaderPragma                = 4,    // general-header [section 4.5]
 ,"Trailer"               //  HttpHeaderTrailer               = 5,    // general-header [section 4.5]
 ,"Transfer-Encoding"     //  HttpHeaderTransferEncoding      = 6,    // general-header [section 4.5]
 ,"Upgrade"               //  HttpHeaderUpgrade               = 7,    // general-header [section 4.5]
 ,"Via"                   //  HttpHeaderVia                   = 8,    // general-header [section 4.5]
 ,"Warning"               //  HttpHeaderWarning               = 9,    // general-header [section 4.5]
 ,"Allow"                 //  HttpHeaderAllow                 = 10,   // entity-header  [section 7.1]
 ,"Content-Length"        //  HttpHeaderContentLength         = 11,   // entity-header  [section 7.1]
 ,"Content-Type"          //  HttpHeaderContentType           = 12,   // entity-header  [section 7.1]
 ,"Content-Encoding"      //  HttpHeaderContentEncoding       = 13,   // entity-header  [section 7.1]
 ,"Content-Language"      //  HttpHeaderContentLanguage       = 14,   // entity-header  [section 7.1]
 ,"Content-Location"      //  HttpHeaderContentLocation       = 15,   // entity-header  [section 7.1]
 ,"Content-Md5"           //  HttpHeaderContentMd5            = 16,   // entity-header  [section 7.1]
 ,"Content-Range"         //  HttpHeaderContentRange          = 17,   // entity-header  [section 7.1]
 ,"Expires"               //  HttpHeaderExpires               = 18,   // entity-header  [section 7.1]
 ,"Last-Modified"         //  HttpHeaderLastModified          = 19,   // entity-header  [section 7.1]
 ,"Accept"                //  HttpHeaderAccept                = 20,   // request-header [section 5.3]
 ,"Accept-Charset"        //  HttpHeaderAcceptCharset         = 21,   // request-header [section 5.3]
 ,"Accept-Encoding"       //  HttpHeaderAcceptEncoding        = 22,   // request-header [section 5.3]
 ,"Accept-Language"       //  HttpHeaderAcceptLanguage        = 23,   // request-header [section 5.3]
 ,"Authorization"         //  HttpHeaderAuthorization         = 24,   // request-header [section 5.3]
 ,"Cookie"                //  HttpHeaderCookie                = 25,   // request-header [not in rfc]
 ,"Expect"                //  HttpHeaderExpect                = 26,   // request-header [section 5.3]
 ,"From"                  //  HttpHeaderFrom                  = 27,   // request-header [section 5.3]
 ,"Host"                  //  HttpHeaderHost                  = 28,   // request-header [section 5.3]
 ,"If-Match"              //  HttpHeaderIfMatch               = 29,   // request-header [section 5.3]
 ,"If-Modified-Since"     //  HttpHeaderIfModifiedSince       = 30,   // request-header [section 5.3]
 ,"If-None-Match"         //  HttpHeaderIfNoneMatch           = 31,   // request-header [section 5.3]
 ,"If-Range"              //  HttpHeaderIfRange               = 32,   // request-header [section 5.3]
 ,"If-Unmodified-Since"   //  HttpHeaderIfUnmodifiedSince     = 33,   // request-header [section 5.3]
 ,"Max-Forwards"          //  HttpHeaderMaxForwards           = 34,   // request-header [section 5.3]
 ,"Proxy-Authorization"   //  HttpHeaderProxyAuthorization    = 35,   // request-header [section 5.3]
 ,"Referer"               //  HttpHeaderReferer               = 36,   // request-header [section 5.3]
 ,"Header-Range"          //  HttpHeaderRange                 = 37,   // request-header [section 5.3]
 ,"Te"                    //  HttpHeaderTe                    = 38,   // request-header [section 5.3]
 ,"Translate"             //  HttpHeaderTranslate             = 39,   // request-header [webDAV, not in rfc 2518]
 ,"UserAgent"             //  HttpHeaderUserAgent             = 40,   // request-header [section 5.3]
};

// Known headers for a HTTP response from server back to the client 
// Range  0-19 are identical to the request headers
// Range 20-29 are only used in responses
// Range 30-40 are never used in responses
//
static const char* all_responses[] =
{
  "Accept-Ranges"         //  HttpHeaderAcceptRanges          = 20,   // response-header [section 6.2]
 ,"Header-Age"            //  HttpHeaderAge                   = 21,   // response-header [section 6.2]
 ,"Header-Etag"           //  HttpHeaderEtag                  = 22,   // response-header [section 6.2]
 ,"Location"              //  HttpHeaderLocation              = 23,   // response-header [section 6.2]
 ,"Proxy-Authenticate"    //  HttpHeaderProxyAuthenticate     = 24,   // response-header [section 6.2]
 ,"Retry-After"           //  HttpHeaderRetryAfter            = 25,   // response-header [section 6.2]
 ,"Server"                //  HttpHeaderServer                = 26,   // response-header [section 6.2]
 ,"Set-Cookie"            //  HttpHeaderSetCookie             = 27,   // response-header [not in rfc]
 ,"Header-Vary"           //  HttpHeaderVary                  = 28,   // response-header [section 6.2]
 ,"WWW-Authenticate"      //  HttpHeaderWwwAuthenticate       = 29,   // response-header [section 6.2]
};

// Finding a 'known' header on the current header line.
int
Request::FindKnownHeader(CString p_header)
{
  // skip whitespace before and after the header name
  p_header.Trim();
  int len = p_header.GetLength();

  // Find known header
  for(int ind = 0;ind < HttpHeaderRequestMaximum; ++ind)
  {
    if(p_header.CompareNoCase(all_headers[ind]) == 0)
    {
      return ind;
    }
  }
  // Nothing found
  return -1;
}

// Find our connection settings. can have two values:
// keep-alive  -> Keep socket connection open
// close       -> Close connection after servicing the request
void
Request::FindKeepAlive()
{
  CString connection = m_request.Headers.KnownHeaders[HttpHeaderConnection].pRawValue;
  connection.Trim();
  m_keepAlive = connection.CompareNoCase("keep-alive") == 0;
}

// Reply with a client error in the range 400 - 499
void
Request::ReplyClientError(int p_error, CString p_errorText)
{
  // Drain the request first
  DrainRequest();
  m_status = RQ_ANSWERING;

  bool retry = false;

  CString header;
  header.Format("HTTP/1.1 %d %s\r\n",p_error,p_errorText);
  header += "Content-Type: text/html; charset=us-ascii\r\n";

  CString body;
  body.Format(http_client_error,p_error,p_errorText);

  if(m_challenge)
  {
    header += "WWW-Authenticate: " + m_challenge + "\r\n";
    m_challenge.Empty();
    retry   = true;
  }

  header.AppendFormat("Content-Length: %d\r\n",body.GetLength());
  header.AppendFormat("Date: %s\r\n", HTTPSystemTime());
  header += "\r\n";
  header += body;

  // Write out to the client:
  ULONG written = 0;
  WriteBuffer((PVOID)header.GetString(),header.GetLength(),&written);

  if(!retry)
  {
    // And be done with this request;
    m_keepAlive = false;
    CloseRequest();
  }
}

// General Client error 400 (Client did something stupid)
void
Request::ReplyClientError()
{
  ReplyClientError(HTTP_STATUS_BAD_REQUEST,"General client error");
}

// Reply with a server error in the range 500-599
void
Request::ReplyServerError(int p_error,CString p_errorText)
{
  // Drain the request first
  DrainRequest();
  m_status = RQ_ANSWERING;

  // The answer
  CString header;
  header.Format("HTTP/1.1 %d %s\r\n", p_error, p_errorText);
  header += "Content-Type: text/html; charset=us-ascii\r\n";

  CString body;
  body.Format(http_server_error, p_error, p_errorText);

  header.AppendFormat("Content-Length: %d\r\n", body.GetLength());
  header.AppendFormat("Date: %s\r\n",HTTPSystemTime());
  header += "\r\n";
  header += body;

  // Write out to the client:
  ULONG written = 0;
  WriteBuffer((PVOID)header.GetString(),header.GetLength(),&written);
  // And be done with this request. Always close the request
  m_keepAlive = false;
  CloseRequest();
}


// General server error 500 (Server not capable of doing this)
void
Request::ReplyServerError()
{
  ReplyServerError(HTTP_STATUS_SERVER_ERROR,"General server error");
}

// Copy the part of the first initial read to this read buffer
int
Request::CopyInitialBuffer(PVOID p_buffer,ULONG p_size,PULONG p_bytes)
{
  // Calculate how much we have left in the buffer
  ULONG length = m_initialLength - m_bufferPosition;

  // Test if we can copy the buffer in ONE go!
  if(length > p_size)
  {
    // NO: Not possible to do it in one go
    // Copy as much as we can from our initial buffer
    memcpy_s(p_buffer,p_size,&m_initialBuffer[m_bufferPosition],p_size);
    m_bufferPosition += p_size;
    m_bytesRead += p_size;
    if(p_bytes)
    {
      *p_bytes = p_size;
    }
    return ERROR_MORE_DATA;
  }

  // OK, We have enough buffer. Do it in one go, and be done with the buffer
  memcpy_s(p_buffer,length,&m_initialBuffer[m_bufferPosition],length);
  FreeInitialBuffer();
  if (p_bytes)
  {
    *p_bytes = length;
  }
  m_bytesRead += length;
  return NO_ERROR;
}

// Authentication of the request
// Return 'false' if continue to application
// Return 'true'  if more authentication is needed in a loop
bool 
Request::CheckAuthentication()
{
  ULONG scheme = m_url->m_urlGroup->GetAuthenticationScheme();

  // No authentication requested for this URL. So we do nothing
  if(scheme == 0)
  {
    return false;
  }

  // See if we have a cached authentication token
  PHTTP_REQUEST_AUTH_INFO info = GetAuthenticationInfoRecord();
  if(AlreadyAuthenticated(info))
  {
    return false;
  }

  // This is our authorization (if any)
  CString authorization(m_request.Headers.KnownHeaders[HttpHeaderAuthorization].pRawValue);

  // Find method and payload of that method
  CString method;
  CString payload;
  SplitString(authorization,method,payload,' ');

       if(method.CompareNoCase("Basic")     == 0)  info->AuthType = HttpRequestAuthTypeBasic;
  else if(method.CompareNoCase("NTLM")      == 0)  info->AuthType = HttpRequestAuthTypeNTLM;
  else if(method.CompareNoCase("Negotiate") == 0)  info->AuthType = HttpRequestAuthTypeNegotiate;
  else if(method.CompareNoCase("Digest")    == 0)  info->AuthType = HttpRequestAuthTypeDigest;
  else if(method.CompareNoCase("Kerberos")  == 0)  info->AuthType = HttpRequestAuthTypeKerberos;
  else
  {
    // Leave it to the serviced application to return a HTTP_STATUS_DENIED
    return false; 
  }

  // BASIC is a special case. Others through SSPI
  if(info->AuthType == HttpRequestAuthTypeBasic)
  {
    // Base64 encryption of the "username:password" combination
    return CheckBasicAuthentication(info,payload);
  }
  else
  {
    // All done through the SSPI provider through "secur32.dll"
    return CheckAuthenticationProvider(info,payload,method);
  }
}

// Get present record, or create a new one
PHTTP_REQUEST_AUTH_INFO
Request::GetAuthenticationInfoRecord()
{
  // See if we already have the record
  int number = m_request.RequestInfoCount;
  for(int index = 0;index < number;++index)
  {
    if(m_request.pRequestInfo[index].InfoType == HttpRequestInfoTypeAuth)
    {
      // Already defined: reuse it!
      return (PHTTP_REQUEST_AUTH_INFO) m_request.pRequestInfo[index].pInfo;
    }
  }

  // Create AUTH_INFO object with defaults
  int size = ++m_request.RequestInfoCount;
  m_request.pRequestInfo = (PHTTP_REQUEST_INFO)realloc(m_request.pRequestInfo, size * sizeof(HTTP_REQUEST_INFO));

  m_request.pRequestInfo[number].InfoType = HttpRequestInfoTypeAuth;
  m_request.pRequestInfo[number].InfoLength = sizeof(HTTP_REQUEST_AUTH_INFO);
  m_request.pRequestInfo[number].pInfo = (PHTTP_REQUEST_AUTH_INFO)calloc(1, sizeof(HTTP_REQUEST_AUTH_INFO));

  PHTTP_REQUEST_AUTH_INFO info = (PHTTP_REQUEST_AUTH_INFO)m_request.pRequestInfo[number].pInfo;
  info->AuthStatus = HttpAuthStatusNotAuthenticated;
  info->AuthType   = HttpRequestAuthTypeNone;

  return info;
}

// See if we have a cached authentication token
bool
Request::AlreadyAuthenticated(PHTTP_REQUEST_AUTH_INFO p_info)
{
  if(p_info->AuthStatus  == HttpAuthStatusSuccess   && 
     p_info->AuthType    == HttpRequestAuthTypeNTLM && 
     p_info->AccessToken != NULL)
  {
    int  seconds = (clock() - m_timestamp) / CLOCKS_PER_SEC;
    bool caching = m_url->m_urlGroup->GetAuthenticationCaching();
    int  timeout = m_url->m_urlGroup->GetTimeoutIdleConnection();

    // Re-use the authentication token if caching is 'on' and it was
    // less than 'timeout' seconds ago that we've gotten the token
    if(caching && seconds < timeout)
    {
      return true;
    }

    // Flush the authentication status
    p_info->AuthStatus  = HttpAuthStatusNotAuthenticated;
    p_info->AuthType    = HttpRequestAuthTypeNone;
    p_info->AccessToken = NULL;
    // Flush SSPI context
    m_context.dwLower   = 0;
    m_context.dwUpper   = 0;
    m_timestamp         = 0;

    // Remove primary token!
    if(m_token)
    {
      CloseHandle(m_token);
      m_token = NULL;
    }
  }
  // Not authenticated (any more...)
  return false;
}

bool
Request::CheckBasicAuthentication(PHTTP_REQUEST_AUTH_INFO p_info,CString p_payload)
{
  // Prepare decoding the base64 payload
  CodeBase64 base;
  int len = (int) base.Ascii_length(p_payload.GetLength());
  CString decoded;
  char* dest = decoded.GetBufferSetLength(len);

  // Decrypt into a string
  base.Decrypt((const unsigned char*)p_payload.GetString(),p_payload.GetLength(),(unsigned char*)dest);
  decoded.ReleaseBuffer();
  
  // Split into user and password
  CString user;
  CString domain;
  CString password;
  SplitString(decoded,user,password,':');

  // Try to find a domain name and split it off
  int pos = user.Find('\\');
  if(pos > 0)
  {
    // This is a "domain\user" string 
    domain = user.Left(pos);
    user   = user.Mid(pos+1);
  }

  // Try to login for this client
  HANDLE token = 0;
  if(LogonUser(user,domain,password,LOGON32_LOGON_BATCH,LOGON32_PROVIDER_WINNT40,&token))
  {
    p_info->AuthStatus  = HttpAuthStatusSuccess;
    p_info->AccessToken = token;
    m_token     = token;
    m_timestamp = clock();
    RevertToSelf();
  }
  else
  {
    p_info->AuthStatus = HttpAuthStatusFailure;
  }
  return false;
}

bool
Request::CheckAuthenticationProvider(PHTTP_REQUEST_AUTH_INFO p_info,CString p_payload,CString p_provider)
{
  // Prepare decoding the base64 payload
  CodeBase64 base;
  int len = (int)base.Ascii_length(p_payload.GetLength());

  // Decode in a buffer
  unsigned char* buffer = new unsigned char[len + 1];
  // Decrypt into a string
  base.Decrypt((const unsigned char*)p_payload.GetString(),p_payload.GetLength(),buffer);

  bool       continuation = false;
  CredHandle credentials;
  TimeStamp  lifetime;
  ULONG      contextAttributes = 0;
  ZeroMemory(&credentials,sizeof(CredHandle));
  ZeroMemory(&lifetime,sizeof(TimeStamp));
  DWORD      maxTokenLength = GetProviderMaxTokenLength(p_provider);

  SECURITY_STATUS ss = AcquireCredentialsHandle(NULL,(LPSTR)p_provider.GetString(),SECPKG_CRED_INBOUND,NULL,NULL,NULL,NULL,&credentials,&lifetime);
  if(ss >= 0)
  {
    TimeStamp         lifetime;
    SecBufferDesc     OutBuffDesc;
    SecBuffer         OutSecBuff;
    SecBufferDesc     InBuffDesc;
    SecBuffer         InSecBuff;
    ULONG             Attribs = 0;
    PBYTE             pOut    = new BYTE[maxTokenLength + 1];
    DWORD             cbOut   = maxTokenLength;

    //  Prepare input buffers.
    InBuffDesc.ulVersion = 0;
    InBuffDesc.cBuffers  = 1;
    InBuffDesc.pBuffers  = &InSecBuff;

    InSecBuff.cbBuffer   = len;
    InSecBuff.BufferType = SECBUFFER_TOKEN;
    InSecBuff.pvBuffer   = buffer;

    //  Prepare output buffers.
    OutBuffDesc.ulVersion = 0;
    OutBuffDesc.cBuffers  = 1;
    OutBuffDesc.pBuffers  = &OutSecBuff;

    OutSecBuff.cbBuffer   = cbOut;
    OutSecBuff.BufferType = SECBUFFER_TOKEN;
    OutSecBuff.pvBuffer   = pOut;

    // See if we are continuing on a security conversation
    bool contextNew = m_context.dwLower == 0 && m_context.dwUpper == 0;

    // Work on the buffer. Accept it or not
    ss = AcceptSecurityContext(&credentials
                              ,contextNew ? NULL : &m_context
                              ,&InBuffDesc
                              ,contextAttributes
                              ,SECURITY_NATIVE_DREP
                              ,&m_context
                              ,&OutBuffDesc
                              ,&contextAttributes
                              ,&lifetime);
    if(ss >= 0)
    {
      // See if the return code has the SECURITY in the FACILITY position
      if(((ss >> 16) & 0xF) == FACILITY_SECURITY)
      {
        // Continuation of the conversation
        len = (int) base.B64_length(OutSecBuff.cbBuffer);
        unsigned char* challenge = new unsigned char[len + 1];
        base.Encrypt((unsigned char*)pOut,OutSecBuff.cbBuffer,challenge);
        challenge[len] = 0;

        // Send 401 with the correct provider challenge
        m_challenge = p_provider + " " + challenge;
        delete [] challenge;
        continuation = true;
      }
      else
      {
        // Possibly completion needed for the provider?
        if((SEC_I_COMPLETE_NEEDED == ss) || (SEC_I_COMPLETE_AND_CONTINUE == ss))
        {
          ss = CompleteAuthToken(&m_context, &OutBuffDesc);
        }
        // OK: We must be able to retrieve the impersonation token
        if(ss >= 0)
        {
          SecPkgContext_AccessToken token;
          token.AccessToken = NULL;
          ss = QueryContextAttributes(&m_context,SECPKG_ATTR_ACCESS_TOKEN,&token);
          if(ss >= 0)
          {
            m_token             = (HANDLE)token.AccessToken;
            p_info->AccessToken = (HANDLE)token.AccessToken;
            p_info->AuthStatus  = HttpAuthStatusSuccess;
            m_timestamp = clock();
          }
        }
      }
    }
    // And free the handle again
    FreeCredentialsHandle(&credentials);
    delete[] pOut;
  }
  // Free the buffer again
  delete[] buffer;


  // See if it did work out!
  if(!continuation && p_info->AccessToken == NULL)
  {
    p_info->AuthStatus = HttpAuthStatusFailure;
  }

  // Trigger the drivers 401 response if needed
  return continuation;
}

// Getting the maximum security token buffer length
// of the connected SSPI provider package
DWORD
Request::GetProviderMaxTokenLength(CString p_provider)
{
  PSecPkgInfo pkgInfo = nullptr;

  SECURITY_STATUS ss = QuerySecurityPackageInfo((LPSTR)p_provider.GetString(),&pkgInfo);
  if(ss == SEC_E_OK && pkgInfo)
  {
    DWORD size = pkgInfo->cbMaxToken;
    FreeContextBuffer(pkgInfo);
    return size;
  }
  // Return something on the safe side.
  return SSPI_BUFFER_DEFAULT;
}

// Create the general HTTP response status line to the output buffer
void
Request::AddResponseLine(CString& p_buffer,PHTTP_RESPONSE p_response)
{
  // Send the initial response line of the server
  p_buffer.Format("HTTP/%u.%u %d %s\r\n",p_response->Version.MajorVersion
                                        ,p_response->Version.MinorVersion
                                        ,p_response->StatusCode
                                        ,p_response->pReason);
}

// Add all known response headers to the output buffer
void
Request::AddAllKnownResponseHeaders(CString& p_buffer,PHTTP_KNOWN_HEADER p_headers)
{
  for(int index = 0;index < HttpHeaderResponseMaximum;++index)
  {
    if(p_headers[index].pRawValue && strlen(p_headers[index].pRawValue) > 0)
    {
      const char* name = index < HttpHeaderAcceptRanges ? all_headers[index] : all_responses[index - HttpHeaderAcceptRanges];

      p_buffer.AppendFormat("%s: %s\r\n",name,p_headers[index].pRawValue);
    }
  }
}

// Add all 'unknown' response headers to the output buffer
void
Request::AddAllUnknownResponseHeaders(CString& p_buffer,PHTTP_UNKNOWN_HEADER p_headers,int p_count)
{
  for(int index = 0; index < p_count; ++index)
  {
    p_buffer.AppendFormat("%s: %s\r\n",p_headers[index].pName,p_headers[index].pRawValue);
  }
}

// Sending just one (1) entity chunk from a group of chunks
// Continuation of "SendEntityChunks"
int
Request::SendEntityChunk(PHTTP_DATA_CHUNK p_chunk,PULONG p_bytes)
{
  switch(p_chunk->DataChunkType)
  {
    case HttpDataChunkFromMemory:         return SendEntityChunkFromMemory    (p_chunk,p_bytes);
    case HttpDataChunkFromFileHandle:     return SendEntityChunkFromFile      (p_chunk,p_bytes);
    case HttpDataChunkFromFragmentCache:  return SendEntityChunkFromFragment  (p_chunk,p_bytes);
    case HttpDataChunkFromFragmentCacheEx:return SendEntityChunkFromFragmentEx(p_chunk,p_bytes);
    default:                              // Log the error
                                          LogError("Wrong data chunk type for connection: %s",m_request.pRawUrl);
                                          return ERROR_INVALID_PARAMETER;
  }
}

// Write one (1) memory chunk from buffer to the socket
int
Request::SendEntityChunkFromMemory(PHTTP_DATA_CHUNK p_chunk,PULONG p_bytes)
{
  ULONG written = 0;
  int result = WriteBuffer(p_chunk->FromMemory.pBuffer,p_chunk->FromMemory.BufferLength,&written);
  if (result == NO_ERROR)
  {
    *p_bytes += written;
  }
  return result;
}

// Sending one (1) file from opened file handle in the chunk to the socket
// int
// Request::SendEntityChunkFromFile(PHTTP_DATA_CHUNK p_chunk,PULONG p_bytes)
// {
//   if(m_listener->GetSecureMode())
//   {
//     return SendFileByMemoryBlocks(p_chunk, p_bytes);
//   }
//   else
//   {
//     return SendFileByTransmitFunction(p_chunk, p_bytes);
//   }
// }
// 
// int
// Request::SendFileByTransmitFunction(PHTTP_DATA_CHUNK p_chunk,PULONG p_bytes)
// {
//   SOCKET actual = reinterpret_cast<PlainSocket*>(m_socket)->GetActualSocket();
//   PointTransmitFile transmit = m_queue->GetTransmitFile(actual);
// 
//   DWORD high = 0;
//   DWORD size = GetFileSize(p_chunk->FromFileHandle.FileHandle,&high);
// 
//   // But we restrict on this much if possible
//   if(p_chunk->FromFileHandle.ByteRange.Length.LowPart > 0 && 
//      p_chunk->FromFileHandle.ByteRange.Length.LowPart < size)
//   {
//     size = p_chunk->FromFileHandle.ByteRange.Length.LowPart;
//   }
// 
//   if(transmit)
//   {
//     if((*transmit)(actual
//                   ,p_chunk->FromFileHandle.FileHandle
//                   ,(DWORD)p_chunk->FromFileHandle.ByteRange.Length.LowPart
//                   ,0
//                   ,NULL
//                   ,NULL
//                   ,TF_DISCONNECT) == FALSE)
//     {
//       int error = WSAGetLastError();
//       LogError("Error while sending a file: %d", error);
//       return error;
//     }
//   }
//   else
//   {
//     LogError("Could not send a file. Injected 'TransmitFile' functionality not found");
//     return ERROR_CONNECTION_ABORTED;
//   }
// 
//   // Record the fact that we have written this much
//   m_bytesWritten += size;
//   *p_bytes       += size;
// 
//   return NO_ERROR;
// }

int
Request::SendEntityChunkFromFile(PHTTP_DATA_CHUNK p_chunk,PULONG p_bytes)
{
  char buffer[MESSAGE_BUFFER_LENGTH];

  // Getting the total file size
  DWORD fhigh = 0;
  DWORD fsize = GetFileSize(p_chunk->FromFileHandle.FileHandle,&fhigh);
  ULONGLONG totalSize = ((ULONGLONG)fhigh << 32) | (ULONGLONG)fsize;

  // Begin and length of the part of the file to send
  ULONGLONG begin  = p_chunk->FromFileHandle.ByteRange.StartingOffset.QuadPart;
  ULONGLONG length = p_chunk->FromFileHandle.ByteRange.Length.QuadPart;

  // If no file length given, use the whole file
  if(length == 0L)
  {
    length = totalSize;
  }
  else if(begin + length > totalSize)
  {
    length = totalSize - begin;
  }

  // Set file pointer to the beginning of the sending part
  LONG beginHigh = (DWORD)(begin >> 32);
  if(SetFilePointer(p_chunk->FromFileHandle.FileHandle,begin & 0xFFFFFFFF,&beginHigh,FILE_BEGIN) == INVALID_SET_FILE_POINTER)
  {
    return ERROR_HANDLE_EOF;
  }

  ULONGLONG size = 0L;

  // Sending loop: read buffer, send buffer
  while(size < length)
  {
    // Calculate next block size
    ULONG blocksize = MESSAGE_BUFFER_LENGTH;
    if((ULONG)(length - size) < MESSAGE_BUFFER_LENGTH)
    {
      blocksize = (ULONG)(length - size);
    }

    // Read next block size
    ULONG didread = 0L;
    if(ReadFile(p_chunk->FromFileHandle.FileHandle,buffer,blocksize,&didread, nullptr) == FALSE)
    {
      return ERROR_FILE_CORRUPT;
    }

    // Send next block size
    ULONG written = 0;
    int result = WriteBuffer(buffer,didread,&written);
    if (result != NO_ERROR)
    {
      return result;
    }

    // Keep record of how much we already did
    size += written;
  }

  // Tell the result
  if(p_bytes)
  {
    *p_bytes = (ULONG)size;
  }
  return NO_ERROR;
}

// Sending one (1) fragment from the general fragment cache
// This is done by relaying the buffer to sending a chunk from memory.
int
Request::SendEntityChunkFromFragment(PHTTP_DATA_CHUNK p_chunk,PULONG p_bytes)
{
  USES_CONVERSION;
  CString prefix(W2A(p_chunk->FromFragmentCache.pFragmentName));

  // Find memory chunk
  PHTTP_DATA_CHUNK chunk = m_queue->FindFragment(prefix);
  if(chunk)
  {
    return SendEntityChunkFromMemory(chunk,p_bytes);
  }
  // Log error : Chunk not found
  LogError("Data chunk [%s] not found",prefix);
  return ERROR_INVALID_PARAMETER;
}

// Sending one (1) fragment from the general fragment cache
// while defining a specific part of the fragment (begin / length)
// This is done by relaying the buffer to sending a chunk from memory.
int
Request::SendEntityChunkFromFragmentEx(PHTTP_DATA_CHUNK p_chunk,PULONG p_bytes)
{
  USES_CONVERSION;
  CString prefix(W2A(p_chunk->FromFragmentCacheEx.pFragmentName));

  // Find memory chunk
  PHTTP_DATA_CHUNK chunk = m_queue->FindFragment(prefix);
  if (chunk)
  {
    DWORD start = p_chunk->FromFragmentCacheEx.ByteRange.StartingOffset.LowPart;
    DWORD end   = p_chunk->FromFragmentCacheEx.ByteRange.Length.LowPart;

    if(start < chunk->FromMemory.BufferLength && 
       end   < chunk->FromMemory.BufferLength &&
       start < end)
    {
      ULONG written = 0;
      int result = WriteBuffer((PVOID)&(((char*)chunk->FromMemory.pBuffer)[start]),end - start,&written);
      if (result == NO_ERROR)
      {
        *p_bytes += written;
      }
      return result;
    }
    LogError("Data chunk [%s] out of range",prefix);
    return ERROR_RANGE_NOT_FOUND;
  }
  // Log error : Chunk not found
  LogError("Data chunk [%s] not found",prefix);
  return ERROR_INVALID_PARAMETER;
}

const char* weekday_short[7] =
{
  "Sun","Mon","Tue","Wed","Thu","Fri","Sat"
};

const char* month[12] =
{
  "Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"
};


// Print HTTP time in RFC 1123 format (Preferred standard)
// as in "Tue, 8 Dec 2015 21:26:32 GMT"
CString
Request::HTTPSystemTime()
{
  CString    time;
  SYSTEMTIME systemtime;
  GetSystemTime(&systemtime);

  time.Format("%s, %02d %s %04d %2.2d:%2.2d:%2.2d GMT"
             ,weekday_short[systemtime.wDayOfWeek]
             ,systemtime.wDay
             ,month[systemtime.wMonth - 1]
             ,systemtime.wYear
             ,systemtime.wHour
             ,systemtime.wMinute
             ,systemtime.wSecond);

  return time;
}

//////////////////////////////////////////////////////////////////////////
//
// LOW LEVEL READ AND WRITE
//
//////////////////////////////////////////////////////////////////////////

// Low level read from socket
int
Request::ReadBuffer(PVOID p_buffer,ULONG p_size,PULONG p_bytes)
{
  int result = m_socket->RecvPartial(p_buffer,p_size);
  if (result == SOCKET_ERROR)
  {
    // Log the error
    int error = WSAGetLastError();
    LogError("Reading from connection: %s Error: %d",m_request.pRawUrl,error);
    return error;
  }

  // Keep track of bytes read in
  m_bytesRead += result;
  *p_bytes     = result;

  return NO_ERROR;
}

// Low level write to socket
int
Request::WriteBuffer(PVOID p_buffer,ULONG p_size,PULONG p_bytes)
{
  int result = m_socket->SendPartial(p_buffer,p_size);
  if (result == SOCKET_ERROR)
  {
    // Log the error
    int error = WSAGetLastError();
    LogError("Writing to connection: %s Error: %d", m_request.pRawUrl,error);
    return error;
  }

  // Keep track of bytes written
  if (result > 0)
  {
    m_bytesWritten += result;
    *p_bytes       += result;
  }

  // Keep track of service status
  if((m_status == RQ_WRITING) && m_bytesWritten >= m_contentLength)
  {
    m_status = RQ_SERVICED;
  }

  return NO_ERROR;;
}
