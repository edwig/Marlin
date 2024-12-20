//////////////////////////////////////////////////////////////////////////
//
// USER-SPACE IMPLEMENTTION OF HTTP.SYS
//
// 2018 - 2024 (c) ir. W.E. Huisman
// License: MIT
//
//////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "OpaqueHandles.h"
#include <AutoCritical.h>
#include <time.h>
#include <stdlib.h>
#include <limits.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// All handles are stored here
OpaqueHandles g_handles;

// A HANDLE stores a type and a void pointer
//
HTTPHandle::HTTPHandle(HTTPHandleType p_type,void* p_pointer)
           :m_type(p_type)
           ,m_pointer(p_pointer)
{
}

//////////////////////////////////////////////////////////////////////////
//
// HANDLES
//
OpaqueHandles::OpaqueHandles()
{
  // Seed the random number generator at the start
  time_t when = 0;
  time(&when);
  srand((unsigned int)when);

  // To obfuscate the handles, start at a random number
  // and work our way up from there.
  unsigned int number = 0;
  rand_s(&number);
  m_nextHandle = number % 0x07FF;

  InitializeCriticalSection(&m_lock);
}

OpaqueHandles::~OpaqueHandles()
{
  DeleteCriticalSection(&m_lock);
}

// Create a NEW opaque handle and store it with the pointer in the map
HANDLE
OpaqueHandles::CreateOpaqueHandle(HTTPHandleType p_type,void* p_toStore)
{
  HANDLE handle = CreateNewHandle();

  AutoCritSec lock(&m_lock);
  auto result = m_handles.insert(std::make_pair(handle,HTTPHandle(p_type,p_toStore)));
  if(result.second)
  {
    return handle;
  }
  return nullptr;
}

// Create an already created handle in the opaque handles map
bool
OpaqueHandles::StoreOpaqueHandle(HTTPHandleType p_type,HANDLE p_handle,void* p_toStore)
{
  AutoCritSec lock(&m_lock);
  auto result = m_handles.insert(std::make_pair(p_handle,HTTPHandle(p_type,p_toStore)));
  return result.second;
}

// Remove from the map
bool
OpaqueHandles::RemoveOpaqueHandle(HANDLE p_handle)
{
  AutoCritSec lock(&m_lock);
  HandleMap::iterator it = m_handles.find(p_handle);
  if(it != m_handles.end())
  {
    m_handles.erase(it);
    return true;
  }
  return false;
}

// Getting a server session
ServerSession* 
OpaqueHandles::GetSessionFromOpaqueHandle(HTTP_SERVER_SESSION_ID p_handle)
{
  AutoCritSec lock(&m_lock);
  HandleMap::iterator it = m_handles.find((HANDLE)p_handle);
  if(it != m_handles.end())
  {
    if(it->second.m_type == HTTPHandleType::HTTP_Session)
    {
      ServerSession* session = (ServerSession*)(it->second.m_pointer);
      if(session && session->GetIdent() == HTTP_SERVER_IDENT)
      {
        return session;
      }
    }
  }
  return nullptr;
}

// Getting a request queue
RequestQueue*
OpaqueHandles::GetReQueueFromOpaqueHandle(HANDLE p_handle)
{
  AutoCritSec lock(&m_lock);
  HandleMap::iterator it = m_handles.find((HANDLE)p_handle);
  if(it != m_handles.end())
  {
    if(it->second.m_type == HTTPHandleType::HTTP_Queue)
    {
      RequestQueue* queue = (RequestQueue*)(it->second.m_pointer);
      if(queue && queue->GetIdent() == HTTP_QUEUE_IDENT)
      {
        return queue;
      }
    }
  }
  return nullptr;
}

// Getting an URL group
UrlGroup* 
OpaqueHandles::GetUrGroupFromOpaqueHandle(HTTP_URL_GROUP_ID p_handle)
{
  AutoCritSec lock(&m_lock);
  HandleMap::iterator it = m_handles.find((HANDLE)p_handle);
  if(it != m_handles.end())
  {
    if(it->second.m_type == HTTPHandleType::HTTP_UrlGroup)
    {
      UrlGroup* group = (UrlGroup*)(it->second.m_pointer);
      if(group && group->GetIdent() == HTTP_URLGROUP_IDENT)
      {
        return group;
      }
    }
  }
  return nullptr;
}

// getting a request
Request*
OpaqueHandles::GetRequestFromOpaqueHandle(HTTP_REQUEST_ID p_handle)
{
  AutoCritSec lock(&m_lock);
  HandleMap::iterator it = m_handles.find((HANDLE)p_handle);
  if(it != m_handles.end())
  {
    if(it->second.m_type == HTTPHandleType::HTTP_Request)
    {
      Request* req = (Request*)(it->second.m_pointer);
      if(req && req->GetIdent() == HTTP_REQUEST_IDENT)
      {
        return req;
      }
    }
  }
  return nullptr;
}

// Copy all queue handles in a map for processing
int
OpaqueHandles::GetAllQueueHandles(HandleMap& p_queues)
{
  int count = 0;

  AutoCritSec lock(&m_lock);
  for(auto& handle : m_handles)
  {
    if(handle.second.m_type == HTTPHandleType::HTTP_Queue)
    {
      p_queues.insert(std::make_pair(handle.first,handle.second));
      ++count;
    }
  }
  return count;
}

//////////////////////////////////////////////////////////////////////////
//
// PRIVATE
//
//////////////////////////////////////////////////////////////////////////

// Getting a next handle.
// Could clash with 'real' MS-Windows kernel handles
// So we check that it is a unique value
HANDLE
OpaqueHandles::CreateNewHandle()
{
  HANDLE handle = nullptr;
  HandleMap::iterator it;
  do
  {
    handle = (HANDLE)InterlockedIncrement64(&m_nextHandle);
    it = m_handles.find(handle);
  }
  while(it != m_handles.end());

  return handle;
}
