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

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// Splits a URL-Prefix string into an URL object (no context yet)
//
ULONG
SplitURLPrefix(CString p_fullprefix,PURL p_url)
{
  // Check that we are called correctly
  if(p_fullprefix.IsEmpty() || p_url == nullptr)
  {
    return ERROR_INVALID_PARAMETER;
  }

  // Reset the URL
  p_url->m_type     = UrlPrefixType::URLPRE_Nothing;
  p_url->m_secure   = false;
  p_url->m_port     = 0;
  p_url->m_context  = 0L;
  p_url->m_host.Empty();
  p_url->m_abspath.Empty();

  // Take copy of the string to work on
  CString prefix(p_fullprefix);

  // Check if it begins with HTTP
  if(prefix.Left(4).CompareNoCase("http"))
  {
    return ERROR_INVALID_PARAMETER;
  }
  prefix = prefix.Mid(4);

  // Check if we are secure
  if(prefix.Left(1).CompareNoCase("s") == 0)
  {
    p_url->m_secure = true;
    prefix = prefix.Mid(1);
  }

  // Check that we now find "://"
  if(prefix.Left(3).Compare("://"))
  {
    return ERROR_INVALID_PARAMETER;
  }
  prefix = prefix.Mid(3);

  // Check for the type
  int posport = prefix.Find(':');
  int pospath = prefix.Find('/');

  // FInd host binding
  CString host;
  if(posport >= 0)
  {
    host   = prefix.Left(posport);
    prefix = prefix.Mid(posport);
  }
  else if (pospath >= 0)
  {
    host   = prefix.Left(pospath);
    prefix = prefix.Mid(pospath);
  }
  else
  {
    return ERROR_INVALID_PARAMETER;
  }

  if (host == "+")
  {
    p_url->m_type = UrlPrefixType::URLPRE_Strong;
  }
  else if (host == "*")
  {
    p_url->m_type = UrlPrefixType::URLPRE_Weak;
  }
  else if(isdigit(host.GetAt(0)))
  {
    p_url->m_type = UrlPrefixType::URLPRE_Address;
    p_url->m_host = host;
  }
  else if (host.Find('.') >= 0)
  {
    p_url->m_type = UrlPrefixType::URLPRE_FQN;
    p_url->m_host = host;
  }
  else // short host name
  {
    p_url->m_type = UrlPrefixType::URLPRE_Named;
    p_url->m_host = host;
  }

  // See if different port
  if(prefix.GetAt(0) == ':')
  {
    p_url->m_port = (USHORT)atoi(prefix.Mid(1));
  }
  else
  {
    // Use default INTERNET ports
    p_url->m_port = p_url->m_secure ? INTERNET_DEFAULT_HTTPS_PORT : INTERNET_DEFAULT_HTTP_PORT;
  }

  // Copy the resulting absolute path name
  p_url->m_abspath = prefix.Mid(pospath - 1);

  // Ready
  return NO_ERROR;
}
