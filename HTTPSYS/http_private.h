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
#define SOCK_LOGGING_ERRORS    1    // Log HTTP status 400 and above
#define SOCK_LOGGING_ALL       2    // Log all HTTP actions
#define SOCK_LOGGING_FULLTRACE 3    // Full hex dump tracing

// Size of a certificate thumb-print
#define CERT_THUMBPRINT_SIZE  20

// Thread waiting time when too many threads in the system
#define THREAD_RETRY_WAITING 100    // Milliseconds

// The system is/was initialized by calling HttpInitialize
extern bool g_httpsys_initialized;   // Default = false;

// Forward declaration
class ServerSession;
class URL;

// Global server session with global parameters
extern ServerSession* g_session;

// Global error pages for the server
extern LPCTSTR http_server_error;
extern LPCTSTR http_client_error;

// GENERAL SERVICE FUNCTIONS

// Splits a URL-Prefix string into an URL object (no context yet)
ULONG SplitURLPrefix(XString p_fullprefix,URL* p_url);

