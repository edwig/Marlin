//////////////////////////////////////////////////////////////////////////
//
// USER-SPACE IMPLEMENTTION OF HTTP.SYS
//
// 2018 (c) ir. W.E. Huisman
// License: MIT
//
//////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "http_private.h"
#include "URL.h"
#include "RequestQueue.h"
#include "UrlGroup.h"

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
    m_queue = (RequestQueue*)p_requestQueue;
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
UrlGroup::AddUrlPrefix(CString pFullyQualifiedUrl,HTTP_URL_CONTEXT UrlContext)
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
    CString name = url.m_abspath;
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
UrlGroup::DelUrlPrefix(CString pFullyQualifiedUrl,ULONG p_flags)
{
  AutoCritSec lock(&m_lock);

  CString path(pFullyQualifiedUrl);
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

  // If no URL's left in the group, remove the group from the queue
  if(m_urls.empty())
  {
    m_queue->RemoveURLGroup(this);
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
UrlGroup::FindLongestURL(USHORT p_port,CString p_abspath,int& p_length)
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
UrlGroup::SetAuthentication(ULONG p_scheme, CString p_domain, CString p_realm, bool p_caching)
{
  m_scheme = p_scheme;
  m_domain = p_domain;
  m_realm  = p_realm;
  m_ntlmCaching = p_caching;
}

//////////////////////////////////////////////////////////////////////////
//
// PRIVATE
//
//////////////////////////////////////////////////////////////////////////

// Compare two absolute URL paths by determining the '/segment/' parts
// Optimizes for string searches where sizes of segments differ.
int
UrlGroup::SegmentedCompare(const char* p_left,const char* p_right)
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
    const char* posleft  = strchr(&p_left [index],'/');
    const char* posright = strchr(&p_right[index],'/');

    // one of the two or both ends here
    if(posleft == nullptr || posright == nullptr)
    {
      break;
    }
    // Find length of segments
    int lenleft  = (int)((ULONGLONG)posleft  - (ULONGLONG)p_left)  + 1;
    int lenright = (int)((ULONGLONG)posright - (ULONGLONG)p_right) + 1;

    // Different lengths of segments: stop the comparison
    if(lenleft != lenright)
    {
      break;
    }
    // Compare next segment in the absolute URL paths up to the matching segment length
    // Does the smallest possible string compare by just comparing one segment
    if(_strnicmp(&p_left[longest],&p_right[longest],lenleft - longest))
    {
      // NON ZERO result -> Not equal: stop here!
      break;
    }
    longest = lenleft;      // New longest match
    index   = lenleft + 1;  // Restart search on next segment
  }
  return longest;
}

#define BUFF_LEN 1024

// Find out if our URL is registered in the MS-Windows registry
// As created and maintained by the 'netsh' application
bool
UrlGroup::UrlIsRegistered(CString pFullyQualifiedUrl)
{
  bool result = false;

  // Get the absolute prefix path
  CString url(pFullyQualifiedUrl);
  int pos = pFullyQualifiedUrl.Find('?');
  if (pos > 0)
  {
    url = url.Left(pos);
  }

  while(true)
  {
    CString value1;
    DWORD   value2 = 0;
    TCHAR   value3[BUFF_LEN];
    DWORD   size3 = BUFF_LEN;

    if(ReadRegister("UrlAclInfo",url,REG_BINARY,value1,&value2,value3,&size3))
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
  }
  return result;
}

// Read all settings of an URL from the MS-Windows registry
// These are the settings for secure HTTPS (SSL/TLS)
bool
UrlGroup::GetURLSettings(URL& p_url)
{
  CString value1;
  DWORD   value2 = 0;
  TCHAR   value3 [BUFF_LEN];
  DWORD   size3 = BUFF_LEN;

  // Reset the URL
  ZeroMemory(p_url.m_thumbprint,CERT_THUMBPRINT_SIZE + 1);
  p_url.m_requestClientCert = false;

  // Construct the sectie
  CString sectie;
  sectie.Format("SslBindingInfo\\0.0.0.0:%u",p_url.m_port);

  if(ReadRegister(sectie,"SslCertHash",REG_BINARY,value1,&value2,value3,&size3))
  {
    memcpy_s(p_url.m_thumbprint,CERT_THUMBPRINT_SIZE,value3,CERT_THUMBPRINT_SIZE);
    p_url.m_thumbprint[CERT_THUMBPRINT_SIZE] = 0;
  }
  else return false;

  if(ReadRegister(sectie,"SslCertStoreName",REG_SZ,value1,&value2,value3,&size3))
  {
    p_url.m_certStoreName = value1;
  }
  else return false;

  if(ReadRegister(sectie,"DefaultFlags",REG_DWORD,value1,&value2,value3,&size3))
  {
    if (value2 == 0x02)
    {
      p_url.m_requestClientCert = true;
    }
  }
  return true;
}

// Read one value from the registry
// Special function to read HTTP parameters
bool
UrlGroup::ReadRegister(CString  p_sectie,CString p_key,DWORD p_type
                      ,CString& p_value1
                      ,PDWORD   p_value2
                      ,PTCHAR   p_value3,PDWORD p_size3)
{
  HKEY    hkUserURL;
  bool    result = false;
  CString key = "SYSTEM\\ControlSet001\\Services\\HTTP\\Parameters\\" + p_sectie;

  DWORD dwErr = RegOpenKeyEx(HKEY_LOCAL_MACHINE
                            ,(LPCSTR) key
                            ,0
                            ,KEY_QUERY_VALUE
                            ,&hkUserURL);
  if(dwErr == ERROR_SUCCESS)
  {
    DWORD dwIndex = 0;
    DWORD dwType  = 0;
    TCHAR buffName[BUFF_LEN];
    BYTE  buffData[BUFF_LEN];

    //enumerate this key's values
    while(ERROR_SUCCESS == dwErr)
    {
      DWORD dwNameSize = BUFF_LEN;
      DWORD dwDataSize = BUFF_LEN;
      dwErr = RegEnumValue(hkUserURL
                          ,dwIndex++
                          ,buffName
                          ,&dwNameSize
                          ,NULL
                          ,&dwType
                          ,buffData
                          ,&dwDataSize);
      if(dwErr == ERROR_SUCCESS && dwType == REG_SZ && p_type == dwType)
      {
        if(p_key.CompareNoCase(buffName) == 0)
        {
          p_value1 = buffData;
          result   = true;
          break;
        }
      }
      if(dwErr == ERROR_SUCCESS && dwType == REG_DWORD && p_type == dwType)
      {
        if(p_key.CompareNoCase(buffName) == 0)
        {
          *p_value2 = *((PDWORD)buffData);
          result    = true;
          break;
        }
      }
      if(dwErr == ERROR_SUCCESS && dwType == REG_BINARY && p_type == dwType)
      {
        if(p_key.CompareNoCase(buffName) == 0)
        {
          if(*p_size3 >= dwDataSize)
          {
            memcpy_s(p_value3,*p_size3,buffData,dwDataSize);
            *p_size3 = dwDataSize;
            result   = true;
            break;
          }
        }
      }
    }
    RegCloseKey(hkUserURL);
  }
  return result;
}
