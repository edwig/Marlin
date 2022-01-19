/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: SOAPMessage.h
//
// Marlin Server: Internet server/client
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

//////////////////////////////////////////////////////////////////////////
//
// SOAPMessage
//
// See SOAP 1.2 standard: http://www.w3.org/2003/05/soap-envelope
// See SOAP 1.1 standard: http://schemas.xmlsoap.org/soap/envelope/
// See SOAP 1.0 standard: <plain old soap>
//
#pragma once
#include "Headers.h"
#include "SoapTypes.h"
#include "HTTPSite.h"
#include "XMLMessage.h"
#include "Cookie.h"
#include "DefaultNamespace.h"
#include "Routing.h"
#include <winhttp.h>
#include <http.h>
#include <wincrypt.h>
#include <map>
#include <deque>
#include <vector>

// XML & SOAP
constexpr auto NAMESPACE_SOAP11     = "http://schemas.xmlsoap.org/soap/envelope/";
constexpr auto NAMESPACE_SOAP12     = "http://www.w3.org/2003/05/soap-envelope";
constexpr auto NAMESPACE_WSADDRESS  = "http://www.w3.org/2005/08/addressing";
constexpr auto NAMESPACE_XMLSCHEMA  = "http://www.w3.org/2001/XMLSchema";
constexpr auto NAMESPACE_INSTANCE   = "http://www.w3.org/2001/XMLSchema-instance";
constexpr auto NAMESPACE_ENVELOPE   = "http://www.w3.org/2003/05/soap-envelope";
constexpr auto NAMESPACE_RELIABLE   = "http://schemas.xmlsoap.org/ws/2005/02/rm";
// WS-Security
constexpr auto NAMESPACE_SIGNATURE  = "http://www.w3.org/2000/09/xmldsig#";
constexpr auto NAMESPACE_ENCODING   = "http://www.w3.org/2001/04/xmlenc#";
constexpr auto NAMESPACE_SECEXT     = "http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-wssecurity-secext-1.0.xsd";
constexpr auto NAMESPACE_SECUTILITY = "http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-wssecurity-utility-1.0.xsd";
// Must have a default namespace
#ifndef DEFAULT_NAMESPACE 
#define DEFAULT_NAMESPACE "http://www.marlinserver.org/Services"
#endif
#ifndef DEFAULT_TOKEN
#define DEFAULT_TOKEN     ":MarlinServer:"
#endif

using ushort = unsigned short;

// Reliability range
class RelRange
{
public:
  int m_lower;
  int m_upper;
};

// ResponseType controls the behaviour of the Reset() method
//
enum class ResponseType
{
   RESP_ACTION_NAME = 0   // Action name is the RequestResponse node
  ,RESP_EMPTY_BODY  = 1   // Leave body empty after Reset(), body = parameter-body
  ,RESP_ACTION_RESP = 2   // Try to compose "<name>Response" node
};

// Different types of maps for the server message
using RangeMap = std::deque<RelRange>;

// Forward declarations
class WSDLCache;
class HTTPServer;
class HTTPMessage;
class JSONMessage;
class JSONParserSOAP;

//////////////////////////////////////////////////////////////////////////
// 
// Here comes the SOAPMessage
//
//////////////////////////////////////////////////////////////////////////

class SOAPMessage : public XMLMessage
{
  // WSDL Cache nows everything about SOAPMessage
  friend WSDLCache;
public:
  // Default XTOR
  SOAPMessage();
  // XTOR from an incoming message
  SOAPMessage(HTTPMessage*  p_msg);
  // XTOR from a JSON message
  SOAPMessage(JSONMessage* p_msg);
  // XTOR from an incoming message or string data
  SOAPMessage(const char* p_soapMessage,bool p_incoming = true);
  // XTOR for a copy
  SOAPMessage(SOAPMessage* p_orig);
  // XTOR for new outgoing message
  SOAPMessage(CString&    p_namespace
             ,CString&    p_soapAction
             ,SoapVersion p_version = SoapVersion::SOAP_12
             ,CString     p_url     = ""
             ,bool        p_secure  = false
             ,CString     p_server  = ""
             ,int         p_port    = INTERNET_DEFAULT_HTTP_PORT
             ,CString     p_absPath = "");
  // DTOR
  virtual ~SOAPMessage();

  // Reset parameters, transforming it in an answer, preferable in our namespace
  virtual void    Reset(ResponseType p_responseType = ResponseType::RESP_ACTION_NAME,CString p_namespace = "");
  // Parse incoming message to members
  virtual void    ParseMessage(CString& p_message);
  // Parse incoming soap as new body of the message
  virtual void    ParseAsBody(CString& p_message);
  // Parse incoming GET URL to SOAP parameters
  virtual void    Url2SoapParameters(CrackedURL& p_url);

  // FILE OPERATIONS

  // Load from file
  virtual bool    LoadFile(const CString& p_fileName);
  virtual bool    LoadFile(const CString& p_fileName, XMLEncoding p_encoding);
  // Save to file
  virtual bool    SaveFile(const CString& p_fileName, bool p_withBom = false);
  virtual bool    SaveFile(const CString& p_fileName, XMLEncoding p_encoding, bool p_withBom = false);

  // SETTERS

  // Set the alternative namespace
  void            SetNamespace(CString p_namespace);
  // Set Command name
  void            SetSoapAction(CString& p_name);
  void            SetHasInitialAction(bool p_initial);
  void            SetSoapMustBeUnderstood(bool p_addAttribute = true,bool p_understand = true);
  // Set the SOAP version
  void            SetSoapVersion(SoapVersion p_version);
  // Reset incoming to outgoing
  void            SetIncoming(bool p_incoming);
  // Use after answer on message has been sent
  void            SetHasBeenAnswered();
  // Set the content type
  void            SetContentType(CString p_contentType);
  void            SetAcceptEncoding(CString p_encoding);
  // Set the whitespace preserving (instead of CDATA sections)
  bool            SetPreserveWhitespace(bool p_preserve = true);
  // Set the cookies
  void            SetCookie(Cookie& p_cookie);
  void            SetCookie(CString p_name,CString p_value,CString p_metadata = "",bool p_secure = false);
  void            SetCookies(Cookies& p_cookies);
  // Set request Handle
  void            SetRequestHandle(HTTP_OPAQUE_ID p_request);
  // Set URL to send message to
  void            SetURL(CString& p_url);
  void            SetStatus(unsigned p_status);
  // Set parts of the URL
  void            SetSecure(bool p_secure);
  void            SetUser(CString& p_user);
  void            SetPassword(CString& p_password);
  void            SetServer(CString& p_server);
  void            SetPort(unsigned p_port);
  void            SetAbsolutePath(CString& p_path);
  // Set security access token
  void            SetAccessToken(HANDLE p_token);
  // Set senders address
  void            SetSender(PSOCKADDR_IN6 p_sender);
  // Set senders desktop
  void            SetRemoteDesktop(UINT p_desktop);
  // Set the WSDL ordering
  void            SetWSDLOrder(WsdlOrder p_order);
  // Setting status fields
  void            SetLastMessage(bool p_last);
  void            SetErrorState(bool p_error);
  // Set headers
  void            AddHeader(CString p_name,CString p_value);
  void            AddHeader(HTTP_HEADER_ID p_id,CString p_value);
  void            DelHeader(CString p_name);
  void            DelHeader(HTTP_HEADER_ID p_id);
  // Routing
  void            AddRoute(CString p_route);
  // Set Fault elements
  void            SetFault(CString p_code
                          ,CString p_actor
                          ,CString p_string
                          ,CString p_detail);
  // WS-Reliability
  void            SetAddressing(bool p_addressing);
  void            SetReliability(bool p_reliable,bool p_throw = true);
  void            SetClientSequence(CString p_sequence);
  void            SetServerSequence(CString p_sequence);
  void            SetClientMessageNumber(int p_number);
  void            SetServerMessageNumber(int p_number);
  void            SetMessageNonce(CString p_messageGuidID);
  void            SetRange(int p_lower,int p_upper);
  // WS-Security
  void            SetSecurityLevel(XMLEncryption p_enc);
  void            SetSecurityPassword(CString p_password);
  bool            SetSigningMethod(unsigned p_method);

  // GETTERS

  // Get the service level namespace
  CString         GetNamespace() const;
  // Get name of the soap action command (e.g. for messages and debug)
  CString         GetSoapAction() const;
  bool            GetMustUnderstandAction() const;
  // Get the Soap Version
  SoapVersion     GetSoapVersion() const;
  // Incoming or outgoing message?
  bool            GetIncoming() const;
  // Get resulting soap message before sending it
  virtual CString GetSoapMessage();
  virtual CString GetSoapMessageWithBOM();
  virtual CString GetJsonMessage       (bool p_full = false,bool p_attributes = false);
  virtual CString GetJsonMessageWithBOM(bool p_full = false,bool p_attributes = false);
    // Get the content type
  CString         GetContentType() const;
  CString         GetAcceptEncoding() const;
  // Get the cookies
  Cookie*         GetCookie(unsigned p_ind);
  CString         GetCookie(unsigned p_ind = 0, CString p_metadata = "");
  CString         GetCookie(CString p_name = "",CString p_metadata = "");
  Cookies&        GetCookies();
  // Get URL destination
  CString         GetURL() const;
  CString         GetUnAuthorisedURL() const;
  unsigned        GetStatus();
  // Get Request handle
  HTTP_OPAQUE_ID  GetRequestHandle() const;
  HTTPSite*       GetHTTPSite() const;
  // Get a header by name
  CString         GetHeader(CString p_name);
  // Get basic URL from parts
  CString         GetBasicURL() const;
  // Get URL Parts
  bool            GetSecure() const;
  CString         GetUser() const;
  CString         GetPassword() const;
  CString         GetServer() const;
  unsigned        GetPort() const;
  CString         GetAbsolutePath() const;
  // Getting the JSON parameterized URL
  CString         GetJSON_URL();
  // Get the number of parameters
  int             GetParameterCount() const;
  // Get security token
  HANDLE          GetAccessToken() const;
  // Get remote desktop
  UINT            GetRemoteDesktop() const;
  // Get senders address
  PSOCKADDR_IN6   GetSender();
  // Get SOAP Faults
  bool            GetErrorState() const;
  CString         GetFaultCode() const;
  CString         GetFaultActor() const;
  CString         GetFaultString() const;
  CString         GetFaultDetail() const;
  // Get SOAP Fault as a total string
  CString         GetFault();
  // WS-Reliability
  bool            GetAddressing() const;
  bool            GetReliability() const;
  CString         GetClientSequence() const;
  CString         GetServerSequence() const;
  int             GetClientMessageNumber() const;
  int             GetServerMessageNumber() const;
  CString         GetMessageNonce() const;
  bool            GetLastMessage()  const;
  RangeMap&       GetRangeMap();
  HeaderMap*      GetHeaderMap();
  XMLEncryption   GetSecurityLevel() const;
  CString         GetSecurityPassword() const;
  WsdlOrder       GetWSDLOrder() const;
  unsigned        GetSigningMethod() const;
  XMLElement*     GetXMLBodyPart() const;
  CString         GetBodyPart();
  CString         GetCanonicalForm(XMLElement* p_element);
  bool            GetHasInitialAction() const;
  bool            GetHasBeenAnswered();
  Routing&        GetRouting();
  CString         GetRoute(int p_index);

  // PARAMETER INTERFACE
  // Most of it now is in XMLMessage in the Set/Get-Element interface
  XMLElement*     SetParameter(CString p_name,CString&    p_value);
  XMLElement*     SetParameter(CString p_name,const char* p_value);
  XMLElement*     SetParameter(CString p_name,int         p_value);
  XMLElement*     SetParameter(CString p_name,bool        p_value);
  XMLElement*     SetParameter(CString p_name,double      p_value);
  // Set parameter object name
  void            SetParameterObject(CString p_name);
  // Set/Add parameter to the header section (level 1 only)
  XMLElement*     SetHeaderParameter(CString p_paramName, const char* p_value, bool p_first = false);
  // General add a parameter (always adds, so multiple parameters of same name can be added)
  XMLElement*     AddElement(XMLElement* p_base,CString p_name,XmlDataType p_type,CString p_value,bool p_front = false);
  XMLElement*     AddElement(XMLElement* p_base,CString p_name,XmlDataType p_type,const char* p_value,bool p_front = false);
  XMLElement*     AddElement(XMLElement* p_base,CString p_name,XmlDataType p_type,int         p_value,bool p_front = false);
  XMLElement*     AddElement(XMLElement* p_base,CString p_name,XmlDataType p_type,bool        p_value,bool p_front = false);
  XMLElement*     AddElement(XMLElement* p_base,CString p_name,XmlDataType p_type,double      p_value,bool p_front = false);

  CString         GetParameter       (CString p_name);
  int             GetParameterInteger(CString p_name);
  bool            GetParameterBoolean(CString p_name);
  double          GetParameterDouble (CString p_name);
  XMLElement*     GetFirstParameter();
  // Get parameter object name (other than 'parameters')
  CString         GetParameterObject() const;
  XMLElement*     GetParameterObjectNode() const;
  // Get parameter from incoming message with check
  CString         GetParameterMandatory(CString p_paramName);
  // Get parameter from the header
  CString         GetHeaderParameter(CString p_paramName);
  // Get sub parameter from the header
  XMLElement*     GetHeaderParameterNode(CString p_paramName);

  // OPERATORS
  SOAPMessage* operator=(JSONMessage& p_json);

  // Complete the message (members to XML)
  void            CompleteTheMessage();

protected:
  // Encrypt the whole message: yielding a new message
  virtual void    EncryptMessage(CString& p_message);

  // Set the SOAP 1.1 SOAPAction from the HTTP protocol
  void            SetSoapActionFromHTTTP(CString p_action);
  // Set internal structures after XML parsing
  void            CheckAfterParsing();
  // Create header and body accordingly to SOAP version
  void            CreateHeaderAndBody();
  // Create the parameters object
  void            CreateParametersObject(ResponseType p_responseType = ResponseType::RESP_ACTION_NAME);
  // Find Header and Body after a parsed message is coming in
  void            FindHeaderAndBody();
  // TO do after we set parts of the URL in setters
  void            ReparseURL();
  // Remove namespace from a node's name
  CString         StripNamespace(const char* p_naam);
  // Return the namespace of an XML node
  CString         FindNamespace (const char* p_naam);
  // Find Fault nodes in the message at the parse state
  void            HandleSoapFault(XMLElement* p_fault);
  // Setting the fault stack of the message
  void            SetBodyToFault();
  // Parse incoming parameters
  // static bool     GetElementContent(XmlElement* element, const CString& p_xml, CString& result);
  // Add correct SOAP 1.1/1.2 envelope / header / body
  void            SetSoapEnvelope();
  void            SetSoapHeader();
  void            SetSoapBody();
  // Sub functions of AddSoapHeader
  void            AddToHeaderAcknowledgement();
  void            AddToHeaderMessageNumber();
  void            AddToHeaderMessageID();
  void            AddToHeaderReplyTo();
  void            AddToHeaderToService();
  void            AddToHeaderAction();
  void            AddToHeaderSigning();
  // Checking the incoming header for WS-Reliability
  void            CheckHeader();
  void            CheckHeaderHasSequence();
  void            CheckHeaderHasAcknowledgement();
  void            CheckHeaderRelatesTo();
  void            CheckHeaderAction();
  // Clean the body to put something different in it
  void            CleanNode(XMLElement* p_element);
  // Calculate a signing for the body to be put in the envelope header
  CString         SignBody();
  // Encrypt the body part of a message
  void            EncryptNode(CString& p_body);
  // Encrypt the body: yielding a new body
  void            EncryptBody();
  // Get body signing value from header
  CString         CheckBodySigning();
  // Get body encryption value from body
  CString         CheckBodyEncryption();
  // Get whole message encryption value
  CString         CheckMesgEncryption();
  // Get password as a custom token
  CString         GetPasswordAsToken();

  // DATA MEMBERS

  // The command
  CString         m_namespace;                            // Namespace of SOAP action (Service Contract)
  CString         m_soapAction;                           // SOAPAction from transport, or <Envelope>/<Action>
  SoapVersion     m_soapVersion { SoapVersion::SOAP_12 }; // SOAP Version
  CString         m_contentType;                          // Content type
  CString         m_acceptEncoding;                       // Accepted HTTP compression encoding
  bool            m_initialAction { true  };              // Has Action header part
  bool            m_incoming      { false };              // Incoming SOAP message
  bool            m_addAttribute  { true  };              // Add "mustUnderstand" attribute to <Envelope>/<Action>
  bool            m_understand    { true  };              // Set "mustUnderstand" to true or false
  // DESTINATION
  CString         m_url;                                  // Full URL of the soap service
  unsigned        m_status        { HTTP_STATUS_OK };     // HTTP status return code
  HTTP_OPAQUE_ID  m_request       { NULL  };              // Request it must answer
  HTTPSite*       m_site          { nullptr };            // Site for which message is received
  HeaderMap       m_headers;                              // Extra HTTP headers (incoming / outgoing)
  // URL PARTS
  bool            m_secure        { false };              // Connection is secure
  CString         m_user;                                 // Username in server part
  CString         m_password;                             // Password in server part
  CString         m_server;                               // Server name
  int             m_port { INTERNET_DEFAULT_HTTP_PORT };  // Internet Port number
  CString         m_absPath;                              // Inclusive Query + anchor
  // Message details
  XMLElement*     m_header        { nullptr };            // SOAP Header in the envelope
  XMLElement*     m_body          { nullptr };            // SOAP Body   in the envelope
  XMLElement*     m_paramObject   { nullptr };            // Parameter object within the body
  WsdlOrder       m_order         { WsdlOrder::WS_All };  // Order of parameters in the WSDL
  Cookies         m_cookies;                              // Cookies
  HANDLE          m_token         { NULL  };              // Security access token
  SOCKADDR_IN6    m_sender;                               // Senders address
  UINT            m_desktop       { 0     };              // Senders remote desktop
  Routing         m_routing;                              // Routing information from the WEB
  // Fault result                                         // 1.0,1.1         1.2
  bool            m_errorstate    { false };              // Internal for WSDL handling
  CString         m_soapFaultCode;                        // faultcode       Code
  CString         m_soapFaultActor;                       // faultactor      Subcode
  CString         m_soapFaultString;                      // faultstring     Reason/Text
  CString         m_soapFaultDetail;                      // detail          Detail
  // Addressing 
  bool            m_addressing    { false };              // WS-Addressing
  CString         m_messageGuidID;                        // WS-Addressing reply to this-message-ID
  // Reliability 
  bool            m_reliable      { false };              // Do WS-Reliability
  CString         m_guidSequenceClient;                   // Last GUID from client
  CString         m_guidSequenceServer;                   // Last GUID from server
  int             m_serverMessageNumber { 0 };            // Last server's message
  int             m_clientMessageNumber { 0 };            // Last client's message
  bool            m_lastMessage   { false };              // Last in the message-chain
  RangeMap        m_ranges;                               // Received ranges map
  // Signing and encryption
  XMLEncryption   m_encryption    { XMLEncryption::XENC_Plain };   // Signing or encrypting
  unsigned        m_signingMethod { CALG_SHA1 };          // Signing method according to "http://www.w3.org/2000/09/xmldsig#"
  CString         m_enc_user;                             // User token
  CString         m_enc_password;                         // Encryption password
};

inline void
SOAPMessage::SetNamespace(CString p_namespace)
{
  m_namespace = p_namespace;
}

inline CString 
SOAPMessage::GetNamespace() const
{
  return m_namespace;
}

inline void
SOAPMessage::SetSoapAction(CString& p_name)
{
  m_soapAction = p_name;
}

inline CString 
SOAPMessage::GetSoapAction() const
{
  return m_soapAction;
}

inline CString
SOAPMessage::GetURL() const
{
  return m_url;
}

inline int
SOAPMessage::GetParameterCount() const
{
  if(m_paramObject)
  {
    return (int)m_paramObject->GetChildren().size();
  }
  return 0;
}

inline void
SOAPMessage::SetRequestHandle(HTTP_OPAQUE_ID p_request)
{
  m_request = p_request;
}

inline HTTP_OPAQUE_ID
SOAPMessage::GetRequestHandle() const
{
  return m_request;
}

inline void
SOAPMessage::SetSecure(bool p_secure)
{
  m_secure = p_secure;
  ReparseURL();
}

inline void    
SOAPMessage::SetUser(CString& p_user)
{
  m_user = p_user;
  ReparseURL();
}

inline void    
SOAPMessage::SetPassword(CString& p_password)
{
  m_password = p_password;
  ReparseURL();
}

inline void    
SOAPMessage::SetServer(CString& p_server)
{
  m_server = p_server;
  ReparseURL();
}

inline void    
SOAPMessage::SetPort(unsigned p_port)
{
  m_port = p_port;
  ReparseURL();
}

inline void    
SOAPMessage::SetAbsolutePath(CString& p_path)
{
  m_absPath = p_path;
  ReparseURL();
}

inline bool
SOAPMessage::GetSecure() const
{
  return m_secure;
}

inline CString  
SOAPMessage::GetUser() const
{
  return m_user;
}

inline CString  
SOAPMessage::GetPassword() const
{
  return m_password;
}

inline CString  
SOAPMessage::GetServer() const
{
  return m_server;
}

inline unsigned 
SOAPMessage::GetPort() const
{
  return m_port;
}

inline CString  
SOAPMessage::GetAbsolutePath() const
{
  return m_absPath;
}

inline SoapVersion 
SOAPMessage::GetSoapVersion() const
{
  return m_soapVersion;
}

inline void
SOAPMessage::SetFault(CString p_code
                     ,CString p_actor
                     ,CString p_string
                     ,CString p_detail)
{
  if(!p_code.IsEmpty())
  {
    m_soapFaultCode   = p_code;
    m_soapFaultActor  = p_actor;
    m_soapFaultString = p_string;
    m_soapFaultDetail = p_detail;
  }
}

inline bool
SOAPMessage::GetErrorState() const
{
  return m_internalError != XmlError::XE_NoError || m_errorstate;
}
                      
inline CString
SOAPMessage::GetFaultCode() const
{
  return m_soapFaultCode;
}

inline CString
SOAPMessage::GetFaultActor() const
{
  return m_soapFaultActor;
}

inline CString
SOAPMessage::GetFaultString() const
{
  return m_soapFaultString;
}

inline CString
SOAPMessage::GetFaultDetail() const
{
  return m_soapFaultDetail;
}

inline HANDLE
SOAPMessage::GetAccessToken() const
{
  return m_token;
}

inline void
SOAPMessage::SetAccessToken(HANDLE p_token)
{
  m_token = p_token;
}

inline PSOCKADDR_IN6
SOAPMessage::GetSender() 
{
  return &m_sender;
}

inline void
SOAPMessage::SetContentType(CString p_contentType)
{
  m_contentType = p_contentType;
}

inline void    
SOAPMessage::SetClientSequence(CString p_sequence)
{
  m_guidSequenceClient = p_sequence;
}

inline void    
SOAPMessage::SetServerSequence(CString p_sequence)
{
  m_guidSequenceServer = p_sequence;
}

inline void    
SOAPMessage::SetClientMessageNumber(int p_number)
{
  m_clientMessageNumber = p_number;
}

inline void    
SOAPMessage::SetServerMessageNumber(int p_number)
{
  m_serverMessageNumber = p_number;
}

inline void    
SOAPMessage::SetMessageNonce(CString p_messageGuidID)
{
  m_messageGuidID = p_messageGuidID;
}

inline bool
SOAPMessage::GetAddressing() const
{
  return m_addressing;
}

inline bool     
SOAPMessage::GetReliability() const
{
  return m_reliable;
}

inline CString  
SOAPMessage::GetClientSequence() const
{
  return m_guidSequenceClient;
}

inline CString  
SOAPMessage::GetServerSequence() const
{
  return m_guidSequenceServer;
}

inline int      
SOAPMessage::GetClientMessageNumber() const
{
  return m_clientMessageNumber;
}

inline int      
SOAPMessage::GetServerMessageNumber() const
{
  return m_serverMessageNumber;
}

inline CString  
SOAPMessage::GetMessageNonce() const
{
  return m_messageGuidID;
}

inline bool
SOAPMessage::GetLastMessage() const
{
  return m_lastMessage;
}

inline void
SOAPMessage::SetRange(int p_lower,int p_upper)
{
  RelRange range;
  range.m_lower = p_lower;
  range.m_upper = p_upper;
  m_ranges.push_back(range);
}

inline RangeMap& 
SOAPMessage::GetRangeMap()
{
  return m_ranges;
}

inline HeaderMap*
SOAPMessage::GetHeaderMap()
{
  return &m_headers;
}

inline void
SOAPMessage::SetRemoteDesktop(UINT p_desktop)
{
  m_desktop = p_desktop;
}

inline UINT
SOAPMessage::GetRemoteDesktop() const
{
  return m_desktop;
}

inline void
SOAPMessage::SetSecurityLevel(XMLEncryption p_enc)
{
  m_encryption  = p_enc;
  m_soapVersion = SoapVersion::SOAP_12;
}

inline void
SOAPMessage::SetSecurityPassword(CString p_password)
{
  m_enc_password = p_password;
}

inline XMLEncryption 
SOAPMessage::GetSecurityLevel() const
{
  return m_encryption;
}

inline CString
SOAPMessage::GetSecurityPassword() const
{
  return m_enc_password;
}

inline void
SOAPMessage::SetCookie(Cookie& p_cookie)
{
  m_cookies.AddCookie(p_cookie);
}

inline Cookies&
SOAPMessage::GetCookies()
{
  return m_cookies;
}

inline void
SOAPMessage::SetCookies(Cookies& p_cookies)
{
  m_cookies = p_cookies;
}

inline HTTPSite*
SOAPMessage::GetHTTPSite() const
{
  return m_site;
}

inline WsdlOrder
SOAPMessage::GetWSDLOrder() const
{
  return m_order;
}

inline void
SOAPMessage::SetWSDLOrder(WsdlOrder p_order)
{
  m_order = p_order;
}

inline unsigned
SOAPMessage::GetSigningMethod() const
{
  return m_signingMethod;
}

inline bool
SOAPMessage::GetHasInitialAction() const
{
  return m_initialAction;
}

inline void
SOAPMessage::SetHasInitialAction(bool p_initial)
{
  m_initialAction = p_initial;
}

inline bool
SOAPMessage::GetIncoming() const
{
  return m_incoming;
}

inline void
SOAPMessage::SetIncoming(bool p_incoming)
{
  m_incoming = p_incoming;
}

inline void
SOAPMessage::SetLastMessage(bool p_last)
{
  m_lastMessage = p_last;
}

inline void
SOAPMessage::SetErrorState(bool p_error)
{
  m_errorstate = p_error;
}

inline XMLElement*
SOAPMessage::GetParameterObjectNode() const
{
  return m_paramObject;
}

inline CString
SOAPMessage::GetAcceptEncoding() const
{
  return m_acceptEncoding;
}

inline bool
SOAPMessage::GetMustUnderstandAction() const
{
  return m_understand;
}

inline XMLElement*
SOAPMessage::GetXMLBodyPart() const
{
  return m_body;
}

inline bool
SOAPMessage::GetHasBeenAnswered()
{
  return m_request == NULL;
}

inline void
SOAPMessage::SetHasBeenAnswered()
{ 
  m_request = NULL; 
}

inline void
SOAPMessage::AddRoute(CString p_route)
{
  m_routing.push_back(p_route);
}

inline Routing&
SOAPMessage::GetRouting()
{
  return m_routing;
}

inline unsigned
SOAPMessage::GetStatus() 
{
  return m_status; 
};

inline void
SOAPMessage::SetStatus(unsigned p_status)
{
  m_status = p_status;
}

//////////////////////////////////////////////////////////////////////////
//
// PARAMETER INTERFACE
//
//////////////////////////////////////////////////////////////////////////

inline CString
SOAPMessage::GetParameter(CString p_name)
{
  return GetElement(m_paramObject,p_name);
}

inline int
SOAPMessage::GetParameterInteger(CString p_name)
{
  return GetElementInteger(m_paramObject,p_name);
}

inline bool
SOAPMessage::GetParameterBoolean(CString p_name)
{
  return GetElementBoolean(m_paramObject,p_name);
}

inline double
SOAPMessage::GetParameterDouble(CString p_name)
{
  return GetElementDouble(m_paramObject,p_name);
}

