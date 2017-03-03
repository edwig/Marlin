/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: HTTPSite.cpp
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
#include "stdafx.h"
#include "HTTPSite.h"
#include "HTTPURLGroup.h"
#include "EnsureFile.h"
#include "Analysis.h"
#include "AutoCritical.h"
#include "GenerateGUID.h"
#include "Crypto.h"
#include "WinINETError.h"
#include "ErrorReport.h"
#include <winerror.h>
#include <sddl.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// Logging via the server
#define DETAILLOG1(text)        m_server->DetailLog (__FUNCTION__,LogType::LOG_INFO,text)
#define DETAILLOGS(text,extra)  m_server->DetailLogS(__FUNCTION__,LogType::LOG_INFO,text,extra)
#define DETAILLOGV(text,...)    m_server->DetailLogV(__FUNCTION__,LogType::LOG_INFO,text,__VA_ARGS__)
#define WARNINGLOG(text,...)    m_server->DetailLogV(__FUNCTION__,LogType::LOG_WARN,text,__VA_ARGS__)
#define ERRORLOG(code,text)     m_server->ErrorLog  (__FUNCTION__,code,text)
#define CRASHLOG(code,text)     if(m_server->GetLogfile())\
                                {\
                                  m_server->GetLogfile()->AnalysisLog(__FUNCTION__,LogType::LOG_ERROR,false,text); \
                                  m_server->GetLogfile()->AnalysisLog(__FUNCTION__,LogType::LOG_ERROR,true,"Error code: %d",code); \
                                }

// Cleanup handler after a crash-report
__declspec(thread) SiteHandler*      g_cleanup  = nullptr;
__declspec(thread) CRITICAL_SECTION* g_throttle = nullptr;

// THE XTOR
HTTPSite::HTTPSite(HTTPServer*   p_server
                  ,int           p_port
                  ,CString       p_site
                  ,CString       p_prefix
                  ,HTTPSite*     p_mainSite /* = nullptr */
                  ,LPFN_CALLBACK p_callback /* = nullptr */)
        :m_server(p_server)
        ,m_port(p_port)
        ,m_site(p_site)
        ,m_prefixURL(p_prefix)
        ,m_mainSite(p_mainSite)
        ,m_callback(p_callback)
{
  if(m_mainSite)
  {
    m_isSubsite = true;
  }
  if(p_callback == nullptr)
  {
    m_callback = (LPFN_CALLBACK) SITE_CALLBACK_HTTP;
  }
  InitializeCriticalSection(&m_filterLock);
  InitializeCriticalSection(&m_sessionLock);
}

HTTPSite::~HTTPSite()
{
  CleanupFilters();
  CleanupHandlers();
  CleanupThrotteling();
  DeleteCriticalSection(&m_filterLock);
  DeleteCriticalSection(&m_sessionLock);
}

// Set site's callback function
void
HTTPSite::SetCallback(LPFN_CALLBACK p_callback)
{
  // Record a new callback.
  // In case we try to set it to a NULL pointer -> Default to a 
  // HTTPThreadpool callback mechanism.
  m_callback = p_callback ? p_callback : (LPFN_CALLBACK) SITE_CALLBACK_HTTP;
}

// Cleanup all site handlers
void
HTTPSite::CleanupHandlers()
{
  for(auto& handler : m_handlers)
  {
    if(handler.second.m_owner)
    {
      delete handler.second.m_handler;
    }
  }
  m_handlers.clear();
}

// Cleanup all filters
void
HTTPSite::CleanupFilters()
{
  for(FilterMap::iterator it = m_filters.begin();it != m_filters.end();++it)
  {
    delete it->second;
  }
  m_filters.clear();
}

// Remove all critical sections from the throtteling map
void
HTTPSite::CleanupThrotteling()
{
  for(ThrottlingMap::iterator it = m_throttels.begin();it != m_throttels.end(); ++it)
  {
    DeleteCriticalSection(it->second);
    delete it->second;
  }
  m_throttels.clear();
}

// OPTIONAL: Set one or more text-based content types
void
HTTPSite::AddContentType(CString p_extension,CString p_contentType)
{
  // Mapping is on lower case
  p_extension.MakeLower();
  p_extension.TrimLeft('.');

  MediaTypeMap::iterator it = m_contentTypes.find(p_extension);

  if(it == m_contentTypes.end())
  {
    // Insert a new one
    m_contentTypes.insert(std::make_pair(p_extension, MediaType(p_extension,p_contentType)));
  }
  else
  {
    // Overwrite the media type
    it->second.SetExtension(p_extension);
    it->second.SetContentType(p_contentType);
  }
}

// Getting a registered content type for a file extension
CString
HTTPSite::GetContentType(CString p_extension)
{
  // Mapping is on lower case
  p_extension.MakeLower();
  p_extension.TrimLeft('.');

  // STEP 1: Look in our own mappings
  MediaTypeMap::iterator it = m_contentTypes.find(p_extension);
  if(it != m_contentTypes.end())
  {
    return it->second.GetContentType();
  }

  // STEP 2: Finding the general default content type
  return g_media.FindContentTypeByExtension(p_extension);
}

// Finding the registered content type from the full resource name
CString
HTTPSite::GetContentTypeByResourceName(CString p_pathname)
{
  EnsureFile ensure;
  CString extens = ensure.ExtensionPart(p_pathname);
  return GetContentType(extens);
}

// OPTIONAL Set filter for HTTP command(s)
bool
HTTPSite::SetFilter(unsigned p_priority,SiteFilter* p_filter)
{
  // Cannot reset a filter
  if(p_filter == nullptr)
  {
    return false;
  }
  // Remember our site. For some filters this does some processing
  p_filter->SetSite(this);

  // Lock from here
  AutoCritSec lock(&m_filterLock);

  FilterMap::iterator it = m_filters.find(p_priority);
  if(it == m_filters.end())
  {
    // Not found: we can add it
    m_filters.insert(std::make_pair(p_priority,p_filter));
    DETAILLOGV("Setting site filter [%s] for [%s] with priority: %d"
              ,p_filter->GetName(),m_site,p_priority);
    return true;
  }
  // already filter for this priority
  CString msg;
  msg.Format("Site filter for [%s] with priority [%d] already exists!",m_site,p_priority);
  ERRORLOG(ERROR_ALREADY_EXISTS,msg);
  return false;
}

SiteFilter*
HTTPSite::GetFilter(unsigned p_priority)
{
  AutoCritSec lock(&m_filterLock);

  FilterMap::iterator it = m_filters.find(p_priority);
  if(it != m_filters.end())
  {
    return it->second;
  }
  return nullptr;
}

void
HTTPSite::SetIsEventStream(bool p_stream)
{
  m_isEventStream = p_stream;

  if(p_stream)
  {
    if(m_callback == (LPFN_CALLBACK)SITE_CALLBACK_HTTP)
    {
      m_callback = (LPFN_CALLBACK)SITE_CALLBACK_EVENT;
    }
  }
  else
  {
    if(m_callback == (LPFN_CALLBACK)SITE_CALLBACK_EVENT)
    {
      m_callback = (LPFN_CALLBACK)SITE_CALLBACK_HTTP;
    }
  }
}

// MANDATORY: Setting a handler for HTTP commands
void
HTTPSite::SetHandler(HTTPCommand p_command,SiteHandler* p_handler,bool p_owner /*=true*/)
{
  // Names are in HTTPMessage.cpp!
  extern const char* headers[];

  HandlerMap::iterator it = m_handlers.find(p_command);

  // Remove handler
  if(p_handler == nullptr)
  {
    if(it != m_handlers.end())
    {
      if(it->second.m_owner)
      {
        delete it->second.m_handler;
      }
      m_handlers.erase(it);
      DETAILLOGS("Removing site handler for HTTP ",headers[(unsigned)p_command]);
    }
    return;
  }

  // Check the command
  if(p_command == HTTPCommand::http_no_command || p_command == HTTPCommand::http_response)
  {
    ERRORLOG(ERROR_INVALID_PARAMETER,"Invalid HTTPCommand for HTTPSite::SetHandler");
    return;
  }

  // p_handler is NOT a nullptr
  // Remember our site
  p_handler->SetSite(this);

  // Register the handler in the handler map
  if(it != m_handlers.end())
  {
    // Not the first handler for this command
    // Add to the end of the chain of handlers
    SiteHandler* handler = it->second.m_handler;
    while(handler && handler->GetNextHandler())
    {
      handler = handler->GetNextHandler();
    }
    handler->SetNextHandler(p_handler);
  }
  else
  {
    // First handler: register it directly
    RegHandler reg;
    reg.m_owner   = p_owner;
    reg.m_handler = p_handler;
    m_handlers[p_command] = reg;
  }
  DETAILLOGS("Setting site handler for HTTP ",headers[(unsigned)p_command]);
}

// Finding the SiteHandler registration
RegHandler*
HTTPSite::FindSiteHandler(HTTPCommand p_command)
{
  HandlerMap::iterator it = m_handlers.find(p_command);
  if(it != m_handlers.end())
  {
    return &it->second;
  }
  return nullptr;
}

SiteHandler*    
HTTPSite::GetSiteHandler(HTTPCommand p_command)
{
  HandlerMap::iterator it = m_handlers.find(p_command);
  if(it != m_handlers.end())
  {
    return it->second.m_handler;
  }
  return nullptr;
}

CString
HTTPSite::GetWebroot()
{
  if(m_webroot.IsEmpty())
  {
    // Webroot of site never set, or reset
    // Reflect general webroot of the server
    return m_server->GetWebroot();
  }
  // Our webroot is the one!
  return m_webroot;
}

bool
HTTPSite::CheckReliable()
{
  if(m_reliable && m_async)
  {
    ERRORLOG(ERROR_INVALID_PARAMETER,"Asynchrone modus en reliable-messaging gaan niet samen");
    return false;
  }
  if(m_reliable)
  {
    if(m_scheme.IsEmpty())
    {
      // Not strictly an error, but issue a serious warning against using RM without authorization
      WARNINGLOG("ReliableMessaging protocol used without an authorization scheme.");
    }
  }
  return true;
}

bool
HTTPSite::StopSite(bool p_force /*=false*/)
{
  // Call all filters 'OnStopSite' methods
  for(auto& filter : m_filters)
  {
    filter.second->OnStopSite();
  }

  // Call all site handlers 'OnStopSite' methods
  for(auto& handler : m_handlers)
  {
    SiteHandler* shand = handler.second.m_handler;
    while(shand)
    {
      shand->OnStopSite();
      shand = shand->GetNextHandler();
    }
  }

  // Try to remove site from the server
  if(m_server->DeleteSite(m_port,m_site,p_force) == false)
  {
    ERRORLOG(ERROR_ACCESS_DENIED,"Cannot remove sub-site before main site: " + m_site);
    return false;
  }
  return true;
}

// Remove the site from the URL group
bool
HTTPSite::RemoveSiteFromGroup()
{
  if(m_group)
  {
    USES_CONVERSION;
    ULONG retCode  = NO_ERROR;
    wstring uniURL = A2CW(m_prefixURL);
    HTTP_URL_GROUP_ID group = m_group->GetUrlGroupID();

    if(m_isSubsite == false)
    {
      retCode = HttpRemoveUrlFromUrlGroup(group,uniURL.c_str(),0);
      if(retCode != NO_ERROR)
      {
        ERRORLOG(retCode,"Cannot remove site from URL group: " + m_site);
      }
    }
    if(retCode == NO_ERROR)
    {
      // Unregister the site
      m_group->UnRegisterSite(this);

      DETAILLOGS("Removed URL site: ",m_site);
      return true;
    }
  }
  else
  {
    ERRORLOG(ERROR_INVALID_PARAMETER,"No recorded URL-Group. Cannot remove URL Site: " + m_site);
  }
  return false;
}

// Come here to handle our HTTP message gotten from the server
// through the HTTPThreadpool
// This is the MAIN entry point for all traffic to this site.
void 
HTTPSite::HandleHTTPMessage(HTTPMessage* p_message)
{
  bool didError = false;
  SiteHandler* handler  = nullptr;

  __try
  {
    // HTTP Throttling is one call per calling address at the time
    if(m_throttling)
    {
      g_throttle = StartThrottling(p_message);
    }

    // Try to read the body / rest of the message
    // This is now done by the threadpool thread, so the central
    // server has more time to handle the incoming requests.
    if(p_message->GetReadBuffer())
    {
      if(m_server->ReceiveIncomingRequest(p_message) == false)
      {
        // Error already report to log, EOF or stream not read
        p_message->Reset();
        p_message->SetStatus(HTTP_STATUS_GONE);
        SendResponse(p_message);
      }
    }

    // Control Origin Resource Sharing
    if(m_useCORS)
    {
      // See that message has expected origin
      CheckCorsOrigin(p_message);
    }

    // If site in asynchronous SOAP/XML mode
    if(m_async)
    {
      // Send back an OK immediately, even before processing the message
      // In fact we might totally ignore it.
      AsyncResponse(p_message);
    }

    // Call all site filters first, in priority order
    // But only if we do have filters
    if(!m_filters.empty())
    {
      CallFilters(p_message);
    }

    // Call the correct handler for this site
    handler = GetSiteHandler(p_message->GetCommand());
    if(handler)
    {
      handler->HandleMessage(p_message);
    }
    else
    {
      HandleHTTPMessageDefault(p_message);
    }

    // Remove the throttling lock!
    if(m_throttling)
    {
      EndThrottling(g_throttle);
    }
  }
  __except(
//#ifdef _DEBUG
    // See if we are live debugging in Visual Studio
    // IsDebuggerPresent() ? EXCEPTION_CONTINUE_SEARCH :
//#endif // _DEBUG
    // After calling the Error::Send method, the Windows stack get unwinded
    // We need to detect the fact that a second exception can occur,
    // so we do **not** call the error report method again
    // Otherwise we would end into an infinite loop
    g_exception ? EXCEPTION_EXECUTE_HANDLER :
    g_exception = true,
    g_exception = ErrorReport::Report(GetExceptionCode(),GetExceptionInformation(),m_webroot,m_site),
    EXCEPTION_EXECUTE_HANDLER)
  {
    if(g_exception)
    {
      // Error while sending an error report
      // This error can originate from another thread, OR from the sending of this error report
      CRASHLOG(310L,"DOUBLE INTERNAL ERROR while making an error report.!!");
      g_exception = false;
    }
    else
    {
      CRASHLOG(WER_S_REPORT_UPLOADED,"CRASH: Errorreport has been made");
    }
    didError = true;
  }  

  // End any impersonation done in the handlers
  // But do that after the _except catching
  ::RevertToSelf();

  // After the SEH: See if we need to post-Handle the error
  if(didError)
  {
    PostHandle(p_message);
  }

  // End of the line: created in HTTPServer::RunServer
  // It gets now destroyed after everything has been done
  delete p_message;
}

// Handle the error after an error report
void
HTTPSite::PostHandle(HTTPMessage* p_message)
{
  __try
  {
    // If we did throtteling, remove the lock
    if(m_throttling)
    {
      EndThrottling(g_throttle);
    }

    // Respond to the client with a server error in all cases. 
    // Do NOT send the error report to the client. No need to show the error-stack!!
    p_message->Reset();
    p_message->GetFileBuffer()->Reset();
    p_message->GetFileBuffer()->ResetFilename();
    p_message->SetStatus(HTTP_STATUS_SERVER_ERROR);
    m_server->SendResponse(p_message);

    // See if we need to cleanup after the call for the site handler
    if(g_cleanup)
    {
      g_cleanup->CleanUp(p_message);
    }
  }
  __except(
//#ifdef _DEBUG
    // See if we are live debugging in Visual Studio
    // IsDebuggerPresent() ? EXCEPTION_CONTINUE_SEARCH :
//#endif // _DEBUG
    // After calling the Error::Send method, the Windows stack get unwinded
    // We need to detect the fact that a second exception can occur,
    // so we do **not** call the error report method again
    // Otherwise we would end into an infinite loop
    g_exception ? EXCEPTION_EXECUTE_HANDLER :
    g_exception = true,
    g_exception = ErrorReport::Report(GetExceptionCode(),GetExceptionInformation(),m_webroot,m_site),
    EXCEPTION_EXECUTE_HANDLER)
  {
    if(g_exception)
    {
      // Error while sending an error report
      // This error can originate from another thread, OR from the sending of this error report
      CRASHLOG(0xFFFF,"DOUBLE INTERNAL ERROR while making an error report.!!");
      g_exception = false;
    }
    else
    {
      CRASHLOG(WER_S_REPORT_UPLOADED,"CRASH: Errorreport has been made");
    }
  }
}

// Call the filters in priority order
void
HTTPSite::CallFilters(HTTPMessage* p_message)
{
  FilterMap filters;

  // Use lock to make a copy of the map
  {
    AutoCritSec lock(&m_filterLock);
    filters = m_filters;
  }

  // Now call all filters
  for(FilterMap::iterator it = filters.begin(); it != filters.end();++it)
  {
    it->second->Handle(p_message);
  }
}

// Direct asynchronous response
void
HTTPSite::AsyncResponse(HTTPMessage* p_message)
{
  // Send back an OK immediately, without waiting for
  // worker thread to process the message in the callback
  m_server->SendResponse(this,p_message,HTTP_STATUS_OK,"","","");
  p_message->SetRequestHandle(NULL);
  DETAILLOG1("Sent a HTTP status 200 = OK for asynchroneous message");
}

// See that message has expected origin
void
HTTPSite::CheckCorsOrigin(HTTPMessage* p_message)
{
  if(!m_allowOrigin.IsEmpty() && m_useCORS)
  {
    CString comesFrom = p_message->GetHeader("Origin");
    if(m_allowOrigin.CompareNoCase(comesFrom))
    {
      ERRORLOG(ERROR_ACCESS_DENIED,"CORS origin server error: " + comesFrom);
      // Message is not allowed
      p_message->Reset();
      // Empty the response part. Just to be sure!
      CString empty;
      p_message->GetFileBuffer()->Reset();
      p_message->SetFile(empty);
      // Answer with error
      p_message->SetStatus(HTTP_STATUS_DENIED);
      m_server->SendResponse(p_message);
    }
  }
}

// Call the correct EventStream handler
void 
HTTPSite::HandleEventStream(EventStream* p_stream)
{
  SiteHandler* handler = GetSiteHandler(HTTPCommand::http_get);

  if(handler)
  {
    handler->HandleStream(p_stream);
  }
  else
  {
    ERRORLOG(ERROR_INVALID_PARAMETER,"Event stream can only be initialized through a HTTP GET to a event-handling site.");
    HTTPMessage msg(HTTPCommand::http_response,HTTP_STATUS_BAD_REQUEST);
    msg.SetRequestHandle(p_stream->m_requestID);
    HandleHTTPMessageDefault(&msg);
  }
}

// Default is to respond with a status 400 (BAD REQUEST)
// We come here in case we get traffic for which no handlers have been registered
// All we can do or should do is send a BAD-REQUEST message to this client
void
HTTPSite::HandleHTTPMessageDefault(HTTPMessage* p_message)
{
  DETAILLOG1("Default HTTPSite repsonse HTTP_STATUS_BAD_REQUEST (400)");
  p_message->SetCommand(HTTPCommand::http_response);
  p_message->SetStatus(HTTP_STATUS_BAD_REQUEST);
  m_server->SendResponse(p_message);
}

// Getting the 'ALLOW'ed handlers for the HTTP OPTION request
// This will make a list of all current handler types
CString
HTTPSite::GetAllowHandlers()
{
  CString allow;

  // Simply list all filled handlers
  for(auto& handler : m_handlers)
  {
    allow += " ";
    allow += headers[(unsigned)handler.first];
    allow += " ";
  }

  // Create comma separated list
  allow.Trim();
  allow.Replace("  ",", ");

  return allow;
}

// Send responses to the HTTPServer (if any)
bool 
HTTPSite::SendResponse(HTTPMessage* p_message)
{
  if(m_server)
  {
    m_server->SendResponse(p_message);
    return true;
  }
  return false;
}

bool 
HTTPSite::SendResponse(SOAPMessage* p_message)
{
  if(m_server)
  {
    m_server->SendResponse(p_message);
    return true;
  }
  return false;
}

bool
HTTPSite::SendResponse(JSONMessage* p_message)
{
  if(m_server)
  {
    m_server->SendResponse(p_message);
    return true;
  }
  return false;
}

// Removing a filter with a certain priority
bool 
HTTPSite::RemoveFilter(unsigned p_priority)
{
  AutoCritSec lock(&m_filterLock);

  FilterMap::iterator it = m_filters.find(p_priority);
  if(it == m_filters.end())
  {
    CString msg;
    msg.Format("Filter with priority [%d] for site [%s] not found!",p_priority,m_site);
    ERRORLOG(ERROR_NOT_FOUND,msg);
    return false;
  }
  // Remove the filter
  delete it->second;
  // Remove from map
  m_filters.erase(it);

  return true;
}

//////////////////////////////////////////////////////////////////////////
//
// HTTP Throtteling
//
//////////////////////////////////////////////////////////////////////////

CRITICAL_SECTION* 
HTTPSite::StartThrottling(HTTPMessage* p_message)
{
  CRITICAL_SECTION* section = nullptr;

  // Get the address of the sender (USER/IP/Desktop combination)
  // Do not fill in the URL absPath member !!
  SessionAddress address;
  address.m_userSID = GetStringSID(p_message->GetAccessToken());
  address.m_desktop = p_message->GetRemoteDesktop();
  memcpy(&address.m_address,p_message->GetSender(),sizeof(SOCKADDR_IN6));

  // Finding/creating the throttle
  ThrottlingMap::iterator it = m_throttels.find(address);
  if(it == m_throttels.end())
  {
    // Create a throttle for this address
    section = new CRITICAL_SECTION();
    InitializeCriticalSection(section);
    m_throttels.insert(std::make_pair(address,section));
    it = m_throttels.find(address);
  }
  else
  {
    // Already created
    section = it->second;
  }
  // Throttle is now found, consume it
  EnterCriticalSection(section);

  return section;
}

void
HTTPSite::EndThrottling(CRITICAL_SECTION*& p_throttle)
{
  // That easy!
  LeaveCriticalSection(p_throttle);

  // See if we must start the cleanup process
  // Thread has already serviced the HTTP call
  if(m_throttels.size() > MAX_HTTP_THROTTLES)
  {
    TryFlushThrottling();
  }
}

// Erase all throttles that are not in use
// But leave the once that are currently used by a session call
// And so reducing the amount of memory used by the site
void
HTTPSite::TryFlushThrottling()
{
  AutoCritSec lock(&m_sessionLock);

  ThrottlingMap::iterator it = m_throttels.begin();
  while(it != m_throttels.end())
  {
    if(TryEnterCriticalSection(it->second))
    {
      DeleteCriticalSection(it->second);
      delete it->second;
      it = m_throttels.erase(it);
    }
    else
    {
      ++it;
    }
  }
}

//////////////////////////////////////////////////////////////////////////
//
// WS-Reliable Messaging handling
//
//////////////////////////////////////////////////////////////////////////

// Check WS-ReliableMessaging protocol
// return true  -> Klaar met verwerken
// return false -> Callback moet verder gaan met verwerken
bool
HTTPSite::HttpReliableCheck(SOAPMessage* p_message)
{
  // Get the address of the sender (USER/IP/Desktop combination)
  SessionAddress address;
  address.m_userSID = GetStringSID(p_message->GetAccessToken());
  address.m_desktop = p_message->GetRemoteDesktop();
  address.m_absPath = p_message->GetAbsolutePath();
  memcpy(&address.m_address,p_message->GetSender(),sizeof(SOCKADDR_IN6));
  address.m_absPath.MakeLower();

  if(p_message->GetInternalError() != XmlError::XE_NoError)
  {
    // Must at least get a SOAP/XML message
    SendSOAPFault(address
                 ,p_message
                 ,"Client"
                 ,"Not a valid SOAP/XML message"
                 ,"Client program"
                 ,"Ill formed XML message. Review your program logic. Reported: " + p_message->GetInternalErrorString());
    return true;
  }

  // Handle the WS-ReliableMessaging protocol
  if(p_message->GetNamespace() == NAMESPACE_RELIABLE)
  {
    if(!GetReliable())
    {
      SendSOAPFault(address
                   ,p_message
                   ,"Client"
                   ,"Must not use WS-ReliableMessaging"
                   ,"Settings"
                   ,"Encountered a SOAP/XML request using the WS-ReliableMessaging protocol. Review your settings!");
      return true;
    }
    // For WS-ReliableMessaging we must use a logged in session
    if(address.m_userSID.IsEmpty() && m_reliableLogIn)
    {
      // Return SOAP FAULT: Not logged with a user/password combination
      SendSOAPFault(address
                   ,p_message
                   ,"Client"
                   ,"Not logged with a user/password combination"
                   ,"User"
                   ,"User should login with a user/password combination to make use of a reliable webservice connection");
      return true;
    }

    if(p_message->GetSoapAction() == "CreateSequence")
    {
      RM_HandleCreateSequence(address,p_message);
      return true;
    }
    if(p_message->GetSoapAction() == "LastMessage")
    {
      RM_HandleLastMessage(address,p_message);
      return true;
    }
    if(p_message->GetSoapAction() == "TerminateSequence")
    {
      RM_HandleTerminateSequence(address,p_message);
      return true;
    }
    else
    {
      // return SOAP FAULT: Unknown RM message type
      SendSOAPFault(address
                   ,p_message
                  ,"Client"
                  ,"Unknown WS-ReliableMessaging request"
                  ,"Client program"
                  ,CString("Encountered a WS-ReliableMessaging request that is unknown to the server: ") + p_message->GetSoapAction());

      return true;
    }
  }
  // Check that we've gotten a reliable message
  if(p_message->GetReliability() == false)
  {
    // return SOAP FAULT: No RM used
    SendSOAPFault(address
                 ,p_message
                ,"Client"
                ,"Must use WS-ReliableMessaging"
                ,"Settings"
                ,"Encountered a SOAP/XML request without using the WS-ReliableMessaging protocol. Review your settings!");
    return true;
  }
  // Normal message, try to increment server ID
  return RM_HandleMessage(address,p_message);
}

// Normal RM message, try to increment server ID
bool
HTTPSite::RM_HandleMessage(SessionAddress& p_address,SOAPMessage* p_message)
{
  SessionSequence* sequence = FindSequence(p_address);
  if(sequence == NULL)
  {
    // Return SOAP Fault: Already a sequence for this session
    SendSOAPFault(p_address
                 ,p_message
                 ,"Client"
                 ,"No RM sequence found"
                 ,"Client program"
                 ,"No reliable-messaging protocol with 'CreateSequence' found for this connection yet\n"
                  "Server can only respond to WS-ReliableMessaging SOAP protocol. Review your program logic.");
    return true;
  }
  // Check message
  // 1: Correct client GUID
  CString clientGUID = p_message->GetClientSequence();
  if(clientGUID.CompareNoCase(sequence->m_serverGUID))
  {
    // SOAP FAULT
    SendSOAPFault(p_address
                 ,p_message
                 ,"Client"
                 ,"Wrong RM sequence found"
                 ,"Client program"
                 ,"Client send wrong server sequence nonce in ReliableMessaging protocol. Review your program logic");
    return true;
  }
  // 2: Correct server GUID
  CString serverGUID = p_message->GetServerSequence();
  if(!serverGUID.IsEmpty() && serverGUID.CompareNoCase(sequence->m_clientGUID))
  {
    // SOAP FAULT
    SendSOAPFault(p_address
                 ,p_message
                 ,"Client"
                 ,"Wrong RM sequence found"
                 ,"Client program"
                 ,"Client send wrong client sequence nonce in ReliableMessaging protocol. Review your program logic");
    return true;
  }
  // 3: Check client ID is 1 (one) higher than last
  if((sequence->m_clientMessageID + 1) == p_message->GetClientMessageNumber())
  {
    sequence->m_clientMessageID++;
  }
  else
  {
    // SOAP FAULT : Handle reliable type 
  }

  // CHECKS OUT OK, PROCEED TO NEXT MESSAGE
  // Server saw one more message
  sequence->m_serverMessageID++;

  // Record in message
  p_message->SetClientSequence(sequence->m_serverGUID);
  p_message->SetServerSequence(sequence->m_clientGUID);
  p_message->SetClientMessageNumber(sequence->m_serverMessageID);
  p_message->SetServerMessageNumber(sequence->m_clientMessageID);
  // DO NOT FILL IN THE NONCE, WE ARE RESPONDING ON THIS ID!!
  // p_message->SetMessageNonce(m_messageGuidID);
  p_message->SetAddressing(true);

  // Server yet to handle real message content
  return false;
}

bool
HTTPSite::RM_HandleCreateSequence(SessionAddress& p_address,SOAPMessage* p_message)
{
  SessionSequence* sequence = FindSequence(p_address);
  if(sequence != NULL)
  {
    // Return SOAP Fault: Already a sequence for this session
    SendSOAPFault(p_address
                 ,p_message
                 ,"Client"
                 ,"Already a RM sequence"
                 ,"Client program"
                 ,"Program requested a new RM-sequence, but a sequence for this session already exists. Review your program logic.");
    return true;
  }
  // Client offers a nonce
  CString guidSequenceClient;
  XMLElement* xmlOffer = p_message->FindElement("Offer");
  XMLElement* xmlIdent = p_message->FindElement(xmlOffer,"Identifier");
  if(xmlIdent)
  {
    guidSequenceClient = xmlIdent->GetValue();
  }

  // Check if we got something
  if(guidSequenceClient.IsEmpty())
  {
    // SOAP Fault: no client sequence nonce offered
    SendSOAPFault(p_address
                 ,p_message
                 ,"Client"
                 ,"No ReliableMessage nonce"
                 ,"Client program"
                 ,"Program requested a new RM-sequence, but did not offer a client nonce (GUID). Review your program logic.");
    return true;
  }

  // React to 'AcksTo' "/anonymous" or some other user 

  // Create the sequence and record offered nonce
  sequence = CreateSequence(p_address);
  sequence->m_clientGUID = guidSequenceClient;

  // Make CreateSequenceResponse
  p_message->Reset();

  // Message body 
  p_message->SetParameter("Identifier",sequence->m_serverGUID);
  XMLElement* accept = p_message->SetParameter("Accept","");
  p_message->SetElement(accept,"Address",p_message->GetURL());

  ReliableResponse(sequence,p_message);
  return true;
}

bool
HTTPSite::RM_HandleLastMessage(SessionAddress& p_address,SOAPMessage* p_message)
{
  SessionSequence* sequence = FindSequence(p_address);
  if(sequence == NULL)
  {
    // Return SOAP Fault: no sequence for this session
    SendSOAPFault(p_address
                 ,p_message
                 ,"Client"
                 ,"No RM sequence"
                 ,"Client program"
                 ,"Program flagged a last-message in a RM-sequence, but the sequence doesn't exist. Review your program logic.");
    return true;
  }
  if(sequence->m_lastMessage)
  {
    // SOAP FAULT: already last message
    SendSOAPFault(p_address
                 ,p_message
                 ,"Client"
                 ,"LastMessage already passed"
                 ,"Client program"
                 ,"Program has sent the 'LastMessage' more than once. Review your program logic.");
    return true;
  }
  // Record the fact that we saw the last message
  sequence->m_lastMessage = true;

  RM_HandleMessage(p_address,p_message);
  p_message->Reset();

  ReliableResponse(sequence,p_message);
  return true;
}

bool
HTTPSite::RM_HandleTerminateSequence(SessionAddress& p_address,SOAPMessage* p_message)
{
  SessionSequence* sequence = FindSequence(p_address);
  if(sequence == NULL)
  {
    // Return SOAP Fault: no sequence for this session
    SendSOAPFault(p_address
                 ,p_message
                ,"Client"
                ,"No RM sequence"
                ,"Client program"
                ,"Program flagged a 'TerminateSequence' in a RM-sequence, but the sequence doesn't exist. Review your program logic.");
    return true;

  }
  if(sequence->m_lastMessage == false)
  {
    // SOAP FAULT: Missing last message
    SendSOAPFault(p_address
                 ,p_message
                ,"Client"
                ,"No LastMessage before TerminateSequence"
                ,"Client program"
                ,"Encountered a 'TerminateSequence' of the RM protocol, but no 'LastMessage' has passed.");
    return true;
  }

  // Check Sequence to be ended
  CString serverGUID = p_message->GetParameter("Identifier");
  if(serverGUID.CompareNoCase(sequence->m_serverGUID))
  {
    // SOAP FAULT: Missing last message
    SendSOAPFault(p_address
                 ,p_message
                 ,"Client"
                 ,"TerminateSequence for wrong sequence"
                 ,"Client program"
                 ,"Encountered a 'TerminateSequence' of the RM protocol, but for a different client. Review your settings.");
    return true;
  }

  // Tell our client that we will end it's nonce
  p_message->Reset();
  p_message->SetParameter("Identifier",sequence->m_clientGUID);

  // Return last response
  RM_HandleMessage(p_address,p_message);
  ReliableResponse(sequence,p_message);

  // Remove the sequence legally
  RemoveSequence(p_address);
  return true;
}

void
HTTPSite::DebugPrintSessionAddress(CString p_prefix,SessionAddress& p_address)
{
  CString address;
  for(unsigned ind = 0;ind < sizeof(SOCKADDR_IN6); ++ind)
  {
    BYTE byte = ((BYTE*)&p_address.m_address)[ind];
    address.AppendFormat("%2.2X",byte);
  }
  
  DETAILLOGV("DEBUG ADDRESS AT   : %s",p_prefix);
  DETAILLOGV("Session address    : %s",address);
  DETAILLOGV("Session address SID: %s",p_address.m_userSID);
  DETAILLOGV("Session desktop    : %d",p_address.m_desktop);
  DETAILLOGV("Session abs. path  : %s",p_address.m_absPath);
}

SessionSequence*
HTTPSite::FindSequence(SessionAddress& p_address)
{
  // Lock for the m_sequences map
  AutoCritSec lock(&m_sessionLock);

// #ifdef _DEBUG
//   DebugPrintSessionAddress("Find sequence",p_address);
// #endif

  ReliableMap::iterator it = m_sequences.find(p_address);
  if(it != m_sequences.end())
  {
    return &(it->second);
  }
  return NULL;
}

SessionSequence*
HTTPSite::CreateSequence(SessionAddress& p_address)
{
  // Lock for the m_sequences map
  AutoCritSec lock(&m_sessionLock);

// #ifdef _DEBUG
//   DebugPrintSessionAddress("CreateSequence",p_address);
// #endif

  SessionSequence sequence;
  sequence.m_serverGUID      = "urn:uuid:" + GenerateGUID();
  sequence.m_clientMessageID = 1;
  sequence.m_serverMessageID = 0;
  sequence.m_lastMessage     = false;

  m_sequences.insert(std::make_pair(p_address,sequence));
  ReliableMap::iterator it = m_sequences.find(p_address);
  if(it != m_sequences.end())
  {
    return &(it->second); 
  }
  return NULL;
}

void
HTTPSite::RemoveSequence(SessionAddress& p_address)
{
  // Lock for the m_sequences map
  AutoCritSec lock(&m_sessionLock);

// #ifdef _DEBUG
//   DebugPrintSessionAddress("RemoveSequence",p_address);
// #endif

  ReliableMap::iterator it = m_sequences.find(p_address);
  if(it != m_sequences.end())
  {
    m_sequences.erase(it);
  }
}

// Send a soap fault as result of a RM-message
void
HTTPSite::SendSOAPFault(SessionAddress& p_address
                       ,SOAPMessage*    p_message
                       ,CString         p_code 
                       ,CString         p_actor
                       ,CString         p_string
                       ,CString         p_detail)
{
  // Destroy the session.
  // Clients must start new RM session after a fault has been received
  RemoveSequence(p_address);

  // Reset the message and set the fault members
  p_message->Reset();
  p_message->SetFault(p_code,p_actor,p_string,p_detail);

  // Send as an SOAP Message
  m_server->SendResponse(p_message);
}

// Return ReliableMessaging response to the client
void
HTTPSite::ReliableResponse(SessionSequence* p_sequence,SOAPMessage* p_message)
{
  p_message->SetReliability(m_reliable,false);
  // REVERSE SEQUENCES AND ID'S, SO CLIENT WILL REACT CORRECTLY
  p_message->SetClientSequence(p_sequence->m_serverGUID);
  p_message->SetServerSequence(p_sequence->m_clientGUID);
  p_message->SetClientMessageNumber(p_sequence->m_serverMessageID);
  p_message->SetServerMessageNumber(p_sequence->m_clientMessageID);
  // DO NOT FILL IN THE NONCE, WE ARE RESPONDING ON THIS ID!!
  // p_message->SetMessageNonce(m_messageGuidID);
  p_message->SetAddressing(true);

  // Adding Encryption 
  HTTPSite* site = p_message->GetHTTPSite();
  if(site && site->GetEncryptionLevel() != XMLEncryption::XENC_Plain)
  {
    p_message->SetSecurityLevel   (site->GetEncryptionLevel());
    p_message->SetSecurityPassword(site->GetEncryptionPassword());
  }

  // Send as a SOAP response, HTTP_STATUS = OK
  m_server->SendResponse(p_message);
}

// Get user SID from an internal SID
CString
HTTPSite::GetStringSID(HANDLE p_token)
{
  CString stringSID;
  LPTSTR  stringSIDpointer = NULL;
  DWORD   dwSize = 0;
  BYTE*   tokenUser = NULL;

  if(!GetTokenInformation(p_token
                         ,TokenUser
                         ,&tokenUser
                         ,0
                         ,&dwSize))
  {
    if(dwSize)
    {
      tokenUser = new BYTE[dwSize];
      if(GetTokenInformation(p_token,TokenUser,tokenUser,dwSize,&dwSize))
      {
        if(ConvertSidToStringSid(((PTOKEN_USER)tokenUser)->User.Sid,&stringSIDpointer))
        {
          stringSID = stringSIDpointer;
          LocalFree(stringSIDpointer);
        }
      }
      delete [] tokenUser;
    }
  }
  return stringSID;
}

//////////////////////////////////////////////////////////////////////////
//
// WS-Security 2004 protocol
//
//////////////////////////////////////////////////////////////////////////

// Check WS-Security protocol
bool
HTTPSite::HttpSecurityCheck(HTTPMessage* p_http,SOAPMessage* p_soap)
{
  // Get the address of the sender (USER/IP/Desktop combination)
  SessionAddress address;
  address.m_userSID = GetStringSID(p_soap->GetAccessToken());
  address.m_desktop = p_soap->GetRemoteDesktop();
  memcpy(&address.m_address,p_soap->GetSender(),sizeof(SOCKADDR));

  if(m_securityLevel != p_soap->GetSecurityLevel())
  {
    SendSOAPFault(address
                 ,p_soap
                 ,"Client"
                 ,"Configuration"
                 ,"Same security level"
                 ,"Client and server should have the same security level (Signing, body-encryption or message-encryption).");
    return true;
  }

  switch(m_securityLevel)
  {
    case XMLEncryption::XENC_Signing: return CheckBodySigning   (address,p_soap);
    case XMLEncryption::XENC_Body:    return CheckBodyEncryption(address,p_soap,p_http->GetBody());
    case XMLEncryption::XENC_Message: return CheckMesgEncryption(address,p_soap,p_http->GetBody());
    case XMLEncryption::XENC_Plain:   // Fall through
    default:                          break;
  }
  // Noting done
  return false;
}

// Check for correct body signing
bool
HTTPSite::CheckBodySigning(SessionAddress& p_address
                          ,SOAPMessage*    p_message)
{
  bool ready = true;

  // Finding the Signature value
  XMLElement* sigValue = p_message->FindElement("SignatureValue");
  if(sigValue)
  {
    CString signature = sigValue->GetValue();
    if(!signature.IsEmpty())
    {
      // Finding the signing method
      CString method = "sha1"; // Default method
      XMLElement* digMethod = p_message->FindElement("DigestMethod");
      if(digMethod)
      {
        CString usedMethod = p_message->GetAttribute(digMethod,"Algorithm");
        if(!usedMethod.IsEmpty())
        {
          method = usedMethod;
          // Could be: http://www.w3.org/2000/09/xmldsig#rsa-sha1
          int pos = usedMethod.Find('#');
          if(pos > 0)
          {
            method = usedMethod.Mid(pos + 1);
          }
        }
      }

      // Finding the reference ID
      CString signedXML;
      XMLElement* refer = p_message->FindElement("Reference");
      if(refer)
      {
        CString uri = p_message->GetAttribute(refer,"URI");
        if(!uri.IsEmpty())
        {
          uri = uri.TrimLeft("#");
          XMLElement* uriPart = p_message->FindElementByAttribute("Id",uri);
          if(uriPart)
          {
            signedXML = p_message->GetCanonicalForm(uriPart);
          }
        }
      }

      // Fallback on the body part as a signed message part
      if(signedXML.IsEmpty())
      {
        signedXML = p_message->GetBodyPart();
      }

      Crypto sign;
      sign.SetHashMethod(method);
      p_message->SetSigningMethod(sign.GetHashMethod());
      CString digest = sign.Digest(signedXML,m_enc_password);

      if(signature.CompareNoCase(digest) == 0)
      {
        // Not yet ready with this message
        ready = false;
      }
    }
  }
  if(ready)
  {
    SendSOAPFault(p_address
                 ,p_message
                 ,"Client"
                 ,"Configuration"
                 ,"No signing"
                 ,"SOAP message should have a signed body. Singing is incorrect or missing.");
  }
  // ALL OK?
  return ready;
}

bool
HTTPSite::CheckBodyEncryption(SessionAddress& p_address
                             ,SOAPMessage*    p_soap
                             ,CString         p_body)
{
  bool ready = true;
  CString crypt = p_soap->GetSecurityPassword();
  // Restore password for return answer
  p_soap->SetSecurityPassword(m_enc_password);

  // Decrypt
  Crypto crypting;
  CString newBody = crypting.Decryptie(crypt,m_enc_password);

  int beginPos = p_body.Find("Body>");
  int endPos   = p_body.Find("Body>",beginPos + 5);
  if(beginPos > 0 && endPos > 0 && endPos > beginPos)
  {
    // Finding begin of the body before the namespace
    for(int ind = beginPos;ind >= 0; --ind)
    {
      if(p_body.GetAt(ind) == '<') 
      {
        beginPos = ind;
        break;
      }
    }
    // Finding begin of the end-of-body before the namespace
    int extra = 6;
    for(int ind = endPos; ind >= 0; --ind)
    {
      if(p_body.GetAt(ind) == '<')
      {
        endPos = ind;
        break;
      }
      ++extra;
    }

    // Reparse from here and set to NOT-ENCRYPTED
    CString message = p_body.Left(beginPos) + newBody + p_body.Mid(endPos + extra);
    p_soap->Reset();
    p_soap->ParseMessage(message);
    p_soap->SetSecurityLevel(XMLEncryption::XENC_Plain);

    // Error check
    if(p_soap->GetInternalError() == XmlError::XE_NoError     ||
       p_soap->GetInternalError() == XmlError::XE_EmptyCommand )
    {
      // OK, Message found
      ready = false;
    }
  }

  // Any errors found?
  if(ready)
  {
    SendSOAPFault(p_address
                 ,p_soap
                 ,"Client"
                 ,"Configuration"
                 ,"No encryption"
                 ,"SOAP message should have a encrypted body. Encryption is incorrect or missing.");
  }
  return ready;
}

bool
HTTPSite::CheckMesgEncryption(SessionAddress& p_address
                             ,SOAPMessage*    p_soap
                             ,CString         p_body)
{
  bool ready = true;
  CString crypt = p_soap->GetSecurityPassword();
  // Restore password for return answer
  p_soap->SetSecurityPassword(m_enc_password);

  // Decrypt
  Crypto crypting;
  CString newBody = crypting.Decryptie(crypt,m_enc_password);

  int beginPos = p_body.Find("Envelope>");
  int endPos   = p_body.Find("Envelope>",beginPos + 2);
  if(beginPos > 0 && endPos > 0 && endPos > beginPos)
  {
    // Finding begin of the envelope before the namespace
    for (int ind = beginPos; ind >= 0; --ind)
    {
      if (p_body.GetAt(ind) == '<')
      {
        beginPos = ind;
        break;
      }
    }
    // Finding begin of the end-of-envelope before the namespace
    int extra = 10;
    for (int ind = endPos; ind >= 0; --ind)
    {
      if (p_body.GetAt(ind) == '<')
      {
        endPos = ind;
        break;
      }
      ++extra;
    }

    // Reparsing the message
    CString message = p_body.Left(beginPos) + newBody + p_body.Mid(endPos + extra);
    p_soap->Reset();
    p_soap->ParseMessage(message);
    p_soap->SetSecurityLevel(XMLEncryption::XENC_Plain);

    if(p_soap->GetInternalError() == XmlError::XE_NoError     ||
       p_soap->GetInternalError() == XmlError::XE_EmptyCommand )
    {
      // OK, Message found
      ready = false;
    }
  }

  // Any errors found?
  if(ready)
  {
    SendSOAPFault(p_address
                 ,p_soap
                 ,"Client"
                 ,"WS-Security"
                 ,"No encryption"
                 ,"SOAP message should have a encrypted message. Encryption is incorrect or missing.");
  }
  return ready;
}

//////////////////////////////////////////////////////////////////////////
//
// Standard headers added by call responses from this site
//
//////////////////////////////////////////////////////////////////////////

void
HTTPSite::SetXFrameOptions(XFrameOption p_option,CString p_uri)
{
  m_xFrameOption = p_option;
  if(m_xFrameOption == XFrameOption::XFO_ALLOWFROM)
  {
    m_xFrameAllowed = p_uri;
  }
  else
  {
    m_xFrameAllowed.Empty();
  }
}

// HSTS options
void
HTTPSite::SetStrictTransportSecurity(unsigned p_maxAge,bool p_subDomains)
{
  m_hstsMaxAge = p_maxAge;
  m_hstsSubDomains = p_subDomains;
}

// No sniffing of my context type (ASCII/UTF-8 etc)
void
HTTPSite::SetXContentTypeOptions(bool p_nosniff)
{
  m_xNoSniff = p_nosniff;
}

void
HTTPSite::SetXSSProtection(bool p_on,bool p_block)
{
  m_xXSSProtection = p_on;
  m_xXSSBlockMode = p_block;
}

void
HTTPSite::SetBlockCacheControl(bool p_block)
{
  m_blockCache = p_block;
}

// Add all necessary extra headers to the response
void
HTTPSite::AddSiteOptionalHeaders(UKHeaders& p_headers)
{
  CString value;

  // Add X-Frame-Options
  if(m_xFrameOption != XFrameOption::XFO_NO_OPTION)
  {
    switch(m_xFrameOption)
    {
      case XFrameOption::XFO_DENY:        value = "DENY";        break;
      case XFrameOption::XFO_SAMEORIGIN:  value = "SAMEORIGIN";  break;
      case XFrameOption::XFO_ALLOWFROM:   value = "ALLOW-FROM ";
        value += m_xFrameAllowed;
      default:                            break;
    }
    p_headers.insert(std::make_pair("X-Frame-Options",value));
  }
  // Add HSTS headers
  if(m_hstsMaxAge > 0)
  {
    value.Format("max-age=%u",m_hstsMaxAge);
    if(m_hstsSubDomains)
    {
      value += "; includeSubDomains";
    }
    p_headers.insert(std::make_pair("Strict-Transport-Security",value));
  }
  // Browsers should take our content-type for granted!!
  if(m_xNoSniff)
  {
    p_headers.insert(std::make_pair("X-Content-Type-Options","nosniff"));
  }
  // Add protection against XSS 
  if(m_xXSSProtection)
  {
    value = "1";
    if(m_xXSSBlockMode)
    {
      value += "; mode=block";
    }
    p_headers.insert(std::make_pair("X-XSS-Protection",value));
  }
  // Blocking the browser cache for this site!
  // Use for responsive applications only!
  if(m_blockCache)
  {
    p_headers.insert(std::make_pair("Cache-Control","no-store, no-cache, must-revalidate, max-age=0, post-check=0, pre-check=0"));
    p_headers.insert(std::make_pair("Pragma","no-cache"));
    p_headers.insert(std::make_pair("Expires","0"));
  }

  // If we use CORS, make sure we advertise the origin
  if(m_useCORS)
  {
    p_headers.insert(std::make_pair("Access-Control-Allow-Origin",m_allowOrigin.IsEmpty() ? "*" : m_allowOrigin));
  }
}
