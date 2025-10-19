/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: ServiceReporting.h
//
// BaseLibrary: Indispensable general objects and functions
// 
// Created: 2014-2025 ir. W.E. Huisman
// MIT License
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
#include "pch.h"
#include "ServiceReporting.h"
#include "EventLogRegistration.h"
#include "AutoCritical.h"
#include "GetLastErrorAsString.h"
#include "Alert.h"
#include "strsafe.h"

// Each buffer in a event buffer array has a limit of 31K characters
// See MSDN: ReportEvent function (31.839 bytes)
#define EVENTBUFFER  ((31 * 1024) / sizeof(TCHAR))

PTCHAR           g_eventBuffer = nullptr;
CRITICAL_SECTION g_eventBufferLock;
TCHAR            g_svcname[SERVICE_NAME_LENGTH] = _T("");

static void
SvcFreeEventBuffer()
{
  // Deallocate the logging buffer of the server
  if(g_eventBuffer)
  {
    delete [] g_eventBuffer;
    g_eventBuffer = nullptr;
  }
  DeleteCriticalSection(&g_eventBufferLock);
}

static void
SvcStartEventBuffer()
{
  // Already there: nothing to be done
  if(g_eventBuffer)
  {
    return;
  }
  g_eventBuffer = alloc_new TCHAR[EVENTBUFFER];

  // Initialize the logging lock
  InitializeCriticalSection(&g_eventBufferLock);
  // Deallocate our event buffer at the end
  atexit(SvcFreeEventBuffer);
}

void
SvcReportInfoEvent(bool p_doFormat,LPCTSTR p_message,...)
{
  // Be sure our event system is started
  SvcStartEventBuffer();

  AutoCritSec lock(&g_eventBufferLock);
  if(p_doFormat)
  {
    va_list vl;
    va_start(vl,p_message);
    _vsntprintf_s(g_eventBuffer,EVENTBUFFER,_TRUNCATE,p_message,vl);
    va_end(vl);
  }
  else
  {
    StringCchCopyN(g_eventBuffer,EVENTBUFFER,p_message,EVENTBUFFER - 1);
  }

  HANDLE hEventSource = OpenEventLog(nullptr,g_svcname);
  if(hEventSource != nullptr)
  {
    LPCTSTR lpszStrings[2];
    lpszStrings[0] = g_svcname;
    lpszStrings[1] = g_eventBuffer;

    ReportEvent(hEventSource,                 // event log handle
                EVENTLOG_INFORMATION_TYPE,    // event type
                0,                            // event category
                SVC_INFO,                     // event identifier
                nullptr,                      // no security identifier
                2,                            // size of lpszStrings array
                0,                            // no binary data
                lpszStrings,                  // array of strings
                nullptr);                     // no binary data
    CloseEventLog(hEventSource);
  }
}

void
SvcReportSuccessEvent(LPCTSTR p_message)
{
  // Be sure our event system is started
  SvcStartEventBuffer();

  HANDLE hEventSource = OpenEventLog(nullptr,g_svcname);

  if(hEventSource != nullptr)
  {
    LPCTSTR lpszStrings[2];
    lpszStrings[0] = g_svcname;
    lpszStrings[1] = p_message;

    ReportEvent(hEventSource,        // event log handle
                EVENTLOG_SUCCESS,    // event type
                0,                   // event category
                SVC_SUCCESS,         // event identifier
                nullptr,             // no security identifier
                2,                   // size of lpszStrings array
                0,                   // no binary data
                lpszStrings,         // array of strings
                nullptr);            // no binary data

    CloseEventLog(hEventSource);
  }
}

// Purpose:       Logs messages to the event log
// Parameters:    szFunction - name of function that failed
// Return value:  None
// Remarks:       The service must have an entry in the Application event log.
//
void
SvcReportErrorEvent(int p_module,bool p_doFormat,LPCTSTR szFunction,LPCTSTR p_message,...)
{
  TCHAR   buffer1[256];
  TCHAR   buffer2[256];
  int     lastError = GetLastError();

  // Be sure our event system is started
  SvcStartEventBuffer();

  AutoCritSec lock(&g_eventBufferLock);
  if(p_doFormat)
  {
    va_list vl;
    va_start(vl,p_message);
    _vsntprintf_s(g_eventBuffer,EVENTBUFFER,_TRUNCATE,p_message,vl);
    va_end(vl);
  }
  else
  {
    StringCchCopyN(g_eventBuffer,EVENTBUFFER,p_message,EVENTBUFFER - 1);
  }
  _stprintf_s(buffer1, 256, _T("Function %s"), szFunction);
  _stprintf_s(buffer2, 256, _T("Last OS error: [%d] %s"),lastError,GetLastErrorAsString(lastError).GetString());

  HANDLE hEventSource = OpenEventLog(nullptr,g_svcname);

  if(hEventSource != nullptr)
  {
    LPCTSTR lpszStrings[4];

    lpszStrings[0] = g_svcname;
    lpszStrings[1] = g_eventBuffer;
    lpszStrings[2] = buffer1;
    lpszStrings[3] = buffer2;

    ReportEvent(hEventSource,        // event log handle
                EVENTLOG_ERROR_TYPE, // event type
                0,                   // event category
                SVC_ERROR,           // event identifier
                nullptr,             // no security identifier
                4,                   // size of lpszStrings array
                0,                   // no binary data
                lpszStrings,         // array of strings
                nullptr);            // no binary data
    CloseEventLog(hEventSource);
  }

  // Create alert file if requested
  if(p_module)
  {
    CreateAlert(szFunction,buffer2,g_eventBuffer,p_module);
  }
}
