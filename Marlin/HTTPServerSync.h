/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: HTTPServerSync.h
//
// Marlin Server: Internet server/client
// 
// Copyright (c) 2015-2018 ir. W.E. Huisman
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
#include "HTTPServerMarlin.h"

class HTTPServerSync: public HTTPServerMarlin
{
public:
  HTTPServerSync(CString p_name);
  virtual ~HTTPServerSync();

  // Running the server 
  virtual void Run();
  // Stop the server
  virtual void StopServer();
  // Initialise a HTTP server and server-session
  virtual bool Initialise();
  // Receive (the rest of the) incoming HTTP request
  virtual bool ReceiveIncomingRequest(HTTPMessage* p_message);
  // Sending response for an incoming message
  virtual void SendResponse(HTTPMessage* p_message);
  // Create a new WebSocket in the subclass of our server
  virtual WebSocket* CreateWebSocket(CString p_uri);

  // FUNCTIONS FOR STAND-ALONE SERVER

  // Running the main loop of the WebServer in same thread
  void         RunHTTPServer();

protected:
  // Cleanup the server
  virtual void Cleanup();
  // Init the stream response
  virtual bool InitEventStream(EventStream& p_stream);
  // Used for canceling a WebSocket for an event stream
  virtual void CancelRequestStream(HTTP_OPAQUE_ID p_response);

private:
  // Preparing a response
  void      InitializeHttpResponse(HTTP_RESPONSE* p_response,USHORT p_status,PSTR p_reason);
  void      AddKnownHeader        (HTTP_RESPONSE& p_response,HTTP_HEADER_ID p_header,const char* p_value);
  PHTTP_UNKNOWN_HEADER AddUnknownHeaders(UKHeaders& p_headers);
  // Sub-functions for SendResponse
  bool      SendResponseBuffer     (PHTTP_RESPONSE p_response,HTTP_OPAQUE_ID p_request,FileBuffer* p_buffer,size_t p_totalLength);
  void      SendResponseBufferParts(PHTTP_RESPONSE p_response,HTTP_OPAQUE_ID p_request,FileBuffer* p_buffer,size_t p_totalLength);
  void      SendResponseFileHandle (PHTTP_RESPONSE p_response,HTTP_OPAQUE_ID p_request,FileBuffer* p_buffer);
  void      SendResponseError      (PHTTP_RESPONSE p_response,HTTP_OPAQUE_ID p_request,CString& p_page,int p_error,const char* p_reason);

  // For the handling of the event streams: Sending a chunk to an event stream
  virtual bool SendResponseEventBuffer(HTTP_OPAQUE_ID     p_request
                                      ,CRITICAL_SECTION*  p_lock
                                      ,const char*        p_buffer
                                      ,size_t             p_totalLength
                                      ,bool               p_continue = true);

  // PRIVATE DATA of the stand-alone HTTPServer
  URLGroupMap  m_urlGroups;              // All URL Groups
  HANDLE       m_serverThread { NULL };
};

