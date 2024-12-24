#pragma once
#include "http_private.h"
#include "ServerSession.h"
#include <LogAnalysis.h>

void    PrintHexDump(DWORD p_length,const void* p_buffer);

#ifdef _DEBUG
#define DebugMsg(p_message,...) if(g_session->GetSocketLogging()) g_session->GetLogfile()->AnalysisLog(_T(__FUNCTION__),LogType::LOG_INFO, true,p_message,__VA_ARGS__)
#define LogError(p_message,...) if(g_session->GetSocketLogging()) g_session->GetLogfile()->AnalysisLog(_T(__FUNCTION__),LogType::LOG_ERROR,true,p_message,__VA_ARGS__)
#else
#define DebugMsg(p_message,...) 
#define LogError(p_message,...) 
#endif
