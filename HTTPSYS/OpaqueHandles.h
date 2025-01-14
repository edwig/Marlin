//////////////////////////////////////////////////////////////////////////
//
// USER-SPACE IMPLEMENTTION OF HTTP.SYS
//
// 2024 (c) ir. W.E. Huisman
// License: MIT
//
//////////////////////////////////////////////////////////////////////////

#pragma once
#include "ServerSession.h"
#include "RequestQueue.h"
#include "UrlGroup.h"
#include "Request.h"
#include <http.h>
#include <map>

enum class HTTPHandleType
{
  HTTP_Session
 ,HTTP_Queue
 ,HTTP_UrlGroup
 ,HTTP_Request
};

class HTTPHandle
{
public:
  HTTPHandle(HTTPHandleType p_type,void* p_pointer);

  HTTPHandleType m_type;
  void*          m_pointer;
};

using HandleMap = std::map<HANDLE,HTTPHandle>;

class OpaqueHandles
{
public:
  OpaqueHandles();
 ~OpaqueHandles();

  // CTOR/DTOR of the opaque handle
  HANDLE          CreateOpaqueHandle(HTTPHandleType p_type,void* p_toStore);
  bool            RemoveOpaqueHandle(HANDLE p_handle);
  bool             StoreOpaqueHandle(HTTPHandleType p_type,HANDLE p_handle,void* p_toStore);

  // Getting one of the handle's object
  ServerSession*  GetSessionFromOpaqueHandle(HTTP_SERVER_SESSION_ID p_handle);
  RequestQueue*   GetReQueueFromOpaqueHandle(HANDLE p_handle);
  UrlGroup*       GetUrGroupFromOpaqueHandle(HTTP_URL_GROUP_ID p_handle);
  Request*        GetRequestFromOpaqueHandle(HTTP_REQUEST_ID p_handle);

  int             GetAllQueueHandles(HandleMap& p_queues);

private:
  // Making a new ordinal handle value
  HANDLE    CreateNewHandle();
  // Storing the handles
  HandleMap m_handles;
  LONG64    m_nextHandle { 0L };
  // Multi-threaded lock
  CRITICAL_SECTION m_lock;
};

// All handles are stored here
extern OpaqueHandles g_handles;
