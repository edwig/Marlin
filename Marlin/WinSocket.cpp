#include "stdafx.h"
#include "WinSocket.h"

void _cdecl MarlinCleanupWinsocket()
{
  WSACleanup();
}

bool MarlinStartupWinsocket()
{
  WSADATA wsaData;

  // Startup WinSockets
  // Recent versions of MS-Windows use WinSock 2.2 library
  WORD wVersionRequested = MAKEWORD(2,2);
  if(WSAStartup(wVersionRequested,&wsaData))
  {
    // Could not start WinSockets?
    return false;
  }

  // Register for cleanup when closing program
  atexit(MarlinCleanupWinsocket);

  return true;
}