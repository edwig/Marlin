/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: HTTPSiteMarlin.cpp
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
#include "stdafx.h"
#include "AutoCritical.h"
#include "HTTPSiteMarlin.h"
#include "HTTPServerMarlin.h"
#include "HTTPURLGroup.h"
#include "EnsureFile.h"

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

HTTPSiteMarlin::HTTPSiteMarlin(HTTPServerMarlin* p_server
                              ,int               p_port
                              ,CString           p_site
                              ,CString           p_prefix
                              ,HTTPSite*         p_mainSite /*=nullptr*/
                              ,LPFN_CALLBACK     p_callback /*=nullptr*/)
  :HTTPSite(p_server,p_port,p_site,p_prefix,p_mainSite,p_callback)
{
}

bool
HTTPSiteMarlin::StartSite()
{
  bool result = false;
  DETAILLOGS("Starting website. URL: ",m_site);

  // Getting the global settings
  InitSite(m_server->GetWebConfig());

  // If we have a site web.config file: read it
  // Overrides the programmatical settings between HTTPServer::CreateSite and HTTPSite::StartSite
  CString siteConfigFile = WebConfig::GetSiteConfig(m_prefixURL);
  if(!siteConfigFile.IsEmpty())
  {
    WebConfig config(siteConfigFile);
    if(config.IsFilled())
    {
      InitSite(config);
    }
  }

  // Now log the settings, once we read all web.config files
  LogSettings();

  // See if we have a reliable messaging WITH authentication
  CheckReliable();

  // Checking our webroot
  if(!SetWebroot(m_webroot))
  {
    ERRORLOG(ERROR_INVALID_NAME,"Cannot start site: invalid webroot");
    return false;
  }

  // Call all filters 'OnStartSite' methods
  for(auto& filter : m_filters)
  {
    filter.second->OnStartSite();
  }

  // Call all site handlers 'OnStartSite' methods
  for(auto& handler : m_handlers)
  {
    SiteHandler* shand = handler.second.m_handler;
    while(shand)
    {
      shand->OnStartSite();
      shand = shand->GetNextHandler();
    }
  }

  // Lock for the initialization of the site
  AutoCritSec lock(m_server->AcquireSitesLockObject());

  // Find our server type
  HTTPServerMarlin* server = reinterpret_cast<HTTPServerMarlin*>(m_server);

  // Start a group for our site, and record the group
  m_group = server->FindUrlGroup(m_scheme,m_authScheme,m_ntlmCache,m_realm,m_domain);
  if(m_group)
  {
    // Register site in the group
    m_group->RegisterSite(this);

    // The registration
    if(m_mainSite)
    {
      DETAILLOGV("URL not registered to URL-Group. Site [%s] is subsite of [%s]",m_site.GetString(),m_mainSite->GetSite().GetString());
      result = true;
    }
    else
    {
      USES_CONVERSION;
      // Add URL to our URL-Group: using HttpAddUrlToUrlGroup
      wstring uniURL = A2CW(m_prefixURL);
      HTTP_URL_GROUP_ID group = m_group->GetUrlGroupID();
      ULONG retCode = HttpAddUrlToUrlGroup(group,uniURL.c_str(),(HTTP_URL_CONTEXT)this,0);
      if(retCode != NO_ERROR)
      {
        CString error;
        error.Format("Cannot add URL to the URL-Group: %s",m_prefixURL.GetString());
        ERRORLOG(retCode,error);
      }
      else
      {
        DETAILLOGS("URL added to URL-Group: ",m_prefixURL);
        result = true;
      }
    }
  }
  else
  {
    ERRORLOG(ERROR_INVALID_PARAMETER,"No URL group created/found for authentication scheme: " + m_scheme);
  }
  // Return the fact that we started successfully or not
  return (m_isStarted = result);
}

bool
HTTPSiteMarlin::SetWebroot(CString p_webroot)
{
  // Cleaning the webroot is simple
  if(p_webroot.IsEmpty())
  {
    m_webroot = m_server->GetWebroot();
    return true;
  }

  // Check the directory
  CString siteWebroot;
  if(m_virtualDirectory)
  {
    // Check already done by setting virtual directory
    return true;
  }
  else if(p_webroot.Left(10).CompareNoCase("virtual://") == 0)
  {
    // It's a HTTP virtual directory outside of the server's webroot
    siteWebroot = p_webroot.Mid(10);
    m_virtualDirectory = true;
  }
  else
  {
    siteWebroot = p_webroot;

    // Checking the webroot of the site against the webroot of the server
    CString serverWebroot = m_server->GetWebroot();
    serverWebroot.MakeLower();
    siteWebroot  .MakeLower();
    serverWebroot.Replace("/","\\");
    siteWebroot  .Replace("/","\\");

    if(siteWebroot.Find(serverWebroot) != 0)
    {
      CString error;
      ERRORLOG(ERROR_INVALID_PARAMETER,error);
      return false;
    }
  }

  // Make sure the directory is there
  EnsureFile ensure(siteWebroot);
  int error = ensure.CheckCreateDirectory();
  if(error)
  {
    ERRORLOG(error,"Website's root directory does not exist and cannot create it");
  }

  // Acceptable webroot of a site
  m_webroot = siteWebroot;
  return true;
}

void
HTTPSiteMarlin::InitSite(WebConfig& p_config)
{
  // Get the webroot
  m_webroot     = p_config.GetParameterString ("Server","WebRoot",m_webroot);
  // Check Unicode forcing
  m_sendUnicode = p_config.GetParameterBoolean("Server","RespondUnicode",m_sendUnicode);
  m_sendSoapBOM = p_config.GetParameterBoolean("Server","RespondSoapBOM",m_sendSoapBOM);
  m_sendJsonBOM = p_config.GetParameterBoolean("Server","RespondJsonBOM",m_sendJsonBOM);

  // Read XML Signing en encryption from the config
  CString level;
  switch(m_securityLevel)
  {
    case XMLEncryption::XENC_Plain:   level = "";        break;
    case XMLEncryption::XENC_Signing: level = "sign";    break;
    case XMLEncryption::XENC_Body:    level = "body";    break;
    case XMLEncryption::XENC_Message: level = "message"; break;
  }
  // Getting various settings
  level           = p_config.GetParameterString ("Encryption","Level",          level);
  m_enc_password  = p_config.GetEncryptedString ("Encryption","Password",       m_enc_password);
  m_reliable      = p_config.GetParameterBoolean("Server",    "Reliable",       m_reliable);
  m_verbTunneling = p_config.GetParameterBoolean("Server",    "VerbTunneling",  m_verbTunneling);
  m_compression   = p_config.GetParameterBoolean("Server",    "HTTPCompression",m_compression);
  m_throttling    = p_config.GetParameterBoolean("Server",    "HTTPThrotteling",m_throttling);

  // Now set the resulting security level
       if(level == "sign")    m_securityLevel = XMLEncryption::XENC_Signing;
  else if(level == "body")    m_securityLevel = XMLEncryption::XENC_Body;
  else if(level == "message") m_securityLevel = XMLEncryption::XENC_Message;
  else                        m_securityLevel = XMLEncryption::XENC_Plain;

  // AUTHENTICATION
  // Setup the authentication of the URL group
  CString scheme    = p_config.GetParameterString ("Authentication","Scheme",           m_scheme);
  bool    ntlmCache = p_config.GetParameterBoolean("Authentication","NTLMCache",        m_ntlmCache);
  CString realm     = p_config.GetParameterString ("Authentication","Realm",            m_realm);
  CString domain    = p_config.GetParameterString ("Authentication","Domain",           m_domain);

  // CHECK AND FIND the Authentication scheme
  ULONG authScheme = 0;
  if(!scheme.IsEmpty())
  {
    // Detect authentication scheme
         if(scheme == "Anonymous") authScheme = 0;
    else if(scheme == "Basic")     authScheme = HTTP_AUTH_ENABLE_BASIC;
    else if(scheme == "Digest")    authScheme = HTTP_AUTH_ENABLE_DIGEST;
    else if(scheme == "NTLM")      authScheme = HTTP_AUTH_ENABLE_NTLM;
    else if(scheme == "Negotiate") authScheme = HTTP_AUTH_ENABLE_NEGOTIATE;
    else if(scheme == "Kerberos")  authScheme = HTTP_AUTH_ENABLE_KERBEROS | HTTP_AUTH_ENABLE_NEGOTIATE;
    else if(scheme != "")
    {
      ERRORLOG(ERROR_INVALID_PARAMETER,"Invalid parameter for authentication scheme");
    }
  }

  // Remember the authentication scheme
  m_authScheme = authScheme;
  m_scheme     = scheme;
  m_realm      = realm;
  m_domain     = domain;
  m_ntlmCache  = ntlmCache;

  // Add and report the automatic headers as a last resort for responsive app's
  SetAutomaticHeaders(p_config);
}

// Write the settings to the logfile
// But only after we read all the config files
void
HTTPSiteMarlin::LogSettings()
{
  // Nothing to do
  if(m_server->GetLogLevel() < HLL_LOGGING)
  {
    return;
  }

  // Read XML Signing en encryption from the config
  CString level;
  switch(m_securityLevel)
  {
    case XMLEncryption::XENC_Plain:   level = "plain";   break;
    case XMLEncryption::XENC_Signing: level = "sign";    break;
    case XMLEncryption::XENC_Body:    level = "body";    break;
    case XMLEncryption::XENC_Message: level = "message"; break;
    default:                          level = "UNKNOWN"; break;
  }

  // Translate X-Frame options back
  CString option;
  switch(m_xFrameOption)
  {
    case XFrameOption::XFO_DENY:      option = "DENY";        break;
    case XFrameOption::XFO_SAMEORIGIN:option = "SAME-ORIGIN"; break;
    case XFrameOption::XFO_ALLOWFROM: option = "ALLOW-FROM";  break;
    case XFrameOption::XFO_NO_OPTION: option = "";            break;
    default:                          option = "UNKNOWN";     break;
  }

  // Authentication scheme
  CString scheme;
  switch(m_authScheme)
  {
    case 0:                           scheme = "Anonymous"; break;
    case HTTP_AUTH_ENABLE_BASIC:      scheme = "Basic";     break;
    case HTTP_AUTH_ENABLE_DIGEST:     scheme = "Digest";    break;
    case HTTP_AUTH_ENABLE_NTLM:       scheme = "NTLM";      break;
    case HTTP_AUTH_ENABLE_NEGOTIATE:  scheme = "Negotiate"; break;
    case HTTP_AUTH_ENABLE_KERBEROS | HTTP_AUTH_ENABLE_NEGOTIATE: // fall through
    case HTTP_AUTH_ENABLE_KERBEROS:   scheme = "Kerberos";  break;
    default:                          scheme = "UNKNOWN";   break;
  }

  // List other settings of the site
  //         "---------------------------------- : ------------"
  DETAILLOGS("Site SOAP WS-Security level        : ",       level);
  DETAILLOGS("Site authentication scheme         : ",       scheme);
  DETAILLOGV("Site authentication realm/domain   : %s/%s",  m_realm.GetString(),m_domain.GetString());
  DETAILLOGS("Site NT-LanManager caching         : ",       m_ntlmCache     ? "ON" : "OFF");
  DETAILLOGV("Site a-synchronious SOAP setting to: %sSYNC", m_async         ? "A-" : ""   );
  DETAILLOGS("Site accepting Server-Sent-Events  : ",       m_isEventStream ? "ON" : "OFF");
  DETAILLOGS("Site retaining all headers         : ",       m_allHeaders    ? "ON" : "OFF");
  DETAILLOGS("Site allows for HTTP-VERB Tunneling: ",       m_verbTunneling ? "ON" : "OFF");
  DETAILLOGS("Site uses HTTP Throtteling         : ",       m_throttling   ? "ON" : "OFF");
  DETAILLOGS("Site forces response to UTF-16     : ",       m_sendUnicode   ? "ON" : "OFF");
  DETAILLOGS("Site forces SOAP response UTF BOM  : ",       m_sendSoapBOM   ? "ON" : "OFF");
  DETAILLOGS("Site forces JSON response UTF BOM  : ",       m_sendJsonBOM   ? "ON" : "OFF");
  DETAILLOGS("Site WS-ReliableMessaging setting  : ",       m_reliable      ? "ON" : "OFF");
  DETAILLOGS("Site WS-RM needs logged in user    : ",       m_reliableLogIn ? "ON" : "OFF");
  DETAILLOGS("Site IFRAME options header         : ",       option);
  DETAILLOGS("Site allows to be IFRAME'd from    : ",       m_xFrameAllowed);
  DETAILLOGV("Site is HTTPS-only for at least    : %d seconds",m_hstsMaxAge);
  DETAILLOGS("Site does allow HTTPS subdomains   : ",       m_hstsSubDomains ? "YES":  "NO");
  DETAILLOGS("Site does allow content sniffing   : ",       m_xNoSniff       ? "NO" : "YES");
  DETAILLOGS("Site has XSS Protection set to     : ",       m_xXSSProtection ? "ON" : "OFF");
  DETAILLOGS("Site has XSS Protection block mode : ",       m_xXSSBlockMode  ? "ON" : "OFF");
  DETAILLOGS("Site blocking the browser caching  : ",       m_blockCache     ? "ON" : "OFF");
  DETAILLOGS("Site Cross-Origin-Resource-Sharing : ",       m_useCORS        ? "ON" : "OFF");
  DETAILLOGS("Site allows cross-origin(s)        : %s",     m_allowOrigin.IsEmpty() ? "*" : m_allowOrigin);
}

//////////////////////////////////////////////////////////////////////////
//
// Standard headers added by call responses from this site
//
//////////////////////////////////////////////////////////////////////////

// Set automatic headers upon starting site
void
HTTPSiteMarlin::SetAutomaticHeaders(WebConfig& p_config)
{
  // Find default value for xFrame options
  CString option;
  switch(m_xFrameOption)
  {
    case XFrameOption::XFO_NO_OPTION: option = "NOT-SET";     break;
    case XFrameOption::XFO_DENY:      option = "DENY";        break;
    case XFrameOption::XFO_SAMEORIGIN:option = "SAME-ORIGIN"; break;
    case XFrameOption::XFO_ALLOWFROM: option = "ALLOW-FROM";  break;
    default:                          option = "Unknown";     break;
  }

  // Read everything from the webconfig
  option            = p_config.GetParameterString ("Security", "XFrameOption",    option);
  m_xFrameAllowed   = p_config.GetParameterString ("Security", "XFrameAllowed",   m_xFrameAllowed);
  m_allowOrigin     = p_config.GetParameterString ("Security", "CORS_AllowOrigin",m_allowOrigin);
  m_hstsMaxAge      = p_config.GetParameterInteger("Security", "HSTSMaxAge",      m_hstsMaxAge);
  m_hstsSubDomains  = p_config.GetParameterBoolean("Security", "HSTSSubDomains",  m_hstsSubDomains);
  m_xNoSniff        = p_config.GetParameterBoolean("Security", "ContentNoSniff",  m_xNoSniff);
  m_xXSSProtection  = p_config.GetParameterBoolean("Security", "XSSProtection",   m_xXSSProtection);
  m_xXSSBlockMode   = p_config.GetParameterBoolean("Security", "XSSBlockMode",    m_xXSSBlockMode);
  m_blockCache      = p_config.GetParameterBoolean("Security", "NoCacheControl",  m_blockCache);
  m_useCORS         = p_config.GetParameterBoolean("Security", "CORS",            m_useCORS);

  // Translate X-Frame options back
       if(option.CompareNoCase("DENY")        == 0) m_xFrameOption = XFrameOption::XFO_DENY;
  else if(option.CompareNoCase("SAME-ORIGIN") == 0) m_xFrameOption = XFrameOption::XFO_SAMEORIGIN;
  else if(option.CompareNoCase("ALLOW-FROM")  == 0) m_xFrameOption = XFrameOption::XFO_ALLOWFROM;
  else                                              m_xFrameOption = XFrameOption::XFO_NO_OPTION;
}

