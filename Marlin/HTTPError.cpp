/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: HTTPError.cpp
//
// Marlin Server: Internet server/client
// 
// Copyright (c) 2016 ir. W.E. Huisman
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
#include "stdafx.h"
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
  ,{ 12017, "Operation cancelled"                     }
  ,{ 12018, "Incorrect handle type"                   }
  ,{ 12019, "Incorrect handle state"                  }
  ,{ 12029, "Cannot connect"                          }
  ,{ 12030, "Connection error"                        }
  ,{ 12032, "Resend request"                          }
  ,{ 12037, "Certificate date invalid"                }
  ,{ 12038, "Certificate common name invalid"         }
  ,{ 12044, "Client authentication certificate needed"}
  ,{ 12045, "Invalid certificate authority"           }
  ,{ 12057, "Certificate revokation failed"           }
  ,{ 12100, "Cannot call before open"                 }
  ,{ 12101, "Cannot call before send"                 }
  ,{ 12102, "Cannot call after send"                  }
  ,{ 12103, "Cannot call aftaer open"                 }
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

CString GetHTTPErrorText(int p_error)
{
  CString text;
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
      text = CString("WINHTTP ") + http_errors[ind].m_text;
      break;
    }
    // Find next error
    ++ind;
  }
  return text;
}
