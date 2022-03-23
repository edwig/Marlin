/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: PrintToken.cpp
//
// BaseLibrary: Indispensable general objects and functions
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

static void
PrintSidUse(XString& p_list,SID_NAME_USE p_sidType)
{
  switch (p_sidType)
  {
    case SidTypeUser:           p_list += "(user)\n";
                                break;
    case SidTypeGroup:          p_list += "(group)\n";
                                break;
    case SidTypeDomain:         p_list += "(domain)\n";
                                break;
    case SidTypeAlias:          p_list += "(alias)\n";
                                break;
    case SidTypeWellKnownGroup: p_list += "(well-known group)\n";
                                break;
    case SidTypeDeletedAccount: p_list += "(deleted account)\n";
                                break;
    case SidTypeInvalid:        p_list += "(invalid)\n";
                                break;
    case SidTypeUnknown:        p_list += "(unknown)\n";
                                break;
    case SidTypeComputer:       p_list += "(computer)\n";
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

  XString lijn("--------------------------------------------------------------\n");

  memset(privilegeName, 0, sizeof(privilegeName));
  memset(userName,      0, sizeof(userName));
  memset(domainName,    0, sizeof(domainName));

  p_list += lijn;
  text.Format("This is a %s token\n", IsTokenRestricted(p_token) ? "restricted" : "unrestricted");
  p_list += text;

  /////////////////////////////////////////////////////////////////
  // Dump token type

  size = 0;
  GetTokenInformation(p_token, TokenType, &type, sizeof(TOKEN_TYPE), &size);
  if (!size)
  {
    text.Format("Error getting token type: error code 0x%lx\n", GetLastError());
    p_list += text;
    return FALSE;
  }

  p_list += "Token type: ";
  if (type == TokenPrimary) 
  {
    p_list += "primary\n";
  }
  else 
  {
    p_list += "impersonation\n";
  }
  if (type == TokenImpersonation)
  {
    size = 0;
    if (!GetTokenInformation(p_token, TokenImpersonationLevel, &impersonationLevel, sizeof(SECURITY_IMPERSONATION_LEVEL), &size) || !size)
    {
      text.Format("Error getting impersonation level: error code 0x%lx\n", GetLastError());
      p_list += text;
      return FALSE;
    }

    p_list += "Impersonation level: ";
    switch (impersonationLevel)
    {
      case SecurityAnonymous:     p_list += "anonymous\n";
                                  break;
      case SecurityIdentification:p_list += "identification\n";
                                  break;
      case SecurityImpersonation: p_list += "impersonation\n";
                                  break;
      case SecurityDelegation:    p_list += "delegation\n";
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
    text.Format("Error getting token statistics: error code 0x%lx\n", GetLastError());
    p_list += text;
    return FALSE;
  }
  
  statistics = (TOKEN_STATISTICS *)new uchar[size];
  GetTokenInformation(p_token, TokenStatistics, statistics, size, &size);
  assert(size);
  text.Format("Token ID: 0x%lX\n", statistics->TokenId.LowPart);
  p_list += text;
  text.Format("Authentication ID: 0x%lX\n", statistics->AuthenticationId.LowPart);
  p_list += text;
  delete [] statistics;

  // Get the Session ID
  size = 0;
  if (!GetTokenInformation(p_token, TokenSessionId, &sessionID, sizeof(sessionID), &size) || !size)
  {
    text.Format("Error getting the Session ID: error code 0x%lx\n", GetLastError());
    p_list += text;
    return FALSE;
  }
  if (sessionID) 
  {
    text.Format("Session ID = 0x%lx\n", sessionID);
    p_list += text;
  }
  /////////////////////////////////////////////////////////////////
  // Dump token owner

  size = 0;
  GetTokenInformation(p_token, TokenOwner, NULL, 0, &size);
  if (!size)
  {
    text.Format("Error getting token owner: error code0x%lx\n", GetLastError());
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

  text.Format("Token's owner: %s\\%s ", domainName, userName);
  p_list += text;
  PrintSidUse(p_list,sidType);

  delete [] owner;

  /////////////////////////////////////////////////////////////////
  // Dump token source

  size = 0;
  if (!GetTokenInformation(p_token, TokenSource, &source, sizeof(TOKEN_SOURCE), &size) || !size)
  {
    text.Format("Error getting token source: error code 0x%lx\n", GetLastError());
    p_list += text;
    return FALSE;
  }

  p_list += "Token's source: ";
  for (i = 0; i < 8 && source.SourceName[i]; i++)
  {
    text.Format("%c", source.SourceName[i]);
    p_list += text;
  }
  text.Format(" (0x%lx)\n", source.SourceIdentifier.LowPart);
  p_list += text;

  /////////////////////////////////////////////////////////////////
  // Dump token user

  size = 0;
  GetTokenInformation(p_token, TokenUser, NULL, 0, &size);
  if (!size)
  {
    text.Format("Error getting token user: error code 0x%lx\n", GetLastError());
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

  text.Format("Token's user: %s\\%s ", domainName, userName);
  p_list += text;
  PrintSidUse(p_list,sidType);

  delete [] user;

  /////////////////////////////////////////////////////////////////
  // Primary group

  size = 0;
  GetTokenInformation(p_token, TokenPrimaryGroup, NULL, 0, &size);
  if (!size)
  {
    text.Format("Error getting primary group: error code 0x%lx\n", GetLastError());
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

  text.Format("Token's primary group: %s\\%s ", domainName, userName);
  p_list += text;
  PrintSidUse(p_list,sidType);
  
  delete [] primaryGroup;

  /////////////////////////////////////////////////////////////////
  // Dump default dacl
  
  size = 0;
  GetTokenInformation(p_token, TokenDefaultDacl, NULL, 0, &size);
  if (!size)
  {
    text.Format("Error getting default DACL: error code 0x%lx\n", GetLastError());
    p_list += text;
    return FALSE;
  }

  defaultDacl = (TOKEN_DEFAULT_DACL *) new uchar[size];
  GetTokenInformation(p_token, TokenDefaultDacl, defaultDacl, size, &size);
  assert(size);
  text.Format("Default DACL (%d bytes):\n", defaultDacl->DefaultDacl->AclSize);
  p_list += text;
  text.Format("ACE count: %d\n", defaultDacl->DefaultDacl->AceCount);
  p_list += text;
    
  if (GetExplicitEntriesFromAcl(defaultDacl->DefaultDacl, &entryCount, &explicitEntries) != ERROR_SUCCESS)
  {
    text.Format("GetExplicitEntriesFromAcl failed: error code 0x%lX\n", GetLastError());
    p_list += text;
    return FALSE;
  }

  for (i = 0, explicitEntry = explicitEntries; i < entryCount; i++, explicitEntry++)
  {
    text.Format("ACE %u:\n", i);
    p_list += text;
    
    p_list += "  Applies to: ";
    if (explicitEntry->Trustee.TrusteeForm == TRUSTEE_BAD_FORM) 
    {
      p_list += "trustee is in bad form\n";
    }
    else if (explicitEntry->Trustee.TrusteeForm == TRUSTEE_IS_NAME)  
    {
      text.Format("%s ", explicitEntry->Trustee.ptstrName);
      p_list += text;
    }
    else if (explicitEntry->Trustee.TrusteeForm == TRUSTEE_IS_SID)
    {
      size = GetLengthSid((SID *)explicitEntry->Trustee.ptstrName);
      assert(size);
      sid = (SID *) new uchar[size];
      CopySid(size, sid, (SID *)explicitEntry->Trustee.ptstrName);
      userSize = (sizeof userName / sizeof *userName)-1;
      domainSize = (sizeof domainName / sizeof *domainName) - 1;
      LookupAccountSid(NULL, sid, userName, &userSize, domainName, &domainSize, &sidType);
      delete [] sid;

      text.Format("%s\\%s ", domainName, userName);
      p_list += text;
    }
    else
    {
      text.Format("Unhandled trustee form %d\n", explicitEntry->Trustee.TrusteeForm);
      p_list += text;
      return FALSE;
    }

    PrintSidUse(p_list,(SID_NAME_USE)explicitEntry->Trustee.TrusteeType);

    p_list += "\n";
    p_list += "  ACE inherited by: ";
    if(!explicitEntry->grfInheritance)                                      p_list += "not inheritable";
    if (explicitEntry->grfInheritance & CONTAINER_INHERIT_ACE)              p_list += "[containers] ";
    if (explicitEntry->grfInheritance & INHERIT_ONLY_ACE)                   p_list += "[inherited objects]";
    if (explicitEntry->grfInheritance & NO_PROPAGATE_INHERIT_ACE)           p_list += "[inheritance flags not propagated] ";
    if (explicitEntry->grfInheritance & OBJECT_INHERIT_ACE)                 p_list += "[objects] ";
    if (explicitEntry->grfInheritance & SUB_CONTAINERS_AND_OBJECTS_INHERIT) p_list += "[containers and objects] ";
    if (explicitEntry->grfInheritance & SUB_CONTAINERS_ONLY_INHERIT)        p_list += "[sub-containers] ";
    if (explicitEntry->grfInheritance & SUB_OBJECTS_ONLY_INHERIT)           p_list += "[sub-objects] ";
    p_list += "\n";
    
    text.Format("  Access permission mask = 0x%08lx\n", explicitEntry->grfAccessPermissions);
    p_list += text;
    p_list += "  Access mode: ";
    switch (explicitEntry->grfAccessMode)
    {
      case GRANT_ACCESS:      p_list += "grant access\n";
                              break;
      case SET_ACCESS:        p_list += "set access (discards any previous controls)\n";
                              break;
      case DENY_ACCESS:       p_list += "deny access\n";
                              break;
      case REVOKE_ACCESS:     p_list += "revoke access (discards any previous controls)\n";
                              break;
      case SET_AUDIT_SUCCESS: p_list += "generate success audit event\n";
                              break;
      case SET_AUDIT_FAILURE: p_list += "generate failure audit event\n";
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
    text.Format("Error getting token privileges: error code 0x%lx\n", GetLastError());
    p_list += text;
    return FALSE;
  }

  privileges = (TOKEN_PRIVILEGES *) new uchar[size];
  GetTokenInformation(p_token, TokenPrivileges, privileges, size, &size);
  assert(size);

  if (privileges->PrivilegeCount) 
  {
    text.Format("Token's privileges (%d total):\n", privileges->PrivilegeCount);
    p_list += text;
  }
  for (i = 0; i < privileges->PrivilegeCount; i++)
  {
    size = (sizeof privilegeName / sizeof *privilegeName) - 1;
    LookupPrivilegeName(NULL, &privileges->Privileges[i].Luid, privilegeName, &size);

    text.Format("  %s (0x%lx) = ", privilegeName, privileges->Privileges[i].Luid.LowPart);
    p_list += text;
    if (!privileges->Privileges[i].Attributes) 
    {
      p_list += "disabled";
    }
    if (privileges->Privileges[i].Attributes & SE_PRIVILEGE_ENABLED)
    {
      if (privileges->Privileges[i].Attributes & SE_PRIVILEGE_ENABLED_BY_DEFAULT) 
      {
        p_list += "[enabled by default] ";
      }
      else
      {
        p_list += "[enabled] ";
      }
    }
    if (privileges->Privileges[i].Attributes & SE_PRIVILEGE_USED_FOR_ACCESS) 
    {
      p_list += "used for access] ";
    }
    p_list += "\n";
  }
  
  delete [] privileges;

  /////////////////////////////////////////////////////////////////
  // Dump restricted SIDs

  size = 0;
  GetTokenInformation(p_token, TokenRestrictedSids, NULL, 0, &size);
  if (!size)
  {
    text.Format("Error getting token restricted SIDs: error code 0x%lx\n", GetLastError());
    p_list += text;
    return FALSE;
  }

  groups = (TOKEN_GROUPS *) new uchar[size];

  GetTokenInformation(p_token, TokenRestrictedSids, groups, size, &size);
  assert(size);

  if (groups->GroupCount)
  {
    text.Format("Restricted SIDs (%d total):\n", groups->GroupCount);
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

    text.Format("  [%u] %s\\%s (", i, domainName, userName);
    p_list += text;
    PrintSidUse(p_list,sidType);
    text.Format("  [%u] Group is: ", i);
    p_list += text;
    if (!groups->Groups[i].Attributes) 
    {
      p_list += "disabled\n";
    }
    if (groups->Groups[i].Attributes & SE_GROUP_ENABLED)
    {
      if (groups->Groups[i].Attributes & SE_GROUP_ENABLED_BY_DEFAULT) 
      {
        p_list += "[enabled by default] ";
      }
      else 
      {
        p_list += "[enabled] ";
      }
    }
    if (groups->Groups[i].Attributes & SE_GROUP_LOGON_ID)           p_list += "[logon_id] ";
    if (groups->Groups[i].Attributes & SE_GROUP_MANDATORY)          p_list += "[mandatory] ";
    if (groups->Groups[i].Attributes & SE_GROUP_USE_FOR_DENY_ONLY)  p_list += "[used for deny only] ";
    p_list += "\n";
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

