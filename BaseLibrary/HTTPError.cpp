/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: HTTPError.cpp
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
#include "pch.h"
#include "BaseLibrary.h"
#include <winhttp.h>
#include "HTTPError.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// Essentially all the errors from <winhttp.h>

HTTPError http_errors[] = 
{
   { 12001, "Out of handles"                          }
  ,{ 12002, "Timeout"                                 }
  ,{ 12004, "Internal error"                          }
  ,{ 12005, "Invalid URL"                             }
  ,{ 12006, "Unrecognized authorization scheme"       }
  ,{ 12007, "Name not resolved"                       }
  ,{ 12009, "Invalid option"                          }
  ,{ 12011, "Option not settable"                     }
  ,{ 12012, "Shutdown"                                }
  ,{ 12015, "Login failure"                           }
  ,{ 12017, "Operation canceled"                      }
  ,{ 12018, "Incorrect handle type"                   }
  ,{ 12019, "Incorrect handle state"                  }
  ,{ 12029, "Cannot connect"                          }
  ,{ 12030, "Connection error"                        }
  ,{ 12032, "Resend request"                          }
  ,{ 12037, "Certificate date invalid"                }
  ,{ 12038, "Certificate common name invalid"         }
  ,{ 12044, "Client authentication certificate needed"}
  ,{ 12045, "Invalid certificate authority"           }
  ,{ 12057, "Certificate revocation failed"           }
  ,{ 12100, "Cannot call before open"                 }
  ,{ 12101, "Cannot call before send"                 }
  ,{ 12102, "Cannot call after send"                  }
  ,{ 12103, "Cannot call after open"                  }
  ,{ 12150, "Header not found"                        }
  ,{ 12152, "Invalid server response"                 }
  ,{ 12153, "Invalid header"                          }
  ,{ 12154, "Invalid query request"                   }
  ,{ 12155, "Header already exists"                   }
  ,{ 12156, "Redirect failed"                         }
  ,{ 12157, "Secure channel error"                    }
  ,{ 12166, "Bad auto proxy script"                   }
  ,{ 12167, "Unable to download script"               }
  ,{ 12169, "Invalid certificate"                     }
  ,{ 12170, "Certificate revoked"                     }
  ,{ 12172, "Not initialized"                         }
  ,{ 12175, "Secure failure"                          }
  ,{ 12176, "Unahandled script type"                  }
  ,{ 12177, "Script execution error"                  }
  ,{ 12178, "Auto proxy service error"                }
  ,{ 12179, "Certificate wrong usage"                 }
  ,{ 12180, "Autodetection failed"                    }
  ,{ 12181, "Header count exceeded"                   }
  ,{ 12182, "Header size overflow"                    }
  ,{ 12183, "Chunked encoding header size overflow"   }
  ,{ 12184, "Response drain overflow"                 }
  ,{ 12185, "Client certificate without private key"  }
  ,{ 12186, "Client certificate no access to private key"}
  ,{ 0,     "" }
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
      text = XString("WINHTTP ") + http_errors[ind].m_text;
      break;
    }
    // Find next error
    ++ind;
  }
  return text;
}


// Get text from HTTP_STATUS code
const char*
GetHTTPStatusText(int p_status)
{
  switch(p_status)
  {
    // 100
    case HTTP_STATUS_CONTINUE:           return "Continue";
    case HTTP_STATUS_SWITCH_PROTOCOLS:   return "Switching protocols";
    // 200
    case HTTP_STATUS_OK:                 return "OK";
    case HTTP_STATUS_CREATED:            return "Created";
    case HTTP_STATUS_ACCEPTED:           return "Accepted";
    case HTTP_STATUS_PARTIAL:            return "Partial completion";
    case HTTP_STATUS_NO_CONTENT:         return "No info";
    case HTTP_STATUS_RESET_CONTENT:      return "Completed, form cleared";
    case HTTP_STATUS_PARTIAL_CONTENT:    return "Partial GET";
    // 300
    case HTTP_STATUS_AMBIGUOUS:          return "Server could not decide";
    case HTTP_STATUS_MOVED:              return "Moved resource";
    case HTTP_STATUS_REDIRECT:           return "Redirect to moved resource";
    case HTTP_STATUS_REDIRECT_METHOD:    return "Redirect to new access method";
    case HTTP_STATUS_NOT_MODIFIED:       return "Not modified since time";
    case HTTP_STATUS_USE_PROXY:          return "Use specified proxy";
    case HTTP_STATUS_REDIRECT_KEEP_VERB: return "HTTP/1.1: Keep same verb";
    // 400
    case HTTP_STATUS_BAD_REQUEST:        return "Invalid syntax";
    case HTTP_STATUS_DENIED:             return "Access denied";
    case HTTP_STATUS_PAYMENT_REQ:        return "Payment required";
    case HTTP_STATUS_FORBIDDEN:          return "Request forbidden";
    case HTTP_STATUS_NOT_FOUND:          return "URL/Object not found";
    case HTTP_STATUS_BAD_METHOD:         return "Method is not allowed";
    case HTTP_STATUS_NONE_ACCEPTABLE:    return "No acceptable response found";
    case HTTP_STATUS_PROXY_AUTH_REQ:     return "Proxy authentication required";
    case HTTP_STATUS_REQUEST_TIMEOUT:    return "Server timed out";
    case HTTP_STATUS_CONFLICT:           return "Conflict";
    case HTTP_STATUS_GONE:               return "Resource is no longer available";
    case HTTP_STATUS_LENGTH_REQUIRED:    return "Length required";
    case HTTP_STATUS_PRECOND_FAILED:     return "Precondition failed";
    case HTTP_STATUS_REQUEST_TOO_LARGE:  return "Request body too large";
    case HTTP_STATUS_URI_TOO_LONG:       return "URI too long";
    case HTTP_STATUS_UNSUPPORTED_MEDIA:  return "Unsupported media type";
    case HTTP_STATUS_RETRY_WITH:         return "Retry after appropriate action";
    // 500
    case HTTP_STATUS_SERVER_ERROR:       return "Internal server error";
    case HTTP_STATUS_NOT_SUPPORTED:      return "Not supported";
    case HTTP_STATUS_BAD_GATEWAY:        return "Error from gateway";
    case HTTP_STATUS_SERVICE_UNAVAIL:    return "Temporarily overloaded";
    case HTTP_STATUS_GATEWAY_TIMEOUT:    return "Gateway timeout";
    case HTTP_STATUS_VERSION_NOT_SUP:    return "HTTP version not supported";
    case HTTP_STATUS_LEGALREASONS:       return "Unavailable for legal reasons";
    default:                             return "Unknown HTTP Status";
  }
}
