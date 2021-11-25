/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: Analysis.h
//
// Marlin Server: Internet server/client
// 
// Copyright (c) 2014-2021 ir. W.E. Huisman
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
// YYYY-MM-DD HH:MM:SS Function_name..........Formatted string with info
//
//////////////////////////////////////////////////////////////////////////

#pragma once
#include "HTTPLoglevel.h"
#include <deque>
#include <time.h>

constexpr auto ANALYSIS_FUNCTION_SIZE = 48;                            // Size of prefix printing in logfile
constexpr auto LOGWRITE_INTERVAL      = (CLOCKS_PER_SEC * 30);         // Median  is once per 30 seconds
constexpr auto LOGWRITE_INTERVAL_MIN  = (CLOCKS_PER_SEC * 10);         // Minimum is once per 10 seconds
constexpr auto LOGWRITE_INTERVAL_MAX  = (CLOCKS_PER_SEC * 60 * 10);    // Maximum is once per 10 minutes
constexpr auto LOGWRITE_CACHE         = 1000;                          // Median  number of lines cached
constexpr auto LOGWRITE_MINCACHE      = 100;                           // Minimum number of lines cached (below is not efficient)
constexpr auto LOGWRITE_MAXCACHE      = 100000;                        // Maximum number of lines cached (above is too slow)
constexpr auto LOGWRITE_FORCED        = 4;                             // Every x intervals we force a flush
constexpr auto LOGWRITE_MAXHEXCHARS   = 64;                            // Max width of a hexadecimal dump view
constexpr auto LOGWRITE_MAXHEXDUMP    = (32 * 1024);                   // First 32 K of a file or message
constexpr auto LOGWRITE_KEEPFILES     = 128;                           // Keep last 128 logfiles in a directory (2 months, 2 per day)
constexpr auto LOGWRITE_KEEPLOG_MIN   = 10;                            // Keep last 10 logfiles as a minimum
constexpr auto LOGWRITE_KEEPLOG_MAX   = 500;                           // Keep no more than 500 logfiles of a server

// Various types of log events
enum class LogType
{
  LOG_TRACE   = 0
 ,LOG_INFO    = 1
 ,LOG_ERROR   = 2
 ,LOG_WARN    = 3
};

// Caching list for log-lines
using LogList = std::deque<CString>;

class LogAnalysis
{
public:
  LogAnalysis(CString p_name);
 ~LogAnalysis();

  // Log this line
  // Intended to be called as:
  // AnalysisLog(__FUNCTION__,LOG_INFO,true,"My info for the logfile: %s %d",StringParam,IntParam);
  bool    AnalysisLog(const char* p_function,LogType p_type,bool p_doFormat,const char* p_format,...);

  // Hexadecimal view of an object
  // Intended to be called as:
  // AnalysisHex(__FUNCTION__,"MyMessage",buffer,len);
  bool    AnalysisHex(const char* p_function,CString p_name,void* p_buffer,unsigned long p_length,unsigned p_linelength = 16);

  // Use sparringly!
  void    BareStringLog(const char* p_buffer,int p_length);

  // Flushing the log file
  void    ForceFlush();
  // Reset to not-used
  void    Reset();

  // SETTERS
  void    SetLogFilename(CString p_filename,bool p_perUser = false);
  void    SetDoLogging(bool p_logging);
  void    SetLogLevel(int p_logLevel);
  void    SetDoTiming(bool p_doTiming)         { m_doTiming    = p_doTiming; }
  void    SetDoEvents(bool p_doEvents)         { m_doEvents    = p_doEvents; }
  void    SetLogRotation(bool p_rotate)        { m_rotate      = p_rotate;   }
  void    SetKeepfiles(int p_keepfiles);
  void    SetCache   (int  p_cache);
  void    SetInterval(int  p_interval);
  bool    SetBackgroundWriter(bool p_writer);

  // GETTERS
  bool    GetIsOpen();
  bool    GetDoLogging();
  int     GetLogLevel()                        { return m_logLevel;   }
  bool    GetDoEvents()                        { return m_doEvents;   }
  bool    GetDoTiming()                        { return m_doTiming;   }
  CString GetLogFileName()                     { return m_logFileName;}
  int     GetInterval()                        { return m_interval;   }
  bool    GetLogRotation()                     { return m_rotate;     }
  int     GetKeepfiles()                       { return m_keepfiles;  }
  bool    GetBackgroundWriter()                { return m_useWriter;  }
  int     GetCacheSize();
  int     GetCacheMaxSize();

  // INTERNALS ONLY: DO NOT CALL EXTERNALLY
  // Must be public to start a writing thread
  void    RunLog();
  void    RunLogAnalysis();
  static  void WriteEvent(HANDLE p_eventLog,LogType p_type,CString& p_buffer);

private:
  void    Initialisation();
  void    ReadConfig();
  void    AppendDateTimeToFilename();
  void    RemoveLastMonthsFiles(struct tm& today);
  void    RemoveLogfilesKeeping();
  CString CreateUserLogfile(CString p_filename);
  // Writing out a log line
  void    Flush(bool p_all);
  void    WriteLog(CString& p_buffer);

  // Settings
  CString m_name;                               // For WMI Event viewer
  int     m_logLevel    { HLL_NOLOG };          // Active logging level
  bool    m_doTiming    { true  };              // Prepend date-and-time to log-lines
  bool    m_doEvents    { false };              // Also write to WMI event log
  bool    m_rotate      { false };              // Log rotation for server solutions
  bool    m_useWriter   { true  };              // Use thread writer in the background
  size_t  m_cacheMaxSize{ LOGWRITE_CACHE     }; // Number of cached lines
  int     m_interval    { LOGWRITE_INTERVAL  }; // Interval between writes (in seconds)
  int     m_keepfiles   { LOGWRITE_KEEPFILES }; // Keep a maximum of n files in a directory

  // Internal bookkeeping
  bool    m_initialised { false };              // Logging is initialized and ready
  HANDLE  m_logThread   { NULL };               // Async writing threads ID
  HANDLE  m_event       { NULL };               // Event for waking writing thread
  HANDLE  m_file        { NULL };               // File handle to write log to
  HANDLE  m_eventLog    { NULL };               // WMI handle to write event-log to
  CString m_logFileName { "Logfile.txt" };      // Name of the logging file
  LogList m_list;                               // Cached list of logging lines

  // Multi-threading issues
  CRITICAL_SECTION m_lock;
};

