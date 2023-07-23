// From Network Programming for Microsoft Windows, Second Edition by 
// Anthony Jones and James Ohlund.  
// Copyright 2002.   Reproduced by permission of Microsoft Press.  
// All rights reserved.
//
// Sample: IPv4 and IPv6 Ping Sample
//
// Files:
//    iphdr.h       - IPv4 and IPv6 packet header definitions
//    ping.cpp      - this file
//    resolve.cpp   - Common name resolution routine
//    resolve.h     - Header file for common name resolution routines
//
// Description:
//    This sample illustrates how to use raw sockets to send ICMP
//    echo requests and receive their response. This sample performs
//    both IPv4 and IPv6 ICMP echo requests. When using raw sockets,
//    the protocol value supplied to the socket API is used as the
//    protocol field (or next header field) of the IP packet. Then
//    as a part of the data submitted to sendto, we include both
//    the ICMP request and data.
//
//    For IPv4 the IP record route option is supported via the 
//    IP_OPTIONS socket option.
//
#include "pch.h"

#ifdef _IA64_
#pragma warning (disable: 4267)
#endif

#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <stdlib.h>
#include <strsafe.h>
#include "WinPingIPHeader.h"
#include "WinPing.h"
#include "HPFCounter.h"

#define DEFAULT_RECV_TIMEOUT   6000     // six second
#define MAX_RECV_BUF_LEN       0xFFFF   // Max incoming packet size.

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//
// Function: PrintAddress
//
// Description:
//    This routine takes a SOCKADDR structure and its length and prints
//    converts it to a string representation. This string is printed
//    to the console via stdout.
//
static int PrintAddress(SOCKADDR* sa,int salen,char* p_errorbuffer)
{
  char host[NI_MAXHOST];
  char serv[NI_MAXSERV];
  int  hostlen = NI_MAXHOST;
  int  servlen = NI_MAXSERV;
  int  rc = 0;

  if(!sa)
  {
    printf("<NO ADDRESS FOUND>");
    return ERROR_INVALID_FUNCTION;
  }

  rc = getnameinfo(sa,salen,host,hostlen,serv,servlen,NI_NUMERICHOST | NI_NUMERICSERV);
  if(rc != 0)
  {
    sprintf_s(p_errorbuffer,ERROR_BUFFER_SIZE,"%s: getnameinfo failed: %d\n",__FILE__,rc);
    return rc;
  }

  // If the port is zero then don't print it
  if(strcmp(serv,"0") != 0)
  {
    if (sa->sa_family == AF_INET)
    {
      printf("[%s]:%s", host, serv);
    }
    else
    {
      printf("%s:%s", host, serv);
    }
  }
  else
  {
    printf("%s", host);
  }
  return NO_ERROR;
}

//
// Function: FormatAddress
//
// Description:
//    This is similar to the PrintAddress function except that instead of
//    printing the string address to the console, it is formatted into
//    the supplied string buffer.
//
static int FormatAddress(SOCKADDR* sa,int salen,char* addrbuf,int addrbuflen,char* p_errorbuffer)
{
  char    host[NI_MAXHOST];
  char    serv[NI_MAXSERV];
  int     hostlen = NI_MAXHOST;
  int     servlen = NI_MAXSERV;
  int     rc;
  HRESULT hRet;

  if(!sa || !addrbuf || salen == 0 || addrbuflen == 0)
  {
    sprintf_s(p_errorbuffer,ERROR_BUFFER_SIZE,"<NO ADDRESS>\n");
    return ERROR_INVALID_FUNCTION;
  }

  rc = getnameinfo(sa,salen,host,hostlen,serv,servlen,NI_NUMERICHOST | NI_NUMERICSERV);
  if(rc != 0)
  {
    sprintf_s(p_errorbuffer,ERROR_BUFFER_SIZE,"%s: getnameinfo failed: %d\n",__FILE__,rc);
    return rc;
  }

  if((strlen(host) + strlen(serv) + 1) > (unsigned)addrbuflen)
  {
    return WSAEFAULT;
  }
  addrbuf[0] = '\0';

  if(sa->sa_family == AF_INET)
  {
    if(FAILED(hRet = StringCchPrintf(addrbuf,addrbuflen,"%s:%s",host,serv)))
    {
      sprintf_s(p_errorbuffer,ERROR_BUFFER_SIZE,"%s StringCchPrintf failed: 0x%x\n",__FILE__,hRet);
      return (int)hRet;
    }
  }
  else if(sa->sa_family == AF_INET6)
  {
    if(FAILED(hRet = StringCchPrintf(addrbuf,addrbuflen,"[%s]:%s",host,serv)))
    {
      sprintf_s(p_errorbuffer,ERROR_BUFFER_SIZE,"%s StringCchPrintf failed: 0x%x\n",__FILE__,hRet);
      return (int)hRet;
    }
  }
  return NO_ERROR;
}

//
// Function: ResolveAddress
//
// Description:
//    This routine resolves the specified address and returns a list of addrinfo
//    structure containing SOCKADDR structures representing the resolved addresses.
//    Note that if 'addr' is non-NULL, then getaddrinfo will resolve it whether
//    it is a string literal address or a host name.
//
static struct addrinfo* ResolveAddress(char* addr,char* port,int af,int type,int proto,char* p_errorbuffer)
{
  struct addrinfo  hints;
  struct addrinfo* res = nullptr;
  int rc = 0;

  memset(&hints,0,sizeof(hints));
  hints.ai_flags    = ((addr) ? 0 : AI_PASSIVE);
  hints.ai_family   = af;
  hints.ai_socktype = type;
  hints.ai_protocol = proto;

  rc = getaddrinfo(addr,port,&hints,&res);
  if(rc != 0)
  {
    sprintf_s(p_errorbuffer,ERROR_BUFFER_SIZE,"Invalid address %s, getaddrinfo failed: %d\n",addr,rc);
    return NULL;
  }
  return res;
}

//
// Function: ReverseLookup
//
// Description:
//    This routine takes a SOCKADDR and does a reverse lookup for the name
//    corresponding to that address.
//
static int ReverseLookup(SOCKADDR* sa,int salen,char* buf,int buflen,char* p_errorbuffer)
{
  char    host[NI_MAXHOST];
  int     hostlen = NI_MAXHOST;
  int     rc = 0;
  HRESULT hRet;

  rc = getnameinfo(sa,salen,host,hostlen,NULL,0,0);
  if(rc != 0)
  {
    sprintf_s(p_errorbuffer,ERROR_BUFFER_SIZE,"getnameinfo failed: %d\n",rc);
    return rc;
  }

  buf[0] = '\0';
  if(FAILED(hRet = StringCchCopy(buf,buflen,host)))
  {
    sprintf_s(p_errorbuffer,ERROR_BUFFER_SIZE,"StringCchCopy failed: 0x%x\n",hRet);
    return (int)hRet;
  }
  return NO_ERROR;
}

// 
// Function: InitIcmpHeader
//
// Description:
//    Helper function to fill in various stuff in our ICMP request.
//
static void InitIcmpHeader(char* buf,int datasize,char* p_errorbuffer)
{
  ICMP_HDR* icmp_hdr = NULL;
  char* datapart = NULL;

  if(!buf || datasize <= 0)
  {
    sprintf_s(p_errorbuffer,ERROR_BUFFER_SIZE,"<NO ICMP HEADER>\n");
    return;
  }

  icmp_hdr = (ICMP_HDR*)buf;
  icmp_hdr->icmp_type = ICMPV4_ECHO_REQUEST_TYPE;        // request an ICMP echo
  icmp_hdr->icmp_code = ICMPV4_ECHO_REQUEST_CODE;
  icmp_hdr->icmp_id = (USHORT)GetCurrentProcessId();
  icmp_hdr->icmp_checksum = 0;
  icmp_hdr->icmp_sequence = 0;

  datapart = buf + sizeof(ICMP_HDR);
  //
  // Place some data in the buffer.
  //
  memset(datapart,'E',datasize);
}

//
// Function: InitIcmp6Header
//
// Description:
//    Initialize the ICMP6 header as well as the echo request header.
//
static int InitIcmp6Header(char* buf,int datasize)
{
  ICMPV6_HDR* icmp6_hdr = NULL;
  ICMPV6_ECHO_REQUEST* icmp6_req = NULL;
  char* datapart = NULL;

  // Initialize the ICMP6 header fields
  icmp6_hdr = (ICMPV6_HDR*)buf;
  icmp6_hdr->icmp6_type = ICMPV6_ECHO_REQUEST_TYPE;
  icmp6_hdr->icmp6_code = ICMPV6_ECHO_REQUEST_CODE;
  icmp6_hdr->icmp6_checksum = 0;

  // Initialize the echo request fields
  icmp6_req = (ICMPV6_ECHO_REQUEST*)(buf + sizeof(ICMPV6_HDR));
  icmp6_req->icmp6_echo_id = (USHORT)GetCurrentProcessId();
  icmp6_req->icmp6_echo_sequence = 0;

  datapart = (char*)buf + sizeof(ICMPV6_HDR) + sizeof(ICMPV6_ECHO_REQUEST);

  memset(datapart,'#',datasize);

  return (sizeof(ICMPV6_HDR) + sizeof(ICMPV6_ECHO_REQUEST));
}

// Function: ICMPChecksum
// Description:
//    This function calculates the 16-bit one's complement sum
//    of the supplied buffer (ICMP) header.
//
static USHORT ICMPChecksum(USHORT* buffer,int size)
{
  unsigned long cksum = 0;

  while(size > 1)
  {
    cksum += *buffer++;
    size -= sizeof(USHORT);
  }
  if(size)
  {
    cksum += *(UCHAR*)buffer;
  }
  cksum = (cksum >> 16) + (cksum & 0xffff);
  cksum += (cksum >> 16);
  return (USHORT)(~cksum);
}

// Function: SetIcmpSequence
//
// Description:
//    This routine sets the sequence number of the ICMP request packet.
//
static void SetIcmpSequence(char* buf,int p_family,char* p_errorbuffer)
{
  ULONGLONG sequence = 0;

  if(!buf)
  {
    sprintf_s(p_errorbuffer,ERROR_BUFFER_SIZE,"<NO ICMP SEQUENCE>\n");
    return;
  }

  sequence = GetTickCount64();
  if(p_family == AF_INET)
  {
    ICMP_HDR* icmpv4 = NULL;
    icmpv4 = (ICMP_HDR*)buf;
    if(icmpv4->icmp_sequence)
    {
      icmpv4->icmp_sequence = (USHORT)sequence;
    }
  }
  else if(p_family == AF_INET6)
  {
    ICMPV6_ECHO_REQUEST* req6 = NULL;

    req6 = (ICMPV6_ECHO_REQUEST*)(buf + sizeof(ICMPV6_HDR));

    if(req6->icmp6_echo_sequence)
    {
      req6->icmp6_echo_sequence = (USHORT)sequence;
    }
  }
}

// Function: ComputeIcmp6PseudoHeaderChecksum
//
// Description:
//    This routine computes the ICMP6 checksum which includes the pseudo
//    header of the IPv6 header (see RFC2460 and RFC2463). The one difficulty
//    here is we have to know the source and destination IPv6 addresses which
//    will be contained in the IPv6 header in order to compute the checksum.
//    To do this we call the SIO_ROUTING_INTERFACE_QUERY ioctl to find which
//    local interface for the outgoing packet.
//
static USHORT ComputeIcmp6PseudoHeaderChecksum(SOCKET s,char* icmppacket,int icmplen,struct addrinfo* dest,char* p_errorbuffer)
{
  SOCKADDR_STORAGE localif;
  DWORD            bytes;
  char             proto = 0;
  int              rc,total,length,i;

  if(!s || !icmppacket || !dest)
  {
    sprintf_s(p_errorbuffer,ERROR_BUFFER_SIZE,"<Compute ICMP6 failed>\n");
    return 0xFFFF;
  }

  // Find out which local interface for the destination
  rc = WSAIoctl(s,
                SIO_ROUTING_INTERFACE_QUERY,
                dest->ai_addr,
                (DWORD)dest->ai_addrlen,
                (SOCKADDR*)&localif,
                (DWORD)sizeof(localif),
                &bytes,
                NULL,
                NULL);
  if(rc == SOCKET_ERROR)
  {
    sprintf_s(p_errorbuffer,ERROR_BUFFER_SIZE,"WSAIoctl failed: %d\n",WSAGetLastError());
    return 0xFFFF;
  }

  // We use a temporary buffer to calculate the pseudo header. 
  char* tmp = new char[MAX_RECV_BUF_LEN];
  char* ptr = tmp;
  memset(ptr,0,MAX_RECV_BUF_LEN);
  total = 0;

  // Copy source address
  memcpy(ptr,&((SOCKADDR_IN6*)&localif)->sin6_addr,sizeof(struct in6_addr));
  ptr   += sizeof(struct in6_addr);
  total += sizeof(struct in6_addr);

//   printf("%x%x%x%x\n",
//          ((SOCKADDR_IN6*)&localif)->sin6_addr.u.Byte[0],
//          ((SOCKADDR_IN6*)&localif)->sin6_addr.u.Byte[1],
//          ((SOCKADDR_IN6*)&localif)->sin6_addr.u.Byte[2],
//          ((SOCKADDR_IN6*)&localif)->sin6_addr.u.Byte[3]
//   );

// Copy destination address
  memcpy(ptr,&((SOCKADDR_IN6*)dest->ai_addr)->sin6_addr,sizeof(struct in6_addr));
  ptr   += sizeof(struct in6_addr);
  total += sizeof(struct in6_addr);

  // Copy ICMP packet length
  length = htonl(icmplen);

  memcpy(ptr,&length,sizeof(length));
  ptr   += sizeof(length);
  total += sizeof(length);

//   printf("%x%x%x%x\n",
//          (char)*(ptr - 4),
//          (char)*(ptr - 3),
//          (char)*(ptr - 2),
//          (char)*(ptr - 1)
//   );

// Zero the 3 bytes
  memset(ptr,0,3);
  ptr   += 3;
  total += 3;

  // Copy next hop header
  proto = IPPROTO_ICMP6;

  memcpy(ptr,&proto,sizeof(proto));
  ptr   += sizeof(proto);
  total += sizeof(proto);

  // Copy the ICMP header and payload
  memcpy(ptr,icmppacket,icmplen);
  ptr   += icmplen;
  total += icmplen;

  for(i = 0; i < icmplen % 2;i++)
  {
    *ptr = 0;
    ptr++;
    total++;
  }

  // Calculate resulting checksum
  USHORT sum = ICMPChecksum((USHORT*)tmp,total);
  delete [] tmp;
  return sum;
}

//
// Function: ComputeIcmpChecksum
//
// Description:
//    This routine computes the checksum for the ICMP request. For IPv4 its
//    easy, just compute the checksum for the ICMP packet and data. For IPv6,
//    its more complicated. The pseudo checksum has to be computed for IPv6
//    which includes the ICMP6 packet and data plus portions of the IPv6
//    header which is difficult since we aren't building our own IPv6
//    header.
//
static void ComputeIcmpChecksum(SOCKET s,char* buf,int packetlen,struct addrinfo* dest,int p_family,char* p_errorbuffer)
{
  if(!buf)
  {
    sprintf_s(p_errorbuffer,ERROR_BUFFER_SIZE,"<COMPUTE ICMP CHECKSUM>\n");
    return;
  }

  if(p_family == AF_INET)
  {
    ICMP_HDR* icmpv4 = NULL;

    icmpv4 = (ICMP_HDR*)buf;
    icmpv4->icmp_checksum = 0;
    icmpv4->icmp_checksum = ICMPChecksum((USHORT*)buf,packetlen);
  }
  else if(p_family == AF_INET6)
  {
    ICMPV6_HDR* icmpv6 = NULL;

    icmpv6 = (ICMPV6_HDR*)buf;
    icmpv6->icmp6_checksum = 0;
    icmpv6->icmp6_checksum = ComputeIcmp6PseudoHeaderChecksum(s,buf,packetlen,dest,p_errorbuffer);
  }
}

//
// Function: PostRecvfrom
//
// Description:
//    This routine posts an overlapped WSARecvFrom on the raw socket.
//
static int PostRecvfrom(SOCKET s,char* buf,int buflen,SOCKADDR* from,int* fromlen,WSAOVERLAPPED* ol,char* p_errorbuffer)
{
  WSABUF  wbuf;
  DWORD   flags;
  DWORD   bytes;
  int     rc;

  wbuf.buf = buf;
  wbuf.len = buflen;

  flags = 0;

  rc = WSARecvFrom(s,&wbuf,1,&bytes,&flags,from,fromlen,ol,NULL);
  if(rc == SOCKET_ERROR)
  {
    if(WSAGetLastError() != WSA_IO_PENDING)
    {
      sprintf_s(p_errorbuffer,ERROR_BUFFER_SIZE,"WSARecvFrom failed: %d\n",WSAGetLastError());
      return SOCKET_ERROR;
    }
  }
  return NO_ERROR;
}

//
// Function: PrintPayload
// 
// Description:
//    This routine is for IPv4 only. It determines if there are any IP options
//    present (by seeing if the IP header length is greater than 20 bytes) and
//    if so it prints the IP record route options.
//
static void PrintPayload(char* buf,int bytes,int p_family,char* p_errorbuffer)
{

  UNREFERENCED_PARAMETER(bytes);

  if(p_family == AF_INET)
  {
    SOCKADDR_IN      hop;
    IPV4_OPTION_HDR* v4opt = NULL;
    IPV4_HDR* v4hdr = NULL;
    int hdrlen = 0;

    hop.sin_family = (USHORT)p_family;
    hop.sin_port = 0;

    if(!buf)
    {
      sprintf_s(p_errorbuffer,ERROR_BUFFER_SIZE,"<PrintPayload failed>\n");
      return;
    }

    v4hdr = (IPV4_HDR*)buf;
    hdrlen = (v4hdr->ip_verlen & 0x0F) * 4;

    // If the header length is greater than the size of the basic IPv4
    //    header then there are options present. Find them and print them.
    if(hdrlen > sizeof(IPV4_HDR))
    {
      v4opt = (IPV4_OPTION_HDR*)(buf + sizeof(IPV4_HDR));
      int routes = (v4opt->opt_ptr / sizeof(ULONG)) - 1;
      for(int i = 0; i < routes;i++)
      {
        hop.sin_addr.s_addr = v4opt->opt_addr[i];

        // Print the route
        if(i == 0) printf("    Route: ");
        else       printf("           ");

        PrintAddress((SOCKADDR*)&hop,sizeof(hop),p_errorbuffer);

        if(i < routes - 1) printf(" ->\n");
        else               printf("\n");
      }
    }
  }
  return;
}

//
// Function: SetTtl
//
// Description:
//    Sets the TTL on the socket.
//
static int SetTTL(SOCKET p_socket,int p_ttl,int p_family,char* p_errorbuffer)
{
  int optlevel = 0;
  int option   = 0;
  int rc;

  rc = NO_ERROR;
  if(p_family == AF_INET)
  {
    optlevel = IPPROTO_IP;
    option   = IP_TTL;
  }
  else if(p_family == AF_INET6)
  {
    optlevel = IPPROTO_IPV6;
    option   = IPV6_UNICAST_HOPS;
  }
  else
  {
    rc = SOCKET_ERROR;
  }
  if(rc == NO_ERROR)
  {
    rc = setsockopt(p_socket,optlevel,option,(char*)&p_ttl,sizeof(p_ttl));
  }
  if(rc == SOCKET_ERROR)
  {
    sprintf_s(p_errorbuffer,ERROR_BUFFER_SIZE,"SetTTL: setsockopt failed: %d\n",WSAGetLastError());
  }
  return rc;
}

// Function: WinPing
//
// Description:
//    Setup the ICMP raw socket and create the ICMP header. 
//    Add the appropriate IP option header and start sending ICMP
//    echo requests to the endpoint. For each send and receive we
//    set a timeout value so that we don't wait forever for a 
//    response in case the endpoint is not responding.
//    When we receive a packet decode it.
//
double WinPing(char* p_destination // internet address to ping
              ,char* p_errorbuffer // Errors along the way
              ,bool  p_print       // Show results on stdout
              ,int   p_sendCount   /*= DEFAULT_SEND_COUNT  */
              ,int   p_family      /*= AF_UNSPEC           */
              ,int   p_timeToLive  /*= DEFAULT_TTL         */
              ,int   p_dataSize    /*= DEFAULT_DATA_SIZE   */)
{
  WSADATA             wsd;
  WSAOVERLAPPED       recvol;
  int                 protocol   = IPPROTO_ICMP;      // Protocol value
  SOCKET              sock       = INVALID_SOCKET;
  int                 recvbuflen = MAX_RECV_BUF_LEN;  // Length of received packets.
  char*               recvbuf    = nullptr;
  char*               icmpbuf    = nullptr;
  struct addrinfo*    dest       = nullptr;
  struct addrinfo*    local      = nullptr;
  double*             all_times  = nullptr;
//IPV4_OPTION_HDR     ipopt;                          // Record route
  SOCKADDR_STORAGE    from;
  double              time       = 0.0;
  DWORD               bytes      = 0;
  DWORD               flags      = 0;
  int                 packetlen  = 0;
  int                 fromlen    = 0;
  int                 rc         = 0;
  int                 status     = 0;
  HPFCounter counter;

  recvol.hEvent = WSA_INVALID_EVENT;

  // Load Winsock2
  if((rc = WSAStartup(MAKEWORD(2,2),&wsd)) != 0)
  {
    sprintf_s(p_errorbuffer,ERROR_BUFFER_SIZE,"WSAStartup() failed: %d\n",rc);
    status = -1;
    goto EXIT;
  }

  // Resolve the destination address
  dest = ResolveAddress(p_destination,(char*)"0",p_family,0,0,p_errorbuffer);
  if(dest == NULL)
  {
    if(p_errorbuffer[0] == 0)
    {
      sprintf_s(p_errorbuffer,ERROR_BUFFER_SIZE,"Bad address name: %s\n",p_destination);
    }
    status = -2;
    goto CLEANUP;
  }
  p_family = dest->ai_family;

  if(p_family == AF_INET)
  {
    protocol = IPPROTO_ICMP;
  }
  else if(p_family == AF_INET6)
  {
    protocol = IPPROTO_ICMP6;
  }

// Get the bind address
  local = ResolveAddress(NULL,(char*)"0",p_family,0,0,p_errorbuffer);
  if(local == NULL)
  {
    if(p_errorbuffer[0] == 0)
    {
      sprintf_s(p_errorbuffer,ERROR_BUFFER_SIZE,"Unable to obtain the bind address for: %s\n",p_destination);
    }
    status = -3;
    goto CLEANUP;
  }

  // Create the raw socket
  sock = socket(p_family,SOCK_RAW,protocol);
  if(sock == INVALID_SOCKET)
  {
    sprintf_s(p_errorbuffer,ERROR_BUFFER_SIZE,"Call to create socket failed: %d\n",WSAGetLastError());
    status = -4;
    goto CLEANUP;
  }

  SetTTL(sock,p_timeToLive,p_family,p_errorbuffer);

  // Figure out the size of the ICMP header and payload
  if(p_family == AF_INET)
  {
    packetlen += sizeof(ICMP_HDR);
  }
  else if(p_family == AF_INET6)
  {
    packetlen += sizeof(ICMPV6_HDR) + sizeof(ICMPV6_ECHO_REQUEST);
  }
  // Add in the data size
  packetlen += p_dataSize;

  // Allocate the buffer that will contain the ICMP request
  icmpbuf = (char*)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,packetlen);
  if(icmpbuf == NULL)
  {
    sprintf_s(p_errorbuffer,ERROR_BUFFER_SIZE,"HeapAlloc failed: %d\n",GetLastError());
    status = -5;
    goto CLEANUP;
  }

  // Initialize the ICMP headers
  if(p_family == AF_INET)
  {
//     if(p_recordRoute)
//     {
//       // Setup the IP option header to go out on every ICMP packet
//       ZeroMemory(&ipopt,sizeof(ipopt));
//       ipopt.opt_code = IP_RECORD_ROUTE; // record route option
//       ipopt.opt_ptr = 4;                // point to the first addr offset
//       ipopt.opt_len = 39;               // length of option header
// 
//       rc = setsockopt(sock,IPPROTO_IP,IP_OPTIONS,(char*)&ipopt,sizeof(ipopt));
//       if(rc == SOCKET_ERROR)
//       {
//         sprintf_s(p_errorbuffer,ERROR_BUFFER_SIZE,"setsockopt(IP_OPTIONS) failed: %d\n",WSAGetLastError());
//         status = -6;
//         goto CLEANUP;
//       }
//     }
    InitIcmpHeader(icmpbuf,p_dataSize,p_errorbuffer);
  }
  else if(p_family == AF_INET6)
  {
    InitIcmp6Header(icmpbuf,p_dataSize);
  }

  // Bind the socket -- need to do this since we post a receive first
  rc = bind(sock,local->ai_addr,(int)local->ai_addrlen);
  if(rc == SOCKET_ERROR)
  {
    sprintf_s(p_errorbuffer,ERROR_BUFFER_SIZE,"bind failed: %d\n",WSAGetLastError());
    status = -7;
    goto CLEANUP;
  }

  // Setup the receive operation
  memset(&recvol,0,sizeof(recvol));
  recvol.hEvent = WSACreateEvent();
  if(recvol.hEvent == WSA_INVALID_EVENT)
  {
    sprintf_s(p_errorbuffer,ERROR_BUFFER_SIZE,"WSACreateEvent failed: %d\n",WSAGetLastError());
    status = -8;
    goto CLEANUP;
  }

  // Allocate receive buffer
  recvbuf = new char[recvbuflen];     // For received packets

  // Post the first overlapped receive
  fromlen = sizeof(from);
  PostRecvfrom(sock,recvbuf,recvbuflen,(SOCKADDR*)&from,&fromlen,&recvol,p_errorbuffer);

  if(p_print)
  {
    printf("\nPinging %s [",p_destination);
    PrintAddress(dest->ai_addr,(int)dest->ai_addrlen,p_errorbuffer);
    printf("] with %d bytes of data\n\n",p_dataSize);
  }
  // Allocate mean-times array
  all_times = (double*) calloc(p_sendCount,sizeof(double));

  // Start sending the ICMP requests
  for(int index = 0; index < p_sendCount;++index)
  {
    // Set the sequence number and compute the checksum
    SetIcmpSequence(icmpbuf,p_family,p_errorbuffer);
    ComputeIcmpChecksum(sock,icmpbuf,packetlen,dest,p_family,p_errorbuffer);

    all_times[index] = 0.0;
    counter.Reset();
    counter.Start();
    rc = sendto(sock,icmpbuf,packetlen,0,dest->ai_addr,(int)dest->ai_addrlen);
    if(rc == SOCKET_ERROR)
    {
      sprintf_s(p_errorbuffer,ERROR_BUFFER_SIZE,"sendto failed: %d\n",WSAGetLastError());
      status = -9;
      goto CLEANUP;
    }

    // Waite for a response
    rc = WaitForSingleObject((HANDLE)recvol.hEvent,DEFAULT_RECV_TIMEOUT);
    if(rc == WAIT_FAILED)
    {
      sprintf_s(p_errorbuffer,ERROR_BUFFER_SIZE,"WaitForSingleObject failed: %d\n",GetLastError());
      status = -10;
      goto CLEANUP;
    }
    else if(rc == WAIT_TIMEOUT)
    {
      sprintf_s(p_errorbuffer,ERROR_BUFFER_SIZE,"Request timed out.\n");
    }
    else
    {
      rc = WSAGetOverlappedResult(sock,&recvol,&bytes,FALSE,&flags);
      if(rc == FALSE)
      {
        sprintf_s(p_errorbuffer,ERROR_BUFFER_SIZE,"WSAGetOverlappedResult failed: %d\n",WSAGetLastError());
      }
      counter.Stop();
      time = counter.GetCounter() * 1000;
      all_times[index] = time;

      WSAResetEvent(recvol.hEvent);

      if(p_print)
      {
        printf("Reply from ");
        PrintAddress((SOCKADDR*)&from,fromlen,p_errorbuffer);
        printf(": bytes=%d TTL=%d time=%.4fms\n",p_dataSize,p_timeToLive,time);
        PrintPayload(recvbuf,bytes,p_family,p_errorbuffer);
      }
      if(index < p_sendCount - 1)
      {
        fromlen = sizeof(from);
        PostRecvfrom(sock,recvbuf,recvbuflen,(SOCKADDR*)&from,&fromlen,&recvol,p_errorbuffer);
      }
    }
    if(index < (p_sendCount - 1))
    {
      Sleep(500);
    }
  }

CLEANUP:

  //
  // Cleanup
  //
  if(dest)
  {
    freeaddrinfo(dest);
  }
  if(local)
  {
    freeaddrinfo(local);
  }
  if(sock != INVALID_SOCKET)
  {
    closesocket(sock);
  }
  if(recvol.hEvent != WSA_INVALID_EVENT)
  {
    WSACloseEvent(recvol.hEvent);
  }
  if(icmpbuf)
  {
    HeapFree(GetProcessHeap(),0,icmpbuf);
  }
  WSACleanup();

  if(recvbuf)
  {
    delete[] recvbuf;
  }

EXIT:

  // CALCULATE STATISTICS
  int    received    = 0;
  double totalTime   = 0.0;
  double minimumTime = 0.0;
  double maximumTime = 0.0;

  if(all_times)
  {
    for(int index = 0; index < p_sendCount; ++index)
    {
      // Received packets
      if(all_times[index] != 0.0)
      {
        ++received;
        if((minimumTime == 0.0) || (all_times[index] < minimumTime))
        {
          minimumTime = all_times[index];
        }
        if(all_times[index] > maximumTime)
        {
          maximumTime = all_times[index];
        }
      }
      // Account for total
      totalTime += all_times[index];
    }
    free(all_times);
  }

  if(p_print)
  {
    printf("\n");
    printf("Ping statistics for: %s\n", p_destination);
    printf("Packets sent: %d, received: %d, lost: %d (%d%% loss)\n"
          ,p_sendCount
          ,received
          ,p_sendCount - received
          ,(int)((p_sendCount-received) * 100/p_sendCount));
    printf("Roundtrip times in milliseconds:\n");
    printf("Minimum time:  %.6fms\n",minimumTime);
    printf("Maximum time:  %.6fms\n",maximumTime);
    printf("Average time:  %.6fms\n",totalTime/(double)received);
  }
  // Return mean pinging time
  if(received)
  {
    return (totalTime / received);
  }
  // Received nothing
  return (double)status;
}

