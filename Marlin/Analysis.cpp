/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: Analysis.cpp
//
// Marlin Server: Internet server/client
// 
// Copyright (c) 2017 ir. W.E. Huisman
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

//////////////////////////////////////////////////////////////////////////
//
// ANALYSIS
//
// Logs in the format:
// YYYY-MM-DD HH:MM:SS T Function_name..........Formatting string
//
//////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "Analysis.h"
#include "GetUserAccount.h"
#include "GetLastErrorAsString.h"
#include "AutoCritical.h"
#include "EnsureFile.h"
#include <string.h>
#include <sys/timeb.h>
#include <io.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

LogAnalysis::LogAnalysis(CString p_name)
            :m_name(p_name)
{
  InitializeCriticalSection(&m_lock);
}

LogAnalysis::~LogAnalysis()
{
  Reset();
  DeleteCriticalSection(&m_lock);
}

void
LogAnalysis::Reset()
{
  // Tell we closed 'normally'
  if(m_initialised)
  {
    AnalysisLog("Analysis log is:", LogType::LOG_INFO,false,"Closed");
  }
  if(!m_list.empty())
  {
    // Flushing the cache and ending all writing activity
    SetEvent(m_event);
    // Wait max 10 seconds to sync the logfile
    for(unsigned ind = 0; ind < 100; ++ind)
    {
      // Extra context for lock
      {
        AutoCritSec lock(&m_lock);
        if(m_list.empty())
        {
          break;
        }
      }
      Sleep(100);
    }
  }

  // De-initialize and so stopping the thread
  m_initialised = false;
  SetEvent(m_event);

  // Wait for a max of 200ms For the thread to stop
  for(unsigned ind = 0; ind < 10; ++ind)
  {
    if(m_logThread == NULL)
    {
      break;
    }
    Sleep(20);
  }

  // Flush file to disk, after thread has ended
  if(m_file)
  {
    CloseHandle(m_file);
    m_file = NULL;
  }
  // Clear the list (any remains get freed)
  m_list.clear();

  // Close events log
  if(m_eventLog)
  {
    DeregisterEventSource(m_eventLog);
    m_eventLog = NULL;
  }
}

CString
LogAnalysis::CreateUserLogfile(CString p_filename)
{
  CString extensie;
  CString filepart;
  int pos = p_filename.ReverseFind('.');
  if(pos >= 0)
  {
    filepart = p_filename.Left(pos);
    extensie = p_filename.Mid(pos);
  }
  else
  {
    // No extension found
    filepart = p_filename;
  }
  CString user(GetUserAccount(NameUnknown,true));
  filepart += "_";
  filepart += user;
  filepart += extensie;

  return filepart;
}

void    
LogAnalysis::SetLogFilename(CString p_filename,bool p_perUser /*=false*/)
{ 
  if(p_perUser)
  {
    p_filename = CreateUserLogfile(p_filename);
  }
  if(m_logFileName.CompareNoCase(p_filename) != 0)
  {
    // See if a full reset is needed to flush and close the current file
    if(m_file || m_initialised)
    {
      Reset();
    }
    // Re-init at next log-line
    m_logFileName = p_filename; 
  }
};

void
LogAnalysis::SetCache(int p_cache)
{
  m_cache = p_cache;

  if(m_cache < LOGWRITE_MINCACHE) m_cache = LOGWRITE_MINCACHE;
  if(m_cache > LOGWRITE_MAXCACHE) m_cache = LOGWRITE_MAXCACHE;
}

void
LogAnalysis::SetInterval(int p_interval)
{
  m_interval = p_interval;

  if(m_interval < LOGWRITE_INTERVAL_MIN) m_interval = LOGWRITE_INTERVAL_MIN;
  if(m_interval > LOGWRITE_INTERVAL_MAX) m_interval = LOGWRITE_INTERVAL_MAX;
}

size_t
LogAnalysis::GetCacheSize()
{
  AutoCritSec lock(&m_lock);
  return m_list.size();
};

// Intialize our logfile/log event
void
LogAnalysis::Initialisation()
{
  // Still something to do?
  if(m_initialised)
  {
    return;
  }

  // Read the config file (if any)
  ReadConfig();

  // We are now initialized
  // Must do this here, otherwise an endless loop will occur..
  m_initialised = true;

  // See if we must do something
  if(m_doLogging == false)
  {
    return;
  }

  // Append date time to logfilename
  if(m_rotate)
  {
    AppendDateTimeToFilename();
  }

  // Open the logfile
  m_file = CreateFile(m_logFileName
                     ,GENERIC_WRITE
                     ,FILE_SHARE_READ | FILE_SHARE_WRITE
                     ,NULL              // Security
                     ,CREATE_ALWAYS     // Always throw away old log
                     ,FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED 
                     ,NULL);
  if(m_file ==  INVALID_HANDLE_VALUE)
  {
    char buffer[MAX_PATH+1];
    if(GetEnvironmentVariable("TMP",buffer,MAX_PATH))
    {
      CString file(buffer);
      if(file.Right(1) != "\\") file += "\\";
      file += "Analysis.log";

      // Open the logfile in shared writing mode
      // more applications can write to the file
      m_file = CreateFile(m_logFileName
                         ,GENERIC_WRITE
                         ,FILE_SHARE_READ | FILE_SHARE_WRITE
                         ,NULL              // Security
                         ,OPEN_ALWAYS       // Open existing, or create
                         ,FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED 
                         ,NULL);
      if(m_file == INVALID_HANDLE_VALUE)
      {
        // Give up. Cannot create a logfile
        m_file = NULL;
        m_doLogging = false;
        return;
      }
    }
  }
  if(m_doEvents)
  {
    // Use standard application event log
    m_eventLog = RegisterEventSource(NULL,m_name);
    if(m_eventLog == NULL)
    {
      m_doEvents = false;
    }
  }

  // Starting the log writing thread
  RunLog();

  // Tell that we are now running
  CString name = "Logfile for: " + m_name;
  AnalysisLog(name.GetString(), LogType::LOG_INFO,false,"Running");
}

bool
LogAnalysis::AnalysisLog(const char* p_function,LogType p_type,bool p_doFormat,const char* p_format,...)
{
  // Multi threaded protection
  AutoCritSec lock(&m_lock);
  CString logBuffer;
  bool result = false;

  // Make sure the system is initialized
  Initialisation();

  // Make sure we ARE logging
  if(m_doLogging == false)
  {
    return result;
  }

  // Timing position in the buffer
  int position = 0;

  // Set the type
  char type = ' ';
  switch(p_type)
  {
    case LogType::LOG_TRACE:type = 'T'; break;
    case LogType::LOG_INFO: type = '-'; break;
    case LogType::LOG_ERROR:type = 'E'; break;
    case LogType::LOG_WARN: type = 'W'; break;
  }

  // Get/print the time
  if(m_doTiming)
  {
    __timeb64 now;
    struct tm today;

    position = 26;  // Prefix string length
    _ftime64_s(&now);
    _localtime64_s(&today,&now.time);
    logBuffer.Format("%4.4d-%2.2d-%2.2d %2.2d:%2.2d:%2.2d.%03d %c "
                    ,today.tm_year + 1900
                    ,today.tm_mon  + 1
                    ,today.tm_mday
                    ,today.tm_hour
                    ,today.tm_min
                    ,today.tm_sec
                    ,now.millitm
                    ,type);
  }

  // Print the calling function
  logBuffer += p_function;
  if(logBuffer.GetLength() < position + ANALYSIS_FUNCTION_SIZE)
  {
    logBuffer.Append("                                          "
                    ,position + ANALYSIS_FUNCTION_SIZE - logBuffer.GetLength());
  }

  // Print the arguments
  if(p_doFormat)
  {
    va_list  varargs;
    va_start(varargs,p_format);
    logBuffer.AppendFormatV(p_format,varargs);
    va_end(varargs);
  }
  else
  {
    logBuffer += p_format;
  }

  // Add end-of line
  logBuffer += "\n";

  if(m_file)
  {
    // Locked m_list gets a buffer
    m_list.push_back(logBuffer);
    result = true;
  }
  else if(m_doEvents)
  {
    WriteEvent(m_eventLog,p_type,logBuffer);
    result = true;
  }
  // In case of an error, flush immediately!
  if(m_file && p_type == LogType::LOG_ERROR)
  {
    SetEvent(m_event);
  }
  return result;
}

void
LogAnalysis::WriteEvent(HANDLE p_eventLog,LogType p_type,CString& p_buffer)
{
  LPCTSTR lpszStrings[2];
  lpszStrings[0] = p_buffer.GetString();
  lpszStrings[1] = nullptr;
  ReportEvent(p_eventLog,static_cast<WORD>(p_type),0,0,NULL,1,0,lpszStrings,0);
}

void
LogAnalysis::ForceFlush()
{
  SetEvent(m_event);
}

// At certain times, you may wish to flush the cache
void    
LogAnalysis::Flush(bool p_all)
{
  // See if we have a file at all
  if(m_file == nullptr)
  {
    return;
  }

  // Multi threaded protection
  AutoCritSec lock(&m_lock);

  // See if we have log-lines
  if(!m_list.empty())
  {
    // See if list has become to long
    if(((int)m_list.size() > m_cache) || p_all)
    {
      // Collecting the buffered list in a string
      CString buffer;

      // Gather the list in the buffer, destroying the list
      while(!m_list.empty())
      {
        CString line = m_list.front();
        buffer += line;
        m_list.pop_front();
      }
      // Write out the buffer
      // Overlapped IO returns immediately, so we end the lock
      WriteLog(buffer);
    }
  }
}

// Write out a log line
void
LogAnalysis::WriteLog(CString p_buffer)
{
  DWORD written = 0L;
  if(!WriteFile(m_file
               ,(void*)p_buffer.GetString()
               ,(DWORD)p_buffer.GetLength()
               ,&written
               ,NULL))
  {
    TRACE("Cannot write logfile. Error: %d\n",GetLastError());
  }
}

// Read the 'Logfile.Config' in the current directory
// for overloads on the settings of the logfile
void
LogAnalysis::ReadConfig()
{
  char buffer[256] = "";
  CString fileName = "Logfile.config";

  FILE* file = NULL;
  fopen_s(&file,fileName,"r");
  if(file)
  {
    while(fread(buffer,255,1,file))
    {
      size_t len = strlen(buffer);
      while(len)
      {
        --len;
        if(buffer[len] == '\n' || buffer[len] == '\r')
        {
          buffer[len] = 0;
        }
      }
      if(buffer[0] == ';' || buffer[0] == '#')
      {
        continue;
      }
      if(_strnicmp(buffer,"logfile=",8) == 0)
      {
        m_logFileName = &buffer[8];
        continue;
      }
      if(_strnicmp(buffer,"logging=",8) == 0)
      {
        m_doLogging = atoi(&buffer[8]) > 0;
        continue;
      }
      if(_strnicmp(buffer,"rotate=",7) == 0)
      {
        m_rotate = atoi(&buffer[7]) > 0;
        continue;
      }
      if(_strnicmp(buffer,"timing=",7) == 0)
      {
        m_doTiming = atoi(&buffer[7]) > 0;
        continue;
      }
      if(_strnicmp(buffer,"events=",7) == 0)
      {
        m_doEvents = atoi(&buffer[7]) > 0;
      }
      if(_strnicmp(buffer,"cache=",6) == 0)
      {
        m_cache = atoi(&buffer[6]);
      }
      if(_strnicmp(buffer,"interval=",9) == 0)
      {
        m_interval = atoi(&buffer[9]) * CLOCKS_PER_SEC;
      }
    }
    fclose(file);

    // Check what we just read
    if(m_cache    < LOGWRITE_MINCACHE)     m_cache    = LOGWRITE_MINCACHE;
    if(m_cache    > LOGWRITE_MAXCACHE)     m_cache    = LOGWRITE_MAXCACHE;
    if(m_interval < LOGWRITE_INTERVAL_MIN) m_interval = LOGWRITE_INTERVAL_MIN;
    if(m_interval > LOGWRITE_INTERVAL_MAX) m_interval = LOGWRITE_INTERVAL_MAX;
  }
}

static unsigned int
__stdcall StartingTheLog(void* pParam)
{
  LogAnalysis* theLog = reinterpret_cast<LogAnalysis*> (pParam);
  theLog->RunLogAnalysis();
  return 0;
}

// Running the logfile in a separate thread
void
LogAnalysis::RunLog()
{
  // See if the thread is already running
  if(WaitForSingleObject(m_logThread,0) == WAIT_TIMEOUT)
  {
    // OK, Thread is kicking and alive.
    return;
  }
  else
  {
    // Create the event before starting the thread and before ending this call!!
    m_event = CreateEvent(NULL,FALSE,FALSE,NULL);
    // Basis thread of the InOutPort
    unsigned int threadID;
    if((m_logThread = (HANDLE)_beginthreadex(NULL,0,StartingTheLog,(void *)(this),0,&threadID)) == INVALID_HANDLE_VALUE)
    {
      m_logThread = NULL;
      TRACE("Cannot make a thread for the LogAnalysis function\n");
    }
  }
}

// Running the main thread of the logfile
// Expects to see IO_COMPLETION
void
LogAnalysis::RunLogAnalysis()
{
  DWORD sync = 0;

  while(m_initialised)
  {
    DWORD res = WaitForSingleObjectEx(m_event,m_interval,true);

    switch(res)
    {
      case WAIT_OBJECT_0:       // Full flushing requested, do it
                                Flush(true);
      case WAIT_IO_COMPLETION:  break;
      case WAIT_ABANDONED:      break;
      case WAIT_TIMEOUT:        // Timeout - see if we must flush
                                // Every fourth round, we do a forced flush
                                Flush((++sync % LOGWRITE_FORCED) == 0);
                                break;
    }
  }
  // Closing our event
  CloseHandle(m_event);
  m_event = NULL;

  // Also ending this thread
  m_logThread = NULL;
}

// Append date time to logfilename
// But only if logfile name ends in .txt
// So it is possible to restart a program many times a day
// and not overwrite last times logging files
void
LogAnalysis::AppendDateTimeToFilename()
{
  CString append;
  __timeb64 now;
  struct tm today;

  _ftime64_s(&now);
  _localtime64_s(&today,&now.time);
  append.Format("_%4.4d%2.2d%2.2d_%2.2d%2.2d%2.2d_%03d.txt"
                ,today.tm_year + 1900
                ,today.tm_mon  + 1
                ,today.tm_mday
                ,today.tm_hour
                ,today.tm_min
                ,today.tm_sec
                ,now.millitm);

  // Finding the .txt extension
  CString file(m_logFileName);
  file.MakeLower();
  int pos = file.Find(".txt");
  if(pos > 0)
  {
    m_logFileName = m_logFileName.Left(pos);
    RemoveLastMonthsFiles(today);
    m_logFileName += append;
  }
}

// Contrary to the name, it removes the files from TWO months ago
// This leaves time on the verge of a new month not to delete yesterdays files
// But it will eventually cleanup the servers log directories.
void
LogAnalysis::RemoveLastMonthsFiles(struct tm& p_today)
{
  // Go two months back
  p_today.tm_mon -= 2;
  if(p_today.tm_mon < 0)
  {
    p_today.tm_mon += 12;
    p_today.tm_year--;
  }

  // Append year and month and a pattern
  CString append;
  append.Format("_%4.4d%2.2d"
                ,p_today.tm_year + 1900
                ,p_today.tm_mon  + 1);

  CString pattern = m_logFileName + append + "*.txt";
  EnsureFile ensure;
  CString direct = ensure.DirectoryPart(pattern);

  // Walk through all the files in the pattern
  // Even works on relative file paths
  intptr_t nHandle = 0;
  struct _finddata_t fileInfo;
  nHandle = _findfirst((LPCSTR)pattern,&fileInfo);
  if(nHandle != -1)
  {
    do
    {
      // Only delete if it's a 'real' file
      if((fileInfo.attrib & _A_SUBDIR) == 0)
      {
        CString fileRemove = direct + fileInfo.name;
        DeleteFile(fileRemove);
      }
    }
    while(_findnext(nHandle,&fileInfo) != -1);
  }
  _findclose(nHandle);
}
