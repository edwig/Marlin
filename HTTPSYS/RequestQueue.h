//////////////////////////////////////////////////////////////////////////
//
// USER-SPACE IMPLEMENTTION OF HTTP.SYS
//
// 2018 (c) ir. W.E. Huisman
// License: MIT
//
//////////////////////////////////////////////////////////////////////////

#pragma once
#include "URL.h"
#include "Listener.h"
#include "Request.h"
#include <mswsock.h>
#include <vector>
#include <deque>
#include <map>

#define HTTP_QUEUE_IDENT 0xEDED0000EDED0000

#define HTTP_REQUEST_QUEUE_MINIMUM    200   // As long as the WinSock backlog
#define HTTP_REQUEST_QUEUE_DEFAULT    400   // Twice the standard WinSock backlog
#define HTTP_REQUEST_QUEUE_MAXIMUM  64000   // Seriously overloaded server?

class UrlGroup;
class SYSWebSocket;

// Made requests with an Overlapped I/O registration
typedef struct _reg_http_request
{
  UINT              r_size;
  RequestQueue*     r_queue;
  HTTP_REQUEST_ID   r_id;
  ULONG             r_flags;
  PHTTP_REQUEST     r_requestBuffer;
  ULONG             r_requestBufferLength;
  LPOVERLAPPED      r_overlapped;
}
REGISTER_HTTP_RECEIVE_REQUEST,*PREGISTER_HTTP_RECEIVE_REQUEST;

using UrlGroups       = std::vector<UrlGroup*>;
using Listeners       = std::map<USHORT,Listener*>;
using Requests        = std::deque<Request*>;
using Fragments       = std::map<CString,PHTTP_DATA_CHUNK>;
using WaitingRequests = std::deque<PREGISTER_HTTP_RECEIVE_REQUEST>;
using WebSockets      = std::map<CString,SYSWebSocket*>;

typedef BOOL (* PointTransmitFile)(SOCKET hSocket,
                                   HANDLE hFile,
                                   DWORD nNumberOfBytesToWrite,
                                   DWORD nNumberOfBytesPerSend,
                                   LPOVERLAPPED lpOverlapped,
                                   LPTRANSMIT_FILE_BUFFERS lpFileBuffers,
                                   DWORD dwReserved);

// Forward declaration to handle a request
void StartAsyncReceiveHttpRequest(PREGISTER_HTTP_RECEIVE_REQUEST reg);

//////////////////////////////////////////////////////////////////////////
//
// REQUEST QUEUE
//
//////////////////////////////////////////////////////////////////////////

class RequestQueue
{
public:
  RequestQueue(CString p_name);
 ~RequestQueue();

  // GETTERS
  ULONGLONG                   GetIdent()            { return m_ident;       }
  HANDLE                      GetHandle()           { return m_handle;      }
  CString                     GetName()             { return m_name;        }
  HTTP_503_RESPONSE_VERBOSITY GetVerbosity()        { return m_verbosity;   }
  ULONG                       GetQueueLength()      { return m_queueLength; }
  HTTP_ENABLED_STATE          GetEnabledState()     { return m_state;       }
  HANDLE                      GetIOCompletionPort() { return m_iocPort;     }
  ULONG_PTR                   GetIOCompletionKey()  { return m_iocKey;      }

  // SETTERS
  bool      SetEnabledState(HTTP_ENABLED_STATE p_state);
  bool      SetQueueLength(ULONG p_length);
  bool      SetVerbosity(HTTP_503_RESPONSE_VERBOSITY p_verbosity);
  bool      SetIOCompletionPort(HANDLE p_iocp);
  bool      SetIOCompletionKey(ULONG_PTR p_key);

  // Functions
  void      AddURLGroup   (UrlGroup* p_group);
  void      RemoveURLGroup(UrlGroup* p_group);
  UrlGroup* FirstURLGroup();

  ULONG     StartListener(USHORT p_port,URL* p_url,USHORT p_timeout);
  ULONG     StopListener (USHORT p_port);
  Listener* FindListener (USHORT p_port);

  HANDLE    CreateHandle();
  void      AddIncomingRequest(Request* p_request);
  bool      ResetToServicing  (Request* p_request);
  URL*      FindLongestURL(USHORT p_port,CString p_abspath);
  void      RemoveRequest(Request* p_request);
  void      ClearIncomingWaiters();
  bool      RequestStillInService(Request* p_request);
  // Demand start
  ULONG     RegisterDemandStart();
  void      DemandStart();

  // Waiting Overlapped I/O requests
  void      AddWaitingRequest(PREGISTER_HTTP_RECEIVE_REQUEST p_register);
  PREGISTER_HTTP_RECEIVE_REQUEST FirstWaitingRequest();

  // Our workhorse. Implementations call this to get the next HTTP request
  ULONG     GetNextRequest(HTTP_REQUEST_ID RequestId,ULONG Flags,PHTTP_REQUEST RequestBuffer,ULONG RequestBufferLength,PULONG Bytes);

  // The fragment cache
  ULONG             AddFragment  (CString p_prefix,PHTTP_DATA_CHUNK p_chunk);
  ULONG             FlushFragment(CString p_prefix,ULONG Flags);
  PHTTP_DATA_CHUNK  FindFragment (CString p_prefix);

  // WebSocket functionality
  SYSWebSocket* FindWebSocket     (CString p_websocketKey);
  bool          AddWebSocket      (CString p_websocketKey,SYSWebSocket* p_websocket);
  bool          ReconnectWebsocket(CString p_websocketKey,SYSWebSocket* p_websocket);
  bool          DeleteWebSocket   (CString p_websocketKey);

private:
  void        StopAllListeners();
  ULONG       NumberOfPorts(USHORT p_port);
  void        DeleteAllFragments();
  void        DeleteAllWaiters();
  void        DeleteAllServicing();
  void        DeleteAllWebSockets();
  void        CreateEvent();
  void        CloseEvent();
  void        CloseQueueHandle();

  // Identification of the request queue
  ULONGLONG                   m_ident       { HTTP_QUEUE_IDENT };
  HANDLE                      m_handle      { NULL };
  CString                     m_name;
  // Request queue properties
  HTTP_503_RESPONSE_VERBOSITY m_verbosity   { Http503ResponseVerbosityBasic };
  ULONG                       m_queueLength { HTTP_REQUEST_QUEUE_DEFAULT    };
  HTTP_ENABLED_STATE          m_state       { HttpEnabledStateActive        };
  // All of our URL groups
  UrlGroups                   m_groups;
  // All listeners
  Listeners                   m_listeners;
  bool                        m_listening { false };
  // All requests from HTTP. Our 'real' queues
  Requests                    m_incoming;       // Incoming (unserviced) requests
  Requests                    m_servicing;      // Currently serviced by server
  // All WebSockets
  WebSockets                  m_websockets;
  // I/O Completion of the session registration
  HANDLE                      m_iocPort { NULL };
  ULONG_PTR                   m_iocKey  { NULL };
  // The fragment cache
  Fragments                   m_fragments;
  PointTransmitFile           m_transmitFile { nullptr };
  // Waiting Overlapped I/O
  WaitingRequests             m_waiting;
  // Synchronization
  HANDLE                      m_start { NULL }; // Demand start event
  CRITICAL_SECTION            m_lock;           // Queue synchronization
  HANDLE                      m_event { 0L };   // Queue waiting event
};
