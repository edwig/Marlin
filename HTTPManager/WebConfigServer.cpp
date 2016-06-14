/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: WebConfigServer.cpp
//
// Marlin Component: Internet server/client
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
#include "HTTPManager.h"
#include "WebConfigServer.h"
#include "WebConfig.h"
#include "MapDialoog.h"
#include "FileDialog.h"
#include "HTTPClient.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// WebConfigDlg dialog

IMPLEMENT_DYNAMIC(WebConfigServer, CDialogEx)

WebConfigServer::WebConfigServer(CWnd* pParent /*=NULL*/)
                :CDialog(WebConfigServer::IDD, pParent)
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
  m_useReliable       = false;
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

  // SERVER OVERRIDES
  m_port            = 0;
  m_reliable        = false;
  m_backlogQueue    = 0;
  m_tunneling       = false;
  m_minThreads      = 0;
  m_maxThreads      = 0;
  m_stackSize       = 0;
  m_serverUnicode   = false;
  m_gzip            = false;
  m_streamingLimit  = STREAMING_LIMIT;
  m_compressLimit   = COMPRESS_LIMIT;
  m_throtteling     = false;
}

WebConfigServer::~WebConfigServer()
{
}

void WebConfigServer::DoDataExchange(CDataExchange* pDX)
{
  CDialog::DoDataExchange(pDX);
  // USING FIELDS
  DDX_Control(pDX,IDC_USE_WEBROOT,    m_buttonUseWebroot);
  DDX_Control(pDX,IDC_USE_URL,        m_buttonUseBaseURL);
  DDX_Control(pDX,IDC_USE_PROTOCOL,   m_buttonUseProtocol);
  DDX_Control(pDX,IDC_USE_BINDING,    m_buttonUseBinding);
  DDX_Control(pDX,IDC_USE_PORT,       m_buttonUsePort);
  DDX_Control(pDX,IDC_USE_RELIABLE,   m_buttonUseReliable);
  DDX_Control(pDX,IDC_USE_BACKLOG,    m_buttonUseBacklog);
  DDX_Control(pDX,IDC_USE_VERBTUNNEL, m_buttonUseTunneling);
  DDX_Control(pDX,IDC_USE_MIN_THREADS,m_buttonUseMinThreads);
  DDX_Control(pDX,IDC_USE_MAX_THREADS,m_buttonUseMaxThreads);
  DDX_Control(pDX,IDC_USE_STACKSIZE,  m_buttonUseStacksize);
  DDX_Control(pDX,IDC_USE_SERVUNI,    m_buttonUseServerUnicode);
  DDX_Control(pDX,IDC_USE_GZIP,       m_buttonUseGzip);
  DDX_Control(pDX,IDC_USE_STREAM,     m_buttonUseStreamingLimit);
  DDX_Control(pDX,IDC_USE_COMPR,      m_buttonUseCompressLimit);
  DDX_Control(pDX,IDC_USE_THROTTLE,   m_buttonUseThrotteling);

  // SERVER OVERRIDES
  DDX_Text   (pDX,IDC_WEBROOT,        m_webroot);
  DDX_Text   (pDX,IDC_BASE_URL,       m_baseURL);
  DDX_Control(pDX,IDC_PROTOCOL,       m_comboProtocol);
  DDX_Control(pDX,IDC_BINDING,        m_comboBinding);
  DDX_Control(pDX,IDC_RELIABLE,       m_comboReliable);
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

    m_comboProtocol      .EnableWindow(m_useProtocol);
    m_comboBinding       .EnableWindow(m_useBinding);
    m_comboReliable      .EnableWindow(m_useReliable);
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
  ON_CBN_SELCHANGE(IDC_RELIABLE,      &WebConfigServer::OnCbnSelchangeReliable)
  ON_EN_CHANGE    (IDC_STREAM_LIMIT,  &WebConfigServer::OnEnChangeStreamingLimit)
  ON_EN_CHANGE    (IDC_COMP_LIMIT,    &WebConfigServer::OnEnChangeCompressLimit)
  ON_BN_CLICKED   (IDC_THROTTLE,      &WebConfigServer::OnBnClickedThrotteling)
  // USING BUTTONS
  ON_BN_CLICKED(IDC_USE_WEBROOT,      &WebConfigServer::OnBnClickedUseWebroot)
  ON_BN_CLICKED(IDC_USE_URL,          &WebConfigServer::OnBnClickedUseUrl)
  ON_BN_CLICKED(IDC_USE_PROTOCOL,     &WebConfigServer::OnBnClickedUseProtocol)
  ON_BN_CLICKED(IDC_USE_BINDING,      &WebConfigServer::OnBnClickedUseBinding)
  ON_BN_CLICKED(IDC_USE_PORT,         &WebConfigServer::OnBnClickedUsePort)
  ON_BN_CLICKED(IDC_USE_RELIABLE,     &WebConfigServer::OnBnClickedUseReliable)
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

  UpdateData(FALSE);
  return TRUE;
}

void 
WebConfigServer::SetSiteConfig(CString p_urlPrefix,CString p_fileName)
{
  m_url            = p_urlPrefix;
  m_siteConfigFile = p_fileName;
}

void
WebConfigServer::InitComboboxes()
{
  // HTTP/HTTPS protocol
  m_comboProtocol.AddString("http");
  m_comboProtocol.AddString("https");

  // Port binding: strong, named, address, full, weak
  m_comboBinding.AddString("");
  m_comboBinding.AddString("Strong (+)");
  m_comboBinding.AddString("Short name");
  m_comboBinding.AddString("IP Address");
  m_comboBinding.AddString("Full DNS Name");
  m_comboBinding.AddString("Weak (*)");

  // WS-Reliable messaging
  m_comboReliable.AddString("Normal");
  m_comboReliable.AddString("WS-Reliable");

  // Stacksize
  m_comboStack.AddString("1048576");
  m_comboStack.AddString("2097152");
  m_comboStack.AddString("3145728");
  m_comboStack.AddString("4194304");
  m_comboStack.AddString("5242880");
  m_comboStack.AddString("6291456");
  m_comboStack.AddString("7340032");
  m_comboStack.AddString("8388608");
}

void
WebConfigServer::ReadWebConfig(WebConfig& config)
{
  // READ THE SERVER OVERRIDES
  m_useWebroot        = config.HasParameter("Server","WebRoot");
  m_useBaseURL        = config.HasParameter("Server","BaseURL");
  m_useProtocol       = config.HasParameter("Server","Secure");
  m_useBinding        = config.HasParameter("Server","ChannelType");
  m_usePort           = config.HasParameter("Server","Port");
  m_useReliable       = config.HasParameter("Server","Reliable");
  m_useBacklog        = config.HasParameter("Server","QueueLength");
  m_useTunneling      = config.HasParameter("Server","VerbTunneling");
  m_useMinThreads     = config.HasParameter("Server","MinThreads");
  m_useMaxThreads     = config.HasParameter("Server","MaxThreads");
  m_useStacksize      = config.HasParameter("Server","StackSize");
  m_useServerUnicode  = config.HasParameter("Server","RespondUnicode");
  m_useGzip           = config.HasParameter("Server","HTTPCompression");
  m_useStreamingLimit = config.HasParameter("Server","StreamingLimit");
  m_useCompressLimit  = config.HasParameter("Server","CompressLimit");
  m_useThrotteling    = config.HasParameter("Server","HTTPThrotteling");

  m_webroot           = config.GetParameterString ("Server","WebRoot",           "");
  m_baseURL           = config.GetParameterString ("Server","BaseURL",           "");
  m_secureProtocol    = config.GetParameterBoolean("Server","Secure",         false);
  m_port              = config.GetParameterInteger("Server","Port",              80);
  m_binding           = config.GetParameterString ("Server","ChannelType",       "");
  m_reliable          = config.GetParameterBoolean("Server","Reliable",       false);
  m_backlogQueue      = config.GetParameterInteger("Server","QueueLength",       64);
  m_tunneling         = config.GetParameterBoolean("Server","VerbTunneling",  false);
  m_minThreads        = config.GetParameterInteger("Server","MinThreads",         4);
  m_maxThreads        = config.GetParameterInteger("Server","MaxThreads",       100);
  m_stackSize         = config.GetParameterInteger("Server","StackSize",    1048576);
  m_serverUnicode     = config.GetParameterBoolean("Server","RespondUnicode", false);
  m_gzip              = config.GetParameterBoolean("Server","HTTPCompression",false);
  m_streamingLimit    = config.GetParameterInteger("Server","StreamingLimit",STREAMING_LIMIT);
  m_compressLimit     = config.GetParameterInteger("Server","CompressLimit", COMPRESS_LIMIT);
  m_throtteling       = config.GetParameterBoolean("Server","HTTPThrotteling",false);

  // READ THE SECURITY OVERRIDES
  m_useXFrameOpt      = config.HasParameter("Security","XFrameOption");
  m_useXFrameAllow    = config.HasParameter("Security","XFrameAllowed");
  m_useHstsMaxAge     = config.HasParameter("Security","HSTSMaxAge");
  m_useHstsDomain     = config.HasParameter("Security","HSTSSubDomains");
  m_useNoSniff        = config.HasParameter("Security","ContentNoSniff");
  m_useXssProtect     = config.HasParameter("Security","XSSProtection");
  m_useXssBlock       = config.HasParameter("Security","XSSBlockMode");
  m_useNoCache        = config.HasParameter("Security","NoCacheControl");

  m_xFrameOption      = config.GetParameterString ("Security","XFrameOption",     "");
  m_xFrameAllowed     = config.GetParameterString ("Security","XFrameAllowed",    "");
  m_hstsMaxAge        = config.GetParameterInteger("Security","HSTSMaxAge",        0);
  m_hstsSubDomain     = config.GetParameterBoolean("Security","HSTSSubDomains",false);
  m_xNoSniff          = config.GetParameterBoolean("Security","ContentNoSniff",false);
  m_XSSProtection     = config.GetParameterBoolean("Security","XSSProtection", false);
  m_XSSBlockMode      = config.GetParameterBoolean("Security","XSSBlockMode",  false);
  m_noCacheControl    = config.GetParameterBoolean("Security","NoCacheControl",false);

  // INIT THE COMBO BOXES
  m_comboProtocol.SetCurSel(m_secureProtocol ? 1 : 0);
  m_comboReliable.SetCurSel(m_reliable       ? 1 : 0);

  // Protocol
  if(m_binding.CompareNoCase("strong")  == 0) m_comboBinding.SetCurSel(1);
  if(m_binding.CompareNoCase("named")   == 0) m_comboBinding.SetCurSel(2);
  if(m_binding.CompareNoCase("address") == 0) m_comboBinding.SetCurSel(3);
  if(m_binding.CompareNoCase("full")    == 0) m_comboBinding.SetCurSel(4);
  if(m_binding.CompareNoCase("weak")    == 0) m_comboBinding.SetCurSel(5);

  // INIT THE CHECKBOXES
  m_buttonServerUnicode.SetCheck(m_serverUnicode);
  m_buttonTunneling.SetCheck(m_tunneling);
  m_buttonGzip.SetCheck(m_gzip);
  m_buttonThrotteling.SetCheck(m_throtteling);

  // INIT ALL USING FIELDS
  m_buttonUseWebroot       .SetCheck(m_useWebroot);
  m_buttonUseBaseURL       .SetCheck(m_useBaseURL);
  m_buttonUseProtocol      .SetCheck(m_useProtocol);
  m_buttonUseBinding       .SetCheck(m_useBinding);
  m_buttonUsePort          .SetCheck(m_usePort);
  m_buttonUseReliable      .SetCheck(m_useReliable);
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

  UpdateData(FALSE);
}

void
WebConfigServer::WriteWebConfig(WebConfig& config)
{
  // GET STRINGS
  CString port;       port      .Format("%d",m_port);
  CString minThreads; minThreads.Format("%d",m_minThreads);
  CString maxThreads; maxThreads.Format("%d",m_maxThreads);
  CString stackSize;  stackSize .Format("%d",m_stackSize);

  // WRITE THE CONFIG PARAMETERS
  config.SetSection("Server");

  if(m_useWebroot)        config.SetParameter   ("Server","WebRoot",      m_webroot);
  else                    config.RemoveParameter("Server","WebRoot");
  if(m_useBaseURL)        config.SetParameter   ("Server","BaseURL",      m_baseURL);
  else                    config.RemoveParameter("Server","BaseURL");
  if(m_useProtocol)       config.SetParameter   ("Server","Secure",       m_secureProtocol);
  else                    config.RemoveParameter("Server","Secure");
  if(m_usePort)           config.SetParameter   ("Server","Port",         port);
  else                    config.RemoveParameter("Server","Port");
  if(m_useBinding)        config.SetParameter   ("Server","ChannelType",  m_binding);
  else                    config.RemoveParameter("Server","ChannelType");
  if(m_useReliable)       config.SetParameter   ("Server","Reliable",     m_reliable);
  else                    config.RemoveParameter("Server","Reliable");
  if(m_useBacklog)        config.SetParameter   ("Server","QueueLength",  m_backlogQueue);
  else                    config.RemoveParameter("Server","QueueLength");
  if(m_useTunneling)      config.SetParameter   ("Server","VerbTunneling",m_tunneling);
  else                    config.RemoveParameter("Server","VerbTunneling");
  if(m_useMinThreads)     config.SetParameter   ("Server","MinThreads",   minThreads);
  else                    config.RemoveParameter("Server","MinThreads");
  if(m_useMaxThreads)     config.SetParameter   ("Server","MaxThreads",   maxThreads);
  else                    config.RemoveParameter("Server","MaxThreads");
  if(m_useStacksize)      config.SetParameter   ("Server","StackSize",    stackSize);
  else                    config.RemoveParameter("Server","StackSize");
  if(m_useServerUnicode)  config.SetParameter   ("Server","RespondUnicode", m_serverUnicode);
  else                    config.RemoveParameter("Server","RespondUnicode");
  if(m_useGzip)           config.SetParameter   ("Server","HTTPCompression",m_gzip);
  else                    config.RemoveParameter("Server","HTTPCompression");
  if(m_useStreamingLimit) config.SetParameter   ("Server","StreamingLimit",(int)m_streamingLimit);
  else                    config.RemoveParameter("Server","StreamingLimit");
  if(m_useCompressLimit)  config.SetParameter   ("Server","CompressLimit",(int)m_compressLimit);
  else                    config.RemoveParameter("Server","CompressLimit");
  if(m_useThrotteling)    config.SetParameter   ("Server","HTTPThrotteling", m_throtteling);
  else                    config.RemoveParameter("Server","HTTPThrotteling");

  config.SetSection("Security");

  if(m_useXFrameOpt)    config.SetParameter   ("Security","XFrameOption",   m_xFrameOption);
  else                  config.RemoveParameter("Security","XFrameOption");
  if(m_useXFrameAllow)  config.SetParameter   ("Security","XFrameAllowed",  m_xFrameAllowed);
  else                  config.RemoveParameter("Security","XFrameAllowed");
  if(m_useHstsMaxAge)   config.SetParameter   ("Security","HSTSMaxAge",     (int)m_hstsMaxAge);
  else                  config.RemoveParameter("Security","HSTSMaxAge");
  if(m_useHstsDomain)   config.SetParameter   ("Security","HSTSSubDomains", m_hstsSubDomain);
  else                  config.RemoveParameter("Security","HSTSSubDomains");
  if(m_useNoSniff)      config.SetParameter   ("Security","ContentNoSniff", m_xNoSniff);
  else                  config.RemoveParameter("Security","ContentNoSniff");
  if(m_useXssProtect)   config.SetParameter   ("Security","XSSProtection",  m_XSSProtection);
  else                  config.RemoveParameter("Security","XSSProtection");
  if(m_useXssBlock)     config.SetParameter   ("Security","XSSBlockMode",   m_XSSBlockMode);
  else                  config.RemoveParameter("Security","XSSBlockMode");
  if(m_useNoCache)      config.SetParameter   ("Security","NoCacheControl", m_noCacheControl);
  else                  config.RemoveParameter("Security","NoCacheControl");
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
  CString rootdir;
  MapDialoog map;
  if(map.Browse(GetSafeHwnd()
               ,"Get the path to the webroot directory"
               ,m_webroot
               ,rootdir
               ,false    // files
               ,true))   // status
  {
    CString pad = map.GetPath();
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
      default:// Fall through
      case 0: m_binding.Empty();
              break;
      case 1: m_binding = "strong";
              break;
      case 2: m_binding = "named";
              break;
      case 3: m_binding = "address";
              break;
      case 4: m_binding = "full";
              break;
      case 5: m_binding = "weak";
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
WebConfigServer::OnCbnSelchangeReliable()
{
  int sel = m_comboReliable.GetCurSel();
  if(sel >= 0)
  {
    m_reliable = (sel > 0);
  }
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
    CString size;
    m_comboStack.GetLBText(sel,size);
    m_stackSize = atoi(size);
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
WebConfigServer::OnBnClickedUseReliable()
{
  m_useReliable = m_buttonUseReliable.GetCheck() > 0;
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