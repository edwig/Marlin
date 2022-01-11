/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: CreateURLPrefix.cpp
//
// Marlin Server: Internet server/client
// 
// Copyright (c) 2014-2022 ir. W.E. Huisman
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
// HOSTNAME_SHORT  -> e.g. "myMachine" etc
// HOSTNAME_FULL   -> e.g. "myMachine.network.org"
//
CString GetHostName(int p_type)
{
  char  buffer[MAX_PATH + 1];
  DWORD size = MAX_PATH;
  COMPUTER_NAME_FORMAT format = p_type == HOSTNAME_FULL ? ComputerNameDnsFullyQualified : ComputerNameDnsDomain;

  if(::GetComputerNameEx(format,buffer,&size))
  {
    return CString(buffer);
  }
  return "";
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
