/////////////////////////////////////////////////////////////////////////////
// 
// SSL Helper functions
//
// Original idea:
// David Maw:      https://www.codeproject.com/Articles/1000189/A-Working-TCP-Client-and-Server-With-SSL
// Brandon Wilson: https://blogs.technet.microsoft.com/askpfeplat/2017/11/13/demystifying-schannel/
// License:        https://www.codeproject.com/info/cpol10.aspx
//
#include "stdafx.h"
#include "SSLUtilities.h"
#include "CreateCertificate.h"
#include "Logging.h"
#include <LogAnalysis.h>
#include <algorithm>
#include <WinDNS.h>
#include <iostream>
#include <wincrypt.h>

#pragma comment(lib, "Dnsapi.lib")

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// Miscellaneous functions in support of SSL

// Utility function to get the hostname of the host I am running on
XString GetHostName(COMPUTER_NAME_FORMAT WhichName)
{
  DWORD NameLength = 0;
  //BOOL R = GetComputerNameExW(ComputerNameDnsHostname, NULL, &NameLength);
  if (ERROR_SUCCESS == ::GetComputerNameEx(WhichName, NULL, &NameLength))
  {
    XString ComputerName;
    if (1 == ::GetComputerNameEx(WhichName, ComputerName.GetBufferSetLength(NameLength), &NameLength))
    {
      ComputerName.ReleaseBuffer();
      return ComputerName;
    }
  }
  return XString();
}

// Utility function to return the user name I'm running under
XString GetUserName(void)
{
  DWORD NameLength = 0;
  //BOOL R = GetComputerNameExW(ComputerNameDnsHostname, NULL, &NameLength);
  if (ERROR_SUCCESS == ::GetUserName(NULL, &NameLength))
  {
    XString UserName;
    if (1 == ::GetUserName(UserName.GetBufferSetLength(NameLength), &NameLength))
    {
      UserName.ReleaseBuffer();
      return UserName;
    }
  }
  return XString();
}

bool IsUserAdmin()
{
  SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
  PSID AdministratorsGroup;
  BOOL result = AllocateAndInitializeSid(&NtAuthority,
                                         2,
                                         SECURITY_BUILTIN_DOMAIN_RID,
                                         DOMAIN_ALIAS_RID_ADMINS,
                                         0,0,0,0,0,0,
                                         &AdministratorsGroup);
  if(result)
  {
    if(!CheckTokenMembership(NULL,AdministratorsGroup,&result))
    {
      result = FALSE;
    }
    FreeSid(AdministratorsGroup);
  }
  return (result == TRUE);
}

//
// Usage: SetThreadName ("MainThread"[, threadID]);
//
const DWORD MS_VC_EXCEPTION = 0x406D1388;

#pragma pack(push,8)
typedef struct tagTHREADNAME_INFO
{
  DWORD  dwType;       // Must be 0x1000.
  LPCSTR szName;       // Pointer to name (in user addr space).
  DWORD  dwThreadID;   // Thread ID (MAXDWORD=caller thread).
  DWORD  dwFlags;      // Reserved for future use, must be zero.
}
THREADNAME_INFO;
#pragma pack(pop)

void SetThreadName(LPCSTR threadName,DWORD dwThreadID)
{
  THREADNAME_INFO info;
  info.dwType     = 0x1000;
  info.szName     = threadName;
  info.dwThreadID = dwThreadID;
  info.dwFlags    = 0;

  __try
  {
    RaiseException(MS_VC_EXCEPTION,0,sizeof(info) / sizeof(ULONG_PTR),(ULONG_PTR*)&info);
  }
  __except(EXCEPTION_EXECUTE_HANDLER)
  {
  }
}

void SetThreadName(LPCSTR threadName)
{
  SetThreadName(threadName, MAXDWORD);
}

bool HostNameMatches(XString HostName, LPWSTR pDNSName)
{
  XString DNSName(pDNSName);

  if(DnsNameCompare(HostName,DNSName)) // The HostName is the DNSName
  {
    return true;
  }
  else if(DNSName.Find(L'*') < 0) // The DNSName is a hostname, but did not match
  {
    return false;
  }
  else // The DNSName is wildcarded
  {
    int suffixLen = HostName.GetLength() - HostName.Find(L'.'); // The length of the fixed part
    if(DNSName.GetLength() > suffixLen + 2) // the hostname domain part must be longer than the DNSName
    {
      return false;
    }
    else if(DNSName.GetLength() - DNSName.Find(L'.') != suffixLen) // The two suffix lengths must match
    {
      return false;
    }
    else if(HostName.Right(suffixLen) != DNSName.Right(suffixLen))
    {
      return false;
    }
    else // at this point, the decision is whether the last hostname node matches the wildcard
    {
      DNSName = DNSName.SpanExcluding(_T("."));
      XString HostShortName = HostName.SpanExcluding(_T("."));
      return (S_OK == PathMatchSpecEx(HostShortName, DNSName, PMSF_NORMAL));
    }
  }
}

// See http://etutorials.org/Programming/secure+programming/Chapter+10.+Public+Key+Infrastructure/10.8+Adding+Hostname+Checking+to+Certificate+Verification/
// for a pre C++11 version of this algorithm
bool MatchCertHostName(PCCERT_CONTEXT pCertContext, LPCTSTR hostname) 
{
  /* Try SUBJECT_ALT_NAME2 first - it supersedes SUBJECT_ALT_NAME */
  auto szOID = szOID_SUBJECT_ALT_NAME2;
  auto pExtension = CertFindExtension(szOID
                                    ,pCertContext->pCertInfo->cExtension
                                    ,pCertContext->pCertInfo->rgExtension);
  if (!pExtension)
  {
    szOID = szOID_SUBJECT_ALT_NAME;
    pExtension = CertFindExtension(szOID
                                  ,pCertContext->pCertInfo->cExtension
                                  ,pCertContext->pCertInfo->rgExtension);
  }
  XString HostName(hostname);

  // Extract the SAN information (list of names) 
  DWORD cbStructInfo = 0xFFFF;
  if (pExtension && CryptDecodeObject(X509_ASN_ENCODING
                                    ,szOID
                                    ,pExtension->Value.pbData
                                    ,pExtension->Value.cbData
                                    ,0
                                    ,0
                                    ,&cbStructInfo))
  {
    auto pvS = std::make_unique<byte[]>(cbStructInfo);
    CryptDecodeObject(X509_ASN_ENCODING
                      ,szOID
                      ,pExtension->Value.pbData
                      ,pExtension->Value.cbData
                      ,0
                      ,pvS.get()
                      ,&cbStructInfo);
    auto pNameInfo = (CERT_ALT_NAME_INFO *)pvS.get();

    auto it = std::find_if(&pNameInfo->rgAltEntry[0], &pNameInfo->rgAltEntry[pNameInfo->cAltEntry], [HostName](_CERT_ALT_NAME_ENTRY Entry)
    {
      return Entry.dwAltNameChoice == CERT_ALT_NAME_DNS_NAME && HostNameMatches(HostName, Entry.pwszDNSName);
    }
    );
    return (it != &pNameInfo->rgAltEntry[pNameInfo->cAltEntry]); // left pointing past the end if not found
  }

  /* No SubjectAltName extension -- check CommonName */
  auto dwCommonNameLength = CertGetNameString(pCertContext, CERT_NAME_ATTR_TYPE, 0,(PVOID) szOID_COMMON_NAME, 0, 0);
  if(!dwCommonNameLength) // No CN found
  {
    return false;
  }
  USES_CONVERSION;
  XString CommonName;
  CertGetNameString(pCertContext, CERT_NAME_ATTR_TYPE, 0,(PVOID) szOID_COMMON_NAME, CommonName.GetBufferSetLength(dwCommonNameLength), dwCommonNameLength);
  CommonName.ReleaseBufferSetLength(dwCommonNameLength);
  return HostNameMatches(HostName,(LPWSTR) T2CW(CommonName));
}

// Select, and return a handle to a client certificate
// We take a best guess at a certificate to be used as the SSL certificate for this client 
SECURITY_STATUS CertFindClient(PCCERT_CONTEXT & pCertContext, const LPCTSTR pszSubjectName)
{
  HCERTSTORE  hMyCertStore = NULL;
  TCHAR pszFriendlyNameString[128];
  TCHAR	pszNameString[128];

  if(pCertContext)
  {
    return SEC_E_INVALID_PARAMETER;
  }
  hMyCertStore = CertOpenSystemStore(NULL, _T("MY"));

  if (!hMyCertStore)
  {
    int err = GetLastError();

    if(err == ERROR_ACCESS_DENIED)
    {
      LogError(_T("**** CertOpenStore failed with 'access denied'"));
    }
    else
    {
      LogError(_T("**** Error %d returned by CertOpenStore"),err);
    }
    return HRESULT_FROM_WIN32(err);
  }

  if(pCertContext)	// The caller passed in a certificate context we no longer need, so free it
  {
    CertFreeCertificateContext(pCertContext);
    pCertContext = NULL;
  }

  const char* serverauth = szOID_PKIX_KP_CLIENT_AUTH;
  CERT_ENHKEY_USAGE eku;
  PCCERT_CONTEXT  pCertContextCurrent = NULL;
  eku.cUsageIdentifier = 1;
  eku.rgpszUsageIdentifier = (LPSTR*) &serverauth;
  // Find a client certificate. Note that this code just searches for a 
  // certificate that has the required enhanced key usage for server authentication
  // it then selects the best one (ideally one that contains the client name somewhere
  // in the subject name).

  while (NULL != (pCertContextCurrent = CertFindCertificateInStore(hMyCertStore
                                                                  ,X509_ASN_ENCODING
                                                                  ,CERT_FIND_OPTIONAL_ENHKEY_USAGE_FLAG
                                                                  ,CERT_FIND_ENHKEY_USAGE
                                                                  ,&eku
                                                                  ,pCertContextCurrent)))
  {
    //ShowCertInfo(pCertContext);
    if (!CertGetNameString(pCertContextCurrent, CERT_NAME_FRIENDLY_DISPLAY_TYPE, 0, NULL, pszFriendlyNameString, sizeof(pszFriendlyNameString)))
    {
        DebugMsg(_T("CertGetNameString failed getting friendly name."));
        continue;
    }
    DebugMsg(_T("Certificate '%s' is allowed to be used for client authentication."), pszFriendlyNameString);
    if (!CertGetNameString(pCertContextCurrent, CERT_NAME_SIMPLE_DISPLAY_TYPE, 0, NULL, pszNameString, sizeof(pszNameString)))
    {
        LogError(_T("CertGetNameString failed getting subject name."));
        continue;
    }
    DebugMsg(_T("   Subject name = %s."), pszNameString);
    // We must be able to access cert's private key
    HCRYPTPROV_OR_NCRYPT_KEY_HANDLE hCryptProvOrNCryptKey = NULL;
    BOOL fCallerFreeProvOrNCryptKey = FALSE;
    DWORD dwKeySpec;
    if (!CryptAcquireCertificatePrivateKey(pCertContextCurrent, 0, NULL, &hCryptProvOrNCryptKey, &dwKeySpec, &fCallerFreeProvOrNCryptKey))
    {
      DWORD LastError = GetLastError();
      if(LastError == CRYPT_E_NO_KEY_PROPERTY)
      {
        LogError(_T("   Certificate is unsuitable, it has no private key"));
      }
      else
      {
        LogError(_T("   Certificate is unsuitable, its private key not accessible, Error = 0x%08x"),LastError);
      }
      continue; // Since it has no private key it is useless, just go on to the next one
    }
    // The minimum requirements are now met, 
    DebugMsg(_T("   Certificate will be saved in case it is needed."));
    if(pCertContext)	// We have a saved certificate context we no longer need, so free it
    {
      CertFreeCertificateContext(pCertContext);
    }
    pCertContext = CertDuplicateCertificateContext(pCertContextCurrent);
    if(pszSubjectName && _tcscmp(pszNameString,pszSubjectName))
    {
      DebugMsg(_T("   Subject name does not match."));
    }
    else
    {
      DebugMsg(_T("   Certificate is ideal, terminating search."));
      break;
    }
  }

  if (!pCertContext)
  {
    DWORD LastError = GetLastError();
    DebugMsg(_T("**** Error 0x%08x returned"), LastError);
    return HRESULT_FROM_WIN32(LastError);
  }
  return SEC_E_OK;
}

SECURITY_STATUS CertFindFromIssuerList(PCCERT_CONTEXT & pCertContext, SecPkgContext_IssuerListInfoEx & IssuerListInfo)
{
  PCCERT_CHAIN_CONTEXT pChainContext = NULL;
  CERT_CHAIN_FIND_BY_ISSUER_PARA FindByIssuerPara = { 0 };
  SECURITY_STATUS Status = SEC_E_CERT_UNKNOWN;
  HCERTSTORE  hMyCertStore = NULL;

  hMyCertStore = CertOpenSystemStore(NULL, _T("MY"));

  //
  // Enumerate possible client certificates.
  //

  FindByIssuerPara.cbSize             = sizeof(FindByIssuerPara);
  FindByIssuerPara.pszUsageIdentifier = szOID_PKIX_KP_CLIENT_AUTH;
  FindByIssuerPara.dwKeySpec          = 0;
  FindByIssuerPara.cIssuer            = IssuerListInfo.cIssuers;
  FindByIssuerPara.rgIssuer           = IssuerListInfo.aIssuers;

  pChainContext = NULL;

  while (TRUE)
  {
    // Find a certificate chain.
    pChainContext = CertFindChainInStore(hMyCertStore
                                        ,X509_ASN_ENCODING
                                        ,0
                                        ,CERT_CHAIN_FIND_BY_ISSUER
                                        ,&FindByIssuerPara
                                        ,pChainContext);
    if (pChainContext == NULL)
    {
      DWORD LastError = GetLastError();
      if(LastError == CRYPT_E_NOT_FOUND)
      {
        LogError(_T("No certificate was found that chains to the one in the issuer list"));
      }
      else
      {
        LogError(_T("Error 0x%08x finding cert chain"),LastError);
      }
      Status = HRESULT_FROM_WIN32(LastError);
      break;
    }
    DebugMsg(_T("certificate chain found"));
    // Get pointer to leaf certificate context.
    if(pCertContext)	// We have a saved certificate context we no longer need, so free it
    {
      CertFreeCertificateContext(pCertContext);
    }
    pCertContext = CertDuplicateCertificateContext(pChainContext->rgpChain[0]->rgpElement[0]->pCertContext);
    if(false && (g_session->GetSocketLogging() == SOCK_LOGGING_FULLTRACE) && pCertContext)
    {
      ShowCertInfo(pCertContext,_T("Certificate at the end of the chain selected"));
    }
    CertFreeCertificateChain(pChainContext);
    Status = SEC_E_OK;
    break;
  }
  return Status;
}

HRESULT FindCertificateByName(PCCERT_CONTEXT & pCertContext, const LPCTSTR pszSubjectName)
{
  HCERTSTORE  hMyCertStore = NULL;

  hMyCertStore = CertOpenSystemStore(NULL, _T("MY"));

  if (!hMyCertStore)
  {
    int err = GetLastError();

    if(err == ERROR_ACCESS_DENIED)
    {
      LogError(_T("**** CertOpenStore failed with 'access denied'"));
    }
    else
    {
      LogError(_T("**** Error %d returned by CertOpenStore"),err);
    }
    return HRESULT_FROM_WIN32(err);
  }

  pCertContext = NULL;

  // Find a client certificate. Note that this code just searches for a 
  // certificate that contains the name somewhere in the subject name.
  // If we ever really start using user names there's probably a better scheme.
  //
  // If a subject name is not specified just return a null credential.
  //

  if (pszSubjectName)
  {
    pCertContext = CertFindCertificateInStore(hMyCertStore
                                             ,X509_ASN_ENCODING
                                             ,0
                                             ,CERT_FIND_SUBJECT_STR
                                             ,pszSubjectName
                                             ,NULL);
    if (pCertContext)
    {
      return S_OK;
    }
    else
    {
      DWORD Err = GetLastError();
      LogError(_T("**** Error 0x%x returned by CertFindCertificateInStore"), Err);
      return HRESULT_FROM_WIN32(Err);
    }
  }
  else
  {
    // Succeeded, but not S_OK
    return S_FALSE; 
  }
}

// Return an indication of whether a certificate is trusted by asking Windows to validate the
// trust chain (basically asking is the certificate issuer trusted)
HRESULT CertTrusted(PCCERT_CONTEXT pCertContext)
{
	HTTPSPolicyCallbackData  polHttps;
	CERT_CHAIN_POLICY_PARA   PolicyPara;
	CERT_CHAIN_POLICY_STATUS PolicyStatus;
	CERT_CHAIN_PARA          ChainPara;
	PCCERT_CHAIN_CONTEXT     pChainContext = NULL;
	HRESULT                  Status;

  LPCSTR rgszUsages[] = 
  { 
    szOID_PKIX_KP_SERVER_AUTH
   ,szOID_SERVER_GATED_CRYPTO
   ,szOID_SGC_NETSCAPE 
  };
	DWORD cUsages = _countof(rgszUsages);

	// Build certificate chain.
	ZeroMemory(&ChainPara, sizeof(ChainPara));
	ChainPara.cbSize = sizeof(ChainPara);
	ChainPara.RequestedUsage.dwType = USAGE_MATCH_TYPE_OR;
	ChainPara.RequestedUsage.Usage.cUsageIdentifier = cUsages;
	ChainPara.RequestedUsage.Usage.rgpszUsageIdentifier = (LPSTR *) rgszUsages;

	if(!CertGetCertificateChain(NULL
                             ,pCertContext
                             ,NULL
                             ,pCertContext->hCertStore
                             ,&ChainPara
                             ,0
                             ,NULL
                             ,&pChainContext))
	{
		Status = GetLastError();
		LogError(_T("Error %#x returned by CertGetCertificateChain!"), Status);
		goto cleanup;
	}

	// Validate certificate chain.
	ZeroMemory(&polHttps, sizeof(HTTPSPolicyCallbackData));
	polHttps.cbStruct   = sizeof(HTTPSPolicyCallbackData);
	polHttps.dwAuthType = AUTHTYPE_SERVER;
	polHttps.fdwChecks  = 0;    // dwCertFlags;
	polHttps.pwszServerName = NULL; // ServerName - checked elsewhere

	ZeroMemory(&PolicyPara, sizeof(PolicyPara));
	PolicyPara.cbSize = sizeof(PolicyPara);
	PolicyPara.pvExtraPolicyPara = &polHttps;

	ZeroMemory(&PolicyStatus, sizeof(PolicyStatus));
	PolicyStatus.cbSize = sizeof(PolicyStatus);

	if(!CertVerifyCertificateChainPolicy(CERT_CHAIN_POLICY_SSL
                                      ,pChainContext
                                      ,&PolicyPara
                                      ,&PolicyStatus))
	{
		Status = HRESULT_FROM_WIN32(GetLastError());
		LogError(_T("Error %#x returned by CertVerifyCertificateChainPolicy!"), Status);
		goto cleanup;
	}

	if (PolicyStatus.dwError)
	{
		Status = S_FALSE;
		//DisplayWinVerifyTrustError(PolicyStatus.dwError); 
		goto cleanup;
	}

	Status = SEC_E_OK;

cleanup:
  if(pChainContext)
  {
    CertFreeCertificateChain(pChainContext);
  }
	return Status;
}

// Display a UI with the certificate info and also write it to the SSL_socket_logging output
HRESULT ShowCertInfo(PCCERT_CONTEXT pCertContext, XString Title)
{
	TCHAR pszNameString[256];
	void*            pvData;
	DWORD            cbData;
	DWORD            dwPropId = 0;


  if(false)
  {
	  //  Display the certificate.
	  if(!CryptUIDlgViewContext(CERT_STORE_CERTIFICATE_CONTEXT
                             ,pCertContext
                             ,NULL
                             ,CStringW(Title)
                             ,0
                             ,NULL))
	  {
		  DebugMsg(_T("UI failed."));
	  }
}
	if(CertGetNameString(pCertContext
                      ,CERT_NAME_SIMPLE_DISPLAY_TYPE
                      ,0
                      ,NULL
                      ,pszNameString
                      ,128))
	{
		DebugMsg(_T("Certificate for %s"), pszNameString);
	}
  else
  {
    LogError(_T("CertGetName failed."));
  }

	int Extensions = pCertContext->pCertInfo->cExtension;

	auto *p = pCertContext->pCertInfo->rgExtension;
	for (int i = 0; i < Extensions; i++)
	{
		DebugMsg(_T("Extension %s"), (p++)->pszObjId);
	}

	//-------------------------------------------------------------------
	// Loop to find all of the property identifiers for the specified  
	// certificate. The loop continues until 
	// CertEnumCertificateContextProperties returns zero.
	while (0 != (dwPropId = CertEnumCertificateContextProperties(pCertContext, // The context whose properties are to be listed.
		                                                           dwPropId)))    // Number of the last property found.  
		// This must be zero to find the first 
		// property identifier.
	{
		//-------------------------------------------------------------------
		// When the loop is executed, a property identifier has been found.
		// Print the property number.

		DebugMsg(_T("Property # %d found->"), dwPropId);

		//-------------------------------------------------------------------
		// Indicate the kind of property found.

    switch(dwPropId)
    {
      case CERT_FRIENDLY_NAME_PROP_ID:			        DebugMsg(_T("Friendly name: "));			                    break;
      case CERT_SIGNATURE_HASH_PROP_ID:				      DebugMsg(_T("Signature hash identifier "));			          break;
      case CERT_KEY_PROV_HANDLE_PROP_ID:			      DebugMsg(_T("KEY PROVE HANDLE")); 			                  break;
      case CERT_KEY_PROV_INFO_PROP_ID:  			      DebugMsg(_T("KEY PROV INFO PROP ID "));			              break;
      case CERT_SHA1_HASH_PROP_ID:      			      DebugMsg(_T("SHA1 HASH identifier"));			                break;
      case CERT_MD5_HASH_PROP_ID:       			      DebugMsg(_T("md5 hash identifier "));			                break;
      case CERT_KEY_CONTEXT_PROP_ID:    			      DebugMsg(_T("KEY CONTEXT PROP identifier"));			        break;
      case CERT_KEY_SPEC_PROP_ID:       			      DebugMsg(_T("KEY SPEC PROP identifier"));			            break;
      case CERT_ENHKEY_USAGE_PROP_ID:   			      DebugMsg(_T("ENHKEY USAGE PROP identifier"));			        break;
      case CERT_NEXT_UPDATE_LOCATION_PROP_ID:	      DebugMsg(_T("NEXT UPDATE LOCATION PROP identifier"));			break;
      case CERT_PVK_FILE_PROP_ID:       			      DebugMsg(_T("PVK FILE PROP identifier "));			          break;
      case CERT_DESCRIPTION_PROP_ID:    			      DebugMsg(_T("DESCRIPTION PROP identifier "));			        break;
      case CERT_ACCESS_STATE_PROP_ID:   			      DebugMsg(_T("ACCESS STATE PROP identifier "));		        break;
      case CERT_SMART_CARD_DATA_PROP_ID:			      DebugMsg(_T("SMART_CARD DATA PROP identifier "));			    break;
      case CERT_EFS_PROP_ID:            			      DebugMsg(_T("EFS PROP identifier "));			                break;
      case CERT_FORTEZZA_DATA_PROP_ID:  			      DebugMsg(_T("FORTEZZA DATA PROP identifier "));			      break;
      case CERT_ARCHIVED_PROP_ID:       			      DebugMsg(_T("ARCHIVED PROP identifier "));			          break;
      case CERT_KEY_IDENTIFIER_PROP_ID: 			      DebugMsg(_T("KEY IDENTIFIER PROP identifier "));			    break;
      case CERT_AUTO_ENROLL_PROP_ID:    			      DebugMsg(_T("AUTO ENROLL identifier. "));			            break;
      case CERT_ISSUER_PUBLIC_KEY_MD5_HASH_PROP_ID:	DebugMsg(_T("ISSUER PUBLIC KEY MD5 HASH identifier. "));  break;
    } // End switch.

		//-------------------------------------------------------------------
		// Retrieve information on the property by first getting the 
		// property size. 
		// For more information, see CertGetCertificateContextProperty.

		if(CertGetCertificateContextProperty(pCertContext
                                        ,dwPropId
                                        ,NULL
                                        ,&cbData))
		{
			//  Continue.
		}
		else
		{
			// If the first call to the function failed,
			// exit to an error routine.
			LogError(_T("Call #1 to GetCertContextProperty failed."));
			return E_FAIL;
		}
		//-------------------------------------------------------------------
		// The call succeeded. Use the size to allocate memory 
		// for the property.

		if (NULL != (pvData = (void*)malloc(cbData)))
		{
			// Memory is allocated. Continue.
		}
		else
		{
			// If memory allocation failed, exit to an error routine.
			LogError(_T("Memory allocation failed."));
			return E_FAIL;
		}
		//----------------------------------------------------------------
		// Allocation succeeded. Retrieve the property data.

		if(CertGetCertificateContextProperty(pCertContext
                                        ,dwPropId
                                        ,pvData
                                        ,&cbData))
		{
			// The data has been retrieved. Continue.
		}
		else
		{
			// If an error occurred in the second call, 
   		// exit to an error routine.
			LogError(_T("Call #2 failed."));
			return E_FAIL;
		}
		//---------------------------------------------------------------
		// Show the results.

		DebugMsg(_T("The Property Content is"));
		PrintHexDump(cbData, pvData);

		//----------------------------------------------------------------
		// Free the certificate context property memory.

		free(pvData);
	}
	return S_OK;
}

// SERVERSIDE ONLY
// FIND SSL Server Certificate
//
// Select, and return a handle to a server certificate located by name
// Usually used for a best guess at a certificate to be used as the SSL certificate for a server 
//
SECURITY_STATUS CertFindServerByName(PCCERT_CONTEXT & pCertContext, LPCTSTR pszSubjectName, boolean fUserStore)
{
  HCERTSTORE  hMyCertStore = NULL;
  TCHAR pszFriendlyNameString[128];
  TCHAR	pszNameString[128];

  if (pszSubjectName == NULL || _tcslen(pszSubjectName) == 0)
  {
    LogError(_T("**** No subject name specified!"));
    return E_POINTER;
  }

  if(fUserStore)
  {
    hMyCertStore = CertOpenSystemStore(NULL,_T("MY"));
  }
  else
  {	
    // Open the local machine certificate store.
    hMyCertStore = CertOpenStore(CERT_STORE_PROV_SYSTEM
                                ,X509_ASN_ENCODING 
                                ,NULL
                                ,CERT_STORE_OPEN_EXISTING_FLAG | CERT_STORE_READONLY_FLAG | CERT_SYSTEM_STORE_LOCAL_MACHINE
                                ,L"TRUST");
  }

  if (!hMyCertStore)
  {
    int err = GetLastError();

    if(err == ERROR_ACCESS_DENIED)
    {
      LogError(_T("**** CertOpenStore failed with 'access denied'"));
    }
    else
    {
      LogError(_T("**** Error %d returned by CertOpenStore"),err);
    }
    return HRESULT_FROM_WIN32(err);
  }

  if(pCertContext)	// The caller passed in a certificate context we no longer need, so free it
  {
    CertFreeCertificateContext(pCertContext);
  }
  pCertContext = NULL;

  const char* serverauth = szOID_PKIX_KP_SERVER_AUTH;
  CERT_ENHKEY_USAGE eku;
  PCCERT_CONTEXT pCertContextSaved = NULL;
  eku.cUsageIdentifier = 1;
  eku.rgpszUsageIdentifier = (LPSTR*) &serverauth;
  // Find a server certificate. Note that this code just searches for a 
  // certificate that has the required enhanced key usage for server authentication
  // it then selects the best one (ideally one that contains the server name
  // in the subject name).

  while (NULL != (pCertContext = CertFindCertificateInStore(hMyCertStore
                                                           ,X509_ASN_ENCODING 
                                                           ,NULL // CERT_FIND_OPTIONAL_ENHKEY_USAGE_FLAG
                                                           ,CERT_FIND_ANY // CERT_FIND_ENHKEY_USAGE
                                                           ,NULL // &eku
                                                           ,pCertContext)))
  {
    //ShowCertInfo(pCertContext);
    if (!CertGetNameString(pCertContext, CERT_NAME_FRIENDLY_DISPLAY_TYPE, 0, NULL, pszFriendlyNameString, sizeof(pszFriendlyNameString)))
    {
      LogError(_T("CertGetNameString failed getting friendly name."));
      continue;
    }
    DebugMsg(_T("Certificate '%s' is allowed to be used for server authentication."), pszFriendlyNameString);
    if(!CertGetNameString(pCertContext,CERT_NAME_SIMPLE_DISPLAY_TYPE,0,NULL,pszNameString,sizeof(pszNameString)))
    {
      LogError(_T("CertGetNameString failed getting subject name."));
    }
    else if(!MatchCertHostName(pCertContext,pszSubjectName))  //  (_tcscmp(pszNameString, pszSubjectName))
    {
      LogError(_T("Certificate has wrong subject name."));
    }
    else if (CertCompareCertificateName(X509_ASN_ENCODING, &pCertContext->pCertInfo->Subject, &pCertContext->pCertInfo->Issuer))
    {
      if (!pCertContextSaved)
      {
        DebugMsg(_T("A self-signed certificate was found and saved in case it is needed."));
        pCertContextSaved = CertDuplicateCertificateContext(pCertContext);
      }
    }
    else
    {
      DebugMsg(_T("Certificate is acceptable."));
      // We have a saved self signed certificate context we no longer need, so free it
      if(pCertContextSaved)	
      {
        CertFreeCertificateContext(pCertContextSaved);
        pCertContextSaved = NULL;
      }
      break;
    }
  }

  if (pCertContextSaved && !pCertContext)
  {	
    // We have a saved self-signed certificate and nothing better 
    DebugMsg(_T("A self-signed certificate was the best we had."));
    pCertContext = pCertContextSaved;
    pCertContextSaved = NULL;
  }

  if (!pCertContext)
  {
    DWORD LastError = GetLastError();
//     if (LastError == CRYPT_E_NOT_FOUND)
//     {
//       DebugMsg("**** CertFindCertificateInStore did not find a certificate, creating one");
//       pCertContext = CreateCertificate(true, pszSubjectName);
//       if (!pCertContext)
//       {
//         LastError = GetLastError();
//         LogError("**** Error 0x%x returned by CreateCertificate", LastError);
//         LogError("Could not create certificate, are you running as administrator?");
//         return HRESULT_FROM_WIN32(LastError);
//       }
//     }
//     else
//     {
      LogError(_T("**** Error 0x%x returned by CertFindCertificateInStore"), LastError);
      return HRESULT_FROM_WIN32(LastError);
//    }
  }

  return SEC_E_OK;
}

// Encoding a thumbprint string 
// Preparing it to search in the certificate stores
// Accepts two kinds of thumbprints
// With    spaces: "DB 34 40 64 f2 fc 13 18 DD 90 f5 07 FE 78 e8 1b 03 16 00"
// Without spaces: "db344064f2fc1318dd90f507fe78e81b031600"
// P_blob must point to a blob that is sufficiently large (20 bytes)
bool
SSLEncodeThumbprint(XString& p_thumbprint,PCRYPT_HASH_BLOB p_blob,DWORD p_len)
{
  // Removing
  p_thumbprint.Replace(_T(" "), _T(""));

  BYTE* bpointer = p_blob->pbData;

  for(int ind = 0; ind < p_thumbprint.GetLength();)
  {
    int c1 = _totupper(p_thumbprint.GetAt(ind++));
    int c2 = _totupper(p_thumbprint.GetAt(ind++));

    c1 = (c1 <= '9') ? c1 = c1 - _T('0') : c1 - _T('A') + 10;
    c2 = (c2 <= '9') ? c2 = c2 - _T('0') : c2 - _T('A') + 10;
    *bpointer++ = static_cast<BYTE>((c1 << 4) + c2);

    p_blob->cbData = ind / 2;

    if(p_blob->cbData == p_len)
    {
      return false;
    }
  }
  return true;
}

//////////////////////////////////////////////////////////////////////////
//
// ENTRY POINTS FOR THE LISTENER
//
//////////////////////////////////////////////////////////////////////////

XString 
GetCertificateName(PCCERT_CONTEXT pCertContext)
{
  XString certName;
  auto good = CertGetNameString(pCertContext, CERT_NAME_FRIENDLY_DISPLAY_TYPE, 0, NULL, certName.GetBuffer(128), certName.GetAllocLength() - 1);
  certName.ReleaseBuffer();
  if (good)
  {
    return certName;
  }
  else
  {
    return _T("<unknown>");
  }
}

SECURITY_STATUS
SelectServerCert(PCCERT_CONTEXT& pCertContext, LPCTSTR p_storeName,BYTE* p_thumbprint)
{
  // Go looking for this serverside publishing certificate
  SECURITY_STATUS status = FindCertificateByThumbprint(pCertContext,p_storeName,p_thumbprint);
  if(pCertContext)
  {
    DebugMsg(_T("Server certificate requested for found %s"),GetCertificateName(pCertContext));
  }
  return status;
}

bool
ClientCertAcceptable(PCCERT_CONTEXT pCertContext, const bool trusted)
{
  XString type;
  if(trusted)
  {
    type = _T("A trusted");
  }
  else
  {
    type = _T("An untrusted");
  }
  DebugMsg(_T("%s client certificate was returned for: %s"),type,GetCertificateName(pCertContext));

  // Meaning any certificate is fine, trusted or not, but there must be one
  return NULL != pCertContext;
}

// Find server certificate by thumbprint
bool
FindCertificateByThumbprint(PCCERT_CONTEXT& p_certContext,LPCTSTR p_store,BYTE* p_thumbprint)
{
  bool result = false;

  // Open the store of the certificate
  HCERTSTORE hStore = CertOpenSystemStore(0,p_store);
  if(hStore)
  {
    CRYPT_HASH_BLOB blob;
    blob.cbData = CERT_THUMBPRINT_SIZE;
    blob.pbData = p_thumbprint; 

    // Finding our certificate by hash-blob of the thumbprint
    PCCERT_CONTEXT certificate = CertFindCertificateInStore(hStore
                                                           ,X509_ASN_ENCODING | PKCS_7_ASN_ENCODING
                                                           ,0
                                                           ,CERT_FIND_SHA1_HASH
                                                           ,(LPVOID)&blob
                                                           ,NULL);
    if(certificate)
    {
      p_certContext = certificate;
      DebugMsg(_T("Client certificate found in [%s]"),p_store);
      result = true;
    }
    else
    {
      LogError(_T("Requested certificate not found in: [%d] %s"),GetLastError(),p_store);
    }
    // Caveat: Never use the 'force' flag, as WinHTTP still needs the store
    CertCloseStore(hStore,0);
  }
  else
  {
    LogError(_T("Requested certificate store not found [%d] %s"),GetLastError(),p_store);
  }
  return result;
}

// Used for authentication records
bool
SplitString(XString p_input,XString& p_output1,XString& p_output2,TCHAR p_separator)
{
  // Prepare
  p_input.Trim();
  p_output1.Empty();
  p_output2.Empty();

  // Find separator
  int pos = p_input.Find(p_separator);

  // See if we can split
  if(pos > 0)
  {
    p_output1 = p_input.Left(pos);
    p_output2 = p_input.Mid(pos + 1);
    p_output2.TrimLeft();

    return true;
  }
  // Not split!!
  return false;
}
