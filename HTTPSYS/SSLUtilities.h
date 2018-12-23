/////////////////////////////////////////////////////////////////////////////
// 
// SSL Helper functions
//
// Original idea:
// David Maw:      https://www.codeproject.com/Articles/1000189/A-Working-TCP-Client-and-Server-With-SSL
// Brandon Wilson: https://blogs.technet.microsoft.com/askpfeplat/2017/11/13/demystifying-schannel/
// License:        https://www.codeproject.com/info/cpol10.aspx
//
#pragma once
#define SECURITY_WIN32
#include <security.h>
#include <schannel.h>
#include <cryptuiapi.h>

// Automatically link to these libraries
#pragma comment(lib,"secur32.lib")
#pragma comment(lib,"cryptui.lib")

#ifndef SCH_USE_STRONG_CRYPTO   // Needs KB 2868725 which is only in Windows 7+
#define SCH_USE_STRONG_CRYPTO   0x00400000
#endif

// handy functions declared in this file
HRESULT         ShowCertInfo(PCCERT_CONTEXT  pCertContext, CString Title);
HRESULT         CertTrusted(PCCERT_CONTEXT  pCertContext);
bool            MatchCertHostName(PCCERT_CONTEXT  pCertContext, LPCSTR hostname);
SECURITY_STATUS CertFindClient(PCCERT_CONTEXT& pCertContext, const LPCTSTR pszSubjectName = NULL);
SECURITY_STATUS CertFindFromIssuerList(PCCERT_CONTEXT& pCertContext, SecPkgContext_IssuerListInfoEx & IssuerListInfo);
CString         GetHostName(COMPUTER_NAME_FORMAT WhichName = ComputerNameDnsHostname);
CString         GetUserName(void);
bool            SplitString(CString p_input,CString& p_output1,CString& p_output2,char p_separator);
bool            IsUserAdmin();
void            SetThreadName(const char* threadName);
void            SetThreadName(const char* threadName, DWORD dwThreadID);
bool            SSLEncodeThumbprint(CString& p_thumbprint,PCRYPT_HASH_BLOB p_blob,DWORD p_len);
bool            FindCertificateByThumbprint(PCCERT_CONTEXT& p_certContext, LPCTSTR p_store, PTCHAR p_thumbprint);

// Server side
SECURITY_STATUS CertFindServerByName(PCCERT_CONTEXT & pCertContext, LPCTSTR pszSubjectName, boolean fUserStore);

SECURITY_STATUS SelectServerCert    (PCCERT_CONTEXT& pCertContext,LPCTSTR    p_certStore,PTCHAR p_thumbprint);
bool            ClientCertAcceptable(PCCERT_CONTEXT  pCertContext,const bool trusted);
