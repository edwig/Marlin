#pragma once

//
// MessageId: WER_S_REPORT_UPLOADED
//
// MessageText:
//
// Report was uploaded.
//
#define WER_S_REPORT_UPLOADED            _HRESULT_TYPEDEF_(0x001B0001L)


//
// WININET.DLL errors - propagated as HRESULT's using FACILITY=WIN32
//
//
// MessageId: WININET_E_OUT_OF_HANDLES
//
// MessageText:
//
// No more Internet handles can be allocated
//
#define WININET_E_OUT_OF_HANDLES         _HRESULT_TYPEDEF_(0x80072EE1L)

//
// MessageId: WININET_E_TIMEOUT
//
// MessageText:
//
// The operation timed out
//
#define WININET_E_TIMEOUT                _HRESULT_TYPEDEF_(0x80072EE2L)

//
// MessageId: WININET_E_EXTENDED_ERROR
//
// MessageText:
//
// The server returned extended information
//
#define WININET_E_EXTENDED_ERROR         _HRESULT_TYPEDEF_(0x80072EE3L)

//
// MessageId: WININET_E_INTERNAL_ERROR
//
// MessageText:
//
// An internal error occurred in the Microsoft Internet extensions
//
#define WININET_E_INTERNAL_ERROR         _HRESULT_TYPEDEF_(0x80072EE4L)

//
// MessageId: WININET_E_INVALID_URL
//
// MessageText:
//
// The URL is invalid
//
#define WININET_E_INVALID_URL            _HRESULT_TYPEDEF_(0x80072EE5L)

//
// MessageId: WININET_E_UNRECOGNIZED_SCHEME
//
// MessageText:
//
// The URL does not use a recognized protocol
//
#define WININET_E_UNRECOGNIZED_SCHEME    _HRESULT_TYPEDEF_(0x80072EE6L)

//
// MessageId: WININET_E_NAME_NOT_RESOLVED
//
// MessageText:
//
// The server name or address could not be resolved
//
#define WININET_E_NAME_NOT_RESOLVED      _HRESULT_TYPEDEF_(0x80072EE7L)

//
// MessageId: WININET_E_PROTOCOL_NOT_FOUND
//
// MessageText:
//
// A protocol with the required capabilities was not found
//
#define WININET_E_PROTOCOL_NOT_FOUND     _HRESULT_TYPEDEF_(0x80072EE8L)

//
// MessageId: WININET_E_INVALID_OPTION
//
// MessageText:
//
// The option is invalid
//
#define WININET_E_INVALID_OPTION         _HRESULT_TYPEDEF_(0x80072EE9L)

//
// MessageId: WININET_E_BAD_OPTION_LENGTH
//
// MessageText:
//
// The length is incorrect for the option type
//
#define WININET_E_BAD_OPTION_LENGTH      _HRESULT_TYPEDEF_(0x80072EEAL)

//
// MessageId: WININET_E_OPTION_NOT_SETTABLE
//
// MessageText:
//
// The option value cannot be set
//
#define WININET_E_OPTION_NOT_SETTABLE    _HRESULT_TYPEDEF_(0x80072EEBL)

//
// MessageId: WININET_E_SHUTDOWN
//
// MessageText:
//
// Microsoft Internet Extension support has been shut down
//
#define WININET_E_SHUTDOWN               _HRESULT_TYPEDEF_(0x80072EECL)

//
// MessageId: WININET_E_INCORRECT_USER_NAME
//
// MessageText:
//
// The user name was not allowed
//
#define WININET_E_INCORRECT_USER_NAME    _HRESULT_TYPEDEF_(0x80072EEDL)

//
// MessageId: WININET_E_INCORRECT_PASSWORD
//
// MessageText:
//
// The password was not allowed
//
#define WININET_E_INCORRECT_PASSWORD     _HRESULT_TYPEDEF_(0x80072EEEL)

//
// MessageId: WININET_E_LOGIN_FAILURE
//
// MessageText:
//
// The login request was denied
//
#define WININET_E_LOGIN_FAILURE          _HRESULT_TYPEDEF_(0x80072EEFL)

//
// MessageId: WININET_E_INVALID_OPERATION
//
// MessageText:
//
// The requested operation is invalid
//
#define WININET_E_INVALID_OPERATION      _HRESULT_TYPEDEF_(0x80072EF0L)

//
// MessageId: WININET_E_OPERATION_CANCELLED
//
// MessageText:
//
// The operation has been canceled
//
#define WININET_E_OPERATION_CANCELLED    _HRESULT_TYPEDEF_(0x80072EF1L)

//
// MessageId: WININET_E_INCORRECT_HANDLE_TYPE
//
// MessageText:
//
// The supplied handle is the wrong type for the requested operation
//
#define WININET_E_INCORRECT_HANDLE_TYPE  _HRESULT_TYPEDEF_(0x80072EF2L)

//
// MessageId: WININET_E_INCORRECT_HANDLE_STATE
//
// MessageText:
//
// The handle is in the wrong state for the requested operation
//
#define WININET_E_INCORRECT_HANDLE_STATE _HRESULT_TYPEDEF_(0x80072EF3L)

//
// MessageId: WININET_E_NOT_PROXY_REQUEST
//
// MessageText:
//
// The request cannot be made on a Proxy session
//
#define WININET_E_NOT_PROXY_REQUEST      _HRESULT_TYPEDEF_(0x80072EF4L)

//
// MessageId: WININET_E_REGISTRY_VALUE_NOT_FOUND
//
// MessageText:
//
// The registry value could not be found
//
#define WININET_E_REGISTRY_VALUE_NOT_FOUND _HRESULT_TYPEDEF_(0x80072EF5L)

//
// MessageId: WININET_E_BAD_REGISTRY_PARAMETER
//
// MessageText:
//
// The registry parameter is incorrect
//
#define WININET_E_BAD_REGISTRY_PARAMETER _HRESULT_TYPEDEF_(0x80072EF6L)

//
// MessageId: WININET_E_NO_DIRECT_ACCESS
//
// MessageText:
//
// Direct Internet access is not available
//
#define WININET_E_NO_DIRECT_ACCESS       _HRESULT_TYPEDEF_(0x80072EF7L)

//
// MessageId: WININET_E_NO_CONTEXT
//
// MessageText:
//
// No context value was supplied
//
#define WININET_E_NO_CONTEXT             _HRESULT_TYPEDEF_(0x80072EF8L)

//
// MessageId: WININET_E_NO_CALLBACK
//
// MessageText:
//
// No status callback was supplied
//
#define WININET_E_NO_CALLBACK            _HRESULT_TYPEDEF_(0x80072EF9L)

//
// MessageId: WININET_E_REQUEST_PENDING
//
// MessageText:
//
// There are outstanding requests
//
#define WININET_E_REQUEST_PENDING        _HRESULT_TYPEDEF_(0x80072EFAL)

//
// MessageId: WININET_E_INCORRECT_FORMAT
//
// MessageText:
//
// The information format is incorrect
//
#define WININET_E_INCORRECT_FORMAT       _HRESULT_TYPEDEF_(0x80072EFBL)

//
// MessageId: WININET_E_ITEM_NOT_FOUND
//
// MessageText:
//
// The requested item could not be found
//
#define WININET_E_ITEM_NOT_FOUND         _HRESULT_TYPEDEF_(0x80072EFCL)

//
// MessageId: WININET_E_CANNOT_CONNECT
//
// MessageText:
//
// A connection with the server could not be established
//
#define WININET_E_CANNOT_CONNECT         _HRESULT_TYPEDEF_(0x80072EFDL)

//
// MessageId: WININET_E_CONNECTION_ABORTED
//
// MessageText:
//
// The connection with the server was terminated abnormally
//
#define WININET_E_CONNECTION_ABORTED     _HRESULT_TYPEDEF_(0x80072EFEL)

//
// MessageId: WININET_E_CONNECTION_RESET
//
// MessageText:
//
// The connection with the server was reset
//
#define WININET_E_CONNECTION_RESET       _HRESULT_TYPEDEF_(0x80072EFFL)

//
// MessageId: WININET_E_FORCE_RETRY
//
// MessageText:
//
// The action must be retried
//
#define WININET_E_FORCE_RETRY            _HRESULT_TYPEDEF_(0x80072F00L)

//
// MessageId: WININET_E_INVALID_PROXY_REQUEST
//
// MessageText:
//
// The proxy request is invalid
//
#define WININET_E_INVALID_PROXY_REQUEST  _HRESULT_TYPEDEF_(0x80072F01L)

//
// MessageId: WININET_E_NEED_UI
//
// MessageText:
//
// User interaction is required to complete the operation
//
#define WININET_E_NEED_UI                _HRESULT_TYPEDEF_(0x80072F02L)

//
// MessageId: WININET_E_HANDLE_EXISTS
//
// MessageText:
//
// The handle already exists
//
#define WININET_E_HANDLE_EXISTS          _HRESULT_TYPEDEF_(0x80072F04L)

//
// MessageId: WININET_E_SEC_CERT_DATE_INVALID
//
// MessageText:
//
// The date in the certificate is invalid or has expired
//
#define WININET_E_SEC_CERT_DATE_INVALID  _HRESULT_TYPEDEF_(0x80072F05L)

//
// MessageId: WININET_E_SEC_CERT_CN_INVALID
//
// MessageText:
//
// The host name in the certificate is invalid or does not match
//
#define WININET_E_SEC_CERT_CN_INVALID    _HRESULT_TYPEDEF_(0x80072F06L)

//
// MessageId: WININET_E_HTTP_TO_HTTPS_ON_REDIR
//
// MessageText:
//
// A redirect request will change a non-secure to a secure connection
//
#define WININET_E_HTTP_TO_HTTPS_ON_REDIR _HRESULT_TYPEDEF_(0x80072F07L)

//
// MessageId: WININET_E_HTTPS_TO_HTTP_ON_REDIR
//
// MessageText:
//
// A redirect request will change a secure to a non-secure connection
//
#define WININET_E_HTTPS_TO_HTTP_ON_REDIR _HRESULT_TYPEDEF_(0x80072F08L)

//
// MessageId: WININET_E_MIXED_SECURITY
//
// MessageText:
//
// Mixed secure and non-secure connections
//
#define WININET_E_MIXED_SECURITY         _HRESULT_TYPEDEF_(0x80072F09L)

//
// MessageId: WININET_E_CHG_POST_IS_NON_SECURE
//
// MessageText:
//
// Changing to non-secure post
//
#define WININET_E_CHG_POST_IS_NON_SECURE _HRESULT_TYPEDEF_(0x80072F0AL)

//
// MessageId: WININET_E_POST_IS_NON_SECURE
//
// MessageText:
//
// Data is being posted on a non-secure connection
//
#define WININET_E_POST_IS_NON_SECURE     _HRESULT_TYPEDEF_(0x80072F0BL)

//
// MessageId: WININET_E_CLIENT_AUTH_CERT_NEEDED
//
// MessageText:
//
// A certificate is required to complete client authentication
//
#define WININET_E_CLIENT_AUTH_CERT_NEEDED _HRESULT_TYPEDEF_(0x80072F0CL)

//
// MessageId: WININET_E_INVALID_CA
//
// MessageText:
//
// The certificate authority is invalid or incorrect
//
#define WININET_E_INVALID_CA             _HRESULT_TYPEDEF_(0x80072F0DL)

//
// MessageId: WININET_E_CLIENT_AUTH_NOT_SETUP
//
// MessageText:
//
// Client authentication has not been correctly installed
//
#define WININET_E_CLIENT_AUTH_NOT_SETUP  _HRESULT_TYPEDEF_(0x80072F0EL)

//
// MessageId: WININET_E_ASYNC_THREAD_FAILED
//
// MessageText:
//
// An error has occurred in a Wininet asynchronous thread. You may need to restart
//
#define WININET_E_ASYNC_THREAD_FAILED    _HRESULT_TYPEDEF_(0x80072F0FL)

//
// MessageId: WININET_E_REDIRECT_SCHEME_CHANGE
//
// MessageText:
//
// The protocol scheme has changed during a redirect operaiton
//
#define WININET_E_REDIRECT_SCHEME_CHANGE _HRESULT_TYPEDEF_(0x80072F10L)

//
// MessageId: WININET_E_DIALOG_PENDING
//
// MessageText:
//
// There are operations awaiting retry
//
#define WININET_E_DIALOG_PENDING         _HRESULT_TYPEDEF_(0x80072F11L)

//
// MessageId: WININET_E_RETRY_DIALOG
//
// MessageText:
//
// The operation must be retried
//
#define WININET_E_RETRY_DIALOG           _HRESULT_TYPEDEF_(0x80072F12L)

//
// MessageId: WININET_E_NO_NEW_CONTAINERS
//
// MessageText:
//
// There are no new cache containers
//
#define WININET_E_NO_NEW_CONTAINERS      _HRESULT_TYPEDEF_(0x80072F13L)

//
// MessageId: WININET_E_HTTPS_HTTP_SUBMIT_REDIR
//
// MessageText:
//
// A security zone check indicates the operation must be retried
//
#define WININET_E_HTTPS_HTTP_SUBMIT_REDIR _HRESULT_TYPEDEF_(0x80072F14L)

//
// MessageId: WININET_E_SEC_CERT_ERRORS
//
// MessageText:
//
// The SSL certificate contains errors.
//
#define WININET_E_SEC_CERT_ERRORS        _HRESULT_TYPEDEF_(0x80072F17L)

//
// MessageId: WININET_E_SEC_CERT_REV_FAILED
//
// MessageText:
//
// It was not possible to connect to the revocation server or a definitive response could not be obtained.
//
#define WININET_E_SEC_CERT_REV_FAILED    _HRESULT_TYPEDEF_(0x80072F19L)

//
// MessageId: WININET_E_HEADER_NOT_FOUND
//
// MessageText:
//
// The requested header was not found
//
#define WININET_E_HEADER_NOT_FOUND       _HRESULT_TYPEDEF_(0x80072F76L)

//
// MessageId: WININET_E_DOWNLEVEL_SERVER
//
// MessageText:
//
// The server does not support the requested protocol level
//
#define WININET_E_DOWNLEVEL_SERVER       _HRESULT_TYPEDEF_(0x80072F77L)

//
// MessageId: WININET_E_INVALID_SERVER_RESPONSE
//
// MessageText:
//
// The server returned an invalid or unrecognized response
//
#define WININET_E_INVALID_SERVER_RESPONSE _HRESULT_TYPEDEF_(0x80072F78L)

//
// MessageId: WININET_E_INVALID_HEADER
//
// MessageText:
//
// The supplied HTTP header is invalid
//
#define WININET_E_INVALID_HEADER         _HRESULT_TYPEDEF_(0x80072F79L)

//
// MessageId: WININET_E_INVALID_QUERY_REQUEST
//
// MessageText:
//
// The request for a HTTP header is invalid
//
#define WININET_E_INVALID_QUERY_REQUEST  _HRESULT_TYPEDEF_(0x80072F7AL)

//
// MessageId: WININET_E_HEADER_ALREADY_EXISTS
//
// MessageText:
//
// The HTTP header already exists
//
#define WININET_E_HEADER_ALREADY_EXISTS  _HRESULT_TYPEDEF_(0x80072F7BL)

//
// MessageId: WININET_E_REDIRECT_FAILED
//
// MessageText:
//
// The HTTP redirect request failed
//
#define WININET_E_REDIRECT_FAILED        _HRESULT_TYPEDEF_(0x80072F7CL)

//
// MessageId: WININET_E_SECURITY_CHANNEL_ERROR
//
// MessageText:
//
// An error occurred in the secure channel support
//
#define WININET_E_SECURITY_CHANNEL_ERROR _HRESULT_TYPEDEF_(0x80072F7DL)

//
// MessageId: WININET_E_UNABLE_TO_CACHE_FILE
//
// MessageText:
//
// The file could not be written to the cache
//
#define WININET_E_UNABLE_TO_CACHE_FILE   _HRESULT_TYPEDEF_(0x80072F7EL)

//
// MessageId: WININET_E_TCPIP_NOT_INSTALLED
//
// MessageText:
//
// The TCP/IP protocol is not installed properly
//
#define WININET_E_TCPIP_NOT_INSTALLED    _HRESULT_TYPEDEF_(0x80072F7FL)

//
// MessageId: WININET_E_DISCONNECTED
//
// MessageText:
//
// The computer is disconnected from the network
//
#define WININET_E_DISCONNECTED           _HRESULT_TYPEDEF_(0x80072F83L)

//
// MessageId: WININET_E_SERVER_UNREACHABLE
//
// MessageText:
//
// The server is unreachable
//
#define WININET_E_SERVER_UNREACHABLE     _HRESULT_TYPEDEF_(0x80072F84L)

//
// MessageId: WININET_E_PROXY_SERVER_UNREACHABLE
//
// MessageText:
//
// The proxy server is unreachable
//
#define WININET_E_PROXY_SERVER_UNREACHABLE _HRESULT_TYPEDEF_(0x80072F85L)

//
// MessageId: WININET_E_BAD_AUTO_PROXY_SCRIPT
//
// MessageText:
//
// The proxy auto-configuration script is in error
//
#define WININET_E_BAD_AUTO_PROXY_SCRIPT  _HRESULT_TYPEDEF_(0x80072F86L)

//
// MessageId: WININET_E_UNABLE_TO_DOWNLOAD_SCRIPT
//
// MessageText:
//
// Could not download the proxy auto-configuration script file
//
#define WININET_E_UNABLE_TO_DOWNLOAD_SCRIPT _HRESULT_TYPEDEF_(0x80072F87L)

//
// MessageId: WININET_E_SEC_INVALID_CERT
//
// MessageText:
//
// The supplied certificate is invalid
//
#define WININET_E_SEC_INVALID_CERT       _HRESULT_TYPEDEF_(0x80072F89L)

//
// MessageId: WININET_E_SEC_CERT_REVOKED
//
// MessageText:
//
// The supplied certificate has been revoked
//
#define WININET_E_SEC_CERT_REVOKED       _HRESULT_TYPEDEF_(0x80072F8AL)

//
// MessageId: WININET_E_FAILED_DUETOSECURITYCHECK
//
// MessageText:
//
// The Dialup failed because file sharing was turned on and a failure was requested if security check was needed
//
#define WININET_E_FAILED_DUETOSECURITYCHECK _HRESULT_TYPEDEF_(0x80072F8BL)

//
// MessageId: WININET_E_NOT_INITIALIZED
//
// MessageText:
//
// Initialization of the WinINet API has not occurred
//
#define WININET_E_NOT_INITIALIZED        _HRESULT_TYPEDEF_(0x80072F8CL)

//
// MessageId: WININET_E_LOGIN_FAILURE_DISPLAY_ENTITY_BODY
//
// MessageText:
//
// Login failed and the client should display the entity body to the user
//
#define WININET_E_LOGIN_FAILURE_DISPLAY_ENTITY_BODY _HRESULT_TYPEDEF_(0x80072F8EL)

//
// MessageId: WININET_E_DECODING_FAILED
//
// MessageText:
//
// Content decoding has failed
//
#define WININET_E_DECODING_FAILED        _HRESULT_TYPEDEF_(0x80072F8FL)

//
// MessageId: WININET_E_NOT_REDIRECTED
//
// MessageText:
//
// The HTTP request was not redirected
//
#define WININET_E_NOT_REDIRECTED         _HRESULT_TYPEDEF_(0x80072F80L)

//
// MessageId: WININET_E_COOKIE_NEEDS_CONFIRMATION
//
// MessageText:
//
// A cookie from the server must be confirmed by the user
//
#define WININET_E_COOKIE_NEEDS_CONFIRMATION _HRESULT_TYPEDEF_(0x80072F81L)

//
// MessageId: WININET_E_COOKIE_DECLINED
//
// MessageText:
//
// A cookie from the server has been declined acceptance
//
#define WININET_E_COOKIE_DECLINED        _HRESULT_TYPEDEF_(0x80072F82L)

//
// MessageId: WININET_E_REDIRECT_NEEDS_CONFIRMATION
//
// MessageText:
//
// The HTTP redirect request must be confirmed by the user
//
#define WININET_E_REDIRECT_NEEDS_CONFIRMATION _HRESULT_TYPEDEF_(0x80072F88L)

