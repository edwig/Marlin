/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: HTTPClient.cpp
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
#include "StdAfx.h"
#include "HTTPClient.h"
#include "CrackURL.h"
#include "GetLastErrorAsString.h"
#include "ConvertWideString.h"
#include "HTTPMessage.h"
#include "SOAPMessage.h"
#include "AutoCritical.h"
#include "Analysis.h"
#include "ThreadPool.h"
#include "EventSource.h"
#include "Crypto.h"
#include "GetUserAccount.h"
#include "HTTPCertificate.h"
#include "HTTPClientTracing.h"
#include "HTTPError.h"
#include "OAuth2Cache.h"
#include "gzip.h"
#include <winerror.h>
#include <wincrypt.h>
#include <atlconv.h>
#include <process.h>
#include <vector>
#ifndef SECURITY_WIN32
#define SECURITY_WIN32
#endif
#include <Security.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// Logging macro's
#undef  DETAILLOG
#undef  ERRORLOG
#define DETAILLOG(text,...)   if(MUSTLOG(HLL_LOGGING) && m_log) m_log->AnalysisLog(__FUNCTION__,LogType::LOG_INFO, true,text,__VA_ARGS__)
#define ERRORLOG(text,...)    if(MUSTLOG(HLL_ERRORS)  && m_log) m_log->AnalysisLog(__FUNCTION__,LogType::LOG_ERROR,true,text,__VA_ARGS__)

// HTTP_STATUS_CONTINUE        -> No server yet, or server (temporarily down)
// HTTP_STATUS_OK              -> Server found
// HTTP_STATUS_SERVICE_UNAVAIL -> EOF on server stream channel, wait for reappearance of the server
// HTTP_STATUS_NO_CONTENT      -> W3C Standard says that a server can end a event stream with tis HTTP status 
//
#define CONTINUE_STREAM(status) ((m_status == HTTP_STATUS_CONTINUE) || \
                                 (m_status >= HTTP_STATUS_OK && m_status < HTTP_STATUS_NO_CONTENT) || \
                                 (m_status == HTTP_STATUS_SERVICE_UNAVAIL))

// CTOR for the client
HTTPClient::HTTPClient()
{
  InitializeCriticalSection(&m_queueSection);
  InitializeCriticalSection(&m_sendSection);
  
  // Stop the counter initially
  m_counter.Stop();
}

// DTOR for the client
HTTPClient::~HTTPClient()
{
  // Reset all
  Reset();
  CleanQueue();
  // Cleaning out the locks 
  DeleteCriticalSection(&m_queueSection);
  DeleteCriticalSection(&m_sendSection);
}

void
HTTPClient::Reset()
{
  // Execute only once!
  if(!m_initialized)
  {
    return;
  }

  // Stopping the queue
  StopClient();

  m_url.Empty();
  m_user.Empty();
  m_proxy.Empty();
  m_server.Empty();
  m_password.Empty();
  m_certName.Empty();
  m_proxyUser.Empty();
  m_soapAction.Empty();
  m_contentType.Empty();
  m_proxyBypass.Empty();
  m_proxyPassword.Empty();
  m_enc_password.Empty();

  m_agent           = "HTTPClient/1.0";
  m_scheme          = "http";
  m_retries         = 0;
  m_useProxy        = ProxyType::PROXY_IEPROXY;
  m_body            = NULL;
  m_bodyLength      = 0;
  m_buffer          = NULL;
  m_lastError       = 0;
  m_verb            = "GET";
  m_port            = INTERNET_DEFAULT_HTTP_PORT;
  m_secure          = false;
  m_sso             = false;
  m_status          = 0;
  m_responseLength  = 0;
  m_relax           = 0;
  m_ssltls          = WINHTTP_FLAG_SECURE_PROTOCOL_MARLIN;
  m_soapCompress    = false;
  m_httpCompression = false;
  m_verbTunneling   = false;
  m_terminalServices= false;
  m_securityLevel   = XMLEncryption::XENC_NoInit;
  m_ssltls          = WINHTTP_FLAG_SECURE_PROTOCOL_MARLIN;
  m_certPreset      = false;
  m_certStore       = "MY";
  m_sendUnicode     = false;
  m_sniffCharset    = true;
  m_sendBOM         = false;
  m_readAllHeaders  = false;
  m_pushEvents      = false;
  m_onCloseSeen     = false;
  // Timeouts
  m_timeoutResolve  = DEF_TIMEOUT_RESOLVE;
  m_timeoutConnect  = DEF_TIMEOUT_CONNECT;
  m_timeoutSend     = DEF_TIMEOUT_SEND;
  m_timeoutReceive  = DEF_TIMEOUT_RECEIVE;

  if(m_request)
  {
    WinHttpCloseHandle(m_request);
    m_request = NULL;
  }
  if(m_connect)
  {
    WinHttpCloseHandle(m_connect);
    m_connect = NULL;
  }
  if(m_session)
  {
    WinHttpCloseHandle(m_session);
    m_session = NULL;
  }
  if(m_response)
  {
    delete [] m_response;
    m_response = nullptr;
    m_responseLength = 0;
  }
  if(m_trace)
  {
    delete m_trace;
    m_trace = nullptr;
  }
  // Clear the maps
  m_requestHeaders.clear();
  m_responseHeaders.clear();
  m_cookies.Clear();
  m_resultCookies.Clear();

  // Reset the logging
  if(m_log && m_logOwner)
  {
    delete m_log;
    m_log      = NULL;
    m_logOwner = false;
  }
  m_logLevel = HLL_NOLOG;

  // Reset status
  m_initialized = false;
}

void
HTTPClient::CleanQueue()
{
  // Clean the queue
  HttpQueue::iterator it;
  for(it = m_queue.begin(); it != m_queue.end(); ++it)
  {
    MsgBuf& msg = *it;
    switch(msg.m_type)
    {
      case MsgType::HTPC_HTTP: delete msg.m_message.m_httpMessage; break;
      case MsgType::HTPC_SOAP: delete msg.m_message.m_soapMessage; break;
      case MsgType::HTPC_JSON: delete msg.m_message.m_jsonMessage; break;
    }
  }
  m_queue.clear();
}

// Disconnect from server
// Send() will now do a connect
void 
HTTPClient::Disconnect()
{
  if(m_connect)
  {
    DETAILLOG("Disconnect from server: %s", m_lastServer.GetString());
    WinHttpCloseHandle(m_connect);
    WinHttpCloseHandle(m_session);
    m_lastServer.Empty();
    m_scheme      = "http";
    m_connect     = NULL;
    m_session     = NULL;
    m_lastPort    = 0;
    m_lastSecure  = false;
    m_initialized = false;
  }
}

// Only disconnect if we are sending
// to a different host machine
void
HTTPClient::TestReconnect()
{
  if(m_server != m_lastServer ||
     m_port   != m_lastPort   ||
     m_secure != m_lastSecure)
  {
    Disconnect();
  }
}

int    
HTTPClient::GetError(CString* p_message /*=NULL*/)
{
  if(p_message && m_lastError)
  {
    p_message->Format("[%d] %s",m_lastError,GetLastErrorAsString(m_lastError).GetString());
  }
  return m_lastError;
}

CString
HTTPClient::GetStatusText()
{
  CString errorText;
  GetError(&errorText);
  CString text;
  text.Format("Status [%u] Windows error %lu: %s",m_status,m_lastError,errorText.GetString());
  return text;
}

// Logging error text MUST have the formatting of
// text text text %d %s
// Where %d is the error number and
// where %s is the error text explanation
int
HTTPClient::ErrorLog(const char* p_function,const char* p_logText)
{
  m_lastError = GetLastError();
  CString msg = GetLastErrorAsString(m_lastError);

  // ERRORLOG macro
  if(m_log && m_logLevel)
  {
    m_log->AnalysisLog(p_function, LogType::LOG_ERROR,true,p_logText,m_lastError,msg.GetString());
  }
  return m_lastError;
}

bool
HTTPClient::Initialize()
{
  USES_CONVERSION;
  CString totalURL;

  // Something to be done?
  if(m_initialized)
  {
    return true;
  }
  DETAILLOG("Initializing HTTP Client");
  m_initialized = true;

  // Try to read the Marlin.config file in the current directory
  if(m_marlinConfig.IsFilled())
  {
    DETAILLOG("Configuration file \'Marlin.config\' has already been read!");
  }
  else
  {
    m_marlinConfig.ReadConfig("Marlin.config");
    if(m_marlinConfig.IsFilled())
    {
      DETAILLOG("Configuration file \'marlin.config\' has been read!");
    }
    else
    {
      DETAILLOG("No configuration file [Marlin.config] found!");
    }
  }

  // Init logging. Log from here on
  InitLogging();

  // Init client settings from here
  InitSettings();

  // Reset error status
  m_lastError = 0;

  // Check that OS supports WinHttp functions
  if(!WinHttpCheckPlatform())
  {
    m_lastError = ERROR_APP_WRONG_OS;
    ERRORLOG("Your current MS-Windows OS does not support the WinHTTP Server/Client API");
    return false;
  }

  // Prepare for the fallback proxy in the OPEN call
  switch(m_useProxy)
  {
    case ProxyType::PROXY_IEPROXY:     [[fallthrough]];
    case ProxyType::PROXY_AUTOPROXY:   // Use proxy finder for overrides
                                       totalURL.Format("%s%s://%s:%d%s"
                                                      ,m_scheme.GetString()
                                                      ,m_secure ? "s" : ""
                                                      ,m_server.GetString()
                                                      ,m_port
                                                      ,m_url.GetString());
                                       m_proxy       = m_proxyFinder.Find(totalURL,m_secure);
                                       m_proxyBypass = m_proxyFinder.GetIngoreList();
                                       break;
    case ProxyType::PROXY_MYPROXY:     // Use proxy settings from the marlin.config. Do nothing
                                       break;
    case ProxyType::PROXY_NOPROXY:     // Reset all proxy settings. Forget everything
                                       m_proxy.Empty();
                                       m_proxyBypass.Empty();
                                       m_proxyUser.Empty();
                                       m_proxyPassword.Empty();
                                       break;

  }

  // Use WinHttpOpen to obtain a session handle.
  std::wstring wAgent       = A2CW(m_agent);
  std::wstring wProxy       = A2CW(m_proxy);;
  std::wstring wProxyBypass = A2CW(m_proxyBypass);

  m_session = WinHttpOpen(wAgent.c_str()
                         ,m_proxy.GetLength()       ? WINHTTP_ACCESS_TYPE_NAMED_PROXY : WINHTTP_ACCESS_TYPE_NO_PROXY
                         ,m_proxy.GetLength()       ? wProxy.c_str()                  : WINHTTP_NO_PROXY_NAME
                         ,m_proxyBypass.GetLength() ? wProxyBypass.c_str()            : WINHTTP_NO_PROXY_BYPASS
                         ,0); // Synchronized I/O
  if(m_session == NULL)
  {
    ErrorLog(__FUNCTION__,"Cannot open HTTP session. Error [%d] %s");
    return false;
  }
  DETAILLOG("HTTP Session opened for: %s",m_agent.GetString());
  if(m_proxy.GetLength())
  {
    DETAILLOG("HTTP Session set for proxy [%s] bypass is [%s]",m_proxy.GetString(),m_proxyBypass.GetString());
  }

  // We have a connection. Set the timeout values
  if(!WinHttpSetTimeouts(m_session
                        ,m_timeoutResolve
                        ,m_timeoutConnect
                        ,m_timeoutSend
                        ,m_timeoutReceive))
  {
    ErrorLog(__FUNCTION__,"Cannot set HTTP timeouts. Error [%d] %s");
    return false;
  }
  DETAILLOG("HTTP Timeouts set [%d/%d/%d/%d]",m_timeoutResolve,m_timeoutConnect,m_timeoutSend,m_timeoutReceive);

  // Prepare a tracing agent
  if(MUSTLOG(HLL_LOGBODY) && !m_trace)
  {
    m_trace = new HTTPClientTracing(this);
  }
  return true;
}

void
HTTPClient::InitializeSingleSignOn()
{
  if(m_request == NULL)
  {
    ERRORLOG("INTERNAL: Single-signon initialisation called without a valid request handle");
    return;
  }

  // Set the auto log on policy
  unsigned long option = WINHTTP_AUTOLOGON_SECURITY_LEVEL_LOW;
  if(WinHttpSetOption(m_request
                     ,WINHTTP_OPTION_AUTOLOGON_POLICY
                     ,&option
                     ,sizeof(unsigned long)))
  {
    DETAILLOG("HTTP Autologon policy set (For use on local intranet servers only!)");
  }
  else
  {
    ErrorLog(__FUNCTION__,"Cannot set the auto-logon policy. Error [%d] %s");
    return;
  }
}

void
HTTPClient::InitLogging()
{
  // Something to be done?
  if(m_initialized)
  {
    return;
  }

  // Get parameters from Marlin.config
  CString file = m_marlinConfig.GetParameterString ("Logging","Logfile",  "");
  bool logging = m_marlinConfig.GetParameterBoolean("Logging","DoLogging",false);
  bool timing  = m_marlinConfig.GetParameterBoolean("Logging","DoTiming", true);
  bool events  = m_marlinConfig.GetParameterBoolean("Logging","DoEvents", false);
  int  cache   = m_marlinConfig.GetParameterInteger("Logging","Cache",    100);
  int  level   = m_marlinConfig.GetParameterInteger("Logging","LogLevel", m_logLevel);

  // Check for a logging object
  if(m_log == NULL && !file.IsEmpty() && logging)
  {
    // Create a new one
    m_log = new LogAnalysis(m_agent);
    m_logOwner = true;
  }

  // Make other settings effective on the created logfile
  // BUT REMEMBER: "Logging.config" can override these settings.
  if(m_log && !file.IsEmpty())
  {
    m_log->SetLogFilename(file);
  }
  if(m_log && m_marlinConfig.HasParameter("Logging","DoLogging"))
  {
    m_log->SetDoLogging(logging);
  }
  if(m_log && m_marlinConfig.HasParameter("Logging","DoTiming"))
  {
    m_log->SetDoTiming(timing);
  }
  if(m_log && m_marlinConfig.HasParameter("Logging","DoEvents"))
  {
    m_log->SetDoEvents(events);
  }
  if(m_log && m_marlinConfig.HasParameter("Logging","Cache"))
  {
    m_log->SetCache(cache);
  }
  if(m_log && m_marlinConfig.HasParameter("Logging","LogLevel"))
  {
    m_log->SetLogLevel(m_logLevel = level);
  }
}

void
HTTPClient::SetLogging(LogAnalysis* p_log,bool p_transferOwnership /*= false*/)
{
  if(m_log && m_logOwner)
  {
    delete m_log;
    m_log = nullptr;
    m_logOwner = false;
  }
  // Remember the setting or resetting of the logfile
  m_log = p_log;

  // HTTP Client will destroy the logfile object!
  if(p_transferOwnership)
  {
    m_logOwner = true;
  }

  // In case we just gotten a new logfile, init it!
  if(m_log)
  {
    m_logLevel = p_log->GetLogLevel();
    InitLogging();
  }
}

void
HTTPClient::InitSettings()
{
  // General client settings
  m_retries        =             m_marlinConfig.GetParameterInteger("Client","RetryCount",       m_retries);
  m_agent          =             m_marlinConfig.GetParameterString ("Client","Agent",            m_agent);
  m_useProxy       = (ProxyType) m_marlinConfig.GetParameterInteger("Client","UseProxy",         (int)m_useProxy);
  m_proxy          =             m_marlinConfig.GetParameterString ("Client","Proxy",            m_proxy);
  m_proxyBypass    =             m_marlinConfig.GetParameterString ("Client","ProxyBypass",      m_proxyBypass);
  m_timeoutResolve =             m_marlinConfig.GetParameterInteger("Client","TimeoutResolve",   m_timeoutResolve);
  m_timeoutConnect =             m_marlinConfig.GetParameterInteger("Client","TimeoutConnect",   m_timeoutConnect);
  m_timeoutSend    =             m_marlinConfig.GetParameterInteger("Client","TimeoutSend",      m_timeoutSend);
  m_timeoutReceive =             m_marlinConfig.GetParameterInteger("Client","TimeoutReceive",   m_timeoutReceive);
  m_soapCompress   =             m_marlinConfig.GetParameterBoolean("Client","SOAPCompress",     m_soapCompress);
  m_httpCompression=             m_marlinConfig.GetParameterBoolean("Client","HTTPCompression",  m_httpCompression);
  m_sendUnicode    =             m_marlinConfig.GetParameterBoolean("Client","SendUnicode",      m_sendUnicode);
  m_sendBOM        =             m_marlinConfig.GetParameterBoolean("Client","SendBOM",          m_sendBOM);
  m_verbTunneling  =             m_marlinConfig.GetParameterBoolean("Client","VerbTunneling",    m_verbTunneling);
  m_certPreset     =             m_marlinConfig.GetParameterBoolean("Client","CertificatePreset",m_certPreset);
  m_certStore      =             m_marlinConfig.GetParameterString ("Client","CertificateStore", m_certStore);
  m_certName       =             m_marlinConfig.GetParameterString ("Client","CertificateName",  m_certName);
  m_corsOrigin     =             m_marlinConfig.GetParameterString ("Client","CORS_Origin",      m_corsOrigin);

  // Test environments must often do with relaxed certificate settings
  bool m_relaxValid     = m_marlinConfig.GetParameterBoolean("Client","RelaxCertificateValid",     false);
  bool m_relaxDate      = m_marlinConfig.GetParameterBoolean("Client","RelaxCertificateDate",      false);
  bool m_relaxAuthority = m_marlinConfig.GetParameterBoolean("Client","RelaxCertificateAuthority", false);
  bool m_relaxUsage     = m_marlinConfig.GetParameterBoolean("Client","RelaxCertificateUsage",     false);

  if(m_relaxValid)      m_relax |= SECURITY_FLAG_IGNORE_CERT_CN_INVALID;
  if(m_relaxDate)       m_relax |= SECURITY_FLAG_IGNORE_CERT_DATE_INVALID;
  if(m_relaxAuthority)  m_relax |= SECURITY_FLAG_IGNORE_UNKNOWN_CA;
  if(m_relaxUsage)      m_relax |= SECURITY_FLAG_IGNORE_CERT_WRONG_USAGE;

  // Overrides for Secure HTTP protocol
  unsigned ssltls = 0;
  CString  ssltlsLogging(":");
  if(m_marlinConfig.GetParameterBoolean("Client","SecureSSL20",(m_ssltls & WINHTTP_FLAG_SECURE_PROTOCOL_SSL2) > 0))
  {
    ssltls |= WINHTTP_FLAG_SECURE_PROTOCOL_SSL2;
    ssltlsLogging += "SSL2:";
  }
  if(m_marlinConfig.GetParameterBoolean("Client","SecureSSL30",(m_ssltls & WINHTTP_FLAG_SECURE_PROTOCOL_SSL3) > 0))
  {
    ssltls |= WINHTTP_FLAG_SECURE_PROTOCOL_SSL3;
    ssltlsLogging += "SSL3:";
  }
  if(m_marlinConfig.GetParameterBoolean("Client","SecureTLS10",(m_ssltls & WINHTTP_FLAG_SECURE_PROTOCOL_TLS1) > 0))
  {
    ssltls |= WINHTTP_FLAG_SECURE_PROTOCOL_TLS1;
    ssltlsLogging += "TLS1:";
  }
  if(m_marlinConfig.GetParameterBoolean("Client","SecureTLS11",(m_ssltls & WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_1) > 0))
  {
    ssltls |= WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_1;
    ssltlsLogging += "TLS1.1:";
  }
  if(m_marlinConfig.GetParameterBoolean("Client","SecureTLS12",(m_ssltls & WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2) > 0))
  {
    ssltls |= WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2;
    ssltlsLogging += "TLS1.2:";
  }
  m_ssltls = ssltls;

  // Overrides of the URL can be set manual
  m_user          = m_marlinConfig.GetParameterString ("Authentication","User",         m_user);
  m_password      = m_marlinConfig.GetEncryptedString ("Authentication","Password",     m_password);
  m_sso           = m_marlinConfig.GetParameterBoolean("Authentication","SSO",          m_sso);
  m_proxyUser     = m_marlinConfig.GetParameterString ("Client",        "ProxyUser",    m_proxyUser);
  m_proxyPassword = m_marlinConfig.GetEncryptedString ("Client",        "ProxyPassword",m_proxyPassword);

  // Test that we do not keep on nagging the server
  if(m_retries > CLIENT_MAX_RETRIES)
  {
    m_retries = CLIENT_MAX_RETRIES;
  }

  // Proxy type
  CString proxyType;
  switch(m_useProxy)
  {
    case ProxyType::PROXY_IEPROXY:   proxyType = "Always use IE autoproxy settings in the connect if possible (default)"; break;
    case ProxyType::PROXY_AUTOPROXY: proxyType = "Use IE autoproxy if connection fails as a default fallback";            break;
    case ProxyType::PROXY_MYPROXY:   proxyType = "Use MY proxy settings from Marlin.config";                              break;
    case ProxyType::PROXY_NOPROXY:   proxyType = "Never use any proxy";                                                   break;
    default:                         proxyType = "Unknown and unsupported proxy settings!!";
                                     ERRORLOG(proxyType);
                                     break;
  }

  // Logging level
  CString loglevel;
  switch(m_logLevel)
  {
    case HLL_NOLOG:     loglevel = "No logging";          break;
    case HLL_ERRORS:    loglevel = "Errors and warnings"; break;
    case HLL_LOGGING:   loglevel = "Info logging";        break;
    case HLL_TRACE:     loglevel = "Tracing and bodies";  break;
    case HLL_TRACEDUMP: loglevel = "Tracing bodies, headers and hex-dump"; break;
  }

  // Logging of our settings
  DETAILLOG("Client logging level                 : %s",loglevel.GetString());
  DETAILLOG("Client retry counts                  : %d",m_retries);
  DETAILLOG("Client agent string                  : %s",m_agent.GetString());
  DETAILLOG("CLient compresses soap output        : %s",m_soapCompress   ? "yes" : "no");
  DETAILLOG("Client allows HTTP compression (gzip): %s",m_httpCompression? "yes" : "no");
  DETAILLOG("Client relax validity  of certificate: %s",m_relaxValid     ? "yes" : "no");
  DETAILLOG("Client relax date      on certificate: %s",m_relaxDate      ? "yes" : "no");
  DETAILLOG("Client relax authority of certificate: %s",m_relaxAuthority ? "yes" : "no");
  DETAILLOG("Client relax usage     of certificate: %s",m_relaxUsage     ? "yes" : "no");
  DETAILLOG("Client accepts secure protocols      : %s",ssltlsLogging.GetString());
  if(!m_user.IsEmpty())
  {
    DETAILLOG("Client authorized for user           : %s",m_user.GetString());
    DETAILLOG("Client authroized password filled    : %s",m_password.IsEmpty() ? "no" : "yes");
  }
  if(m_useProxy >= ProxyType::PROXY_IEPROXY && m_useProxy <= ProxyType::PROXY_NOPROXY)
  {
    DETAILLOG("Client proxy usage                   : %s",proxyType.GetString());
    DETAILLOG("Client uses proxy                    : %s",m_proxy.GetString());
    DETAILLOG("Clients will ignore these proxies    : %s",m_proxyBypass.GetString());
    if(!m_proxyUser.IsEmpty())
    {
      DETAILLOG("Client proxy user                    : %s",m_proxyUser.GetString());
      DETAILLOG("Client proxy password filled         : %s",m_proxyPassword.IsEmpty() ? "no" : "yes");
    }
  }
  DETAILLOG("Client single-sign-on NT-LanManager  : %s",m_sso           ? "yes" : "no");
  DETAILLOG("Client forces sending in UTF-16      : %s",m_sendUnicode   ? "yes" : "no");
  DETAILLOG("Client forces sending of Unicode BOM : %s",m_sendBOM       ? "yes" : "no");
  DETAILLOG("Client forces HTTP VERB Tunneling    : %s",m_verbTunneling ? "yes" : "no");
  DETAILLOG("Client will use CORS origin header   : %s",m_corsOrigin.GetString());
}

// Initialise the security mechanisms
void
HTTPClient::InitSecurity()
{
  // Already initialized?
  if(m_securityLevel > XMLEncryption::XENC_NoInit)
  {
    return;
  }
  // Read XML Signing en encryption from the config
  CString level;
  switch(m_securityLevel)
  {
    case XMLEncryption::XENC_Plain:   level = "";        break;
    case XMLEncryption::XENC_Signing: level = "sign";    break;
    case XMLEncryption::XENC_Body:    level = "body";    break;
    case XMLEncryption::XENC_Message: level = "message"; break;
  }
  level          = m_marlinConfig.GetParameterString("Encryption","Level",   level);
  m_enc_password = m_marlinConfig.GetEncryptedString("Encryption","Password",m_enc_password);

  // Now set the resulting security level
       if(level == "sign")    m_securityLevel = XMLEncryption::XENC_Signing;
  else if(level == "body")    m_securityLevel = XMLEncryption::XENC_Body;
  else if(level == "message") m_securityLevel = XMLEncryption::XENC_Message;
  else                        m_securityLevel = XMLEncryption::XENC_Plain;

  if(!level.IsEmpty())
  {
    DETAILLOG("Client set for SOAP WS-Security level: %s",level.GetString());
  }
}

void
HTTPClient::ReplaceSetting(CString* m_setting,CString p_potential)
{
  if(p_potential.IsEmpty())
  {
    // No settings found
    return;
  }
  // Potential is now filled
  if(m_setting->IsEmpty())
  {
    // New setting found
    *m_setting = p_potential;
    return;
  }
  // Both are now filled
  if(m_setting->Compare(p_potential))
  {
    // Replace if they are different
    *m_setting = p_potential;
    return;
  }
  // both filled and equal: do nothing
}

bool
HTTPClient::SetURL(CString p_url)
{
  // Keep the complete URL
  m_url = p_url;
  DETAILLOG("URL set to: %s",p_url.GetString());

  CrackedURL url(p_url);
  if(url.Valid())
  {
    m_scheme   = url.m_scheme;
    m_secure   = url.m_secure;
    m_server   = url.m_host;
    m_port     = url.m_port;
    m_url      = url.AbsolutePath();
    return true;
  }
  // Generic path-not-found error
  m_lastError = ERROR_PATH_NOT_FOUND;
  ERRORLOG("Invalid URL: %s",p_url.GetString());
  return false;
}

// Add extra header for the call
bool 
HTTPClient::AddHeader(CString p_header)
{
  int pos = p_header.Find(':');
  if (pos > 0)
  {
    CString name   = p_header.Left(pos);
    CString value  = p_header.Mid(pos + 1).Trim();

    AddHeader(name,value);
    return true;
  }
  return false;
}

// Add extra header by name and value pair
void
HTTPClient::AddHeader(CString p_name,CString p_value)
{
  // Case-insensitive search!
  HeaderMap::iterator it = m_requestHeaders.find(p_name);
  if(it != m_requestHeaders.end())
  {
    // Check if we set it a duplicate time
    // If appended, we do not append it a second time
    if(it->second.Find(p_value) >= 0)
    {
      return;
    }
    if(p_name.CompareNoCase("Set-Cookie") == 0)
    {
      // Insert as a new header
      m_requestHeaders.insert(std::make_pair(p_name,p_value));
      return;
    }
    // New value of the header
    it->second = p_value;
  }
  else
  {
    // Insert as a new header
    m_requestHeaders.insert(std::make_pair(p_name,p_value));
  }
}

// Delete a header
bool 
HTTPClient::DelHeader(CString p_name)
{
  HeaderMap::iterator it = m_requestHeaders.find(p_name);
  if(it != m_requestHeaders.end())
  {
    m_requestHeaders.erase(it);
    return true;
  }
  return false;
}

// Add extra cookie for the call
bool 
HTTPClient::AddCookie(CString p_cookie)
{
  m_cookies.AddCookie(p_cookie);
  return true;
}

void 
HTTPClient::SetBody(CString& p_body)
{
  m_body = (void*) p_body.GetString();
  m_bodyLength =   p_body.GetLength();
}

void 
HTTPClient::SetBody(void* p_body,unsigned p_length)
{ 
  m_body       = p_body;    
  m_bodyLength = p_length;
};

void    
HTTPClient::GetBody(void*& p_body,unsigned& p_length)
{
  p_body   = m_body;
  p_length = m_bodyLength;
}

void    
HTTPClient::GetResponse(BYTE*& p_response,unsigned& p_length)
{
  p_response = m_response;
  p_length   = m_responseLength;
}

HINTERNET
HTTPClient::GetWebsocketHandle()
{
  if(m_websocket)
  {
    // We only expose our private request handle
    // in the case of a WebSocket upgrade!
    return m_request;
  }
  return NULL;
}

//////////////////////////////////////////////////////////////////////////
// 
// OLD logging interface
//
//////////////////////////////////////////////////////////////////////////

bool
HTTPClient::GetDetailLogging()
{
  TRACE("WARNING: Rewrite your program with GetLogLevel()\n");
  return (m_logLevel > HLL_ERRORS);
}

bool
HTTPClient::GetTraceRequest()
{
  TRACE("WARNING: Rewrite your program with GetLogLevel()\n");
  return (m_logLevel >= HLL_TRACE);
}

bool
HTTPClient::GetTraceData()
{
  TRACE("WARNING: Rewrite your program with GetLogLevel()\n");
  return (m_logLevel >= HLL_TRACEDUMP);
}

void 
HTTPClient::SetDetailLogging(bool p_detail)
{
  TRACE("WARNING: Rewrite your program with SetLogLevel()\n");
  m_logLevel = p_detail ? HLL_LOGGING : HLL_NOLOG;
}

void 
HTTPClient::SetTraceRequest(bool p_trace)
{
  TRACE("WARNING: Rewrite your program with SetLogLevel()\n");
  m_logLevel = p_trace ? HLL_TRACE : HLL_LOGGING;
}

void
HTTPClient::SetTraceData(bool p_trace)
{
  TRACE("WARNING: Rewrite your program with SetLogLevel()\n");
  m_logLevel = p_trace ? HLL_TRACEDUMP : HLL_LOGGING;
}

// New interface for loglevel

void 
HTTPClient::SetLogLevel(int p_logLevel)
{
  // Check for boundaries
  if(p_logLevel < HLL_NOLOG)   p_logLevel = HLL_NOLOG;
  if(p_logLevel > HLL_HIGHEST) p_logLevel = HLL_HIGHEST;

  // Remember our loglevel for the client AND the logfile
  m_logLevel = p_logLevel;
  if (m_log)
  {
    m_log->SetLogLevel(p_logLevel);
  }
}

//////////////////////////////////////////////////////////////////////////
//
// Sending our request, and everything it depends on
// (authorization, proxies etc)
//
//////////////////////////////////////////////////////////////////////////

void
HTTPClient::AddProxyInfo()
{
  WINHTTP_PROXY_INFO* pinfo = &m_proxyFinder.GetProxyInfo()->cfg;
  
  if(pinfo && wcslen(pinfo->lpszProxy) > 0)
  {
    if(::WinHttpSetOption(m_request,WINHTTP_OPTION_PROXY,pinfo,sizeof(WINHTTP_PROXY_INFO)))
    {
      DETAILLOG("Proxy enabled to proxy: %s",m_proxy.GetString());
      DETAILLOG("Proxy settings will be ignored for: %s",m_proxyBypass.GetString());
    }
    else
    {
      ErrorLog(__FUNCTION__,"Error enabling proxy settings [%d] %s");
      ERRORLOG("Proxy that cannot be enabled: %s",m_proxy.GetString());
    }
  }
  else
  {
    DETAILLOG("No proxy found or configured");
  }
}

void
HTTPClient::AddHostHeader()
{
  if(m_server.IsEmpty())
  {
    // Huh? No host server?
    ERRORLOG("Cannot set a host-header. No known host server!");
    return;
  }

  // Create host header
  CString host(m_server);
  if((m_secure && m_port != INTERNET_DEFAULT_HTTPS_PORT) ||
    (!m_secure && m_port != INTERNET_DEFAULT_HTTP_PORT))
  {
    host.AppendFormat(":%d",m_port);
  }
  AddHeader("Host",host);
}

// Add content length header
void
HTTPClient::AddLengthHeader()
{
  // Remove old/incorrect content-length header
  DelHeader("Content-Length");

  // Set our header according to what we are about to send
  CString length;
  length.Format("%lu",m_bodyLength);
  AddHeader("Content-Length",length);
}

void
HTTPClient::AddSecurityOptions()
{
  // If HTTPS, then the client is very susceptible to invalid certificates
  // and other specific settings.
  // Allow security settings to be tweaked before sending the request
  if(m_secure)
  {
    if(m_relax)
    {
      if(!::WinHttpSetOption(m_request
                            ,WINHTTP_OPTION_SECURITY_FLAGS
                            ,(LPVOID)&m_relax
                            ,sizeof(DWORD)))
      {
        ErrorLog(__FUNCTION__,"Security options NOT set. Error [%d] %s");
        return;
      }
      DETAILLOG("Security relax options set: %X",m_relax);
    }
    // Accept SSL (2&3) and TLS for now
    // m_ssltls is standard set to WINHTTP_FLAG_SECURE_PROTOCOL_ALL from winhttp.h
    DWORD options = m_ssltls;
    if(!::WinHttpSetOption(m_session
                          ,WINHTTP_OPTION_SECURE_PROTOCOLS
                          ,(LPVOID)&options
                          ,sizeof(DWORD)))
    {
      ErrorLog(__FUNCTION__,"Security protocols NOT set. Error [%d] %s");
      return;
    }
    if(MUSTLOG(HLL_LOGGING))
    {
      CString allow;
      if(m_ssltls & WINHTTP_FLAG_SECURE_PROTOCOL_SSL2)   allow  = "SSL 2.0, "; 
      if(m_ssltls & WINHTTP_FLAG_SECURE_PROTOCOL_SSL3)   allow += "SSL 3.0, "; 
      if(m_ssltls & WINHTTP_FLAG_SECURE_PROTOCOL_TLS1)   allow += "TLS 1.0, ";
      if(m_ssltls & WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_1) allow += "TLS 1.1, ";
      if(m_ssltls & WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2) allow += "TLS 1.2, ";
      allow.TrimRight(", ");

      DETAILLOG("Security protocol allows: %s",allow.GetString());
    }
  }
}

// Cross Origin Resource Sharing
void
HTTPClient::AddCORSHeaders()
{
  // See if we must do CORS
  if(m_corsOrigin.IsEmpty())
  {
    return;
  }

  // Creating our origin header
  AddHeader("Origin",m_corsOrigin);
  DETAILLOG("Added CORS origin header: %s",m_corsOrigin.GetString());

  // In a Pre-flight request, we can add request method and headers
  if(m_verb.Compare("OPTIONS") == 0)
  {
    if(!m_corsMethod.IsEmpty())
    {
      AddHeader("Access-Control-Request-Method",m_corsMethod);
      DETAILLOG("Added CORS request method: %s",m_corsMethod.GetString());
    }
    if(!m_corsHeaders.IsEmpty())
    {
      AddHeader("Access-Control-Request-Headers",m_corsHeaders);
      DETAILLOG("Added CORS request headers: %s",m_corsHeaders.GetString());
    }
  }
}

// Add all extra headers that we came upon
void
HTTPClient::AddExtraHeaders()
{
  if(!m_contentType.IsEmpty())
  {
    AddHeader("Content-Type",m_contentType);
  }
  if(m_httpCompression)
  {
    AddHeader("Accept-Encoding","gzip");
  }
  if(m_terminalServices)
  {
    // Try to get the TerminalServices desktop session number
    // int desktop = GetSystemMetrics(SM_REMOTESESSION);
    // Find port offset for listener/logins
    DWORD session = 0;
    DWORD pid = GetCurrentProcessId();
    if(ProcessIdToSessionId(pid,&session))
    {
      char number[20];
      _itoa_s(session,number,20,10);
      AddHeader("RemoteDesktop",number);
    }
  }
}

void
HTTPClient::AddCookieHeaders()
{
  if(m_cookies.GetSize())
  {
    AddHeader("Cookie",m_cookies.GetCookieText());
  }
}

// Add OAuth2 authorization if configured for this call
bool
HTTPClient::AddOAuth2authorization()
{
  bool result = false;

  if(m_oauthCache && m_oauthSession)
  {
    CString token = m_oauthCache->GetBearerToken(m_oauthSession);
    if(!token.IsEmpty())
    {
      CString bearerToken("Bearer ");
      bearerToken += token;
      AddHeader("Authorization",bearerToken);
      m_lastBearerToken = token;
      result = true;
    }
  }
  return result;
}

void 
HTTPClient::FlushAllHeaders()
{
  // Set all of our headers
  USES_CONVERSION;
  for(auto& head : m_requestHeaders)
  {
    CString header = head.first + ":" + head.second;
    wstring theHeader = A2CW(header);

    DWORD modifiers = WINHTTP_ADDREQ_FLAG_ADD | WINHTTP_ADDREQ_FLAG_REPLACE;
    if(!::WinHttpAddRequestHeaders(m_request
                                  ,theHeader.c_str()
                                  ,(DWORD)theHeader.size()
                                  ,modifiers))
    {
      // Log error but continue. A lot of headers are optional to a HTTP call
      ErrorLog(__FUNCTION__,"Request headers NOT set. Error [%d] %s");
    }
    DETAILLOG("Header => %S",theHeader.c_str());
  }
}

// Setting the proxy authorization (if any)
void
HTTPClient::AddProxyAuthorization()
{
  if(m_proxy.GetLength() > 0)
  {
    if(m_proxyUser.GetLength() > 0)
    {
      USES_CONVERSION;

      wstring user(A2CW(m_proxyUser));
      if(!::WinHttpSetOption(m_request, WINHTTP_OPTION_PROXY_USERNAME, (LPVOID)user.c_str(), (DWORD)user.size() * sizeof(wchar_t)))
      {
        ErrorLog(__FUNCTION__,"Proxy username NOT set. Error [%d] %s");
      }
      DETAILLOG("Proxy user: %s",m_proxyUser.GetString());
      if(m_proxyPassword.GetLength() > 0)
      {
        wstring password(A2CW(m_proxyPassword));
        if(!::WinHttpSetOption(m_request, WINHTTP_OPTION_PROXY_PASSWORD, (LPVOID)password.c_str(), (DWORD)password.size() * sizeof(wchar_t)))
        {
          ErrorLog(__FUNCTION__,"Proxy password NOT set. Error [%d] %s");
        }
        DETAILLOG("Proxy password set: ******");
      }
    }
  }
}

// Add authorization up-front before the call
// Sparing an extra round-trip, if you know the site will ask for it!
void
HTTPClient::AddPreEmptiveAuthorization()
{
  if(m_preemtive && !m_user.IsEmpty() && !m_password.IsEmpty())
  {
    if(m_preemtive & (WINHTTP_AUTH_SCHEME_BASIC      |
                      WINHTTP_AUTH_SCHEME_NTLM       |
                      WINHTTP_AUTH_SCHEME_PASSPORT   |
                      WINHTTP_AUTH_SCHEME_DIGEST     |
                      WINHTTP_AUTH_SCHEME_NEGOTIATE  ))
    {
      USES_CONVERSION;

      wstring user = A2CW(m_user);
      wstring pass = A2CW(m_password);

      if(!WinHttpSetCredentials(m_request
                               ,WINHTTP_AUTH_TARGET_SERVER
                               ,m_preemtive
                               ,user.c_str()
                               ,pass.c_str()
                               ,NULL))
      {
        // Could not set the credentials
        ErrorLog(__FUNCTION__,"Setting HTTP Credentials pre-emptively. Error [%d] %s");
      }
      else
      {
        DETAILLOG("Authentication set for user: %s",m_user.GetString());
      }
    }
    else 
    {
      ERRORLOG("Illegal pre-emptive authorization setting: %d",m_preemtive);
    }
  }
}

void
HTTPClient::ResetOAuth2Session()
{
  if(m_oauthCache && m_oauthSession)
  {
    m_oauthCache->SetExpired(m_oauthSession);
  }
}

void
HTTPClient::AddWebSocketUpgrade()
{
  // Look if we have work to do
  if(!m_websocket)
  {
    return;
  }
  // Principal WebSocket handshake
  if(::WinHttpSetOption(m_request,WINHTTP_OPTION_UPGRADE_TO_WEB_SOCKET,NULL,0))
  {
    DETAILLOG("Prepared for WebSocket upgrade.");
  }
  else
  {
    ErrorLog(__FUNCTION__,"Cannot set option to upgrade to WebSocket. Error [%d] %s");
  }
}

CString
HTTPClient::GetResultHeader(DWORD p_header,DWORD p_index)
{
  DWORD dwSize = 0;
  CString result;

  // Get the result size
  BOOL bResult = ::WinHttpQueryHeaders(m_request,
                                       p_header,
                                       WINHTTP_HEADER_NAME_BY_INDEX,
                                       NULL,
                                       &dwSize,
                                       &p_index);

  if(bResult || (!bResult && (::GetLastError() == ERROR_INSUFFICIENT_BUFFER)))
  {
    wchar_t* szValue = new wchar_t[dwSize];
    if (szValue != NULL)
    {
      USES_CONVERSION;

      memset(szValue, 0, dwSize* sizeof(wchar_t));
      if (::WinHttpQueryHeaders(m_request,
                                p_header,
                                WINHTTP_HEADER_NAME_BY_INDEX,
                                szValue,
                                &dwSize,
                                &p_index))
      {
        result = CW2A(szValue);
        DETAILLOG("Result header: %s",result.GetString());
      }
      else
      {
        ErrorLog(__FUNCTION__,"Cannot get known result header. Error [%d] %s");
      }
      delete[] szValue;
    }
  }
  return result;
}

void
HTTPClient::ProcessResultCookies()
{
  m_resultCookies.Clear();

  CString header = GetResultHeader(WINHTTP_QUERY_COOKIE,WINHTTP_NO_HEADER_INDEX);
  m_resultCookies.AddCookie(header);

  unsigned ind = 0;
  do
  {
    header = GetResultHeader(WINHTTP_QUERY_SET_COOKIE,ind++);
    if(!header.IsEmpty())
    {
      m_resultCookies.AddCookie(header);
    }
  }
  while(!header.IsEmpty());
}

DWORD 
HTTPClient::ChooseAuthScheme(DWORD p_dwSupportedSchemes)
{
  //  It is the server's responsibility only to accept 
  //  authentication schemes that provide a sufficient level
  //  of security to protect the server's resources.
  //
  //  The client is also obligated only to use an authentication
  //  scheme that adequately protects its username and password.
  //
  //  Thus, this client does not use Basic authentication  
  //  because Basic authentication exposes the client's username 
  //  and password to anyone monitoring the connection.

  if(p_dwSupportedSchemes & WINHTTP_AUTH_SCHEME_NEGOTIATE)
  {
    DETAILLOG("Autorisatie: Negotiate");
    return WINHTTP_AUTH_SCHEME_NEGOTIATE;
  }
  else if(p_dwSupportedSchemes & WINHTTP_AUTH_SCHEME_NTLM)
  {
    DETAILLOG("Autorisatie: NTLM");
    return WINHTTP_AUTH_SCHEME_NTLM;
  }
  else if(p_dwSupportedSchemes & WINHTTP_AUTH_SCHEME_PASSPORT)
  {
    DETAILLOG("Autorisatie: Passport");
    return WINHTTP_AUTH_SCHEME_PASSPORT;
  }
  else if(p_dwSupportedSchemes & WINHTTP_AUTH_SCHEME_DIGEST)
  {
    DETAILLOG("Autorisatie: Digest");
    return WINHTTP_AUTH_SCHEME_DIGEST;
  }
  else if(p_dwSupportedSchemes & WINHTTP_AUTH_SCHEME_BASIC)
  {
    DETAILLOG("Autorisatie: Basic");
    return WINHTTP_AUTH_SCHEME_BASIC;
  }
  else
  {
    // NO AUTHORIZATION!!
    DETAILLOG("Autorisatie: No authorization");
    return 0;
  }
}

bool
HTTPClient::AddAuthentication(bool p_ntlm3Step)
{
  DWORD dwSupportedSchemes = 0;
  DWORD dwFirstScheme = 0;
  DWORD dwTarget = 0;
  DWORD dwSelectedScheme = 0;
  bool  setCredentials = false;
  
  if(p_ntlm3Step)
  {
    dwSelectedScheme = WINHTTP_AUTH_SCHEME_NTLM;
  }
  else
  {
    // Obtain the supported and preferred schemes.
    if(WinHttpQueryAuthSchemes(m_request, 
                              &dwSupportedSchemes, 
                              &dwFirstScheme, 
                              &dwTarget ))
    {
      dwSelectedScheme = ChooseAuthScheme(dwSupportedSchemes);

      if(dwSelectedScheme == 0)
      {
        SetLastError(ERROR_ACCESS_DENIED);
        ErrorLog(__FUNCTION__,"Choose authentication scheme. Error [%d] %s");
        return false;
      }
      else
      {
        setCredentials = true;
      }
    }
    else
    {
      // Could not get the authorization scheme
      ErrorLog(__FUNCTION__,"Cannot query authentication schema. Error [%d] %s");
      return false;
    }
  }

  // Check if we must do single-sign-on
  if(m_sso)
  {
    if(m_user.IsEmpty())
    {
      m_user = GetUserAccount();
    }
  }

  // Set the credentials
  if(setCredentials || p_ntlm3Step)
  {
    USES_CONVERSION;

    wstring user = A2CW(m_user);
    wstring pass = A2CW(m_password);

    if(!WinHttpSetCredentials(m_request
                             ,dwTarget
                             ,dwSelectedScheme
                             ,user.c_str()
                             ,pass.c_str()
                             ,NULL))
    {
      // Could not set the credentials
      ErrorLog(__FUNCTION__,"Setting HTTP Credentials pre-emptively. Error [%d] %s");
      return false;
    }
    DETAILLOG("Authentication set for user: %s",m_user.GetString());
  }
  return true;
}

bool
HTTPClient::AddProxyAuthentication()
{
  DWORD dwSupportedSchemes = 0;
  DWORD dwFirstScheme = 0;
  DWORD dwTarget = 0;

  // Obtain the supported and preferred schemes.
  if(WinHttpQueryAuthSchemes(m_request, 
                            &dwSupportedSchemes, 
                            &dwFirstScheme, 
                            &dwTarget ))
  {
    DWORD dwSelectedScheme = ChooseAuthScheme(dwSupportedSchemes);

    if(dwSelectedScheme == 0)
    {
      m_lastError = ERROR_ACCESS_DENIED;
      return false;
    }
    else
    {
      USES_CONVERSION;

      static wstring user = A2CW(m_proxyUser);
      static wstring pass = A2CW(m_proxyPassword);

      if(!WinHttpSetCredentials(m_request
                               ,WINHTTP_AUTH_TARGET_PROXY
                               ,dwSelectedScheme
                               ,user.c_str()
                               ,pass.c_str()
                               ,NULL))
      {
        // Could not set the credentials
        ErrorLog(__FUNCTION__,"Setting HTTP Credentials pre-emptively. Error [%d] %s");
        return false;
      }
      DETAILLOG("Proxy authentication set for user: %s",m_user.GetString());
    }
  }
  else
  {
    // Could not get the authorization scheme
    ErrorLog(__FUNCTION__,"Cannot query authentication schema for proxy answer. Error [%d] %s");
    return false;
  }
  return true;
}

// Sending the body of the message to the server
// Works in 4 different configurations:
// 1) One burst of the m_body member is first priority
// 2) Send in one go from the m_buffer member as one (1) buffer block
// 3) Send multiple buffer parts from the m_buffer member
// 4) Send the indicated file from m_buffer by cycling through the file
//
// Beware: Cannot send blocks of length 0, as the HTTP channel will stall!!
//
void
HTTPClient::SendBodyData()
{
  // If we did have a body, we sent it now, right after the header
  // Try to do it in one (1) burst, to be as optimal as possible
  if(m_body != nullptr)
  {
    if(m_bodyLength)
    {
      // PART 1: SEND OUR DATA IN ONE (1) GO
      // ONLY m_body/m_bodylength IS FILLED IN.
      DWORD dwWritten = 0;
      if (!::WinHttpWriteData(m_request
                             ,m_body
                             ,m_bodyLength
                             ,&dwWritten))
      {
        ErrorLog(__FUNCTION__,"Write body: Data in 1 go. Error [%d] %s");
      }
    }
    DETAILLOG("Write body. Data in 1 go. Size: %d",m_bodyLength);
  }
  else if(m_buffer != nullptr)
  {
    if(m_buffer->GetFileName().IsEmpty())
    {
      uchar* buffer = nullptr;
      size_t length = 0;

      m_buffer->GetBuffer(buffer,length);
      if(buffer)
      {
        DWORD dwWritten = 0;
        if(length)
        {
          // PART 2: SEND In 1 GO FROM FileBUFFER
          // Only 1 GetBufer required
          if (!::WinHttpWriteData(m_request
                                 ,buffer
                                 ,(DWORD)length
                                 ,&dwWritten))
          {
            ErrorLog(__FUNCTION__,"Write body: File buffer. Error [%d] %s");
          }
        }
        DETAILLOG("Write body. File buffer. Size: %d. Written: %d",length,dwWritten);
      }
      else
      {
        // PART 3: SEND ALL BUFFERS FROM FileBuffer
        // OPTIMIZED TO SEND ALL DATA
        int part = 0;
      
        while(m_buffer->GetBufferPart(part,buffer,length))
        {
          if(length)
          {
            DWORD dwWritten = 0;
            if (!::WinHttpWriteData(m_request
                                   ,buffer
                                   ,(DWORD)length
                                   ,&dwWritten))
            {
              CString msg;
              msg.Format("Write body: Buffer part [%d]. Error [%%d] %%s",part + 1);
              ErrorLog(__FUNCTION__,(char*) msg.GetString());
              break;
            }
          }
          DETAILLOG("Write body. Buffer part [%d]. Size: %d",part + 1,length);
          // ((char*)buffer)[length] = 0;
          // TRACELOG((char*)buffer);
          // Next part
          ++part;
        } 
      }
    }
    else
    {
      // PART 4: SEND FROM A FILE
      // FILE MUST BE OPENED TO SEND FROM
      BYTE* buffer = new BYTE[INT_BUFFERSIZE + 1];
      DWORD dwSize = 0;
      DWORD dwRead = 0;

      if(m_buffer->OpenFile() == false)
      {
        m_lastError = ERROR_FILE_NOT_FOUND;
        ERRORLOG("File not found on reading: %s",m_buffer->GetFileName().GetString());
        delete [] buffer;
        return;
      }
      DETAILLOG("File opened for reading: %s",m_buffer->GetFileName().GetString());

      // Get resulting file handle
      HANDLE file = m_buffer->GetFileHandle();

      do 
      {
        if(ReadFile(file,buffer,INT_BUFFERSIZE,&dwRead,NULL))
        {
          // Bookkeeping of bytes read-in
          dwSize += dwRead;

          DWORD dwWritten = 0;
          if(dwRead && !::WinHttpWriteData(m_request
                                          ,buffer
                                          ,dwRead
                                          ,&dwWritten))
          {
            ErrorLog(__FUNCTION__,"Write body: File part. Error [%d] %s");
            break;
          }
          DETAILLOG("Write body. File part. Size: %d -> %d",dwRead,dwWritten);
        }
        else
        {
          // Nothing more to read/write or error
          m_lastError = GetLastError();
          if(m_lastError)
          {
            ErrorLog(__FUNCTION__,"Error while reading from file. File length changed? Error [%d] %s");
          }
          break;
        }
      } 
      while (dwSize < m_bodyLength);

      // Close our file (done or not!)
      m_buffer->CloseFile();
      // Free our writing buffer
      delete [] buffer;
      DETAILLOG(" File closed: %s",m_buffer->GetFileName().GetString());
    }
  }
}

void
HTTPClient::ReceiveResponseData()
{
  // Reset the response
  if(m_response)
  {
    delete [] m_response;
    m_responseLength = 0;
  }

  // Only open file for writing for a get.
  // File buffer on a "PUT" also contains a filename, but this is the file
  // we have written on the send, so do not overwrite the file in the response-phase
  // of the "PUT" HTTP message
  if(m_buffer && !m_buffer->GetFileName().IsEmpty() && (m_verb == "GET"))
  {
    ReceiveResponseDataFile();
  }
  else
  {
    ReceiveResponseDataBuffer();
  }
}

void
HTTPClient::ReceiveResponseDataFile()
{
  DWORD dwSize = 0;

  // Receive data in a file, and close the file afterwards
  // File **MUST** be opened in WRITE modus
  if(m_buffer->OpenFile(false) == false)
  {
    m_lastError = ERROR_PATH_NOT_FOUND;
    ERRORLOG("Cannot open file for write: %s",m_buffer->GetFileName().GetString());
    return;
  }

  // Tell how we receive the data
  DETAILLOG("Receive response data in file: %s",m_buffer->GetFileName().GetString());

  // Get file handle
  HANDLE file = m_buffer->GetFileHandle();

  do 
  {
    if(WinHttpQueryDataAvailable(m_request,&dwSize))
    {
      if(dwSize)
      {
        BYTE* response = new BYTE[dwSize + 1];
        memset(response,0,(dwSize+1) * sizeof(BYTE));
        DWORD dwRead = 0;
        if(WinHttpReadData(m_request,response,dwSize,&dwRead))
        {
          DWORD dwWritten = 0;
          if(!WriteFile(file,response,dwRead,&dwWritten,NULL))
          {
            CString msg;
            msg.Format("Cannot write block to file. Size [%d] Error [%%d] %%s",dwRead);
            ErrorLog(__FUNCTION__,(char*)msg.GetString());
            break;
          }
          else
          {
            m_responseLength += dwWritten;
            DETAILLOG("Received file block. Recieved: %d Written: %d",dwRead,dwWritten);
          }
        }
        delete [] response;
      }
    }
  } 
  while (dwSize > 0);

  // Close the file, flush it to the file system
  m_buffer->CloseFile();
  DETAILLOG("File closed");
}

// We receive our data in the m_response data member
// Done by enlarging the response buffer each time
void
HTTPClient::ReceiveResponseDataBuffer()
{
  DWORD dwSize = 0;

  do
  {
    if(::WinHttpQueryDataAvailable(m_request,&dwSize))
    {
      if(dwSize)
      {
        BYTE* response = new BYTE[dwSize + 1];
        memset(response, 0,dwSize + 1);
        DWORD dwRead = 0;
        if (::WinHttpReadData(m_request,
                              response,
                              dwSize,
                              &dwRead))
        {
          DETAILLOG("Reading response data block. Size: %d",dwRead);
          if(m_response)
          {
            BYTE* newResponse = new BYTE[m_responseLength + dwRead + 1];
            memcpy(newResponse,m_response,m_responseLength);
            memcpy(&newResponse[m_responseLength],response,dwRead);
            delete [] response;
            delete [] m_response;
            m_response = newResponse;
          }
          else
          {
            // Simple record our response
            m_response = response;
          }
          // Bookkeeping
          m_responseLength += dwRead;
        }
        else
        {
          delete [] response;
          ErrorLog(__FUNCTION__,"Error while reading data block. Error [%d] %s");
        }
      }
      else
      {
        // Could be the end of the reading of all blocks
        // if not, show the MS-Windows error
        if(GetLastError())
        {
          ErrorLog(__FUNCTION__,"Reading data block. Error [%d] %s");
        }
      }
    }
  }
  while (dwSize > 0);

  // Handeling the result
  if(m_response)
  {
    m_response[m_responseLength] = 0;
  }
}

void
HTTPClient::ReceivePushEvents()
{
  // Give open signal to event source
  if(m_eventSource->GetReadyState() == CONNECTING)
  {
    m_eventSource->OnOpen(new ServerEvent("open"));
    // Event source should now be in 'OPEN' ready state
  }

  // Receive data from the server stream with push-events
  // Will be one long stream that is kept alive
  do
  {
    DWORD dwSize = 0;
    DWORD dwRead = 0;

    if(::WinHttpQueryDataAvailable(m_request,&dwSize))
    {
      if(dwSize == 0)
      {
        ErrorLog(__FUNCTION__,"Server closed event-stream prematurely. Error [%d] %s");
        break;
      }
      BYTE* response = new BYTE[dwSize + 1];
      if(response == NULL)
      {
        ERRORLOG("Out of memory");
        m_status = HTTP_STATUS_REQUEST_TOO_LARGE;
        return;
      }
      // After Windows2008, chunking is removed, so everything is received
      // in one call to ReadData, without a need to loop
      // 2016: But reappeared on Windows 10!
      if(::WinHttpReadData(m_request,
                           response,
                           dwSize,
                           &dwRead))
      {
        // String ending
        response[dwRead] = 0;

        // Store the read-in data
        if(m_response)
        {
          // Append to a previous response (not parsed!)
          BYTE* newResponse = new BYTE[m_responseLength + dwRead + 1];
          memcpy(newResponse,m_response,m_responseLength);
          memcpy(&newResponse[m_responseLength],response,dwRead);
          delete [] response;
          delete [] m_response;
          m_response = newResponse;
        }
        else
        {
          // Simple record our response
          m_response = response;
        }
        // Bookkeeping
        m_responseLength += dwRead;
        m_response[m_responseLength] = 0;
        DETAILLOG("Reading response data block. Size: %d",dwRead);
      }
      else
      {
        ErrorLog(__FUNCTION__,"Reading data block. Error [%d] %s");
        m_status = HTTP_STATUS_SERVER_ERROR;
        return;
      }

      // Somebody closed me
      switch(m_eventSource->GetReadyState())
      {
        case CLOSED:           m_status = HTTP_STATUS_GONE;
                               return;
        case CLOSED_BY_SERVER: m_status = HTTP_STATUS_NO_CONTENT;
                               return;
      }

      // Receiving SSE event stream. See if we must trace the results
      if(MUSTLOG(HLL_LOGBODY) && m_log)
      {
        m_log->BareStringLog((const char*)m_response,m_responseLength);
        if(MUSTLOG(HLL_TRACEDUMP))
        {
          m_log->AnalysisHex(__FUNCTION__,"SSE",m_response,m_responseLength);
        }
      }

      // Parse to event responses
      m_eventSource->Parse(m_response,m_responseLength);

      // In case of logging, log the highest new event ID
      if(MUSTLOG(HLL_LOGGING) && m_log)
      {
        m_log->AnalysisLog(__FUNCTION__,LogType::LOG_INFO,true,"SSE received. Last ID = %u",m_eventSource->GetLastEventID());
      }

      // Clean the queue if needed and parsed completely
      if(m_responseLength == 0 && m_response)
      {
        delete [] m_response;
        m_response = nullptr;
      }
    }
    else
    {
      // Error in polling HTTP Status
      DWORD er = GetLastError();
      switch(er)
      {
        case ERROR_WINHTTP_CONNECTION_ERROR:          m_status = HTTP_STATUS_SERVICE_UNAVAIL;   break;
        case ERROR_WINHTTP_INCORRECT_HANDLE_STATE:    m_status = HTTP_STATUS_RETRY_WITH;        break;
        case ERROR_WINHTTP_INCORRECT_HANDLE_TYPE:     m_status = HTTP_STATUS_PRECOND_FAILED;    break;
        case ERROR_WINHTTP_INTERNAL_ERROR:            m_status = HTTP_STATUS_NONE_ACCEPTABLE;   break;
        case ERROR_NOT_ENOUGH_MEMORY:                 m_status = HTTP_STATUS_REQUEST_TOO_LARGE; break;
        case ERROR_WINHTTP_TIMEOUT:                   [[fallthrough]];
        case ERROR_WINHTTP_OPERATION_CANCELLED:       [[fallthrough]];
        default:                                      m_status = HTTP_STATUS_SERVICE_UNAVAIL;   break;
      }

      // In case of explicit close of the channel by the server
      if(m_eventSource->GetReadyState() == CLOSED_BY_SERVER)
      {
        m_status = HTTP_STATUS_NO_CONTENT;
        return;
      }

      // Make error event and dispatch it
      ServerEvent* event = new ServerEvent("error");
      event->m_data.Format("OS Error: %lu, HTTP Error: %u",er,m_status);
      ERRORLOG(event->m_data);
      m_eventSource->OnError(event);

      // In case of end-of-channel, try reconnecting
      if(m_status == HTTP_STATUS_SERVICE_UNAVAIL)
      {
        m_eventSource->SetConnecting();
      }

      // End on error situation. Possibly reconnecting to EventSource
      return;
    }
  } 
  while(m_onCloseSeen == false);
}

//////////////////////////////////////////////////////////////////////////
//
// ALL FORMS OF THE SEND COMMAND
//
//////////////////////////////////////////////////////////////////////////

// Send HTTP to an URL
bool
HTTPClient::Send(CString& p_url)
{
  AutoCritSec lock(&m_sendSection);

  // Init logging. Log from here on
  InitLogging();

  if(SetURL(p_url))
  {
    return SendAndRedirect();
  }
  return false;
}

// Send HTTP + body to an URL
bool
HTTPClient::Send(CString& p_url,CString& p_body)
{
  AutoCritSec lock(&m_sendSection);

  // Init logging. Log from here on
  InitLogging();

  if(SetURL(p_url))
  {
    SetBody((void*) p_body.GetString(),p_body.GetLength());
    return SendAndRedirect();
  }
  return false;
}

// Send HTTP + body-buffer to an URL
// WARNING: Do the rest of the plumbing yourself!!
// Otherwise use: Send(HTTPMessage), Send(SOAPMessage) or Send(JSONMessage)
bool 
HTTPClient::Send(CString& p_url,void* p_buffer,unsigned p_length)
{
  AutoCritSec lock(&m_sendSection);

  // Init logging. Log from here on
  InitLogging();

  if(SetURL(p_url))
  {
    // Test for a reconnect
    TestReconnect();

    SetBody(p_buffer,p_length);
    return SendAndRedirect();
  }
  return false;
}

// Send to an URL to GET a file in a buffer
bool
HTTPClient::Send(CString&    p_url
                ,FileBuffer* p_buffer
                ,CString*    p_contentType /*=nullptr*/
                ,Cookies*    p_cookies     /*=nullptr*/
                ,HeaderMap*  p_headers     /*=nullptr*/)
{
  AutoCritSec lock(&m_sendSection);
  bool result = false;

  // Init logging. Log from here on
  InitLogging();

  if(SetURL(p_url) == false)
  {
    return false;
  }
  // Test for a reconnect
  TestReconnect();

  // Most definitely a get
  m_verb = "GET";

  // Reset as far as needed
  m_requestHeaders.clear();
  m_cookies.Clear();

  // Using our parameters (if any)
  if(p_contentType)
  {
    m_contentType = *p_contentType;
  }
  if(p_cookies)
  {
    m_cookies = *p_cookies;
  }
  if(p_headers)
  {
    for(auto& header : *p_headers)
    {
      AddHeader(header.first,header.second);
    }
  }

  // Go get our HTTP file
  result = SendAndRedirect();

  p_buffer->Reset();
  p_buffer->ResetFilename();
  p_buffer->SetBuffer(m_response,m_responseLength);

  return result;
}

// Send HTTPMessage
bool 
HTTPClient::Send(HTTPMessage* p_msg)
{
  AutoCritSec lock(&m_sendSection);
  bool result = false;

  // Init logging. Log from here on
  InitLogging();

  if(!p_msg->GetServer().IsEmpty() &&
     !p_msg->GetAbsolutePath().IsEmpty() &&
      p_msg->GetPort() != 0)
  {
    ReplaceSetting(&m_user,    p_msg->GetUser());
    ReplaceSetting(&m_password,p_msg->GetPassword());

    m_server   = p_msg->GetServer();
    m_port     = p_msg->GetPort();
    m_url      = p_msg->GetAbsolutePath();
    m_secure   = p_msg->GetSecure();
  }
  else
  {
    SetURL(p_msg->GetURL());
  }
  // Test for a reconnect
  TestReconnect();

  // Propagate verb tunneling before requesting the verb itself
  if(m_verbTunneling)
  {
    p_msg->SetVerbTunneling(true);
  }

  // Transfer all headers to the client
  AddMessageHeaders(p_msg);

  // Set the client properties before Send()
  SetVerb(p_msg->GetVerb());
  m_contentType = p_msg->GetContentType();
  m_buffer      = p_msg->GetFileBuffer();
  m_bodyLength  = (DWORD)m_buffer->GetLength();
  m_body        = NULL;
  m_cookies     = p_msg->GetCookies();

  // NOW GO SEND IT
  result = SendAndRedirect();

  // Forget what we did send
  p_msg->Reset();

  // Keep response as new body. Might contain an error!!
  p_msg->SetBody(m_response,m_responseLength);
  p_msg->SetCookies(m_resultCookies);
  // Getting all headers from the answer
  if(m_readAllHeaders)
  {
    for(HeaderMap::iterator it = m_responseHeaders.begin(); it != m_responseHeaders.end(); ++it)
    {
      p_msg->AddHeader(it->first,it->second);
    }
  }
  // Set result
  p_msg->SetStatus(m_status);

  // Reset our input buffer
  m_buffer = NULL;
  m_bodyLength = 0;

  return result;
}

// Send HTTP + SOAP Message
bool 
HTTPClient::Send(SOAPMessage* p_msg)
{
  AutoCritSec lock(&m_sendSection);

  // Init logging. Log from here on
  InitLogging();

  // Try to optimize and not re-parse the complete URL
  if(!p_msg->GetServer().IsEmpty() &&
     !p_msg->GetAbsolutePath().IsEmpty() &&
      p_msg->GetPort() != INTERNET_DEFAULT_HTTP_PORT)
  {
    ReplaceSetting(&m_user,    p_msg->GetUser());
    ReplaceSetting(&m_password,p_msg->GetPassword());

    m_server   = p_msg->GetServer();
    m_port     = p_msg->GetPort();
    m_url      = p_msg->GetAbsolutePath();
    m_secure   = p_msg->GetSecure();

    DETAILLOG("SOAP Message for: http%s//%s:%d%s",m_secure ? "s" : "",m_server.GetString(),m_port,m_url.GetString());
  }
  else
  {
    // Minimum set not filled. Reparse complete URL
    SetURL(p_msg->GetURL());
  }
  // Test for a reconnect
  TestReconnect();

  // Setting members for posting soap-xml
  CString verb("POST");
  SetVerb(verb);
  
  // Use the signing/encryption options
  InitSecurity();
  XMLEncryption security = (XMLEncryption) m_securityLevel;
  CString password = m_enc_password;
  if(security != XMLEncryption::XENC_Plain)
  {
    // Whole client is in security mode
    p_msg->SetSecurityLevel(security);
    p_msg->SetSecurityPassword(m_enc_password);
  }
  else
  {
    // Send one security message only
    security = p_msg->GetSecurityLevel();
    password = p_msg->GetSecurityPassword();
  }

  // Trying to optimize the output formatting to spare bytes
  if(m_soapCompress)
  {
    p_msg->SetCondensed(true);
  }

  // Test sending of a Unicode posting
  // Place soap message on the stack and set it as a body
  CString soap;
  uchar* buffer = nullptr;
  int    length = 0;

  if(m_sendBOM)
  {
    p_msg->SetSendBOM(true);
  }

  if(m_sendUnicode || p_msg->GetSendUnicode())
  {
    p_msg->SetSendUnicode(true);
    soap = p_msg->GetSoapMessage();

    if(TryCreateWideString(soap,"",p_msg->GetSendBOM(),&buffer,length))
    {
      m_contentType = p_msg->GetContentType() + "; charset=utf-16";
      SetBody(buffer,length);
    }
    else
    {
      m_lastError = ERROR_INVALID_PARAMETER;
      return false;
    }
  }
  else
  {
    soap = p_msg->GetSoapMessage();
    m_contentType = p_msg->GetContentType();
    XMLEncoding encoding = p_msg->GetEncoding();

    if(FindCharsetInContentType(m_contentType).IsEmpty())
    {
      // Take care of character encoding
      int acp = -1;
      switch(encoding)
      {
        case XMLEncoding::ENC_Plain:   acp = -1;    break; // Find Active Code Page
        case XMLEncoding::ENC_UTF8:    acp = 65001; break; // See ConvertWideString.cpp
        case XMLEncoding::ENC_UTF16:   acp = 1200;  break; // See ConvertWideString.cpp
        case XMLEncoding::ENC_ISO88591:acp = 28591; break; // See ConvertWideString.cpp
        default:                       break;
      }
      m_contentType.AppendFormat("; charset=%s",CodepageToCharset(acp).GetString());
    }
    SetBody(soap);
  }

  // Transfer all headers to the client
  AddMessageHeaders(p_msg);

  // Create a SOAPAction header value
  CString soapAction(m_soapAction);
  if(soapAction.IsEmpty())
  {
    soapAction += p_msg->GetSoapAction();
    if(soapAction.Find('/') < 0)
    {
      CString namesp = p_msg->GetNamespace();
      if(!namesp.IsEmpty() && namesp.Right(1) != '/')
      {
        namesp += "/";
      }
      soapAction = namesp + soapAction;
    }
  }
  // Apply the SOAPAction header value to the appropriate HTTP header
  if(p_msg->GetSoapVersion() < SoapVersion::SOAP_12) 
  {
    AddHeader("SOAPAction",soapAction);
  }
  else // SOAP 1.2
  {
    if(FindFieldInHTTPHeader(m_contentType,"action").IsEmpty())
    {
      m_contentType.AppendFormat("; action=\"%s\"",soapAction.GetString());
    }
  }

  // Set cookies
  m_cookies = p_msg->GetCookies();
  
  // Put in logfile
  DETAILLOG("Outgoing SOAP message: %s",p_msg->GetSoapAction().GetString());

  // Now go send our XML (Never redirected)
  bool result = Send();

  // Headers from the answer
  CString nosniff     = FindHeader("X-Content-Type-Options");
  CString charset     = FindCharsetInContentType(m_contentType);

  // Process our answer
  p_msg->Reset();
  p_msg->SetStatus(m_status);

  CString answer;
  bool sendBom = false;
  if(charset.Left(6).CompareNoCase("utf-16") == 0)
  {
    // Works for "UTF-16", "UTF-16LE" and "UTF-16BE" as of RFC 2781
    if(TryConvertWideString(m_response,m_responseLength,"",answer,sendBom))
    {
      p_msg->SetSendBOM(sendBom);
    }
    else
    {
      // SET SOAP FAULT
      CString message;
      message.Format("Cannot convert UTF-16 message");
      p_msg->SetFault("Server","Charset",message,"Possibly unknown UNICODE charset, or non-standard machine charset");
      result = false;
      answer.Empty();
    }
  }
  else if(nosniff.CompareNoCase("nosniff"))
  {
    int uni = IS_TEXT_UNICODE_UNICODE_MASK;
    if(IsTextUnicode(m_response,m_responseLength,&uni))
    {
      // Find specific code page and try to convert
      if(!TryConvertWideString(m_response,m_responseLength,"",answer,sendBom))
      {
        // SET SOAP FAULT
        CString message;
        message.Format("Unknown charset from server: %s",charset.GetString());
        p_msg->SetFault("Server","Charset",message,"Possibly unknown UNICODE charset, or non-standard machine charset");
        answer.Empty();
        result = false;
      }
    }
    else
    {
      // Otherwise assume MBCS plaintext in current codepage or UTF-8
      answer = ((const char*)m_response);
    }
  }
  else
  {
    // Sender forces to not sniffing
    // So assume the standard codepages are used (ACP=1252, UTF-8 or ISO-8859-1)
    answer = ((const char*)m_response);
  }

  // Keep response as new body. Might contain an error!!
  p_msg->ParseMessage(answer);

  soapAction = p_msg->GetSoapAction();
  if(!soapAction.IsEmpty())
  {
    DETAILLOG("Incoming SOAP answer: %s",soapAction.GetString());
  }
  // Keep cookies
  p_msg->SetCookies(m_resultCookies);

  // Getting all headers from the answer
  if(m_readAllHeaders)
  {
    for(HeaderMap::iterator it = m_responseHeaders.begin(); it != m_responseHeaders.end(); ++it)
    {
      p_msg->AddHeader(it->first,it->second);
    }
  }

  // Only check the answer in case of a correct HTTP answer
  if(result)
  {
    if(security > XMLEncryption::XENC_Plain)
    {
      CheckAnswerSecurity(p_msg,answer,security,password);
    }
  }
  else
  {
    CString response;
    if(m_response)
    {
      response = m_response;
      response += ". ";
    }
    if(m_lastError >= WINHTTP_ERROR_BASE && m_lastError <= WINHTTP_ERROR_LAST)
    {
      response.AppendFormat("%s. Error number [%d]",GetHTTPErrorText(m_lastError).GetString(),m_lastError);
    }
    // In case of an error
    p_msg->Reset();
    p_msg->SetFault("Client","Send error","HTTPClient send result",response);
  }

  // Freeing Unicode UTF-16 buffer
  if(buffer)
  {
    delete [] buffer;
  }

  return result;
}

// Send HTTP + JSON Message
bool
HTTPClient::Send(JSONMessage* p_msg)
{
  AutoCritSec lock(&m_sendSection);
  bool result = false;

  // Init logging. Log from here on
  InitLogging();

  if(!p_msg->GetServer().IsEmpty() &&
     !p_msg->GetAbsolutePath().IsEmpty() &&
      p_msg->GetPort() != 0)
  {
    ReplaceSetting(&m_user,p_msg->GetUser());
    ReplaceSetting(&m_password,p_msg->GetPassword());

    m_server = p_msg->GetServer();
    m_port   = p_msg->GetPort();
    m_url    = p_msg->GetAbsolutePath();
    m_secure = p_msg->GetSecure();
  }
  else
  {
    SetURL(p_msg->GetURL());
  }
  // Test for a reconnect
  TestReconnect();

  // Set the client properties before Send()
  CString json;
  uchar* buffer = nullptr;
  int    length = 0;

  if(m_sendBOM)
  {
    p_msg->SetSendBOM(true);
  }

  // Preparing the body of the transmission
  if(m_sendUnicode || 
     p_msg->GetSendUnicode() || 
     p_msg->GetEncoding() == JsonEncoding::JENC_UTF16)
  {
    // SEND AS 16 BITS UTF MESSAGE
    p_msg->SetSendUnicode(true);
    json = p_msg->GetJsonMessage(JsonEncoding::JENC_Plain);
    if(TryCreateWideString(json,"",p_msg->GetSendBOM(),&buffer,length))
    {
      m_contentType = p_msg->GetContentType() + "; charset=utf-16";
      SetBody(buffer,length);
    }
    else
    {
      m_lastError = ERROR_INVALID_PARAMETER;
      return false;
    }
  }
  else
  {
    // SEND IN OTHER ENCODINGS
    m_contentType = p_msg->GetContentType();
    JsonEncoding encoding = p_msg->GetEncoding();
    json = p_msg->GetJsonMessageWithBOM(encoding);

    // Take care of character encoding
    int acp = -1;
    switch(encoding)
    {
      case JsonEncoding::JENC_Plain:   acp =    -1; break; // Find Active Code Page
      case JsonEncoding::JENC_UTF8:    acp = 65001; break; // See ConvertWideString.cpp
      case JsonEncoding::JENC_UTF16:   acp =  1200; break; // See ConvertWideString.cpp
      case JsonEncoding::JENC_ISO88591:acp = 28591; break; // See ConvertWideString.cpp
      default:                         break;
    }
    m_contentType.AppendFormat("; charset=%s",CodepageToCharset(acp).GetString());
    SetBody(json);
  }

  if(m_verbTunneling)
  {
    p_msg->SetVerbTunneling(true);
  }

  // Transfer all headers to the client
  AddMessageHeaders(p_msg);

  // Setting verb en cookies
  SetVerb(p_msg->GetVerb());
  m_cookies = p_msg->GetCookies();

  // Put in logfile
  DETAILLOG("Outgoing JSON message");
  
  // NOW GO SEND IT (Never redirected)
  result = Send();

  ProcessJSONResult(p_msg,result);

  // Free Unicode UTF-16 buffer
  if(buffer)
  {
    delete [] buffer;
  }
  return result;
}

void
HTTPClient::ProcessJSONResult(JSONMessage* p_msg,bool& p_result)
{
  // Headers from the answer
  CString nosniff = FindHeader("X-Content-Type-Options");
  CString charset = FindCharsetInContentType(m_contentType);

  // Process our answer, Forget what we did send
  p_msg->Reset();

  // Prepare the answer
  CString answer;
  JsonEncoding encoding = JsonEncoding::JENC_UTF8;
  bool doBom = false;
  int uni = IS_TEXT_UNICODE_UNICODE_MASK;  // Intel/AMD processors

  // Do the following casse
  if(charset.Left(6).CompareNoCase("utf-16") == 0)
  {
    // Works for "UTF-16", "UTF-16LE" and "UTF-16BE" as of RFC 2781
    if(TryConvertWideString(m_response,m_responseLength,"",answer,doBom))
    {
      p_msg->SetSendBOM(doBom);
      encoding = JsonEncoding::JENC_Plain;
    }
    else
    {
      // SET ERROR STATE
      CString message;
      message.Format("Cannot convert UTF-16 message");
      p_msg->SetLastError(message);
      p_msg->SetErrorstate(true);
      p_result = false;
      answer.Empty();
    }
  }
  else if(nosniff.CompareNoCase("nosniff") &&
          IsTextUnicode(m_response,m_responseLength,&uni))
  {
    // Find specific code page and try to convert
    if(TryConvertWideString(m_response,m_responseLength,"",answer,doBom))
    {
      p_msg->SetSendBOM(doBom);
      encoding = JsonEncoding::JENC_Plain;
    }
    else
    {
      // SET ERROR STATE
      CString message;
      message.Format("Unknown charset from server: %s",charset.GetString());
      p_msg->SetLastError(message);
      p_msg->SetErrorstate(true);
      answer.Empty();
      p_result = false;
    }
  }
  else
  {
    // Sender forces to not sniffing
    // So assume the standard codepages are used (ACP=1252, UTF-8 or ISO-8859-1)
    CString currentCP = CodepageToCharset(GetACP());

    // Other special cases of the charset
    if(charset.Left(currentCP.GetLength()).CompareNoCase(currentCP) == 0)
    {
      encoding = JsonEncoding::JENC_Plain;
    }
    else if(charset.Left(10).CompareNoCase("iso-8859-1") == 0)
    {
      encoding = JsonEncoding::JENC_ISO88591;
    }
    // Answer is the raw response
    answer = ((const char*)m_response);
  }

  // Keep response as new body. Might contain an error!!
  DETAILLOG("Incoming JSON answer");
  p_msg->ParseMessage(answer,encoding);

  // Keep cookies
  p_msg->SetCookies(m_resultCookies);
  // Getting all headers from the answer
  if(m_readAllHeaders)
  {
    for(HeaderMap::iterator it = m_responseHeaders.begin(); it != m_responseHeaders.end(); ++it)
    {
      p_msg->AddHeader(it->first,it->second);
    }
  }
  // Reset our input buffer
  m_buffer     = NULL;
  m_bodyLength = 0;
}

// Translate SOAP to JSON, send/receive and translate back
bool
HTTPClient::SendAsJSON(SOAPMessage* p_msg)
{
  // Try to get a JSON url + parameters
  CString url = p_msg->GetJSON_URL();
  if(url.IsEmpty())
  {
    // Probably one or more complex objects
    return false;
  }

  AutoCritSec lock(&m_sendSection);
  bool result = false;

  // Init logging. Log from here on
  InitLogging();

  if(SetURL(url) == false)
  {
    return false;
  }
  // Test for a reconnect
  TestReconnect();

  // Most definitely a get
  m_verb = "GET";
  // Most definitely we want a JSON back
  m_contentType = "application/json";
  m_bodyLength  = 0;
  m_body        = (void*) "";

  // Reset as far as needed
  m_requestHeaders.clear();
  m_cookies.Clear();

  // Using our parameters (if any)
  AddMessageHeaders(p_msg);

  // Set cookies
  m_cookies = p_msg->GetCookies();

  // Put in logfile
  DETAILLOG("Outgoing SOAP->JSON: GET %s",url.GetString());

  // Go get our JSON response
  result = SendAndRedirect();

  // Reset our original request
  p_msg->Reset();
  if(result)
  {
    // Translate our result back to JSON
    JSONMessage json;
    ProcessJSONResult(&json,result);

    // Translate our JSON back to SOAP
    *p_msg = json;
  }
  else
  {
    CString status;
    status.Format("Webserver returned status: %d",m_status);
    CString response = (LPCTSTR)m_response;
    p_msg->SetFault("JSON-GET","Server",status,response);
  }
  return result;
}

EventSource*
HTTPClient::CreateEventSource(CString p_url)
{
  if(m_eventSource)
  {
    delete m_eventSource;
  }
  return (m_eventSource = new EventSource(this,p_url));
}

// Start server push-event stream on this url
bool
HTTPClient::StartEventStream(CString& p_url)
{
  AutoCritSec lock(&m_sendSection);

  if(m_eventSource == nullptr)
  {
    ERRORLOG(__FUNCTION__,"Cannot start a new event stream. No eventsource defined!");
    return false;
  }

  if(SetURL(p_url))
  {
    // Now turn on our eventstreaming thread
    return StartEventStreamingThread();
  }
  return false;
}

bool
HTTPClient::DoRedirectionAfterSend()
{
  CString redirURL = GetResultHeader(WINHTTP_QUERY_LOCATION, 0);
  if(!redirURL.IsEmpty())
  {
    m_url = redirURL;
    return true;
  }
  return false;
}

// Send message and redirect if so requested by the server
bool
HTTPClient::SendAndRedirect()
{
  bool result = false;
  bool redirecting = false;
  CString redirURL;

  do 
  {
    // Do send our message
    result = Send();
    // And test for status in the 300 range
    if(result && (m_status >= HTTP_STATUS_AMBIGUOUS && m_status < HTTP_STATUS_BAD_REQUEST))
    {
      switch(m_status)
      {
        case HTTP_STATUS_AMBIGUOUS:         if(m_verb.Compare("HEAD") != 0)
                                            {
                                              redirecting = DoRedirectionAfterSend();
                                            }
                                            break;
        case HTTP_STATUS_MOVED:             // Fall through
        case HTTP_STATUS_REDIRECT:          // Fall through
        case HTTP_STATUS_REDIRECT_KEEP_VERB:if(m_verb == "GET" || m_verb == "HEAD")
                                            {
                                              redirecting = DoRedirectionAfterSend();
                                            }
                                            break;
        case HTTP_STATUS_REDIRECT_METHOD:   redirecting = DoRedirectionAfterSend();
                                            if(redirecting)
                                            {
                                              m_verb = "GET";
                                            }
                                            break;
        case HTTP_STATUS_NOT_MODIFIED:      // if-modified-since was not modified
                                            // NEVER REDIRECTED!!
                                            break;
        case HTTP_STATUS_USE_PROXY:         redirecting = DoRedirectionAfterSend();
                                            if(!redirecting)
                                            {
                                              // USE_PROXY **MUST** have the "Location" header
                                              result = false;
                                            }
                                            break;
        case HTTP_STATUS_PERMANENT_REDIRECT:redirecting = DoRedirectionAfterSend();
                                            break;
      }
    }
    else
    {
      redirecting = false;
    }
  } 
  while(redirecting);

  return result;
}

// Our primary function: send a message
bool
HTTPClient::Send()
{
  AutoCritSec lock(&m_sendSection);

  bool retValue            = false;
  bool getReponseSucceed   = false;
  unsigned int iRetryTimes = 0;
  // Use standard UNICODE conversion
  USES_CONVERSION;

  // Reset error status
  m_lastError = 0;

  // Check for a session
  if(m_session == NULL)
  {
    if(Initialize() == false)
    {
      return false;
    }
    // We now have a session!
  }

  // Establish the 'real' server/port by proxy replacement
  // In the case we must do any proxy replacement
  wstring server(A2CW(m_server));
  int port(m_port);

  switch(m_useProxy)
  {
    case ProxyType::PROXY_IEPROXY:  // Use explicit proxy from settings 
    case ProxyType::PROXY_MYPROXY:  // Use explicit proxy from Marlin.config
                                    if(!m_proxy.IsEmpty())
                                    {
                                      CrackedURL url(m_proxy);
                                      server = A2CW(url.m_host);
                                      port   = url.m_port;
                                    }
                                    break;
    case ProxyType::PROXY_AUTOPROXY:// Try URL. Fallback is on the session level
    case ProxyType::PROXY_NOPROXY:  // No proxy. Do nothing
                                    break;
  }

  // Before we try anything: let's log the connection in full
  if(MUSTLOG(HLL_LOGGING))
  {
    LogTheSend(server,port);
  }

  // Check on localhost for development systems.
  // IIS has a problem to service 'localhost' directly and can lead
  // to system 'hangs' for up to 8 seconds.
  if(_wcsicmp(server.c_str(),L"localhost") == 0)
  {
    server = L"127.0.0.1";
  }

  // First: do the connection to the server:port pair
  // Connection could have been opened by earlier Send
  if(m_connect == NULL)
  {
    // No try the connect
    m_connect = WinHttpConnect(m_session,server.c_str(),(INTERNET_PORT)port,0);
    if(m_connect == NULL)
    {
      CString msg;
      msg.Format("Cannot connect to [%s:%d] Error [%%d] %%s",W2CA(server.c_str()),port);
      ErrorLog(__FUNCTION__,msg);
      return false;
    }
    m_lastServer = m_server;
    m_lastPort   = m_port;
    m_lastSecure = m_secure;
  }

  // Get our flags:
  DWORD flags = WINHTTP_FLAG_REFRESH;
  if(m_secure)
  {
    flags |= WINHTTP_FLAG_SECURE;
    DETAILLOG("Making secure HTTPS connection");
  }

  // Make a request header to be send
  wstring verb = A2CW(m_verb);
  wstring url  = A2CW(m_url);
  m_request = WinHttpOpenRequest(m_connect
                                ,verb.c_str()
                                ,url.c_str()
                                ,NULL
                                ,WINHTTP_NO_REFERER
                                ,WINHTTP_DEFAULT_ACCEPT_TYPES
                                ,flags);
  if(m_request == NULL)
  {
    CString msg;
    msg.Format("Cannot open a HTTP request for a [%s %s] Error [%%d] %%s",m_verb.GetString(),m_url.GetString());
    ErrorLog(__FUNCTION__,msg);
    return false;
  }
  // Prepare a fresh response buffer
  if(m_response)
  {
    delete [] m_response;
    m_response = nullptr;
  }

  // Nothing gotten yet
  // Reset our response length, will otherwise lead to wrong answers
  // in the Unicode detection of answers. See RFC 491138
  m_responseLength = 0;

  // Attaching proxy info
  AddProxyInfo();
  // Standard proxy authorization (if any)
  AddProxyAuthorization();
  // Add authorization up-front before the call
  AddPreEmptiveAuthorization();
  AddOAuth2authorization();
  // Always add a host header
  AddHostHeader();
  // Add our recorded cookies
  AddCookieHeaders();
  // Set security headers
  AddSecurityOptions();
  // Add Cross Origin headers
  AddCORSHeaders();
  // Add all recorded external header lines (e.g. SOAPAction)
  AddExtraHeaders();
  // Add WebSocket preparation
  AddWebSocketUpgrade();
  // Always add OUR Content-length header
  AddLengthHeader();
  // Now flush all headers to the WinHTTP client
  FlushAllHeaders();

  // If always using a client certificate, set it upfront
  if(m_certPreset)
  {
    SetClientCertificate(m_request);
  }
  // If set: initialize single sign on
  if(m_sso)
  {
    InitializeSingleSignOn();
  }

  // Request status is now known, trace it?
  if(MUSTLOG(HLL_LOGBODY))
  {
    TraceTheSend();
  }

  // variables for the main loop
  DWORD lastStatus = 0;
  bool  sendRequestSucceed = false;
  unsigned retries = m_retries;

  // Retry for several times if fails.
  while(!getReponseSucceed && iRetryTimes++ <= retries)
  {
    // Reset the status fields
    m_status    = 0;
    m_lastError = 0;

    // Clearing the response headers
    m_responseHeaders.clear();
    m_contentType.Empty();

    // Send the header only
    // But add body length to set the "content-length:" header
    sendRequestSucceed = false;
    if(WinHttpSendRequest(m_request,
                          WINHTTP_NO_ADDITIONAL_HEADERS,
                          0,
                          WINHTTP_NO_REQUEST_DATA,
                          0,
                          m_bodyLength,
                          NULL))
    {
      // Successfully sent our request
      sendRequestSucceed = true;
      // If we have a body or a file, send it, to complete the send
      SendBodyData();

      // Now waiting for a response from the server
      BOOL receivedResponse = WinHttpReceiveResponse(m_request,NULL);
      if(receivedResponse == FALSE)
      {
        // Get last error and log it
        ErrorLog(__FUNCTION__,"Response from HTTP Server: [%d] %s");

        if(m_lastError == ERROR_WINHTTP_CLIENT_AUTH_CERT_NEEDED)
        {
          if(SetClientCertificate(m_request))
          {
            --iRetryTimes;
            continue;
          }
        }
      }
      if(receivedResponse || m_lastError == ERROR_INTERNET_FORCE_RETRY)
      {
        // Get the result status code (200 = OK)
        DWORD dwSize = sizeof(DWORD);
        DWORD statusCode = 0;
        bool ntlm3Step = false;

        // Try to get the status
        if(m_lastError == ERROR_INTERNET_FORCE_RETRY)
        {
          // Happens on timeout of the NTLM Caching mechanism.
          // Hence the 3-step authentication. Try one more time!!
          ntlm3Step  = true;
          m_status   = HTTP_STATUS_DENIED;
          lastStatus = 0; // One more try
        }
        else
        {
          if (::WinHttpQueryHeaders(m_request,
                                    WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                                    NULL, // WINHTTP_HEADER_NAME_BY_INDEX,
                                    &statusCode,
                                    &dwSize,
                                    WINHTTP_NO_HEADER_INDEX))
          {
            m_status = statusCode;
          }
          else
          {
            // Huh? No HTTP status code found
            ErrorLog(__FUNCTION__,"No HTTP status code found in the answer! Error [%d] %s");
          }
        }

        // What to do with the status code
        switch(m_status)
        {
          default:                [[fallthrough]];
          case HTTP_STATUS_OK:    getReponseSucceed = true;
                                  // Get the content type header
                                  m_contentType = ReadHeaderField(WINHTTP_QUERY_CONTENT_TYPE);
                                  break;
          case HTTP_STATUS_DENIED:if(lastStatus == HTTP_STATUS_DENIED)
                                  {
                                    // Cannot do this twice!
                                    getReponseSucceed = true;
                                    iRetryTimes = retries + 1;
                                  }
                                  else
                                  {
                                    // Record our status
                                    lastStatus = m_status;

                                    // Retry one extra round
                                    --iRetryTimes;

                                    // Add authentication headers
                                    ResetOAuth2Session();
                                    if(AddOAuth2authorization()     == false &&
                                       AddAuthentication(ntlm3Step) == false )
                                    {
                                      getReponseSucceed = true;
                                      iRetryTimes = retries + 1;
                                    }
                                  }
                                  break;
          case HTTP_STATUS_PROXY_AUTH_REQ:
                                  if(lastStatus == HTTP_STATUS_PROXY_AUTH_REQ)
                                  {
                                    getReponseSucceed = true;
                                    iRetryTimes = retries + 1;
                                  }
                                  else
                                  {
                                    // Record our status
                                    lastStatus = m_status;

                                    // Retry one extra round
                                    --iRetryTimes;

                                    // Add authentication headers
                                    if(AddProxyAuthentication() == false)
                                    {
                                      getReponseSucceed = true;
                                      iRetryTimes = retries + 1;
                                    }
                                  }
                                  break;
          case HTTP_STATUS_SWITCH_PROTOCOLS:
                                  // The protocol-upgrader sits in a tight loop around 'Send()'
                                  // And waits for confirmation to do the upgrade of the m_request handle
                                  if(m_websocket)
                                  {
                                    ReadAllResponseHeaders();
                                    return true;
                                  }
                                  break;
        }

        // LOOP THROUGH ALL RESPONSE PARTS
        if(m_status == HTTP_STATUS_OK || getReponseSucceed == true)
        {
          // Getting our results
          ProcessResultCookies();

          // Optionally getting all response headers
          if(m_readAllHeaders || MUSTLOG(HLL_LOGBODY))
          {
            ReadAllResponseHeaders();
          }

          if(m_pushEvents)
          {
            // Handle incoming event-stream
            ReceivePushEvents();
          }
          else
          {
            // One time receive data
            ReceiveResponseData();
          }

          // Break inner and outer loop
          if(m_status >= HTTP_STATUS_OK &&
             m_status <  HTTP_STATUS_BAD_REQUEST)
          {
            getReponseSucceed = true;
            retValue = true;
          }
        }
      }
    }
    else
    {
      // Already did the 'Connect' but no 'SendRequest' action
      // Getting the HTTP result status
      ErrorLog(__FUNCTION__,"Cannot send a HTTP request. Error [%d] %s");

      if(m_lastError == ERROR_WINHTTP_CLIENT_AUTH_CERT_NEEDED)
      {
        if(SetClientCertificate(m_request))
        {
          --iRetryTimes;
          continue;
        }
      }
    }
  } // while retry

  // Log error after retries are up..
  if(!sendRequestSucceed)
  {
    ERRORLOG("Failed to send a [%s] to: %s",m_verb.GetString(),m_url.GetString());
  }
  DETAILLOG("Returned HTTP status: %d:%s",m_status,GetHTTPStatusText(m_status));

  // See if we must do 'gzip' decompression
  CString compress = ReadHeaderField(WINHTTP_QUERY_CONTENT_ENCODING);
  if(compress.CompareNoCase("gzip") == 0)
  {
    // DECompress in-memory with ZLib from a 'gzip' HTTP buffer 
    size_t before = m_responseLength;
    std::vector<uint8_t> out_data;
    if(gzip_decompress_memory(m_response,m_responseLength,out_data))
    {
      m_responseLength = (unsigned int) out_data.size();

      delete [] m_response;
      m_response = new BYTE[m_responseLength + 1];
      for(size_t ind = 0; ind < m_responseLength; ++ind)
      {
        m_response[ind] = out_data[ind];
      }
      m_response[m_responseLength] = 0;
      DETAILLOG("Unzipped gzip content from [%lu] to [%lu] (%d %%)",before,m_responseLength,(100 * before)/m_responseLength);

      // Replace the already read response header for the length of the content
      HeaderMap::iterator it = m_responseHeaders.find("Content-length");
      if(it != m_responseHeaders.end())
      {
        CString newlen;
        newlen.Format("%d",m_responseLength);
        it->second = newlen;
      }

      // Remove content-encoding
      it = m_responseHeaders.find("Content-Encoding");
      if(it != m_responseHeaders.end())
      {
        m_responseHeaders.erase(it);
      }
    }
  }

  // Response gotten: trace it?
  if(MUSTLOG(HLL_LOGBODY) && m_response)
  {
    TraceTheAnswer();
  }

  // Close our request
  WinHttpCloseHandle(m_request);
  m_request = NULL;

  // CORS testing?
  if(retValue && !m_corsOrigin.IsEmpty())
  {
    retValue = CheckCORSAnswer();
  }
  // Return result
  return retValue;
}

// Logging the essential sending parameters of the HTTP call
void
HTTPClient::LogTheSend(wstring& p_server,int p_port)
{
  // See if we have anything to do
  if(m_log == nullptr || m_logLevel < HLL_LOGGING)
  {
    return;
  }

  USES_CONVERSION;
  CString proxy;
  CString server = (CString) CW2A(p_server.c_str());


  // Find secure call
  CString secure = m_secure && m_scheme.Right(1) != 's' ? "s" : "";

  // Log in full, do the raw logging call directly
  m_log->AnalysisLog("HTTPClient::Send",LogType::LOG_INFO,true
                    ,"%s %s%s://%s:%d%s HTTP/1.1"
                    ,m_verb.GetString()
                    ,m_scheme.GetString()
                    ,secure.GetString()
                    ,m_server.GetString()
                    ,m_port
                    ,m_url.GetString());
  // Calculate the proxy (if any)
  if(p_port != m_port || server.CompareNoCase(m_server))
  {
    m_log->AnalysisLog("HTTPClient::Send",LogType::LOG_INFO,true
                      ,"Connect through proxy [%s:%d]",server.GetString(),p_port);
  }
}

void
HTTPClient::TraceTheSend()
{
  USES_CONVERSION;

  // See if we have anything to do
  if(m_log == nullptr || m_logLevel < HLL_LOGBODY)
  {
    return;
  }

  // Before showing the request we show the configuration of the HTTP channel
  if(MUSTLOG(HLL_TRACE))
  {
    m_trace->Trace((char*)"BEFORE SENDING",m_session,m_request);
  }

  // THE HTTP HEADER LINE

  CString header;
  m_log->AnalysisLog(__FUNCTION__,LogType::LOG_TRACE,true,"Outgoing");
  header.Format("%s %s://%s:%d%s HTTP/1.1"
                ,m_verb.GetString()
                ,m_scheme.GetString()
                ,m_server.GetString()
                ,m_port
                ,m_url.GetString());
  m_log->BareStringLog(header, header.GetLength());

  // TRACE ALL INTERNAL STATES

  header.Format("INTERNAL -> Host: %s",m_server.GetString());
  m_log->BareStringLog(header.GetString(),header.GetLength());
  header.Format("INTERNAL -> Content-Length: %lu",m_bodyLength);
  m_log->BareStringLog(header.GetString(),header.GetLength());
  header.Format("INTERNAL -> Content-Type: %s",m_contentType.GetString());
  m_log->BareStringLog(header.GetString(),header.GetLength());
  if (m_httpCompression)
  {
    header = "INTERNAL -> Accept-Encoding: gzip";
    m_log->BareStringLog(header.GetString(),header.GetLength());
  }
  if (m_soapAction.GetLength())
  {
    header.Format("INTERNAL -> SOAPAction: %s",m_soapAction.GetString());
    m_log->BareStringLog(header.GetString(),header.GetLength());
  }
  if (m_terminalServices)
  {
    DWORD session = 0;
    DWORD pid = GetCurrentProcessId();
    if (ProcessIdToSessionId(pid, &session))
    {
      header.Format("INTERNAL -> RemoteDesktop: %d",session);
      m_log->BareStringLog(header.GetString(),header.GetLength());
    }
  }
  if(m_cookies.GetSize())
  {
    header = "INTERNAL -> Cookie: " + m_cookies.GetCookieText();
    m_log->BareStringLog(header.GetString(),header.GetLength());
  }
  // TRACE ALL HEADERS

  // Trace all other extra headers, including CORS headers
  for(auto& head : m_requestHeaders)
  {
    header  = "HTTP Header -> ";
    header += head.first + ":" + head.second;
    m_log->BareStringLog(header.GetString(),header.GetLength());
  }

  // Empty line between headers and body
  m_log->BareStringLog("",0);

  // THE BODY

  // Trace all parts of the body
  if(m_body)
  {
    m_log->BareStringLog((const char*)m_body,m_bodyLength);
    if (MUSTLOG(HLL_TRACEDUMP))
    {
      m_log->AnalysisHex("TraceHTTP","Outgoing",m_body,m_bodyLength);
    }
  }
  else if(m_buffer && m_buffer->GetLength())
  {
    if(!m_buffer->GetFileName().IsEmpty())
    {
      DETAILLOG("SENDING FILE: %s",m_buffer->GetFileName().GetString());
    }
    else
    {
      int    part   = 0;
      uchar* buffer = nullptr;
      size_t length = 0;

      while(m_buffer->GetBufferPart(part++,buffer,length))
      {
        m_log->BareStringLog((const char*)buffer,(int)length);
      }
      if(MUSTLOG(HLL_TRACEDUMP))
      {
        part = 0;
        while(m_buffer->GetBufferPart(part++,buffer,length))
        {
          m_log->AnalysisHex("TraceHTTP","Outgoing",buffer,(unsigned)length);
        }
      }
    }
  }
  else
  {
    header = "<No body sent>";
    m_log->BareStringLog(header.GetString(),header.GetLength());
  }
}

void
HTTPClient::TraceTheAnswer()
{
  // We must have a logfile
  if(m_log == nullptr)
  {
    return;
  }

  // Now here comes the answer
  m_log->AnalysisLog(__FUNCTION__,LogType::LOG_TRACE,false,"Incoming response");

  CString header;
  header.Format("HTTP/1.1 %d %s",m_status,GetHTTPStatusText(m_status));
  m_log->BareStringLog(header.GetString(),header.GetLength());

  // All answer headers
  for(auto& head : m_responseHeaders)
  {
    header.Format("%s: %s",head.first.GetString(),head.second.GetString());
    m_log->BareStringLog(header,header.GetLength());
  }

  // Empty line between headers and body
  m_log->BareStringLog("",0);

  // Answer body or none received
  if(m_response)
  {
    m_log->BareStringLog((const char*)m_response,(int)m_responseLength);
    if(MUSTLOG(HLL_TRACEDUMP))
    {
      m_log->AnalysisHex("TraceHTTP","Incoming",m_response,m_responseLength);
    }
  }
  else
  {
    header = "<No body received>";
    m_log->BareStringLog(header.GetString(),header.GetLength());
  }
}

bool
HTTPClient::CheckCORSAnswer()
{
  bool result = true;

  if(!m_corsMethod.IsEmpty())
  {
    CString methods = FindHeader("Access-Control-Allow-Methods");
    if(methods.Find(m_corsMethod) < 0)
    {
      // We may NOT use this method on this address
      result = false;
      m_status = HTTP_STATUS_BAD_METHOD;
      ERRORLOG("CORS method not allowed for this site: %s",m_corsMethod.GetString());
    }
  }

  if(!m_corsHeaders.IsEmpty())
  {
    CString allowed = FindHeader("Access-Control-Allow-Headers");
    if(allowed.Find(m_corsHeaders) < 0)
    {
      // We may NOT use method and headers
      result = false;
      ERRORLOG("CORS headers not allowed for this site: %s",m_corsHeaders.GetString());
    }
  }

  // Reset one-shot checks for OPTIONS
  m_corsMethod.Empty();
  m_corsHeaders.Empty();

  return result;
}

// Setting a client certificate on the request handle
// See: https://msdn.microsoft.com/en-us/library/windows/desktop/aa384076(v=vs.85).aspx
bool
HTTPClient::SetClientCertificate(HINTERNET p_request)
{
  bool result = false;

  // See if w have work to do
  if(m_certName.IsEmpty())
  {
    return false;
  }

  // Open the store of the certificate
  HCERTSTORE hStore = CertOpenSystemStore(0,m_certStore);
  if(hStore)
  {
    // Finding our certificate by name
    PCCERT_CONTEXT pCertificate = NULL;
    pCertificate = CertFindCertificateInStore(hStore,
                                              X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                                              0,
                                              CERT_FIND_SUBJECT_STR_A,
                                              (LPVOID)m_certName.GetString(), //Subject string in the certificate.
                                              NULL);
    if(pCertificate)
    {
      // If found, adding the certificate to the request handle
      if(WinHttpSetOption(p_request,
                          WINHTTP_OPTION_CLIENT_CERT_CONTEXT,
                          (LPVOID)pCertificate,
                          sizeof(CERT_CONTEXT)))
      {
        DETAILLOG("Client certificate set [%s:%s]",m_certStore.GetString(),m_certName.GetString());
        // Now GO and retry the request
        result = true;
      }
      CertFreeCertificateContext(pCertificate);
    }
    // Caveat:
    // Certificate store may NOT be closed with the 'force' flag
    // as the WinHTTP library will still need it to work with it
    // So always provide the '0' parameter flag in dwFlags
    CertCloseStore(hStore,0);
  }
  else
  {
    ERRORLOG("Cannot open the certificate store: %s",m_certStore.GetString());
  }
  if(result == false)
  {
    ErrorLog(__FUNCTION__,"Cannot set the client certificate for the request. Error [%d] %s");
  }
  return result;
}

CString
HTTPClient::ReadHeaderField(int p_header)
{
  CString field;

  // Get the needed size
  DWORD dwSize = 0;
  WinHttpQueryHeaders(m_request,
                      p_header,
                      WINHTTP_HEADER_NAME_BY_INDEX,
                      WINHTTP_NO_OUTPUT_BUFFER,
                      &dwSize,
                      WINHTTP_NO_HEADER_INDEX);

  if(GetLastError() == ERROR_INSUFFICIENT_BUFFER)
  {
    WCHAR* buffer = new WCHAR[dwSize + 1];
    if(buffer)
    {
      if (::WinHttpQueryHeaders(m_request,
                                p_header,
                                WINHTTP_HEADER_NAME_BY_INDEX,
                                buffer,
                                &dwSize,
                                WINHTTP_NO_HEADER_INDEX) == FALSE)
      {
        ErrorLog(__FUNCTION__,"Cannot read a HTTP header field. Error [%d] %s");
      }
      else
      {
        USES_CONVERSION;
        field = CW2A(buffer);
      }
      delete [] buffer;
    }
  }
  else
  {
    // No HTTP content type not in the answer
    // Can be normal with special 409 or 20x answers
  }
  return field;
}

// Get all response headers from server
void
HTTPClient::ReadAllResponseHeaders()
{
  // Clean out previous response headers
  m_responseHeaders.clear();

  DWORD dwSize = 0;
  WinHttpQueryHeaders(m_request
                     ,WINHTTP_QUERY_RAW_HEADERS_CRLF
                     ,WINHTTP_HEADER_NAME_BY_INDEX
                     ,WINHTTP_NO_OUTPUT_BUFFER
                     ,&dwSize,
                      WINHTTP_NO_HEADER_INDEX);

  if(GetLastError() == ERROR_INSUFFICIENT_BUFFER)
  {
    WCHAR* buffer = new WCHAR[dwSize];
    if(buffer)
    {
      if(::WinHttpQueryHeaders(m_request
                              ,WINHTTP_QUERY_RAW_HEADERS_CRLF
                              ,WINHTTP_HEADER_NAME_BY_INDEX
                              ,buffer
                              ,&dwSize
                              ,WINHTTP_NO_HEADER_INDEX) == FALSE)
      {
        ErrorLog(__FUNCTION__,"Cannot read all-raw-headers. Error [%d] %s");
      }
      else
      {
        USES_CONVERSION;
        CString all = (CString) CW2A(buffer);
        m_responseHeaders.clear();

        int pos = all.Find("\r\n");
        while(pos >= 0)
        {
          // Getting 1 header from all-headers
          CString name,value;
          CString header = all.Left(pos);
          all = all.Mid(pos+2);

          // Splitting header in name,value
          int namepos = header.Find(':');
          if(namepos > 0)
          {
            CString hname   = header.Left(namepos).Trim();
            CString hvalue  = header.Mid(namepos + 1).Trim();
            m_responseHeaders.insert(std::make_pair(hname,hvalue));
          }
          // Find next
          pos = all.Find("\r\n");
        }
      }
      delete[] buffer;
    }
  }
  else
  {
    ErrorLog(__FUNCTION__,"Cannot find the all-headers size for the query-headers operation. Error [%d] %s");
  }
}

CString  
HTTPClient::FindHeader(CString p_header)
{
  // Case insensitive find
  HeaderMap::iterator it = m_responseHeaders.find(p_header);
  if(it != m_responseHeaders.end())
  {
    return it->second;
  }
  return "";
}

// Transfer all headers to the client before calling
void
HTTPClient::AddMessageHeaders(HTTPMessage* p_message)
{
  // Clear all headers of the client
  m_requestHeaders.clear();

  // Re-align message VERB if necessary through extra header
  if(p_message->GetVerbTunneling())
  {
    p_message->UseVerbTunneling();
  }

  // Copy headers in different format
  CString header;
  HeaderMap* allheaders = p_message->GetHeaderMap();
  for(HeaderMap::iterator it = allheaders->begin(); it != allheaders->end(); ++it)
  {
    AddHeader(it->first,it->second);
  }
  // Implement if-modified-since method
  if(p_message->GetUseIfModified())
  {
    AddHeader("if-modified-since",p_message->HTTPTimeFormat(p_message->GetSystemTime()));
  }
}

void
HTTPClient::AddMessageHeaders(SOAPMessage* p_message)
{
  // Clear all headers of the client
  m_requestHeaders.clear();

  // Copy headers in different format
  CString header;
  HeaderMap* allheaders = p_message->GetHeaderMap();
  for(HeaderMap::iterator it = allheaders->begin(); it != allheaders->end(); ++it)
  {
    AddHeader(it->first,it->second);
  }
}

void
HTTPClient::AddMessageHeaders(JSONMessage* p_message)
{
  // Clear all headers of the client
  m_requestHeaders.clear();

  // Replacing message VERB if necessary through extra header
  if(p_message->GetVerbTunneling())
  {
    p_message->UseVerbTunneling();
  }

  // Copy headers in different format
  CString header;
  HeaderMap* allheaders = p_message->GetHeaderMap();
  for(HeaderMap::iterator it = allheaders->begin(); it != allheaders->end(); ++it)
  {
    AddHeader(it->first,it->second);
  }
}

// Set and check a client certificate by store/thumbprint pair
bool
HTTPClient::SetClientCertificateThumbprint(CString p_store,CString p_thumbprint)
{
  bool result = false;

  // Open the store of the certificate
  HCERTSTORE hStore = CertOpenSystemStore(0,p_store);
  if(hStore)
  {
    // Store exists, remember it
    m_certStore = p_store;

    // Finding our certificate by hash-blob of the thumbprint
    PCCERT_CONTEXT certificate = NULL;
    BYTE thumb[40];
    CRYPT_HASH_BLOB blob;
    blob.cbData = 0;
    blob.pbData = thumb;
    
    // Encode the thumbprint
    HTTPCertificate::EncodeThumbprint(p_thumbprint,&blob,40);

    // Go look in the store for the certificate
    certificate = CertFindCertificateInStore(hStore
                                            ,X509_ASN_ENCODING | PKCS_7_ASN_ENCODING
                                            ,0
                                            ,CERT_FIND_SHA1_HASH
                                            ,(LPVOID)&blob
                                            ,NULL);
    if(certificate)
    {
      HTTPCertificate cert(certificate->pbCertEncoded,certificate->cbCertEncoded);
      m_certName = cert.GetSubject();
      if(!m_certName.IsEmpty())
      {
        result = true;
        DETAILLOG("Client certificate set [%s:%s]",m_certStore.GetString(),m_certName.GetString());
      }
      else
      {
        ErrorLog(__FUNCTION__,"Certificate found, but no subject name found in the certificate [%d] %s");
      }
      CertFreeCertificateContext(certificate);
    }
    else
    {
      ErrorLog(__FUNCTION__,"Requested certificate not found [%d] %s");
    }
    // Caveat: Never use the 'force' flag, as WinHTTP still needs the store
    CertCloseStore(hStore,0);
  }
  else
  {
    ErrorLog(__FUNCTION__,"Requested certificate store not found [%d] %s");
  }
  return result;
}

void 
HTTPClient::SetCORSOrigin(CString p_origin)
{
  // Remember CORS origin
  m_corsOrigin = p_origin;
  // Must read all response headers for CORS answers
  if(!m_corsOrigin.IsEmpty())
  {
    m_readAllHeaders = true;
  }
}

// Simply note the Cross Origin Resource Sharing options
// for the "OPTIONS" call
bool 
HTTPClient::SetCORSPreFlight(CString p_method,CString p_headers)
{
  if(!m_corsOrigin.IsEmpty())
  {
    m_corsMethod  = p_method;
    m_corsHeaders = p_headers;
    return true;
  }
  return false;
}

//////////////////////////////////////////////////////////////////////////
//
// WS-Security answer
//
//////////////////////////////////////////////////////////////////////////

void
HTTPClient::CheckAnswerSecurity(SOAPMessage* p_msg,CString p_answer,XMLEncryption p_security,CString p_password)
{
  if(p_security != p_msg->GetSecurityLevel())
  {
    p_msg->SetFault("Server","WS-Security","Same security level"
                   ,"Client and server must have the same security level (signing, body- or message encryption).");
    return;
  }

  switch(p_security)
  {
    case XMLEncryption::XENC_Signing: CheckBodySigning(p_password,p_msg);          break;
    case XMLEncryption::XENC_Body:    DecodeBodyEncryption(p_password,p_msg,p_answer); break;
    case XMLEncryption::XENC_Message: DecodeMesgEncryption(p_password,p_msg,p_answer); break;
    case XMLEncryption::XENC_Plain:   [[fallthrough]];
    default:                          break;
  }
}

void
HTTPClient::CheckBodySigning(CString p_password,SOAPMessage* p_message)
{
  // Restore password
  p_message->SetSecurityPassword(p_password);

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
      CString digest = sign.Digest(signedXML,p_password);

      if(signature.CompareNoCase(digest) == 0)
      {
        // Signature checks out OK.
        return;
      }
    }
  }
  p_message->SetFault("Server","WS-Security","Incorrect signing",
                      "SOAP message should have a signed body. Signing is incorrect or missing.");
}

void
HTTPClient::DecodeBodyEncryption(CString p_password,SOAPMessage* p_msg,CString p_answer)
{
  CString crypt = p_msg->GetSecurityPassword();
  p_msg->SetSecurityPassword(p_password);

  // Decrypt
  Crypto crypting;
  CString newBody = crypting.Decryption(crypt,p_password);

  int beginPos = p_answer.Find("Body>");
  int endPos = p_answer.Find("Body>",beginPos + 2);
  if(beginPos > 0 && endPos > 0 && endPos > beginPos)
  {
    // Finding begin of the body before the namespace
    for(int ind = beginPos; ind >= 0; --ind)
    {
      if(p_answer.GetAt(ind) == '<')
      {
        beginPos = ind;
        break;
      }
    }
    // Finding begin of the end-of-body before the namespace
    int extra = 6;
    for(int ind = endPos; ind >= 0; --ind)
    {
      if(p_answer.GetAt(ind) == '<')
      {
        endPos = ind;
        break;
      }
      ++extra;
    }

    // Now re-parse the message
    CString message = p_answer.Left(beginPos) + newBody + p_answer.Mid(endPos + extra);
    p_msg->Reset();
    p_msg->ParseMessage(message);
    p_msg->SetSecurityLevel(XMLEncryption::XENC_Plain);

    if(p_msg->GetInternalError() == XmlError::XE_NoError)
    {
      // OK, Message found
      return;
    }
  }
  // Not the error in a SOAP Fault.
  p_msg->SetFault("Server"
                 ,"Configuration"
                 ,"No encryption"
                 ,"SOAP message should have a encrypted body. Encryption is incorrect or missing.");
}

void
HTTPClient::DecodeMesgEncryption(CString p_password,SOAPMessage* p_msg,CString p_answer)
{
  CString crypt = p_msg->GetSecurityPassword();
  // Restore password for return answer
  p_msg->SetSecurityPassword(p_password);

  // Decrypt
  Crypto crypting;
  CString newBody = crypting.Decryption(crypt,p_password);

  int beginPos = p_answer.Find("Envelope>");
  int endPos = p_answer.Find("Envelope>",beginPos + 2);
  if(beginPos > 0 && endPos > 0 && endPos > beginPos)
  {
    // Finding begin of the envelope before the namespace
    for(int ind = beginPos; ind >= 0; --ind)
    {
      if(p_answer.GetAt(ind) == '<')
      {
        beginPos = ind;
        break;
      }
    }
    // Finding begin of the end-of-envelope before the namespace
    int extra = 10;
    for(int ind = endPos; ind >= 0; --ind)
    {
      if(p_answer.GetAt(ind) == '<')
      {
        endPos = ind;
        break;
      }
      ++extra;
    }

    // Reparse the envelope
    CString message = p_answer.Left(beginPos) + newBody + p_answer.Mid(endPos + extra);
    p_msg->Reset();
    p_msg->ParseMessage(message);
    p_msg->SetSecurityLevel(XMLEncryption::XENC_Plain);

    if(p_msg->GetInternalError() == XmlError::XE_NoError)
    {
      // OK, Message found
      return;
    }
  }
  p_msg->SetFault("Server"
                 ,"WS-Security"
                 ,"No encryption"
                 ,"SOAP message should have a encrypted message. Encryption is incorrect or missing.");
}

//////////////////////////////////////////////////////////////////////////
//
// THE HTTP QUEUE
//
//////////////////////////////////////////////////////////////////////////

// Add an HTTP message in the queue
void 
HTTPClient::AddToQueue(HTTPMessage* p_entry)
{
  AutoCritSec lock(&m_queueSection);
    
  MsgBuf msg;
  msg.m_type = MsgType::HTPC_HTTP;
  msg.m_message.m_httpMessage = p_entry;

  // Put at the back of the queue
  m_queue.push_back(msg);

  // Start worker thread
  StartQueue();

  // Tell to the output port that we have work to do!
  SetEvent(m_queueEvent);
}

// Add a SOAP message to the queue
void
HTTPClient::AddToQueue(SOAPMessage* p_message)
{
  AutoCritSec lock(&m_queueSection);

  MsgBuf msg;
  msg.m_type = MsgType::HTPC_SOAP;
  msg.m_message.m_soapMessage = p_message;

  // Put at the back of the queue
  m_queue.push_back(msg);

  // Start worker thread
  StartQueue();

  // Tell to the output port that we have work to do!
  SetEvent(m_queueEvent);
}

// Add a JSON message to the queue
void
HTTPClient::AddToQueue(JSONMessage* p_message)
{
  AutoCritSec lock(&m_queueSection);

  MsgBuf msg;
  msg.m_type = MsgType::HTPC_JSON;
  msg.m_message.m_jsonMessage = p_message;

  // Put at the back of the queue
  m_queue.push_back(msg);

  // Start worker thread
  StartQueue();

  // Tell to the output port that we have work to do!
  SetEvent(m_queueEvent);
}

// Get next message from the queue
bool
HTTPClient::GetFromQueue(HTTPMessage** p_entry1,
                         SOAPMessage** p_entry2,
                         JSONMessage** p_entry3)
{
  AutoCritSec lock(&m_queueSection);

  // Reset the entry
  *p_entry1 = NULL;
  *p_entry2 = NULL;

  // Optimize: nothing in the queue
  if(m_queue.empty())
  {
    return false;
  }

  // Get from the beginning of the queue
  HttpQueue::iterator it = m_queue.begin();
  MsgBuf& msg = *it;
  switch(msg.m_type)
  {
    case MsgType::HTPC_HTTP: *p_entry1 = msg.m_message.m_httpMessage; break;
    case MsgType::HTPC_SOAP: *p_entry2 = msg.m_message.m_soapMessage; break;
    case MsgType::HTPC_JSON: *p_entry3 = msg.m_message.m_jsonMessage; break;
  }
  m_queue.erase(it);
  return true;
}

void
HTTPClient::CreateQueueEvent()
{
  m_queueEvent = ::CreateEvent(NULL,FALSE,FALSE,NULL);
  if(m_lastError > 0)
  {
    ErrorLog(__FUNCTION__,"Error while creating an OS-Event for the HTTPClient queue. Error [%d] %s");
  }
  if(m_queueEvent == NULL)
  {
    ErrorLog(__FUNCTION__,"OS-Event for the HTTPClient queue could not be made. Error [%d] %s");
  }
}

unsigned int 
__stdcall StartingTheQueue(void* pParam)
{
  HTTPClient* client = reinterpret_cast<HTTPClient*> (pParam);
  client->QueueRunning();
  return 0;
}

void
HTTPClient::StartQueue()
{
  if(m_queueEvent == NULL)
  {
    CreateQueueEvent();
  }
  if(m_queueThread == NULL || m_running == false)
  {
    // Basis thread of the InOutPort
    unsigned int threadID = 0;
    if((m_queueThread = (HANDLE) _beginthreadex(NULL, 0,StartingTheQueue,(void *)(this),0,&threadID)) == INVALID_HANDLE_VALUE)
    {
      m_queueThread = NULL;
      threadID = 0;
      ErrorLog(__FUNCTION__,"Cannot start a thread for the HTTPClient queue. Error [%d] %s");
    }
    else
    {
      DETAILLOG("Started thread with ID [%d] for HTTPClient queue",threadID);
    }
  }
}

void
HTTPClient::QueueRunning()
{
  // Installing our SEH to exception translator
  _set_se_translator(SeTranslator);

  HTTPMessage* message1 = NULL;
  SOAPMessage* message2 = NULL;
  JSONMessage* message3 = NULL;

  // Start the counter
  m_counter.Start();

  // Set the queue to running
  m_running = true;

  do 
  {
    // Do the entire queue
    if(GetFromQueue(&message1,&message2,&message3))
    {
      // Fire and forget. No return status processed
      if(message1)
      {
        ProcessQueueMessage(message1);
        message1 = NULL;
      }
      else if(message2)
      {
        ProcessQueueMessage(message2);
        message2 = NULL;
      }
      else if(message3)
      {
        ProcessQueueMessage(message3);
        message3 = NULL;
      }
    }
    else
    {
      // Wait for a maximum amount of time
      m_counter.Stop();
      DWORD result = WaitForSingleObject(m_queueEvent,m_queueRetention);
      m_counter.Start();
      if(result != WAIT_OBJECT_0)
      {
        // Stop immediately
        m_running = false;
        // one of these
        if(result != WAIT_ABANDONED && result != WAIT_TIMEOUT && result != WAIT_IO_COMPLETION)
        {
          ERRORLOG("Unexpected result in waiting for queue event: %d",result);
        }
      }
    }
  }
  while(m_running);

  // Stop the counter
  m_counter.Stop();

  // Last thing the queue is doing
  m_queueThread = NULL;
}

void
HTTPClient::ProcessQueueMessage(HTTPMessage* p_message)
{
  if(Send(p_message))
  {
    DETAILLOG("Did send queued HTTPMessage to: %s",p_message->GetCrackedURL().SafeURL().GetString());
  }
  else
  {
    ERRORLOG("Error while sending queued HTTPMessage to: %s",p_message->GetCrackedURL().SafeURL().GetString());
  }
  // End of the line: Remove the queue message
  delete p_message;
}

void
HTTPClient::ProcessQueueMessage(SOAPMessage* p_message)
{
  if(Send(p_message))
  {
    DETAILLOG("Did send queued SOAPMessage [%s] to: %s",p_message->GetSoapAction().GetString(),CrackedURL(p_message->GetURL()).SafeURL().GetString());
  }
  else
  {
    ERRORLOG("Error while sending queued SOAPMessage [%s] to: %s", p_message->GetSoapAction().GetString(),CrackedURL(p_message->GetURL()).SafeURL().GetString());
  }
  // End of the line: Remove the queue message
  delete p_message;
}

void
HTTPClient::ProcessQueueMessage(JSONMessage* p_message)
{
  if(Send(p_message))
  {
    DETAILLOG("Did send queued JSONMessage to: %s",p_message->GetCrackedURL().SafeURL().GetString());
  }
  else
  {
    ERRORLOG("Error while sending queued JSONMessage to: %s", p_message->GetCrackedURL().SafeURL().GetString());
  }
  // End of the line: Remove the queue message
  delete p_message;
}


//////////////////////////////////////////////////////////////////////////
//
// EVENT SOURCE THREAD
//
//////////////////////////////////////////////////////////////////////////

unsigned int __stdcall StartingTheEventThread(void* p_context)
{
  HTTPClient* client = reinterpret_cast<HTTPClient*>(p_context);
  client->EventThreadRunning();
  return 0;
}

// Start a thread for the streaming server-push event interface
bool
HTTPClient::StartEventStreamingThread()
{
  if(m_queueThread == NULL || m_running == false)
  {
    // Thread for the client queue
    unsigned int threadID = 0;
    if((m_queueThread = (HANDLE)_beginthreadex(NULL,0,StartingTheEventThread,(void *)(this),0,&threadID)) == INVALID_HANDLE_VALUE)
    {
      m_queueThread = NULL;
      threadID = 0;
      ErrorLog(__FUNCTION__,"Cannot start a thread for an event-streaming client.");
    }
    else
    {
      DETAILLOG("Thread started with threadID [%d] for SSE event stream.",threadID);
      return true;
    }
  }
  return false;
}

// Main loop of the event-interface
void
HTTPClient::EventThreadRunning()
{
  // Installing our SEH to exception translator
  _set_se_translator(SeTranslator);

  // Tell we are running
  m_running = true;
  // We are an event-stream handler
  m_pushEvents = true;
  // Should handle content of this type
  m_contentType = "text/event-stream";
  // Should never be cached - make a pass-through
  AddHeader("cache-control: no-cache");
  // Accepts text-event streams
  AddHeader("accept: text/event-stream");

  // Add channel security if so set
  if(!m_eventSource->GetCookieName().IsEmpty())
  {
    CString value;
    value.Format("%s=%s"
                 ,m_eventSource->GetCookieName().GetString()
                 ,m_eventSource->GetCookieValue().GetString());
    AddHeader("Cookie",value);
  }

  do
  {
    // Reset the stream status
    m_onCloseSeen = false;

    // Set to connecting state for HTTP
    m_eventSource->SetConnecting();

    DETAILLOG("Starting the HTTP event-stream");

    // Simply send a HTTP GET for this URL (Never redirected)
    Send();

    // If coming out of loop and OnClose is seen
    if(m_onCloseSeen)
    {
      break;
    }

    // Wait some more for reconnection to to take place
    if(CONTINUE_STREAM(m_status))
    {
      ULONG reconnect = m_eventSource->GetReconnectionTime();
      DETAILLOG("Waiting for reconnection: %d ms",reconnect);
      Sleep(reconnect);
    }
  }
  while(CONTINUE_STREAM(m_status));

  // Closing the thread
  m_queueThread = NULL;
  m_running = false;
}

// OnClose has been passed from EventSource
void
HTTPClient::OnCloseSeen()
{
  // Status of the event stream
  m_onCloseSeen = true;

  // Closing our HTTP connection by closing our request
  WinHttpCloseHandle(m_request);
  m_request = NULL;

  DETAILLOG("OnClose event seen: closed the HTTP event stream");
};

//////////////////////////////////////////////////////////////////////////
//
// STOPPING
//
//////////////////////////////////////////////////////////////////////////

// Stopping the client queue
void
HTTPClient::StopClient()
{
  // Only stop client if in initialized state
  if(!m_initialized)
  {
    return;
  }
  DETAILLOG("Stopping the HTTPClient");
  bool stopping = false;

  // Cleaning out the event source
  if(m_eventSource)
  {
    DETAILLOG("Stopping the EventSource");
    if(m_request)
    {
      ERRORLOG("Stopping of the HTTP event-source channel");
      OnCloseSeen();
    }
    for (int i = 0; i < 10; ++i)
    {
      Sleep(100);
      if(m_queueThread == 0)
      {
        break;
      }
    }
    if(m_queueThread)
    {
      // Only way to cancel from NTDLL.DLL on the HTTP Stack
      // Do not complain about "TermnateThread". It is the only way. We know!
      TerminateThread(m_queueThread,0);
      m_queueThread = NULL;
      m_running = false;
      ERRORLOG("Forced stopping of the event-source listner");
    }
    delete m_eventSource;
    m_eventSource = nullptr;
    DETAILLOG("Stopped the push-events EventSource");
    stopping = true;
  }
  if(m_queueThread && m_queueEvent)
  {
    // Stop the queue by resetting the thread
    DETAILLOG("Stopping the send queue");
    m_running = false;
    SetEvent(m_queueEvent);

    // Wait for a maximum of 30 seconds for the queue to drain
    // All queue operations should have been ended
    for(int ind = 0; ind < CLOSING_INTERVALS; ++ind)
    {
      Sleep(CLOSING_WAITTIME);
      // Wait until the mainloop ends
      if(m_queueThread == NULL)
      {
        break;
      }
    }
    // IF not kill the queue thread
    // Do not complain about "TermnateThread". It is the only way. We know!
    if(m_queueThread)
    {
      TerminateThread(m_queueThread,3);
      ErrorLog(__FUNCTION__,"Killed the HTTPClient queue thread. Error [%d] %s");
    }
    // Now free the event
    CloseHandle(m_queueEvent);
    m_queueEvent = NULL;
    DETAILLOG("Closed the client HTTP queue");
    stopping = true;
  }
  // We are now stopped. Reset also
  if(stopping)
  {
    Reset();
  }
}

bool 
HTTPClient::GetIsRunning()
{
  return (m_queueThread != NULL);
}

//////////////////////////////////////////////////////////////////////////
//
// Lock for sending (setting parameters and sending)
// This is so you can do the following:
// 1) Acquire a lock on the client
// 2) Setting parameters like headers/cookies/verb etc
// 3) Calling Send()
// 4) Getting the status and the results
// 5) Release the lock on the client
//
//////////////////////////////////////////////////////////////////////////

void
HTTPClient::LockClient()
{
  EnterCriticalSection(&m_sendSection);
}

void
HTTPClient::UnlockClient()
{
  LeaveCriticalSection(&m_sendSection);
}

