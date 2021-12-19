/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: CreateURLPrefix.cpp
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
#include "StdAfx.h"
#include <ws2tcpip.h>
#include "CreateURLPrefix.h"
#include <winhttp.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// Host name can be cached. 
// Machine is going nowhere while program is running :-)
static int  g_hostType = 0;
static char g_hostName[NI_MAXHOST] = "";

// Requesting the current hostname
// Host names can be returned in three formats:
//
// HOSTNAME_SHORT  -> e.g. "localhost", "myMachine" etc
// HOSTNAME_IP     -> e.g. "123.67.18.12"
// HOSTNAME_FULL   -> e.g. "myMachine.network.org"
//
CString GetHostName(int p_type)
{
  WORD wVersionRequested;
  WSADATA wsaData;
  DWORD ipaddr = 0;

  // Requesting the same name type again. Use the cached 
  if(g_hostName[0] && g_hostType == p_type)
  {
    return CString(g_hostName);
  }
  g_hostType = p_type;

  // Startup WinSockets
  wVersionRequested = MAKEWORD(2,2);
  if(WSAStartup(wVersionRequested,&wsaData))
  {
    // Could not start WinSockets?
    return "";
  }

  // Get hour host name (short form)
  if(gethostname(g_hostName,NI_MAXHOST) != 0)
  {
    strcpy_s(g_hostName,NI_MAXHOST,"localhost");
  }

  // Only named form requested
  if(p_type == HOSTNAME_SHORT)
  {
    WSACleanup();
    return CString(g_hostName);
  }

  // Find IP address by way of addressinfo
  struct addrinfo aiHints;
  struct addrinfo *aiList = NULL;
  const char*  port = "80";

  // Setup the hints address info structure
  // which is passed to the getaddrinfo() function
  memset(&aiHints, 0, sizeof(aiHints));
  aiHints.ai_family   = AF_INET;
  aiHints.ai_socktype = SOCK_STREAM;
  aiHints.ai_protocol = IPPROTO_TCP;

  if(getaddrinfo(g_hostName,port,&aiHints,&aiList) != 0)
  {
    // Cleanup again. Server will do Startup later
    WSACleanup();
    strcpy_s(g_hostName,NI_MAXHOST,"127.0.0.1");
    return CString(g_hostName);
  }

  // Get the IP address in network order
  ipaddr = *((DWORD*) &aiList->ai_addr->sa_data[2]);

  if(p_type == HOSTNAME_IP)
  {
    // Cleanup again. Server will do Startup later
    WSACleanup();
    // Convert to string
    struct in_addr addr;
    addr.S_un.S_addr  = ipaddr;

    g_hostName[0] = 0;
    inet_ntop(AF_INET,(PVOID)&addr,g_hostName,NI_MAXHOST);
    return CString(g_hostName);
  }

  // Find Full Qualified server name
  // Can time-out a long time (3 seconds or more)
  getnameinfo(aiList->ai_addr
             ,sizeof(sockaddr_in)
             ,g_hostName
             ,NI_MAXHOST
             ,NULL
             ,0
             ,NI_NAMEREQD);

  // Cleanup again. Server will do Startup later
  WSACleanup();
  return CString(g_hostName);
}

CString
SocketToServer(PSOCKADDR_IN6 p_address)
{
  char host[NI_MAXHOST+1] = { 0 };
  // Translate IPV4, IPV6 address to the hostname
  int res = getnameinfo((PSOCKADDR)p_address
                       ,sizeof(SOCKADDR_IN6)
                       ,host  // The hostname
                       ,NI_MAXHOST
                       ,NULL  // No service name
                       ,0
                       ,NI_NUMERICHOST | NI_NUMERICSERV);

  if(res == WSANOTINITIALISED)
  {
    WSADATA wsaData;
    WORD    wVersionRequested = MAKEWORD(2,2);
    if(WSAStartup(wVersionRequested,&wsaData))
    {
      return "";
    }

    // Try again
    res = getnameinfo((PSOCKADDR)p_address
                      ,sizeof(SOCKADDR_IN6)
                      ,host  // hostname
                      ,NI_MAXHOST
                      ,NULL  // No service name
                      ,0
                      ,NI_NUMERICHOST | NI_NUMERICSERV);
  }
  if(res == 0)
  {
    // See if we need to make a FULL Teredo address of this
    if(p_address && p_address->sin6_family == AF_INET6)
    {
      CString hostname;
      hostname.Format("[%x:%x:%x:%x:%x:%x:%x:%x]"
                     ,ntohs(p_address->sin6_addr.u.Word[0])
                     ,ntohs(p_address->sin6_addr.u.Word[1])
                     ,ntohs(p_address->sin6_addr.u.Word[2])
                     ,ntohs(p_address->sin6_addr.u.Word[3])
                     ,ntohs(p_address->sin6_addr.u.Word[4])
                     ,ntohs(p_address->sin6_addr.u.Word[5])
                     ,ntohs(p_address->sin6_addr.u.Word[6])
                     ,ntohs(p_address->sin6_addr.u.Word[7]));
      return hostname;
    }
    return CString(host);
  }
  return "";
}

CString 
CreateURLPrefix(PrefixType p_type
               ,bool       p_secure
               ,int        p_port
               ,CString    p_path)
{
  CString prefix;
  // Empty result and set protocol
  prefix = p_secure ? "https://" : "http://";

  switch(p_type)
  {
    case PrefixType::URLPRE_Strong: prefix += "+";
                                    break;
    case PrefixType::URLPRE_Weak:   prefix += "*";
                                    break;
    case PrefixType::URLPRE_Named:  prefix += GetHostName(HOSTNAME_SHORT);
                                    break;
    case PrefixType::URLPRE_Address:prefix += GetHostName(HOSTNAME_IP);
                                    break;
    case PrefixType::URLPRE_FQN:    prefix += GetHostName(HOSTNAME_FULL);
                                    break;
    default:                        prefix.Empty();
                                    return prefix;

  }
  // Add port 
  CString portNum;
  portNum.Format(":%d",p_port);
  prefix += portNum;

  // Check path naming and append
  if(!p_path.IsEmpty() && p_path.Left(1) != "/")
  {
    p_path = "/" + p_path;
  }
  if(!p_path.IsEmpty() && p_path.Right(1) != "/")
  {
    p_path += "/";
  }
  prefix += p_path;

  // Ready
  return prefix;
}
