/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: SOAPMessage.h
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
#include "XMLMessage.h"
#include "Cookie.h"
#include "Routing.h"
#include "CrackURL.h"
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
constexpr auto NAMESPACE_SECURITY   = "http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-soap-message-security-1.0";

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
class HTTPSite;

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
  explicit SOAPMessage(HTTPMessage*  p_msg);
  // XTOR from a JSON message
  explicit SOAPMessage(JSONMessage* p_msg);
  // XTOR from an incoming message or string data
  explicit SOAPMessage(const char* p_soapMessage,bool p_incoming = true);
  // XTOR for a copy
  explicit SOAPMessage(SOAPMessage* p_orig);
  // XTOR for new outgoing message
  explicit SOAPMessage(XString&    p_namespace
                      ,XString&    p_soapAction
                      ,SoapVersion p_version = SoapVersion::SOAP_12
                      ,XString     p_url     = ""
                      ,bool        p_secure  = false
                      ,XString     p_server  = ""
                      ,int         p_port    = INTERNET_DEFAULT_HTTP_PORT
                      ,XString     p_absPath = "");
  // DTOR
  virtual ~SOAPMessage();

  // Reset parameters, transforming it in an answer, preferable in our namespace
  virtual void    Reset(ResponseType p_responseType = ResponseType::RESP_ACTION_NAME,XString p_namespace = "");
  // Parse incoming message to members
  virtual void    ParseMessage(XString& p_message);
  // Parse incoming soap as new body of the message
  virtual void    ParseAsBody(XString& p_message);
  // Parse incoming GET URL to SOAP parameters
  virtual void    Url2SoapParameters(const CrackedURL& p_url);

  // FILE OPERATIONS

  // Load from file
  virtual bool    LoadFile(const XString& p_fileName) override;
  virtual bool    LoadFile(const XString& p_fileName, StringEncoding p_encoding) override;
  // Save to file
  virtual bool    SaveFile(const XString& p_fileName, bool p_withBom = false) override;
  virtual bool    SaveFile(const XString& p_fileName, StringEncoding p_encoding, bool p_withBom = false) override;

  // SETTERS

  // Set the alternative namespace
  void            SetNamespace(XString p_namespace);
  // Set Command name
  void            SetSoapAction(const XString& p_name);
  void            SetHasInitialAction(bool p_initial);
  void            SetSoapMustBeUnderstood(bool p_addAttribute = true,bool p_understand = true);
  // Set the SOAP version
  void            SetSoapVersion(SoapVersion p_version);
  // Reset incoming to outgoing
  void            SetIncoming(bool p_incoming);
  // Use after answer on message has been sent
  void            SetHasBeenAnswered();
  // Set the content type
  void            SetContentType(XString p_contentType);
  void            SetAcceptEncoding(XString p_encoding);
  // Set the whitespace preserving (instead of CDATA sections)
  bool            SetPreserveWhitespace(bool p_preserve = true);
  // Set the cookies
  void            SetCookie(Cookie& p_cookie);
  void            SetCookie(XString p_name,XString p_value,XString p_metadata = "",bool p_secure = false);
  void            SetCookies(const Cookies& p_cookies);
  // Set request Handle
  void            SetRequestHandle(HTTP_OPAQUE_ID p_request);
  // Set URL to send message to
  void            SetURL(const XString& p_url);
  void            SetStatus(unsigned p_status);
  // Set parts of the URL
  void            SetSecure(bool p_secure);
  void            SetServer(const XString& p_server);
  void            SetPort(unsigned p_port);
  void            SetAbsolutePath(const XString& p_path);
  void            SetExtension(XString p_extension);
  // Set security details
  void            SetUser(const XString& p_user);
  void            SetPassword(const XString& p_password);
  void            SetTokenNonce(XString p_nonce);
  void            SetTokenCreated(XString p_created);
  bool            SetTokenProfile(XString p_user,XString p_password,XString p_created,XString p_nonce = "");
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
  void            AddHeader(XString p_name,XString p_value);
  void            AddHeader(HTTP_HEADER_ID p_id,XString p_value);
  void            DelHeader(XString p_name);
  void            DelHeader(HTTP_HEADER_ID p_id);
  // Routing
  void            AddRoute(XString p_route);
  // Set Fault elements
  void            SetFault(XString p_code
                          ,XString p_actor
                          ,XString p_string
                          ,XString p_detail);
  // WS-Reliability
  void            SetAddressing(bool p_addressing);
  void            SetReliability(bool p_reliable,bool p_throw = true);
  void            SetClientSequence(XString p_sequence);
  void            SetServerSequence(XString p_sequence);
  void            SetClientMessageNumber(int p_number);
  void            SetServerMessageNumber(int p_number);
  void            SetMessageNonce(XString p_messageGuidID);
  void            SetRange(int p_lower,int p_upper);
  // WS-Security
  void            SetSecurityLevel(XMLEncryption p_enc);
  void            SetSecurityPassword(XString p_password);
  bool            SetSigningMethod(unsigned p_method);

  // GETTERS

  // Get the service level namespace
  XString         GetNamespace() const;
  // Get name of the soap action command (e.g. for messages and debug)
  XString         GetSoapAction() const;
  bool            GetMustUnderstandAction() const;
  // Get the Soap Version
  SoapVersion     GetSoapVersion() const;
  // Incoming or outgoing message?
  bool            GetIncoming() const;
  // Get resulting soap message before sending it
  virtual XString GetSoapMessage();
  virtual XString GetSoapMessageWithBOM();
  virtual XString GetJsonMessage       (bool p_full = false,bool p_attributes = false);
  virtual XString GetJsonMessageWithBOM(bool p_full = false,bool p_attributes = false);
    // Get the content type
  XString         GetContentType() const;
  XString         GetAcceptEncoding() const;
  // Get the cookies
  Cookie*         GetCookie(unsigned p_ind);
  XString         GetCookie(unsigned p_ind = 0, XString p_metadata = "");
  XString         GetCookie(XString p_name = "",XString p_metadata = "");
  const Cookies&  GetCookies() const;
  // Get URL destination
  XString         GetURL() const;
  const CrackedURL& GetCrackedURL() const;
  XString         GetUnAuthorisedURL() const;
  unsigned        GetStatus() const;
  // Get Request handle
  HTTP_OPAQUE_ID  GetRequestHandle() const;
  HTTPSite*       GetHTTPSite() const;
  // Get a header by name
  XString         GetHeader(XString p_name);
  // Get URL Parts
  bool            GetSecure() const;
  XString         GetServer() const;
  unsigned        GetPort() const;
  XString         GetAbsolutePath() const;
  XString         GetExtension() const;
  // Get Security parts
  XString         GetUser() const;
  XString         GetPassword() const;
  XString         GetTokenNonce() const;
  XString         GetTokenCreated() const;
  // Getting the JSON parameterized URL
  XString         GetJSON_URL();
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
  XString         GetFaultCode() const;
  XString         GetFaultActor() const;
  XString         GetFaultString() const;
  XString         GetFaultDetail() const;
  // Get SOAP Fault as a total string
  XString         GetFault();
  // WS-Reliability
  bool            GetAddressing() const;
  bool            GetReliability() const;
  XString         GetClientSequence() const;
  XString         GetServerSequence() const;
  int             GetClientMessageNumber() const;
  int             GetServerMessageNumber() const;
  XString         GetMessageNonce() const;
  bool            GetLastMessage()  const;
  RangeMap&       GetRangeMap();
  const HeaderMap* GetHeaderMap() const;
  XMLEncryption   GetSecurityLevel() const;
  XString         GetSecurityPassword() const;
  WsdlOrder       GetWSDLOrder() const;
  unsigned        GetSigningMethod() const;
  XMLElement*     GetXMLBodyPart() const;
  XString         GetBodyPart();
  XString         GetCanonicalForm(XMLElement* p_element);
  bool            GetHasInitialAction() const;
  bool            GetHasBeenAnswered();
  const Routing&  GetRouting() const;
  XString         GetRoute(int p_index);

  // PARAMETER INTERFACE
  // Most of it now is in XMLMessage in the Set/Get-Element interface
  XMLElement*     SetParameter(XString p_name,XString&    p_value);
  XMLElement*     SetParameter(XString p_name,const char* p_value);
  XMLElement*     SetParameter(XString p_name,int         p_value);
  XMLElement*     SetParameter(XString p_name,bool        p_value);
  XMLElement*     SetParameter(XString p_name,double      p_value);
  // Set parameter object name
  void            SetParameterObject(XString p_name);
  // Set/Add parameter to the header section (level 1 only)
  XMLElement*     SetHeaderParameter(XString p_paramName, const char* p_value, bool p_first = false);
  // General add a parameter (always adds, so multiple parameters of same name can be added)
  XMLElement*     AddElement(XMLElement* p_base,XString p_name,XmlDataType p_type,XString           p_value,bool p_front = false);
  XMLElement*     AddElement(XMLElement* p_base,XString p_name,XmlDataType p_type,const char*       p_value,bool p_front = false);
  XMLElement*     AddElement(XMLElement* p_base,XString p_name,XmlDataType p_type,int               p_value,bool p_front = false);
  XMLElement*     AddElement(XMLElement* p_base,XString p_name,XmlDataType p_type,unsigned          p_value,bool p_front = false);
  XMLElement*     AddElement(XMLElement* p_base,XString p_name,XmlDataType p_type,bool              p_value,bool p_front = false);
  XMLElement*     AddElement(XMLElement* p_base,XString p_name,XmlDataType p_type,double            p_value,bool p_front = false);
  XMLElement*     AddElement(XMLElement* p_base,XString p_name,XmlDataType p_type,const bcd&        p_value,bool p_front = false);
  XMLElement*     AddElement(XMLElement* p_base,XString p_name,XmlDataType p_type,__int64           p_value,bool p_front = false);
  XMLElement*     AddElement(XMLElement* p_base,XString p_name,XmlDataType p_type,unsigned __int64  p_value,bool p_front = false);

  XString         GetParameter       (XString p_name);
  int             GetParameterInteger(XString p_name);
  bool            GetParameterBoolean(XString p_name);
  double          GetParameterDouble (XString p_name);
  XMLElement*     GetFirstParameter();
  // Get parameter object name (other than 'parameters')
  XString         GetParameterObject() const;
  XMLElement*     GetParameterObjectNode() const;
  // Get parameter from incoming message with check
  XString         GetParameterMandatory(XString p_paramName);
  // Get parameter from the header
  XString         GetHeaderParameter(XString p_paramName);
  // Get sub parameter from the header
  XMLElement*     GetHeaderParameterNode(XString p_paramName);

  // OPERATORS
  SOAPMessage* operator=(JSONMessage& p_json);

  // Complete the message (members to XML)
  void            CompleteTheMessage();
  // Clean up the empty elements in the message
  bool            CleanUp();

protected:
  // Encrypt the whole message: yielding a new message
  virtual void    EncryptMessage(XString& p_message) override;

  // Set the SOAP 1.1 SOAPAction from the HTTP protocol
  void            SetSoapActionFromHTTTP(XString p_action);
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
  XString         StripNamespace(const char* p_naam);
  // Return the namespace of an XML node
  XString         FindNamespace (const char* p_naam);
  // Find Fault nodes in the message at the parse state
  void            HandleSoapFault(XMLElement* p_fault);
  // Setting the fault stack of the message
  void            SetBodyToFault();
  // Parse incoming parameters
  // static bool     GetElementContent(XmlElement* element, const XString& p_xml, XString& result);
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
  XString         SignBody();
  // Encrypt the body part of a message
  void            EncryptNode(XString& p_body);
  // Encrypt the body: yielding a new body
  void            EncryptBody();
  // Get body signing value from header
  void            CheckBodySigning();
  // Get incoming authentication from the Security/UsernameToken
  void            CheckUsernameToken(XMLElement* p_token);
  // Get body encryption value from body
  XString         CheckBodyEncryption();
  // Get whole message encryption value
  XString         CheckMesgEncryption();
  // Get password as a custom token
  XString         GetPasswordAsToken();

  // DATA MEMBERS

  // The command
  XString         m_namespace;                            // Namespace of SOAP action (Service Contract)
  XString         m_soapAction;                           // SOAPAction from transport, or <Envelope>/<Action>
  SoapVersion     m_soapVersion { SoapVersion::SOAP_12 }; // SOAP Version
  XString         m_contentType;                          // Content type
  XString         m_acceptEncoding;                       // Accepted HTTP compression encoding
  bool            m_initialAction { true  };              // Has Action header part
  bool            m_incoming      { false };              // Incoming SOAP message
  bool            m_addAttribute  { true  };              // Add "mustUnderstand" attribute to <Envelope>/<Action>
  bool            m_understand    { true  };              // Set "mustUnderstand" to true or false
  // DESTINATION
  unsigned        m_status        { HTTP_STATUS_OK };     // HTTP status return code
  HTTP_OPAQUE_ID  m_request       { NULL  };              // Request it must answer
  HTTPSite*       m_site          { nullptr };            // Site for which message is received
  HeaderMap       m_headers;                              // Extra HTTP headers (incoming / outgoing)
  // URL PARTS
  XString         m_url;                                  // Full URL of the soap service
  CrackedURL      m_cracked;                              // URL in cracked parts
  // Security details
  XString         m_user;                                 // Username in server part
  XString         m_password;                             // Password in server part
  XString         m_tokenNonce;                           // Username token profile nonce
  XString         m_tokenCreated;                         // Username token profile created
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
  XString         m_soapFaultCode;                        // faultcode       Code
  XString         m_soapFaultActor;                       // faultactor      Subcode
  XString         m_soapFaultString;                      // faultstring     Reason/Text
  XString         m_soapFaultDetail;                      // detail          Detail
  // Addressing 
  bool            m_addressing    { false };              // WS-Addressing
  XString         m_messageGuidID;                        // WS-Addressing reply to this-message-ID
  // Reliability 
  bool            m_reliable      { false };              // Do WS-Reliability
  XString         m_guidSequenceClient;                   // Last GUID from client
  XString         m_guidSequenceServer;                   // Last GUID from server
  int             m_serverMessageNumber { 0 };            // Last server's message
  int             m_clientMessageNumber { 0 };            // Last client's message
  bool            m_lastMessage   { false };              // Last in the message-chain
  RangeMap        m_ranges;                               // Received ranges map
  // Signing and encryption
  XMLEncryption   m_encryption    { XMLEncryption::XENC_Plain };   // Signing or encrypting
  unsigned        m_signingMethod { CALG_SHA1 };          // Signing method according to "http://www.w3.org/2000/09/xmldsig#"
  XString         m_enc_user;                             // User token
  XString         m_enc_password;                         // Encryption password
};

inline void
SOAPMessage::SetNamespace(XString p_namespace)
{
  m_namespace = p_namespace;
}

inline XString 
SOAPMessage::GetNamespace() const
{
  return m_namespace;
}

inline void
SOAPMessage::SetSoapAction(const XString& p_name)
{
  m_soapAction = p_name;
}

inline XString 
SOAPMessage::GetSoapAction() const
{
  return m_soapAction;
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
  m_cracked.m_secure = p_secure;
  ReparseURL();
}

inline void    
SOAPMessage::SetUser(const XString& p_user)
{
  m_user = p_user;
}

inline void    
SOAPMessage::SetPassword(const XString& p_password)
{
  m_password = p_password;
}

inline void
SOAPMessage::SetTokenNonce(XString p_nonce)
{
  m_tokenNonce = p_nonce;
}

inline void
SOAPMessage::SetTokenCreated(XString p_created)
{
  m_tokenCreated = p_created;
}

inline void    
SOAPMessage::SetServer(const XString& p_server)
{
  m_cracked.m_host = p_server;
  ReparseURL();
}

inline void    
SOAPMessage::SetPort(unsigned p_port)
{
  m_cracked.m_port = p_port;
  ReparseURL();
}

inline void
SOAPMessage::SetExtension(XString p_extension)
{
  m_cracked.SetExtension(p_extension);
  ReparseURL();
}

inline void    
SOAPMessage::SetAbsolutePath(const XString& p_path)
{
  m_cracked.m_path = p_path;
  ReparseURL();
}

inline const CrackedURL& 
SOAPMessage::GetCrackedURL() const
{
  return m_cracked;
}

inline bool
SOAPMessage::GetSecure() const
{
  return m_cracked.m_secure;
}

inline XString  
SOAPMessage::GetUser() const
{
  return m_user;
}

inline XString  
SOAPMessage::GetPassword() const
{
  return m_password;
}

inline XString
SOAPMessage::GetTokenNonce() const
{
  return m_tokenNonce;
}

inline XString
SOAPMessage::GetTokenCreated() const
{
  return m_tokenCreated;
}

inline XString  
SOAPMessage::GetServer() const
{
  return m_cracked.m_host;
}

inline unsigned 
SOAPMessage::GetPort() const
{
  return m_cracked.m_port;
}

inline XString
SOAPMessage::GetExtension() const
{
  return m_cracked.GetExtension();
}

inline XString  
SOAPMessage::GetAbsolutePath() const
{
  return m_cracked.AbsolutePath();
}

inline SoapVersion 
SOAPMessage::GetSoapVersion() const
{
  return m_soapVersion;
}

inline void
SOAPMessage::SetFault(XString p_code
                     ,XString p_actor
                     ,XString p_string
                     ,XString p_detail)
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
                      
inline XString
SOAPMessage::GetFaultCode() const
{
  return m_soapFaultCode;
}

inline XString
SOAPMessage::GetFaultActor() const
{
  return m_soapFaultActor;
}

inline XString
SOAPMessage::GetFaultString() const
{
  return m_soapFaultString;
}

inline XString
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
SOAPMessage::SetContentType(XString p_contentType)
{
  m_contentType = p_contentType;
}

inline void    
SOAPMessage::SetClientSequence(XString p_sequence)
{
  m_guidSequenceClient = p_sequence;
}

inline void    
SOAPMessage::SetServerSequence(XString p_sequence)
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
SOAPMessage::SetMessageNonce(XString p_messageGuidID)
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

inline XString  
SOAPMessage::GetClientSequence() const
{
  return m_guidSequenceClient;
}

inline XString  
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

inline XString  
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

inline const HeaderMap*
SOAPMessage::GetHeaderMap() const
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
SOAPMessage::SetSecurityPassword(XString p_password)
{
  m_enc_password = p_password;
}

inline XMLEncryption 
SOAPMessage::GetSecurityLevel() const
{
  return m_encryption;
}

inline XString
SOAPMessage::GetSecurityPassword() const
{
  return m_enc_password;
}

inline void
SOAPMessage::SetCookie(Cookie& p_cookie)
{
  m_cookies.AddCookie(p_cookie);
}

inline const Cookies&
SOAPMessage::GetCookies() const
{
  return m_cookies;
}

inline void
SOAPMessage::SetCookies(const Cookies& p_cookies)
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

inline XString
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
SOAPMessage::AddRoute(XString p_route)
{
  m_routing.push_back(p_route);
}

inline const Routing&
SOAPMessage::GetRouting() const
{
  return m_routing;
}

inline unsigned
SOAPMessage::GetStatus() const
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

inline XString
SOAPMessage::GetParameter(XString p_name)
{
  return GetElement(m_paramObject,p_name);
}

inline int
SOAPMessage::GetParameterInteger(XString p_name)
{
  return GetElementInteger(m_paramObject,p_name);
}

inline bool
SOAPMessage::GetParameterBoolean(XString p_name)
{
  return GetElementBoolean(m_paramObject,p_name);
}

inline double
SOAPMessage::GetParameterDouble(XString p_name)
{
  return GetElementDouble(m_paramObject,p_name);
}

