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

private:
  // Preparing a response
  void      InitializeHttpResponse(HTTP_RESPONSE* p_response,USHORT p_status,PSTR p_reason);
  void      AddKnownHeader(HTTP_RESPONSE& p_response,HTTP_HEADER_ID p_header,const char* p_value);
  PHTTP_UNKNOWN_HEADER AddUnknownHeaders(UKHeaders& p_headers);
  // Subfunctions for SendResponse
  bool      SendResponseBuffer     (PHTTP_RESPONSE p_response,HTTP_REQUEST_ID p_request,FileBuffer* p_buffer,size_t p_totalLength);
  void      SendResponseBufferParts(PHTTP_RESPONSE p_response,HTTP_REQUEST_ID p_request,FileBuffer* p_buffer,size_t p_totalLength);
  void      SendResponseFileHandle (PHTTP_RESPONSE p_response,HTTP_REQUEST_ID p_request,FileBuffer* p_buffer);
  void      SendResponseError      (PHTTP_RESPONSE p_response,HTTP_REQUEST_ID p_request,CString& p_page,int p_error,const char* p_reason);
  bool      SendResponseEventBuffer(                          HTTP_REQUEST_ID p_request,const char* p_buffer,size_t p_totalLength,bool p_continue = true);

  // PRIVATE DATA of the stand-alone HTTPServer

  URLGroupMap             m_urlGroups;              // All URL Groups
};
