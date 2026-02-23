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
#include <windows.h>

extern "C" BOOL WINAPI DllMain(HINSTANCE const /*instance*/,  // handle to DLL module
                               DWORD     const reason,        // reason for calling function
                               LPVOID    const /*reserved*/)  // reserved
{
  // Perform actions based on the reason for calling.
  switch(reason)
  {
    case DLL_PROCESS_ATTACH:  // Initialize once for each new process.
                              // Return FALSE to fail DLL load.
                              break;
    case DLL_THREAD_ATTACH:   // Do thread-specific initialization.
                              break;
    case DLL_THREAD_DETACH:   // Do thread-specific cleanup.
                              break;
    case DLL_PROCESS_DETACH:  // Perform any necessary cleanup.
                              if(g_session && g_httpsys_initialized)
                              {
                                delete g_session;
                                g_session = nullptr;
                              }
                              break;
  }
  return TRUE;  // Successful DLL_PROCESS_ATTACH.
}
