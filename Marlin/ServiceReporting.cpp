/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: ServiceReporting.h
//
// Marlin Server: Internet server/client
// 
// Copyright (c) 2015-2018 ir. W.E. Huisman
// All rights reserved
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
#include "stdafx.h"
#include "ServiceReporting.h"
#include "AutoCritical.h"
#include "ServerMain.h"
#include "GetLastErrorAsString.h"
#include "strsafe.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// Each buffer in a event buffer array has a limit of 32K characters
// See MSDN: ReportEvent function
#define EVENTBUFFER  (32 * 1024)

char*            g_eventBuffer = NULL;
CRITICAL_SECTION g_eventBufferLock;
extern char      g_svcname[];

void
SvcStartEventBuffer()
{
  // Initialise the logging lock
  InitializeCriticalSection(&g_eventBufferLock);
}

void
SvcFreeEventBuffer()
{
  // Deallocate the logging buffer of the server
  if(g_eventBuffer)
  {
    free(g_eventBuffer);
    g_eventBuffer = NULL;
  }

  DeleteCriticalSection(&g_eventBufferLock);
}

void
SvcAllocEventBuffer()
{
  // Lock against double allocation
  AutoCritSec lock(&g_eventBufferLock);

  // Already there: nothing to be done
  if(g_eventBuffer)
  {
    return;
  }
  g_eventBuffer = (char*)malloc(EVENTBUFFER + 1);
  if(g_eventBuffer == NULL)
  {
    SvcReportSuccessEvent("ERROR: Cannot make a buffer for errors and events");
  }
}

void
SvcReportInfoEvent(bool p_doFormat,LPCTSTR p_message,...)
{
  HANDLE hEventSource;
  LPCTSTR lpszStrings[2];

  SvcAllocEventBuffer();
  if(p_doFormat)
  {
    va_list vl;
    va_start(vl,p_message);
    _vsnprintf_s(g_eventBuffer,EVENTBUFFER,_TRUNCATE,p_message,vl);
    va_end(vl);
  }
  else
  {
    StringCchCopy(g_eventBuffer,EVENTBUFFER,p_message);
  }

  hEventSource = OpenEventLog(NULL,g_svcname);

  if(hEventSource != NULL)
  {
    lpszStrings[0] = g_svcname;
    lpszStrings[1] = g_eventBuffer;

    ReportEvent(hEventSource,                 // event log handle
                EVENTLOG_INFORMATION_TYPE,    // event type
                0,                            // event category
                SVC_INFO,                     // event identifier
                NULL,                         // no security identifier
                2,                            // size of lpszStrings array
                0,                            // no binary data
                lpszStrings,                  // array of strings
                NULL);                        // no binary data
    CloseEventLog(hEventSource);
  }
}

void
SvcReportSuccessEvent(LPCTSTR p_message)
{
  HANDLE hEventSource;
  LPCTSTR lpszStrings[2];

  hEventSource = OpenEventLog(NULL,g_svcname);

  if(hEventSource != NULL)
  {
    lpszStrings[0] = g_svcname;
    lpszStrings[1] = p_message;

    ReportEvent(hEventSource,        // event log handle
                EVENTLOG_SUCCESS,    // event type
                0,                   // event category
                SVC_SUCCESS,         // event identifier
                NULL,                // no security identifier
                2,                   // size of lpszStrings array
                0,                   // no binary data
                lpszStrings,         // array of strings
                NULL);               // no binary data

    CloseEventLog(hEventSource);
  }
}

// Purpose:       Logs messages to the event log
// Parameters:    szFunction - name of function that failed
// Return value:  None
// Remarks:       The service must have an entry in the Application event log.
//
void
SvcReportErrorEvent(bool p_doFormat,LPCTSTR szFunction,LPCTSTR p_message,...)
{
  HANDLE  hEventSource;
  LPCTSTR lpszStrings[3];
  TCHAR   Buffer[256];
  int     lastError = GetLastError();

  SvcAllocEventBuffer();
  if(p_doFormat)
  {
    va_list vl;
    va_start(vl,p_message);
    _vsnprintf_s(g_eventBuffer,EVENTBUFFER,_TRUNCATE,p_message,vl);
    va_end(vl);
  }
  else
  {
    StringCchCopy(g_eventBuffer,EVENTBUFFER,p_message);
  }

  hEventSource = OpenEventLog(NULL,g_svcname);

  if(hEventSource != nullptr)
  {
    StringCchPrintf(Buffer,256,"Function %s. Last OS error: [%d] %s",szFunction,lastError,GetLastErrorAsString(lastError).GetString());

    lpszStrings[0] = g_svcname;
    lpszStrings[1] = g_eventBuffer;
    lpszStrings[2] = Buffer;

    ReportEvent(hEventSource,        // event log handle
                EVENTLOG_ERROR_TYPE, // event type
                0,                   // event category
                SVC_ERROR,           // event identifier
                NULL,                // no security identifier
                3,                   // size of lpszStrings array
                0,                   // no binary data
                lpszStrings,         // array of strings
                NULL);               // no binary data
    CloseEventLog(hEventSource);
  }
}
