/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: GetUserAccount.cpp
//
// BaseLibrary: Indispensable general objects and functions
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
#include "pch.h"
#include "BaseLibrary.h"
#include "GetUserAccount.h"

#ifdef _AFX
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#endif

XString
GetUserAccount(EXTENDED_NAME_FORMAT p_format /*=NameUnknown*/,bool p_normalized /*=false*/)
{
  BOOL ret = FALSE;
  TCHAR  name[MAX_USER_NAME + 1] = _T("");
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
