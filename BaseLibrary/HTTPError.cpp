/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: HTTPError.cpp
//
// BaseLibrary: Indispensable general objects and functions
// 
// Copyright (c) 2014-2024 ir. W.E. Huisman
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
#include "pch.h"
#include "BaseLibrary.h"
#include "HTTPMessage.h"
#include "HTTPError.h"
#include <winhttp.h>

#ifdef _AFX
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#endif

// Essentially all the errors from <winhttp.h>

HTTPError http_errors[] = 
{
   { 12001, _T("Out of handles")                          }
  ,{ 12002, _T("Timeout")                                 }
  ,{ 12004, _T("Internal error")                          }
  ,{ 12005, _T("Invalid URL")                             }
  ,{ 12006, _T("Unrecognized authorization scheme")       }
  ,{ 12007, _T("Name not resolved")                       }
  ,{ 12009, _T("Invalid option")                          }
  ,{ 12011, _T("Option not settable")                     }
  ,{ 12012, _T("Shutdown")                                }
  ,{ 12015, _T("Login failure")                           }
  ,{ 12017, _T("Operation canceled")                      }
  ,{ 12018, _T("Incorrect handle type")                   }
  ,{ 12019, _T("Incorrect handle state")                  }
  ,{ 12029, _T("Cannot connect")                          }
  ,{ 12030, _T("Connection error")                        }
  ,{ 12032, _T("Resend request")                          }
  ,{ 12037, _T("Certificate date invalid")                }
  ,{ 12038, _T("Certificate common name invalid")         }
  ,{ 12044, _T("Client authentication certificate needed")}
  ,{ 12045, _T("Invalid certificate authority")           }
  ,{ 12057, _T("Certificate revocation failed")           }
  ,{ 12100, _T("Cannot call before open")                 }
  ,{ 12101, _T("Cannot call before send")                 }
  ,{ 12102, _T("Cannot call after send")                  }
  ,{ 12103, _T("Cannot call after open")                  }
  ,{ 12150, _T("Header not found")                        }
  ,{ 12152, _T("Invalid server response")                 }
  ,{ 12153, _T("Invalid header")                          }
  ,{ 12154, _T("Invalid query request")                   }
  ,{ 12155, _T("Header already exists")                   }
  ,{ 12156, _T("Redirect failed")                         }
  ,{ 12157, _T("Secure channel error")                    }
  ,{ 12166, _T("Bad auto proxy script")                   }
  ,{ 12167, _T("Unable to download script")               }
  ,{ 12169, _T("Invalid certificate")                     }
  ,{ 12170, _T("Certificate revoked")                     }
  ,{ 12172, _T("Not initialized")                         }
  ,{ 12175, _T("Secure failure")                          }
  ,{ 12176, _T("Unahandled script type")                  }
  ,{ 12177, _T("Script execution error")                  }
  ,{ 12178, _T("Auto proxy service error")                }
  ,{ 12179, _T("Certificate wrong usage")                 }
  ,{ 12180, _T("Autodetection failed")                    }
  ,{ 12181, _T("Header count exceeded")                   }
  ,{ 12182, _T("Header size overflow")                    }
  ,{ 12183, _T("Chunked encoding header size overflow")   }
  ,{ 12184, _T("Response drain overflow")                 }
  ,{ 12185, _T("Client certificate without private key")  }
  ,{ 12186, _T("Client certificate no access to private key")}
  ,{ 0,     _T("") }
};

XString GetHTTPErrorText(int p_error)
{
  XString text;
  unsigned ind = 0;

  // Must at least be in the designated error range
  if(p_error <= WINHTTP_ERROR_BASE || p_error > WINHTTP_ERROR_LAST)
  {
    return text;
  }
  while(http_errors[ind].m_error)
  {
    if(http_errors[ind].m_error == p_error)
    {
      text = XString(_T("WINHTTP ")) + http_errors[ind].m_text;
      break;
    }
    // Find next error
    ++ind;
  }
  return text;
}


// Get text from HTTP_STATUS code
LPCTSTR
GetHTTPStatusText(int p_status)
{
  switch(p_status)
  {
    // 100
    case HTTP_STATUS_CONTINUE:           return _T("Continue");
    case HTTP_STATUS_SWITCH_PROTOCOLS:   return _T("Switching protocols");
    case HTTP_STATUS_PROCESSING:         return _T("Processing");
    case HTTP_STATUS_EARLYHINTS:         return _T("Early hints");
    case HTTP_STATUS_STALE:              return _T("Response is stale");
    case HTTP_STATUS_REVALIDATION_FAILED:return _T("Revalidation failed");
    case HTTP_STATUS_DISCONNECTED:       return _T("Disconnected operation");
    case HTTP_STATUS_HEURISTIC_EXPIRE:   return _T("Heuristic expiration");
    case HTTP_STATUS_WARNING:            return _T("Miscellaneous warning");
    // 200
    case HTTP_STATUS_OK:                 return _T("OK");
    case HTTP_STATUS_CREATED:            return _T("Created");
    case HTTP_STATUS_ACCEPTED:           return _T("Accepted");
    case HTTP_STATUS_PARTIAL:            return _T("Partial completion");
    case HTTP_STATUS_NO_CONTENT:         return _T("No content");
    case HTTP_STATUS_RESET_CONTENT:      return _T("Completed, form cleared");
    case HTTP_STATUS_PARTIAL_CONTENT:    return _T("Partial content");
    case HTTP_STATUS_MULTISTATUS:        return _T("Multi-status");
    case HTTP_STATUS_ALREADYREPORTED:    return _T("Already reported");
    case HTTP_STATUS_TRANSFORMATION:     return _T("Transformation applied");
    case HTTP_STATUS_THIS_IS_FINE:       return _T("This is fine");
    case HTTP_STATUS_IM_USED:            return _T("IM used");
    case HTTP_STATUS_PERSISTENT_WARNING: return _T("Miscellaneous Persistent Warning");
    // 300
    case HTTP_STATUS_AMBIGUOUS:          return _T("Server could not decide");
    case HTTP_STATUS_MOVED:              return _T("Moved resource");
    case HTTP_STATUS_REDIRECT:           return _T("Redirect to moved resource");
    case HTTP_STATUS_REDIRECT_METHOD:    return _T("Redirect to new access method");
    case HTTP_STATUS_NOT_MODIFIED:       return _T("Not modified since time");
    case HTTP_STATUS_USE_PROXY:          return _T("Use specified proxy");
    case HTTP_STATUS_UNUSED:             return _T("Unused");
    case HTTP_STATUS_REDIRECT_KEEP_VERB: return _T("HTTP/1.1: Keep same verb");
    case HTTP_STATUS_PERMANENT_REDIRECT: return _T("Permanent redirection with same verb");
    // 400
    case HTTP_STATUS_BAD_REQUEST:        return _T("Invalid syntax");
    case HTTP_STATUS_DENIED:             return _T("Access denied");
    case HTTP_STATUS_PAYMENT_REQ:        return _T("Payment required");
    case HTTP_STATUS_FORBIDDEN:          return _T("Request forbidden");
    case HTTP_STATUS_NOT_FOUND:          return _T("URL/Object not found");
    case HTTP_STATUS_BAD_METHOD:         return _T("Method is not allowed");
    case HTTP_STATUS_NONE_ACCEPTABLE:    return _T("No acceptable response found");
    case HTTP_STATUS_PROXY_AUTH_REQ:     return _T("Proxy authentication required");
    case HTTP_STATUS_REQUEST_TIMEOUT:    return _T("Server timed out");
    case HTTP_STATUS_CONFLICT:           return _T("Conflict");
    case HTTP_STATUS_GONE:               return _T("Resource is no longer available");
    case HTTP_STATUS_LENGTH_REQUIRED:    return _T("Length required");
    case HTTP_STATUS_PRECOND_FAILED:     return _T("Precondition failed");
    case HTTP_STATUS_REQUEST_TOO_LARGE:  return _T("Request body too large");
    case HTTP_STATUS_URI_TOO_LONG:       return _T("URI too long");
    case HTTP_STATUS_UNSUPPORTED_MEDIA:  return _T("Unsupported media type");
    case HTTP_STATUS_RANGE_SATISFIABLE:  return _T("Range not satisfiable");
    case HTTP_STATUS_EXPECTATION_FAILED: return _T("Expectation failed");
    case HTTP_STATUS_IM_A_TEAPOT:        return _T("I'm a teapot");
    case HTTP_STATUS_MISDIRECTED:        return _T("Misdirected request");
    case HTTP_STATUS_UNPROCESSABLE:      return _T("Unprocessable entity");
    case HTTP_STATUS_LOCKED:             return _T("Locked");
    case HTTP_STATUS_FAILED_DEPENDENCY:  return _T("Failed dependency");
    case HTTP_STATUS_TOO_EARLY:          return _T("Too early");
    case HTTP_STATUS_UPGRADE_REQUIRED:   return _T("Upgrade required");
    case HTTP_STATUS_PRECOND_REQUIRED:   return _T("Precondition required");
    case HTTP_STATUS_TOO_MANY_REQUESTS:  return _T("Too many requests");
    case HTTP_STATUS_HEADERS_TOO_LARGE:  return _T("Headers too large");
    case HTTP_STATUS_LOGIN_TIMEOUT:      return _T("Login Time-out");
    case HTTP_STATUS_NO_RESPONSE:        return _T("No response");
    case HTTP_STATUS_RETRY_WITH:         return _T("Retry after appropriate action");
    case HTTP_STATUS_LEGALREASONS:       return _T("Unavailable for legal reasons");
    case HTTP_STATUS_CLIENT_CLOSE_PREM:  return _T("Client closed connection prematurely");
    case HTTP_STATUS_TOO_MANY_FORWARD:   return _T("Too many forwarded IP addresses");
    case HTTP_STATUS_INCOMPAT_PROTOCOL:  return _T("Incompatible protocol");
    case HTTP_STATUS_REQ_HEADER_LARGE:   return _T("Request header too large");
    case HTTP_STATUS_CERT_ERROR:         return _T("Certificate error");
    case HTTP_STATUS_CERT_REQUIRED:      return _T("Certificate required");
    case HTTP_STATUS_HTTP_HTTPS:         return _T("HTTP Request Sent to HTTPS Port");
    case HTTP_STATUS_INVALID_TOKEN:      return _T("Invalid authorization token");
    case HTTP_STATUS_TOKEN_REQUIRED:     return _T("Authorization token required");
    // 500
    case HTTP_STATUS_SERVER_ERROR:       return _T("Internal server error");
    case HTTP_STATUS_NOT_SUPPORTED:      return _T("Not supported");
    case HTTP_STATUS_BAD_GATEWAY:        return _T("Error from gateway");
    case HTTP_STATUS_SERVICE_UNAVAIL:    return _T("Temporarily overloaded");
    case HTTP_STATUS_GATEWAY_TIMEOUT:    return _T("Gateway timeout");
    case HTTP_STATUS_VERSION_NOT_SUP:    return _T("HTTP version not supported");
    case HTTP_STATUS_VARIANT_NEG:        return _T("Variant alsoo negotiates");
    case HTTP_STATUS_INSUF_STORAGE:      return _T("Insufficient storage");
    case HTTP_STATUS_LOOP_DETECTED:      return _T("Infinite loop detected");
    case HTTP_STATUS_NOT_EXTENDED:       return _T("Server not extended");
    case HTTP_STATUS_NETWORK_AUTHENT:    return _T("Network authentication required");
    case HTTP_STATUS_UNKNOWN_ERROR:      return _T("Web server is returning an unknown error");
    case HTTP_STATUS_SERVER_DOWN:        return _T("Web server is down");
    case HTTP_STATUS_CONN_TIME_OUT:      return _T("Connection timed out");
    case HTTP_STATUS_ORIGIN_UNREACHABLE: return _T("Origin is Unreachable");
    case HTTP_STATUS_TIMEOUT:            return _T("A timeout occurred");
    case HTTP_STATUS_HANDSHAKE_FAILED:   return _T("SSL Handshake failed");
    case HTTP_STATUS_INVALID_CERTIFICATE:return _T("Invalid SSL Certificate");
    case HTTP_STATUS_RAILGUN_LISTENER:   return _T("Railgun listener to Origin");
    case HTTP_STATUS_OVERLOADED:         return _T("The service is overloaded");
    case HTTP_STATUS_SITE_FROZEN:        return _T("Site frozen");
    case HTTP_STATUS_SERVER_UNAUTH:      return _T("Unauthorized by fault of the server");
    case HTTP_STATUS_NETWRK_READ_TIMEOUT:return _T("Network read timeout");
    case HTTP_STATUS_NETWRK_CONN_TIMEOUT:return _T("Network connection timeout");
    // 900
    case HTTP_STATUS_REQUEST_DENIED:     return _T("Request denied");
    // Fallback
    default:                             return _T("Unknown HTTP Status");
  }
}


// Used by HttpSys.dll
LPCTSTR http_verbs[]
{
  _T("UNPARSED"),
  _T("UNKNOWN"),
  _T("INVALID"),
  _T("OPTIONS"),
  _T("GET"),
  _T("HEAD"),
  _T("POST"),
  _T("PUT"),
  _T("DELETE"),
  _T("TRACE"),
  _T("CONNECT"),
  _T("TRACK"),
  _T("MOVE"),
  _T("COPY"),
  _T("PROPFIND"),
  _T("PROPPATCH"),
  _T("MKCOL"),
  _T("LOCK"),
  _T("UNLOCK"),
  _T("SEARCH")
};

XString
GetHTTPVerb(HTTP_VERB p_verb, const char* p_unknown /*=nullptr*/)
{
  if(p_verb >= HTTP_VERB::HttpVerbUnparsed &&
     p_verb <= HTTP_VERB::HttpVerbMaximum)
  {
    if(p_verb != HTTP_VERB::HttpVerbUnknown)
    {
      return XString(http_verbs[(int)p_verb]);
    }
  }
  if(p_unknown && *p_unknown)
  {
    return LPCSTRToString(p_unknown);
  }
  return XString(http_verbs[HTTP_VERB::HttpVerbUnknown]);
}

// Get the formalized VERB for a command
XString
GetHTTPVerb(HTTPCommand p_verb,const char* p_unknown /*=nullptr*/)
{
  if(p_verb >= HTTPCommand::http_post &&
     p_verb <= HTTPCommand::http_last_command)
  {
    return XString(headers[(int)p_verb]);
  }
  if(p_unknown && *p_unknown)
  {
    return XString(LPCSTRToString(p_unknown));
  }
  return _T("");
}

