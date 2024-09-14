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
#include "RequestQueue.h"
#include "Request.h"
#include "UrlGroup.h"
#include "ServerSession.h"
#include "SSLUtilities.h"
#include <LogAnalysis.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// Forward declarations
unsigned __stdcall StartAsyncReceiveHttpBody(void* p_param);

typedef struct _reg_http_receive_body
{
  UINT          r_size;
  RequestQueue* r_queue;
  Request*      r_request;
  PVOID         r_entityBuffer;
  ULONG         r_entityBufferLength;
  LPOVERLAPPED  r_overlapped;
}
REGISTER_HTTP_RECEIVE_BODY,*PREGISTER_HTTP_RECEIVE_BODY;

//////////////////////////////////////////////////////////////////////////
//
// The actual call
//
//////////////////////////////////////////////////////////////////////////

ULONG WINAPI
HttpReceiveRequestEntityBody(IN HANDLE          RequestQueueHandle
                            ,IN HTTP_REQUEST_ID RequestId
                            ,IN ULONG           Flags
                            ,_Out_writes_bytes_to_(EntityBufferLength, *BytesReturned) PVOID EntityBuffer
                            ,IN ULONG           EntityBufferLength
                            ,_Out_opt_ PULONG   BytesReturned
                            ,IN LPOVERLAPPED    Overlapped OPTIONAL)
{
  // Initialize BytesReturned
  if(BytesReturned)
  {
    *BytesReturned = 0;
  }

  // VARIOUS CHECKS
  // Both cannot be supplied (sync I/O or Asynchronous I/O must be performed)
  if((BytesReturned == nullptr && Overlapped == nullptr) ||
     (BytesReturned != nullptr && Overlapped != nullptr))
  {
    return ERROR_INVALID_PARAMETER;
  }

  // Check possible values of the Flags parameter
  if(Flags != 0 && Flags != HTTP_RECEIVE_REQUEST_ENTITY_BODY_FLAG_FILL_BUFFER)
  {
    return ERROR_INVALID_PARAMETER;
  }

  // In case of asynchronous I/O we will not wait for the total of all buffers
  // but we need to react on a per-call basis
  if(Overlapped && Flags != 0)
  {
    return ERROR_INVALID_PARAMETER;
  }

  // We should have a buffer of at least 1 byte
  if(EntityBuffer == nullptr || EntityBufferLength == 0)
  {
    return ERROR_INVALID_PARAMETER;
  }

  // Getting the elementary objects
  RequestQueue* queue = GetRequestQueueFromHandle(RequestQueueHandle);
  Request*    request = GetRequestFromHandle(RequestId);

  if(queue == nullptr || request == nullptr)
  {
    return ERROR_INVALID_PARAMETER;
  }

  // Try to receive for this request
  if(queue->RequestStillInService(request))
  {
    if(Overlapped)
    {
      // Register our parameters
      PREGISTER_HTTP_RECEIVE_BODY reg = new REGISTER_HTTP_RECEIVE_BODY();
      reg->r_size               = sizeof(REGISTER_HTTP_RECEIVE_BODY);
      reg->r_queue              = queue;
      reg->r_request            = request;
      reg->r_entityBuffer       = EntityBuffer;
      reg->r_entityBufferLength = EntityBufferLength;
      reg->r_overlapped         = Overlapped;
      uintptr_t thread = 0L;
      do 
      {
        thread = _beginthreadex(nullptr,0,StartAsyncReceiveHttpBody,reg,0,nullptr);
        if(thread)
        {
          break;
        }
        if(errno != EAGAIN)
        {
          return GetLastError();
        }
        // To many threads in the system, wait until drained!
        Sleep(THREAD_RETRY_WAITING);
      } 
      while(thread == 0L);

      return ERROR_IO_PENDING;
    }

    return request->ReceiveBuffer(EntityBuffer
                                 ,EntityBufferLength
                                 ,BytesReturned
                                 ,Flags == HTTP_RECEIVE_REQUEST_ENTITY_BODY_FLAG_FILL_BUFFER);
  }
  return ERROR_HANDLE_EOF;
}

//////////////////////////////////////////////////////////////////////////
//
// ASYNC IO
//
//////////////////////////////////////////////////////////////////////////

unsigned __stdcall StartAsyncReceiveHttpBody(void* p_param)
{
  PREGISTER_HTTP_RECEIVE_BODY req = reinterpret_cast<PREGISTER_HTTP_RECEIVE_BODY>(p_param);
  if (req == nullptr || req->r_size != sizeof(REGISTER_HTTP_RECEIVE_BODY))
  {
    return ERROR_INVALID_PARAMETER;
  }
  SetThreadName("HTTPReceiveBody");
  _set_se_translator(SeTranslator);
  XString error;

  try
  {
    ULONG bytes = 0L;
    int result = req->r_request->ReceiveBuffer(req->r_entityBuffer,req->r_entityBufferLength,&bytes,false);
    if(result)
    {
      error.Format("Cannot receive next request. Error: %d ",result);
    }

    // Remember the results
    req->r_overlapped->Internal     = (DWORD) result;
    req->r_overlapped->InternalHigh = (DWORD) bytes;

    // Post a completion to the applications completion port for the request queue
    if(PostQueuedCompletionStatus(req->r_queue->GetIOCompletionPort(),bytes,req->r_queue->GetIOCompletionKey(),req->r_overlapped) == 0)
    {
      error += _T("Cannot post a completion status to the IOCP");
    }
  }
  catch(StdException& ex)
  {
    error += ex.GetErrorMessage();
  }
  if(!error.IsEmpty())
  {
    // Not yet implemented in the Microsoft base HTTPSYS.DLL driver.
    UrlGroup* group = req->r_queue->FirstURLGroup();
    if (group)
    {
      ServerSession* session = group->GetServerSession();
      if (session)
      {
        session->GetLogfile()->AnalysisLog(_T(__FUNCTION__),LogType::LOG_INFO,false,error.GetString());
      }
    }
  }

  // Ready with the request
  delete req;

  return NO_ERROR;
}
