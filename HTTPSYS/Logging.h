#pragma once
#include "http_private.h"
#include "ServerSession.h"
#include "Logfile.h"

#define DebugMsg(p_message,...) if(g_session) g_session->GetLogfile()->WriteLog(__FUNCTION__,LogType::LOG_INFO, true,p_message,__VA_ARGS__)
#define LogError(p_message,...) if(g_session) g_session->GetLogfile()->WriteLog(__FUNCTION__,LogType::LOG_ERROR,true,p_message,__VA_ARGS__)
void    PrintHexDump(DWORD p_length, const void* p_buffer);


