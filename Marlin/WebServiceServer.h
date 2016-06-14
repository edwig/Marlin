/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: WebServiceServer.h
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
//////////////////////////////////////////////////////////////////////////
//
// WSServer : Webservices-Server
//
//////////////////////////////////////////////////////////////////////////
#pragma once
#include "HTTPServer.h"
#include "WSDLCache.h"
#include "HTTPThreadPool.h"
#include "CreateURLPrefix.h"
#include "HTTPMessage.h"
#include "SOAPMessage.h"
#include "Analysis.h"
#include "SiteHandlerSoap.h"
#include "SiteHandlerGet.h"

// DERIVED CLASSES CAN HAVE A WEBSERVICE MAP
// DEFINED JUST AS THE INTERFACE MAPPING IN MFC
#define WEBSERVICE_MAP_BEGIN(cl)    void cl::OnProcessPost(int p_code,SOAPMessage* p_message)\
                                    {\
                                      switch(p_code){
#define WEBSERVICE(id,method)         case id: method(id,p_message); break;
#define WEBSERVICE_MAP_END          }}
#define WEBSERVICE_MAP              virtual void OnProcessPost(int p_code,SOAPMessage* p_message);
#define WEBSERVICE_DECLARE(method)  void  method(int p_id,SOAPMessage* p_message);


#define CachePolicy HTTP_CACHE_POLICY_TYPE  

//////////////////////////////////////////////////////////////////////////
//
// HANDLERS for the WebServiceServer
//
//////////////////////////////////////////////////////////////////////////

class SiteHandlerSoapWSService: public SiteHandlerSoap
{
public:
  virtual bool Handle(SOAPMessage* p_message);
};

class SiteHandlerGetWSService: public SiteHandlerGet
{
public:
  virtual bool Handle(HTTPMessage* p_message);
};

//////////////////////////////////////////////////////////////////////////
//
// After the handlers we define the Service class itself
//
//////////////////////////////////////////////////////////////////////////

class WebServiceServer
{
public:
  WebServiceServer(CString    p_name
                  ,CString    p_webroot
                  ,CString    p_url
                  ,PrefixType p_channelType
                  ,CString    p_targetNamespace
                  ,unsigned   p_maxThreads);
  virtual ~WebServiceServer();
 
  // Reset: Use before starting again
  void            Reset();
  // Add SOAP message call and answer. Call once for all messages in the service
  bool            AddOperation(int p_code,CString p_name,SOAPMessage* p_input,SOAPMessage* p_output);
  // Running the service
  virtual bool    Run();
  // Stopping the service
  virtual void    Stop();
  // Is service running
  virtual bool    IsRunning();
  // Send response in the form of a HTTPMessage
  virtual void    SendResponse(HTTPMessage* p_response);
  // Send response in the form of a SOAPMessage
  virtual void    SendResponse(SOAPMessage* p_response);
  // Send response in form of a SOAPMessage AND error status
  virtual void    SendResponse(SOAPMessage* p_response,int p_httpStatus);
 
  // SETTERS

  // HIGHLY RECOMMENDED:  Setting an existing HTTPServer
  void    SetHTTPServer(HTTPServer* p_server);
  // HIGHLY RECOMMENDED:  Set a logging analysis file
  void    SetLogAnalysis(LogAnalysis* p_log);
  // HIGHLY RECOMMENDED:  Set external WSDL Caching
  void    SetWsdlCache(WSDLCache* p_wsdl);
  // HIGHLY RECOMMENDED:  Set external ThreadPool
  void    SetThreadPool(HTTPThreadPool* p_pool);
  // HIGHLY RECOMMENDED:  Set the error report object
  void    SetErrorReport(ErrorReport* p_errorReport);
  // OPTIONAL:  Set a logfile name (if no LogAnalysis given)
  void    SetLogFilename(CString p_logFilename);
  // OPTIONAL:  Set detailed logging
  void    SetDetailedLogging(bool p_logging);
  // OPTIONAL:  Set create as a subsite from another site
  void    SetSubsite(bool p_subsite)                 { m_subsite = p_subsite;              };
  // OPTIONAL:  Set the WSDLCache to generate a WSDL file
  void    SetGenerateWsdl(bool p_generate)           { m_generateWsdl = p_generate;        };
  // OPTIONAL:  Set an external WSDL file to be read
  void    SetExternalWsdl(CString p_filename)        { m_externalWsdl = p_filename;        };
  // OPTIONAL:  Set checking of incoming messages
  void    SetCheckIncoming(bool p_check)             { m_checkIncoming = p_check;          };
  // OPTIONAL:  Set checking of outgoing messages
  void    SetCheckOutgoing(bool p_check)             { m_checkOutgoing = p_check;          };
  // OPTIONAL:  Set checking of field values against XSD
  void    SetCheckFieldValues(bool p_check)          { m_checkFieldvalues = p_check;       };
  // OPTIONAL:  Set JSON-SOAP roundtrip engineering
  void    SetJsonSoapTranslation(bool p_json)        { m_jsonTranslation = p_json;         };
  // OPTIONAL:  Set SOAP 1.0
  void    SetPerformSoap10(bool p_perform)           { m_wsdl->SetPerformSoap10(p_perform);};
  // OPTIONAL:  Set SOAP 1.2
  void    SetPerformSoap12(bool p_perform)           { m_wsdl->SetPerformSoap12(p_perform);};
  // OPTIONAL:  Set service postfix other than ".aspcxx"
  void    SetServicePostfix(CString p_postfix)       { m_servicePostfix = p_postfix;       };
  // OPTIONAL:  Set extra content type
  void    SetContentType(CString p_extension,CString p_contentType);
  // OPTIONAL:  Set async mode for receiving of soap messages only
  void    SetAsynchroneous(bool p_async)             { m_asyncMode = p_async;              };
  // OPTIONAL:  Set authentication scheme (Basic, Digest, NTLM, Negotiate, All)
  void    SetAuthenticationScheme(CString p_scheme)  { m_authentScheme = p_scheme;         };
  // OPTIONAL:  Set WS-Reliability
  void    SetReliable(bool p_reliable)               { m_reliable = p_reliable;            };
  // OPTIONAL:  Set security encryption level
  void    SetEncryptionLevel(XMLEncryption p_level)  { m_securityLevel = p_level;          };
  // OPTIONAL:  Set security encryption password
  void    SetEncryptionPassword(CString p_password)  { m_enc_password = p_password;        };
  // OPTIONAL:  Set cache policy
  void    SetCachePolicy(HTTP_CACHE_POLICY_TYPE p_type,ULONG p_seconds);
  // OPTIONAL:  Set an authentication Realm
  void    SetAuthenticationRealm(CString p_realm)    { m_authentRealm = p_realm;           };
  // OPTIONAL:  Set an authentication Domain
  void    SetAuthenticationDomain(CString p_domain)  { m_authentDomain = p_domain;         };
  // OPTIONAL: Set a different SOAP handler, but beware!
  void    SetSoapHandler(SiteHandler* p_handler);

  // GETTERS

  CString       GetName()               { return m_name;              };
  CString       GetWebroot()            { return m_webroot;           };
  CString       GetURL()                { return m_url;               };
  CString       GetAbsolutePath()       { return m_absPath;           };
  PrefixType    GetChannelType()        { return m_channelType;       };
  bool          GetSubsite()            { return m_subsite;           };
  CString       GetContract()           { return m_targetNamespace;   };
  CString       GetLogFilename()        { return m_logFilename;       };
  bool          GetGenerateWsdl()       { return m_generateWsdl;      };
  CString       GetErrorMessage()       { return m_errorMessage;      };
  CString       GetServicePostfix()     { return m_servicePostfix;    };
  int           GetMaxThreads()         { return m_maxThreads;        };
  bool          GetCheckIncoming()      { return m_checkIncoming;     };
  bool          GetCheckOutgoing()      { return m_checkOutgoing;     };
  bool          GetCheckFieldValues()   { return m_checkFieldvalues;  };
  bool          GetJsonSoapTranslation(){ return m_jsonTranslation;   };
  LogAnalysis*  GetLogfile()            { return m_log;               };
  ErrorReport*  GetErrorReport()        { return m_errorReport;       };
  bool          GetDetailedLogging()    { return m_doDetailedLogging; };
  bool          GetAsynchronous()       { return m_asyncMode;         };
  CString       GetAuthentScheme()      { return m_authentScheme;     };
  CString       GetAuthentRealm()       { return m_authentRealm;      };
  CString       GetAuthentDomain()      { return m_authentDomain;     };
  bool          GetReliable()           { return m_reliable;          };
  XMLEncryption GetEncryptionLevel()    { return m_securityLevel;     };
  CString       GetEncryptionPassword() { return m_enc_password;      };
  CachePolicy   GetCachePolicy()        { return m_cachePolicy;       };
  int           GetCacheSeconds()       { return m_cacheSeconds;      };
  ThreadPool*   GetThreadpool()         { return m_pool;              };
  HTTPServer*   GetHTTPServer()         { return m_httpServer;        };
  HTTPSite*     GetHTTPSite()           { return m_site;              };
  WSDLCache*    GetWSDLCache()          { return m_wsdl;              };
  SiteHandler*  GetPostHandler()        { return m_soapHandler;       };
  SiteHandler*  GetGetHandler()         { return m_getHandler;        };
  int           GetCommandCode(CString p_commandName);

  // CALLBACKS FOR THE SITEHANDLERS

  // Process as a SOAP message
  bool          ProcessPost(SOAPMessage* p_message);
  // Process a GET HTTPMessage
  bool          ProcessGet(HTTPMessage* p_message);

protected:
  // Start WSDL
  void          StartWsdl();
  // Reading the settings from web.config files
  void          ReadingWebconfig(CString p_webconfig);
  // Define for your derived class by using the WEBSERVICE* macros
  // at the beginning of this interface description
  virtual void  OnProcessPost(int p_code,SOAPMessage* p_message);

  // Protected data of the service
  CString         m_name;
  CString         m_webroot;
  CString         m_url;
  CString         m_absPath;
  PrefixType      m_channelType       { PrefixType::URLPRE_Strong };
  bool            m_subsite           { false };
  CString         m_targetNamespace;
  CString         m_logFilename;
  CString         m_servicePostfix    { ".acx" };
  int             m_maxThreads        { NUM_THREADS_MINIMUM };
  CString         m_errorMessage;
  bool            m_checkIncoming     { true  };
  bool            m_checkOutgoing     { false };
  bool            m_checkFieldvalues  { false };
  bool            m_jsonTranslation   { false };
  bool            m_wsdlOwner         { false };
  bool            m_serverOwner       { false };
  bool            m_logOwner          { false };
  bool            m_poolOwner         { false };
  bool            m_generateWsdl      { true  };
  bool            m_doDetailedLogging { false };
  CString         m_externalWsdl; 
  // Server settings
  bool            m_isRunning         { false };
  bool            m_asyncMode         { false };
  CString         m_authentScheme;
  CString         m_authentRealm;
  CString         m_authentDomain;
  bool            m_reliable          { false };
  XMLEncryption   m_securityLevel     { XMLEncryption::XENC_Plain };
  CString         m_enc_password;
  CachePolicy     m_cachePolicy       { HttpCachePolicyNocache };
  int             m_cacheSeconds      { 0 };
  // Complex objects
  MediaTypeMap    m_contentTypes;
  HTTPThreadPool* m_pool              { nullptr };
  WSDLCache*      m_wsdl              { nullptr };
  HTTPServer*     m_httpServer        { nullptr };
  ErrorReport*    m_errorReport       { nullptr };
  HTTPSite*       m_site              { nullptr };
  SiteHandler*    m_soapHandler       { nullptr };
  SiteHandler*    m_getHandler        { nullptr };
  LogAnalysis*    m_log               { nullptr };
};

inline void
WebServiceServer::SetCachePolicy(HTTP_CACHE_POLICY_TYPE p_type,ULONG p_seconds)
{
  m_cachePolicy  = p_type;
  m_cacheSeconds = p_seconds;
}

// Is service running
inline bool
WebServiceServer::IsRunning()
{
  return m_isRunning;
}

inline void
WebServiceServer::SetErrorReport(ErrorReport* p_report)
{
  m_errorReport = p_report;
}

