//////////////////////////////////////////////////////////////////////////
//
// USER-SPACE IMPLEMENTTION OF HTTP.SYS
//
// 2018 - 2024 (c) ir. W.E. Huisman
// License: MIT
//
//////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "http_private.h"
#include "URL.h"
#include "RequestQueue.h"
#include "UrlGroup.h"
#include "HTTPReadRegister.h"
#include "ServerSession.h"
#include "OpaqueHandles.h"
#include "http_private.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

UrlGroup::UrlGroup(ServerSession* p_session)
         :m_session(p_session)
         ,m_queue(nullptr)
{
  InitializeCriticalSection(&m_lock);
}

UrlGroup::~UrlGroup()
{
  DeleteCriticalSection(&m_lock);
}

// Remember the request queue the URL group was registered for.
// For some reason the API wants to do this after creating the group
// Apparently we must be able to switch request queues?
void
UrlGroup::SetRequestQueue(HANDLE p_requestQueue)
{
  AutoCritSec lock(&m_lock);

  if(p_requestQueue)
  {
    // Finding our request queue
    m_queue = g_handles.GetReQueueFromOpaqueHandle(p_requestQueue);
    if(m_queue == nullptr)
    {
      return;
    }
    m_queue->AddURLGroup(this);
  }
  else if(m_queue)
  {
    // Resetting (parameter is null)
    m_queue->RemoveURLGroup(this);
    m_queue = nullptr;
  }
}

// Enable or disable the state of the URL group
// No functional application known why we want to disable a group
void
UrlGroup::SetEnabledState(HTTP_ENABLED_STATE p_state)
{
  if(p_state == HttpEnabledStateActive || p_state == HttpEnabledStateInactive)
  {
    m_state = p_state;
  }
}

// Adding an URL to the Url-Group and registering its context pointer/number
// URL Must be registered in the MS-Windows registry (services\http\parameters)
// Optionally start a listener for the portnumber if not already started
ULONG 
UrlGroup::AddUrlPrefix(XString pFullyQualifiedUrl,HTTP_URL_CONTEXT UrlContext)
{
  AutoCritSec lock(&m_lock);

  // Check if registered with "netsh" command
  if(!UrlIsRegistered(pFullyQualifiedUrl))
  {
    return ERROR_INVALID_PARAMETER;
  }

  URL url;
  ULONG result = SplitURLPrefix(pFullyQualifiedUrl, &url);
  if(result == NO_ERROR)
  {
    // Record the context
    url.m_context  = UrlContext;
    url.m_urlGroup = this;

    // Find the SSL/TLS settings from the registry
    if(url.m_secure && !GetURLSettings(url))
    {
      return ERROR_INVALID_PARAMETER;
    }
    // Getting the parameters
    XString name = url.m_abspath;
    USHORT  port = url.m_port;

    // See if registered before
    pFullyQualifiedUrl.MakeLower();
    URLNames::iterator it = m_urls.find(pFullyQualifiedUrl);
    if(it != m_urls.end())
    {
      return ERROR_ALREADY_EXISTS;
    }

    // Find if listener already exists and is contradictory to our URL
    // Beware: if it exists and is secure, first registered Certificate will be used!
    Listener* listener = m_queue->FindListener(port);
    if(listener && (listener->GetSecureMode() != url.m_secure))
    {
      return ERROR_ALREADY_EXISTS;
    }

    // Keep URL in our mapping
    m_urls.insert(std::make_pair(pFullyQualifiedUrl,url));

    // Listener already exists. Do NOT make a new one
    if(listener)
    {
      return NO_ERROR;
    }

    // Add listener to the request queue
    // Use the settings from this particular URL (but does not retain the URL)
    // And use the drain-entity body as the main timeout for the listener worker threads
    return m_queue->StartListener(port,&url,m_timeoutDrainEntityBody);
  }
  return result;
}

// Remove an URL from an URL-Group. 
// If it was the last URL, remove the group from the request-queue
ULONG 
UrlGroup::DelUrlPrefix(HTTP_URL_GROUP_ID p_handle,XString pFullyQualifiedUrl,ULONG p_flags)
{
  AutoCritSec lock(&m_lock);

  XString path(pFullyQualifiedUrl);
  path.MakeLower();

  URLNames::iterator it = m_urls.begin();
  while(it != m_urls.end())
  {
    if((p_flags == HTTP_URL_FLAG_REMOVE_ALL) || path.Compare(it->first) == 0)
    {
      // Remove listener from the request queue
      m_queue->StopListener(it->second.m_port);

      // Remove the URL first
      // delete url;
      // Now remove the URL
      it = m_urls.erase(it);
    }
    else ++it;
  }

  // If no URL's left in the group, remove the group from the queue and the session
  if(m_urls.empty())
  {
    g_session->RemoveUrlGroup(p_handle,this);
  }
  return NO_ERROR;
}

// Counts the number of URL's with this port number in the group
ULONG 
UrlGroup::NumberOfPorts(USHORT p_port)
{
  AutoCritSec lock(&m_lock);

  ULONG total = 0;

  for(auto& url : m_urls)
  {
    if(url.second.m_port == p_port)
    {
      ++total;
    }
  }
  return total;
}

// Find best fitting URL through the longest match algorithm
// Optimized for segmented compare of site names
URL*
UrlGroup::FindLongestURL(USHORT p_port,XString p_abspath,int& p_length)
{
  AutoCritSec lock(&m_lock);

  URL* longesturl = nullptr;
  int  longest = 0;
  
  for(auto& url : m_urls)
  {
    // Other ports now? Stop searching.
    if(p_port != url.second.m_port)
    {
      continue;
    }

    // Find longest match
    int length = SegmentedCompare(p_abspath,url.second.m_abspath);

    // Keep longest matching URL
    if(length > longest)
    {
      longesturl = &url.second;
      longest    = length;
    }
    // Cannot get any longer than this, so we're done
    if(length == p_abspath.GetLength())
    {
      break;
    }
  }
  // What we found
  p_length = longest;
  return longesturl;
}

// Just copy the properties. 
void 
UrlGroup::SetAuthentication(ULONG p_scheme, XString p_domain, XString p_realm, bool p_caching)
{
  m_scheme = p_scheme;
  m_domain = p_domain;
  m_realm  = p_realm;
  m_ntlmCaching = p_caching;
}

void 
UrlGroup::SetAuthenticationWide(wstring p_domain, wstring p_realm)
{
  m_domainWide = p_domain;
  m_realmWide  = p_realm;
}

//////////////////////////////////////////////////////////////////////////
//
// PRIVATE
//
//////////////////////////////////////////////////////////////////////////

// Compare two absolute URL paths by determining the '/segment/' parts
// Optimizes for string searches where sizes of segments differ.
int
UrlGroup::SegmentedCompare(LPCTSTR p_left,LPCTSTR p_right)
{
  // Both parameters must be given
  if(p_left == nullptr || p_right == nullptr)
  {
    return 0;
  }
  // Absolute paths must start with a '/'
  if (p_left[0] != '/' || p_right[0] != '/')
  {
    return 0;
  }
  int index   = 1; // Current segment index in the path
  int longest = 0; // Longest found match

  while(true)
  {
    // Find next '/' for the end of the segment
    LPCTSTR posleft  = _tcschr(&p_left [index],'/');
    LPCTSTR posright = _tcschr(&p_right[index],'/');

    // one of the two or both ends here
    if(posleft == nullptr || posright == nullptr)
    {
      break;
    }
    // Find length of segments
    int lenleft  = (int)(((ULONGLONG)posleft  - (ULONGLONG)p_left)  / sizeof(TCHAR)) + 1;
    int lenright = (int)(((ULONGLONG)posright - (ULONGLONG)p_right) / sizeof(TCHAR)) + 1;

    // Different lengths of segments: stop the comparison
    if(lenleft != lenright)
    {
      break;
    }
    // Compare next segment in the absolute URL paths up to the matching segment length
    // Does the smallest possible string compare by just comparing one segment
    if(_tcsnicmp(&p_left[longest],&p_right[longest],lenleft - longest))
    {
      // NON ZERO result -> Not equal: stop here!
      break;
    }
    longest = lenleft;      // New longest match
    index   = lenleft + 1;  // Restart search on next segment
  }
  return longest;
}

// Find out if our URL is registered in the MS-Windows registry
// As created and maintained by the 'netsh' application
bool
UrlGroup::UrlIsRegistered(XString pFullyQualifiedUrl)
{
  bool result = false;

  // Get the absolute prefix path
  XString url(pFullyQualifiedUrl);
  int pos = pFullyQualifiedUrl.Find('?');
  if (pos > 0)
  {
    url = url.Left(pos);
  }

  while(true)
  {
    XString value1;
    DWORD   value2 = 0;
    TCHAR   value3[BUFF_LEN];
    DWORD   size3 = BUFF_LEN;

    if(HTTPReadRegister(_T("UrlAclInfo"),url,REG_BINARY,value1,&value2,value3,&size3))
    {
      // Allowed
      result = true;
      break;
    }

    // Trim off one level, and see if we have a registration for that!
    url.TrimRight('/');
    pos = url.ReverseFind('/');
    if (pos > 0)
    {
      url = url.Left(pos + 1);
    }
    if(url.GetLength() < 7)
    {
      break;
    }
  }
  return result;
}

// Read all settings of an URL from the MS-Windows registry
// These are the settings for secure HTTPS (SSL/TLS)
bool
UrlGroup::GetURLSettings(URL& p_url)
{
  XString value1;
  DWORD   value2 = 0;
  TCHAR   value3 [BUFF_LEN];
  DWORD   size3 = BUFF_LEN;

  // Reset the URL
  ZeroMemory(p_url.m_thumbprint,CERT_THUMBPRINT_SIZE + 1);
  p_url.m_requestClientCert = false;

  // Construct the sectie
  XString sectie;
  sectie.Format(_T("SslBindingInfo\\0.0.0.0:%u"),p_url.m_port);

  if(HTTPReadRegister(sectie,_T("SslCertHash"),REG_BINARY,value1,&value2,value3,&size3))
  {
    memcpy_s(p_url.m_thumbprint,CERT_THUMBPRINT_SIZE,value3,CERT_THUMBPRINT_SIZE);
    p_url.m_thumbprint[CERT_THUMBPRINT_SIZE] = 0;
  }
  else return false;

  if(HTTPReadRegister(sectie,_T("SslCertStoreName"),REG_SZ,value1,&value2,value3,&size3))
  {
    p_url.m_certStoreName = value1;
  }
  else return false;

  if(HTTPReadRegister(sectie,_T("DefaultFlags"),REG_DWORD,value1,&value2,value3,&size3))
  {
    if (value2 == 0x02)
    {
      p_url.m_requestClientCert = true;
    }
  }
  return true;
}

