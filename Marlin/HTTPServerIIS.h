/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: HTTPServerIIS.h
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
#pragma once
#include "HTTPServer.h"

class IHttpResponse;

class HTTPServerIIS : public HTTPServer
{
public:
  HTTPServerIIS(CString p_name);
  virtual ~HTTPServerIIS();

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
  // Sending response for an incoming message
  virtual void       SendResponse(HTTPMessage* p_message);
  // Send a response in one-go
  virtual DWORD      SendResponse(HTTPSite*    p_site
                                 ,HTTPMessage* p_message
                                 ,USHORT       p_statusCode
                                 ,PSTR         p_reason
                                 ,PSTR         p_entityString
                                 ,CString      p_authScheme
                                 ,PSTR         p_cookie      = NULL
                                 ,PSTR         p_contentType = NULL);
  
  // FUNCTIONS FOR IIS

  // Building the essential HTTPMessage from the request area
  HTTPMessage* GetHTTPMessageFromRequest(HTTPSite* p_site,PHTTP_REQUEST p_request);

protected:
  // Cleanup the server
  virtual void Cleanup();
  // Init the stream response
  virtual bool InitEventStream(EventStream& p_stream);


private:
  // Reading the first chunks directly from the request handle from IIS
  void ReadEntityChunks(HTTPMessage* p_message,PHTTP_REQUEST p_request);

  // Add unknown headers to the response
  void AddUnknownHeaders(IHttpResponse* p_response,UKHeaders& p_headers);
  // Setting the overal status of the response message
  void SetResponseStatus(IHttpResponse* p_response,USHORT p_status,CString p_statusMessage);
  // Setting a reponse header by name
  void SetResponseHeader(IHttpResponse* p_response,CString p_name,     CString p_value,bool p_replace);
  void SetResponseHeader(IHttpResponse* p_response,HTTP_HEADER_ID p_id,CString p_value,bool p_replace);

  // Subfunctions for SendResponse
  bool SendResponseBuffer     (IHttpResponse* p_response,FileBuffer* p_buffer,size_t p_totalLength);
  void SendResponseBufferParts(IHttpResponse* p_response,FileBuffer* p_buffer,size_t p_totalLength);
  void SendResponseFileHandle (IHttpResponse* p_response,FileBuffer* p_buffer);
  void SendResponseError      (IHttpResponse* p_response,CString& p_page,int p_error,const char* p_reason);
};
