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
  // Add unknown headers to the response
  void AddUnknownHeaders(IHttpResponse* p_response,UKHeaders& p_headers);
  // Setting the overal status of the response message
  void SetResponseStatus(IHttpResponse* p_response,USHORT p_status,CString p_statusMessage);

  // Subfunctions for SendResponse
  bool SendResponseBuffer     (IHttpResponse* p_response,FileBuffer* p_buffer,size_t p_totalLength);
  void SendResponseBufferParts(IHttpResponse* p_response,FileBuffer* p_buffer,size_t p_totalLength);
  void SendResponseFileHandle (IHttpResponse* p_response,FileBuffer* p_buffer);
  void SendResponseError      (IHttpResponse* p_response,CString& p_page,int p_error,const char* p_reason);
};
