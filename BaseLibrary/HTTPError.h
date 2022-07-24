/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: HTTPError.h
//
// BaseLibrary: Indispensable general objects and functions
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
#pragma once

// Extra HTTP status numbers (not in <winhttp.h>)
// Extra statusses found on: https://developer.mozilla.org/en-US/docs/Web/HTTP/Status
//
#define HTTP_STATUS_PROCESSING          102   // Used in WebDav
#define HTTP_STATUS_EARLYHINTS          103   // Used in conjunction with 'Link' header
#define HTTP_STATUS_MULTISTATUS         207   // Used in WebDav for multiple status returns in body
#define HTTP_STATUS_ALREADYREPORTED     208   // Used in WebDav for property-status
#define HTTP_STATUS_IM_USED             226   // Delta-Encoding for partial changes
#define HTTP_STATUS_UNUSED              306   // No longer in use
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
#define HTTP_STATUS_LEGALREASONS        451   // Unavailable due to legal reasons (GDPR)
#define HTTP_STATUS_VARIANT_NEG         506   // Variant negotiates
#define HTTP_STATUS_INSUF_STORAGE       507   // Used in WebDav. Insufficient storage available
#define HTTP_STATUS_LOOP_DETECTED       508   // Server detected infinite loop in request
#define HTTP_STATUS_NOT_EXTENDED        510   // Further extensions to the server needed for request
#define HTTP_STATUS_NETWORK_AUTHENT     511   // Further network authentication needed

typedef struct _httpError
{
  int         m_error;
  const char* m_text;
}
HTTPError;

XString GetHTTPErrorText(int p_error);

// Get text from HTTP_STATUS code
const char* GetHTTPStatusText(int p_status);
