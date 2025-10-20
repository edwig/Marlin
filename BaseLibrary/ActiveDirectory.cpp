/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: ActiveDirectory.cpp
//
// BaseLibrary: Indispensable general objects and functions
// 
// Created: 2014-2025 ir. W.E. Huisman
// MIT License
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
#include "ActiveDirectory.h"
#include "JSONMessage.h"
#include "JSONPath.h"
#include "StringUtilities.h"

#define SECURITY_WIN32
#include <sspi.h>
#include <secext.h>

// ADSI headers
#include <comdef.h>
#include <objbase.h>
#include <activeds.h>

// Needed libraries
#pragma comment(lib,"ActiveDS")
#pragma comment(lib,"AdsIID")
#pragma comment(lib,"Secur32")

_COM_SMARTPTR_TYPEDEF(IADsADSystemInfo,__uuidof(IADsADSystemInfo));
_COM_SMARTPTR_TYPEDEF(IADs,__uuidof(IADs));

// General COM object on the stack
class CoInit
{
public:
  CoInit()
  {
    std::ignore = CoInitialize(nullptr);
  }
  ~CoInit()
  {
    CoUninitialize();
  }
};

// Getting the Distinguished Name from the current Windows user
//
static XString DistinguishedName()
{
  HRESULT hr;

  // Init the ADS object
  CoInit coInit;
  IADsADSystemInfoPtr pADsys;
  hr = CoCreateInstance(CLSID_ADSystemInfo,
                        nullptr,
                        CLSCTX_INPROC_SERVER,
                        IID_IADsADSystemInfo,
                        (void**)&pADsys);
  if(FAILED(hr))
  {
    return _T("");
  }

  // Getting the DN
  BSTR str;
  if(SUCCEEDED(pADsys->get_UserName(&str)))
  {
    // Copy the name
    XString dn((LPCWSTR)str);
    SysFreeString(str);
    return dn;
  }
  return _T("");
}

// Read a user attribute from the ADSI
//
static XString
ReadAttribute(const XString& dn,const XString& attribute)
{
  HRESULT hr;
  USES_CONVERSION;

  // Initialize the COM subsystem
  CoInit coInit;

  // Create an LDAP query
  std::wstring query = L"LDAP://";
  query += CT2W(dn);

  // Open the LDAP object
  IADsPtr pAds;
  hr = ADsGetObject(query.c_str(),IID_IADs,(void**)&pAds);
  if(FAILED(hr))
  {
    return _T("");
  }

  // Prepare variant
  CComBSTR    attrib(attribute);
  CComVariant var;

  // Read the attribute
  hr = pAds->Get(attrib,&var);

  if(SUCCEEDED(hr))
  {
    XString result = CW2T(var.bstrVal);
    return result;
  }
  // Failure
  return _T("");
}

XString PrimaryOfficeUser()
{
  HKEY   hk(nullptr);
  TCHAR  szBuf[8 * MAX_PATH + 1] = _T("");
  XString mail;

  if(RegOpenKeyEx(HKEY_CURRENT_USER,_T("Software\\Microsoft\\IdentityCRL\\UserExtendedProperties"),0,KEY_READ,&hk))
  {
    // Key could not be opened
    return _T("");
  }

  DWORD size = MAX_PATH;
  FILETIME ftLastWriteTime;
  memset(szBuf,0,MAX_PATH);

  DWORD retCode = RegEnumKeyEx(hk,
                               0,
                               szBuf,
                               &size,
                               NULL,
                               NULL,
                               NULL,
                               &ftLastWriteTime);

  if(retCode == ERROR_SUCCESS)
  {
    mail = szBuf;
  }
  RegCloseKey(hk);
  return mail;
}

static XString organization;

// Gets the users email address from the AD
// If not connected to the AD, it will retrieve the primary MS-Office mail address
XString GetUserMailaddress()
{
  XString user = DistinguishedName();
  XString mail = ReadAttribute(user,_T("mail"));
  if(mail.IsEmpty())
  {
    mail = ReadAttribute(user,_T("proxyAddresses"));
  }

  // See if multiple addresses are loaded
  // If so, just use the very first email address
  std::vector<XString> addresses;
  SplitString(mail,addresses,_T(';'));
  for(auto& address : addresses)
  {
    // Strip the fact that it is a SMTP proxy address.
    // "SMTP:" is the primary email address
    // "smtp:" is a secondary email address
    if(address.Left(5).Compare(_T("SMTP:")) == 0)
    {
      mail = address.Mid(5);
      break;
    }
    // Preset a secondary email address, in case we do not find the primary
    if(address.Left(5).Compare(_T("smtp:")) == 0)
    {
      mail = address.Mid(5);
    }
  }

  // Possibly no Active-Directory user.
  // Try to find an office outlook user
  if(mail.IsEmpty())
  {
    mail = PrimaryOfficeUser();
  }

  // Find the organization
  organization.Empty();
  int pos = mail.Find('@');
  if(pos > 0)
  {
    organization = mail.Mid(pos + 1);
  }
  return mail;
}

// Getting the organization name according to the AD
XString GetADOrganization()
{
  if(organization.IsEmpty())
  {
    GetUserMailaddress();
  }
  return organization;
}

// Returns <domain>\<usercode>
XString GetUserLogincode()
{
  TCHAR name[MAX_PATH + 1];
  unsigned long lengte = MAX_PATH;
  GetUserNameEx(NameSamCompatible,name,&lengte);

  return XString(name);
}

// Returns <user>@organisation.com (mostly!)
XString GetUserPrincipalName()
{
  TCHAR name[MAX_PATH + 1];
  unsigned long lengte = MAX_PATH;
  GetUserNameEx(NameUserPrincipal,name,&lengte);

  return XString(name);
}
