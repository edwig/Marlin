/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: PrintToken.cpp
//
// BaseLibrary: Indispensable general objects and functions
// 
// Copyright (c) 2014-2025 ir. W.E. Huisman
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
// Code to dump out a token on Windows
// Original code from: // Matt Conover (shok@dataforce.net)
// See: http://www.w00w00.org
//
#include "pch.h"
#include "BaseLibrary.h"
#include "PrintToken.h"
#include <windows.h>
#include <aclapi.h>
#include <accctrl.h>
#include <stdio.h>
#include <assert.h>
#include <tchar.h>

#ifdef _AFX
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#endif

static void
PrintSidUse(XString& p_list,SID_NAME_USE p_sidType)
{
  switch (p_sidType)
  {
    case SidTypeUser:           p_list += _T("(user)\n");
                                break;
    case SidTypeGroup:          p_list += _T("(group)\n");
                                break;
    case SidTypeDomain:         p_list += _T("(domain)\n");
                                break;
    case SidTypeAlias:          p_list += _T("(alias)\n");
                                break;
    case SidTypeWellKnownGroup: p_list += _T("(well-known group)\n");
                                break;
    case SidTypeDeletedAccount: p_list += _T("(deleted account)\n");
                                break;
    case SidTypeInvalid:        p_list += _T("(invalid)\n");
                                break;
    case SidTypeUnknown:        p_list += _T("(unknown)\n");
                                break;
    case SidTypeComputer:       p_list += _T("(computer)\n");
                                break;
  }
}

// Print Authentication token to a string list
BOOL DumpToken(XString& p_list,HANDLE p_token)
{
  unsigned int  i;
  XString       text;
  DWORD         size;
  DWORD         userSize;
  DWORD         domainSize;
  SID*          sid;
  SID_NAME_USE  sidType;
  TCHAR         userName[64];
  TCHAR         domainName[64];
  TCHAR         privilegeName[64];

  DWORD                 sessionID = 0;
  TOKEN_TYPE            type;
  TOKEN_STATISTICS*     statistics;
  TOKEN_SOURCE          source;
  TOKEN_OWNER*          owner;
  TOKEN_USER*           user;
  TOKEN_PRIMARY_GROUP*  primaryGroup;
  TOKEN_DEFAULT_DACL*   defaultDacl;
  TOKEN_PRIVILEGES*     privileges;
  TOKEN_GROUPS*         groups;
  SECURITY_IMPERSONATION_LEVEL impersonationLevel;

  DWORD                 entryCount;
  EXPLICIT_ACCESS*      explicitEntries;
  EXPLICIT_ACCESS*      explicitEntry;

  XString lijn(_T("--------------------------------------------------------------\n"));

  memset(privilegeName, 0, sizeof(privilegeName));
  memset(userName,      0, sizeof(userName));
  memset(domainName,    0, sizeof(domainName));

  p_list += lijn;
  text.Format(_T("This is a %s token\n"), IsTokenRestricted(p_token) ? _T("restricted") :_T("unrestricted"));
  p_list += text;

  /////////////////////////////////////////////////////////////////
  // Dump token type

  size = 0;
  GetTokenInformation(p_token, TokenType, &type, sizeof(TOKEN_TYPE), &size);
  if (!size)
  {
    text.Format(_T("Error getting token type: error code 0x%lx\n"), GetLastError());
    p_list += text;
    return FALSE;
  }

  p_list += _T("Token type: ");
  if (type == TokenPrimary) 
  {
    p_list += _T("primary\n");
  }
  else 
  {
    p_list += _T("impersonation\n");
  }
  if (type == TokenImpersonation)
  {
    size = 0;
    if (!GetTokenInformation(p_token, TokenImpersonationLevel, &impersonationLevel, sizeof(SECURITY_IMPERSONATION_LEVEL), &size) || !size)
    {
      text.Format(_T("Error getting impersonation level: error code 0x%lx\n"), GetLastError());
      p_list += text;
      return FALSE;
    }

    p_list += _T("Impersonation level: ");
    switch (impersonationLevel)
    {
      case SecurityAnonymous:     p_list += _T("anonymous\n");
                                  break;
      case SecurityIdentification:p_list += _T("identification\n");
                                  break;
      case SecurityImpersonation: p_list += _T("impersonation\n");
                                  break;
      case SecurityDelegation:    p_list += _T("delegation\n");
                                  break;
    }
  }

  /////////////////////////////////////////////////////////////////
  // Dump the token IDs

  // Get the Token and Authentication IDs
  size = 0;
  GetTokenInformation(p_token, TokenStatistics, NULL, 0, &size);
  if (!size)
  {
    text.Format(_T("Error getting token statistics: error code 0x%lx\n"), GetLastError());
    p_list += text;
    return FALSE;
  }
  
  statistics = (TOKEN_STATISTICS *)new uchar[size];
  GetTokenInformation(p_token, TokenStatistics, statistics, size, &size);
  assert(size);
  text.Format(_T("Token ID: 0x%lX\n"), statistics->TokenId.LowPart);
  p_list += text;
  text.Format(_T("Authentication ID: 0x%lX\n"), statistics->AuthenticationId.LowPart);
  p_list += text;
  delete [] statistics;

  // Get the Session ID
  size = 0;
  if (!GetTokenInformation(p_token, TokenSessionId, &sessionID, sizeof(sessionID), &size) || !size)
  {
    text.Format(_T("Error getting the Session ID: error code 0x%lx\n"), GetLastError());
    p_list += text;
    return FALSE;
  }
  if (sessionID) 
  {
    text.Format(_T("Session ID = 0x%lx\n"), sessionID);
    p_list += text;
  }
  /////////////////////////////////////////////////////////////////
  // Dump token owner

  size = 0;
  GetTokenInformation(p_token, TokenOwner, NULL, 0, &size);
  if (!size)
  {
    text.Format(_T("Error getting token owner: error code0x%lx\n"), GetLastError());
    p_list += text;
    return FALSE;
  }
  
  owner = (TOKEN_OWNER *)new uchar[size];
  GetTokenInformation(p_token, TokenOwner, owner, size, &size);

  size = GetLengthSid(owner->Owner);
  assert(size);
  
  sid = (SID *) new uchar[size];

  CopySid(size, sid, owner->Owner);

    userSize = (sizeof userName / sizeof *userName) - 1;
  domainSize = (sizeof domainName / sizeof *domainName) - 1;
  LookupAccountSid(NULL, sid, userName, &userSize, domainName, &domainSize, &sidType);
  delete [] sid;

  text.Format(_T("Token's owner: %s\\%s "), domainName, userName);
  p_list += text;
  PrintSidUse(p_list,sidType);

  delete [] owner;

  /////////////////////////////////////////////////////////////////
  // Dump token source

  size = 0;
  if (!GetTokenInformation(p_token, TokenSource, &source, sizeof(TOKEN_SOURCE), &size) || !size)
  {
    text.Format(_T("Cannot get the token source: error code 0x%lx\n"), GetLastError());
    p_list += text;
  }
  else
  {
    p_list += _T("Token's source: ");
    for(i = 0; i < 8 && source.SourceName[i]; i++)
    {
      text.Format(_T("%c"),source.SourceName[i]);
      p_list += text;
    }
    text.Format(_T(" (0x%lx)\n"),source.SourceIdentifier.LowPart);
    p_list += text;
  }
  /////////////////////////////////////////////////////////////////
  // Dump token user

  size = 0;
  GetTokenInformation(p_token, TokenUser, NULL, 0, &size);
  if (!size)
  {
    text.Format(_T("Error getting token user: error code 0x%lx\n"), GetLastError());
    p_list += text;
    return FALSE;
  }

  user = (TOKEN_USER *)new uchar[size];
  GetTokenInformation(p_token, TokenUser, user, size, &size);
  assert(size);

  size = GetLengthSid(user->User.Sid);
  assert(size);

  sid = (SID *)new uchar[size];

  CopySid(size, sid, user->User.Sid);
  userSize = (sizeof userName / sizeof *userName) - 1;
  domainSize = (sizeof domainName / sizeof *domainName) - 1;
  LookupAccountSid(NULL, sid, userName, &userSize, domainName, &domainSize, &sidType);
  delete [] sid;

  text.Format(_T("Token's user: %s\\%s "), domainName, userName);
  p_list += text;
  PrintSidUse(p_list,sidType);

  delete [] user;

  /////////////////////////////////////////////////////////////////
  // Primary group

  size = 0;
  GetTokenInformation(p_token, TokenPrimaryGroup, NULL, 0, &size);
  if (!size)
  {
    text.Format(_T("Error getting primary group: error code 0x%lx\n"), GetLastError());
    p_list += text;
    return FALSE;
  }

  primaryGroup = (TOKEN_PRIMARY_GROUP *)new uchar[size];
  GetTokenInformation(p_token, TokenPrimaryGroup, primaryGroup, size, &size);
  assert(size);

  size = GetLengthSid(primaryGroup->PrimaryGroup);
  assert(size);

  sid = (SID *) new uchar[size];
  CopySid(size, sid, primaryGroup->PrimaryGroup);

    userSize = (sizeof userName / sizeof *userName)-1;
  domainSize = (sizeof domainName / sizeof *domainName) - 1;
  
  LookupAccountSid(NULL, sid, userName, &userSize, domainName, &domainSize, &sidType);
  delete [] sid;

  text.Format(_T("Token's primary group: %s\\%s "), domainName, userName);
  p_list += text;
  PrintSidUse(p_list,sidType);
  
  delete [] primaryGroup;

  /////////////////////////////////////////////////////////////////
  // Dump default DACL
  
  size = 0;
  GetTokenInformation(p_token, TokenDefaultDacl, NULL, 0, &size);
  if (!size)
  {
    text.Format(_T("Error getting default DACL: error code 0x%lx\n"), GetLastError());
    p_list += text;
    return FALSE;
  }

  defaultDacl = (TOKEN_DEFAULT_DACL *) new uchar[size];
  GetTokenInformation(p_token, TokenDefaultDacl, defaultDacl, size, &size);
  assert(size);
  text.Format(_T("Default DACL (%d bytes):\n"), defaultDacl->DefaultDacl->AclSize);
  p_list += text;
  text.Format(_T("ACE count: %d\n"), defaultDacl->DefaultDacl->AceCount);
  p_list += text;
    
  if (GetExplicitEntriesFromAcl(defaultDacl->DefaultDacl, &entryCount, &explicitEntries) != ERROR_SUCCESS)
  {
    text.Format(_T("GetExplicitEntriesFromAcl failed: error code 0x%lX\n"), GetLastError());
    p_list += text;
    return FALSE;
  }

  for (i = 0, explicitEntry = explicitEntries; i < entryCount; i++, explicitEntry++)
  {
    text.Format(_T("ACE %u:\n"), i);
    p_list += text;
    
    p_list += _T("  Applies to: ");
    if (explicitEntry->Trustee.TrusteeForm == TRUSTEE_BAD_FORM) 
    {
      p_list += _T("trustee is in bad form\n");
    }
    else if (explicitEntry->Trustee.TrusteeForm == TRUSTEE_IS_NAME)  
    {
      text.Format(_T("%s "), explicitEntry->Trustee.ptstrName);
      p_list += text;
    }
    else if (explicitEntry->Trustee.TrusteeForm == TRUSTEE_IS_SID)
    {
      size = GetLengthSid((SID *)explicitEntry->Trustee.ptstrName);
      assert(size);
      sid = (SID *) new uchar[size];
      CopySid(size, sid, (SID *)explicitEntry->Trustee.ptstrName);
      userSize   = (sizeof userName   / sizeof *userName  ) - 1;
      domainSize = (sizeof domainName / sizeof *domainName) - 1;
      LookupAccountSid(NULL, sid, userName, &userSize, domainName, &domainSize, &sidType);
      delete [] sid;

      text.Format(_T("%s\\%s "), domainName, userName);
      p_list += text;
    }
    else
    {
      text.Format(_T("Unhandled trustee form %d\n"), explicitEntry->Trustee.TrusteeForm);
      p_list += text;
      return FALSE;
    }

    PrintSidUse(p_list,(SID_NAME_USE)explicitEntry->Trustee.TrusteeType);

    p_list += _T("\n");
    p_list += _T("  ACE inherited by: ");
    if(!explicitEntry->grfInheritance)                                      p_list += _T("not inheritable");
    if (explicitEntry->grfInheritance & CONTAINER_INHERIT_ACE)              p_list += _T("[containers] ");
    if (explicitEntry->grfInheritance & INHERIT_ONLY_ACE)                   p_list += _T("[inherited objects]");
    if (explicitEntry->grfInheritance & NO_PROPAGATE_INHERIT_ACE)           p_list += _T("[inheritance flags not propagated] ");
    if (explicitEntry->grfInheritance & OBJECT_INHERIT_ACE)                 p_list += _T("[objects] ");
    if (explicitEntry->grfInheritance & SUB_CONTAINERS_AND_OBJECTS_INHERIT) p_list += _T("[containers and objects] ");
    if (explicitEntry->grfInheritance & SUB_CONTAINERS_ONLY_INHERIT)        p_list += _T("[sub-containers] ");
    if (explicitEntry->grfInheritance & SUB_OBJECTS_ONLY_INHERIT)           p_list += _T("[sub-objects] ");
    p_list += _T("\n");
    
    text.Format(_T("  Access permission mask = 0x%08lx\n"), explicitEntry->grfAccessPermissions);
    p_list += text;
    p_list += _T("  Access mode: ");
    switch (explicitEntry->grfAccessMode)
    {
      case GRANT_ACCESS:      p_list += _T("grant access\n");
                              break;
      case SET_ACCESS:        p_list += _T("set access (discards any previous controls)\n");
                              break;
      case DENY_ACCESS:       p_list += _T("deny access\n");
                              break;
      case REVOKE_ACCESS:     p_list += _T("revoke access (discards any previous controls)\n");
                              break;
      case SET_AUDIT_SUCCESS: p_list += _T("generate success audit event\n");
                              break;
      case SET_AUDIT_FAILURE: p_list += _T("generate failure audit event\n");
                              break;
    }
  }

  LocalFree(explicitEntries);
  delete [] defaultDacl;

  /////////////////////////////////////////////////////////////////
  // Dump privileges

  size = 0;
  GetTokenInformation(p_token, TokenPrivileges, NULL, 0, &size);
  if (!size)
  {
    text.Format(_T("Error getting token privileges: error code 0x%lx\n"), GetLastError());
    p_list += text;
    return FALSE;
  }

  privileges = (TOKEN_PRIVILEGES *) new uchar[size];
  GetTokenInformation(p_token, TokenPrivileges, privileges, size, &size);
  assert(size);

  if (privileges->PrivilegeCount) 
  {
    text.Format(_T("Token's privileges (%d total):\n"), privileges->PrivilegeCount);
    p_list += text;
  }
  for (i = 0; i < privileges->PrivilegeCount; i++)
  {
    size = (sizeof privilegeName / sizeof *privilegeName) - 1;
    LookupPrivilegeName(NULL, &privileges->Privileges[i].Luid, privilegeName, &size);

    text.Format(_T("  %s (0x%lx) = "), privilegeName, privileges->Privileges[i].Luid.LowPart);
    p_list += text;
    if (!privileges->Privileges[i].Attributes) 
    {
      p_list += _T("disabled");
    }
    if (privileges->Privileges[i].Attributes & SE_PRIVILEGE_ENABLED)
    {
      if (privileges->Privileges[i].Attributes & SE_PRIVILEGE_ENABLED_BY_DEFAULT) 
      {
        p_list += _T("[enabled by default] ");
      }
      else
      {
        p_list += _T("[enabled] ");
      }
    }
    if (privileges->Privileges[i].Attributes & SE_PRIVILEGE_USED_FOR_ACCESS) 
    {
      p_list += _T("used for access] ");
    }
    p_list += _T("\n");
  }
  
  delete [] privileges;

  /////////////////////////////////////////////////////////////////
  // Dump restricted SIDs

  size = 0;
  GetTokenInformation(p_token, TokenRestrictedSids, NULL, 0, &size);
  if (!size)
  {
    text.Format(_T("Error getting token restricted SIDs: error code 0x%lx\n"), GetLastError());
    p_list += text;
    return FALSE;
  }

  groups = (TOKEN_GROUPS *) new uchar[size];

  GetTokenInformation(p_token, TokenRestrictedSids, groups, size, &size);
  assert(size);

  if (groups->GroupCount)
  {
    text.Format(_T("Restricted SIDs (%d total):\n"), groups->GroupCount);
    p_list += text;
  }
  for (i = 0; i < groups->GroupCount; i++)
  {
    size = GetLengthSid(groups->Groups[i].Sid);
    assert(size);

    sid = (SID *) new uchar[size];
    CopySid(size, sid, groups->Groups[i].Sid);
    userSize = (sizeof userName / sizeof TCHAR) - 1;
    domainSize = (sizeof domainName / sizeof *domainName) - 1;
    LookupAccountSid(NULL, sid, userName, &userSize, domainName, &domainSize, &sidType);
    delete [] sid;

    text.Format(_T("  [%u] %s\\%s ("), i, domainName, userName);
    p_list += text;
    PrintSidUse(p_list,sidType);
    text.Format(_T("  [%u] Group is: "), i);
    p_list += text;
    if (!groups->Groups[i].Attributes) 
    {
      p_list += _T("disabled\n");
    }
    if (groups->Groups[i].Attributes & SE_GROUP_ENABLED)
    {
      if (groups->Groups[i].Attributes & SE_GROUP_ENABLED_BY_DEFAULT) 
      {
        p_list += _T("[enabled by default] ");
      }
      else 
      {
        p_list += _T("[enabled] ");
      }
    }
    if (groups->Groups[i].Attributes & SE_GROUP_LOGON_ID)           p_list += _T("[logon_id] ");
    if (groups->Groups[i].Attributes & SE_GROUP_MANDATORY)          p_list += _T("[mandatory] ");
    if (groups->Groups[i].Attributes & SE_GROUP_USE_FOR_DENY_ONLY)  p_list += _T("[used for deny only] ");
    p_list += _T("\n");
  }
  delete [] groups;
  p_list += lijn;
  return TRUE;
}

bool
GetTokenOwner(HANDLE p_token,XString& p_userName)
{
  DWORD       size = 0;
  TOKEN_USER* user = NULL;
  SID*         sid = NULL;
  SID_NAME_USE sidType;
  DWORD   userSize   = 0;
  DWORD   domainSize = 0;
  TCHAR   userName  [64 + 1];
  TCHAR   domainName[64 + 1];

  GetTokenInformation(p_token, TokenUser, NULL, 0, &size);
  if (!size)
  {
    return false;
  }

  user = (TOKEN_USER *) new uchar[size];
  GetTokenInformation(p_token, TokenUser, user, size, &size);
  assert(size);

  size = GetLengthSid(user->User.Sid);
  assert(size);

  sid = (SID *) new uchar[size];
  CopySid(size, sid, user->User.Sid);

  userSize   = (sizeof userName   / sizeof *userName)   - 1;
  domainSize = (sizeof domainName / sizeof *domainName) - 1;
  LookupAccountSid(NULL, sid, userName, &userSize, domainName, &domainSize, &sidType);

  delete [] sid;
  delete [] user;

  // Assign result
  p_userName = userName;
  return true;
}

