/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: HTTPSite.h
//
// Marlin Server: Internet server/client
// 
// Copyright (c) 2014-2022 ir. W.E. Huisman
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
#include "SoapTypes.h"
#include "HTTPServer.h"
#include "HTTPMessage.h"
#include "SOAPMessage.h"
#include "JSONMessage.h"
#include "ThreadPool.h"
#include "SiteFilter.h"
#include "SiteHandler.h"
#include "Cookie.h"
#include <map>

// Session address for reliable messaging
class SessionAddress
{
public:
  CString       m_userSID; // User in the SIDL form
  ULONG         m_address; // sin6_flowinfo of IP address of caller
  UINT          m_desktop; // TerminalServices desktop
  CString       m_absPath; // Absolute path of session
};

// Reliable messaging server sequence
class SessionSequence
{
public:
  CString m_clientGUID;       // Client challenging nonce
  CString m_serverGUID;       // Server challenging nonce
  int     m_clientMessageID;  // Clients message number
  int     m_serverMessageID;  // Servers message number
  bool    m_lastMessage;      // Last message in stream flag
};

// Operator for associative mapping for the internet addresses
struct AddressCompare
{
  bool operator()(const SessionAddress& p_left, const SessionAddress& p_right) const
  {
    // Compare internet address (fastest compare)
    // Uses from SOCKADDR_IN6 the sin6_family, sin6_port, sin6_flowinfo members
    int cmp = memcmp(&p_left.m_address,&p_right.m_address,sizeof(USHORT) + sizeof(USHORT) + sizeof(ULONG));
    if(cmp < 0) return true;
    if(cmp > 0) return false;
    // Compare desktops
    if(p_left.m_desktop < p_right.m_desktop) return true;
    if(p_left.m_desktop > p_right.m_desktop) return false;
    // Compare absolute paths
    cmp = p_left.m_absPath.Compare(p_right.m_absPath);
    if(cmp < 0) return true;
    if(cmp > 0) return false;
    // Compare users (longest string)
    cmp = p_left.m_userSID.Compare(p_right.m_userSID);
    if(cmp < 0) return true;
    return false;
  }
};

// Options for X-Frame headers
enum class XFrameOption
{
  XFO_NO_OPTION  = 0
 ,XFO_DENY       = 1
 ,XFO_SAMEORIGIN = 2
 ,XFO_ALLOWFROM  = 3
};

// Special callback options for different handlers
void HTTPSiteCallbackMessage(void* p_argument);
void HTTPSiteCallbackEvent  (void* p_argument);

// Default maximum number of HTTP Throttling addresses
constexpr long MAX_HTTP_THROTTLES = 1000;

class HTTPURLGroup;
class MarlinConfig;
class SiteFilter;
class SiteHandler;

// Keeping a mapping of all the site handlers
typedef struct _regHandler
{
  SiteHandler* m_handler;
  bool         m_owner;
}
RegHandler;

// Mapping of all content types processed
//using ContentTypeMap  = std::vector<CString>;
using FilterMap       = std::map<unsigned,SiteFilter*>;
using ReliableMap     = std::map<SessionAddress,SessionSequence,AddressCompare>;
using HandlerMap      = std::map<HTTPCommand,RegHandler>;
using ThrottlingMap   = std::map<SessionAddress,CRITICAL_SECTION*,AddressCompare>;

// Cleanup handler after a crash-report
extern __declspec(thread) SiteHandler* g_cleanup;

class HTTPSite
{
public:
  HTTPSite(HTTPServer*    p_server
          ,int            p_port
          ,CString        p_site
          ,CString        p_prefix
          ,HTTPSite*      p_mainSite = nullptr
          ,LPFN_CALLBACK  p_callback = nullptr);
  virtual ~HTTPSite();

  // MANDATORY: Explicitly starting after configuration of the site
  virtual bool StartSite() = 0;
  // OPTIONAL: Explicitly stopping a site
  bool         StopSite(bool p_force = false);

  // SETTERS

  // OPTIONAL: Set the webroot of the site
  virtual bool    SetWebroot(CString p_webroot) = 0;
  // MANDATORY: Set handler for HTTP command(s)
  void            SetHandler(HTTPCommand p_command,SiteHandler* p_handler,bool p_owner = true);
  // OPTIONAL Set filter for HTTP command(s)
  bool            SetFilter(unsigned p_priority,SiteFilter* p_filter);
  // OPTIONAL: Set server in async-accept mode
  void            SetAsync(bool p_async);
  // OPTIONAL: Site can be a event-stream starter
  void            SetIsEventStream(bool p_stream);
  // OPTIONAL: Set site's callback function
  void            SetCallback(LPFN_CALLBACK p_callback);
  // OPTIONAL: Set one or more text-based content types
  void            AddContentType(CString p_extension,CString p_contentType);
  // OPTIONAL: Set the encryption level (signing/body/message)
  void            SetEncryptionLevel(XMLEncryption p_level);
  // OPTIONAL: Set the encryption password
  void            SetEncryptionPassword(CString p_password);
  // OPTIONAL: Set authentication scheme (Basic, Digest, NTLM, Negotiate, All)
  void            SetAuthenticationScheme(CString p_scheme);
  // OPTIONAL: Set authentication parameters
  void            SetAuthenticationNTLMCache(bool p_cache);
  // OPTIONAL: Set an authentication Realm
  void            SetAuthenticationRealm(CString p_realm);
  // OPTIONAL: Set an authentication Domain
  void            SetAuthenticationDomain(CString p_domain);
  // OPTIONAL: Set the 'retain-all-headers' code
  void            SetAllHeaders(bool p_allHeaders);
  // OPTIONAL: Set WS-Reliability
  void            SetReliable(bool p_reliable);
  // OPTIONAL: Set WS-ReliableMessaging requires logged-in-user
  void            SetReliableLogIn(bool p_logIn);
  // OPTIONAL: Set optional site payload, so you can hide your own context
  void            SetPayload(void* p_payload);
  // OPTIONAL: Set XFrame options on server answer
  void            SetXFrameOptions(XFrameOption p_option,CString p_uri);
  // OPTIONAL: Set Strict Transport Security (HSTS)
  void            SetStrictTransportSecurity(unsigned p_maxAge,bool p_subDomains);
  // OPTIONAL: Set X-Content-Type options
  void            SetXContentTypeOptions(bool p_nosniff);
  // OPTIONAL: Set protection against X-Site scripting
  void            SetXSSProtection(bool p_on,bool p_block);
  // OPTIONAL: Set cache control
  void            SetBlockCacheControl(bool p_block);
  // OPTIONAL: Set send in UTF-16 Unicode
  void            SetSendUnicode(bool p_unicode);
  // OPTIONAL: Set prepend SOAP message with BOM
  void            SetSendSoapBOM(bool p_bom);
  // OPTIONAL: Set prepend JSON message with BOM
  void            SetSendJsonBOM(bool p_bom);
  // OPTIONAL: Set HTTP VERB tunneling 
  void            SetVerbTunneling(bool p_tunnel);
  // OPTIONAL: Set HTTP gzip compression
  void            SetHTTPCompression(bool p_compression);
  // OPTIONAL: Set HTTP throttling per address
  void            SetHTTPThrotteling(bool p_throttel);
  // OPTIONAL: Set use CORS (Cross Origin Resource Sharing)
  void            SetUseCORS(bool p_use);
  // OPTIONAL: Set use this origin for CORS (otherwise all = '*')
  void            SetCORSOrigin(CString p_origin);
  // OPTIONAL: Set use CORS max age promise
  void            SetCORSMaxAge(unsigned p_maxAge);
  // OPTIONAL: Set all cookies HttpOnly
  void            SetCookiesHttpOnly(bool p_only);
  // OPTIONAL: Set all cookies Secure
  void            SetCookiesSecure(bool p_secure);
  // OPTIONAL: Set all cookies same-site attribute
  void            SetCookiesSameSite(CookieSameSite p_same);

  // GETTERS
  CString         GetSite()                         { return m_site;          };
  int             GetPort()                         { return m_port;          };
  bool            GetIsStarted()                    { return m_isStarted;     };
  bool            GetIsSubsite()                    { return m_isSubsite;     };
  bool            GetAsync()                        { return m_async;         };
  CString         GetPrefixURL()                    { return m_prefixURL;     };
  bool            GetIsEventStream()                { return m_isEventStream; };
  LPFN_CALLBACK   GetCallback()                     { return m_callback;      };
  MediaTypeMap&   GetContentTypeMap()               { return m_contentTypes;  };
  XMLEncryption   GetEncryptionLevel()              { return m_securityLevel; };
  CString         GetEncryptionPassword()           { return m_enc_password;  };
  bool            GetAllHeaders()                   { return m_allHeaders;    };
  bool            GetReliable()                     { return m_reliable;      };
  bool            GetReliableLogIn()                { return m_reliableLogIn; };
  void*           GetPayload()                      { return m_payload;       };
  HTTPServer*     GetHTTPServer()                   { return m_server;        };
  HTTPSite*       GetMainSite()                     { return m_mainSite;      };
  bool            GetSendUnicode()                  { return m_sendUnicode;   };
  bool            GetSendSoapBOM()                  { return m_sendSoapBOM;   };
  bool            GetSendJsonBOM()                  { return m_sendJsonBOM;   };
  bool            GetVerbTunneling()                { return m_verbTunneling; };
  bool            GetHTTPCompression()              { return m_compression;   };
  bool            GetHTTPThrotteling()              { return m_throttling;    };
  bool            GetUseCORS()                      { return m_useCORS;       };
  CString         GetCORSOrigin()                   { return m_allowOrigin;   };
  CString         GetCORSHeaders()                  { return m_allowHeaders;  };
  int             GetCORSMaxAge()                   { return m_corsMaxAge;    };
  bool            GetCORSAllowCredentials()         { return m_corsCredentials;  }
  bool            GetCookieHasSecure()              { return m_cookieHasSecure;  }
  bool            GetCookieHasHttpOnly()            { return m_cookieHasHttp;    }
  bool            GetCookieHasSameSite()            { return m_cookieHasSame;    }
  CookieSameSite  GetCookiesSameSite()              { return m_cookieSameSite;   }
  bool            GetCookiesSecure()                { return m_cookieSecure;     }
  bool            GetCookiesHttpOnly()              { return m_cookieHttpOnly;   }
  CString         GetAuthenticationScheme();
  bool            GetAuthenticationNTLMCache();
  CString         GetAuthenticationRealm();
  CString         GetAuthenticationDomain();
  CString         GetAllowHandlers();
  CString         GetWebroot();
  SiteHandler*    GetSiteHandler(HTTPCommand p_command);
  SiteFilter*     GetFilter(unsigned p_priority);
  CString         GetContentType(CString p_extension);
  CString         GetContentTypeByResourceName(CString p_pathname);
  virtual bool    GetHasAnonymousAuthentication(HANDLE p_token);

  // FUNCTIONS

  // Remove the site from the URL group
  bool RemoveSiteFromGroup();
  // Removing a filter with a certain priority
  bool RemoveFilter(unsigned p_priority);
  // Call the correct HTTP handler!
  void HandleHTTPMessage(HTTPMessage* p_message);
  // Call the correct EventStream handler
  void HandleEventStream(HTTPMessage* p_message,EventStream* p_stream);
  // Check WS-ReliableMessaging protocol
  bool HttpReliableCheck(SOAPMessage* p_message);
  // Check WS-Security protocol
  bool HttpSecurityCheck(HTTPMessage* p_http,SOAPMessage* p_soap);
  // Setting cleanup handler
  void SetCleanup(SiteHandler* p_cleanup);
  // Soap fault in response on error in SOAP/XML/RM protocol
  void SendSOAPFault(SessionAddress&  p_address
                    ,SOAPMessage*     p_message
                    ,CString          p_code
                    ,CString          p_actor
                    ,CString          p_string
                    ,CString          p_detail);
  // Add all optional extra headers of this site
  void AddSiteOptionalHeaders(UKHeaders& p_headers);
  // Send responses
  bool SendResponse(HTTPMessage* p_message);
  bool SendResponse(SOAPMessage* p_message);
  bool SendResponse(JSONMessage* p_message);

protected:
  // Init parameters from Marlin.config
  void              InitSite(MarlinConfig& p_config);
  // Set automatic headers upon starting site
  void              SetAutomaticHeaders(MarlinConfig& p_config);
  // Log all settings to the site
  void              LogSettings();
  // Cleanup the site when stopping
  void              CleanupHandlers();
  void              CleanupFilters();
  void              CleanupThrotteling();
  // Finding the SiteHandler registration
  RegHandler*       FindSiteHandler(HTTPCommand p_command);
  // Calling all filters
  bool              CallFilters(HTTPMessage* p_message);
  // All registered site handlers, and the default action
  void              HandleHTTPMessageDefault(HTTPMessage* p_message);
  // Direct asynchronous response
  void              AsyncResponse(HTTPMessage* p_message);
  // Handle the error after an error report
  void              PostHandle(HTTPMessage* p_message,bool p_reset = true);
    // Check that m_reliable and m_async do not mix
  bool              CheckReliable();
  // Convert authentication token to SID string.
  CString           GetStringSID(HANDLE p_token);
  // Check for correct body signing
  bool              CheckBodySigning   (SessionAddress& p_address,SOAPMessage* p_soap);
  bool              CheckBodyEncryption(SessionAddress& p_address,SOAPMessage* p_soap,CString p_body);
  bool              CheckMesgEncryption(SessionAddress& p_address,SOAPMessage* p_soap,CString p_body);
  // Handling the Reliable-Messaging protocol
  bool              RM_HandleCreateSequence   (SessionAddress&  p_address, SOAPMessage* p_message);
  bool              RM_HandleLastMessage      (SessionAddress&  p_address, SOAPMessage* p_message);
  bool              RM_HandleTerminateSequence(SessionAddress&  p_address, SOAPMessage* p_message);
  bool              RM_HandleMessage          (SessionAddress&  p_address, SOAPMessage* p_message);
  void              ReliableResponse          (SessionSequence* p_sequence,SOAPMessage* p_message);
  // Handle all sequences
  void              RemoveSequence(SessionAddress& p_address);
  SessionSequence*  FindSequence  (SessionAddress& p_address);
  SessionSequence*  CreateSequence(SessionAddress& p_address);
  void              DebugPrintSessionAddress(CString p_prefix,SessionAddress& p_address);

  // Handle HTTP throttling
  CRITICAL_SECTION* StartThrottling(HTTPMessage* p_message);
  void              EndThrottling(CRITICAL_SECTION*& p_throttle);
  void              TryFlushThrottling();

  // Unique site
  CString           m_site;                               // Absolute path of the URL
  int               m_port            { INTERNET_DEFAULT_HTTP_PORT };  // HTTP Port of this site
  // Status registration of the site
  bool              m_isStarted       { false   };        // Site is correctly started
  bool              m_isSubsite       { false   };        // Site is a sub-site of another site
  HTTPServer*       m_server          { nullptr };        // Site of this server
  HTTPURLGroup*     m_group           { nullptr };        // Site is in this group
  HTTPSite*         m_mainSite        { nullptr };        // Main site of this site
  CString           m_webroot;                            // Webroot of this site
  bool              m_virtualDirectory{ false   };        // Webroot is a virtual directory outside the server webroot
  CString           m_prefixURL;                          // Channel prefix
  bool              m_isEventStream   { false   };        // Server-push-event stream
  void*             m_payload         { nullptr };        // Your own context payload
  LPFN_CALLBACK     m_callback        { nullptr };        // Context for the threadpool
  bool              m_async           { false   };        // Site in async-accept mode
  MediaTypeMap      m_contentTypes;                       // Text based content type
  XMLEncryption     m_securityLevel   { XMLEncryption::XENC_Plain };  // Security level
  CString           m_enc_password;                       // Security encryption password
  bool              m_allHeaders      { false   };        // Retain all headers in the HTTPMessage
  bool              m_sendUnicode     { false   };        // Send UTF-16 Unicode answers
  bool              m_sendSoapBOM     { false   };        // Prepend UTF-16 SOAP message with BOM 
  bool              m_sendJsonBOM     { false   };        // Prepend UTF-16 JSON message with BOM
  bool              m_compression     { false   };        // Allows for HTTP gzip compression
  bool              m_throttling      { false   };        // Perform throttling per address
  // CORS Cross Origin Resource Sharing
  bool              m_useCORS         { false   };        // Use CORS header methods
  CString           m_allowOrigin;                        // Client that can call us or '*' for everyone
  CString           m_allowHeaders;                       // White-listing of exposed headers
  unsigned          m_corsMaxAge      { 86400   };        // Pre-flight results are valid this long (1 day in seconds)
  bool              m_corsCredentials { false   };        // CORS allows credentials to be sent
                                                          // CORS methods comes from the m_handlers map !!!
  // Authentication
  ULONG             m_authScheme      { 0       };        // Authentication scheme's
  CString           m_scheme;                             // Authentication scheme names
  bool              m_ntlmCache       { true    };        // Authentication NTLM Cache 
  CString           m_realm;                              // Authentication realm
  CString           m_domain;                             // Authentication domain
  // WS-ReliableMessaging
  bool              m_reliable        { false   };        // Does WS-Reliable messaging in SOAP POST's
  bool              m_reliableLogIn   { false   };        // RM implies logged-in user
  ReliableMap       m_sequences;                          // Reliable messaging sequences
  // HTTP Site handlers and filters
  HandlerMap        m_handlers;                           // Site handlers
  FilterMap         m_filters;                            // Site filters
  // Multi-threading
  CRITICAL_SECTION  m_filterLock;                         // Adding/deleting/calling filters
  CRITICAL_SECTION  m_sessionLock;                        // Adding/deleting sessions sequences
  ThrottlingMap     m_throttels;                          // Addresses to be throttled
  // Cookie settings enforcement
  bool              m_cookieHasSecure { false };          // Site override voor 'secure'   cookies
  bool              m_cookieHasHttp   { false };          // Site override voor 'httpOnly' cookies
  bool              m_cookieHasSame   { false };          // Site override voor 'SameSite' cookies
  bool              m_cookieSecure    { false };          // All cookies have the 'secure'   attribute
  bool              m_cookieHttpOnly  { false };          // All cookies have the 'httpOnly' attribute
  CookieSameSite    m_cookieSameSite  { CookieSameSite::NoSameSite }; // Same site setting of cookies
  // Auto HTTP headers added to all response traffic
  XFrameOption      m_xFrameOption    { XFrameOption::XFO_NO_OPTION };  // Standard frame options
  CString           m_xFrameAllowed;                      // IFrame allowed from this URI
  unsigned          m_hstsMaxAge      { 0       };        // Max age from HSTS
  bool              m_hstsSubDomains  { false   };        // Subdomains allowed on HSTS
  bool              m_xNoSniff        { false   };        // Block sniffing on ASCII content type
  bool              m_xXSSProtection  { false   };        // XSS Protection is on or off
  bool              m_xXSSBlockMode   { false   };        // No mode, or block mode
  bool              m_blockCache      { false   };        // Blocking the cache control
  bool              m_verbTunneling   { false   };        // Verb tunneling allowed
};

// SETTERS

inline void
HTTPSite::SetAsync(bool p_async)
{
  m_async = p_async;
}

inline void
HTTPSite::SetReliable(bool p_reliable)
{
  m_reliable = p_reliable;
}

inline void
HTTPSite::SetEncryptionLevel(XMLEncryption p_level)
{
  m_securityLevel = p_level;
}

inline void
HTTPSite::SetEncryptionPassword(CString p_password)
{
  m_enc_password = p_password;
}

inline void
HTTPSite::SetAuthenticationScheme(CString p_scheme)
{
  m_scheme = p_scheme;
}

inline void
HTTPSite::SetAuthenticationNTLMCache(bool p_cache)
{
  m_ntlmCache = p_cache;
}

inline void
HTTPSite::SetAuthenticationRealm(CString p_realm)
{
  m_realm = p_realm;
}

inline void
HTTPSite::SetAuthenticationDomain(CString p_domain)
{
  m_domain = p_domain;
}

inline void
HTTPSite::SetAllHeaders(bool p_allHeaders)
{
  m_allHeaders = p_allHeaders;
}

inline void
HTTPSite::SetReliableLogIn(bool p_logIn)
{
  m_reliableLogIn = p_logIn;
}

inline void
HTTPSite::SetPayload(void* p_payload)
{
  m_payload = p_payload;
}

inline void 
HTTPSite::SetCleanup(SiteHandler* p_cleanup)
{
  g_cleanup = p_cleanup;
}

inline void
HTTPSite::SetSendUnicode(bool p_unicode)
{
  m_sendUnicode = p_unicode;
}

inline void
HTTPSite::SetSendSoapBOM(bool p_bom)
{
  m_sendSoapBOM = p_bom;
}

inline void
HTTPSite::SetSendJsonBOM(bool p_bom)
{
  m_sendJsonBOM = p_bom;
}

inline void
HTTPSite::SetVerbTunneling(bool p_tunnel)
{
  m_verbTunneling = p_tunnel;
}

inline void
HTTPSite::SetHTTPCompression(bool p_compression)
{
  m_compression = p_compression;
}

inline void
HTTPSite::SetHTTPThrotteling(bool p_throttel)
{
  m_throttling = p_throttel;
}

inline void
HTTPSite::SetUseCORS(bool p_use)
{
  m_useCORS = p_use;
}

inline void
HTTPSite::SetCORSOrigin(CString p_origin)
{
  m_allowOrigin = p_origin;
}

inline void
HTTPSite::SetCORSMaxAge(unsigned p_maxAge)
{
  m_corsMaxAge = p_maxAge;
}
