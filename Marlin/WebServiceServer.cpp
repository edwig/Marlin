/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: WebServiceServer.cpp
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

//////////////////////////////////////////////////////////////////////////
//
// WSServer : WebserviceServer
//            Counterpart of the WebServiceClient
//
// The intention here is to service a GROUP of webservices in one
// And support this to the world with the publication of a WSDL
// (= Web Service Definition Language) file
//
//////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "WebServiceServer.h"
#include "SiteHandlerSoap.h"
#include "SiteHandlerGet.h"
#include "SiteHandlerJson2Soap.h"
#include "ThreadPool.h"
#include "GetLastErrorAsString.h"
#include "HTTPServerMarlin.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//////////////////////////////////////////////////////////////////////////
//
// HANDLERS for the WebServiceServer
//
//////////////////////////////////////////////////////////////////////////

bool
SiteHandlerSoapWSService::Handle(SOAPMessage* p_message)
{
  HTTPSite* site = p_message->GetHTTPSite();
  WebServiceServer* server = reinterpret_cast<WebServiceServer*>(site->GetPayload());

  return server->ProcessPost(p_message);
}

bool
SiteHandlerGetWSService::Handle(HTTPMessage* p_message)
{
  HTTPSite* site = p_message->GetHTTPSite();
  WebServiceServer* server = reinterpret_cast<WebServiceServer*>(site->GetPayload());

  return server->ProcessGet(p_message);
}

//////////////////////////////////////////////////////////////////////////
//
// Here comes the WebServiceServer itself
//
//////////////////////////////////////////////////////////////////////////

WebServiceServer::WebServiceServer(CString    p_name
                                  ,CString    p_webroot
                                  ,CString    p_url
                                  ,PrefixType p_channelType
                                  ,CString    p_targetNamespace
                                  ,unsigned   p_maxThreads)
                 :m_name(p_name)
                 ,m_webroot(p_webroot)
                 ,m_url(p_url)
                 ,m_channelType(p_channelType)
                 ,m_targetNamespace(p_targetNamespace)
                 ,m_maxThreads(p_maxThreads)
{
  // Default settings for: ActiveServerPages (ASP) C++ = CXX
  m_servicePostfix = ".acx";
}

WebServiceServer::~WebServiceServer()
{
  Reset();
}

// Stopping the service. Default is a reset
void
WebServiceServer::Stop()
{
  Reset();
}

void
WebServiceServer::Reset()
{
  // Stop calling Reset in an endless loop
  if(m_isRunning == false)
  {
    return;
  }

  // No longer running
  m_isRunning = false;

  // Clean up the webservice site
  if(m_site)
  {
    if(m_site->StopSite() == false)
    {
      // If not removed by server, signal that and do it ourselves
      // Otherwise would result in a memory leak!
      m_errorMessage.Format("Service site [%s] not removed by server.",m_name.GetString());
      m_log->AnalysisLog(__FUNCTION__,LogType::LOG_ERROR,false,m_errorMessage);
      delete m_site;
    }
    m_site = nullptr;
  }


  if(m_wsdl && m_wsdlOwner)
  {
    delete m_wsdl;
    m_wsdl = nullptr;
    m_wsdlOwner = false;
  }

  if(m_pool && m_poolOwner)
  {
    delete m_pool;
    m_pool = nullptr;
    m_poolOwner = false;
  }

  // De-register ourselves with the server
  if(m_httpServer)
  {
    if(m_httpServer->FindService(m_name))
    {
      // Last thing we do, after this the object is invalid
      m_httpServer->UnRegisterService(m_name);
    }
  }

  // If it's ours, clean up the HTTP server and logfile
  if(m_httpServer && m_serverOwner)
  {
    m_httpServer->StopServer();
    delete m_httpServer;
    m_httpServer = nullptr;
    m_serverOwner = false;
  }
  if(m_log && m_logOwner)
  {
    delete m_log;
    m_log = nullptr;
    m_logOwner = false;
  }

  // SoapHandler & GetHandler
  // are cleaned up by the HTTPSite!
}

// OPTIONAL: Set the SOAP handler
// But beware: Standard soap handling (primary objective of this class)
//             can be compromised, as the handlers may not get called
void    
WebServiceServer::SetSoapHandler(SiteHandler* p_handler)
{
  // Does the implicit cast!
  m_soapHandler = p_handler;
  if(m_site)
  {
    // SOAP handler always is the POST handler!!
    m_site->SetHandler(HTTPCommand::http_post,p_handler);
  }
}

// OPTIONAL: Set a different GET handler, but beware!
// But beware: Standard Interface paging and WSDL/SOAP explanation
//             pages can be compromised, as it will not be called
void
WebServiceServer::SetGetHandler(SiteHandler* p_handler)
{
  m_getHandler = p_handler;
  if(m_site)
  {
    m_site->SetHandler(HTTPCommand::http_get,p_handler);
  }
}

// OPTIONAL: Set a PUT handler, but beware!
void
WebServiceServer::SetPutHandler(SiteHandler* p_handler)
{
  m_putHandler = p_handler;
  if(m_site)
  {
    m_site->SetHandler(HTTPCommand::http_put,p_handler);
  }
}

// OPTIONAL: Register a new HTTP server object
void
WebServiceServer::SetHTTPServer(HTTPServer* p_server)
{
  if(p_server)
  {
    if(m_httpServer && m_serverOwner)
    {
      m_httpServer->StopServer();
      delete m_httpServer;
      m_httpServer  = nullptr;
      m_serverOwner = false;
    }
    m_httpServer = p_server;
  }
}

// OPTIONAL:  Set a logging analysis file
void
WebServiceServer::SetLogAnalysis(LogAnalysis* p_log)
{
  // Remove our own logfile
    if(m_log && m_logOwner)
    {
      delete m_log;
      m_log = nullptr;
      m_logOwner = false;
    }

  // Take this logfile
    m_log      = p_log;
  
  // If not reset, copy the loglevel
  if(p_log)
  {
    m_logLevel = p_log->GetLogLevel();
  }
}

// OLD loglevel interface
bool
WebServiceServer::GetDetailedLogging()
{
  return m_logLevel >= HLL_LOGGING;
}

// OPTIONAL: Set the log level
void
WebServiceServer::SetLogLevel(int p_logLevel)
{
  // Check boundaries
  if(p_logLevel < HLL_NOLOG)   p_logLevel = HLL_NOLOG;
  if(p_logLevel > HLL_HIGHEST) p_logLevel = HLL_HIGHEST;

  // keep the loglevel
  m_logLevel = p_logLevel;
  if(m_log)
  {
    m_log->SetLogLevel(p_logLevel);
  }
  if(m_httpServer)
  {
    m_httpServer->SetLogLevel(p_logLevel);
  }
}


// OPTIONAL:  Set external WSDL Caching
void
WebServiceServer::SetWsdlCache(WSDLCache* p_wsdl)
{
  if(p_wsdl)
  {
    if(m_wsdl && m_wsdlOwner)
    {
      delete m_wsdl;
      m_wsdl = nullptr;
      m_wsdlOwner = false;
    }
    m_wsdl = p_wsdl;
  }
}

// OPTIONAL:  Set external ThreadPool
void
WebServiceServer::SetThreadPool(ThreadPool* p_pool)
{
  if(p_pool)
  {
    if(m_pool && m_poolOwner)
    {
      delete m_pool;
      m_pool = nullptr;
      m_poolOwner = false;
    }
    m_pool = p_pool;
  }
}

// OPTIONAL: Set extra text content type
void
WebServiceServer::SetContentType(CString p_extension,CString p_contentType)
{
  m_contentTypes.insert(std::make_pair(p_extension,MediaType(p_extension,p_contentType)));
}

// OPTIONAL:  Set a logfile name (if no LogAnalysis given)
void
WebServiceServer::SetLogFilename(CString p_logFilename)
{
  if(m_log)
  {
    m_log->SetLogFilename(p_logFilename);
  }
  m_logFilename = p_logFilename;
}

void
WebServiceServer::SetDetailedLogging(bool p_logging)
{
  m_logLevel = p_logging ? HLL_LOGGING : HLL_NOLOG;
  if(m_log)
  {
    m_log->SetLogLevel(m_logLevel);
  }
  if(m_httpServer)
  {
    m_httpServer->SetLogLevel(m_logLevel);
  }
}

// Add SOAP message call and answer. Call once for all messages in the service
bool
WebServiceServer::AddOperation(int p_code,CString p_name,SOAPMessage* p_input,SOAPMessage* p_output)
{
  if(m_wsdl == nullptr)
  {
    StartWsdl();  
  }
  return m_wsdl->AddOperation(p_code,p_name,p_input,p_output);
}

// Starting our WSDL
void
WebServiceServer::StartWsdl()
{
  if(m_wsdl == nullptr)
  {
    m_wsdl = new WSDLCache(true);
    m_wsdlOwner = true;
  }
}

void
WebServiceServer::ReadingWebconfig()
{
  HTTPServerMarlin* server = dynamic_cast<HTTPServerMarlin*>(m_httpServer);

  if(server)
  {
    // Read the general settings first
    ReadingWebconfig("web.config");
    CString siteConfigFile = WebConfig::GetSiteConfig(m_site->GetPrefixURL());
    ReadingWebconfig(siteConfigFile);
  }
}

void
WebServiceServer::ReadingWebconfig(CString p_webconfig)
{
  WebConfig config(p_webconfig);

  // Only process the config if it's filled
  if(config.IsFilled() == false)
  {
    return;
  }

  m_checkIncoming    = config.GetParameterBoolean("WebServices","CheckWSDLIncoming", m_checkIncoming);
  m_checkOutgoing    = config.GetParameterBoolean("WebServices","CheckWSDLOutgoing", m_checkOutgoing);
  m_checkFieldvalues = config.GetParameterBoolean("WebServices","CheckFieldValues",  m_checkFieldvalues);
  m_jsonTranslation  = config.GetParameterBoolean("WebServices","JSONTranslations",  m_jsonTranslation);

  // Log it
  if(m_log)
  {
    m_log->AnalysisLog(__FUNCTION__, LogType::LOG_INFO,true,"Checking incoming webservices against WSDL: %s",m_checkIncoming ? "YES" : "NO");
    m_log->AnalysisLog(__FUNCTION__, LogType::LOG_INFO,true,"Checking outgoing webservices against WSDL: %s",m_checkOutgoing ? "YES" : "NO");
    if(m_checkIncoming || m_checkOutgoing)
    {
      m_log->AnalysisLog(__FUNCTION__, LogType::LOG_INFO,true,"Field values checked  : %s",m_checkFieldvalues ? "YES" : "NO");
      m_log->AnalysisLog(__FUNCTION__, LogType::LOG_INFO,true,"Webconfig file checked: %s",p_webconfig.GetString());
      m_log->AnalysisLog(__FUNCTION__, LogType::LOG_INFO,true,"WSDL to check against : %s",m_wsdl->GetWSDLFilename().GetString());
    }
    m_log->AnalysisLog(__FUNCTION__,LogType::LOG_INFO,true,"Support GET-SOAP-JSON roundtrip translation: %s",m_jsonTranslation ? "YES" : "NO");
  }
}
// Running the service
bool
WebServiceServer::RunService()
{
  // Configure our threadpool
  if(m_pool == nullptr)
  {
    m_pool = new ThreadPool();
    m_poolOwner = true;
  }
  // Setting the correct parameters
  if(m_maxThreads)
  {
    m_pool->TrySetMaximum(m_maxThreads);
  }

  // Starting the logfile
  if(m_log == nullptr)
  {
    m_logOwner = true;
    m_log = new LogAnalysis("Webservice: " + m_name);
    if(!m_logFilename.IsEmpty())
    {
      m_log->SetLogFilename(m_logFilename);
    }
  }
  // Default is logging in file, propagate that setting
  m_log->SetLogLevel(m_logLevel);

  // Start right type of server if not already given.
  if(m_httpServer == nullptr)
  {
    m_httpServer  = new HTTPServerMarlin(m_name);
    m_serverOwner = true;
    m_httpServer->SetWebroot(m_webroot);
  }
  else
  {
    // Use the webroot of the existing server!
    m_webroot = m_httpServer->GetWebroot();
  }
  // Try to set our caching policy
  m_httpServer->SetCachePolicy(m_cachePolicy,m_cacheSeconds);

  // Set our logfile
  m_httpServer->SetLogging(m_log);
  m_httpServer->SetLogLevel(m_logLevel);
  // Use our error reporting facility
  if(m_errorReport)
  {
    m_httpServer->SetErrorReport(m_errorReport);
  }

  // Make sure the server is initialized
  m_httpServer->Initialise();

  // Starting a WSDL 
  StartWsdl();

  // Try to read an external WSDL
  if(!m_externalWsdl.IsEmpty())
  {
    m_generateWsdl = false;
    if(m_wsdl->ReadWSDLFile(m_externalWsdl) == false)
    {
      m_errorMessage.Format("ERROR reading WSDL file: %s\n",m_externalWsdl.GetString());
      m_errorMessage += m_wsdl->GetErrorMessage();
      m_log->AnalysisLog(__FUNCTION__,LogType::LOG_ERROR,false,m_errorMessage);
      return false;
    }
  }

  // Init the WSDL cache for this service
  m_wsdl->SetWebroot(m_webroot);
  m_wsdl->SetTargetNamespace(m_targetNamespace);
  m_wsdl->SetService(m_name,m_url);
  m_wsdl->SetServicePostfix(m_servicePostfix);
  m_wsdl->SetLogAnalysis(m_log);

  // Try to generate the WSDL
  if(m_generateWsdl && m_wsdl->GenerateWSDL() == false)
  {
    m_errorMessage.Format("Cannot start the service. No legal WSDL generated for: %s",m_url.GetString());
    m_log->AnalysisLog(__FUNCTION__, LogType::LOG_ERROR,false,m_errorMessage);
    return false;
  }

  // Analyze the URL
  CrackedURL cracked(m_url);
  if(cracked.Valid() == false)
  {
    m_errorMessage.Format("WebServiceServer has no legal URL to run on: %s",m_url.GetString());
    m_log->AnalysisLog(__FUNCTION__,LogType::LOG_ERROR,false,m_errorMessage);
    return false;
  }
  // Keep the absolute path in separate form
  m_absPath = cracked.m_path;
  if(m_absPath.Right(1) != "/")
  {
    m_absPath += "/";
  }

  // Creating a HTTP site
  m_site = m_httpServer->CreateSite(m_channelType
                                    ,cracked.m_secure
                                    ,cracked.m_port
                                    ,cracked.m_path
                                    ,m_subsite);
  if(m_site == nullptr)
  {
    m_errorMessage.Format("Cannot register an HTTP channel on this machine for URL: %s",m_url.GetString());
    m_log->AnalysisLog(__FUNCTION__, LogType::LOG_ERROR,false,m_errorMessage);
    return false;
  }
  // Setting the service as the payload!!
  // Used in the SiteHandlerSoapWSService
  m_site->SetPayload(this);

  // Read the general settings first
  ReadingWebconfig();

  // Standard text content types
  m_site->AddContentType("",   "text/xml");              // SOAP 1.0 / 1.1
  m_site->AddContentType("xml","application/soap+xml");  // SOAP 1.2
  m_site->SetEncryptionLevel(m_securityLevel);
  m_site->SetEncryptionPassword(m_enc_password);
  m_site->SetAsync(m_asyncMode);
  m_site->SetAuthenticationScheme(m_authentScheme);
  m_site->SetAuthenticationRealm(m_authentRealm);
  m_site->SetAuthenticationDomain(m_authentDomain);
  m_site->SetReliable(m_reliable);

  // If already set, pass on the SOAP handler
  // It's always the POST handler!
  if(m_soapHandler)
  {
    m_site->SetHandler(HTTPCommand::http_post,m_soapHandler);
  }
  else
  {
    // If not use the default WSService handler
    m_soapHandler = new SiteHandlerSoapWSService();
    m_site->SetHandler(HTTPCommand::http_post,m_soapHandler);
  }

  // If already set, pass on the GET handler
  if(m_getHandler)
  {
    m_site->SetHandler(HTTPCommand::http_get,m_getHandler);
  }
  else if(m_jsonTranslation)
  {
    // If JSON/SOAP translation: provide a handler for that
    m_getHandler = new SiteHandlerJson2Soap();
    m_site->SetHandler(HTTPCommand::http_get,m_getHandler);
  }
  else if(m_wsdl->GetOperationsCount() > 0)
  {
    // If not, use the default GetWSService handler
    // but only if we generate a WSDL for the users
    m_getHandler = new SiteHandlerGetWSService();
    m_site->SetHandler(HTTPCommand::http_get,m_getHandler);
  }

  // If set, pass on the PUT handler
  if(m_putHandler)
  {
    m_site->SetHandler(HTTPCommand::http_put,m_putHandler);
  }

  // Adding the extra content types to the site
  for(auto& ctype : m_contentTypes)
  {
    m_site->AddContentType(ctype.first,ctype.second.GetContentType());
  }

  // New: Explicit starting the site for HTTP API Version 2.0
  if(m_site->StartSite() == false)
  {
    m_errorMessage.Format("Cannot start HTTPSite for webservice server on: %s",m_url.GetString());
    m_log->AnalysisLog(__FUNCTION__, LogType::LOG_ERROR,false,m_errorMessage);
    return false;
  }

  // Only starting the server if no errors found
  if(m_httpServer->GetLastError() == NO_ERROR)
  {
    // Go run our service!!
    if(m_httpServer->GetIsRunning() == false)
    {
      m_httpServer->Run();
    }
  }
  else
  {
    m_errorMessage.Format("Cannot start HTTPServer. Server in error state. Error %lu: %s"
                          ,m_httpServer->GetLastError()
                          ,GetLastErrorAsString(m_httpServer->GetLastError()).GetString());
    m_log->AnalysisLog(__FUNCTION__, LogType::LOG_ERROR,false,m_errorMessage);
    return false;
  }
  // Now legally running
  return (m_isRunning = true);
}

int  
WebServiceServer::GetCommandCode(CString p_commandName)
{
  return m_wsdl->GetCommandCode(p_commandName);
}

// Send response in the form of a HTTPMessage
void
WebServiceServer::SendResponse(HTTPMessage* p_response)
{
  m_httpServer->SendResponse(p_response);
}

// Send response in the form of a SOAPMessage
void
WebServiceServer::SendResponse(SOAPMessage* p_response)
{
  m_httpServer->SendResponse(p_response);
}

// Send response in form of a SOAPMessage AND error status
void
WebServiceServer::SendResponse(SOAPMessage* p_response,int p_httpStatus)
{
  HTTPMessage* answer = new HTTPMessage(HTTPCommand::http_response,p_response);
  answer->SetStatus(p_httpStatus);
  m_httpServer->SendResponse(answer);
  answer->DropReference();

  // Do not respond with the original message
  p_response->SetRequestHandle(NULL);
}

// SHOULD NEVER COME HERE. DEFINE YOUR OWN DERIVED CLASS!!!
void 
WebServiceServer::OnProcessPost(int p_code,SOAPMessage* p_message)
{
  CString fault;
  fault.Format("Programming error for interface [%d] %s",p_code,p_message->GetSoapAction().GetString());
  p_message->SetFault("Critical","Server",fault,"Derive your own handler class from WebServiceServer!");
}

bool  
WebServiceServer::PreProcessPost(SOAPMessage* /*p_message*/)
{
  // NO-OP: Does nothing if derived class does not define this interface
  return true;
}

void  
WebServiceServer::PostProcessPost(SOAPMessage* /*p_message*/)
{
  // NO-OP: Does nothing if derived class does not define this interface
}

// Process as a SOAP message
bool
WebServiceServer::ProcessPost(SOAPMessage* p_message)
{
  CString action = p_message->GetSoapAction();
  int code = GetCommandCode(action);

  if(code == 0)
  {
    m_errorMessage.Format("Message to process NOT FOUND: %s",action.GetString());
    m_log->AnalysisLog(__FUNCTION__,LogType::LOG_ERROR,false,m_errorMessage);
    p_message->Reset();
    p_message->SetFault("Critical","Server","Failed to process the message.",m_errorMessage);
    return false;

  }

  try
  {
    // Check request with WSDL
    if(m_checkIncoming)
    {
      if(m_wsdl->CheckIncomingMessage(p_message,m_checkFieldvalues) == false)
      {
        return false;
      }
    }

    // Process in a derived class a la MFC interface mappings
    // Calls the WEBSERVICE_MAP for the derived class
    OnProcessPost(code,p_message);

    // Check answer with WSDL
    if(m_checkOutgoing)
    {
      if(m_wsdl->CheckOutgoingMessage(p_message,m_checkFieldvalues) == false)
      {
        return false;
      }
    }
  }
  catch(CString& fault)
  {
    m_errorMessage.Format("Error in processing the message: %s : %s",action.GetString(),fault.GetString());
    m_log->AnalysisLog(__FUNCTION__, LogType::LOG_ERROR,false,m_errorMessage);
    p_message->Reset();
    p_message->SetFault("Critical","Server","Failed to process the message.",m_errorMessage);
    return false;
  }
  // Try next handler
  return true;
}  

// Process as a HTTPMessage (Discovery)
bool
WebServiceServer::ProcessGet(HTTPMessage* p_message)
{
  CString absPath   = p_message->GetAbsolutePath();
  CrackedURL& crack = p_message->GetCrackedURL();
  bool hasWsdlParam = crack.HasParameter("wsdl");

  // The following situations are processed as requesting the WSDL
  // 1: URL/servicename.wsdl
  // 2: URL/servicename<postfix>?wsdl
  // 3: URL/servicename?wsdl
  CString wsdlBase = m_absPath + m_name;
  CString wsdlPath = wsdlBase  + ".wsdl";
  CString basePage = m_absPath + m_name + m_servicePostfix;
  CString baseWsdl = basePage  + "?wsdl";

  if((wsdlPath.CompareNoCase(absPath) == 0) ||
     (wsdlBase.CompareNoCase(absPath) == 0 && hasWsdlParam) ||
     (basePage.CompareNoCase(absPath) == 0 && hasWsdlParam) ||
     (baseWsdl.CompareNoCase(absPath) == 0 && hasWsdlParam)  )
  {
    p_message->Reset();
    CString wsdlFileInWebroot = m_wsdl->GetWSDLFilename();
    p_message->GetFileBuffer()->SetFileName(wsdlFileInWebroot);
    p_message->SetContentType("text/xml");
    m_httpServer->SendResponse(p_message);
    return true;
  }
  // 4: URL/servicename.aspcxx
  // 5: URL/Servicename<postfix>
  // ASPCXX = ASP for C++
  if(basePage.CompareNoCase(absPath) == 0)
  {
    p_message->Reset();
    CString pagina = m_wsdl->GetServicePage();
    p_message->AddBody((void*)pagina.GetString(),pagina.GetLength());
    p_message->SetContentType("text/html");
    m_httpServer->SendResponse(p_message);
    return true;
  }
  // 6: URL/servicename.<operation>.html
  if(absPath.Right(5).CompareNoCase(".html") == 0)
  {
    // Knock of the ".html"
    absPath = absPath.Left(absPath.GetLength() - 5);

    // Knock of the basepath
    if(absPath.Left(m_absPath.GetLength()).CompareNoCase(m_absPath) == 0)
    {
      absPath = absPath.Mid(m_absPath.GetLength());

      // Knock of the servicename
      if(absPath.Left(m_name.GetLength()).CompareNoCase(m_name) == 0)
      {
        absPath = absPath.Mid(m_name.GetLength());

        // Knock of the period
        if(absPath.GetAt(0) == '.')
        {
          absPath = absPath.Mid(1);
          // Naming convention applies
          p_message->Reset();
          CString pagina = m_wsdl->GetOperationPage(absPath,m_httpServer->GetHostname());
          if(!pagina.IsEmpty())
          {
            p_message->AddBody((void*)pagina.GetString(),pagina.GetLength());
            p_message->SetContentType("text/html");
            m_httpServer->SendResponse(p_message);
            return true;
          }
        }
      }
    }
  }

  // Preset an error
  p_message->SetStatus(HTTP_STATUS_BAD_REQUEST);

  // Maybe luck in the next handler
  return false;
}
