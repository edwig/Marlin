//////////////////////////////////////////////////////////////////////////
//
// USER-SPACE IMPLEMENTTION OF HTTP.SYS
//
// 2018 - 2024 (c) ir. W.E. Huisman
// License: MIT
//
//////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "http_private.h"
#include "ServerSession.h"
#include "UrlGroup.h"
#include "Logging.h"
#include "HTTPReadRegister.h"
#include "RequestQueue.h"
#include "OpaqueHandles.h"
#include <LogAnalysis.h>
#include <ConvertWideString.h>
#include <winhttp.h>

// CTOR of HTTP Server session
ServerSession::ServerSession()
{
  assert(g_session == nullptr);

  ReadRegistrySettings();
  InitializeCriticalSection(&m_lock);
}

ServerSession::~ServerSession()
{
  assert(g_session);

  // Remove all URL groups
  do
  {
    AutoCritSec lock(&m_lock);

    for (auto& group : m_groups)
    {
      delete group;
    }
  } 
  while (false);

  // Remove the logfile
  if(m_logfile)
  {
    HANDLE writer = m_logfile->GetBackgroundWriterThread();

    LogAnalysis::DeleteLogfile(m_logfile);
    m_logfile = nullptr;

    if(writer)
    {
      WaitForSingleObject(writer,10 * CLOCKS_PER_SEC);
    }
  }

  DeleteCriticalSection(&m_lock);
}

LPCSTR
ServerSession::GetServerVersion()
{
  if(!m_server[0])
  {
    sprintf_s(m_server,50,"Marlin-HTTPAPI/%s Version: %s",VERSION_HTTPAPI,VERSION_HTTPSYS);
  }
  return m_server;
}


ULONG
ServerSession::AddUrlGroup(UrlGroup* p_group)
{
  AutoCritSec lock(&m_lock);

  m_groups.push_back(p_group);
  return (ULONG)m_groups.size();
}

bool
ServerSession::RemoveUrlGroup(HTTP_URL_GROUP_ID p_handle,UrlGroup* p_group)
{
  AutoCritSec lock(&m_lock);

  for(UrlGroups::iterator it = m_groups.begin();it != m_groups.end(); ++it)
  {
    if(*it == p_group)
    {
      // Remove UrlGroup from global handles as soon as possible
      g_handles.RemoveOpaqueHandle((HANDLE)p_handle);

      // Remove group from all queues
      HandleMap map;
      g_handles.GetAllQueueHandles(map);
      for(auto& handle : map)
      {
        RequestQueue* queue = g_handles.GetReQueueFromOpaqueHandle(handle.first);
        if(queue)
        {
          queue->RemoveURLGroup(p_group);
        }
      }

      // Remove the group
      delete p_group;
      m_groups.erase(it);
      return true;
    }
  }
  return false;
}

// Changing our state means changing the state of all URL groups!
void
ServerSession::SetEnabledState(HTTP_ENABLED_STATE p_state)
{
  AutoCritSec lock(&m_lock);

  for(auto& group : m_groups)
  {
    group->SetEnabledState(p_state);
  }
  m_state = p_state;
}

void 
ServerSession::SetSocketLogging(int p_logging)
{
  if(p_logging >= SOCK_LOGGING_OFF && p_logging <= SOCK_LOGGING_FULLTRACE)
  {
    m_socketLogging = p_logging;
  }
}

void 
ServerSession::SetTimeoutEntityBody(USHORT p_timeout)
{ 
  AutoCritSec lock(&m_lock);

  for(auto& group : m_groups)
  {
    group->SetTimeoutEntityBody(p_timeout);
  }
  m_timeoutEntityBody = p_timeout;
}

void 
ServerSession::SetTimeoutDrainBody(USHORT p_timeout)
{ 
  AutoCritSec lock(&m_lock);

  for(auto& group : m_groups)
  {
    group->SetTimeoutDrainBody(p_timeout);
  }
  m_timeoutDrainEntityBody = p_timeout;
}

void 
ServerSession::SetTimeoutRequestQueue(USHORT p_timeout)
{ 
  AutoCritSec lock(&m_lock);

  for(auto& group : m_groups)
  {
    group->SetTimeoutRequestQueue(p_timeout);
  }
  m_timeoutRequestQueue = p_timeout;
}

void 
ServerSession::SetTimeoutIdleConnection(USHORT p_timeout)
{ 
  AutoCritSec lock(&m_lock);

  for(auto& group : m_groups)
  {
    group->SetTimeoutIdleConnection(p_timeout);
  }
  m_timeoutIdleConnection = p_timeout;
}

void 
ServerSession::SetTimeoutHeaderWait(USHORT p_timeout)
{ 
  AutoCritSec lock(&m_lock);

  for(auto& group : m_groups)
  {
    group->SetTimeoutHeaderWait(p_timeout);
  }
  m_timeoutHeaderWait = p_timeout; 
}

void 
ServerSession::SetTimeoutMinSendRate(ULONG  p_rate) 
{
  AutoCritSec lock(&m_lock);

  for(auto& group : m_groups)
  {
    group->SetTimeoutMinSendRate(p_rate);
  }
  m_timeoutMinSendRate = p_rate;
}

void     
ServerSession::SetAuthentication(ULONG p_scheme
                                ,const XString& p_domain
                                ,const XString& p_realm
                                ,const wstring& p_domainW
                                ,const wstring& p_realmW
                                ,bool p_caching)
{
  AutoCritSec lock(&m_lock);

  for(auto& group : m_groups)
  {
    group->SetAuthentication(p_scheme,p_domain,p_realm,p_caching);
    group->SetAuthenticationWide(p_domainW,p_realmW);
  }
}

bool
ServerSession::AddConnection()
{
  if(!m_maxConnections)
  {
    InterlockedIncrement(&m_connections);
    return true;
  }
  if(m_connections < m_maxConnections)
  {
    InterlockedIncrement(&m_connections);
    return true;
  }
  return false;
}

bool
ServerSession::AddEndpoint()
{
  if(!m_maxEndpoints)
  {
    InterlockedIncrement(&m_endpoints);
    return true;
  }
  if(m_endpoints < m_maxEndpoints)
  {
    InterlockedIncrement(&m_endpoints);
    return true;
  }
  return false;
}

void
ServerSession::RemoveConnection()
{
  if(m_connections)
  {
    InterlockedDecrement(&m_connections);
  }
}

void
ServerSession::RemoveEndpoint()
{
  if(m_endpoints)
  {
    InterlockedDecrement(&m_endpoints);
  }
}

bool
ServerSession::SetupForLogging(PHTTP_LOGGING_INFO p_information)
{
  if(p_information->Flags.Present == 0)
  {
    m_socketLogging = SOCK_LOGGING_OFF;
    if(m_logfile)
    {
      LogAnalysis::DeleteLogfile(m_logfile);
      m_logfile = nullptr;
    }
    return true;
  }
  // Set up the log file
  if(m_socketLogging == SOCK_LOGGING_OFF)
  {
    // Not for this session
    return true;
  }
  // Only supported format
  if(p_information->Format != HttpLoggingTypeW3C)
  {
    return false;
  }
  // Must have a software name
  if(p_information->SoftwareNameLength == 0)
  {
    return false;
  }

  // The following is ignored:
  // p_information->Fields

  int loglevel = 3;
  if(p_information->LoggingFlags & HTTP_LOGGING_FLAG_LOG_ERRORS_ONLY)
  {
    loglevel = 1;
  }
  if(p_information->LoggingFlags & HTTP_LOGGING_FLAG_LOG_SUCCESS_ONLY)
  {
    loglevel = 2;
  }
  CStringW softwareNameW(p_information->SoftwareName);
  XString  softwareName(softwareNameW);

  CStringW directoryNameW(p_information->DirectoryName);
  XString  directoryName(directoryNameW);

  bool rotate = false;
  if(p_information->RolloverType >= HttpLoggingRolloverDaily)
  {
    rotate = true;
  }

  // Creating the logfile name
  XString filename(directoryName);
  if(filename.IsEmpty())
  {
    if(!filename.GetEnvironmentVariable(_T("WINDIR")))
    {
      filename = _T("C:\\Windows");
    }
    filename += _T("\\TEMP");
  }
  filename += _T("\\");
  filename += softwareName;
  filename += _T(".txt");

  m_logfile = LogAnalysis::CreateLogfile(softwareName);
  m_logfile->SetLogRotation(rotate);
  m_logfile->SetLogLevel(m_socketLogging = loglevel);
  m_logfile->SetLogFilename(filename);

  return true;
}

//////////////////////////////////////////////////////////////////////////
//
// GLOBAL LOGGING FUNCTIONS
//
//////////////////////////////////////////////////////////////////////////

void
ServerSession::ProcessLogData(PHTTP_LOG_DATA p_data)
{
  PHTTP_LOG_FIELDS_DATA log = reinterpret_cast<PHTTP_LOG_FIELDS_DATA>(p_data);

  // See if it is the correct subtype
  // For now the HTTPAPI only knows of one (1) type
  if(log->Base.Type != HttpLogDataTypeFields)
  {
    return;
  }
  // Are we doing a logfile?
  if(m_logfile == nullptr)
  {
    return;
  }
  // Check if we should only log the errors
  if(m_socketLogging == 1 && log->ProtocolStatus < HTTP_STATUS_BAD_REQUEST)
  {
    return;
  }

  // Printing W3C Format (approximately)
  // referrer username client-ip method port uri-stem uri-query protocol-status sub-status (win32status)
  CStringA line;
  line.Format("%s %s %s %s %s %d.%d (%ld)"
             ,log->Referrer
             ,(PCHAR)log->UserName
             ,log->ClientIp
             ,log->Method
             ,(PCHAR)log->UriStem
             ,log->ProtocolStatus
             ,log->SubStatus
             ,log->Win32Status);

  LogType type = log->ProtocolStatus >= HTTP_STATUS_BAD_REQUEST ? LogType::LOG_ERROR : LogType::LOG_INFO;

#ifdef _UNICODE
  XString theline(line);
  m_logfile->AnalysisLog(_T("HTTPSYS"),type,false,theline.GetString());
#else
  m_logfile->AnalysisLog(_T("HTTPSYS"),type,false,line.GetString());
#endif
}

static void PrintHexDumpActual(DWORD p_length,const void* p_buffer)
{
  DWORD count = 0;
	TCHAR digits[] = _T("0123456789abcdef");
	TCHAR line[100];
	int   pos = 0;
	const byte* buffer = static_cast<const byte *>(p_buffer);

  // Only print the 'full' message at the highest level
  // otherwise, just print the first line
  if((g_session && g_session->GetSocketLogging() < SOCK_LOGGING_FULLTRACE) && (p_length > 16))
  {
    p_length = 16;
  }

	for(int index = 0; p_length; p_length -= count, buffer += count, index += count) 
	{
    DWORD i = 0;
		count = (p_length > 16) ? 16:p_length;

		_stprintf_s(line, sizeof(line),_T("%4.4x  "), index);
		pos = 6;

		for(i = 0; i < count;i++) 
		{
			line[pos++] = digits[buffer[i] >> 4];
			line[pos++] = digits[buffer[i] & 0x0f];
			if(i == 7) 
			{
				line[pos++] = ':';
			} 
			else 
			{
				line[pos++] = ' ';
			}
		}
		for(; i < 16; i++) 
		{
			line[pos++] = ' ';
			line[pos++] = ' ';
			line[pos++] = ' ';
		}

		line[pos++] = ' ';

		for(i = 0; i < count; i++) 
		{
      if(buffer[i] < 32 || buffer[i] > 126 || buffer[i] == '%')
      {
        line[pos++] = '.';
      }
      else
      {
        line[pos++] = buffer[i];
      }
		}
		line[pos++] = 0;
		DebugMsg(line);
	}
}

void DebugPrintHexDump(DWORD p_length, const void* p_buffer)
{
  if(g_session && g_session->GetSocketLogging() >= SOCK_LOGGING_FULLTRACE)
  {
    PrintHexDumpActual(p_length,p_buffer);
  }
}

//////////////////////////////////////////////////////////////////////////
//
// PRIVATE
//
//////////////////////////////////////////////////////////////////////////

// Reading the general registry settings
void    
ServerSession::ReadRegistrySettings()
{
  XString value1;
  XString sectie;
  DWORD   value2 = 0;
  TCHAR   value3[BUFF_LEN];
  DWORD   size3 = BUFF_LEN;

  // Usage of the HTTP header "Server:"
  // 0 -> Add our own server header "Marlin-HTTPAPI/2.0"
  // 1 -> Only add our own server header if status code is < 400
  // 2 -> Remove the server header altogether
  if(HTTPReadRegister(sectie,_T("DisableServerHeader"),REG_DWORD,value1,&value2,value3,&size3))
  {
    if(value2 >= 0 && value2 <= 2)
    {
      m_disableServerHeader = value2;
    }
  }

  // Maximum number of live connections to service clients
  // Normally between 1024 and 2031616 
  // Default = 0 (no restrictions)
  if(HTTPReadRegister(sectie,_T("MaxConnections"),REG_DWORD,value1,&value2,value3,&size3))
  {
    if(value2 >= SESSION_MIN_CONNECTIONS && value2 <= SESSION_MAX_CONNECTIONS)
    {
      m_maxConnections = value2;
    }
  }

  // Maximum number of URL endpoints to service
  // Normally between 1 and 1024
  // Default = 0 (no restrictions)
  if(HTTPReadRegister(sectie,_T("MaxEndpoints"),REG_DWORD,value1,&value2,value3,&size3))
  {
    if(value2 <= SESSION_MAX_ENDPOINTS)
    {
      m_maxEndpoints = value2;
    }
  }

  // Logging level
  // 0    No logging
  // 1    Error logging (HTTP status 400 and above)
  // 2    Log all actions
  // 3    Full hex dump tracing
  if(HTTPReadRegister(sectie,_T("Logging"),REG_DWORD,value1,&value2,value3,&size3))
  {
    if(value2 >= SOCK_LOGGING_OFF && value2 <= SOCK_LOGGING_FULLTRACE)
    {
      m_socketLogging = value2;
    }
  }

  // Max number of bytes for a header field
  // Normally 16k (default) upto 64k
  if(HTTPReadRegister(sectie,_T("MaxFieldLength"),REG_DWORD,value1,&value2,value3,&size3))
  {
    if(value2 >= SESSION_DEF_FIELDLENGTH && value2 <= SESSION_MAX_FIELDLENGTH)
    {
      m_maxFieldLength = value2;
    }
  }

  // Max number of bytes of the initial request (HTTP line + URL + headers)
  // Normally 16k (default) upto 16MB
  if(HTTPReadRegister(sectie,_T("MaxRequestBytes"),REG_DWORD,value1,&value2,value3,&size3))
  {
    if(value2 >= SESSION_DEF_REQUESTBYTES && value2 <= SESSION_MAX_REQUESTBYTES)
    {
      m_maxRequestBytes = value2;
      if(m_maxRequestBytes < m_maxFieldLength)
      {
        m_maxFieldLength = m_maxRequestBytes;
      }
    }
  }
}
