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
        ,m_secure(false)
        ,m_url(nullptr)
{
  // HTTP_REQUEST_V1 && V2
  ZeroMemory(&m_request,sizeof(HTTP_REQUEST_V2));

  // Setting other members
  m_status        = RQ_CREATED;
  m_bytesRead     = 0L;
  m_contentLength = 0L;

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
  try
  {
    InitiateSSL();
    ReceiveHeaders();
    CheckAuthentication();
    m_queue->AddIncomingRequest(this);
  }
  catch(int error)
  {
    if(error == HTTP_STATUS_DENIED)
    {
      ReplyClientError(error,"Not authenticated");
    }
    else if(error == HTTP_STATUS_SERVICE_UNAVAIL)
    {
      ReplyServerError(error,"Service temporarily unavailable");
    }
    else
    {
      LogError("Illegal HTTP client call. Error: %d", error);
      ReplyClientError();
      delete this;
    }
  }
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
  CString buffer;

  // Reset bytes written
  m_bytesWritten = 0;

  // Create the response line
  AddResponseLine(buffer,p_response);

  // Add all response headers
  AddAllKnownResponseHeaders  (buffer,p_response->Headers.KnownHeaders);
  AddAllUnknownResponseHeaders(buffer,p_response->Headers.pUnknownHeaders
                                     ,p_response->Headers.UnknownHeaderCount);
  
  // Line between headers and body
  buffer += "\r\n";

  // Perform one write of all header lines in one go!
  int result = WriteBuffer((PVOID)buffer.GetString(),buffer.GetLength());
  if(result == NO_ERROR)
  {
    *p_bytes = m_bytesWritten;
  }

  // Reset byte pointers
  m_bytesWritten  = 0;
  m_contentLength = 0;
  if(p_response->Headers.KnownHeaders[HttpHeaderContentLength].pRawValue)
  {
    m_contentLength = atoi(p_response->Headers.KnownHeaders[HttpHeaderContentLength].pRawValue);
  }

  // Also send our body right away
  if(p_response->EntityChunkCount)
  {
    ULONG bytes = 0;
    int  result = SendEntityChunks(p_response->pEntityChunks,p_response->EntityChunkCount,&bytes);
    if (result == NO_ERROR)
    {
      *p_bytes += bytes;
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
//
void
Request::Reset()
{
  // HTTP_REQUEST_V1

  // VERB & URL
  if(m_request.pUnknownVerb)       free((void*)m_request.pUnknownVerb);
  if(m_request.pRawUrl)            free((void*)m_request.pRawUrl);
  if(m_request.CookedUrl.pFullUrl) free((void*)m_request.CookedUrl.pFullUrl);

  // Unknown headers/trailers
  if(m_request.Headers.pUnknownHeaders)
  {
    for(int ind = 0;ind < m_request.Headers.UnknownHeaderCount; ++ind)
    {
      free((void*)m_request.Headers.pUnknownHeaders[ind].pName);
      free((void*)m_request.Headers.pUnknownHeaders[ind].pRawValue);
    }
    free(m_request.Headers.pUnknownHeaders);
  }
  if(m_request.Headers.pTrailers)
  {
    for(int ind = 0;ind < m_request.Headers.TrailerCount;++ind)
    {
      free((void*)m_request.Headers.pTrailers[ind].pName);
      free((void*)m_request.Headers.pTrailers[ind].pRawValue);
    }
  }

  // Known headers
  for(int ind = 0;ind < HttpHeaderRequestMaximum; ++ind)
  {
    if(m_request.Headers.KnownHeaders[ind].pRawValue)
    {
      free((void*)m_request.Headers.KnownHeaders[ind].pRawValue);
    }
  }

  // Addresses
  if(m_request.Address.pLocalAddress)
  {
    free((void*)m_request.Address.pLocalAddress);
  }
  if(m_request.Address.pRemoteAddress)
  {
    free((void*)m_request.Address.pRemoteAddress);
  }

  // Entity chunks
  if(m_request.pEntityChunks)
  {
    free(m_request.pEntityChunks);
    // Data buffers are allocated by the calling program!!
    // They are never free-ed here!
  }

  // SSL Info and Certificate
  if(m_request.pSslInfo)
  {
    free((void*)m_request.pSslInfo->pServerCertIssuer);
    free((void*)m_request.pSslInfo->pServerCertSubject);

    if(m_request.pSslInfo->pClientCertInfo)
    {
      free((void*)m_request.pSslInfo->pClientCertInfo->pCertEncoded);
    }

    free(m_request.pSslInfo);
  }

  // HTTP_REQUEST_V2
  // Can have 1 (authentication) or 2 (SSL) records
  if(m_request.pRequestInfo)
  {
    if(m_request.RequestInfoCount >= 1)
    {
      PHTTP_REQUEST_AUTH_INFO info = (PHTTP_REQUEST_AUTH_INFO)m_request.pRequestInfo[0].pInfo;
//       if(info->AccessToken)
//       {
//         CloseHandle(info->AccessToken);
//       }
      free(info);
    }
    if(m_request.RequestInfoCount >= 2)
    {
      free(m_request.pRequestInfo[1].pInfo);
    }
    free(m_request.pRequestInfo);
  }
  m_request.pRequestInfo     = nullptr;
  m_request.RequestInfoCount = 0;

  // Free the headers line
  FreeInitialBuffer();
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

void
Request::InitiateSSL()
{
  if(m_secure)
  {
    // Go do the SSL handshake
    SecureServerSocket* secure = reinterpret_cast<SecureServerSocket*>(m_socket);
    if(secure->InitializeSSL() != S_OK)
    {
      throw ERROR_INTERNET_SECURITY_CHANNEL_ERROR;
    }
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

  // Reset number of bytes read. We will now go read the body
  m_bytesRead = 0;
}

// Grab the first available message part just under the optimal buffer length
// so we can parse off the initial headers of the message
void
Request::ReadInitialMessage()
{
  m_initialBuffer = (char*) malloc(MESSAGE_BUFFER_LENGTH + 1);
  m_initialLength = 0;

  int length = m_socket->RecvPartial(m_initialBuffer,MESSAGE_BUFFER_LENGTH);
  if (length > 0)
  {
    m_initialBuffer[length] = 0;
    m_initialLength = length;
    m_bytesRead += length;
    return;
  }
  throw ERROR_HANDLE_EOF;
}

// Find the next line int he initial buffers upto the "\r\n"
// Mark that the HTTP IETF RFC clearly states that all header
// lines must and in a <CR><LF> sequence!!
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
 ,"Www-Authenticate"      //  HttpHeaderWwwAuthenticate       = 29,   // response-header [section 6.2]
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

// Reply with a client error in the range 400 - 499
void
Request::ReplyClientError(int p_error, CString p_errorText)
{
  CString header;
  header.Format("HTTP/1.1 %d %s\r\n",p_error,p_errorText);

  CString body;
  body.Format(http_client_error,p_error,p_errorText);

  header.AppendFormat("Content-Length: %d\r\n",body.GetLength());
  header += "\r\n";
  header += body;

  // Write out to the client:
  WriteBuffer((PVOID)header.GetString(),header.GetLength());
  // And be done with this request;
  CloseRequest();
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
  CString header;
  header.Format("HTTP/1.1 %d %s\r\n", p_error, p_errorText);

  CString body;
  body.Format(http_server_error, p_error, p_errorText);

  header.AppendFormat("Content-Length: %d\r\n", body.GetLength());
  header += "\r\n";
  header += body;

  // Write out to the client:
  WriteBuffer((PVOID)header.GetString(), header.GetLength());
  // And be done with this request;
  CloseRequest();
}


// General server error 500 (Server not capable of doing this)
void
Request::ReplyServerError()
{
  ReplyServerError(HTTP_STATUS_SERVER_ERROR,"General server error");
}

void
Request::ReplyAuthorizationRequired()
{
  CString body;
  body.Format(http_client_error,401,"Not authenticated");

  CString line("HTTP/1.1 401 Access denied\r\n");
  line += "Content-Type: application/octet-stream\r\n";
  line.AppendFormat("Content-Length: %d\r\n", body.GetLength());
  line.AppendFormat("WWW-Authenticate: %s\r\n");
  line += "\r\n";
  line += body;

  // Write out to the client:
  WriteBuffer((PVOID)line.GetString(), line.GetLength());
  // And be done with this request;
  CloseRequest();
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
void 
Request::CheckAuthentication()
{
  ULONG scheme = m_url->m_urlGroup->GetAuthenticationScheme();

  // No authentication requested for this URL. So we do nothing
  if(scheme == 0)
  {
    return;
  }

  // This is our authorization
  CString authorization(m_request.Headers.KnownHeaders[HttpHeaderAuthorization].pRawValue);

  // Create AUTH_INFO object with defaults
  USHORT size = ++m_request.RequestInfoCount;
  m_request.pRequestInfo = (PHTTP_REQUEST_INFO) realloc(m_request.pRequestInfo,size * sizeof(HTTP_REQUEST_INFO));

  m_request.pRequestInfo[size-1].InfoType   = HttpRequestInfoTypeAuth;
  m_request.pRequestInfo[size-1].InfoLength = sizeof(HTTP_REQUEST_AUTH_INFO);
  m_request.pRequestInfo[size-1].pInfo      = (PHTTP_REQUEST_AUTH_INFO) calloc(1,sizeof(HTTP_REQUEST_AUTH_INFO));

  PHTTP_REQUEST_AUTH_INFO info = (PHTTP_REQUEST_AUTH_INFO) m_request.pRequestInfo[size-1].pInfo;
  info->AuthStatus = HttpAuthStatusNotAuthenticated;
  info->AuthType   = HttpRequestAuthTypeNone;


  // Find method and payload of that method
  CString method;
  CString payload;
  SplitString(authorization,method,payload,' ');

       if(method.CompareNoCase("Basic")     == 0)  info->AuthType = HttpRequestAuthTypeBasic;
  else if(method.CompareNoCase("NTLM")      == 0)  info->AuthType = HttpRequestAuthTypeNTLM;
  else if(method.CompareNoCase("Negotiate") == 0)  info->AuthType = HttpRequestAuthTypeNegotiate;
  else if(method.CompareNoCase("Digest")    == 0)  info->AuthType = HttpRequestAuthTypeDigest;
  else if(method.CompareNoCase("Kerberos")  == 0)  info->AuthType = HttpRequestAuthTypeKerberos;
  else return; // ???  throw HTTP_STATUS_DENIED;

  if(info->AuthType = HttpRequestAuthTypeBasic)
  {
    // Base64 encryption of the "username:password" combination
    CheckBasicAuthentication(info,payload);
  }
  else
  {
    // All done through "secur32.dll"
    CheckAuthenticationProvider(info,payload);
  }
}

void
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
    RevertToSelf();
  }
  else
  {
    p_info->AuthStatus = HttpAuthStatusFailure;
  }
}

void
Request::CheckAuthenticationProvider(PHTTP_REQUEST_AUTH_INFO p_info, CString p_payload)
{
  // try to authenticate only this need to be filled in
  p_info->AccessToken = 0L;
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
  int result = WriteBuffer(p_chunk->FromMemory.pBuffer,p_chunk->FromMemory.BufferLength);
  if (result == NO_ERROR)
  {
    p_bytes += result;
  }
  return result;
}

// Sending one (1) file from opened file handle in the chunk to the socket
int
Request::SendEntityChunkFromFile(PHTTP_DATA_CHUNK p_chunk,PULONG p_bytes)
{
  if(m_listener->GetSecureMode())
  {
    return SendFileByMemoryBlocks(p_chunk, p_bytes);
  }
  else
  {
    return SendFileByTransmitFunction(p_chunk, p_bytes);
  }
}

int
Request::SendFileByTransmitFunction(PHTTP_DATA_CHUNK p_chunk,PULONG p_bytes)
{
  SOCKET actual = reinterpret_cast<PlainSocket*>(m_socket)->GetActualSocket();
  PointTransmitFile transmit = m_queue->GetTransmitFile(actual);

  DWORD high = 0;
  DWORD size = GetFileSize(p_chunk->FromFileHandle.FileHandle,&high);

  // But we restrict on this much if possible
  if(p_chunk->FromFileHandle.ByteRange.Length.LowPart > 0 && 
     p_chunk->FromFileHandle.ByteRange.Length.LowPart < size)
  {
    size = p_chunk->FromFileHandle.ByteRange.Length.LowPart;
  }

  if(transmit)
  {
    if((*transmit)(actual
                  ,p_chunk->FromFileHandle.FileHandle
                  ,(DWORD)p_chunk->FromFileHandle.ByteRange.Length.LowPart
                  ,0
                  ,NULL
                  ,NULL
                  ,TF_DISCONNECT) == FALSE)
    {
      int error = WSAGetLastError();
      LogError("Error while sending a file: %d", error);
      return error;
    }
  }
  else
  {
    LogError("Could not send a file. Injected 'TransmitFile' functionallity not found");
    return ERROR_CONNECTION_ABORTED;
  }

  // Record the fact that we have written this much
  m_bytesWritten += size;

  return NO_ERROR;
}

int
Request::SendFileByMemoryBlocks(PHTTP_DATA_CHUNK p_chunk,PULONG p_bytes)
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
    int result = WriteBuffer(buffer,didread);
    if (result != NO_ERROR)
    {
      return result;
    }

    // Keep record of how much we already did
    size += didread;
  }

  // Tell the result
  if (p_bytes)
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
      int result = WriteBuffer((PVOID)&(((char*)chunk->FromMemory.pBuffer)[start]),end - start);
      if (result == NO_ERROR)
      {
        p_bytes += result;
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
Request::WriteBuffer(PVOID p_buffer, ULONG p_size)
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
  }

  return NO_ERROR;;
}
