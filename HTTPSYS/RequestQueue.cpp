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
#include "RequestQueue.h"
#include "UrlGroup.h"
#include <malloc.h>
#include <algorithm>
#include <winhttp.h>
#include <algorithm>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// These are all request queues
RequestQueues g_requestQueues;

// CTOR
RequestQueue::RequestQueue(CString p_name)
{
  m_name = p_name;
  InitializeCriticalSection(&m_lock);
  CreateEvent();
}

RequestQueue::~RequestQueue()
{
  StopAllListeners();
  DeleteAllFragments();
  DeleteCriticalSection(&m_lock);
  CloseEvent();
  CloseQueueHandle();
}

HANDLE
RequestQueue::CreateHandle()
{
  if(m_handle)
  {
    return NULL;
  }
  CString tempFilename;
  tempFilename.GetEnvironmentVariable("WINDIR");
  tempFilename += "\\TEMP\\RequestQueue_";
  tempFilename += m_name;

  m_handle = CreateFile(tempFilename
                       ,GENERIC_READ|GENERIC_WRITE
                       ,FILE_SHARE_READ|FILE_SHARE_WRITE
                       ,NULL  // Security
                       ,OPEN_ALWAYS
                       ,FILE_ATTRIBUTE_NORMAL| FILE_FLAG_RANDOM_ACCESS | FILE_FLAG_OVERLAPPED | FILE_FLAG_DELETE_ON_CLOSE
                       ,NULL);
  if(m_handle == INVALID_HANDLE_VALUE)
  {
    return NULL;
  }
  return m_handle;
}

// Add an URL-Group to the queue
void
RequestQueue::AddURLGroup(UrlGroup* p_group)
{
  AutoCritSec lock(&m_lock);

  m_groups.push_back(p_group);
}

// Forget about an URL group
void
RequestQueue::RemoveURLGroup(UrlGroup* p_group)
{
  AutoCritSec lock(&m_lock);

  UrlGroups::iterator it = m_groups.begin();
  while(it != m_groups.end())
  {
    if(*it == p_group)
    {
      it = m_groups.erase(it);
    }
    else ++it;
  }
}

// Enable the request queue (or disable it)
bool
RequestQueue::SetEnabledState(HTTP_ENABLED_STATE p_state)
{
  if (p_state == HttpEnabledStateActive || p_state == HttpEnabledStateInactive)
  {
    m_state = p_state;
    return true;
  }
  return false;
}

// Setting the queue length
bool
RequestQueue::SetQueueLength(ULONG p_length)
{
  if (p_length >= HTTP_REQUEST_QUEUE_MINIMUM && p_length <= HTTP_REQUEST_QUEUE_MAXIMUM)
  {
    m_queueLength = p_length;
    return true;
  }
  return false;
}

// Setting the verbosity of the 503 error
bool
RequestQueue::SetVerbosity(HTTP_503_RESPONSE_VERBOSITY p_verbosity)
{
  if(p_verbosity == Http503ResponseVerbosityBasic   ||
     p_verbosity == Http503ResponseVerbosityLimited ||
     p_verbosity == Http503ResponseVerbosityFull)
  {
    m_verbosity = p_verbosity;
    return true;
  }
  return false;
}

// Starting a new listener, initialize it and LISTEN
ULONG
RequestQueue::StartListener(USHORT p_port,URL* p_url,USHORT p_timeout)
{
  AutoCritSec lock(&m_lock);

  Listeners::iterator it = m_listeners.find(p_port);
  if(it == m_listeners.end())
  {
    Listener* listener = new Listener(this,p_port,p_url,p_timeout);
    m_listeners.insert(std::make_pair(p_port,listener));

    if(listener->Initialize(p_port) == NoError)
    {
      listener->StartListener();
      return NO_ERROR;
    }
    else
    {
      // Log the error
      return ERROR_BAD_COMMAND;
    }
  }
  return ERROR_ALREADY_EXISTS;
}

// Stop a listener, but only if no more URLs are relying on it
ULONG
RequestQueue::StopListener(USHORT p_port)
{
  AutoCritSec lock(&m_lock);

  if(NumberOfPorts(p_port))
  {
    // At least one URL still relies on this port
    // So we cannot remove the listener
    return ERROR_BAD_COMMAND;
  }

  Listeners::iterator it = m_listeners.find(p_port);
  if(it != m_listeners.end())
  {
    // Stop it!
    it->second->StopListener();
    // Remove the listener object
    delete it->second;
    m_listeners.erase(it);
  }
  return NO_ERROR;
}

// Finding a listener for a port number
Listener* 
RequestQueue::FindListener(USHORT p_port)
{
  Listeners::iterator it = m_listeners.find(p_port);
  if (it != m_listeners.end())
  {
    return it->second;
  }
  return nullptr;
}

// Add a new request to the incoming queue
void
RequestQueue::AddIncomingRequest(Request* p_request)
{
  AutoCritSec lock(&m_lock);

  if(m_state == HttpEnabledStateInactive)
  {
    // TODO: Report inactive server receiving calls
    return;
  }

  // Headers now fully received
  p_request->SetStatus(RQ_RECEIVED);

  // See if there is space in the queue
  if(m_incoming.size() < m_queueLength)
  {
    m_incoming.push_back(p_request);
    // Wake up waiters for the request queue
    SetEvent(m_event);
  }
  else
  {
    // Report queue overflow = Server overflow
    throw HTTP_STATUS_SERVICE_UNAVAIL;
  }
}

// Put the request back in the servicing queue
bool
RequestQueue::ResetToServicing(Request* p_request)
{
  AutoCritSec lock(&m_lock);

  Requests::iterator it = std::find(m_servicing.begin(),m_servicing.end(),p_request);
  if(it != m_servicing.end())
  {
    m_servicing.erase(it);
    return true;
  }
  return false;
}

// Our workhorse. Implementations call this to get the next HTTP request
ULONG
RequestQueue::GetNextRequest(HTTP_REQUEST_ID  RequestId
                            ,ULONG            Flags
                            ,PHTTP_REQUEST    RequestBuffer
                            ,ULONG            RequestBufferLength
                            ,PULONG           Bytes)
{
  ULONG result = NO_ERROR;
  Request* request = nullptr;

  if(RequestId == 0)
  {
    // Get a new request from the incoming queue
    AutoCritSec lock(&m_lock);

    if(m_incoming.empty())
    {
      lock.Unlock();
      // Wait for event of incoming request
      DWORD result = WaitForSingleObject(m_event,INFINITE);
      lock.Relock();

      // Still no request in the queue, or event interrupted
      if(m_incoming.empty() || result == WAIT_ABANDONED || result == WAIT_FAILED)
      {
        return ERROR_HANDLE_EOF;
      }
    }

    // Move request to servicing queue
    request = m_incoming.front();
    m_incoming.pop_front();
    m_servicing.push_back(request);
    request->SetStatus(RQ_READING);

    // BitBlitting our request!
    memcpy_s(RequestBuffer,RequestBufferLength,request->GetV2Request(),sizeof(HTTP_REQUEST_V2));

    for(int index = 0;index < RequestBuffer->RequestInfoCount;++index)
    {
      if(RequestBuffer->pRequestInfo[index].InfoType == HttpRequestInfoTypeAuth)
      {
        PHTTP_REQUEST_AUTH_INFO info = (PHTTP_REQUEST_AUTH_INFO)RequestBuffer->pRequestInfo[index].pInfo;
        if(info->AccessToken)
        {
          // Taking a duplicate token
          HANDLE token = NULL;
          if(DuplicateTokenEx(request->GetAccessToken()
                             ,TOKEN_DUPLICATE | TOKEN_IMPERSONATE | TOKEN_ALL_ACCESS | TOKEN_READ | TOKEN_WRITE
                             ,NULL
                             ,SecurityImpersonation
                             ,TokenImpersonation
                             ,&token) == FALSE)
          {
            token = NULL;
          }
          // Set cloned token in the request buffer
          info->AccessToken = token;
        }
      }
    }

  }
  else
  {
    // Check that we are restarting this request
    if(RequestId != RequestBuffer->RequestId)
    {
      return ERROR_INVALID_PARAMETER;
    }
    // This is our request. Flags will be set!
    request = reinterpret_cast<Request*>(RequestId);
  }

  // Reading chunks
  if(Flags)
  {
    // Read a one chunk from the stream
    LONG length  = (LONG)RequestBufferLength - sizeof(HTTP_REQUEST_V2) - 16;
    PVOID buffer = (PUCHAR)RequestBuffer     + sizeof(HTTP_REQUEST_V2) + 16;

    // Buffer must at least be capable to receive one standard message
    // Otherwise the caller must use "HttpReceiveRequestEntityBody" !!
    if(length < MESSAGE_BUFFER_LENGTH)
    {
      return ERROR_MORE_DATA;
    }

    // Get the buffer
    int result = HttpReceiveRequestEntityBody(this,RequestId,Flags,buffer,length,Bytes,NULL);
    if(result == NO_ERROR)
    {
      // If no problem, set as first chunk in the request structure
      request->ReceiveChunk(buffer,length);
    }
  }

  // Reflect total bytes read
  if(Bytes)
  {
    *Bytes = request->GetBytes();
  }
  return result;
}

// Find best matching URL for each incoming request
// Uses the 'longest matching' algorithm
URL*
RequestQueue::FindLongestURL(USHORT p_port,CString p_abspath)
{
  URL* longesturl = nullptr;
  int  longest = 0;

  for(auto& group : m_groups)
  {
    int length = 0;
    URL* url = group->FindLongestURL(p_port,p_abspath,length);
    if(length > longest)
    {
      longest    = length;
      longesturl = url;
    }
    // Longest possible match found?
    if(length == p_abspath.GetLength())
    {
      break;
    }
  }
  // What we found
  return longesturl;
}

// Remove a request from the request queue after servicing
void
RequestQueue::RemoveRequest(Request* p_request)
{
  AutoCritSec lock(&m_lock);

  // Close socket of the request
  p_request->CloseRequest();

  // After servicing find the request in the servicing queue
  Requests::iterator it = std::find(m_servicing.begin(),m_servicing.end(),p_request);
  if(it != m_servicing.end())
  {
    delete p_request;
    m_servicing.erase(it);
    return;
  }

  // Just to be sure, also look in the incoming queue
  it = std::find(m_incoming.begin(),m_incoming.end(),p_request);
  if(it != m_incoming.end())
  {
    delete p_request;
    m_incoming.erase(it);
    return;
  }

  // Request was not found in any queue
  // Delete it all the while
  delete p_request;
}

// Add a fragment to the fragment cache
ULONG
RequestQueue::AddFragment(CString p_prefix, PHTTP_DATA_CHUNK p_chunk)
{
  AutoCritSec lock(&m_lock);

  p_prefix.MakeLower();
  Fragments::iterator it = m_fragments.find(p_prefix);
  if(it != m_fragments.end())
  {
    // Make a copy of the memory data chunk
    PHTTP_DATA_CHUNK chunk = (PHTTP_DATA_CHUNK) calloc(1,sizeof(HTTP_DATA_CHUNK));
    chunk->DataChunkType = HttpDataChunkFromMemory;
    ULONG length = p_chunk->FromMemory.BufferLength;
    chunk->FromMemory.BufferLength =   length;
    chunk->FromMemory.pBuffer = malloc(length + 1);
    memcpy (chunk->FromMemory.pBuffer,p_chunk->FromMemory.pBuffer,length);
    ((char*)chunk->FromMemory.pBuffer)[length] = 0;

    // Add the copied chunk into the cache
    m_fragments.insert(std::make_pair(p_prefix,chunk));
    return NO_ERROR;
  }
  // Duplicate fragment
  return ERROR_DUPLICATE_TAG;
}

// Finding a fragment in the fragment cache
PHTTP_DATA_CHUNK
RequestQueue::FindFragment(CString p_prefix)
{
  AutoCritSec lock(&m_lock);

  p_prefix.MakeLower();
  Fragments::iterator it = m_fragments.find(p_prefix);
  if(it != m_fragments.end())
  {
    return it->second;
  }
  // Fragment not found
  return nullptr;
}

// Flush (remove) a fragment from the cache
// Removes one fragment or all descendants of it.
ULONG
RequestQueue::FlushFragment(CString p_prefix,ULONG Flags)
{
  AutoCritSec lock(&m_lock);

  int erased = 0;
  p_prefix.MakeLower();

  // Try to erase exactly this prefix from the cache
  Fragments::iterator it = m_fragments.find(p_prefix);
  if(it != m_fragments.end())
  {
    PHTTP_DATA_CHUNK chunk = it->second;
    free(chunk->FromMemory.pBuffer);
    free(chunk);
    m_fragments.erase(it);
    ++erased;
  }

  // Also find fragments that are descendants of the prefix given
  if(Flags == HTTP_FLUSH_RESPONSE_FLAG_RECURSIVE)
  {
    Fragments::iterator fr = m_fragments.begin();
    while(fr != m_fragments.end())
    {
      if(p_prefix.Compare(fr->first.Left(p_prefix.GetLength())) == 0)
      {
        PHTTP_DATA_CHUNK chunk = fr->second;
        free(chunk->FromMemory.pBuffer);
        free(chunk);
        fr = m_fragments.erase(fr);
        ++erased;
      }
      else
      {
        ++fr;
      }
    }
  }

  // Return flushed or no fragments found
  return erased > 0 ? NO_ERROR : ERROR_NOT_FOUND;
}

// Return the 'TransmitFile' function for a socket
// This function caches the result, so that it get's called
// only one time (1) for the TCP/IP stack.
PointTransmitFile 
RequestQueue::GetTransmitFile(SOCKET p_socket)
{
  if(m_transmitFile == nullptr)
  {
    GUID  trans = WSAID_TRANSMITFILE;
    DWORD bytes = 0;
    int  result = WSAIoctl(p_socket
                          ,SIO_GET_EXTENSION_FUNCTION_POINTER
                          ,&trans
                          ,sizeof(GUID)
                          ,&m_transmitFile
                          ,sizeof(m_transmitFile)
                          ,&bytes
                          ,NULL,NULL);
    if(result != NO_ERROR)
    {
      result = WSAGetLastError();
      m_transmitFile = nullptr;
    }
  }
  return m_transmitFile;
}

// Signal all listeners to stop listening
void
RequestQueue::ClearIncomingWaiters()
{
  SetEvent(m_event);
  Sleep(50);
}

// Find out if a request is still in the servicing queue
// so we know whether still to destroy it.
bool
RequestQueue::RequestStillInService(Request* p_request)
{
  AutoCritSec lock(&m_lock);

  Requests::iterator it = std::find(m_servicing.begin(),m_servicing.end(),p_request);
  return it != m_servicing.end();
}

// Demand start
ULONG     
RequestQueue::RegisterDemandStart()
{
  if(m_start)
  {
    return ERROR_ALREADY_EXISTS;
  }
  m_start = ::CreateEvent(nullptr,FALSE,FALSE,nullptr);
  DWORD result = WaitForSingleObject(m_start,INFINITE);

  CloseHandle(m_start);
  m_start = NULL;

  return result == WAIT_OBJECT_0 ? NO_ERROR : ERROR_CONNECTION_ABORTED;
}

// Turn the application loose!
void
RequestQueue::DemandStart()
{
  if(m_start)
  {
    SetEvent(m_start);
  }
}

//////////////////////////////////////////////////////////////////////////
//
// PRIVATE
//
//////////////////////////////////////////////////////////////////////////

// Stopping all registered listeners
void
RequestQueue::StopAllListeners()
{
  for(auto& listener : m_listeners)
  {
    listener.second->StopListener();
    delete listener.second;
  }
  m_listeners.clear();
}

// Number of URL's that rely on this port
ULONG
RequestQueue::NumberOfPorts(USHORT p_port)
{
  AutoCritSec lock(&m_lock);
  ULONG total = 0;

  for(auto& group : m_groups)
  {
    total += group->NumberOfPorts(p_port);
  }
  return total;
}

// Creating a stop event
void
RequestQueue::CreateEvent()
{
  if(m_event == 0L)
  {
    m_event = ::CreateEvent(nullptr,FALSE,FALSE,nullptr);
  }
}

// Closing the event, freeing the kernel resources
void
RequestQueue::CloseEvent()
{
  if(m_event)
  {
    CloseHandle(m_event);
    m_event = NULL;
  }
  if(m_start)
  {
    CloseHandle(m_start);
    m_start = NULL;
  }
}

void
RequestQueue::CloseQueueHandle()
{
  if(m_handle)
  {
    CloseHandle(m_handle);
    m_handle = nullptr;
  }
}

// Directly remove all fragments from the fragment cache
void
RequestQueue::DeleteAllFragments()
{
  AutoCritSec lock(&m_lock);

  for(auto& frag : m_fragments)
  {
    PHTTP_DATA_CHUNK chunk = frag.second;
    free(chunk->FromMemory.pBuffer);
    free(chunk);
  }
  m_fragments.clear();
}
