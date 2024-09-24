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
#include "Request.h"
#include "RequestQueue.h"
#include "UrlGroup.h"
#include "ServerSession.h"
#include "LogAnalysis.h"
#include "SSLUtilities.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// Forward declarations
unsigned __stdcall StartAsyncSendHttpResponse(void* p_param);

typedef struct _reg_http_send_response
{
  UINT           r_size;
  RequestQueue*  r_queue;
  Request*       r_request;
  PHTTP_RESPONSE r_httpResponse;
  ULONG          r_flags;
  LPOVERLAPPED   r_overlapped;
}
REGISTER_HTTP_SEND_RESPONSE,*PREGISTER_HTTP_SEND_RESPONSE;

//////////////////////////////////////////////////////////////////////////
//
// Actual call
//
//////////////////////////////////////////////////////////////////////////

ULONG WINAPI
HttpSendHttpResponse(IN HANDLE              RequestQueueHandle
                    ,IN HTTP_REQUEST_ID     RequestId
                    ,IN ULONG               Flags
                    ,IN PHTTP_RESPONSE      HttpResponse
                    ,IN PHTTP_CACHE_POLICY  CachePolicy OPTIONAL
                    ,OUT PULONG             BytesSent   OPTIONAL
                    ,_Reserved_ PVOID       Reserved1
                    ,_Reserved_ ULONG       Reserved2
                    ,IN LPOVERLAPPED        Overlapped  OPTIONAL
                    ,IN PHTTP_LOG_DATA      LogData     OPTIONAL)
{
  // Check if reserved 1 or 2 or Overlapped are set 
  if(Reserved1 || Reserved2)
  {
    return ERROR_INVALID_PARAMETER;
  }

  // We totally ignore the Cache Policy parameter: No caching!
  UNREFERENCED_PARAMETER(CachePolicy);

  // Check that we have the BytesSent parameter
  // BytesSent and Overlapped are mutually exclusive!
  if((BytesSent == nullptr && Overlapped == nullptr) ||
     (BytesSent != nullptr && Overlapped != nullptr))
  {
    return ERROR_INVALID_PARAMETER;
  }

  // Reset bytes sent
  if(BytesSent)
  {
    *BytesSent = 0L;
  }

  // Finding the elementary object
  RequestQueue* queue = GetRequestQueueFromHandle(RequestQueueHandle);
  Request*    request = GetRequestFromHandle(RequestId);
  ULONG        result = ERROR_HANDLE_EOF;

  if(queue == nullptr || request == nullptr)
  {
    return ERROR_INVALID_PARAMETER;
  }

  // Overlapped I/O needs an I/O Completion port from the server.
  if(Overlapped && (queue->GetIOCompletionPort() == nullptr || queue->GetIOCompletionKey() == NULL))
  {
    return ERROR_INVALID_PARAMETER;
  }

  if(queue->RequestStillInService(request))
  {

    if(Overlapped)
    {
      PREGISTER_HTTP_SEND_RESPONSE reg = new REGISTER_HTTP_SEND_RESPONSE();
      reg->r_size         = sizeof(REGISTER_HTTP_SEND_RESPONSE);
      reg->r_queue        = queue;
      reg->r_request      = request;
      reg->r_httpResponse = HttpResponse;
      reg->r_flags        = Flags;
      reg->r_overlapped   = Overlapped;

      uintptr_t thread = 0L;
      do
      {
        thread = _beginthreadex(nullptr,0,StartAsyncSendHttpResponse,reg,0,nullptr);
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
      while (thread == 0L);

      // IO completion pending
      result = ERROR_IO_PENDING;
    }
    else
    {
      // Create and send the response
      result = request->SendResponse(HttpResponse,Flags,BytesSent);

      if(((Flags & (HTTP_SEND_RESPONSE_FLAG_MORE_DATA | HTTP_SEND_RESPONSE_FLAG_OPAQUE)) == 0) && request->GetResponseComplete())
      {
        if(Flags & HTTP_SEND_RESPONSE_FLAG_DISCONNECT)
        {
          // Force close connection
          queue->RemoveRequest(request);
        }
        else
        {
          if(request->RestartConnection() == false)
          {
            // No keep-alive found: Request/Response now complete
            queue->RemoveRequest(request);
          }
        }
      }
    }
  }

  // Log our response
  if(LogData)
  {
    // Not yet implemented in the Microsoft base HTTPSYS.DLL driver.
    UrlGroup* group = queue->FirstURLGroup();
    if(group)
    {
      ServerSession* session = group->GetServerSession();
      if(session)
      {
        session->GetLogfile()->AnalysisLog(_T(__FUNCTION__),LogType::LOG_INFO,true,_T("Response %s: %d bytes"),Overlapped ? _T("pending") : _T("sent"),BytesSent);
      }
    }
  }

  return result;
}

unsigned __stdcall StartAsyncSendHttpResponse(void* p_param)
{
  PREGISTER_HTTP_SEND_RESPONSE reg = reinterpret_cast<PREGISTER_HTTP_SEND_RESPONSE>(p_param);
  if(reg == nullptr || reg->r_size != sizeof(REGISTER_HTTP_SEND_RESPONSE))
  {
    return ERROR_INVALID_PARAMETER;
  }
  SetThreadName(_T("HTTPSendResponse"));
  _set_se_translator(SeTranslator);
  XString error;

  try
  {
    // Create and send the response
    ULONG bytes = 0L;
    int result = reg->r_request->SendResponse(reg->r_httpResponse,reg->r_flags,&bytes);

    if(((reg->r_flags & (HTTP_SEND_RESPONSE_FLAG_MORE_DATA | HTTP_SEND_RESPONSE_FLAG_OPAQUE)) == 0) && reg->r_request->GetResponseComplete())
    {
      if(reg->r_flags & HTTP_SEND_RESPONSE_FLAG_DISCONNECT)
      {
        // Force close connection
        reg->r_queue->RemoveRequest(reg->r_request);
      }
      else
      {
        if(reg->r_request->RestartConnection() == false)
        {
          // No keep-alive found: Request/Response now complete
          reg->r_queue->RemoveRequest(reg->r_request);
        }
      }
    }

    // Remember the results
    reg->r_overlapped->Internal     = (DWORD)result;
    reg->r_overlapped->InternalHigh = (DWORD)bytes;

    // Post a completion to the applications completion port for the request queue
    if(PostQueuedCompletionStatus(reg->r_queue->GetIOCompletionPort(),bytes,reg->r_queue->GetIOCompletionKey(),reg->r_overlapped) == 0)
    {
      error += _T("Cannot post a completion status to the IOCP");
    }
  }
  catch (StdException& ex)
  {
    error += ex.GetErrorMessage();
  }
  if(!error.IsEmpty())
  {
    // Not yet implemented in the Microsoft base HTTPSYS.DLL driver.
    UrlGroup* group = reg->r_queue->FirstURLGroup();
    if (group)
    {
      ServerSession* session = group->GetServerSession();
      if (session)
      {
        session->GetLogfile()->AnalysisLog(_T(__FUNCTION__),LogType::LOG_ERROR,false,error.GetString());
      }
    }
  }

  // Ready with the request
  delete reg;

  return NO_ERROR;
}
