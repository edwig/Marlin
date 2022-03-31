/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: SOAPMessage.cpp
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
#include "SOAPMessage.h"
#include "HTTPMessage.h"
#include "JSONMessage.h"
#include "CrackURL.h"
#include "Crypto.h"
#include "GenerateGUID.h"
#include "Base64.h"
#include "Namespace.h"
#include "ConvertWideString.h"
#include "XMLParser.h"
#include "XMLParserJSON.h"
#include <utility>

#pragma region XTOR

// General XTOR
// Purpose: Opaque SOAP Message
SOAPMessage::SOAPMessage()
{
  memset(&m_sender, 0, sizeof(SOCKADDR_IN6));
}

// XTOR from an incoming message
// Purpose: Incoming SOAP message from HTTP/POST
SOAPMessage::SOAPMessage(HTTPMessage* p_msg)
{
  m_request       = p_msg->GetRequestHandle();
  m_site          = p_msg->GetHTTPSite();
  m_url           = p_msg->GetURL();
  m_cracked       = p_msg->GetCrackedURL();
  m_status        = p_msg->GetStatus();
  m_user          = p_msg->GetUser();
  m_password      = p_msg->GetPassword();
  m_contentType   = p_msg->GetContentType();
  m_acceptEncoding= p_msg->GetAcceptEncoding();
  m_sendBOM       = p_msg->GetSendBOM();
  m_headers       =*p_msg->GetHeaderMap();
  m_incoming      = p_msg->GetCommand() != HTTPCommand::http_response;

  // Overrides from class defaults
  m_soapVersion   = SoapVersion::SOAP_10;
  m_initialAction = false;

  // Duplicate the HTTP token for ourselves
  if(DuplicateTokenEx(p_msg->GetAccessToken()
                     ,TOKEN_DUPLICATE|TOKEN_IMPERSONATE|TOKEN_ALL_ACCESS|TOKEN_READ|TOKEN_WRITE
                     ,NULL
                     ,SecurityImpersonation
                     ,TokenImpersonation
                     ,&m_token) == FALSE)
  {
    m_token = NULL;
  }

  // Duplicate all cookies
  m_cookies = p_msg->GetCookies();

  // Get sender (if any) from the HTTP message
  memcpy(&m_sender,p_msg->GetSender(),sizeof(SOCKADDR_IN6));
  m_desktop = p_msg->GetRemoteDesktop();

  // Copy routing information
  m_routing = p_msg->GetRouting();

  // Parse the body as a posted message
  XString message;
  XString charset = FindCharsetInContentType(m_contentType);
  if(charset.Left(6).CompareNoCase("utf-16") == 0)
  {
    // Works for "UTF-16", "UTF-16LE" and "UTF-16BE" as of RFC 2781
    uchar* buffer = nullptr;
    size_t length = 0;
    p_msg->GetRawBody(&buffer,length);

    int uni = IS_TEXT_UNICODE_UNICODE_MASK;  // Intel/AMD processors + BOM
    if(IsTextUnicode(buffer,(int)length,&uni))
    {
      if(TryConvertWideString(buffer,(int)length,"",message,m_sendBOM))
      {
        m_sendUnicode = true;
      }
      else
      {
        // We are now officially in error state
        m_errorstate = true;
        SetFault("Charset","Client","Cannot convert UTF-16 buffer","");
      }
    }
    else
    {
      // Probably already processed in HandleTextContext of the server
      message = p_msg->GetBody();
    }
    delete [] buffer;
  }
  else
  {
    message = p_msg->GetBody();
  }
  ParseMessage(message);

  // If a SOAP version is not found during parsing
  if(m_soapVersion < SoapVersion::SOAP_12)
  {
    // Getting SOAP 1.0 or 1.1 SOAPAction from the unknown-headers
    SetSoapActionFromHTTTP(p_msg->GetHeader("SOAPAction"));
  }
  else
  {
    // Getting SOAP 1.2 action from the Content-Type header
    XString action = FindFieldInHTTPHeader(m_contentType,"Action");
    SetSoapActionFromHTTTP(action);
  }
}

// XTOR from a JSON message
SOAPMessage::SOAPMessage(JSONMessage* p_msg)
{
  m_request       = p_msg->GetRequestHandle();
  m_site          = p_msg->GetHTTPSite();
  m_url           = p_msg->GetURL();
  m_cracked       = p_msg->GetCrackedURL();
  m_status        = p_msg->GetStatus();
  m_user          = p_msg->GetUser();
  m_password      = p_msg->GetPassword();
  m_contentType   = p_msg->GetContentType();
  m_sendBOM       = p_msg->GetSendBOM();
  m_incoming      = p_msg->GetIncoming();
  m_acceptEncoding= p_msg->GetAcceptEncoding();
  m_headers       =*p_msg->GetHeaderMap();

  // Duplicate all cookies
  m_cookies = p_msg->GetCookies();

  // Duplicate routing
  m_routing = p_msg->GetRouting();

  // Get sender (if any) from the HTTP message
  memcpy(&m_sender,p_msg->GetSender(),sizeof(SOCKADDR_IN6));
  m_desktop = p_msg->GetDesktop();

  // Duplicate the HTTP token for ourselves
  if(DuplicateTokenEx(p_msg->GetAccessToken()
                      ,TOKEN_DUPLICATE | TOKEN_IMPERSONATE | TOKEN_ALL_ACCESS | TOKEN_READ | TOKEN_WRITE
                      ,NULL
                      ,SecurityImpersonation
                      ,TokenImpersonation
                      ,&m_token) == FALSE)
  {
    m_token = NULL;
  }

  CreateHeaderAndBody();
  CreateParametersObject();

  // The message itself
  XMLParserJSON(this,p_msg);

  CheckAfterParsing();
}

// XTOR for an outgoing message
// Purpose: New outgoing SOAP message, yet to be defined
SOAPMessage::SOAPMessage(XString&     p_namespace
                        ,XString&     p_soapAction
                        ,SoapVersion  p_version /*=SOAP_12*/
                        ,XString      p_url     /*=""*/
                        ,bool         p_secure  /*=false*/
                        ,XString      p_server  /*=""*/
                        ,int          p_port    /*=INTERNET_DEFAULT_HHTP_PORT*/
                        ,XString      p_absPath /*=""*/)
            :m_namespace  (p_namespace)
            ,m_soapAction(p_soapAction)
            ,m_soapVersion(p_version)
{
  // Overrides from class defaults
  m_initialAction = (p_version >= SoapVersion::SOAP_11) ? true : false;

  // No senders address
  memset(&m_sender,0,sizeof(SOCKADDR_IN6));

  if(!p_url.IsEmpty())
  {
    m_url = p_url;
    m_cracked.CrackURL(p_url);
  }
  else
  {
    m_cracked.m_secure = p_secure;
    m_cracked.m_host   = p_server;
    m_cracked.m_port   = p_port;
    m_cracked.m_path   = p_absPath;
    m_url = m_cracked.URL();
  }

  // Create for p_version = SOAP 1.2
  CreateHeaderAndBody();
  CreateParametersObject();
}

// XTOR from an incoming message or string data
// Purpose: Incoming SOAP message from other transport protocol than HTTP
SOAPMessage::SOAPMessage(const char* p_soapMessage,bool p_incoming /*=true*/)
            :m_incoming(p_incoming)
{
  // Overrides from class defaults
  m_soapVersion   = SoapVersion::SOAP_10;
  m_initialAction = false;

  // No senders address yet
  memset(&m_sender,0,sizeof(SOCKADDR_IN6));

  XString message(p_soapMessage);
  ParseMessage(message);  
}

// XTOR for a copy constructor
SOAPMessage::SOAPMessage(SOAPMessage* p_orig)
            :XMLMessage(p_orig)
{
  // Copy all data members
  m_errorstate    = p_orig->m_errorstate;
  m_namespace     = p_orig->m_namespace;
  m_soapAction    = p_orig->m_soapAction;
  m_soapVersion   = p_orig->m_soapVersion;
  m_encoding      = p_orig->m_encoding;
  m_acceptEncoding= p_orig->m_acceptEncoding;
  m_sendUnicode   = p_orig->m_sendUnicode;
  m_sendBOM       = p_orig->m_sendBOM;
  m_url           = p_orig->m_url;
  m_cracked       = p_orig->m_cracked;
  m_status        = p_orig->m_status;
  m_request       = p_orig->m_request;
  m_user          = p_orig->m_user;
  m_password      = p_orig->m_password;
  m_site          = p_orig->m_site;
  m_desktop       = p_orig->m_desktop;
  m_order         = p_orig->m_order;
  m_incoming      = p_orig->m_incoming;
  m_headers       = p_orig->m_headers;
  // WS-Reliability
  m_addressing    = p_orig->m_addressing;
  m_reliable      = p_orig->m_reliable;
  m_lastMessage   = p_orig->m_lastMessage;
  m_serverMessageNumber = p_orig->m_serverMessageNumber;
  m_clientMessageNumber = p_orig->m_clientMessageNumber;
  // WS-Security
  m_encryption    = p_orig->m_encryption;
  m_signingMethod = p_orig->m_signingMethod;
  m_initialAction = p_orig->m_initialAction;

  // Duplicate cookies
  m_cookies = p_orig->m_cookies;
  // Duplicate routing
  m_routing = p_orig->m_routing;

  // Duplicate the HTTP token for ourselves
  if(DuplicateTokenEx(p_orig->m_token
                     ,TOKEN_DUPLICATE|TOKEN_IMPERSONATE|TOKEN_ALL_ACCESS|TOKEN_READ|TOKEN_WRITE
                     ,NULL
                     ,SecurityImpersonation
                     ,TokenImpersonation
                     ,&m_token) == FALSE)
  {
    m_token = NULL;
  }

  // Get sender (if any) from the soap message
  memcpy(&m_sender,p_orig->GetSender(),sizeof(SOCKADDR_IN6));

  // Reset the header/body pointers
  FindHeaderAndBody();
}

// DTOR
SOAPMessage::~SOAPMessage()
{
  // Free the authentication token
  if(m_token)
  {
    CloseHandle(m_token);
    m_token = NULL;
  }
}

// Set the SOAP 1.1 SOAPAction from the HTTP protocol
void
SOAPMessage::SetSoapActionFromHTTTP(XString p_action)
{
  // Remove double quotes on both sides
  p_action.Trim('\"');

  // Split the (soap)action
  XString namesp,action;

  // STEP 1: Soap action from HTTPHeaders
  SplitNamespaceAndAction(p_action,namesp,action);
 
  if(m_soapVersion >= SoapVersion::SOAP_12)
  {
    // STEP 2: If SOAP 1.2
    if(m_header)
    {
      // STEP 3: Find WS-Adressing <Header>/<Action>
      XMLElement* xmlaction = FindElement(m_header,"Action");
      if(xmlaction)
      {
        SplitNamespaceAndAction(xmlaction->GetValue(),namesp,action);
      }
    }
  }
  // OK: Use this set (action,namesp)
  m_soapAction = action;
//   if(!namesp.IsEmpty())
//   {
//     m_namespace = namesp;
//   }
}

#pragma endregion XTOR and DTOR of a SOAP message

#pragma region ReUse

// Reset parameters, transforming it in an answer
void 
SOAPMessage::Reset(ResponseType p_responseType  /* = ResponseType::RESP_ACTION_NAME */
                  ,XString      p_namespace     /* = "" */)
{
  XMLMessage::Reset();

  // Reset pointers
  m_body        = m_root;
  m_header      = nullptr;
  m_paramObject = nullptr;

  // Reset the error status
  m_internalError = XmlError::XE_NoError;
  m_internalErrorString.Empty();

  // it is now no longer an incoming message
  m_incoming = false;

  // Reset the HTTP headers
  m_headers.clear();

  // Reset the URL
  m_url.Empty();
  m_cracked.Reset();

  // If, given: use the our namespace for an answer
  if(!p_namespace.IsEmpty())
  {
    m_namespace = p_namespace;
  }

  // Re-Create the XML parts
  CreateHeaderAndBody();
  CreateParametersObject(p_responseType);

  // Reset sender!!
  memset(&m_sender,0,sizeof(SOCKADDR_IN6));

  // Reset the SOAP Fault status
  m_soapFaultCode.Empty();
  m_soapFaultActor.Empty();
  m_soapFaultString.Empty();
  m_soapFaultDetail.Empty();

  // Leave request handle untouched !!
  // Leave access  token  untouched !!
  // Leave reliable       untouched !!
  // Leave encryption     untouched !!
  // Leave cookies        untouched !!
  // Leave server handle  untouched !!
  // Leave routing        untouched !!
}

void
SOAPMessage::SetSoapVersion(SoapVersion p_version)
{
  if(m_soapVersion > SoapVersion::SOAP_10 && p_version == SoapVersion::SOAP_10)
  {
    // Revert back to POS
    m_header      = nullptr;
    m_body        = m_root;
    m_paramObject = m_root;
    CleanNode(m_root);
    m_root->SetName(m_soapAction);
    m_root->SetNamespace("");
  }
  else
  {
    if(m_soapVersion == SoapVersion::SOAP_10 && p_version > SoapVersion::SOAP_10)
    {
      CreateHeaderAndBody();
      CreateParametersObject();
    }
    SetAttribute(m_root,"xmlns:s",p_version == SoapVersion::SOAP_11 ? NAMESPACE_SOAP11 : NAMESPACE_SOAP12);
  }
  // Record the change
  m_soapVersion = p_version;
}

// Add an extra header
void
SOAPMessage::AddHeader(XString p_name,XString p_value)
{
  // Case-insensitive search!
  HeaderMap::iterator it = m_headers.find(p_name);
  if(it != m_headers.end())
  {
    // Check if we set it a duplicate time
    // If appended, we do not append it a second time
    if(it->second.Find(p_value) >= 0)
    {
      return;
    }
    if(p_name.CompareNoCase("Set-Cookie") == 0)
    {
      // Insert as a new header
      m_headers.insert(std::make_pair(p_name,p_value));
      return;
    }
    // New value of the header
    it->second = p_value;
  }
  else
  {
    // Insert as a new header
    m_headers.insert(std::make_pair(p_name,p_value));
  }
}

// Add extra request/response header by well-known ID
void
SOAPMessage::AddHeader(HTTP_HEADER_ID p_id,XString p_value)
{
  extern const char* header_fields[HttpHeaderMaximum];
  int maximum = m_incoming ? HttpHeaderMaximum : HttpHeaderResponseMaximum;

  if(p_id >= 0 && p_id < maximum)
  {
    XString name(header_fields[p_id]);
    AddHeader(name,p_value);
  }
}

void
SOAPMessage::DelHeader(XString p_name)
{
  HeaderMap::iterator it = m_headers.find(p_name);
  if(it != m_headers.end())
  {
    m_headers.erase(it);
  }
}

void
SOAPMessage::DelHeader(HTTP_HEADER_ID p_id)
{
  extern const char* header_fields[HttpHeaderMaximum];
  int maximum = m_incoming ? HttpHeaderMaximum : HttpHeaderResponseMaximum;

  if(p_id >= 0 && p_id < maximum)
  {
    XString name(header_fields[p_id]);
    DelHeader(name);
  }
}

// Finding a header
XString
SOAPMessage::GetHeader(XString p_name)
{
  // Case-insensitive search
  HeaderMap::iterator it = m_headers.find(p_name);
  if(it != m_headers.end())
  {
    return it->second;
  }
  return "";
}

// OPERATORS
SOAPMessage* 
SOAPMessage::operator=(JSONMessage& p_json)
{
  m_request       = p_json.GetRequestHandle();
  m_site          = p_json.GetHTTPSite();
  m_url           = p_json.GetURL();
  m_cracked       = p_json.GetCrackedURL();
  m_user          = p_json.GetUser();
  m_password      = p_json.GetPassword();
  m_contentType   = p_json.GetContentType();
  m_sendBOM       = p_json.GetSendBOM();
  m_incoming      = p_json.GetIncoming();
  m_headers       =*p_json.GetHeaderMap();

  // Duplicate all cookies
  m_cookies = p_json.GetCookies();

  // Get sender (if any) from the HTTP message
  memcpy(&m_sender,p_json.GetSender(),sizeof(SOCKADDR_IN6));
  m_desktop = p_json.GetDesktop();

  // Duplicate the HTTP token for ourselves
  if(DuplicateTokenEx(p_json.GetAccessToken()
                      ,TOKEN_DUPLICATE | TOKEN_IMPERSONATE | TOKEN_ALL_ACCESS | TOKEN_READ | TOKEN_WRITE
                      ,NULL
                      ,SecurityImpersonation
                      ,TokenImpersonation
                      ,&m_token) == FALSE)
  {
    m_token = NULL;
  }

  CreateHeaderAndBody();
  CreateParametersObject();

  // The message itself
  XMLParserJSON(this,&p_json);
  CheckAfterParsing();

  return this;
}

#pragma endregion ReUse

#pragma region File

// Load from file
bool
SOAPMessage::LoadFile(const XString& p_fileName)
{
  if(XMLMessage::LoadFile(p_fileName))
  {
    CheckAfterParsing();
    return true;
  }
  return false;
}

bool
SOAPMessage::LoadFile(const XString& p_fileName,StringEncoding p_encoding)
{
  m_encoding = p_encoding;
  return LoadFile(p_fileName);
}

bool
SOAPMessage::SaveFile(const XString& p_fileName,bool p_withBom /*= false*/)
{
  // Make sure all members are set to XML
  CompleteTheMessage();

  // Whole message encryption is done in the saving function!
  return XMLMessage::SaveFile(p_fileName,p_withBom);
}

bool
SOAPMessage::SaveFile(const XString& p_fileName,StringEncoding p_encoding,bool p_withBom /*= false*/)
{
  m_encoding = p_encoding;
  return SaveFile(p_fileName,p_withBom);
}

#pragma endregion File Loading and Saving

#pragma region GettersSetters

// Addressing relates to reliability
void
SOAPMessage::SetAddressing(bool p_addressing)
{
  m_addressing = p_addressing;
  if(p_addressing == false)
  {
    m_reliable = false;
  }
}

// Reliability relates to addressing
// But only switch, if these pre-conditions are met
void
SOAPMessage::SetReliability(bool p_reliable,bool p_throw /* = true*/)
{
  if(p_reliable)
  {
    if(!m_guidSequenceClient.IsEmpty() &&
       !m_guidSequenceServer.IsEmpty() &&
       !m_messageGuidID     .IsEmpty() &&
        m_clientMessageNumber != 0     &&
        m_addressing          == true  )
    {
      m_reliable = true;
    }
    else if(p_throw)
    {
      XString error("Basic conditions for Reliable-Messaging are not met. Cannot send RM message");
      throw StdException(error);
    }
  }
  else
  {
    m_reliable = false;
  }
}

// Get the content type
XString
SOAPMessage::GetContentType() const
{
  if(m_contentType.IsEmpty())
  {
    switch(m_soapVersion)
    {
      case SoapVersion::SOAP_10: [[fallthrough]];
      case SoapVersion::SOAP_11: return "text/xml";
      case SoapVersion::SOAP_12: return "application/soap+xml";
    }
  }
  // Could be a simple override like "app/xml"
  return m_contentType;
}

void
SOAPMessage::SetAcceptEncoding(XString p_encoding)
{
  m_acceptEncoding = p_encoding;
}

// Addressing the message's has three levels
// 1) The complete url containing both server and port number
// 2) Setting server/port/absolutepath separately
// 3) By remembering the requestID of the caller
void
SOAPMessage::SetURL(XString& p_url)
{
  m_url = p_url;
  m_cracked.CrackURL(p_url);
}

// URL without user/password
XString
SOAPMessage::GetURL() const
{
  return m_url;
}

// Getting the JSON parameterized URL
// ONLY WORKS FOR THE FIRST NODE LEVEL !!
XString
SOAPMessage::GetJSON_URL()
{
  XString url = GetURL();

  // Make sure path ends in a '/'
  if(url.Right(1) != "/")
  {
    url += "/";
  }

  // Add parameters object name
  url += m_paramObject->GetName();

  // Here comes the parameters
  url += "?";

  // Add parameters
  unsigned count = 0;
  XMLElement* param = GetElementFirstChild(m_paramObject);
  while(param)
  {
    // Check for complex objects
    if(!param->GetChildren().empty())
    {
      // NOT POSSIBLE to send as a HTTP 'GET'
      url.Empty();
      break;
    }
    // Do next param in the string
    if(count)
    {
      url += "&";
    }
    // Add the parameter
    url += param->GetName();
    if(!param->GetValue().IsEmpty())
    {
      url += "=";
      url += param->GetValue();
    }
    // Next parameter
    param = GetElementSibling(param);
    ++count;
  }
  return url;
}

XString
SOAPMessage::GetRoute(int p_index)
{
  if(p_index >= 0 && p_index < (int)m_routing.size())
  {
    return m_routing[p_index];
  }
  return "";
}

// TO do after we set parts of the URL in setters
void
SOAPMessage::ReparseURL()
{
  m_url = m_cracked.URL();
}

void
SOAPMessage::SetSender(PSOCKADDR_IN6 p_address)
{
  memcpy(&m_sender,p_address,sizeof(SOCKADDR_IN6));
}

bool
SOAPMessage::SetSigningMethod(unsigned p_method)
{
  // Record only if it yields a valid signing method
  // by the signing standard: "http://www.w3.org/2000/09/xmldsig#" + p_method
  Crypto crypted(p_method);
  if(!crypted.GetHashMethod(p_method).IsEmpty())
  {
    m_signingMethod = p_method;
    return true;
  }
  return false;
}

void 
SOAPMessage::SetCookie(XString p_name
                      ,XString p_value
                      ,XString p_metadata /*= ""*/
                      ,bool    p_secure   /*= false*/)
{
  Cookie monster;
  // Beware: Application logic can never set httpOnly = true!!
  monster.SetCookie(p_name,p_value,p_metadata,p_secure,false);
  m_cookies.AddCookie(monster);
}

XString
SOAPMessage::GetCookie(unsigned p_ind /*= 0*/,XString p_metadata /*= ""*/)
{
  Cookie* monster = m_cookies.GetCookie(p_ind);
  if(monster && monster->GetHttpOnly() == false)
  {
    return monster->GetValue(p_metadata);
  }
  return "";
}

XString
SOAPMessage::GetCookie(XString p_name /*= ""*/,XString p_metadata /*= ""*/)
{
  Cookie* monster = m_cookies.GetCookie(p_name);
  if(monster && monster->GetHttpOnly() == false)
  {
    return monster->GetValue(p_metadata);
  }
  return "";
}

Cookie*
SOAPMessage::GetCookie(unsigned p_ind)
{
  Cookie* monster = m_cookies.GetCookie(p_ind);
  if(monster && monster->GetHttpOnly() == false)
  {
    return monster;
  }
  return nullptr;
}

// Get parameter from incoming message with check
XString 
SOAPMessage::GetParameterMandatory(XString p_paramName)
{
  XMLElement* element = FindElement(p_paramName);
  if(element)
  {
    return element->GetValue();
  }
  XString msg;
  msg.Format("Missing parameter [%s.%s]",m_soapAction.GetString(),p_paramName.GetString());
  throw StdException(msg);
}

// Set/Add parameter to the header section (level 1.1 and 1.2 only!)
XMLElement*  
SOAPMessage::SetHeaderParameter(XString p_name,const char* p_value,bool p_first /*=false*/)
{
  if(!m_header && m_soapVersion > SoapVersion::SOAP_10)
  {
    XString header;
    // Header must be the first child element of Envelope
    if(!m_root->GetNamespace().IsEmpty())
    {
      header = m_root->GetNamespace() + ":";
    }
    header += "Header";
    m_header = SetElement(m_root,header,XDT_String,"",true);
  }
  if(m_header)
  {
    return SetElement(m_header,p_name,XDT_String,p_value,p_first);
  }
  XString error;
  error.Format("Tried to set a header parameter [%s:%s], but no header present (SOAP 1.0)!",p_name.GetString(),p_value);
  throw StdException(error);
}

// General add a parameter (always adds, so multiple parameters of same name can be added)
XMLElement*
SOAPMessage::AddElement(XMLElement* p_base,XString p_name,XmlDataType p_type,XString p_value,bool p_front /*= false*/)
{
  XMLElement* node(p_base);
  if(node == nullptr)
  {
    // By default, this will be the command parameters object
    node = m_paramObject;
    if(node == nullptr)
    {
      // In effect an error, but otherwise information will be list
      // XSD/WSDL will have ot catch this
      node = m_body;
    }
  }
  return XMLMessage::AddElement(node,p_name,p_type,p_value,p_front);
}

// Override for an integer
XMLElement*
SOAPMessage::AddElement(XMLElement* p_base,XString p_name,XmlDataType p_type,int p_value,bool p_front /*=false*/)
{
  XString value;
  value.Format("%d",p_value);
  return AddElement(p_base,p_name,p_type,value,p_front);
}

XMLElement*
SOAPMessage::AddElement(XMLElement* p_base,XString p_name,XmlDataType p_type,unsigned p_value,bool p_front /*=false*/)
{
  XString value;
  value.Format("%u",p_value);
  return AddElement(p_base,p_name,p_type,value,p_front);
}

XMLElement* 
SOAPMessage::AddElement(XMLElement* p_base,XString p_name,XmlDataType p_type,__int64 p_value,bool p_front /*= false*/)
{
  XString value;
  value.Format("%I64d",p_value);
  return AddElement(p_base,p_name,p_type,value,p_front);
}

XMLElement* 
SOAPMessage::AddElement(XMLElement* p_base,XString p_name,XmlDataType p_type,unsigned __int64 p_value,bool p_front /*= false*/)
{
  XString value;
  value.Format("%I64u",p_value);
  return AddElement(p_base,p_name,p_type,value,p_front);
}

XMLElement*
SOAPMessage::AddElement(XMLElement* p_base,XString p_name,XmlDataType p_type,const char* p_value,bool p_front /*=false*/)
{
  XString value(p_value);
  return AddElement(p_base,p_name,p_type,value,p_front);
}

XMLElement*
SOAPMessage::AddElement(XMLElement* p_base,XString p_name,XmlDataType p_type,bool p_value,bool p_front /*=false*/)
{
  XString value = p_value ? "true" : "false";
  return AddElement(p_base,p_name,p_type,value,p_front);
}

XMLElement*
SOAPMessage::AddElement(XMLElement* p_base,XString p_name,XmlDataType p_type,double p_value,bool p_front /*=false*/)
{
  XString value;
  value.Format("16.16%g",p_value);
  value = value.TrimRight('0');
  return AddElement(p_base,p_name,p_type,value,p_front);
}

XMLElement*
SOAPMessage::AddElement(XMLElement* p_base,XString p_name,XmlDataType p_type,bcd p_value,bool p_front /*=false*/)
{
  XString value = p_value.AsString(bcd::Format::Bookkeeping,false,0);
  return AddElement(p_base,p_name,p_type,value,p_front);
}

// Get parameter from the header
XString
SOAPMessage::GetHeaderParameter(XString p_paramName)
{
  if(m_header)
  {
    XMLElement* element = FindElement(m_header,p_paramName);
    if(element)
    {
      return element->GetValue();
    }
    return "";
  }
  XString error;
  error.Format("Tried to get a header parameter [%s], but no header present (yet)!",p_paramName.GetString());
  throw StdException(error);
}

// Get sub parameter from the header
XMLElement*
SOAPMessage::GetHeaderParameterNode(XString p_paramName)
{
  if(m_header)
  {
    return FindElement(m_header,p_paramName);
  }
  return nullptr;
}

XMLElement*
SOAPMessage::GetFirstParameter()
{
  if(m_paramObject)
  {
    return GetElementFirstChild(m_paramObject);
  }
  return nullptr;
}

void
SOAPMessage::SetParameterObject(XString p_name)
{
  if(m_paramObject)
  {
    m_paramObject->SetName(p_name);
  }
}

XString
SOAPMessage::GetParameterObject() const
{
  if(m_paramObject)
  {
    return m_paramObject->GetName();
  }
  return "";
}

XMLElement*
SOAPMessage::SetParameter(XString p_name,XString& p_value)
{
  XMLElement* elem = FindElement(m_paramObject,p_name,false);
  if(elem)
  {
    elem->SetValue(p_value);
    return elem;
  }
  return AddElement(m_paramObject,p_name,XDT_String,p_value);
}

XMLElement*
SOAPMessage::SetParameter(XString p_name,const char* p_value)
{
  XString value(p_value);
  return SetParameter(p_name,value);
}

XMLElement*
SOAPMessage::SetParameter(XString p_name,int p_value)
{
  XString value;
  value.Format("%d",p_value);
  return SetParameter(p_name,value);
}

XMLElement*
SOAPMessage::SetParameter(XString p_name,bool p_value)
{
  XString value = p_value ? "true" : "false";
  return SetParameter(p_name,value);
}

XMLElement*
SOAPMessage::SetParameter(XString p_name,double p_value)
{
  XString value;
  value.Format("%G",p_value);
  return SetParameter(p_name,value);
}

// Specialized URL: e.g. for Accept headers in RM-protocol
XString
SOAPMessage::GetUnAuthorisedURL() const
{
  XString url;
  XString portstr;
  bool secure = m_cracked.m_secure;
  int  port   = m_cracked.m_port;


  if((secure && port!=INTERNET_DEFAULT_HTTPS_PORT)||
    (!secure && port!=INTERNET_DEFAULT_HTTP_PORT))
  {
    portstr.Format(":%d",port);
  }

  url.Format("http%s://%s%s%s"
            ,secure ? "s" : ""
            ,m_cracked.m_host.GetString()
            ,portstr.GetString()
            ,m_cracked.AbsolutePath().GetString());
  return url;
}

// Set the whitespace preserving (instead of CDATA sections)
bool
SOAPMessage::SetPreserveWhitespace(bool p_preserve /*= true*/)
{
  XMLElement* parm = GetParameterObjectNode();
  if(parm)
  {
    SetAttribute(parm,"xml:space",p_preserve ? "preserve" : "default");
    return true;
  }
  return false;
}

#pragma endregion Getting and setting members

#pragma region XMLOutput

// Get resulting soap message before sending it
XString 
SOAPMessage::GetSoapMessage()
{
  // Make sure all members are set to XML
  CompleteTheMessage();

  // Let the XML object print the message
  XString message = XMLMessage::Print();

  // Encryption of the whole-message comes last
  EncryptMessage(message);

  return message;
}

// Complete the message (members to XML)
void
SOAPMessage::CompleteTheMessage()
{
  // Make sure we have the right envelope and header
  // With the correct namespaces in it
  SetSoapEnvelope();
  SetSoapHeader();

  if(!m_soapFaultCode.IsEmpty())
  {
    // Print the soap fault stack
    SetBodyToFault();
  }
  else
  {
    SetSoapBody();

    if(m_soapVersion > SoapVersion::SOAP_11)
    {
      if(m_encryption == XMLEncryption::XENC_Signing)
      {
        AddToHeaderSigning();
      }
      else if(m_encryption == XMLEncryption::XENC_Body)
      {
        EncryptBody();
      }
    }
  }
}

XString
SOAPMessage::GetSoapMessageWithBOM()
{
  if(m_sendBOM && m_encoding == StringEncoding::ENC_UTF8)
  {
    XString msg = ConstructBOM();
    msg += GetSoapMessage();
    return msg;
  }
  return GetSoapMessage();
}

XString 
SOAPMessage::GetJsonMessage(bool p_full /*=false*/,bool p_attributes /*=false*/)
{
  // Printing the body of the message
  if(m_soapVersion > SoapVersion::SOAP_10)
  {
    // Make sure all members are set to XML
    CompleteTheMessage();
  }
  // Let the XML object print the message
  // Full message encryption is NOT supported
  if(p_full)
  {
    return XMLMessage::PrintJson(p_attributes);
  }
  return XMLMessage::PrintElementsJson(m_paramObject,p_attributes);
}

XString 
SOAPMessage::GetJsonMessageWithBOM(bool p_full /*=false*/,bool p_attributes /*=false*/)
{
  if(m_sendBOM && m_encoding == StringEncoding::ENC_UTF8)
  {
    XString msg = ConstructBOM();
    msg += GetJsonMessage(p_full,p_attributes);
    return msg;
  }
  return GetJsonMessage(p_full,p_attributes);
}

// Add correct SOAP 1.1/1.2 envelope
// Use Microsoft defaults as used in WCF for WS-Reliability
void 
SOAPMessage::SetSoapEnvelope()
{
  if(m_soapVersion == SoapVersion::SOAP_10)
  {
    // Plain old soap: No envelope
    return;
  }

  // If no "Envelope" node, fail silently
  if(m_root->GetName().Compare("Envelope"))
  {
    return;
  }

  // Do not meddle with incoming soap messages
  if(m_incoming)
  {
    return;
  }

  // Namespaces for all
  SetElementNamespace(m_root,"i",  NAMESPACE_INSTANCE);
  SetElementNamespace(m_root,"xsd",NAMESPACE_XMLSCHEMA);

  if(m_addressing || m_reliable || m_soapVersion > SoapVersion::SOAP_11)
  {
    SetElementNamespace(m_root,"a",NAMESPACE_WSADDRESS);
  }
  if(m_reliable)
  {
    // Special soap namespace + addressing and RM (reliable messaging)
    SetElementNamespace(m_root,"s", NAMESPACE_ENVELOPE);
    SetElementNamespace(m_root,"rm",NAMESPACE_RELIABLE);
  }
  else
  {
    // Very general soap namespace
    switch (m_soapVersion)
    {
      case SoapVersion::SOAP_11: SetElementNamespace(m_root,"s",NAMESPACE_SOAP11,true);
                                 break;
      case SoapVersion::SOAP_12: SetElementNamespace(m_root,"s",NAMESPACE_SOAP12);
                                 break;
    }
  }

  // Signing and encryption
  // Add WS-Message-Security namespaces
  if(m_encryption != XMLEncryption::XENC_Plain)
  {
    SetElementNamespace(m_root,"ds",  NAMESPACE_SIGNATURE);
    SetElementNamespace(m_root,"xenc",NAMESPACE_ENCODING);
    SetElementNamespace(m_root,"wsse",NAMESPACE_SECEXT);
    SetElementNamespace(m_root,"wsu", NAMESPACE_SECUTILITY);
  }
}

// Clean up the empty elements in the message
bool
SOAPMessage::CleanUp()
{
  bool result = false;

  if(m_body && m_paramObject)
  {
    result = true;
    for(int num = (int)m_paramObject->GetChildren().size() - 1; num >= 0; --num)
    {
      if(!CleanUpElement(m_paramObject->GetChildren()[num],true))
      {
        result = false;
      }
    }
  }
  return result;
}

#pragma endregion XMLOutput

#pragma region WS-Header_Output

// Add correct soap header for WS-Reliability
void
SOAPMessage::SetSoapHeader()
{
  // Do not meddle with incoming soap messages
  // Or with Plain-Old-Soap messages
  if(m_incoming || m_soapVersion == SoapVersion::SOAP_10)
  {
    return;
  }

  // Addressing part
  if(m_addressing || m_reliable || (m_soapVersion == SoapVersion::SOAP_12 && m_initialAction))
  {
    // SoapAction in message instead of HTTP header
    // namespace / service / command
    AddToHeaderAction();
  }
  if(m_addressing)
  {
    /// Add message ID for this message, and store it for the return answer
    AddToHeaderMessageID();

    // Reply to (Expand later?)
    AddToHeaderReplyTo();

    // Must understand our URL/service (url:port/service)
    AddToHeaderToService();
  }

  // Reliability part
  if(m_reliable)
  {
    // Send acknowledgment of server messages so far
    AddToHeaderAcknowledgement();

    // Add message number
    AddToHeaderMessageNumber();
  }
}

void
SOAPMessage::SetSoapBody()
{
  // Do not meddle with incoming soap messages
  if(m_incoming)
  {
    return;
  }

  if(m_soapVersion >= SoapVersion::SOAP_11) 
  {
    if((m_body != m_root) && m_paramObject && (m_paramObject != m_body) && !m_namespace.IsEmpty())
    {
      XMLAttribute* xmlns(nullptr);
      xmlns = FindAttribute(m_paramObject,"xmlns:tns");
      if(xmlns)
      {
        xmlns->m_value = m_namespace;
      }
      else
      {
        xmlns = FindAttribute(m_paramObject,"xmlns");
        if(xmlns)
        {
          xmlns->m_value = m_namespace;
        }
        else
        {
          SetAttribute(m_paramObject,"xmlns",m_namespace);
        }
      }
    }
  }
}

// Usage of the "mustUnderstand" attribute in the <Envelope><Header><Action> node
// These are the possibilities
//
// mustUnderstand    Intermediate role   End server role
// ---------------   -----------------   -------------------------------
// present & true    Must process        Must process
// present & false   May  process        May  process
// absent            May  process        May  process
//
// DON'T LET ME BE MISUNDERSTOOD (free after The Animals & Santa Esmeralda :-)
// Also called the Jan-Jaap Fahner option (We must understand nothing!!)
void
SOAPMessage::SetSoapMustBeUnderstood(bool p_addAttribute /*=true*/,bool p_understand /*=true*/)
{
  m_addAttribute = p_addAttribute;  // Add the attribute to <Action> node
  m_understand   = p_understand;    // Value of the "mustUnderstand" attribute
}

//////////////////////////////////////////////////////////////////////////
//
// Detail functions of AddSoapHeader
// 
//////////////////////////////////////////////////////////////////////////

void
SOAPMessage::AddToHeaderAcknowledgement()
{
  if(m_serverMessageNumber)
  {
    if(FindElement(m_header,"rm:SequenceAcknowledgement") == NULL)
    {
      if(m_ranges.size())
      {
        XMLElement* ack = SetHeaderParameter("rm:SequenceAcknowledgement","");
        SetElement(ack,"rm:Identifier",m_guidSequenceClient);
        for(unsigned int ind = 0;ind < m_ranges.size(); ++ind)
        {
          XMLElement* rng = SetElement(ack,"rm:AcknowledgementRange","");
          SetAttribute(rng,"Lower",m_ranges[ind].m_lower);
          SetAttribute(rng,"Upper",m_ranges[ind].m_upper);
        }
      }
      else
      {
        XMLElement* ack = SetHeaderParameter("rm:SequenceAcknowledgement","");
                          SetElement(ack,"rm:Identifier",m_guidSequenceClient);
        XMLElement* rng = SetElement(ack,"rm:AcknowledgementRange","");
        SetAttribute(rng,"Lower",1);
        SetAttribute(rng,"Upper",m_serverMessageNumber);
      }
    }
  }
}

void
SOAPMessage::AddToHeaderMessageNumber()
{
  if(m_clientMessageNumber && FindElement(m_header,"rm:Sequence") == NULL)
  {
    XMLElement* seq = SetHeaderParameter("rm:Sequence","");
    SetAttribute(seq,"s:mustUnderstand",1);
    SetElement  (seq,"Identifier",   m_guidSequenceServer);
    SetElement  (seq,"MessageNumber",m_clientMessageNumber);
    if(m_lastMessage)
    {
      SetElement(seq,"rm:LastMessage","");
    }
  }
}

void
SOAPMessage::AddToHeaderMessageID()
{
  if(FindElement(m_header,"a:MessageID") == NULL)
  {
    if(m_messageGuidID.IsEmpty())
    {
      // Generate a message GuidID in case of addressing only
      // Do this in the Microsoft WCF fashion as an URN unique ID
      m_messageGuidID = "urn:uuid:" + GenerateGUID();
    }
    SetHeaderParameter("a:MessageID",m_messageGuidID);
  }
}

void
SOAPMessage::AddToHeaderReplyTo()
{
  if(FindElement(m_header,"a:ReplyTo") == NULL)
  {
    XString addressing(NAMESPACE_WSADDRESS);
    addressing += "/anonymous";
    XMLElement* param = SetHeaderParameter("a:ReplyTo","");
    SetElement(param,"a:Address",addressing);
  }
}

void
SOAPMessage::AddToHeaderToService()
{
  if(FindElement(m_header,"a:To") == NULL)
  {
    XMLElement* param = SetHeaderParameter("a:To",GetURL());
    SetAttribute(param,"s:mustUnderstand",1);
  }
}

void
SOAPMessage::AddToHeaderAction()
{
  XString action = CreateSoapAction(m_namespace, m_soapAction);
  XMLElement* actParam = FindElement(m_header,"Action");
  if(actParam == nullptr)
  {
    // Must come as the first element of the header
    actParam = SetHeaderParameter("a:Action",action,true);
  }
  else
  {
    actParam->SetValue(action);
  }
  // Make sure other SOAP Roles (proxy, ESB) parses the action header part or not
  if(actParam && m_addAttribute)
  {
    SetAttribute(actParam,"s:mustUnderstand",m_understand);
  }
}

void
SOAPMessage::AddToHeaderSigning()
{
  XMLElement* secure = SetHeaderParameter("wsse:Security","");

  // Add optional username-token/username
  if(!m_enc_user.IsEmpty())
  {
    XMLElement* usrToken = SetElement(secure,"wsse:UsernameToken",XDT_String,"");
    SetElement(usrToken,"wsse:Username",XDT_String,m_enc_user);
  }

  // Create a password token
  XString token = GetPasswordAsToken();

  // Default namespace settings
  XMLElement* custTok = SetElement(secure,"sym:CustomToken",token);
  SetAttribute(custTok,"wsu:Id","MyToken");
  SetElementNamespace(custTok,"sym",DEFAULT_NAMESPACE);


  // General signature + info
  XMLElement* signat = SetElement(secure,"ds:Signature","");
  XMLElement* sigInf = SetElement(signat,"ds:SignedInfo","");

  // Referring the signing to the body element
  XMLElement* refer  = SetElement(sigInf,"ds:Reference","");

  // Setting the signing
  XMLElement* digMet = SetElement(refer,  "ds:DigestMethod","");
  XMLElement* digVal = FindElement(refer, "ds:DigestValue");
  XMLElement* sigVal = FindElement(signat,"ds:SignatureValue");

  // Only of not previously signed
  if(!sigVal || sigVal->GetValue().IsEmpty())
  {
    // Set body ID
    SetAttribute(refer, "URI",  "#MsgBody");
    SetAttribute(m_body,"wsu:Id","MsgBody");

    // Body extra attribute set and signing is empty
    // now calculate the signing
    XString sign = SignBody();

    Crypto crypto(m_signingMethod);
    XString method = NAMESPACE_SIGNATURE;
    method += crypto.GetHashMethod(m_signingMethod);
    SetAttribute(digMet,"Algorithm",method);

    // setting the signing
    if(digVal)
    {
      SetElementValue(digVal,XDT_String,sign);
    }
    else
    {
      SetElement(refer,"ds:DigestValue",sign);
    }
    if(sigVal)
    {
      SetElementValue(sigVal, XDT_String, sign);
    }
    else
    {
      SetElement(signat,"ds:SignatureValue",sign);
    }
  }
  // KeyInfo reference to the Symphonies token
  XMLElement* keyInf = SetElement(signat,"ds:KeyInfo","");
  XMLElement* secRef = SetElement(keyInf,"wsse:SecurityTokenReference","");
  XMLElement* krefer = SetElement(secRef,"wsse:Reference","");
  SetAttribute(krefer,"URI","#MyToken");
}

#pragma endregion WS-Header_Output

#pragma region XMLIncoming

// Parse incoming message to members
void
SOAPMessage::ParseMessage(XString& p_message)
{
  // Clean out everything we have
  CleanNode(m_root);     // Structure
  m_root->SetName("");     // Envelope name if any

  // Do the 'real' XML parsing
  XMLMessage::ParseMessage(p_message);

  // Balance internal structures
  CheckAfterParsing();
}

// Parse incoming soap as new body of the message
// Ignore the fact that the underlying XMLMessage could
// be prepared for a SOAP 1.2 header/body structure
// or a simple SOAP 1.0 structure
void
SOAPMessage::ParseAsBody(XString& p_message)
{
  SoapVersion oldVersion = m_soapVersion;
  CleanNode(m_body);
  CreateHeaderAndBody();

  XMLElement* node = m_body;
  if(m_body == m_root)
  {
    XMLMessage::Reset();
    node = nullptr;
  }

  // Do the 'real' XML parsing
  XMLMessage::ParseForNode(node,p_message);

  // Balance internal structures
  CheckAfterParsing();

  if(oldVersion > SoapVersion::SOAP_10)
  {
    // Reconstruct the SOAP Action in the header
    m_soapVersion = oldVersion;
    SetSoapHeader();
  }
}

// Parse incoming GET url to SOAP parameters
void    
SOAPMessage::Url2SoapParameters(const CrackedURL& p_url)
{
  CreateHeaderAndBody();
  CreateParametersObject();

  WinFile part(p_url.m_path);
  XString action = part.GetFilenamePartFilename();

  SetSoapAction(action);
  SetParameterObject(action);

  for(unsigned num = 0; num < p_url.GetParameterCount(); ++num)
  {
    const UriParam* param = p_url.GetParameter(num);
    SetParameter(param->m_key,param->m_value);
  }
}

// Set internal structures after XML parsing
void
SOAPMessage::CheckAfterParsing()
{
  // Find header/body/paramObject again
  FindHeaderAndBody();

  if(GetInternalError() != XmlError::XE_NoError)
  {
    return;
  }

  if(m_soapVersion == SoapVersion::SOAP_POS)
  {
    // Plain-Old-Soap
    m_soapAction = m_paramObject ? m_paramObject->GetName() : XString();
    if(m_paramObject) // Could be <Envelope> or <Body>
    {
      XString namesp = GetAttribute(m_paramObject,"xmlns");
      if(!namesp.IsEmpty())
      {
        m_namespace = namesp;
      }
    }
  }
  else
  {
    // get the name of first element within body/root
    if(m_paramObject && m_paramObject != m_body && m_paramObject != m_root)
    {
      m_soapAction = m_paramObject ? m_paramObject->GetName() : XString();
      // SOAP namespace override (leave HTTP header SOAPAction intact)
      XString namesp = GetAttribute(m_paramObject, "xmlns");
      if (namesp.IsEmpty())
      {
        XMLAttribute* targetns = FindAttribute(m_paramObject, "tns");
        if (targetns && targetns->m_namespace.Compare("xmlns") == 0)
        {
          namesp = targetns->m_value;
        }
      }
      if (!namesp.IsEmpty())
      {
        SetNamespace(namesp);
      }
    }
  }
  // OPTIONAL ENCRYPTION CHECK

  // Check Encrypted body
  if(m_soapAction == "EncryptionData")
  {
    // Record the encryption in the password field
    m_enc_password = CheckBodyEncryption();
    if(!m_enc_password.IsEmpty())
    {
      // Break off. Parsing has no further use.
      return;
    }
    m_enc_password = CheckMesgEncryption();
    if(!m_enc_password.IsEmpty())
    {
      // Break off. Parsing has no further use.
      return;
    }
  }

    // If also a header present, check it after the body
    // to make sure we have our message namespace
  if(m_header)
  {
    // And check the addressing/reliability fields
    CheckHeader();

    // Check for body signing and authentication
    CheckBodySigning();
  }
  // See if there exists a <Fault> node
  // SoapVersion already known
  XMLElement* fault = FindElement("Fault");
  if(fault)
  {
    HandleSoapFault(fault);
  }
}

// Find Header and Body after a parsed message is coming in
void
SOAPMessage::FindHeaderAndBody()
{
  // Check if root = 'Envelope'
  if(m_root->GetName().Compare("Envelope"))
  {
    // Not an Envelope, must be POS (Plain-Old-Soap)
    m_soapVersion = SoapVersion::SOAP_10;
    m_header      = nullptr;
    m_body        = m_root;
    m_paramObject = m_body;
  }
  else
  {
    m_soapVersion = SoapVersion::SOAP_11;
    m_header = FindElement(m_root,"Header");
    m_body   = FindElement(m_root,"Body");

    // Header can be missing, but we need a body now!
    if(m_body == nullptr)
    {
      XMLElement* first = GetElementFirstChild(m_root);
      if(first && first->GetName().Compare("EncryptionData") == 0)
      {
        // Not a body, but an encoded message. Which is OK
        m_encryption  = XMLEncryption::XENC_Message;
        m_paramObject = first;
        return;
      }
      // An envelope but NO body. ERROR!
      m_internalError = XmlError::XE_NoBody;
      return;
    }
    // Check for namespace in the header
    XString nmsp = GetAttribute(m_root,m_root->GetNamespace());
    if(CompareNamespaces(nmsp,NAMESPACE_SOAP12) == 0)
    {
      m_soapVersion = SoapVersion::SOAP_12;
      // Check for a SOAP 'Action' in the header
      if(m_header)
      {
        XMLElement* action = FindElement(m_header, "Action");
        if(action)
        {
          // If found, it's an override of the parameter element of the body
          XString namesp,soapAction;
          if(SplitNamespaceAndAction(action->GetValue(),namesp,soapAction))
          {
            SetNamespace (namesp);
            SetSoapAction(soapAction);
          }
        }
      }
    }
    // Parameter object is now the first child within the body
    m_paramObject = GetElementFirstChild(m_body);
  }
}

// Create header and body accordingly to SOAP version
void
SOAPMessage::CreateHeaderAndBody()
{
  if(m_soapVersion == SoapVersion::SOAP_10)
  {
    // Nothing to do: POS = Plain Old Soap
    return;
  }
  if(m_root->GetChildren().size() == 0)
  {
    m_root->SetNamespace("s");
    m_root->SetName("Envelope");
    m_header = SetElement(m_root,"s:Header","");
    m_body   = SetElement(m_root,"s:Body",  "");
  }
}

// Create the parameters object
void
SOAPMessage::CreateParametersObject(ResponseType p_responseType)
{
  if(m_body)
  {
    m_paramObject = GetElementFirstChild(m_body);
    if(m_paramObject == nullptr)
    {
      if(m_soapVersion == SoapVersion::SOAP_10)
      {
        m_paramObject = m_root;
      }
      else
      {
        // When soapVersion = 1.1 or 1.2
        switch(p_responseType)
        {
	        case ResponseType::RESP_ACTION_NAME: m_paramObject = SetElement(m_body,m_soapAction,"");
	                                             break;
	        case ResponseType::RESP_EMPTY_BODY:  m_paramObject = m_body;
	                                             break;
	        case ResponseType::RESP_ACTION_RESP: m_paramObject = SetElement(m_body,m_soapAction + "Response","");
	                                             break;
        }
      }
    }
  }
}

#pragma endregion XMLIncoming

#pragma region WS-Header_Incoming

// Checking the incoming header for WS-Reliability
void
SOAPMessage::CheckHeader()
{
  // 1: CHECK IF <r:Sequence> HAS THE CORRECT MessageNumber
  CheckHeaderHasSequence();

  // 2: CHECK IF <rm:SequenceAcknowledgement> HAS A COMPLETE SERIES
  CheckHeaderHasAcknowledgement();

  // 3: CHECK IF <a:RelatesTo> IS FOR OUR MESSSAGE
  CheckHeaderRelatesTo();

  // 4: CHECK IF ServiceProtocol IN <a:Action> IS KNOWN BY US
  // But only if the result was correct and no SOAP Fault is found
  CheckHeaderAction();

  // Found something?
  if(m_serverMessageNumber && !m_guidSequenceClient.IsEmpty())
  {
    m_reliable = true;
  }
}

void
SOAPMessage::CheckHeaderHasSequence()
{
  XMLElement* sequence = FindElement(m_header,"Sequence");
  if(sequence == NULL) 
  {
    return;
  }
  XMLElement* identifier = FindElement(sequence,"Identifier");
  if(identifier)
  {
    m_guidSequenceClient = identifier->GetValue();
  }
  XMLElement* messageNumber = FindElement(sequence,"MessageNumber");
  m_serverMessageNumber = atoi(messageNumber->GetValue());

  // Find last message
  XMLElement* lastMessage = FindElement(sequence,"LastMessage");
  m_lastMessage = (lastMessage != NULL);
}

void
SOAPMessage::CheckHeaderHasAcknowledgement()
{
  XMLElement* acknow = FindElement(m_header,"SequenceAcknowledgement");
  if(acknow == NULL)
  {
    return;
  }
  XMLElement* identifier = FindElement(acknow,"Identifier");
  m_guidSequenceServer = identifier->GetValue();

  // Find ranges from server
  XString tag;
  XMLElement* range = FindElement(acknow,"AcknowledgementRange");
  if(range) do 
  {
    RelRange relrange = { 0, 0};
    relrange.m_lower = GetAttributeInteger(range,"Lower");
    relrange.m_upper = GetAttributeInteger(range,"Upper");

    // Range incomplete -> Retransmit
    m_ranges.push_back(relrange);
    // CheckMessageRange(lower,upper)
    // Next sibling of AckRange.
    // Skip past Microsoft extensions ("netrm" buffering)
    do 
    {
      range = GetElementSibling(range);
      if(range)
      {
        tag = range->GetName();
      }
    } 
    while(range && tag.Compare("AcknowledgementRange"));
  } 
  while (range);
}

void
SOAPMessage::CheckHeaderRelatesTo()
{
  XMLElement* relates = FindElement(m_header,"RelatesTo");
  if(relates == NULL)
  {
    return;
  }
  m_messageGuidID = relates->GetValue();
}

void
SOAPMessage::CheckHeaderAction()
{
  bool must = false;
  XString expectedResponse = CreateSoapAction(m_namespace,m_soapAction);

  XMLElement* action = FindElement(m_header,"Action");
  if(action == NULL)
  {
    return;
  }
  // OK, We have an action service protocol
  m_initialAction = true;
  must = GetAttributeBoolean(action,"mustUnderstand");

  // Record the mustUnderstand for an incoming message
  if(m_incoming)
  {
    // Must understand can be defined or absent
    // The absent 'mustUnderstand' leads to 'false'
    m_understand = must;
  }

  // Try to 'understand' the SOAP action
  // otherwise, proceed with fingers crossed
  if(must)
  {
    XString error;
    XString response = action->GetValue();
    if(expectedResponse.CompareNoCase(response) == 0)
    {
      // Promote to SOAP 1.2
      m_soapVersion = SoapVersion::SOAP_12;
    }
    else if(response.IsEmpty())
    {
        error.Format("Answer on webservice [%s/%s] with empty response protocol."
                     ,m_namespace.GetString()
                     ,m_soapAction.GetString());
        m_errorstate      = true;
        m_soapFaultCode   = "Soap-Envelope";
        m_soapFaultActor  = "Message";
        m_soapFaultString = error;
        m_internalError   = XmlError::XE_UnknownProtocol;
    }
    else
    {
      // Not what we expected. See if it is a fault
      int pos = response.ReverseFind('/');
      XString fault = response.Mid(pos + 1);
      if(fault.CompareNoCase("fault") == 0)
      {
        // Some sort of a SOAP Fault response is ok
        return;
      }
      XString acknowledge;
      acknowledge.Format("%s/SequenceAcknowledgement",NAMESPACE_RELIABLE);
      if(response.CompareNoCase(acknowledge) == 0)
      {
        // Sequence ack from RM is OK
        m_reliable = true;
        return;
      }
      // See if we have an explicit answer from our namespace
      if(response.Find(m_namespace) != 0)
      {
        // Unknown namespace
        error.Format("Answer on webservice [%s/%s] with unknown response protocol: %s\n"
                     ,m_namespace.GetString()
                     ,m_soapAction.GetString()
                     ,response.GetString());
        m_errorstate      = true;
        m_soapFaultCode   = "WS-Addressing";
        m_soapFaultActor  = "Message";
        m_soapFaultString = error;
        m_internalError   = XmlError::XE_UnknownProtocol;
      }
      else
      {
        // soapAction is now the WS-Addressing action
        SplitNamespaceAndAction(response,m_namespace,m_soapAction);
      }
    }
  }
  // Check for further WS-Addressing above only <Action>
  XMLElement* messID = FindElement(m_header,"MessageID");
  XMLElement* replTO = FindElement(m_header,"ReplyTo");
  XMLElement* parmTO = FindElement(m_header,"To");
  if(messID || replTO || parmTO)
  {
    m_addressing = true;
    if(messID)
    {
      m_messageGuidID = messID->GetValue();
    }
  }
}

#pragma endregion WS-Header_Incoming

#pragma region SoapFault

// Find Fault nodes in the message at the parse state
void
SOAPMessage::HandleSoapFault(XMLElement* p_fault)
{
  m_soapFaultCode  .Empty();
  m_soapFaultActor .Empty();
  m_soapFaultString.Empty();
  m_soapFaultDetail.Empty();

  // Search for soap Version 1.0/1.1 type of faultcode
  XMLElement* fcode = FindElement(p_fault,"faultcode");
  if(fcode)
  {
    XMLElement* actor  = FindElement(p_fault,"faultactor");
    XMLElement* fmess  = FindElement(p_fault,"faultstring");
    XMLElement* detail = FindElement(p_fault,"detail");

    m_soapFaultCode   =          fcode ->GetValue();
    m_soapFaultActor  = actor  ? actor ->GetValue() : XString();
    m_soapFaultString = fmess  ? fmess ->GetValue() : XString();
    m_soapFaultDetail = detail ? detail->GetValue() : XString();
  }
  else
  {
    // Soap Version 1.2 Fault expected
    XMLElement* code    = FindElement(p_fault,"Code");
    XMLElement* reason  = FindElement(p_fault,"Reason");
    XMLElement* detail  = FindElement(p_fault,"Detail");

    XMLElement* value1  = NULL;
    XMLElement* value2  = NULL;
    XMLElement* text    = NULL;

    if(code)
    {
      XMLElement* subcode = NULL;

      value1  = FindElement(code,"Value");
      subcode = FindElement(code,"Subcode");
      if(subcode)
      {
        value2 = FindElement(subcode,"Value");
      }
    }
    if(reason)
    {
      text = FindElement(reason,"Text");
    }
    m_soapFaultCode   = value1 ? value1->GetValue() : XString();
    m_soapFaultActor  = value2 ? value2->GetValue() : XString();
    m_soapFaultString = text   ? text  ->GetValue() : XString();
    m_soapFaultDetail = detail ? detail->GetValue() : XString();
  }
}

void
SOAPMessage::SetBodyToFault()
{
  if(FindElement(m_body,"Fault"))
  {
    // Presume that the Fault is already filled in
    return;
  }

  // Remove original body sofar
  CleanNode(m_body);

  // Opening of a fault
  XMLElement* fault = SetElement(m_body,"Fault","");
  CreateParametersObject();

  if(m_soapVersion < SoapVersion::SOAP_12)
  {
    SetElement(fault,"faultcode",  m_soapFaultCode);
    SetElement(fault,"faultactor", m_soapFaultActor);
    SetElement(fault,"faultstring",m_soapFaultString);
    SetElement(fault,"detail",     m_soapFaultDetail);
  }
  else
  {
    XMLElement* code = SetElement(fault,"Code","");

    // Subcodes
    SetElement(code,"Value",m_soapFaultCode);
    XMLElement* subcode = SetElement(code,"Subcode","");
    SetElement(subcode,"Value",m_soapFaultActor);

    XMLElement* reason = SetElement(fault,"Reason","");
    SetElement(reason,"Text",m_soapFaultString);
    // Voeg detail toe
    SetElement(fault,"Detail",m_soapFaultDetail);
  }
}

// Get SOAP Fault as a total string
XString 
SOAPMessage::GetFault() 
{
  XString soapFault;
  soapFault.Format("Soap message '%s' contains an error:\n\n"
                   "Code  : %s\n"
                   "Actor : %s\n"
                   "String: %s\n"
                   "Detail: %s\n"
                   ,GetSoapAction().GetString()
                   ,GetFaultCode().GetString()
                   ,GetFaultActor().GetString()
                   ,GetFaultString().GetString()
                   ,GetFaultDetail().GetString());
  return soapFault;
}

// Clean the body to put something different in it
// The SOAP Fault or the body encryption
void
SOAPMessage::CleanNode(XMLElement* p_element)
{
  if(!p_element)
  {
    return;
  }
  XMLElement* child = nullptr;
  do
  {
    child = GetElementFirstChild(p_element);
    if(child)
    {
      DeleteElement(p_element,child);
    }
  }
  while(child);
}

#pragma endregion SoapFault

#pragma region ServiceRoutines

// Remove namespace from a node name
XString
SOAPMessage::StripNamespace(const char* p_naam)
{
  char* pointer = (char *) strchr(p_naam,':');
  if(pointer)
  {
    return (const char*)(++pointer);
  }
  return p_naam;
}

// Return only the namespace of a node
XString
SOAPMessage::FindNamespace(const char* p_naam)
{
  XString naam(p_naam);
  int pos = naam.Find(':');
  if(pos > 0)
  {
    return naam.Left(pos);
  }
  return naam;
}

#pragma endregion ServiceRoutines

#pragma region SignEncrypt

// Calculate a signing for the body to be put in the envelope header
XString
SOAPMessage::SignBody()
{
  Crypto md5(m_signingMethod);
  XString total = GetBodyPart();
  XString sign = md5.Digest(total,m_enc_password);

  if(!md5.GetError().IsEmpty())
  {
    sign = md5.GetError();
  }
  return sign;
}

XString 
SOAPMessage::GetBodyPart()
{
  bool utf8 = m_encoding == StringEncoding::ENC_UTF8;
  if(m_body == nullptr)
  {
    FindHeaderAndBody();
  }
  return PrintElements(m_body, utf8);
}

XString
SOAPMessage::GetCanonicalForm(XMLElement* p_element)
{
  bool utf8 = m_encoding == StringEncoding::ENC_UTF8;
  return PrintElements(p_element,utf8);
}

// Encrypt the node of the message
void
SOAPMessage::EncryptNode(XString& p_node)
{
  // Encrypt
  Crypto crypted(m_signingMethod);
  p_node = crypted.Encryption(p_node,m_enc_password);
}

// Encrypt the body: yielding a new body
void
SOAPMessage::EncryptBody()
{
  // If we find a SOAP Fault, return it unencrypted
  if(!m_soapFaultCode  .IsEmpty() ||
     !m_soapFaultActor .IsEmpty() ||
     !m_soapFaultDetail.IsEmpty() ||
     !m_soapFaultString.IsEmpty() )
  {
    return;
  }
  // See if already encrypted. If so, don't do it twice
  XMLElement* crypt = FindElement(m_body,"xenc:EncryptionData",false);
  if(crypt)
  {
    return;
  }

  // Printing the body 
  bool utf8 = (m_encoding == StringEncoding::ENC_UTF8);
  XString bodyString = PrintElements(m_body,utf8);

  // Encrypt the body
  EncryptNode(bodyString);

  // Remove original data from the body
  CleanNode(m_body);

  // Create password token
  XString token = GetPasswordAsToken();

  // Construct the new body
  XMLElement* encrypt = SetElement(m_body, "xenc:EncryptionData","");
  XMLElement* custTok = SetElement(encrypt,"sym:CustomToken",    token);
  XMLElement* keyInfo = SetElement(encrypt,"ds:KeyInfo",         "");
  XMLElement* cdata   = SetElement(encrypt,"ds:CypherData",      "");
                        SetElement(cdata,  "ds:CypherValue",     bodyString);

  // Our default namespace settings
  SetAttribute(custTok,"wsu:Id","MyToken");
  SetElementNamespace(custTok,"sym",DEFAULT_NAMESPACE);

  // KeyInfo reference to the default namespace token
  XMLElement* secRef = SetElement(keyInfo,"wsse:SecurityTokenReference","");
  XMLElement* refer  = SetElement(secRef, "wsse:Reference","");
  SetAttribute(refer,"URI","#MyToken");

  // Re-create the parameters object as 'EncryptionData'
  CreateParametersObject();
}

// Encrypt the whole message: yielding a new message
void
SOAPMessage::EncryptMessage(XString& p_message)
{
  // Only if we should encrypt it
  if(m_encryption != XMLEncryption::XENC_Message)
  {
    return;
  }

  // If we have a SOAP fault, return unencrypted
  if(!m_soapFaultCode  .IsEmpty() ||
     !m_soapFaultActor .IsEmpty() ||
     !m_soapFaultDetail.IsEmpty() ||
     !m_soapFaultString.IsEmpty() )
  {
    return;
  }
  // Encrypt the whole message
  EncryptNode(p_message);

  // Create password token
  XString token = GetPasswordAsToken();

  p_message = "<Envelope>\n"
              "<xenc:EncryptionData>\n"

              // Our token
              "  <sym:CustomToken wsu:Id=\"MyToken\" xmlns:sym=\"" DEFAULT_NAMESPACE  "\">" + token + "</sym:CustomToken>\n"

              // Key Info
              "  <ds:KeyInfo>\n"
              "    <wsse:SecurityTokenReference>\n"
              "      <wsse:Reference URI=\"#MyToken\" />\n"
              "    </wsse:SecurityTokenReference>\n"
              "  </ds:KeyInfo>\n"
              // The real message (coded data)
              "  <ds:CypherData>\n"
              "    <ds:CypherValue>" + p_message + "</ds:CypherValue>\n"
              "  </ds:CypherData>\n"
              "</xenc:EncryptionData>\n"
              "</Envelope>\n";
}

// Get body signing and authentication from the security header
void
SOAPMessage::CheckBodySigning()
{
  XMLElement* secur = FindElement(m_header,"Security",false);
  if(secur)
  {
    XMLElement* usertok = FindElement(secur,"UsernameToken",false);
    if(usertok)
    {
      CheckUsernameToken(usertok);
    }
    XMLElement* sign = FindElement(secur,"Signature",false);
    if(sign)
    {
      XMLElement* sval = FindElement(sign,"SignatureValue",false);
      if(sval)
      {
        m_encryption = XMLEncryption::XENC_Signing;
        m_enc_password = sval->GetValue();
      }
    }
  }
}

// Get body encryption value from body
XString
SOAPMessage::CheckBodyEncryption()
{
  XString encryption;

  // Check explicitly that we have a body,
  // so we distinguish from whole-message encryption
  XMLElement* body = FindElement("Body");
  if(body)
  {
    XMLElement* cypher = FindElement(body,"CypherData");
    if(cypher)
    {
      XMLElement* val = FindElement(cypher,"CypherValue");
      if(val)
      {
        m_encryption = XMLEncryption::XENC_Body;
        encryption = val->GetValue();
      }
    }
  }
  return encryption;
}

// Get whole message encryption value
XString
SOAPMessage::CheckMesgEncryption()
{
  XString encryption;

  XMLElement* cypher = FindElement(m_paramObject,"CypherData");
  if(cypher)
  {
    XMLElement* val = FindElement(cypher,"CypherValue");
    if(val)
    {
      m_encryption = XMLEncryption::XENC_Message;
      encryption = val->GetValue();
    }
  }
  return encryption;
}

// Get password as a custom token
XString
SOAPMessage::GetPasswordAsToken()
{
  XString token;

  // Create password token
  XString revPassword(m_enc_password);
  revPassword.MakeReverse();
  XString encode = revPassword + DEFAULT_TOKEN + m_enc_password;

  // Create binary token
  Base64 base;
  int len = (int) base.B64_length(encode.GetLength());
  char* tokPointer = token.GetBufferSetLength(len);
  base.Encrypt((const unsigned char*) encode.GetString(),encode.GetLength(),(unsigned char*)tokPointer);
  token.ReleaseBufferSetLength(len);

  return token;
}

// Get incoming authentication from the Security/UsernameToken
// See OASIS Web Services Username Token Profile 1.0
// https://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-username-token-profile-1.0.pdf
//
void
SOAPMessage::CheckUsernameToken(XMLElement* p_token)
{
  XMLElement* usern = FindElement(p_token,"Username",false);
  XMLElement* psswd = FindElement(p_token,"Password",false);

  // Nothing to do here
  if(usern == nullptr || psswd == nullptr)
  {
    return;
  }

  // Keep username + password
  m_user     = usern->GetValue();
  m_password = psswd->GetValue();

  // Find nonce/created
  XMLAttribute* type = FindAttribute(psswd,"Type");
  if(type && type->m_value.Find("#PasswordDigest") >= 0)
  {
    XMLElement*  nonce = FindElement(p_token,"Nonce",false);
    XMLElement*  creat = FindElement(p_token,"Created",false);
    // IMPLICIT: Password text = Base64 ( SHA1 ( nonce + created + password ))
    if(nonce && creat)
    {
      m_tokenNonce   = nonce->GetValue();
      m_tokenCreated = creat->GetValue();
    }
  }
}

bool
SOAPMessage::SetTokenProfile(XString p_user,XString p_password,XString p_nonce /*=""*/, XString p_created /*=""*/)
{
  XString namesp(NAMESPACE_SECEXT);

  XMLElement* header = FindElement("Header", false);
  if(!header) return false;
  XMLElement* secur = FindElement(header,"Security",false);
  if(!secur)
  {
    SetElementNamespace(m_root,"wsse",namesp);
    secur = AddElement(header,"wsse:Security",XDT_String,"");
  }

  XMLElement* token = FindElement(secur,"UsernameToken",false);
  if(!token)
  {
    token = AddElement(secur,"wsse:UsernameToken",XDT_String,"");
  }

  XMLElement* user = FindElement(token,"Username",false);
  if(!user)
  {
    user = AddElement(token,"wsse:Username",XDT_String,"");
  }
  user->SetValue(p_user);

  XMLElement* passwd = FindElement(token,"Password",false);
  if(!passwd)
  {
    passwd = AddElement(token,"wsse:Password",XDT_String,"");
  }
  passwd->SetValue(p_password);

  if(!p_nonce.IsEmpty() && !p_created.IsEmpty())
  {
    Crypto crypt;
    // Password text = Base64(SHA1(nonce + created + password))
    XString combined = p_nonce + p_created + p_password;
    m_password = crypt.Digest(combined.GetString(),combined.GetLength(),CALG_SHA1);
    passwd->SetValue(m_password);
    XString digest = namesp + XString("#PasswordDigest");
    SetAttribute(passwd,"Type",digest.GetString());

    XMLElement* nonce = FindElement(token,"Nonce",false);
    if(!nonce)
    {
      nonce = AddElement(token,"wsse:Nonce",XDT_String,"");
    }
    nonce->SetValue(Base64::Encrypt(p_nonce));
    
    XMLElement* creat = FindElement(token,"Created",false);
    if(!creat)
    {
      creat = AddElement(token,"wsse:Created",XDT_String,"");
    }
    creat->SetValue(p_created);
  }

  // Keep all info in members
  m_user         = p_user;
  m_password     = p_password;
  m_tokenNonce   = p_nonce;
  m_tokenCreated = p_created;

  return true;
}

#pragma endregion Signing and Encryption
