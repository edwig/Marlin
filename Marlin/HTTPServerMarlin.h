/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: HTTPServerMarlin.h
//
// Marlin Server: Internet server/client
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
#pragma once
#include "HTTPServer.h"

class HTTPServerMarlin : public HTTPServer
{
public:
  explicit HTTPServerMarlin(XString p_name);
  virtual ~HTTPServerMarlin();

  // Running the server 
  virtual void       Run() override;
  // Stop the server
  virtual void       StopServer() override;
  // Initialise a HTTP server and server-session
  virtual bool       Initialise() override;
  // Return a version string
  virtual XString    GetVersion() override;
  // Create a site to bind the traffic to
  virtual HTTPSite*  CreateSite(PrefixType    p_type
                               ,bool          p_secure
                               ,int           p_port
                               ,XString       p_baseURL
                               ,bool          p_subsite  = false
                               ,LPFN_CALLBACK p_callback = nullptr) override;
  // Delete a site from the remembered set of sites
  virtual bool       DeleteSite(int p_port,XString p_baseURL,bool p_force = false) override;
  // Receive (the rest of the) incoming HTTP request
  virtual bool       ReceiveIncomingRequest(HTTPMessage* p_message,Encoding p_enconding) override;
  // Create a new WebSocket in the subclass of our server
  virtual WebSocket* CreateWebSocket(XString p_uri) override;
  // Receive the WebSocket stream and pass on the the WebSocket
  virtual void       ReceiveWebSocket(WebSocket* p_socket,HTTP_OPAQUE_ID p_request) override;
  // Flushing a WebSocket intermediate
  virtual bool       FlushSocket(HTTP_OPAQUE_ID p_request,XString p_prefix) override;
  // Sending response for an incoming message
  virtual void       SendResponse(HTTPMessage* p_message) override;
  // Sending a response as a chunk
  virtual void       SendAsChunk(HTTPMessage* p_message,bool p_final = false) override;

  // FUNCTIONS FOR STAND-ALONE SERVER

  // Find and make an URL group
  HTTPURLGroup* FindUrlGroup(XString p_authName
                            ,ULONG   p_authScheme
                            ,bool    p_cache
                            ,XString p_realm
                            ,XString p_domain);
  // Remove an URLGroup. Called by HTTPURLGroup itself
  void          RemoveURLGroup(const HTTPURLGroup* p_group);
  // Initialize the logging of the HTTPSYS driver
  void          InitHTTPLogging();

protected:
  // Cleanup the server
  virtual void Cleanup() override;
  // Initialise general server header settings
  virtual void InitHeaders() override;
  // Init the stream response
  virtual bool InitEventStream(EventStream& p_stream) override;
  // Used for canceling a WebSocket for an event stream
  virtual void CancelRequestStream(HTTP_OPAQUE_ID p_response,bool p_reset = false) override;

private:
  // For the handling of the event streams: Sending a chunk to an event stream
  virtual bool SendResponseEventBuffer(HTTP_OPAQUE_ID     p_request
                                      ,CRITICAL_SECTION*  p_lock
                                      ,BYTE**             p_buffer
                                      ,size_t             p_totalLength
                                      ,bool               p_continue = true) override;

  // PRIVATE DATA of the stand-alone HTTPServer
  URLGroupMap             m_urlGroups;              // All URL Groups
};
