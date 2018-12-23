//////////////////////////////////////////////////////////////////////////
//
// USER-SPACE IMPLEMENTTION OF HTTP.SYS
//
// 2018 (c) ir. W.E. Huisman
// License: MIT
//
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
//
// Logs in the format:
// YYYY-MM-DD HH:MM:SS Function_name..........Formatted string with info
//
//////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "http_private.h"
#include "Logfile.h"
#include "GetUserAccount.h"
#include <string.h>
#include <time.h>
#include <sys/timeb.h>
#include <io.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

Logfile::Logfile(CString p_name)
        :m_name(p_name)
{
  InitializeCriticalSection(&m_lock);
}

Logfile::~Logfile()
{
  Reset();
  DeleteCriticalSection(&m_lock);
}

void
Logfile::Reset()
{
  // Tell we closed 'normally'
  if(m_initialised)
  {
    WriteLog("Logfile is:", LogType::LOG_INFO,false,"Closed");
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

  // Reset loglevel
  m_logLevel = HLL_NOLOG;
}

CString
Logfile::CreateUserLogfile(CString p_filename)
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
  CString user(HTTP_GetUserAccount(NameUnknown,true));
  filepart += "_";
  filepart += user;
  filepart += extensie;

  return filepart;
}

void    
Logfile::SetLogFilename(CString p_filename,bool p_perUser /*=false*/)
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

// Old interface: Logging is on or off
bool
Logfile::GetDoLogging()
{
  return m_logLevel >= HLL_ERRORS;
}

// Old interface, simply set logging or turn it of
void
Logfile::SetDoLogging(bool p_logging)
{
  m_logLevel = p_logging ? HLL_LOGGING : HLL_NOLOG;
}

void
Logfile::SetCache(int p_cache)
{
  m_cache = p_cache;

  if(m_cache < LOGWRITE_MINCACHE) m_cache = LOGWRITE_MINCACHE;
  if(m_cache > LOGWRITE_MAXCACHE) m_cache = LOGWRITE_MAXCACHE;
}

void
Logfile::SetInterval(int p_interval)
{
  m_interval = p_interval;

  if(m_interval < LOGWRITE_INTERVAL_MIN) m_interval = LOGWRITE_INTERVAL_MIN;
  if(m_interval > LOGWRITE_INTERVAL_MAX) m_interval = LOGWRITE_INTERVAL_MAX;
}

// Setting our logging level
void
Logfile::SetLogLevel(int p_logLevel)
{
  // Check for correct boundaries
  if(p_logLevel < HLL_NOLOG)   p_logLevel = HLL_NOLOG;
  if(p_logLevel > HLL_HIGHEST) p_logLevel = HLL_HIGHEST;

  // Remember our loglevel;
  if(m_logLevel != p_logLevel)
  {
    // Record new loglevel
    m_logLevel = p_logLevel;

    // Only show new loglevel if logfile was already started
    if(m_initialised)
    {
      CString level;
      switch(p_logLevel)
      {
        case HLL_NOLOG:     level = "No logging";        break;
        case HLL_ERRORS:    level = "Errors & warnings"; break;
        case HLL_LOGGING:   level = "Standard logging";  break;
        case HLL_LOGBODY:   level = "Logging bodies";    break;
        case HLL_TRACE:     level = "Tracing";           break;
        case HLL_TRACEDUMP: level = "Tracing & hexdump"; break;
      }
      WriteLog(__FUNCTION__,LogType::LOG_INFO,true,"Logging level set to [%d:%s]",m_logLevel,level.GetString());
    }
  }
}

size_t
Logfile::GetCacheSize()
{
  AutoCritSec lock(&m_lock);
  return m_list.size();
};

// Initialize our logfile/log event
void
Logfile::Initialisation()
{
  // Still something to do?
  if(m_initialised || m_logLevel == HLL_NOLOG)
  {
    return;
  }

  // Read the config file (if any)
  ReadConfig();

  // We are now initialized
  // Must do this here, otherwise an endless loop will occur..
  m_initialised = true;

  // Try to register the MS-Windows event log  for writing
  if(m_doEvents)
  {
    // Use standard application event log
    m_eventLog = RegisterEventSource(NULL,m_name);
    if(m_eventLog == NULL)
    {
      m_doEvents = false;
    }
  }

  // Append date time to log's filename
  // And also clean up logfiles that are too old.
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
                     ,FILE_ATTRIBUTE_NORMAL
                     ,NULL);
  if(m_file ==  INVALID_HANDLE_VALUE)
  {
    CString file;
    if(file.GetEnvironmentVariable("TMP"))
    {
      if(file.Right(1) != "\\") file += "\\";
      file += "Analysis.log";

      // Open the logfile in shared writing mode
      // more applications can write to the file
      m_file = CreateFile(file
                         ,GENERIC_WRITE
                         ,FILE_SHARE_READ | FILE_SHARE_WRITE
                         ,NULL              // Security
                         ,CREATE_ALWAYS     // Always throw away old log
                         ,FILE_ATTRIBUTE_NORMAL
                         ,NULL);
      if(m_file == INVALID_HANDLE_VALUE)
      {
        // Give up. Cannot create a logfile
        m_file = NULL;
        m_logLevel = HLL_NOLOG;
        return;
      }
      else
      {
        // Alternative file created
        m_logFileName = file;
      }
    }
  }

  if(m_file)
  {
    // Write a BOM to the logfile, so we can read logged UTF-8 strings
    DWORD written = 0;		// Not optional for MS-Windows Server 2012!!
    CString bom = ConstructBOM();
    WriteFile(m_file,bom.GetString(),bom.GetLength(),&written,nullptr);

    // Starting the log writing thread
    RunLog();
  }
  // Tell that we are now running
  WriteLog("Logfile now running for:", LogType::LOG_INFO,false,m_name);
}

// PRIMARY FUNCTION TO WRITE A LINE TO THE LOGFILE
bool
Logfile::WriteLog(const char* p_function,LogType p_type,bool p_doFormat,const char* p_format,...)
{
  // Multi threaded protection
  AutoCritSec lock(&m_lock);
  CString logBuffer;
  bool result = false;

  // Make sure the system is initialized
  Initialisation();

  // Make sure we ARE logging
  if(m_logLevel == HLL_NOLOG)
  {
    return result;
  }

  // Check on the loglevel
  if(m_logLevel == HLL_ERRORS && (p_type == LogType::LOG_INFO || p_type == LogType::LOG_TRACE))
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
    logBuffer.Append("                                            "
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
  logBuffer += "\r\n";

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

// PRIMARY FUNCTION TO WRITE A LINE TO THE MS-WINDOWS EVENT LOG
void
Logfile::WriteEvent(HANDLE p_eventLog,LogType p_type,CString& p_buffer)
{
  LPCTSTR lpszStrings[2];
  lpszStrings[0] = p_buffer.GetString();
  lpszStrings[1] = nullptr;
  ReportEvent(p_eventLog,static_cast<WORD>(p_type),0,0,NULL,1,0,lpszStrings,0);
}

// Hexadecimal view of an object added to the logfile
bool    
Logfile::WriteLogHex(const char* p_function,CString p_name,void* p_buffer,unsigned long p_length,unsigned p_linelength /*=16*/)
{
  // Only dump in the logfile, not to the MS-Windows event log
  if(!m_file || m_logLevel < HLL_TRACEDUMP)
  {
    return false;
  }

  // Check parameters
  if(p_linelength > LOGWRITE_MAXHEXCHARS)
  {
    p_linelength = LOGWRITE_MAXHEXCHARS;
  }
  if(p_length > LOGWRITE_MAXHEXDUMP)
  {
    p_length = LOGWRITE_MAXHEXDUMP;
  }

  // Multi threaded protection
  AutoCritSec lock(&m_lock);

  // Name of the object
  WriteLog(p_function,LogType::LOG_TRACE,true,"Hexadecimal view of: %s. Length: %d",p_name.GetString(),p_length);

  unsigned long  pos    = 0;
  unsigned char* buffer = static_cast<unsigned char*>(p_buffer);

  while(pos < p_length)
  {
    unsigned len = 0;
    CString hexadLine;
    CString asciiLine;

    // Format one hexadecimal view line
    while(pos < p_length && len < p_linelength)
    {
      // One byte at the time
      hexadLine.AppendFormat("%2.2X ",*buffer);
      if(*buffer)
      {
        asciiLine += *buffer;
      }
      // Next byte in the buffer
      ++buffer;
      ++pos;
      ++len;
    }

    // In case of an incomplete last line
    while(len++ < p_linelength)
    {
      hexadLine += "   ";
    }
    asciiLine.Replace("\r","#");
    asciiLine.Replace("\n","#");

    // Add to the list buffer
    CString line(hexadLine + asciiLine + "\n");
    m_list.push_back(line);
  }
  // Large object now written to the buffer. Force write it
  ForceFlush();

  return true;
}

// Use sparingly!
// Dump string buffer in the log
void
Logfile::BareStringLog(const char* p_buffer,int p_length)
{
  // Multi threaded protection
  AutoCritSec lock(&m_lock);

  CString buffer;
  char* pointer = buffer.GetBufferSetLength(p_length + 1);
  memcpy_s(pointer,p_length+1,p_buffer,p_length);
  pointer[p_length] = 0;
  buffer.ReleaseBufferSetLength(p_length);

  // Test for newline
  if(buffer.Right(1) != L"\n")
  {
    buffer += L"\n";
  }

  // Keep the line
  m_list.push_back(buffer);
}

// Construct an UTF-8 Byte-Order-Mark
CString 
Logfile::ConstructBOM()
{
  CString bom;

  // 3 BYTE UTF-8 Byte-order-Mark "\0xEF\0xBB\0xBF"
  // Strange construction, but the only way to do it
  // because a CString is a signed char field
  bom.GetBufferSetLength(4);
  bom.SetAt(0,(unsigned char)0xEF);
  bom.SetAt(1,(unsigned char)0xBB);
  bom.SetAt(2,(unsigned char)0xBF);
  bom.SetAt(3,0);
  bom.ReleaseBuffer(3);

  return bom;
}

// Force flushing of the logfile
void
Logfile::ForceFlush()
{
  SetEvent(m_event);
}

// At certain times, you may wish to flush the cache
void    
Logfile::Flush(bool p_all)
{
  // See if we have a file at all
  if(m_file == nullptr)
  {
    return;
  }

  // Multi threaded protection
  AutoCritSec lock(&m_lock);

  try
  {
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
          buffer += m_list.front();
          m_list.pop_front();
        }
        // Write out the buffer
        WriteLog(buffer);
      }
    }
  }
  catch(std::exception& er)
  {
    // Logfile failed. Where to log this??
    TRACE("%s\n",er.what());
  }
}

// Write out a log line
void
Logfile::WriteLog(CString& p_buffer)
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
Logfile::ReadConfig()
{
  char buffer[256] = "";
  CString fileName = "Logfile.config";

  FILE* file = NULL;
  fopen_s(&file,fileName,"r");
  if(file)
  {
    while(fgets(buffer,255,file))
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
      if(_strnicmp(buffer,"loglevel=",8) == 0)
      {
        m_logLevel = atoi(&buffer[8]) > 0;
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
    if(m_logLevel < HLL_NOLOG)             m_logLevel = HLL_NOLOG;
    if(m_logLevel > HLL_HIGHEST)           m_logLevel = HLL_HIGHEST;
    if(m_cache    < LOGWRITE_MINCACHE)     m_cache    = LOGWRITE_MINCACHE;
    if(m_cache    > LOGWRITE_MAXCACHE)     m_cache    = LOGWRITE_MAXCACHE;
    if(m_interval < LOGWRITE_INTERVAL_MIN) m_interval = LOGWRITE_INTERVAL_MIN;
    if(m_interval > LOGWRITE_INTERVAL_MAX) m_interval = LOGWRITE_INTERVAL_MAX;
  }
}

static unsigned int
__stdcall StartingTheLog(void* pParam)
{
  Logfile* theLog = reinterpret_cast<Logfile*> (pParam);
  theLog->RunLogAnalysis();
  return 0;
}

// Running the logfile in a separate thread
void
Logfile::RunLog()
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
void
Logfile::RunLogAnalysis()
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
  // Flush out the rest
  Flush(true);
  // Closing our event
  CloseHandle(m_event);
  m_event = NULL;

  // Also ending this thread
  m_logThread = NULL;
}

// Append date time to log's filename
// But only if logfile name ends in .TXT
// So it is possible to restart a program many times a day
// and not overwrite last times logging files
void
Logfile::AppendDateTimeToFilename()
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

CString
Logfile::DirectoryPart(CString fullpath)
{
  char drive [_MAX_DRIVE + 1];
  char direct[_MAX_DIR   + 1];
  char fname [_MAX_FNAME + 1];
  char extens[_MAX_EXT   + 1];

  _splitpath_s(fullpath.GetString(),drive,direct,fname,extens);
  CString directory = CString(drive) + CString(direct);
  return directory;
}

// Contrary to the name, it removes the files from TWO months ago
// This leaves time on the verge of a new month not to delete yesterdays files
// But it will eventually cleanup the servers log directories.
void
Logfile::RemoveLastMonthsFiles(struct tm& p_today)
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
  CString direct  = DirectoryPart(pattern);

  // Walk through all the files in the pattern
  // Even works on relative file paths
  intptr_t nHandle = 0;
  struct _finddata_t fileInfo;
  nHandle = _findfirst(pattern.GetString(),&fileInfo);
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
