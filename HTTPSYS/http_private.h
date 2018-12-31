//////////////////////////////////////////////////////////////////////////
//
// USER-SPACE IMPLEMENTTION OF HTTP.SYS
//
// Created for various reasons:
// - Educational purposes
// - Non-disclosed functions (websockets)
// - Because it can be done :-)
//
// Not implemented features of HTTP.SYS are:
// - Asynchronous I/O
// - HTTP 2.0 Server push
//
// Copyright 2018 (c) ir. W.E. Huisman
// License: MIT
//
//////////////////////////////////////////////////////////////////////////

#pragma once

#include "version.h"

// We do not import the functions of HTTP.SYS
// But we redefine it.
#define HTTPAPI_LINKAGE __declspec(dllexport)

// We can now include the standard MS-Windows OS version of hhtp.h
#include <http.h>

// Timeouts on a URL group while listening for HTTP call
#define URL_TIMEOUT_ENTITY_BODY     120   // Seconds
#define URL_TIMEOUT_DRAIN_BODY      120
#define URL_TIMEOUT_REQUEST_QUEUE   120
#define URL_TIMEOUT_IDLE_CONNECTION 120
#define URL_TIMEOUT_HEADER_WAIT     120
#define URL_DEFAULT_MIN_SEND_RATE   150   // Bytes per second

// Standard ports for HTTP connections
#define INTERNET_DEFAULT_HTTP_PORT   80
#define INTERNET_DEFAULT_HTTPS_PORT 443

// Buffering while sending a file
#define BUFFER_SIZE_FILESEND    (16*1000) // Smaller than 16 * 1024

// Logging level in the SSL socket library
//
#define SOCK_LOGGING_OFF       0    // No logging
#define SOCK_LOGGING_ON        1    // Results logging
#define SOCK_LOGGING_TRACE     2    // Hex dump tracing first line
#define SOCK_LOGGING_FULLTRACE 3    // Full hex dump tracing

// Size of a certificate thumbprint
#define CERT_THUMBPRINT_SIZE  20

// The system is/was initialized by calling HttpInitialize
extern bool g_httpsys_initialized;   // Default = false;

// Forward declaration
class ServerSession;
class URL;

// Global server session with global parameters
extern ServerSession* g_session;

// Global error pages for the server
extern const char* http_server_error;
extern const char* http_client_error;

// GENERAL SERVICE FUNCTIONS

// Splits a URL-Prefix string into an URL object (no context yet)
ULONG SplitURLPrefix(CString p_fullprefix,URL* p_url);

// Helper classes
//
class AutoCritSec
{
public:
  AutoCritSec(CRITICAL_SECTION* section) : m_section(section)
  {
    EnterCriticalSection(m_section);
  }
  ~AutoCritSec()
  {
    LeaveCriticalSection(m_section);
  }

  void Unlock() { LeaveCriticalSection(m_section); };
  void Relock() { EnterCriticalSection(m_section); };

private:
  CRITICAL_SECTION* m_section;
};
