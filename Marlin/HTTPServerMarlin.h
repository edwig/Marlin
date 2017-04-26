/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: HTTPServerMarlin.h
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

class HTTPServerMarlin : public HTTPServer
{
public:
  HTTPServerMarlin(CString p_name);
  virtual ~HTTPServerMarlin();

  // Running the server 
  virtual void       Run();
  // Stop the server
  virtual void       StopServer();
  // Initialise a HTTP server and server-session
  virtual bool       Initialise();
  // Return a version string
  virtual CString    GetVersion();
  // Create a site to bind the traffic to
  virtual HTTPSite*  CreateSite(PrefixType    p_type
                               ,bool          p_secure
                               ,int           p_port
                               ,CString       p_baseURL
                               ,bool          p_subsite  = false
                               ,LPFN_CALLBACK p_callback = nullptr);
  // Delete a site from the remembered set of sites
  virtual bool       DeleteSite(int p_port,CString p_baseURL,bool p_force = false);
  // Receive (the rest of the) incoming HTTP request
  virtual bool       ReceiveIncomingRequest(HTTPMessage* p_message);
  // Receive the WebSocket stream and pass on the the WebSocket
  virtual void       ReceiveWebSocket(WebSocket* p_socket,HTTP_REQUEST_ID p_request);
  // Send to a WebSocket
  virtual bool       SendSocket(RawFrame& p_frame,HTTP_REQUEST_ID p_request);
  // Flushing a WebSocket intermediate
  virtual bool       FlushSocket(HTTP_REQUEST_ID p_request);
  // Cancel and close a WebSocket
  virtual bool       CancelSocket(HTTP_REQUEST_ID p_request);
  // Sending response for an incoming message
  virtual void       SendResponse(HTTPMessage* p_message);

  // FUNCTIONS FOR STAND-ALONE SERVER

  // Find and make an URL group
  HTTPURLGroup* FindUrlGroup(CString p_authName
                            ,ULONG   p_authScheme
                            ,bool    p_cache
                            ,CString p_realm
                            ,CString p_domain);
  // Remove an URLGroup. Called by HTTPURLGroup itself
  void          RemoveURLGroup(HTTPURLGroup* p_group);
  // Running the main loop of the webserver in same thread
  void          RunHTTPServer();


protected:
  // Cleanup the server
  virtual void Cleanup();
  // Initialise the logging and error mechanism
  virtual void InitLogging();
  // Initialise general server header settings
  virtual void InitHeaders();
  // Initialise the hard server limits in bytes
  virtual void InitHardLimits();
  // Initialise the threadpool limits
  virtual void InitThreadpoolLimits(int& p_minThreads,int& p_maxThreads,int& p_stackSize);
  // Initialise the servers webroot
  virtual void InitWebroot(CString p_webroot);
  // Init the stream response
  virtual bool InitEventStream(EventStream& p_stream);
  // Send a response in one-go
  virtual DWORD SendResponse(HTTPSite*    p_site
                            ,HTTPMessage* p_message
                            ,USHORT       p_statusCode
                            ,PSTR         p_reason
                            ,PSTR         p_entityString
                            ,CString      p_authScheme
                            ,PSTR         p_cookie      = NULL
                            ,PSTR         p_contentType = NULL);
  // Used for canceling a WebSocket for an event stream
  virtual void CancelRequestStream(HTTP_REQUEST_ID p_response);

private:
  // Preparing a response
  void      InitializeHttpResponse(HTTP_RESPONSE* p_response,USHORT p_status,PSTR p_reason);
  void      AddKnownHeader(HTTP_RESPONSE& p_response,HTTP_HEADER_ID p_header,const char* p_value);
  PHTTP_UNKNOWN_HEADER AddUnknownHeaders(UKHeaders& p_headers);
  // Subfunctions for SendResponse
  bool      SendResponseBuffer     (PHTTP_RESPONSE p_response,HTTP_REQUEST_ID p_request,FileBuffer* p_buffer,size_t p_totalLength,bool p_more = false);
  void      SendResponseBufferParts(PHTTP_RESPONSE p_response,HTTP_REQUEST_ID p_request,FileBuffer* p_buffer,size_t p_totalLength,bool p_more = false);
  void      SendResponseFileHandle (PHTTP_RESPONSE p_response,HTTP_REQUEST_ID p_request,FileBuffer* p_buffer,bool p_more = false);
  void      SendResponseError      (PHTTP_RESPONSE p_response,HTTP_REQUEST_ID p_request,CString& p_page,int p_error,const char* p_reason);

  // For the handling of the event streams: Sending a chunk to an event stream
  virtual bool SendResponseEventBuffer(HTTP_REQUEST_ID p_request,const char* p_buffer,size_t p_totalLength,bool p_continue = true);

  // PRIVATE DATA of the stand-alone HTTPServer
  URLGroupMap             m_urlGroups;              // All URL Groups
};
