//////////////////////////////////////////////////////////////////////////
//
// USER-SPACE IMPLEMENTTION OF HTTP.SYS
//
// 2018 - 2024 (c) ir. W.E. Huisman
// License: MIT
//
//////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "GetUserAccount.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

XString 
HTTP_GetUserAccount(EXTENDED_NAME_FORMAT p_format /*=NameUnknown*/,bool p_normalized /*=false*/)
{
  BOOL  ret = FALSE;
  TCHAR name[MAX_USER_NAME + 1] = _T("");
  unsigned long lengte = MAX_USER_NAME;

  if(p_format != NameUnknown)
  {
    ret = GetUserNameEx(p_format,name,&lengte); 
  }

  if(p_format == NameUnknown || ret == FALSE)
  {
    // Getting the account from the AD-Service
    ret = GetUserNameEx((EXTENDED_NAME_FORMAT)NameSamCompatible,name,&lengte); // SAM = "ORG\johndoe"
    if(ret == 0 || name[0] == 0)
    {
      ret = GetUserNameEx((EXTENDED_NAME_FORMAT)NameUserPrincipal,name,&lengte); // UPN = "jdoe@organization.com"
      if(ret == 0 || name[0] == 0)
      {
        // Last fallback to local user name
        ret = GetUserName(name,&lengte); // WIN = "johndoe"
      }
    }
  }
  if(ret == 0 || name[0] == 0)
  {
    // No ADS or no SAM/WIN32 account
    // or getting the VERY LONG name from the AD?
    return XString(_T("GUEST"));
  }
  // Remember our user
  XString user(name);

  // De-normalize the form for usage in e.g. filenames
  if(p_normalized)
  {
    // EXAMPLES FROM <secext.h>
    // 
    // CN=John Doe, OU=Software, OU=Engineering, O=Widget, C=US
    // Engineering\JohnDoe
    // eg: {4fa050f0-f561-11cf-bdd9-00aa003a77b6}
    // engineering.widget.com/software/John Doe
    // someone@example.com
    // eg: engineering.widget.com/software\nJohn Doe
    // www/srv.engineering.com/engineering.com
    // eg: engineering.widget.com\JohnDoe
    user.Replace(_T("\\"),_T("_"));
    user.Replace(_T("\n"),_T("_"));
    user.Replace(_T("/"), _T("_"));
    user.Replace(_T("="), _T("_"));
    user.Replace(_T("."), _T("_"));
    user.Replace(_T(","), _T("_"));
    user.Replace(_T("{"), _T("_"));
    user.Replace(_T("}"), _T("_"));
    user.Replace(_T(" "), _T("_"));
    user.Replace(_T("@"), _T("_"));
  }
  return user;
}
