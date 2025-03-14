/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: HTTPError.h
//
// BaseLibrary: Indispensable general objects and functions
// 
// Copyright (c) 2014-2025 ir. W.E. Huisman
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
#pragma once
#include <http.h>

// Extra HTTP status numbers (not in <winhttp.h>)
// Extra statuses found on: https://http.dev
//

// 100
#define HTTP_STATUS_PROCESSING          102   // Used in WebDav
#define HTTP_STATUS_EARLYHINTS          103   // Used in conjunction with 'Link' header
#define HTTP_STATUS_STALE               110   // Used after cache has expired
#define HTTP_STATUS_REVALIDATION_FAILED 111   // Proxy cannot reach server
#define HTTP_STATUS_DISCONNECTED        112   // Disconnected through internal error
#define HTTP_STATUS_HEURISTIC_EXPIRE    113   // Expired after more than 24 hours
#define HTTP_STATUS_WARNING             199   // Miscellaneous warning to be logged
// 200
#define HTTP_STATUS_MULTISTATUS         207   // Used in WebDav for multiple status returns in body
#define HTTP_STATUS_ALREADYREPORTED     208   // Used in WebDav for property-status
#define HTTP_STATUS_TRANSFORMATION      214   // Transformation applied: e.g. compression protocols
#define HTTP_STATUS_THIS_IS_FINE        218   // Fine by proxy override
#define HTTP_STATUS_IM_USED             226   // Delta-Encoding for partial changes
#define HTTP_STATUS_PERSISTENT_WARNING  299   // Miscellaneous Persistent Warning
// 300
#define HTTP_STATUS_UNUSED              306   // No longer in use
// 400
#define HTTP_STATUS_RANGE_SATISFIABLE   416   // Ranges in 'range' header field not satisfiable
#define HTTP_STATUS_EXPECTATION_FAILED  417   // Contents of 'Expect' header not met
#define HTTP_STATUS_IM_A_TEAPOT         418   // Definitly *NOT* a coffe-pot!
#define HTTP_STATUS_MISDIRECTED         421   // Misdirected 
#define HTTP_STATUS_UNPROCESSABLE       422   // Not processable request
#define HTTP_STATUS_LOCKED              423   // Used in WebDav. Resource locked by other user
#define HTTP_STATUS_FAILED_DEPENDENCY   424   // Used in WebDav. Failed due to error in previous request
#define HTTP_STATUS_TOO_EARLY           425   // Server unwilling to process due to replay attack
#define HTTP_STATUS_UPGRADE_REQUIRED    426   // Server must be upgraded before request can be serviced.
#define HTTP_STATUS_PRECOND_REQUIRED    428   // Pre-condition required
#define HTTP_STATUS_TOO_MANY_REQUESTS   429   // Too many requests per given time range
#define HTTP_STATUS_HEADERS_TOO_LARGE   431   // Values of header fields are too long
#define HTTP_STATUS_LOGIN_TIMEOUT       440   // Login Time-out. Login session expired
#define HTTP_STATUS_NO_RESPONSE         444   // No response (sent by proxies)
#define HTTP_STATUS_RETRY_WITH          449   // Retry with specified information
#define HTTP_STATUS_LEGALREASONS        451   // Unavailable due to legal reasons (GDPR)
#define HTTP_STATUS_CLIENT_CLOSE_PREM   460   // Client closed connection prematurely
#define HTTP_STATUS_TOO_MANY_FORWARD    463   // Too many forwarded IP addresses
#define HTTP_STATUS_INCOMPAT_PROTOCOL   464   // Incompatible protocol (e.g. no HTTP/2 supported)
#define HTTP_STATUS_REQ_HEADER_LARGE    494   // Request header too large (see limits of your server)
#define HTTP_STATUS_CERT_ERROR          495   // Certificate error (sent by proxies)
#define HTTP_STATUS_CERT_REQUIRED       496   // Certificate required (sent by proxies)
#define HTTP_STATUS_HTTP_HTTPS          497   // Sent as HTTP to a HTTPS URL
#define HTTP_STATUS_INVALID_TOKEN       498   // Invalid authorization token
#define HTTP_STATUS_TOKEN_REQUIRED      499   // Missing authorization token
// 500
#define HTTP_STATUS_VARIANT_NEG         506   // Variant negotiates
#define HTTP_STATUS_INSUF_STORAGE       507   // Used in WebDav. Insufficient storage available
#define HTTP_STATUS_LOOP_DETECTED       508   // Server detected infinite loop in request
#define HTTP_STATUS_NOT_EXTENDED        510   // Further extensions to the server needed for request
#define HTTP_STATUS_NETWORK_AUTHENT     511   // Further network authentication needed

#define HTTP_STATUS_UNKNOWN_ERROR       520   // Web server is returning an unknown error
#define HTTP_STATUS_SERVER_DOWN         521   // Web server is down
#define HTTP_STATUS_CONN_TIME_OUT       522   // Connection timed out
#define HTTP_STATUS_ORIGIN_UNREACHABLE  523   // Origin is Unreachable
#define HTTP_STATUS_TIMEOUT             524   // A timeout occurred
#define HTTP_STATUS_HANDSHAKE_FAILED    525   // SSL Handshake failed
#define HTTP_STATUS_INVALID_CERTIFICATE 526   // Invalid SSL Certificate
#define HTTP_STATUS_RAILGUN_LISTENER    527   // Railgun listener to Origin
#define HTTP_STATUS_OVERLOADED          529   // The service is overloaded
#define HTTP_STATUS_SITE_FROZEN         530   // Site frozen
#define HTTP_STATUS_SERVER_UNAUTH       561   // Unauthorized by fault of the server
#define HTTP_STATUS_NETWRK_READ_TIMEOUT 598   // Network read timeout
#define HTTP_STATUS_NETWRK_CONN_TIMEOUT 599   // Network connection timeout
// 900
#define HTTP_STATUS_REQUEST_DENIED      999   // Catch all error from the server

typedef struct _httpError
{
  int          m_error;
  const PTCHAR m_text;
}
HTTPError;

enum class HTTPCommand;

XString GetHTTPErrorText(int p_error);

// Get text from HTTP_STATUS code
LPCTSTR GetHTTPStatusText(int p_status);

// Getting the VERB from the request structure
XString GetHTTPVerb(HTTPCommand p_verb, const char* p_unknown = nullptr);
XString GetHTTPVerb(HTTP_VERB   p_verb, const char* p_unknown = nullptr);
