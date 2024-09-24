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
#include "UrlGroup.h"
#include "ServerSession.h"
#include <LogAnalysis.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//////////////////////////////////////////////////////////////////////////
//
// Actual call
//
//////////////////////////////////////////////////////////////////////////

ULONG WINAPI
HttpReceiveHttpRequest(IN HANDLE          RequestQueueHandle
                      ,IN HTTP_REQUEST_ID RequestId
                      ,IN ULONG           Flags
                      ,_Out_writes_bytes_to_(RequestBufferLength, *BytesReturned) PHTTP_REQUEST RequestBuffer
                      ,IN ULONG           RequestBufferLength
                      ,_Out_opt_ PULONG   BytesReturned
                      ,IN LPOVERLAPPED    Overlapped OPTIONAL)
{
  // Preset to zero
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

  // Flags must be 0 or one of the body commands
  if(Flags && Flags != HTTP_RECEIVE_REQUEST_FLAG_COPY_BODY && Flags != HTTP_RECEIVE_REQUEST_FLAG_FLUSH_BODY)
  {
    return ERROR_INVALID_PARAMETER;
  }

  // RequestBuffer must be at least as big as the HTTP_REQUEST
  if(RequestBufferLength < sizeof(HTTP_REQUEST_V2) + 16)
  {
    return ERROR_INVALID_PARAMETER;
  }

  // If we re-issue a read call, it must be for the same request
  if(RequestId && RequestBuffer && (RequestId != RequestBuffer->RequestId))
  {
    return ERROR_INVALID_PARAMETER;
  }

  // Find the request queue
  RequestQueue* queue = GetRequestQueueFromHandle(RequestQueueHandle);
  if(queue == nullptr)
  {
    return ERROR_INVALID_PARAMETER;
  }

  // Overlapped I/O needs an I/O Completion port from the server.
  if(Overlapped && (queue->GetIOCompletionPort() == nullptr || queue->GetIOCompletionKey() == NULL))
  {
    return ERROR_INVALID_PARAMETER;
  }

  // In case of overlapped I/O we will return by means of the completion routine
  if(Overlapped)
  {
    // Register our parameters
    PREGISTER_HTTP_RECEIVE_REQUEST req = new REGISTER_HTTP_RECEIVE_REQUEST();
    req->r_size                = sizeof(REGISTER_HTTP_RECEIVE_REQUEST);
    req->r_queue               = queue;
    req->r_id                  = RequestId;
    req->r_flags               = Flags;
    req->r_requestBuffer       = RequestBuffer;
    req->r_requestBufferLength = RequestBufferLength;
    req->r_overlapped          = Overlapped;

    queue->AddWaitingRequest(req);

    // IO completion pending
    return ERROR_IO_PENDING;
  }

  // Sync I/O: We will now get the next request or wait for it
  // Get the next request from the queue
  ULONG bytes  = 0;
  ULONG result = queue->GetNextRequest(RequestId,Flags,RequestBuffer,RequestBufferLength,&bytes);

  // Reflect number of bytes read
  if(BytesReturned)
  {
    *BytesReturned = bytes;
  }

  return result;
}

//////////////////////////////////////////////////////////////////////////
//
// ASYNC IO
//
//////////////////////////////////////////////////////////////////////////

void StartAsyncReceiveHttpRequest(PREGISTER_HTTP_RECEIVE_REQUEST req)
{
  if(req == nullptr || req->r_size != sizeof(REGISTER_HTTP_RECEIVE_REQUEST))
  {
    return;
  }
  _set_se_translator(SeTranslator);
  XString error;

  try
  {
    // ASync I/O: We will now get the next request or wait for it
    // Get the next request from the queue
    ULONG bytes  = 0;
    ULONG result = req->r_queue->GetNextRequest(req->r_id,req->r_flags,req->r_requestBuffer,req->r_requestBufferLength,&bytes);
    if(result)
    {
      error.Format(_T("Cannot receive next request. Error: %d "),result);
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
      if(session)
      {
        session->GetLogfile()->AnalysisLog(_T(__FUNCTION__),LogType::LOG_INFO,false,error.GetString());
      }
    }
  }

  // Ready with the request
  delete req;
}
