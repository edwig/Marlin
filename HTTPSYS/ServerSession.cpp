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
#include "ServerSession.h"
#include "UrlGroup.h"
#include "Logfile.h"
#include "Logging.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// CTOR of HTTP Server session
ServerSession::ServerSession()
{
  ASSERT(g_session == nullptr);

  CreateLogfile();
  InitializeCriticalSection(&m_lock);
}

ServerSession::~ServerSession()
{
  ASSERT(g_session);

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
    delete m_logfile;
    m_logfile = nullptr;
  }

  DeleteCriticalSection(&m_lock);
}

void
ServerSession::CreateLogfile()
{
  CString name = ("HTTP_Server");
  m_logfile = new Logfile(name);
  m_logfile->SetLogRotation(true);
  m_logfile->SetLogLevel(m_socketLogging = SOCK_LOGGING_FULLTRACE);

  CString filename;
  filename.GetEnvironmentVariable("WINDIR");
  filename += "\\TEMP\\HTTP_Server.txt";

  m_logfile->SetLogFilename(filename);
}

ULONG
ServerSession::AddUrlGroup(UrlGroup* p_group)
{
  AutoCritSec lock(&m_lock);

  m_groups.push_back(p_group);
  return (ULONG)m_groups.size();
}

bool
ServerSession::RemoveUrlGroup(UrlGroup* p_group)
{
  AutoCritSec lock(&m_lock);

  for(UrlGroups::iterator it = m_groups.begin();it != m_groups.end(); ++it)
  {
    if(*it == p_group)
    {
      delete p_group;
      m_groups.erase(it);
      return true;
    }
  }
  return false;
}


void 
ServerSession::SetSocketLogging(int p_logging)
{
  if(p_logging >= SOCK_LOGGING_OFF && p_logging <= SOCK_LOGGING_FULLTRACE)
  {
    m_socketLogging = p_logging;
  }
}

//////////////////////////////////////////////////////////////////////////
//
// GLOBAL LOGGING FUNCTIONS
//
//////////////////////////////////////////////////////////////////////////

static void PrintHexDumpActual(DWORD p_length,const void* p_buffer)
{
  DWORD count = 0;
	CHAR  digits[] = "0123456789abcdef";
	CHAR  line[100];
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

		sprintf_s(line, sizeof(line), "%4.4x  ", index);
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

void PrintHexDump(DWORD p_length, const void* p_buffer)
{
  if(g_session && g_session->GetSocketLogging() >= SOCK_LOGGING_TRACE)
  {
    PrintHexDumpActual(p_length,p_buffer);
  }
}
