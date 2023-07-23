/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: WSDLCache.h
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
// WSDLCache
//
//////////////////////////////////////////////////////////////////////////

#pragma once
#include "SOAPMessage.h"
#include "XMLRestriction.h"
#include <vector>
#include <map>

class LogAnalysis;

class WsdlOperation
{
public:
  int          m_code;
  SOAPMessage* m_input;
  SOAPMessage* m_output;
};

using OperationMap = std::map<XString,WsdlOperation>;
using TypeDone     = std::map<XString,int>;
using TypeMap      = std::map<XString,XMLElement*>;

class WSDLCache
{
public:
  explicit WSDLCache(bool p_server = true);
 ~WSDLCache();

  // Reset the complete WSDLCache
  void    Reset();
  // Clear the cache
  void    ClearCache();
  // Generate the resulting WSDL file
  bool    GenerateWSDL();
  // Read an existing WSDL file
  bool    ReadWSDLFile(LPCTSTR p_filename);
  // Read an existing WSDL from file buffer
  bool    ReadWSDLString(XString p_wsdl);
  // Add SOAP message call and answer
  bool    AddOperation(int p_code,XString p_name,SOAPMessage* p_input,SOAPMessage* p_output);
  // Check incoming SOAP message against WSDL
  bool    CheckIncomingMessage(SOAPMessage* p_msg,bool p_checkFields);
  // Check outgoing SOAP message against WSDL
  bool    CheckOutgoingMessage(SOAPMessage* p_msg,bool p_checkFields);

  // SETTERS

  // MANDATORY: Set a service
  bool    SetService(XString p_servicename,XString p_url);
  // MANDATORY: Set a target namespace
  void    SetTargetNamespace(XString p_name)     { m_targetNamespace = p_name;     };
  // MANDATORY: Set a webroot (where to save the WSDL)
  void    SetWebroot(XString p_webroot)          { m_webroot         = p_webroot;  };
  // OPTIONAL:  Set SOAP 1.0
  void    SetPerformSoap10(bool p_perform)       { m_performSoap10   = p_perform;  };
  // OPTIONAL:  Set SOAP 1.2
  void    SetPerformSoap12(bool p_perform)       { m_performSoap12   = p_perform;  };
  // OPTIONAL:  Set service postfix other than ".aspcxx"
  void    SetServicePostfix(XString p_postfix)   { m_servicePostfix  = p_postfix;  };
  // OPTIONAL:  Set the logfile (never owned, or generated!)
  void    SetLogAnalysis(LogAnalysis* p_log)     { m_logging         = p_log;      };
  // OPTIONAL:  Set a new output filename before generating
  void    SetWSDLFilename(XString p_filename)    {m_filename         = p_filename; };

  // GETTERS

  XString GetServicePage();
  XString GetOperationPage(XString p_operation,XString p_hostname);
  XString GetServiceBasePageName();
  int     GetCommandCode(XString& p_commandName);
  size_t  GetOperationsCount()                   { return m_operations.size();     };
  XString GetErrorMessage()                      { return m_errormessage;          };
  XString GetWSDLFilename()                      { return m_filename;              };
  XString GetTargetNamespace()                   { return m_targetNamespace;       };
  XString GetWebroot()                           { return m_webroot;               };
  XString GetServiceName()                       { return m_serviceName;           };
  XString GetServicePostfix()                    { return m_servicePostfix;        };
  XString GetURL()                               { return m_url;                   };
  XString GetBasePath()                          { return m_absPath;               };
  bool    GetPerformSoap10()                     { return m_performSoap10;         };
  bool    GetPerfromSoap12()                     { return m_performSoap12;         };

private:
  // Check message
  bool    CheckMessage(SOAPMessage* p_orig,SOAPMessage* p_tocheck,XString p_who,bool p_checkFields);
  // Check parameters
  bool    CheckParameters(XMLElement*  p_orgBase
                         ,SOAPMessage* p_orig
                         ,XMLElement*  p_checkBase
                         ,SOAPMessage* p_check
                         ,XString      p_who
                         ,bool         p_fields);
  // Check data field in depth
  bool    CheckFieldDatatypeValues(XMLElement*   p_origParam
                                  ,XMLElement*   p_checkParam
                                  ,SOAPMessage*  p_check
                                  ,XString       p_who);
  // GENERATING A WSDL
  void    GenerateTypes(XString& p_wsdlcontent);
  void    GenerateMessageTypes(XString& p_wsdlcontent,SOAPMessage* p_msg,TypeDone& p_gedaan);
  void    GenerateParameterTypes(XString&         p_wsdlcontent
                                ,XString          p_element
                                ,XmlElementMap&   p_map
                                ,TypeDone&        p_done
                                ,WsdlOrder        p_order
                                ,int              p_level);
  void    GenerateMessages(XString& p_wsdlcontent);
  void    GeneratePortType(XString& p_wsdlcontent);
  void    GenerateBindings(XString& p_wsdlcontent);
  void    GenerateBinding(XString& p_wsdlcontent,XString p_binding,XString p_soapNamespace);
  void    GenerateServiceBindings(XString& p_wsdlcontent);
  void    GenerateDefinitions(XString& p_wsdlcontent);

  // READING A WSDL
  bool    ReadWSDLFileSafe(LPCTSTR p_filename);
  bool    ReadWSDLLocalFile(XString p_filename);
  bool    ReadWSDLFileFromURL(XString p_url);
  bool    ReadWSDL             (XMLMessage& p_wsdl);
  bool    ReadDefinitions      (XMLMessage& p_wsdl);
  bool    ReadServiceBindings  (XMLMessage& p_wsdl);
  bool    ReadBindings         (XMLMessage& p_wsdl);
  bool    ReadPortTypes        (XMLMessage& p_wsdl);
  bool    ReadMessage          (XMLMessage& p_wsdl,SOAPMessage& p_message);
  bool    ReadParameters       (XMLMessage& p_wsdl,SOAPMessage& p_message,XString m_element);
  XMLElement* ReadTypesElement (XMLMessage& p_wsdl,XString p_element);
  XMLElement* ReadTypesType    (XMLMessage& p_wsdl,XString p_type);
  int     ReadWSDLOptions      (XMLMessage& p_wsdl,XMLElement* p_element);
  int     ReadWSDLOrdering     (XMLElement* p_order);
  bool    ReadParametersInOrder(XMLMessage& p_wsdl
                               ,SOAPMessage& p_message
                               ,XMLElement* p_base
                               ,XMLElement* p_order);
  void    ReadRestriction      (XMLMessage& p_wsdl
                               ,XMLElement* p_newelem
                               ,XMLElement* p_order
                               ,XString     p_restriction
                               ,int         p_options);
  int     ReadElementaryType   (XString p_type);
  int     ReadWhiteSpace       (XString p_value);
  XString ReadAnnoDocumentation(XMLMessage& p_wsdl,XMLElement* enumeration);

  // Generating HTML pages for documentation
  XString GetPageHeader();
  XString GetPageFooter();
  XString GetOperationWsdl();
  XString GetOperationNameLink(XString p_operation);
  XString GetOperationPageIntro(XString p_operation);
  XString GetOperationPageHttpI(XString p_operation,XString p_hostname,bool p_soapVersion);
  XString GetOperationPageHttpO(bool p_soapVersion);
  XString GetOperationPageSoap (SOAPMessage* p_msg, bool p_soapVersion);
  XString GetOperationPageFooter();
  
  XString m_filename;               // Where we save the WSDL (webroot / service / name.wsdl)
  XString m_targetNamespace;        // Object of this WSDL
  XString m_webroot;                // Webroot from the HTTPServer
  XString m_serviceName;            // Service name in short
  XString m_url;                    // URL servicing this WSDL
  XString m_absPath;                // Absolute URL path
  XString m_servicePostfix;         // Service name postfix other than ".acx"
  XString m_errormessage;           // Last errormessage reading a WSDL
  bool    m_server        { true }; // Server or client side WSDL?
  bool    m_performSoap10 { true }; // Perform SOAP 1.0 bindings
  bool    m_performSoap12 { true }; // Perform SOAP 1.2 bindings
  bool    m_exception     { false}; // Exception while reading WSDL
  // Complex objects
  OperationMap    m_operations;           // All recorded operations of the service
  XMLRestrictions m_restrictions;         // All restrictions on XMLMessage:XMLElement values
  LogAnalysis*    m_logging { nullptr };  // Logging in the logfile
  TypeMap         m_types;                // Used for reading WSDL
};
