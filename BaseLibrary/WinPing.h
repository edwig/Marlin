#pragma once
#include <winsock2.h>

#define DEFAULT_TTL            128      // Max packets
#define DEFAULT_SEND_COUNT     4        // number of ICMP requests to send
#define DEFAULT_DATA_SIZE      32       // default data size
#define ERROR_BUFFER_SIZE     512       // Maximum error message size

// Pinging for an internet address
double WinPing(char* p_destination
              ,char* p_errorbuffer
              ,bool  p_print
              ,int   p_sendCount   = DEFAULT_SEND_COUNT
              ,int   p_family      = AF_UNSPEC
              ,int   p_timeToLive  = DEFAULT_TTL
              ,int   p_dataSize    = DEFAULT_DATA_SIZE
              ,bool  p_recordRoute = false);

