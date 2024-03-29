/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: WebConfigServer.cpp
//
// Marlin Component: Internet server/client
// 
// Copyright (c) 2014-2024 ir. W.E. Huisman
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
#include "HTTPManager.h"
#include "WebConfigServer.h"
#include "MarlinConfig.h"
#include "MapDialog.h"
#include "FileDialog.h"
#include "HTTPClient.h"
#include "EventStream.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// WebConfigDlg dialog

IMPLEMENT_DYNAMIC(WebConfigServer, CDialogEx)

WebConfigServer::WebConfigServer(bool p_iis,CWnd* pParent /*=NULL*/)
                :CDialog(WebConfigServer::IDD, pParent)
                ,m_iis(p_iis)
                ,m_port(80)
                ,m_minThreads(0)
                ,m_maxThreads(0)
                ,m_stackSize(0)
{
  m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

  m_useWebroot        = false;
  m_useBaseURL        = false;
  m_useProtocol       = false;
  m_useBinding        = false;
  m_usePort           = false;
  m_useBacklog        = false;
  m_useTunneling      = false;
  m_useMinThreads     = false;
  m_useMaxThreads     = false;
  m_useStacksize      = false;
  m_useServerUnicode  = false;
  m_useGzip           = false;
  m_useStreamingLimit = false;
  m_useCompressLimit  = false;
  m_useThrotteling    = false;
  m_useCORS           = false;
  m_useKeepalive      = false;
  m_useRetrytime      = false;
  m_useCookieMaxAge   = false;

  // SERVER OVERRIDES
  m_port            = 0;
  m_backlogQueue    = 0;
  m_tunneling       = false;
  m_minThreads      = 0;
  m_maxThreads      = 0;
  m_stackSize       = 0;
  m_cookieExpires   = 0;
  m_serverUnicode   = false;
  m_gzip            = false;
  m_cors            = false;
  m_streamingLimit  = STREAMING_LIMIT;
  m_compressLimit   = COMPRESS_LIMIT;
  m_throtteling     = false;
  m_keepalive       = DEFAULT_EVENT_KEEPALIVE;
  m_retrytime       = DEFAULT_EVENT_RETRYTIME;
}

WebConfigServer::~WebConfigServer()
{
}

void WebConfigServer::DoDataExchange(CDataExchange* pDX)
{
  CDialog::DoDataExchange(pDX);
  // USING FIELDS
  DDX_Control(pDX,IDC_USE_WEBROOT,        m_buttonUseWebroot);
  DDX_Control(pDX,IDC_USE_URL,            m_buttonUseBaseURL);
  DDX_Control(pDX,IDC_USE_PROTOCOL,       m_buttonUseProtocol);
  DDX_Control(pDX,IDC_USE_BINDING,        m_buttonUseBinding);
  DDX_Control(pDX,IDC_USE_PORT,           m_buttonUsePort);
  DDX_Control(pDX,IDC_USE_BACKLOG,        m_buttonUseBacklog);
  DDX_Control(pDX,IDC_USE_VERBTUNNEL,     m_buttonUseTunneling);
  DDX_Control(pDX,IDC_USE_MIN_THREADS,    m_buttonUseMinThreads);
  DDX_Control(pDX,IDC_USE_MAX_THREADS,    m_buttonUseMaxThreads);
  DDX_Control(pDX,IDC_USE_STACKSIZE,      m_buttonUseStacksize);
  DDX_Control(pDX,IDC_USE_SERVUNI,        m_buttonUseServerUnicode);
  DDX_Control(pDX,IDC_USE_GZIP,           m_buttonUseGzip);
  DDX_Control(pDX,IDC_USE_STREAM,         m_buttonUseStreamingLimit);
  DDX_Control(pDX,IDC_USE_COMPR,          m_buttonUseCompressLimit);
  DDX_Control(pDX,IDC_USE_THROTTLE,       m_buttonUseThrotteling);
  DDX_Control(pDX,IDC_USE_KEEPALIVE,      m_buttonUseKeepalive);
  DDX_Control(pDX,IDC_USE_RETRYTIME,      m_buttonUseRetrytime);

  // SERVER OVERRIDES
  DDX_Text   (pDX,IDC_WEBROOT,        m_webroot);
  DDX_Text   (pDX,IDC_BASE_URL,       m_baseURL);
  DDX_Control(pDX,IDC_PROTOCOL,       m_comboProtocol);
  DDX_Control(pDX,IDC_BINDING,        m_comboBinding);
  DDX_Control(pDX,IDC_STACKSIZE,      m_comboStack);
  DDX_Text   (pDX,IDC_PORT,           m_port);
  DDX_Text   (pDX,IDC_BACKLOG,        m_backlogQueue);
  DDX_Control(pDX,IDC_VERBTUNNEL,     m_buttonTunneling);
  DDX_Text   (pDX,IDC_MIN_THREADS,    m_minThreads);
  DDX_Text   (pDX,IDC_MAX_THREADS,    m_maxThreads);
  DDX_Text   (pDX,IDC_STACKSIZE,      m_stackSize);
  DDX_Control(pDX,IDC_SERVUNI,        m_buttonServerUnicode);
  DDX_Control(pDX,IDC_GZIP,           m_buttonGzip);
  DDX_Text   (pDX,IDC_STREAM_LIMIT,   m_streamingLimit);
  DDX_Text   (pDX,IDC_COMP_LIMIT,     m_compressLimit);
  DDX_Control(pDX,IDC_THROTTLE,       m_buttonThrotteling);
  DDX_Text   (pDX,IDC_KALIVE,         m_keepalive);
  DDX_Text   (pDX,IDC_RETRYTIME,      m_retrytime);

  DDV_MinMaxInt(pDX,m_keepalive,EVENT_KEEPALIVE_MIN,EVENT_KEEPALIVE_MAX);
  DDV_MinMaxInt(pDX,m_retrytime,EVENT_RETRYTIME_MIN,EVENT_RETRYTIME_MAX);

  if(pDX->m_bSaveAndValidate == FALSE)
  {
    CWnd* w = nullptr;
    
    w = GetDlgItem(IDC_WEBROOT);        w->EnableWindow(m_useWebroot);
    w = GetDlgItem(IDC_BUTT_WEBROOT);   w->EnableWindow(m_useWebroot);
    w = GetDlgItem(IDC_BASE_URL);       w->EnableWindow(m_useBaseURL);
    w = GetDlgItem(IDC_PORT);           w->EnableWindow(m_usePort);
    w = GetDlgItem(IDC_BACKLOG);        w->EnableWindow(m_useBacklog);
    w = GetDlgItem(IDC_MIN_THREADS);    w->EnableWindow(m_useMinThreads);
    w = GetDlgItem(IDC_MAX_THREADS);    w->EnableWindow(m_useMaxThreads);
    w = GetDlgItem(IDC_STREAM_LIMIT);   w->EnableWindow(m_useStreamingLimit);
    w = GetDlgItem(IDC_COMP_LIMIT);     w->EnableWindow(m_useCompressLimit);
    w = GetDlgItem(IDC_KALIVE);         w->EnableWindow(m_useKeepalive);
    w = GetDlgItem(IDC_RETRYTIME);      w->EnableWindow(m_useRetrytime);

    m_comboProtocol      .EnableWindow(m_useProtocol);
    m_comboBinding       .EnableWindow(m_useBinding);
    m_comboStack         .EnableWindow(m_useStacksize);
    m_buttonTunneling    .EnableWindow(m_useTunneling);
    m_buttonServerUnicode.EnableWindow(m_useServerUnicode);
    m_buttonGzip         .EnableWindow(m_useGzip);
    m_buttonThrotteling  .EnableWindow(m_useThrotteling);
  }
}

BEGIN_MESSAGE_MAP(WebConfigServer, CDialog)
  // SERVER OVERRIDES
  ON_EN_CHANGE    (IDC_WEBROOT,       &WebConfigServer::OnEnChangeWebroot)
  ON_BN_CLICKED   (IDC_BUTT_WEBROOT,  &WebConfigServer::OnBnClickedButtWebroot)
  ON_EN_CHANGE    (IDC_BASE_URL,      &WebConfigServer::OnEnChangeBaseUrl)
  ON_CBN_SELCHANGE(IDC_PROTOCOL,      &WebConfigServer::OnCbnSelchangeProtocol)
  ON_CBN_SELCHANGE(IDC_BINDING,       &WebConfigServer::OnCbnSelchangeBinding)
  ON_EN_CHANGE    (IDC_PORT,          &WebConfigServer::OnEnChangePort)
  ON_EN_CHANGE    (IDC_BACKLOG,       &WebConfigServer::OnEnChangeBacklogQueue)
  ON_BN_CLICKED   (IDC_VERBTUNNEL,    &WebConfigServer::OnBnClickedTunneling)
  ON_BN_CLICKED   (IDC_HEADERS,       &WebConfigServer::OnBnClickedHeaders)
  ON_EN_CHANGE    (IDC_MIN_THREADS,   &WebConfigServer::OnEnChangeMinThreads)
  ON_EN_CHANGE    (IDC_MAX_THREADS,   &WebConfigServer::OnEnChangeMaxThreads)
  ON_CBN_SELCHANGE(IDC_STACKSIZE,     &WebConfigServer::OnCbnSelchangeStacksize)
  ON_CBN_KILLFOCUS(IDC_STACKSIZE,     &WebConfigServer::OnEnChangeStacksize)
  ON_BN_CLICKED   (IDC_SERVUNI,       &WebConfigServer::OnBnClickedServerUnicode)
  ON_BN_CLICKED   (IDC_GZIP,          &WebConfigServer::OnBnClickedGzip)
  ON_EN_CHANGE    (IDC_STREAM_LIMIT,  &WebConfigServer::OnEnChangeStreamingLimit)
  ON_EN_CHANGE    (IDC_COMP_LIMIT,    &WebConfigServer::OnEnChangeCompressLimit)
  ON_BN_CLICKED   (IDC_THROTTLE,      &WebConfigServer::OnBnClickedThrotteling)
  ON_EN_KILLFOCUS (IDC_KALIVE,        &WebConfigServer::OnEnChangeKeepalive)
  ON_EN_KILLFOCUS (IDC_RETRYTIME,     &WebConfigServer::OnEnChangeRetrytime)
  ON_BN_CLICKED   (IDC_SETCOOKIE,     &WebConfigServer::OnBnClickedSetCookie)

  // USING BUTTONS
  ON_BN_CLICKED(IDC_USE_WEBROOT,      &WebConfigServer::OnBnClickedUseWebroot)
  ON_BN_CLICKED(IDC_USE_URL,          &WebConfigServer::OnBnClickedUseUrl)
  ON_BN_CLICKED(IDC_USE_PROTOCOL,     &WebConfigServer::OnBnClickedUseProtocol)
  ON_BN_CLICKED(IDC_USE_BINDING,      &WebConfigServer::OnBnClickedUseBinding)
  ON_BN_CLICKED(IDC_USE_PORT,         &WebConfigServer::OnBnClickedUsePort)
  ON_BN_CLICKED(IDC_USE_BACKLOG,      &WebConfigServer::OnBnClickedUseBacklog)
  ON_BN_CLICKED(IDC_USE_VERBTUNNEL,   &WebConfigServer::OnBnClickedUseTunneling)
  ON_BN_CLICKED(IDC_USE_MIN_THREADS,  &WebConfigServer::OnBnClickedUseMinThreads)
  ON_BN_CLICKED(IDC_USE_MAX_THREADS,  &WebConfigServer::OnBnClickedUseMaxThreads)
  ON_BN_CLICKED(IDC_USE_STACKSIZE,    &WebConfigServer::OnBnClickedUseStacksize)
  ON_BN_CLICKED(IDC_USE_SERVUNI,      &WebConfigServer::OnBnClickedUseServerUnicode)
  ON_BN_CLICKED(IDC_USE_GZIP,         &WebConfigServer::OnBnClickedUseGzip)
  ON_BN_CLICKED(IDC_USE_STREAM,       &WebConfigServer::OnBnClickedUseStreamingLimit)
  ON_BN_CLICKED(IDC_USE_COMPR,        &WebConfigServer::OnBnClickedUseCompressLimit)
  ON_BN_CLICKED(IDC_USE_THROTTLE,     &WebConfigServer::OnBnClickedUseThrotteling)
  ON_BN_CLICKED(IDC_USE_KEEPALIVE,    &WebConfigServer::OnBnClickedUseKeepalive)
  ON_BN_CLICKED(IDC_USE_RETRYTIME,    &WebConfigServer::OnBnClickedUseRetrytime)
END_MESSAGE_MAP()

BOOL
WebConfigServer::OnInitDialog()
{
  CDialog::OnInitDialog();

  // Set the icon for this dialog.  The framework does this automatically
  //  when the application's main window is not a dialog
  SetIcon(m_hIcon, TRUE);			// Set big icon
  SetIcon(m_hIcon, FALSE);		// Set small icon

  InitComboboxes();
  InitIIS();

  UpdateData(FALSE);
  return TRUE;
}

void 
WebConfigServer::SetSiteConfig(XString p_urlPrefix,XString p_fileName)
{
  m_url            = p_urlPrefix;
  m_siteConfigFile = p_fileName;
}

void
WebConfigServer::InitComboboxes()
{
  // HTTP/HTTPS protocol
  m_comboProtocol.AddString(_T("http"));
  m_comboProtocol.AddString(_T("https"));

  // Port binding: strong, named, address, full, weak
  m_comboBinding.AddString(_T(""));
  m_comboBinding.AddString(_T("Strong (+)"));
  m_comboBinding.AddString(_T("Short name"));
  m_comboBinding.AddString(_T("IP Address"));
  m_comboBinding.AddString(_T("Full DNS Name"));
  m_comboBinding.AddString(_T("Weak (*)"));

  // Stacksize
  m_comboStack.AddString(_T("1048576"));
  m_comboStack.AddString(_T("2097152"));
  m_comboStack.AddString(_T("3145728"));
  m_comboStack.AddString(_T("4194304"));
  m_comboStack.AddString(_T("5242880"));
  m_comboStack.AddString(_T("6291456"));
  m_comboStack.AddString(_T("7340032"));
  m_comboStack.AddString(_T("8388608"));
}

void
WebConfigServer::InitIIS()
{
  if(m_iis)
  {
    m_buttonUseBaseURL   .EnableWindow(false);
    m_buttonUseProtocol  .EnableWindow(false);
    m_buttonUseBinding   .EnableWindow(false);
    m_buttonUsePort      .EnableWindow(false);
    m_buttonUseBacklog   .EnableWindow(false);
    m_buttonUseMinThreads.EnableWindow(false);
    m_buttonUseMaxThreads.EnableWindow(false);
    m_buttonUseStacksize .EnableWindow(false);
  }
}

void
WebConfigServer::ReadWebConfig(MarlinConfig& config)
{
  // READ THE SERVER OVERRIDES
  m_useWebroot        = config.HasParameter(_T("Server"),_T("WebRoot"));
  m_useBaseURL        = config.HasParameter(_T("Server"),_T("BaseURL"));
  m_useProtocol       = config.HasParameter(_T("Server"),_T("Secure"));
  m_useBinding        = config.HasParameter(_T("Server"),_T("ChannelType"));
  m_usePort           = config.HasParameter(_T("Server"),_T("Port"));
  m_useBacklog        = config.HasParameter(_T("Server"),_T("QueueLength"));
  m_useTunneling      = config.HasParameter(_T("Server"),_T("VerbTunneling"));
  m_useMinThreads     = config.HasParameter(_T("Server"),_T("MinThreads"));
  m_useMaxThreads     = config.HasParameter(_T("Server"),_T("MaxThreads"));
  m_useStacksize      = config.HasParameter(_T("Server"),_T("StackSize"));
  m_useServerUnicode  = config.HasParameter(_T("Server"),_T("RespondUnicode"));
  m_useGzip           = config.HasParameter(_T("Server"),_T("HTTPCompression"));
  m_useStreamingLimit = config.HasParameter(_T("Server"),_T("StreamingLimit"));
  m_useCompressLimit  = config.HasParameter(_T("Server"),_T("CompressLimit"));
  m_useThrotteling    = config.HasParameter(_T("Server"),_T("HTTPThrotteling"));
  m_useKeepalive      = config.HasParameter(_T("Server"),_T("EventKeepAlive"));
  m_useRetrytime      = config.HasParameter(_T("Server"),_T("EventRetryTime"));

  m_webroot           = config.GetParameterString (_T("Server"),_T("WebRoot"),           _T(""));
  m_baseURL           = config.GetParameterString (_T("Server"),_T("BaseURL"),           _T(""));
  m_secureProtocol    = config.GetParameterBoolean(_T("Server"),_T("Secure"),         false);
  m_port              = config.GetParameterInteger(_T("Server"),_T("Port"),              80);
  m_binding           = config.GetParameterString (_T("Server"),_T("ChannelType"),       _T(""));
  m_backlogQueue      = config.GetParameterInteger(_T("Server"),_T("QueueLength"),       64);
  m_tunneling         = config.GetParameterBoolean(_T("Server"),_T("VerbTunneling"),  false);
  m_minThreads        = config.GetParameterInteger(_T("Server"),_T("MinThreads"),         4);
  m_maxThreads        = config.GetParameterInteger(_T("Server"),_T("MaxThreads"),       100);
  m_stackSize         = config.GetParameterInteger(_T("Server"),_T("StackSize"),    1048576);
  m_serverUnicode     = config.GetParameterBoolean(_T("Server"),_T("RespondUnicode"), false);
  m_gzip              = config.GetParameterBoolean(_T("Server"),_T("HTTPCompression"),false);
  m_streamingLimit    = config.GetParameterInteger(_T("Server"),_T("StreamingLimit"),STREAMING_LIMIT);
  m_compressLimit     = config.GetParameterInteger(_T("Server"),_T("CompressLimit"), COMPRESS_LIMIT);
  m_throtteling       = config.GetParameterBoolean(_T("Server"),_T("HTTPThrotteling"),false);
  m_keepalive         = config.GetParameterInteger(_T("Server"),_T("EventKeepAlive"), DEFAULT_EVENT_KEEPALIVE);
  m_retrytime         = config.GetParameterInteger(_T("Server"),_T("EventRetryTime"), DEFAULT_EVENT_RETRYTIME);

  // READ THE SECURITY OVERRIDES
  m_useXFrameOpt      = config.HasParameter(_T("Security"),_T("XFrameOption"));
  m_useXFrameAllow    = config.HasParameter(_T("Security"),_T("XFrameAllowed"));
  m_useHstsMaxAge     = config.HasParameter(_T("Security"),_T("HSTSMaxAge"));
  m_useHstsDomain     = config.HasParameter(_T("Security"),_T("HSTSSubDomains"));
  m_useNoSniff        = config.HasParameter(_T("Security"),_T("ContentNoSniff"));
  m_useXssProtect     = config.HasParameter(_T("Security"),_T("XSSProtection"));
  m_useXssBlock       = config.HasParameter(_T("Security"),_T("XSSBlockMode"));
  m_useNoCache        = config.HasParameter(_T("Security"),_T("NoCacheControl"));
  m_useCORS           = config.HasParameter(_T("Security"),_T("CORS"));

  m_xFrameOption      = config.GetParameterString (_T("Security"),_T("XFrameOption"),     _T(""));
  m_xFrameAllowed     = config.GetParameterString (_T("Security"),_T("XFrameAllowed"),    _T(""));
  m_hstsMaxAge        = config.GetParameterInteger(_T("Security"),_T("HSTSMaxAge"),        0);
  m_hstsSubDomain     = config.GetParameterBoolean(_T("Security"),_T("HSTSSubDomains"),false);
  m_xNoSniff          = config.GetParameterBoolean(_T("Security"),_T("ContentNoSniff"),false);
  m_XSSProtection     = config.GetParameterBoolean(_T("Security"),_T("XSSProtection"), false);
  m_XSSBlockMode      = config.GetParameterBoolean(_T("Security"),_T("XSSBlockMode"),  false);
  m_noCacheControl    = config.GetParameterBoolean(_T("Security"),_T("NoCacheControl"),false);
  m_cors              = config.GetParameterBoolean(_T("Security"),_T("CORS"),          false);
  m_allowOrigin       = config.GetParameterString (_T("Security"),_T("CORS_AllowOrigin"), _T(""));
  m_allowHeaders      = config.GetParameterString (_T("Security"),_T("CORS_AllowHeaders"),_T(""));
  m_allowMaxAge       = config.GetParameterInteger(_T("Security"),_T("CORS_MaxAge"),   86400);
  m_corsCredentials   = config.GetParameterBoolean(_T("Security"),_T("CORS_AllowCredentials"),false);

  // READ THE COOKIE OVERRIDES
  m_useCookieSecure   = config.HasParameter(_T("Cookies"),_T("Secure"));
  m_useCookieHttpOnly = config.HasParameter(_T("Cookies"),_T("HttpOnly"));
  m_useCookieSameSite = config.HasParameter(_T("Cookies"),_T("SameSite"));
  m_useCookiePath     = config.HasParameter(_T("Cookies"),_T("Path"));
  m_useCookieDomain   = config.HasParameter(_T("Cookies"),_T("Domain"));
  m_useCookieExpires  = config.HasParameter(_T("Cookies"),_T("Expires"));
  m_useCookieMaxAge   = config.HasParameter(_T("Cookies"),_T("MaxAge"));
  
  m_cookieSecure      = config.GetParameterBoolean(_T("Cookies"),_T("Secure"),  false);
  m_cookieHttpOnly    = config.GetParameterBoolean(_T("Cookies"),_T("HttpOnly"),false);
  m_cookieSameSite    = config.GetParameterString (_T("Cookies"),_T("SameSite"),_T(""));
  m_cookiePath        = config.GetParameterString (_T("Cookies"),_T("Path"),    _T(""));
  m_cookieDomain      = config.GetParameterString (_T("Cookies"),_T("Domain"),  _T(""));
  m_cookieExpires     = config.GetParameterInteger(_T("Cookies"),_T("Expires"), 0);
  m_cookieMaxAge      = config.GetParameterInteger(_T("Cookies"),_T("MaxAge"),  0);

  // INIT THE COMBO BOXES
  m_comboProtocol.SetCurSel(m_secureProtocol ? 1 : 0);

  // Protocol
  if(m_binding.CompareNoCase(_T("strong"))  == 0) m_comboBinding.SetCurSel(1);
  if(m_binding.CompareNoCase(_T("named"))   == 0) m_comboBinding.SetCurSel(2);
  if(m_binding.CompareNoCase(_T("address")) == 0) m_comboBinding.SetCurSel(3);
  if(m_binding.CompareNoCase(_T("full"))    == 0) m_comboBinding.SetCurSel(4);
  if(m_binding.CompareNoCase(_T("weak"))    == 0) m_comboBinding.SetCurSel(5);


  // INIT THE CHECKBOXES
  m_buttonServerUnicode .SetCheck(m_serverUnicode);
  m_buttonTunneling     .SetCheck(m_tunneling);
  m_buttonGzip          .SetCheck(m_gzip);
  m_buttonThrotteling   .SetCheck(m_throtteling);

  // INIT ALL USING FIELDS
  m_buttonUseWebroot       .SetCheck(m_useWebroot);
  m_buttonUseBaseURL       .SetCheck(m_useBaseURL);
  m_buttonUseProtocol      .SetCheck(m_useProtocol);
  m_buttonUseBinding       .SetCheck(m_useBinding);
  m_buttonUsePort          .SetCheck(m_usePort);
  m_buttonUseBacklog       .SetCheck(m_useBacklog);
  m_buttonUseTunneling     .SetCheck(m_useTunneling);
  m_buttonUseMinThreads    .SetCheck(m_useMinThreads);
  m_buttonUseMaxThreads    .SetCheck(m_useMaxThreads);
  m_buttonUseStacksize     .SetCheck(m_useStacksize);
  m_buttonUseServerUnicode .SetCheck(m_useServerUnicode);
  m_buttonUseGzip          .SetCheck(m_useGzip);
  m_buttonUseStreamingLimit.SetCheck(m_useStreamingLimit);
  m_buttonUseCompressLimit .SetCheck(m_useCompressLimit);
  m_buttonUseThrotteling   .SetCheck(m_useThrotteling);
  m_buttonUseKeepalive     .SetCheck(m_useKeepalive);
  m_buttonUseRetrytime     .SetCheck(m_useRetrytime);

  UpdateData(FALSE);
}

void
WebConfigServer::WriteWebConfig(MarlinConfig& config)
{
  // GET STRINGS
  XString port;       port      .Format(_T("%d"),m_port);
  XString minThreads; minThreads.Format(_T("%d"),m_minThreads);
  XString maxThreads; maxThreads.Format(_T("%d"),m_maxThreads);
  XString stackSize;  stackSize .Format(_T("%d"),m_stackSize);

  // WRITE THE CONFIG PARAMETERS
  config.SetSection(_T("Server"));

  // STANDALONE SERVER
  if(m_useWebroot)        config.SetParameter   (_T("Server"),_T("WebRoot"),      m_webroot);
  else                    config.RemoveParameter(_T("Server"),_T("WebRoot"));
  if(m_useBaseURL)        config.SetParameter   (_T("Server"),_T("BaseURL"),      m_baseURL);
  else                    config.RemoveParameter(_T("Server"),_T("BaseURL"));
  if(m_useProtocol)       config.SetParameter   (_T("Server"),_T("Secure"),       m_secureProtocol);
  else                    config.RemoveParameter(_T("Server"),_T("Secure"));
  if(m_usePort)           config.SetParameter   (_T("Server"),_T("Port"),         port);
  else                    config.RemoveParameter(_T("Server"),_T("Port"));
  if(m_useBinding)        config.SetParameter   (_T("Server"),_T("ChannelType"),  m_binding);
  else                    config.RemoveParameter(_T("Server"),_T("ChannelType"));
  if(m_useBacklog)        config.SetParameter   (_T("Server"),_T("QueueLength"),  m_backlogQueue);
  else                    config.RemoveParameter(_T("Server"),_T("QueueLength"));
  if(m_useMinThreads)     config.SetParameter   (_T("Server"),_T("MinThreads"),   minThreads);
  else                    config.RemoveParameter(_T("Server"),_T("MinThreads"));
  if(m_useMaxThreads)     config.SetParameter   (_T("Server"),_T("MaxThreads"),   maxThreads);
  else                    config.RemoveParameter(_T("Server"),_T("MaxThreads"));
  if(m_useStacksize)      config.SetParameter   (_T("Server"),_T("StackSize"),    stackSize);
  else                    config.RemoveParameter(_T("Server"),_T("StackSize"));
  // STANDALONE + IIS
  if(m_useServerUnicode)  config.SetParameter   (_T("Server"),_T("RespondUnicode"), m_serverUnicode);
  else                    config.RemoveParameter(_T("Server"),_T("RespondUnicode"));
  if(m_useGzip)           config.SetParameter   (_T("Server"),_T("HTTPCompression"),m_gzip);
  else                    config.RemoveParameter(_T("Server"),_T("HTTPCompression"));
  if(m_useStreamingLimit) config.SetParameter   (_T("Server"),_T("StreamingLimit"), (int)m_streamingLimit);
  else                    config.RemoveParameter(_T("Server"),_T("StreamingLimit"));
  if(m_useCompressLimit)  config.SetParameter   (_T("Server"),_T("CompressLimit"),  (int)m_compressLimit);
  else                    config.RemoveParameter(_T("Server"),_T("CompressLimit"));
  if(m_useThrotteling)    config.SetParameter   (_T("Server"),_T("HTTPThrotteling"),m_throtteling);
  else                    config.RemoveParameter(_T("Server"),_T("HTTPThrotteling"));
  if(m_useTunneling)      config.SetParameter   (_T("Server"),_T("VerbTunneling"),  m_tunneling);
  else                    config.RemoveParameter(_T("Server"),_T("VerbTunneling"));
  if(m_useKeepalive)      config.SetParameter   (_T("Server"),_T("EventKeepAlive"), m_keepalive);
  else                    config.RemoveParameter(_T("Server"),_T("EventKeepAlive"));
  if(m_useRetrytime)      config.SetParameter   (_T("Server"),_T("EventRetryTime"), m_retrytime);
  else                    config.RemoveParameter(_T("Server"),_T("EventRetryTime"));

  // SECURITY HEADERS
  config.SetSection(_T("Security"));

  if(m_useXFrameOpt)    config.SetParameter   (_T("Security"),_T("XFrameOption"),   m_xFrameOption);
  else                  config.RemoveParameter(_T("Security"),_T("XFrameOption"));
  if(m_useXFrameAllow)  config.SetParameter   (_T("Security"),_T("XFrameAllowed"),  m_xFrameAllowed);
  else                  config.RemoveParameter(_T("Security"),_T("XFrameAllowed"));
  if(m_useHstsMaxAge)   config.SetParameter   (_T("Security"),_T("HSTSMaxAge"),     (int)m_hstsMaxAge);
  else                  config.RemoveParameter(_T("Security"),_T("HSTSMaxAge"));
  if(m_useHstsDomain)   config.SetParameter   (_T("Security"),_T("HSTSSubDomains"), m_hstsSubDomain);
  else                  config.RemoveParameter(_T("Security"),_T("HSTSSubDomains"));
  if(m_useNoSniff)      config.SetParameter   (_T("Security"),_T("ContentNoSniff"), m_xNoSniff);
  else                  config.RemoveParameter(_T("Security"),_T("ContentNoSniff"));
  if(m_useXssProtect)   config.SetParameter   (_T("Security"),_T("XSSProtection"),  m_XSSProtection);
  else                  config.RemoveParameter(_T("Security"),_T("XSSProtection"));
  if(m_useXssBlock)     config.SetParameter   (_T("Security"),_T("XSSBlockMode"),   m_XSSBlockMode);
  else                  config.RemoveParameter(_T("Security"),_T("XSSBlockMode"));
  if(m_useNoCache)      config.SetParameter   (_T("Security"),_T("NoCacheControl"), m_noCacheControl);
  else                  config.RemoveParameter(_T("Security"),_T("NoCacheControl"));
  if(m_useCORS)         config.SetParameter   (_T("Security"),_T("CORS"),           m_cors);
  else                  config.RemoveParameter(_T("Security"),_T("CORS"));
  if(m_useCORS)         config.SetParameter   (_T("Security"),_T("CORS_AllowOrigin"),m_allowOrigin);
  else                  config.RemoveParameter(_T("Security"),_T("CORS_AllowOrigin"));
  if(m_useCORS)         config.SetParameter   (_T("Security"),_T("CORS_AllowHeaders"),m_allowHeaders);
  else                  config.RemoveParameter(_T("Security"),_T("CORS_AllowHeaders"));
  if(m_useCORS)         config.SetParameter   (_T("Security"),_T("CORS_MaxAge"),      m_allowMaxAge);
  else                  config.RemoveParameter(_T("Security"),_T("CORS_MaxAge"));
  if(m_useCORS)         config.SetParameter   (_T("Security"),_T("CORS_AllowCredentials"),m_corsCredentials);
  else                  config.RemoveParameter(_T("Security"),_T("CORS_AllowCredentials"));

  // COOKIE HEADERS
  config.SetSection(_T("Cookies"));

  if(m_useCookieSecure)   config.SetParameter   (_T("Cookies"),_T("Secure"),  m_cookieSecure);
  else                    config.RemoveParameter(_T("Cookies"),_T("Secure"));
  if(m_useCookieHttpOnly) config.SetParameter   (_T("Cookies"),_T("HttpOnly"),m_cookieHttpOnly);
  else                    config.RemoveParameter(_T("Cookies"),_T("HttpOnly"));
  if(m_useCookieSameSite) config.SetParameter   (_T("Cookies"),_T("SameSite"),m_cookieSameSite);
  else                    config.RemoveParameter(_T("Cookies"),_T("SameSite"));
  if(m_useCookiePath)     config.SetParameter   (_T("Cookies"),_T("Path"),    m_cookiePath);
  else                    config.RemoveParameter(_T("Cookies"),_T("Path"));
  if(m_useCookieDomain)   config.SetParameter   (_T("Cookies"),_T("Domain"),  m_cookieDomain);
  else                    config.RemoveParameter(_T("Cookies"),_T("Domain"));
  if(m_useCookieExpires)  config.SetParameter   (_T("Cookies"),_T("Expires"), m_cookieExpires);
  else                    config.RemoveParameter(_T("Cookies"),_T("Expires"));
  if(m_useCookieMaxAge)   config.SetParameter   (_T("Cookies"),_T("MaxAge"),  m_cookieMaxAge);
  else                    config.RemoveParameter(_T("Cookies"),_T("MaxAge"));
}

// WebConfigDlg message handlers

//////////////////////////////////////////////////////////////////////////
//
// SERVER OVERRIDES
//
//////////////////////////////////////////////////////////////////////////

void 
WebConfigServer::OnEnChangeWebroot()
{
  UpdateData();
}

void 
WebConfigServer::OnBnClickedButtWebroot()
{
  XString rootdir;
  MapDialog map;
  if(map.Browse(GetSafeHwnd()
               ,_T("Get the path to the webroot directory")
               ,m_webroot
               ,rootdir
               ,false    // files
               ,true))   // status
  {
    XString pad = map.GetPath();
    if(m_webroot.CompareNoCase(pad))
    {
      m_webroot = pad;
      UpdateData(FALSE);
    }
  }
}

void 
WebConfigServer::OnEnChangeBaseUrl()
{
  UpdateData();
}

void 
WebConfigServer::OnCbnSelchangeProtocol()
{
  int sel = m_comboProtocol.GetCurSel();
  if(sel >= 0)
  {
    m_secureProtocol = (sel > 0);
  }
}

void 
WebConfigServer::OnCbnSelchangeBinding()
{
  int sel = m_comboBinding.GetCurSel();
  if(sel >= 0)
  {
    switch(sel)
    {
      default:[[fallthrough]];
      case 0: m_binding.Empty();
              break;
      case 1: m_binding = _T("strong");
              break;
      case 2: m_binding = _T("named");
              break;
      case 3: m_binding = _T("address");
              break;
      case 4: m_binding = _T("full");
              break;
      case 5: m_binding = _T("weak");
              break;
    }
  }
}

void 
WebConfigServer::OnEnChangePort()
{
  UpdateData();
}

void 
WebConfigServer::OnEnChangeBacklogQueue()
{
  UpdateData();
}

void 
WebConfigServer::OnBnClickedTunneling()
{
  m_tunneling = m_buttonTunneling.GetCheck() > 0;
}

void 
WebConfigServer::OnEnChangeMinThreads()
{
  UpdateData();
}

void 
WebConfigServer::OnEnChangeMaxThreads()
{
  UpdateData();
}

void 
WebConfigServer::OnCbnSelchangeStacksize()
{
  int sel = m_comboStack.GetCurSel();
  if(sel >= 0)
  {
    XString size;
    m_comboStack.GetLBText(sel,size);
    m_stackSize = _ttoi(size);
  }
}

void 
WebConfigServer::OnEnChangeStacksize()
{
  UpdateData();
}

void 
WebConfigServer::OnBnClickedServerUnicode()
{
  m_serverUnicode = m_buttonServerUnicode.GetCheck() > 0;
}

void 
WebConfigServer::OnBnClickedGzip()
{
  m_gzip = m_buttonGzip.GetCheck() > 0;
}

void
WebConfigServer::OnEnChangeStreamingLimit()
{
  UpdateData();
}

void
WebConfigServer::OnEnChangeCompressLimit()
{
  UpdateData();
}

void
WebConfigServer::OnBnClickedThrotteling()
{
  m_throtteling = m_buttonThrotteling.GetCheck() > 0;
}

void 
WebConfigServer::OnEnChangeKeepalive()
{
  UpdateData();
}

void 
WebConfigServer::OnEnChangeRetrytime()
{
  UpdateData();
}

void 
WebConfigServer::OnBnClickedSetCookie()
{
  SetCookieDlg dlg(this);
  dlg.DoModal();
}

//////////////////////////////////////////////////////////////////////////
//
// USING FIELDS EVENTS
//
//////////////////////////////////////////////////////////////////////////

void 
WebConfigServer::OnBnClickedUseWebroot()
{
  m_useWebroot = m_buttonUseWebroot.GetCheck() > 0;
  UpdateData(FALSE);
}

void 
WebConfigServer::OnBnClickedUseUrl()
{
  m_useBaseURL = m_buttonUseBaseURL.GetCheck() > 0;
  UpdateData(FALSE);
}

void 
WebConfigServer::OnBnClickedUseProtocol()
{
  m_useProtocol = m_buttonUseProtocol.GetCheck() > 0;
  UpdateData(FALSE);
}

void 
WebConfigServer::OnBnClickedUseBinding()
{
  m_useBinding = m_buttonUseBinding.GetCheck() > 0;
  UpdateData(FALSE);
}

void 
WebConfigServer::OnBnClickedUsePort()
{
  m_usePort = m_buttonUsePort.GetCheck() > 0;
  UpdateData(FALSE);
}

void
WebConfigServer::OnBnClickedUseBacklog()
{
  m_useBacklog = m_buttonUseBacklog.GetCheck() > 0;
  UpdateData(FALSE);
}

void
WebConfigServer::OnBnClickedUseTunneling()
{
  m_useTunneling = m_buttonUseTunneling.GetCheck() > 0;
  UpdateData(FALSE);
}

void
WebConfigServer::OnBnClickedHeaders()
{
  ServerHeadersDlg dlg(this);
  dlg.DoModal();
}

void
WebConfigServer::OnBnClickedUseMinThreads()
{
  m_useMinThreads = m_buttonUseMinThreads.GetCheck() > 0;
  UpdateData(FALSE);
}

void 
WebConfigServer::OnBnClickedUseMaxThreads()
{
  m_useMaxThreads = m_buttonUseMaxThreads.GetCheck() > 0;
  UpdateData(FALSE);
}

void 
WebConfigServer::OnBnClickedUseStacksize()
{
  m_useStacksize = m_buttonUseStacksize.GetCheck() > 0;
  UpdateData(FALSE);
}

void
WebConfigServer::OnBnClickedUseServerUnicode()
{
  m_useServerUnicode = m_buttonUseServerUnicode.GetCheck() > 0;
  UpdateData(FALSE);
}

void
WebConfigServer::OnBnClickedUseGzip()
{
  m_useGzip = m_buttonUseGzip.GetCheck() > 0;
  UpdateData(FALSE);
}

void
WebConfigServer::OnBnClickedUseStreamingLimit()
{
  m_useStreamingLimit = m_buttonUseStreamingLimit.GetCheck() > 0;
  UpdateData(FALSE);
}

void
WebConfigServer::OnBnClickedUseCompressLimit()
{
  m_useCompressLimit = m_buttonUseCompressLimit.GetCheck() > 0;
  UpdateData(FALSE);
}

void
WebConfigServer::OnBnClickedUseThrotteling()
{
  m_useThrotteling = m_buttonUseThrotteling.GetCheck() > 0;
  UpdateData(FALSE);
}

void WebConfigServer::OnBnClickedUseKeepalive()
{
  m_useKeepalive = m_buttonUseKeepalive.GetCheck() > 0;
  UpdateData(FALSE);
}

void WebConfigServer::OnBnClickedUseRetrytime()
{
  m_useRetrytime = m_buttonUseRetrytime.GetCheck() > 0;
  UpdateData(FALSE);
}
