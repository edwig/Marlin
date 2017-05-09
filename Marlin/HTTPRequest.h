/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: HTTPRequest.h
//
// Marlin Server: Internet server/client
// 
// Copyright (c) 2017 ir. W.E. Huisman
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
#include "HTTPServer.h"

class HTTPMessage;
class HTTPSite;

typedef enum _outstanding
{
  IO_Nothing   = 0
 ,IO_Request   = 1
 ,IO_Reading   = 2
 ,IO_Respond   = 3
 ,IO_Writing   = 4
}
OutstandingIO;

// The request is derived from OVERLAPPED, thusly making it possible to 
// pass a pointer to this object to the I/O function calls

class HTTPRequest : public OVERLAPPED
{
public:
  HTTPRequest(HTTPServer* p_server);
 ~HTTPRequest();
 
  // Start a new request against the server
  void StartRequest();
  // Callback from I/O Completion port
  void HandleAsynchroneousIO();

private:
  // Ready with the response
  void Finalize();
  // Clear local memory structures
  void ClearMemory();
  // Detailed handlers of HandleAsynchroneousIO in phases
  void ReceiveRequest();          // 1) Receive general request headers and such
  void ReceiveBodyPart();         // 2) Receive trailing request body
  void HandleRequest();           // 4) Let HTTPSite handle the request
  void SendResponse();            // 5) Send the main response line + headers
  void SendBodyPart();            // 6) Send response body parts
  // Sub procedures for the handlers
  bool CheckAuthentication(HTTPSite* p_site,CString& p_rawUrl,HANDLE& p_token);

  HTTPServer*     m_server;                      // Our server
  HTTP_REQUEST_ID m_requestID  { NULL       };   // The request we are processing
  PHTTP_REQUEST   m_request    { nullptr    };   // Pointer to the request  object
  PHTTP_RESPONSE  m_response   { nullptr    };   // Pointer to the response object
  HTTPSite*       m_site       { nullptr    };   // Site from the HTTP context
  HTTPMessage*    m_message    { nullptr    };   // The message we are processing in the request
  OutstandingIO   m_outstanding{ IO_Nothing };   // Request / reading / writing phase
  bool            m_mayreceive { false      };   // Authentication done: may receive
  bool            m_responding { false      };   // Response already started
  bool            m_logging    { false      };   // Do detailed logging
  DWORD           m_bytes;
};
