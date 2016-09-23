#include "stdafx.h"
#include "HTTPServerIIS.h"
#include "HTTPSiteIIS.h"
#include "AutoCritical.h"
#include "WebServiceServer.h"
#include "HTTPURLGroup.h"
#include "GetLastErrorAsString.h"
#include "MarlinModule.h"
#include "EnsureFile.h"

// Logging macro's
#define DETAILLOG1(text)          if(m_detail && m_log) { DetailLog (__FUNCTION__,LogType::LOG_INFO,text); }
#define DETAILLOGS(text,extra)    if(m_detail && m_log) { DetailLogS(__FUNCTION__,LogType::LOG_INFO,text,extra); }
#define DETAILLOGV(text,...)      if(m_detail && m_log) { DetailLogV(__FUNCTION__,LogType::LOG_INFO,text,__VA_ARGS__); }
#define WARNINGLOG(text,...)      if(m_detail && m_log) { DetailLogV(__FUNCTION__,LogType::LOG_WARN,text,__VA_ARGS__); }
#define ERRORLOG(code,text)       ErrorLog (__FUNCTION__,code,text)
#define HTTPERROR(code,text)      HTTPError(__FUNCTION__,code,text)


HTTPServerIIS::HTTPServerIIS(CString p_name)
              :HTTPServer(p_name)
{
  m_counter.Start();
}

HTTPServerIIS::~HTTPServerIIS()
{
  // Cleanup the server objects
  Cleanup();
}

CString
HTTPServerIIS::GetVersion()
{
  return CString(MARLIN_SERVER_VERSION " on Microsoft IIS");
}

// Initialise a HTTP server
bool
HTTPServerIIS::Initialise()
{
  AutoCritSec lock(&m_sitesLock);

  // See if there is something to do!
  if(m_initialized)
  {
    return true;
  }

  // STEP 1: LOGGING
  // Init logging, so we can complain about errors
  InitLogging();

  // STEP 2: CHECKING OF SETTINGS
  if(GeneralChecks() == false)
  {
    return false;
  }

  // STEP 3: Set the hard limits
  InitHardLimits();

  // STEP 4: Init the response headers to send
  InitHeaders();

  // We are airborne!
  return (m_initialized = true);
}


// Cleanup the server
void
HTTPServerIIS::Cleanup()
{
  USES_CONVERSION;
  AutoCritSec lock1(&m_sitesLock);
  AutoCritSec lock2(&m_eventLock);

  // Remove all event streams within the scope of the eventLock
  for(auto& it : m_eventStreams)
  {
    delete it.second;
  }
  m_eventStreams.clear();

  // Remove all services
  while(!m_allServices.empty())
  {
    ServiceMap::iterator it = m_allServices.begin();
    it->second->Stop();
  }

  // Remove all URL's in the sites map
  while(!m_allsites.empty())
  {
    SiteMap::iterator it = m_allsites.begin();
    it->second->StopSite(true);
  }

  // Closing the logging file
  if(m_log && m_logOwner)
  {
    delete m_log;
    m_log = NULL;
  }
}
void
HTTPServerIIS::InitLogging()
{
  // Check for a logging object
  if(m_log == NULL)
  {
    // Create a new one
    m_log = new LogAnalysis(m_name);
    m_logOwner = true;
  }
  // If you want to tweak the loglevel
  // you will need a "Logfile.config" in your logging directory
}

// Initialise general server header settings
// Can only be overriden from within your ServerApp.
void
HTTPServerIIS::InitHeaders()
{
  CString headertype;
  switch(m_sendHeader)
  {
    case SendHeader::HTTP_SH_MICROSOFT:   headertype = "Microsoft defined server header"; break;
    case SendHeader::HTTP_SH_MARLIN:      headertype = "Standard Marlin server header";   break;
    case SendHeader::HTTP_SH_APPLICATION: headertype = "Application's server name";       break;
    case SendHeader::HTTP_SH_WEBCONFIG:   headertype = "Configured header type name";     break;
    case SendHeader::HTTP_SH_HIDESERVER:  headertype = "Hide server response header";     break;
  }
  DETAILLOGV("Header type: %s",headertype);
}

// Initialise the hard server limits in bytes
// You can only change the hard limits from within your ServerApp
void
HTTPServerIIS::InitHardLimits()
{
  // Cannot be bigger than 2 GB, otherwise use indirect file access!
  if(g_streaming_limit > (0x7FFFFFFF))
  {
    g_streaming_limit = 0x7FFFFFFF;
  }
  // Should not be smaller than 1MB
  if(g_streaming_limit < (1024 * 1024))
  {
    g_streaming_limit = (1024 * 1024);
  }
  // Should not be bigger than 25 4K pages
  if(g_compress_limit > (25 * 4 * 1024))
  {
    g_compress_limit = (25 * 4 * 1024);
  }

  DETAILLOGV("Server hard-limit file-size streaming limit: %d",g_streaming_limit);
  DETAILLOGV("Server hard-limit compression threshold: %d",    g_compress_limit);
}

// Initialise the threadpool limits
void  
HTTPServerIIS::InitThreadpoolLimits(int& p_minThreads,int& p_maxThreads,int& p_stackSize)
{
  // Does nothing. Cannot get the limits from a web.config!
  // If you want to change the limits, do this from within your ServerApp!
  UNREFERENCED_PARAMETER(p_maxThreads);
  UNREFERENCED_PARAMETER(p_minThreads);
  UNREFERENCED_PARAMETER(p_stackSize);
}

// Initialise the servers webroot
void 
HTTPServerIIS::InitWebroot(CString p_webroot)
{
  // Directly set webroot from IIS
  // If you want to change the webroot, do this from within your ServerApp!

  EnsureFile ensure(p_webroot);
  int er = ensure.CheckCreateDirectory();
  if(er)
  {
    ERRORLOG(er,"Cannot reach server root directory: " + p_webroot);
  }
  m_webroot = p_webroot;
}

// Running the server 
void
HTTPServerIIS::Run()
{
  Initialise();

  // See if we are in a state to receive requests
  if(GetLastError() || !m_initialized)
  {
    ERRORLOG(ERROR_INVALID_PARAMETER,"RunHTTPServer called too early");
    return;
  }
  DETAILLOG1("HTTPServer initialised and ready to go!");

  // Check if all sites were properly started
  // Catches programmers who forget to call HTTPSite::StartSite()
  CheckSitesStarted();

  // START OUR MAIN LOOP
  m_running = true;
  DETAILLOG1("HTTPServerIIS started");
}

// Stop the server
void
HTTPServerIIS::StopServer()
{
  m_running = false;
}

// Create a site to bind the traffic to
HTTPSite*
HTTPServerIIS::CreateSite(PrefixType    p_type
                         ,bool          p_secure
                         ,int           p_port
                         ,CString       p_baseURL
                         ,bool          p_subsite  /* = false */
                         ,LPFN_CALLBACK p_callback /* = NULL  */)
{
  // USE OVERRIDES FROM WEBCONFIG 
  // BUT USE PROGRAM'S SETTINGS AS DEFAULT VALUES
  
  if(p_type != PrefixType::URLPRE_Strong || p_secure ||
     p_port != INTERNET_DEFAULT_HTTP_PORT)
  {
    DETAILLOG1("In IIS the prefix type, https and port number are controlled by IIS Admin, and cannot be set!");
  }
  // Create our URL prefix
  CString prefix = CreateURLPrefix(p_type,p_secure,p_port,p_baseURL);
  if(!prefix.IsEmpty())
  {
    HTTPSite* mainSite = nullptr;
    if(p_subsite)
    {
      // Do sub-site lookup on receiving messages
      m_hasSubsites = true;
      // Finding the main site of this sub-site
      mainSite = FindHTTPSite(p_port,p_baseURL);
      if(mainSite == nullptr)
      {
        // No luck: No main site to register against
        CString message;
        message.Format("Tried to register a sub-site, without a main-site: %s",p_baseURL);
        ERRORLOG(ERROR_NOT_FOUND,message);
        return nullptr;
      }
    }
    // Create and register a URL
    // Remember URL Prefix strings, and create the site
    HTTPSiteIIS* registeredSite = new HTTPSiteIIS(this,p_port,p_baseURL,prefix,mainSite,p_callback);
    if(RegisterSite(registeredSite,prefix))
    {
      // Site created and registered
      return registeredSite;
    }
    delete registeredSite;
  }
  // No luck
  return nullptr;
}

// Delete a channel (from prefix OR base URL forms)
bool
HTTPServerIIS::DeleteSite(int p_port,CString p_baseURL,bool p_force /*=false*/)
{
  AutoCritSec lock(&m_sitesLock);
  // Default result
  bool result = false;

  CString search(MakeSiteRegistrationName(p_port,p_baseURL));
  SiteMap::iterator it = m_allsites.find(search);
  if(it != m_allsites.end())
  {
    // Finding our site
    HTTPSite* site = it->second;

    // See if other sites are dependent on this one
    if(p_force == false && site->GetIsSubsite() == false)
    {
      // Walk all sites, to see if subsites still dependent on this main site
      for(SiteMap::iterator fit = m_allsites.begin(); fit != m_allsites.end(); ++fit)
      {
        if(fit->second->GetMainSite() == site)
        {
          // Cannot delete this site, other sites are dependent on this one
          ERRORLOG(ERROR_ACCESS_DENIED,"Cannot remove site. Sub-sites still dependent on: " + p_baseURL);
          return false;
        }
      }
    }
    // And remove from the site map
    delete site;
    m_allsites.erase(it);
    result = true;
  }
  return result;
}

//////////////////////////////////////////////////////////////////////////
//
// FUNCTIONS FOR IIS
//
//////////////////////////////////////////////////////////////////////////

// Building the essential HTTPMessage from the request area
HTTPMessage* 
HTTPServerIIS::GetHTTPMessageFromRequest(HTTPSite* p_site,PHTTP_REQUEST p_request)
{
  USES_CONVERSION;

  // Grab the senders content
  CString   acceptTypes    = p_request->Headers.KnownHeaders[HttpHeaderAccept         ].pRawValue;
  CString   contentType    = p_request->Headers.KnownHeaders[HttpHeaderContentType    ].pRawValue;
  CString   contentLength  = p_request->Headers.KnownHeaders[HttpHeaderContentLength  ].pRawValue;
  CString   acceptEncoding = p_request->Headers.KnownHeaders[HttpHeaderAcceptEncoding ].pRawValue;
  CString   cookie         = p_request->Headers.KnownHeaders[HttpHeaderCookie         ].pRawValue;
  CString   authorize      = p_request->Headers.KnownHeaders[HttpHeaderAuthorization  ].pRawValue;
  CString   modified       = p_request->Headers.KnownHeaders[HttpHeaderIfModifiedSince].pRawValue;
  CString   rawUrl         = CW2A(p_request->CookedUrl.pFullUrl);
  PSOCKADDR sender         = p_request->Address.pRemoteAddress;
  int       remDesktop     = FindRemoteDesktop(p_request->Headers.UnknownHeaderCount
                                              ,p_request->Headers.pUnknownHeaders);

  // Log earliest as possible
  DETAILLOGV("Received HTTP call from [%s] with length: %I64u"
             ,SocketToServer((PSOCKADDR_IN6)sender)
             ,p_request->BytesReceived);

  // See if we must substitute for a sub-site
  if(m_hasSubsites)
  {
    CString absPath = CW2A(p_request->CookedUrl.pAbsPath);
    p_site = FindHTTPSite(p_site,absPath);
  }

  // Translate the command. Now reduced to just this switch
  HTTPCommand type = HTTPCommand::http_no_command;
  switch(p_request->Verb)
  {
    case HttpVerbOPTIONS:   type = HTTPCommand::http_options;    break;
    case HttpVerbGET:       type = HTTPCommand::http_get;        break;
    case HttpVerbHEAD:      type = HTTPCommand::http_head;       break;
    case HttpVerbPOST:      type = HTTPCommand::http_post;       break;
    case HttpVerbPUT:       type = HTTPCommand::http_put;        break;
    case HttpVerbDELETE:    type = HTTPCommand::http_delete;     break;
    case HttpVerbTRACE:     type = HTTPCommand::http_trace;      break;
    case HttpVerbCONNECT:   type = HTTPCommand::http_connect;    break;
    case HttpVerbMOVE:      type = HTTPCommand::http_move;       break;
    case HttpVerbCOPY:      type = HTTPCommand::http_copy;       break;
    case HttpVerbPROPFIND:  type = HTTPCommand::http_proppfind;  break;
    case HttpVerbPROPPATCH: type = HTTPCommand::http_proppatch;  break;
    case HttpVerbMKCOL:     type = HTTPCommand::http_mkcol;      break;
    case HttpVerbLOCK:      type = HTTPCommand::http_lock;       break;
    case HttpVerbUNLOCK:    type = HTTPCommand::http_unlock;     break;
    case HttpVerbSEARCH:    type = HTTPCommand::http_search;     break;
    default:                // Try to get a less known verb as 'last resort'
                            type = GetUnknownVerb(p_request->pUnknownVerb);
                            if(type == HTTPCommand::http_no_command)
                            {
                              ERRORLOG(ERROR_INVALID_PARAMETER,"Unknown HTTP Verb");
                              return false;
                            }
                            break;
  }

// TODO
//   // Receiving the initiation of an event stream for the server
//   acceptTypes.Trim();
//   if((type == HTTPCommand::http_get) && (eventStream || acceptTypes.Left(17).CompareNoCase("text/event-stream") == 0))
//   {
//     CString absolutePath = CW2A(request->CookedUrl.pAbsPath);
//     EventStream* stream = SubscribeEventStream(site,site->GetSite(),absolutePath,request->RequestId,accessToken);
//     if(stream)
//     {
//       stream->m_baseURL = rawUrl;
//       m_pool->SubmitWork(callback,(void*)stream);
//       HTTP_SET_NULL_ID(&requestId);
//       continue;
//     }
//   }

  // For all types of requests: Create the HTTPMessage
  HTTPMessage* message = new HTTPMessage(type,p_site);
  message->SetURL(rawUrl);
  message->SetAuthorization(authorize);
  message->SetConnectionID(p_request->ConnectionId);
  message->SetContentType(contentType);
  message->SetRemoteDesktop(remDesktop);
  message->SetSender((PSOCKADDR_IN6)sender);
  message->SetCookiePairs(cookie);
  message->SetAcceptEncoding(acceptEncoding);

  // DONE in MarlinModule (gotten by the context)
  //message->SetRequestHandle(p_request->RequestId);
  //message->SetAccessToken(accessToken);

  if(p_site->GetAllHeaders())
  {
    // If requested so, copy all headers to the message
    message->SetAllHeaders(&p_request->Headers);
  }
  else
  {
    // As a minimum, always add the unknown headers
    // in case of a 'POST', as the SOAPAction header is here too!
    message->SetUnknownHeaders(&p_request->Headers);
  }

  // Handle modified-since 
  // Rest of the request is then not needed any more
  if(type == HTTPCommand::http_get && !modified.IsEmpty())
  {
    message->SetHTTPTime(modified);
    if(DoIsModifiedSince(message))
    {
      delete message;
      return nullptr;
    }
  }

  // Find X-HTTP-Method VERB Tunneling
  if(type == HTTPCommand::http_post && p_site->GetVerbTunneling())
  {
    if(message->FindVerbTunneling())
    {
      DETAILLOGV("Request VERB changed to: %s",message->GetVerb());
    }
  }

  // Chunks already read by IIS, so just copy them in the message
  if(p_request->EntityChunkCount)
  {
    ReadEntityChunks(message,p_request);
  }

  // Remember the fact that we should read the rest of the message
  message->SetReadBuffer(p_request->Flags & HTTP_REQUEST_FLAG_MORE_ENTITY_BODY_EXISTS,atoi(contentLength));

  // This is the result
  return message;
}

// Reading the first chunks directly from the request handle from IIS
void
HTTPServerIIS::ReadEntityChunks(HTTPMessage* p_message,PHTTP_REQUEST p_request)
{
  unsigned count = p_request->EntityChunkCount;

  // Read all chunks
  for(unsigned ind = 0; ind < count; ++ind)
  {
    PHTTP_DATA_CHUNK chunk = &p_request->pEntityChunks[ind];
    switch(chunk->DataChunkType)
    {
      case HttpDataChunkFromMemory:            p_message->GetFileBuffer()->AddBuffer((uchar*)chunk->FromMemory.pBuffer
                                                                                    ,chunk->FromMemory.BufferLength);
                                               break;
      case HttpDataChunkFromFileHandle:        // Should not happen upon receive
      case HttpDataChunkFromFragmentCache:     // Fall through
      case HttpDataChunkFromFragmentCacheEx:   ERRORLOG(87,"Unhandled HTTP chunk type from IIS");
                                               break;
    }
  }
}

// Receive incoming HTTP request (p_request->Flags > 0)
bool
HTTPServerIIS::ReceiveIncomingRequest(HTTPMessage* p_message)
{
  UNREFERENCED_PARAMETER(p_message);
  IHttpContext* context = reinterpret_cast<IHttpContext*>(p_message->GetRequestHandle());
  IHttpRequest* httpRequest = context->GetRequest();

  // Second extension interface gives the authentication token of the call
  IHttpRequest2* httpRequest2 = nullptr;
  HRESULT hr = HttpGetExtendedInterface(g_iisServer,httpRequest,&httpRequest2);
  if(SUCCEEDED(hr))
  {
    //     PBYTE tokenInfo = nullptr;
    //     DWORD tokenSize = 0;
    //     httpRequest2->GetChannelBindingToken(&tokenInfo,&tokenSize);
  }

  // Reading the buffer
  FileBuffer* fbuffer = p_message->GetFileBuffer();
  if(fbuffer->AllocateBuffer(p_message->GetContentLength()))
  {
    uchar* buffer   = nullptr;
    size_t size     = NULL;
    DWORD  received = NULL;
    BOOL   pending  = FALSE;
    fbuffer->GetBuffer(buffer,size);
      
    hr = httpRequest->ReadEntityBody(buffer,(DWORD)size,FALSE,&received,&pending);
    if(SUCCEEDED(hr))
    {
      return true;
    }
  }
  return false;
}

// Init the stream response
bool
HTTPServerIIS::InitEventStream(EventStream& p_stream)
{
  UNREFERENCED_PARAMETER(p_stream);
  return false;
}

void
HTTPServerIIS::SetResponseHeader(IHttpResponse* p_response,CString p_name,CString p_value,bool p_replace)
{
  if(p_response->SetHeader(p_name,p_value,(USHORT)p_value.GetLength(),p_replace) != S_OK)
  {
    DWORD   val   = GetLastError();
    CString error = GetLastErrorAsString(val);
    CString bark;
    bark.Format("Cannot set HTTP repsonse header [%s] to value [%s] : %s",p_name,p_value,error);
    ERRORLOG(val,bark);
  }
}

void 
HTTPServerIIS::SetResponseHeader(IHttpResponse* p_response,HTTP_HEADER_ID p_id,CString p_value,bool p_replace)
{
  if(p_response->SetHeader(p_id,p_value,(USHORT)p_value.GetLength(),p_replace) != S_OK)
  {
    DWORD   val   = GetLastError();
    CString error = GetLastErrorAsString(val);
    CString bark;
    bark.Format("Cannot set HTTP repsonse header [%d] to value [%s] : %s",p_id,p_value,error);
    ERRORLOG(val,bark);
  }
}

void
HTTPServerIIS::AddUnknownHeaders(IHttpResponse* p_response,UKHeaders& p_headers)
{
  // Something to do?
  if(p_headers.empty())
  {
    return;
  }
  for(UKHeaders::iterator it = p_headers.begin();it != p_headers.end(); ++it)
  {
    CString name  = it->first;
    CString value = it->second;

    SetResponseHeader(p_response,name,value,false);
  }
}

// Setting the overal status of the response message
void
HTTPServerIIS::SetResponseStatus(IHttpResponse* p_response,USHORT p_status,CString p_statusMessage)
{
  p_response->SetStatus(p_status,p_statusMessage);
}

// Sending response for an incoming message
void       
HTTPServerIIS::SendResponse(HTTPMessage* p_message)
{
  IHttpContext*   context     = reinterpret_cast<IHttpContext*>(p_message->GetRequestHandle());
  IHttpResponse*  response    = context ? context->GetResponse() : nullptr;
  FileBuffer*     buffer      = p_message->GetFileBuffer();
  CString         contentType("application/octet-stream"); 

  // See if there is something to send
  if(context == nullptr || response == nullptr)
  {
    ERRORLOG(ERROR_INVALID_PARAMETER,"SendResponse: nothing to send");
    return;
  }


  // Respond to general HTTP status
  int status = p_message->GetStatus();
  if(HTTP_STATUS_SERVER_ERROR <= status && status <= HTTP_STATUS_LAST)
  {
    RespondWithServerError(p_message->GetHTTPSite(),p_message,status,"","");
    p_message->SetRequestHandle(NULL);
    return;
  }
  if(HTTP_STATUS_BAD_REQUEST <= status && status < HTTP_STATUS_SERVER_ERROR)
  {
    RespondWithClientError(p_message->GetHTTPSite(),p_message,status,"","");
    p_message->SetRequestHandle(NULL);
    return;
  }

  // Setting the response status
  SetResponseStatus(response,(USHORT) p_message->GetStatus(),GetStatusText(p_message->GetStatus()));

  // Add a known header. (octet-stream or the message content type)
  if(!p_message->GetContentType().IsEmpty())
  {
    contentType = p_message->GetContentType();
  }

  // AddKnownHeader(response,HttpHeaderContentType,contentType);
  SetResponseHeader(response,HttpHeaderContentType,contentType,true);

  // Add the server header or suppress it
  switch(m_sendHeader)
  {
    case SendHeader::HTTP_SH_MICROSOFT:   // Do nothing, Microsoft will add the server header
                                          break;
    case SendHeader::HTTP_SH_MARLIN:      SetResponseHeader(response,HttpHeaderServer,MARLIN_SERVER_VERSION,true);
                                          break;
    case SendHeader::HTTP_SH_APPLICATION: SetResponseHeader(response,HttpHeaderServer,m_name,true);
                                          break;
    case SendHeader::HTTP_SH_WEBCONFIG:   SetResponseHeader(response,HttpHeaderServer,m_configServerName,true);
                                          break;
    case SendHeader::HTTP_SH_HIDESERVER:  // Fill header with empty string will suppress it
                                          SetResponseHeader(response,HttpHeaderServer,"",true);
                                          break;
  }

  // Add cookies to the unknown response headers
  // Because we can have more than one Set-Cookie: header
  // and HTTP API just supports one set-cookie.
  UKHeaders ukheaders;
  Cookies& cookies = p_message->GetCookies();
  for(auto& cookie : cookies.GetCookies())
  {
    ukheaders.insert(std::make_pair("Set-Cookie",cookie.GetSetCookieText()));
  }

  // Other unknown headers
  HTTPSite* site = p_message->GetHTTPSite();
  if(site)
  {
    site->AddSiteOptionalHeaders(ukheaders);
  }

  // Add extra headers from the message
  HeaderMap* map = p_message->GetHeaderMap();
  for(HeaderMap::iterator it = map->begin(); it != map->end(); ++it)
  {
    ukheaders.insert(std::make_pair(it->first,it->second));
  }

  // Possible zip the contents, and add content-encoding header
  if(p_message->GetHTTPSite()->GetHTTPCompression() && buffer)
  {
    // But only if the client side requested it
    if(p_message->GetAcceptEncoding().Find("gzip") >= 0)
    {
      if(buffer->ZipBuffer())
      {
        DETAILLOGV("GZIP the buffer to size: %lu",buffer->GetLength());
        ukheaders.insert(std::make_pair("Content-Encoding","gzip"));
      }
    }
  }

  // Now add all unknown headers to the response
  AddUnknownHeaders(response,ukheaders);

  // Now after the compression, add the total content length
  CString contentLength;
  size_t totalLength = buffer ? buffer->GetLength() : 0;
#ifdef _WIN64
  contentLength.Format("%I64u",totalLength);
#else
  contentLength.Format("%lu",totalLength);
#endif
  SetResponseHeader(response,HttpHeaderContentLength,contentLength,true);

  // Dependent on the filling of FileBuffer
  // Send 1 or more buffers or the file
  if(buffer->GetHasBufferParts())
  {
    SendResponseBufferParts(response,buffer,totalLength);
  }
  else if(buffer->GetFileName().IsEmpty())
  {
    SendResponseBuffer(response,buffer,totalLength);
  }
  else
  {
    SendResponseFileHandle(response,buffer);
  }
  if(GetLastError())
  {
    // Error handler
    CString message = GetLastErrorAsString(tls_lastError);
    m_log->AnalysisLog(__FUNCTION__, LogType::LOG_ERROR,true,"HTTP Answer [%d:%s]",GetLastError(),message);
    // Reset the last error
    SetError(NO_ERROR);
  }

  // Do **NOT** send an answer twice
  p_message->SetRequestHandle(NULL);
}


// Send a response in one-go
DWORD 
HTTPServerIIS::SendResponse(HTTPSite*    p_site
                           ,HTTPMessage* p_message
                           ,USHORT       p_statusCode
                           ,PSTR         p_reason
                           ,PSTR         p_entityString
                           ,CString      p_authScheme
                           ,PSTR         p_cookie      /*= NULL*/
                           ,PSTR         p_contentType /*= NULL*/)
{
  IHttpContext*   context   = reinterpret_cast<IHttpContext*>(p_message->GetRequestHandle());
  IHttpResponse*  response  = context ? context->GetResponse() : nullptr;
  HTTP_DATA_CHUNK dataChunk;
  DWORD           result = 0L;
  CString         challenge;
  memset(&dataChunk,0,sizeof(HTTP_DATA_CHUNK));

  // Setting the response status
  SetResponseStatus(response,p_statusCode,p_reason);

  // Add a known header.
  if(p_contentType && strlen(p_contentType))
  {
    SetResponseHeader(response,HttpHeaderContentType,p_contentType,true);
  }
  else
  {
    SetResponseHeader(response,HttpHeaderContentType,"text/html",true);
  }
  // Adding authentication schema challenge
  if(p_statusCode == HTTP_STATUS_DENIED)
  {
    // Add authentication scheme
    challenge = BuildAuthenticationChallenge(p_authScheme,p_site->GetAuthenticationRealm());
    SetResponseHeader(response,HttpHeaderWwwAuthenticate,challenge,true);
  }
  else if (p_statusCode >= HTTP_STATUS_AMBIGUOUS)
  {
    // Log responding with error status code
    HTTPError(__FUNCTION__,p_statusCode,"Returning from: " + p_site->GetSite());
  }

  // Adding the body
  if(p_entityString && strlen(p_entityString) > 0)
  {
    // Add an entity chunk.
    dataChunk.DataChunkType           = HttpDataChunkFromMemory;
    dataChunk.FromMemory.pBuffer      = p_entityString;
    dataChunk.FromMemory.BufferLength = (ULONG) strlen(p_entityString);
  }

  // Adding a cookie
  if(p_cookie && strlen(p_cookie) > 0)
  {
    SetResponseHeader(response,HttpHeaderSetCookie,p_cookie,false);
  }

  DWORD sent = 0L;
  BOOL  completion = false;
  HRESULT hr = response->WriteEntityChunks(&dataChunk,1,false,false,&sent,&completion);

  if(SUCCEEDED(hr))
  {
    DETAILLOGV("HTTP Sent %d %s %s",p_statusCode,p_reason,p_entityString);
  }
  else
  {
    result = GetLastError();
    ERRORLOG(result,"SendHttpResponse");
  }
  return result;
}

// Subfunctions for SendResponse
bool
HTTPServerIIS::SendResponseBuffer(IHttpResponse*  p_response
                                 ,FileBuffer*     p_buffer
                                 ,size_t          p_totalLength)
{
  uchar* entity       = NULL;
  DWORD  result       = 0;
  size_t entityLength = 0;
  HTTP_DATA_CHUNK dataChunk;

  // Get buffer to send
  if(p_totalLength)
  {
    p_buffer->GetBuffer(entity,entityLength);
  }

  // Only if a buffer present
  if(entity && entityLength)
  {
    // Add an entity chunk.
    dataChunk.DataChunkType           = HttpDataChunkFromMemory;
    dataChunk.FromMemory.pBuffer      = entity;
    dataChunk.FromMemory.BufferLength = (ULONG)entityLength;

    DWORD sent = 0L;
    BOOL  completion = false;
    HRESULT hr = p_response->WriteEntityChunks(&dataChunk,1,false,false,&sent,&completion);
    if(SUCCEEDED(hr))
    {
      DETAILLOGV("ResponseBuffer [%ul] bytes sent",entityLength);
    }
    else
    {
      result = GetLastError();
      ERRORLOG(result,"ResponseBuffer");
    }
  }
  return (result == NO_ERROR);
}

void
HTTPServerIIS::SendResponseBufferParts(IHttpResponse* p_response
                                      ,FileBuffer*    p_buffer
                                      ,size_t         p_totalLength)
{
  int    transmitPart = 0;
  uchar* entityBuffer = NULL;
  size_t entityLength = 0;
  size_t totalSent    = 0;
  DWORD  bytesSent    = 0;
  HTTP_DATA_CHUNK dataChunk;
  memset(&dataChunk,0,sizeof(HTTP_DATA_CHUNK));

  while(p_buffer->GetBufferPart(transmitPart,entityBuffer,entityLength))
  {
    if(entityBuffer)
    {
      // Add an entity chunk.
      dataChunk.DataChunkType           = HttpDataChunkFromMemory;
      dataChunk.FromMemory.pBuffer      = entityBuffer;
      dataChunk.FromMemory.BufferLength = (ULONG)entityLength;
    }
    // Flag to calculate the last sending part
    bool moreData = (totalSent + entityLength) < p_totalLength;
    BOOL completion = false;

    HRESULT hr = p_response->WriteEntityChunks(&dataChunk,1,false,moreData,&bytesSent,&completion);

    if(SUCCEEDED(hr))
    {
      DETAILLOGV("HTTP ResponseBufferPart [%d] bytes sent",bytesSent);
    }
    else
    {
      DWORD result = GetLastError();
      ERRORLOG(result,"HTTP ResponseBufferPart error");
      break;
    }
    // Next buffer part
    totalSent += entityLength;
    ++transmitPart;
  }
}

void
HTTPServerIIS::SendResponseFileHandle(IHttpResponse* p_response,FileBuffer* p_buffer)
{
  DWORD  result    = 0;
  HANDLE file      = NULL;
  HTTP_DATA_CHUNK dataChunk;
  memset(&dataChunk,0,sizeof(HTTP_DATA_CHUNK));

  // File to transmit
  if(p_buffer->OpenFile() == false)
  {
    ERRORLOG(GetLastError(),"OpenFile for SendHttpResponse");
    return;
  }
  // Get the filehandle from buffer
  file = p_buffer->GetFileHandle();

  // See if file is under 4G large
  DWORD high = 0L;
  ULONG fileSize = GetFileSize(file,&high);
  if(high)
  {
    // 413: Request entity too large
    SendResponseError(p_response,m_clientErrorPage,413,"Request entity too large");
    // Error state
    ERRORLOG(ERROR_INVALID_PARAMETER,"File to send is too big (>4G)");
    // Close the filehandle
    p_buffer->CloseFile();
    return;
  }

  // Send entity body from a file handle.
  dataChunk.DataChunkType = HttpDataChunkFromFileHandle;
  dataChunk.FromFileHandle.ByteRange.StartingOffset.QuadPart = 0;
  dataChunk.FromFileHandle.ByteRange.Length.QuadPart = fileSize; // HTTP_BYTE_RANGE_TO_EOF;
  dataChunk.FromFileHandle.FileHandle = file;

  DWORD sent = 0L;
  BOOL  completion = false;

  // Writing the chunk
  HRESULT hr = p_response->WriteEntityChunks(&dataChunk,1,false,false,&sent,&completion);
  if(SUCCEEDED(hr))
  {
    DETAILLOGV("SendResponseEntityBody for file. Bytes: %ul",fileSize);
  }
  else
  {
    result = GetLastError();
    ERRORLOG(result,"SendResponseEntityBody for file");
  }
  // Now close our filehandle
  p_buffer->CloseFile();
}

void
HTTPServerIIS::SendResponseError(IHttpResponse* p_response
                                ,CString&       p_page
                                ,int            p_error
                                ,const char*    p_reason)
{
  DWORD result = 0;
  CString sending;
  HTTP_DATA_CHUNK dataChunk;
  memset(&dataChunk,0,sizeof(HTTP_DATA_CHUNK));

  // Format our error page
  sending.Format(p_page,p_error,p_reason);

  // Add an entity chunk.
  dataChunk.DataChunkType           = HttpDataChunkFromMemory;
  dataChunk.FromMemory.pBuffer      = (void*) sending.GetString();
  dataChunk.FromMemory.BufferLength = sending.GetLength();

  DWORD sent = 0L;
  BOOL  completion = false;

  // Writing the chunk
  HRESULT hr = p_response->WriteEntityChunks(&dataChunk,1,false,false,&sent,&completion);
  if(SUCCEEDED(hr))
  {
    DETAILLOGV("SendResponseError. Bytes sent: %d",sent);
    if(completion == false)
    {
      ERRORLOG(ERROR_INVALID_FUNCTION,"ResponseFileHandle: But IIS does not expect to deliver the contents!!");
    }
  }
  else
  {
    result = GetLastError();
    ERRORLOG(result,"SendResponseEntityBody for file");
  }
}

