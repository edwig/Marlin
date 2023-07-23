/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: GetLastErrorAsString.cpp
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
#include "pch.h"
#include "GetLastErrorAsString.h"
#include <winerror.h>
#include "WinINETError.h"
#include "HTTPError.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static XString
GetWinInetError(DWORD p_error)
{
  XString error;
  if(0x2EE1 <= p_error && p_error <= 0x2F88)
  {
    // Add FACILITY_WIN32 and COERROR to get the winerror codes
    switch(p_error | 0x80070000)
    {
      case WININET_E_OUT_OF_HANDLES:                error = "No more Internet handles can be allocated";                        break;
      case WININET_E_TIMEOUT:                       error = "The operation timed out";                                          break;
      case WININET_E_EXTENDED_ERROR:                error = "The server returned extended information";                         break;
      case WININET_E_INTERNAL_ERROR:                error = "An internal error occurred in the Microsoft Internet extensions";  break;
      case WININET_E_INVALID_URL:                   error = "The URL is invalid";                                               break;
      case WININET_E_UNRECOGNIZED_SCHEME:           error = "The URL does not use a recognized protocol";                       break;
      case WININET_E_NAME_NOT_RESOLVED:             error = "The server name or address could not be resolved";                 break;
      case WININET_E_PROTOCOL_NOT_FOUND:            error = "A protocol with the required capabilities was not found";          break;
      case WININET_E_INVALID_OPTION:                error = "The option is invalid";                                            break;
      case WININET_E_BAD_OPTION_LENGTH:             error = "The length is incorrect for the option type";                      break;
      case WININET_E_OPTION_NOT_SETTABLE:           error = "The option value cannot be set";                                   break;
      case WININET_E_SHUTDOWN:                      error = "Microsoft Internet Extension support has been shut down";          break;
      case WININET_E_INCORRECT_USER_NAME:           error = "The user name was not allowed";                                    break;
      case WININET_E_INCORRECT_PASSWORD:            error = "The password was not allowed";                                     break;
      case WININET_E_LOGIN_FAILURE:                 error = "The login request was denied";                                     break;
      case WININET_E_INVALID_OPERATION:             error = "The requested operation is invalid";                               break;
      case WININET_E_OPERATION_CANCELLED:           error = "The operation has been canceled";                                  break;
      case WININET_E_INCORRECT_HANDLE_TYPE:         error = "The supplied handle is the wrong type for the requested operation";break;
      case WININET_E_INCORRECT_HANDLE_STATE:        error = "The handle is in the wrong state for the requested operation";     break;
      case WININET_E_NOT_PROXY_REQUEST:             error = "The request cannot be made on a Proxy session";                    break;
      case WININET_E_REGISTRY_VALUE_NOT_FOUND:      error = "The registry value could not be found";                            break;
      case WININET_E_BAD_REGISTRY_PARAMETER:        error = "The registry parameter is incorrect";                              break;
      case WININET_E_NO_DIRECT_ACCESS:              error = "Direct Internet access is not available";                          break;
      case WININET_E_NO_CONTEXT:                    error = "No context value was supplied";                                    break;
      case WININET_E_NO_CALLBACK:                   error = "No status callback was supplied";                                  break;
      case WININET_E_REQUEST_PENDING:               error = "There are outstanding requests";                                   break;
      case WININET_E_INCORRECT_FORMAT:              error = "The information format is incorrect";                              break;
      case WININET_E_ITEM_NOT_FOUND:                error = "The requested item could not be found";                            break;
      case WININET_E_CANNOT_CONNECT:                error = "A connection with the server could not be established";            break;
      case WININET_E_CONNECTION_ABORTED:            error = "The connection with the server was terminated abnormally";         break;
      case WININET_E_CONNECTION_RESET:              error = "The connection with the server was reset";                         break;
      case WININET_E_FORCE_RETRY:                   error = "The action must be retried";                                       break;
      case WININET_E_INVALID_PROXY_REQUEST:         error = "The proxy request is invalid";                                     break;
      case WININET_E_NEED_UI:                       error = "User interaction is required to complete the operation";           break;
      case WININET_E_HANDLE_EXISTS:                 error = "The handle already exists";                                        break;
      case WININET_E_SEC_CERT_DATE_INVALID:         error = "The date in the certificate is invalid or has expired";            break;
      case WININET_E_SEC_CERT_CN_INVALID:           error = "The host name in the certificate is invalid or does not match";    break;
      case WININET_E_HTTP_TO_HTTPS_ON_REDIR:        error = "A redirect request will change a non-secure to a secure connection"; break;
      case WININET_E_HTTPS_TO_HTTP_ON_REDIR:        error = "A redirect request will change a secure to a non-secure connection"; break;
      case WININET_E_MIXED_SECURITY:                error = "Mixed secure and non-secure connections";                          break;
      case WININET_E_CHG_POST_IS_NON_SECURE:        error = "Changing to non-secure post";                                      break;
      case WININET_E_POST_IS_NON_SECURE:            error = "Data is being posted on a non-secure connection";                  break;
      case WININET_E_CLIENT_AUTH_CERT_NEEDED:       error = "A certificate is required to complete client authentication";      break;
      case WININET_E_INVALID_CA:                    error = "The certificate authority is invalid or incorrect";                break;
      case WININET_E_CLIENT_AUTH_NOT_SETUP:         error = "Client authentication has not been correctly installed";           break;
      case WININET_E_ASYNC_THREAD_FAILED:           error = "An error has occurred in a Wininet asynchronous thread. You may need to restart"; break;
      case WININET_E_REDIRECT_SCHEME_CHANGE:        error = "The protocol scheme has changed during a redirect operation";      break;
      case WININET_E_DIALOG_PENDING:                error = "There are operations awaiting retry";                              break;
      case WININET_E_RETRY_DIALOG:                  error = "The operation must be retried";                                    break;
      case WININET_E_NO_NEW_CONTAINERS:             error = "There are no new cache containers";                                break;
      case WININET_E_HTTPS_HTTP_SUBMIT_REDIR:       error = "A security zone check indicates the operation must be retried";    break;
      case WININET_E_SEC_CERT_ERRORS:               error = "The SSL certificate contains errors.";                             break;
      case WININET_E_SEC_CERT_REV_FAILED:           error = "It was not possible to connect to the revocation server or a definitive response could not be obtained."; break;
      case WININET_E_HEADER_NOT_FOUND:              error = "The requested header was not found";                               break;
      case WININET_E_DOWNLEVEL_SERVER:              error = "The server does not support the requested protocol level";         break;
      case WININET_E_INVALID_SERVER_RESPONSE:       error = "The server returned an invalid or unrecognized response";          break;
      case WININET_E_INVALID_HEADER:                error = "The supplied HTTP header is invalid";                              break;
      case WININET_E_INVALID_QUERY_REQUEST:         error = "The request for a HTTP header is invalid";                         break;
      case WININET_E_HEADER_ALREADY_EXISTS:         error = "The HTTP header already exists";                                   break;
      case WININET_E_REDIRECT_FAILED:               error = "The HTTP redirect request failed";                                 break;
      case WININET_E_SECURITY_CHANNEL_ERROR:        error = "An error occurred in the secure channel support";                  break;
      case WININET_E_UNABLE_TO_CACHE_FILE:          error = "The file could not be written to the cache";                       break;
      case WININET_E_TCPIP_NOT_INSTALLED:           error = "The TCP/IP protocol is not installed properly";                    break;
      case WININET_E_DISCONNECTED:                  error = "The computer is disconnected from the network";                    break;
      case WININET_E_SERVER_UNREACHABLE:            error = "The server is unreachable";                                        break;
      case WININET_E_PROXY_SERVER_UNREACHABLE:      error = "The proxy server is unreachable";                                  break;
      case WININET_E_BAD_AUTO_PROXY_SCRIPT:         error = "The proxy auto-configuration script is in error";                  break;
      case WININET_E_UNABLE_TO_DOWNLOAD_SCRIPT:     error = "Could not download the proxy auto-configuration script file";      break;
      case WININET_E_SEC_INVALID_CERT:              error = "The supplied certificate is invalid";                              break;
      case WININET_E_SEC_CERT_REVOKED:              error = "The supplied certificate has been revoked";                        break;
      case WININET_E_FAILED_DUETOSECURITYCHECK:     error = "The Dialup failed because file sharing was turned on and a failure was requested if security check was needed"; break;
      case WININET_E_NOT_INITIALIZED:               error = "Initialization of the WinINet API has not occurred";               break;
      case WININET_E_LOGIN_FAILURE_DISPLAY_ENTITY_BODY: error = "Login failed and the client should display the entity body to the user"; break;
      case WININET_E_DECODING_FAILED:               error = "Content decoding has failed";                                      break;
      case WININET_E_NOT_REDIRECTED:                error = "The HTTP request was not redirected";                              break;
      case WININET_E_COOKIE_NEEDS_CONFIRMATION:     error = "A cookie from the server must be confirmed by the user";           break;
      case WININET_E_COOKIE_DECLINED:               error = "A cookie from the server has been declined acceptance";            break;
      case WININET_E_REDIRECT_NEEDS_CONFIRMATION:   error = "The HTTP redirect request must be confirmed by the user";          break;
    }
  }
  return error;
}

XString
GetLastErrorAsString(DWORD p_error /*=0*/)
{
  // This will be the resulting message
  XString message;

  // This is the "errno" error number
  DWORD dwError = p_error;
  if(dwError == 0)
  {
    dwError = ::GetLastError();
  }
  // Buffer that gets the error message string
  HLOCAL  hlocal = NULL;   
  HMODULE hDll   = NULL;

  // Use the default system locale since we look for Windows messages.
  // Note: this MAKELANGID combination has 0 as value
  DWORD systemLocale = MAKELANGID(LANG_NEUTRAL,SUBLANG_NEUTRAL);

  // Get the error code's textual description
  BOOL fOk = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM     | 
                           FORMAT_MESSAGE_IGNORE_INSERTS  |
                           FORMAT_MESSAGE_ALLOCATE_BUFFER 
                          ,NULL                               // DLL
                          ,dwError                            // The error code
                          ,systemLocale                       // Language
                          ,reinterpret_cast<PTSTR>(&hlocal)   // Buffer handle
                          ,0                                  // Size if not buffer handle
                          ,NULL);                             // Variable arguments
  // If not found: try most generic HTTP errors
  if(!fOk)
  {
    message = GetWinInetError(dwError);
    fOk = !message.IsEmpty();
  }
  if(!fOk)
  {
    message = GetHTTPErrorText(dwError);
    fOk = !message.IsEmpty();
  }
  // Non-generic networking error?
  if(!fOk) 
  {
    // Is it a network-related error?
    hDll = LoadLibraryEx(TEXT("netmsg.dll"),NULL,DONT_RESOLVE_DLL_REFERENCES);

    // Only if NETMSG was found and loaded
    if(hDll != NULL) 
    {
      fOk = FormatMessage(FORMAT_MESSAGE_FROM_HMODULE    | 
                          FORMAT_MESSAGE_IGNORE_INSERTS  |
                          FORMAT_MESSAGE_ALLOCATE_BUFFER 
                         ,hDll
                         ,dwError
                         ,systemLocale
                         ,reinterpret_cast<PTSTR>(&hlocal)
                         ,0
                         ,NULL);
    }
  }
  if(fOk && (hlocal != NULL)) 
  {
    // Getting the message from the buffer;
    message = reinterpret_cast<PCTSTR>(LocalLock(hlocal));
    LocalFree(hlocal);
    // Formatting the message in one line
    message.Replace('\n',' ');
    message.Remove ('\r');
  }
  if(hDll)
  {
    FreeLibrary(hDll);
  }
  // Resulting 
  return message;
}
