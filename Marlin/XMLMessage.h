/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: XMLMessage.h
//
// Marlin Server: Internet server/client
// 
// Copyright (c) 2016 ir. W.E. Huisman
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
#pragma once
#include <deque>

// XML DATA TYPES (SDT)
#define XDT_String      0x000001
#define XDT_Integer     0x000002
#define XDT_Boolean     0x000004
#define XDT_Double      0x000008
#define XDT_Base64      0x000010
#define XDT_DateTime    0x000020
#define XDT_CDATA       0x000040
#define XDT_Complex     0x000080

#define XDT_Mask        0x0000ff

// WSDL Options (USE ONLY ONE (1) !!)
#define WSDL_Mandatory  0x000100
#define WSDL_Optional   0x000200
#define WSDL_ZeroOne    0x000400
#define WSDL_OnceOnly   0x000800
#define WSDL_ZeroMany   0x001000
#define WSDL_OneMany    0x002000
#define WSDL_Choice     0x004000 // Choice
#define WSDL_Sequence   0x008000 // Exact sequence

#define WSDL_Mask       0x00ff00
#define WSDL_MaskOrder  0x00C000
#define WSDL_MaskField  0x003F00


typedef unsigned short ushort;
typedef ushort   XmlDataType;

// XML coding of the message
enum class XMLEncoding
{
   ENC_Plain     = 0  // No action taken, use GetACP(): windows-1252 in "The Netherlands"
  ,ENC_UTF8      = 1  // Default XML encoding
  ,ENC_UTF16     = 2  // Default MS-Windows encoding
  ,ENC_ISO88591  = 3  // Default (X)HTML 4.01 encoding
};

// Ordering of the parameters in the WSDL
enum class WsdlOrder
{
   WS_All        // All parameters must occur, order is not important
  ,WS_Choice     // One of the parameters must occur
  ,WS_Sequence   // All parameters must occur in this order
};

// Internal error codes
enum class XmlError
{
   XE_NoError               // ALL OK!
  ,XE_EmptyXML              // Empty message
  ,XE_IncompatibleEncoding  // Unknown or incompatible encoding BOM
  ,XE_NotAnXMLMessage       // Not a legal XML message
  ,XE_NoRootElement         // Missing root element
  ,XE_MissingClosing        // Missing closing tag or '>'
  ,XE_MissingToken          // Missing token. e.g. '=' for an attribute
  ,XE_MissingElement        // Missing element name after '<'
  ,XE_DTDNotSupported       // Internal DTD parsing not supported
  ,XE_MissingEndTag         // Incorrect or missing end tag
  ,XE_OutOfMemory           // Element not added
  ,XE_ExtraText             // Extra text after closing of root node
  ,XE_HeaderAttribs         // Unknown header attributes instead of version/encoding/standalone
  ,XE_NoBody                // SOAP/XML with Envelope, but no body
  ,XE_EmptyCommand          // SOAP/XML empty message
  ,XE_UnknownProtocol       // Unknown XML protocol
  ,XE_UnknownEncoding       // Unknown encoding in header of the XML message
  ,XE_UnknownXMLParser      // No XMLParser for this message
};

// How we treat whitespace whit in an xml message
enum class WhiteSpace
{
  PRESERVE_WHITESPACE
 ,COLLAPSE_WHITESPACE
};

// Forward declarations
class XMLAttribute;
class XMLElement;
class XMLParser;
class XMLParserImport;
class XMLRestriction;

// Different types of maps for the server message
using XmlElementMap = std::deque<XMLElement*>;
using XmlAttribMap  = std::deque<XMLAttribute>;

// SOAP parameters and attributes are stored in these
class XMLAttribute
{
public:
  XmlDataType  m_type { 0 };
  CString      m_namespace;
  CString      m_name;
  CString      m_value;
};

class XMLElement
{
public:
  XMLElement();
  XMLElement(const XMLElement& p_source);

 ~XMLElement();
  void            Reset();

  // GETTERS
  CString         GetNamespace()    { return m_namespace;   };
  CString         GetName()         { return m_name;        };
  XmlDataType     GetType()         { return m_type;        };
  CString         GetValue()        { return m_value;       };
  XMLRestriction* GetRestriction()  { return m_restriction; };

  // (Public!) data
  CString         m_namespace;
  CString         m_name;
  XmlDataType     m_type { 0 };
  CString         m_value;
  XmlAttribMap    m_attributes;
  XmlElementMap   m_elements;
  XMLElement*     m_parent      { nullptr };
  XMLRestriction* m_restriction { nullptr };
};

//////////////////////////////////////////////////////////////////////////
//
// The XML message
//
//////////////////////////////////////////////////////////////////////////

class XMLMessage
{
public:
  // General XTOR
  XMLMessage();
  // XTOR from another message
  XMLMessage(XMLMessage* p_orig);
  // DTOR
  virtual ~XMLMessage();

  // Reset all internal structures
  virtual void    Reset();
  // Parse string XML to internal structures
  virtual void    ParseMessage(CString& p_message, WhiteSpace p_whiteSpace = WhiteSpace::PRESERVE_WHITESPACE);
  // Parse string with specialized XML parser
  virtual void    ParseMessage(XMLParser* p_parser,CString& p_message,WhiteSpace p_whiteSpace = WhiteSpace::PRESERVE_WHITESPACE);
    // Parse from a beginning node
  virtual void    ParseForNode(XMLElement* p_node, CString& p_message, WhiteSpace p_whiteSpace = WhiteSpace::PRESERVE_WHITESPACE);
  // Print the XML again
  virtual CString Print();
  // Print the XML header
  CString         PrintHeader();
  // Print the elements stack
  virtual CString PrintElements(XMLElement* p_element
                               ,bool        p_utf8  = true
                               ,int         p_level = 0);
  // Print the XML as a JSON object
  virtual CString PrintJson(bool p_attributes);
  // Print the elements stack as a JSON string
  virtual CString PrintElementsJson(XMLElement* p_element
                                   ,bool        p_attributes
                                   ,bool        p_utf8  = true
                                   ,int         p_level = 0);

  // FILE OPERATIONS

  // Load from file
  bool            LoadFile(const CString& p_fileName);
  bool            LoadFile(const CString& p_fileName, XMLEncoding p_encoding);
  // Save to file
  bool            SaveFile(const CString& p_fileName);
  bool            SaveFile(const CString& p_fileName, XMLEncoding p_encoding);

  // SETTERS
  // Set the output encoding of the message
  void            SetEncoding(XMLEncoding p_encoding);
  // Set the name of the root-node
  void            SetRootNodeName(CString p_name);
  // Set condensed format (no spaces or newlines)
  void            SetCondensed(bool p_condens);
  void            SetPrintRestrictions(bool p_restrict);
  // Set sending in Unicode
  void            SetSendUnicode(bool p_unicode);
  void            SetSendBOM(bool p_bom);
  // Setting an element
  XMLElement*     SetElement(CString p_name, CString&    p_value);
  XMLElement*     SetElement(CString p_name, const char* p_value);
  XMLElement*     SetElement(CString p_name, int         p_value);
  XMLElement*     SetElement(CString p_name, bool        p_value);
  XMLElement*     SetElement(CString p_name, double      p_value);

  XMLElement*     SetElement(XMLElement* p_base, CString p_name, CString&    p_value);
  XMLElement*     SetElement(XMLElement* p_base, CString p_name, const char* p_value);
  XMLElement*     SetElement(XMLElement* p_base, CString p_name, int         p_value);
  XMLElement*     SetElement(XMLElement* p_base, CString p_name, bool        p_value);
  XMLElement*     SetElement(XMLElement* p_base, CString p_name, double      p_value);

  // Special setters for elements
  void            SetElementValue(XMLElement* p_elem, XmlDataType p_type, CString p_value);
  void            SetElementOptions(XMLElement* p_elem, XmlDataType p_options);
  void            SetElementNamespace(XMLElement* p_elem, CString p_namespace, CString p_contract, bool p_trailingSlash = false);

  // General base setting of an element
  XMLElement*     SetElement(XMLElement* p_base, CString p_name, XmlDataType p_type, CString p_value, bool p_front = false);
  // General add an element (always adds, so multiple parameters of same name can be added)
  XMLElement*     AddElement(XMLElement* p_base, CString p_name, XmlDataType p_type, CString p_value, bool p_front = false);
  // Set attribute of an element
  XMLAttribute*   SetAttribute(XMLElement* p_elem, CString p_name, CString&    p_value);
  XMLAttribute*   SetAttribute(XMLElement* p_elem, CString p_name, const char* p_value);
  XMLAttribute*   SetAttribute(XMLElement* p_elem, CString p_name, int         p_value);
  XMLAttribute*   SetAttribute(XMLElement* p_elem, CString p_name, bool        p_value);
  XMLAttribute*   SetAttribute(XMLElement* p_elem, CString p_name, double      p_value);

  // GETTERS
  CString         GetRootNodeName();
  XmlError        GetInternalError();
  CString         GetInternalErrorString();
  XMLEncoding     GetEncoding();
  bool            GetCondensed();
  bool            GetPrintRestrictions();
  bool            GetSendUnicode();
  bool            GetSendBOM();
  XMLElement*     GetRoot();
  CString         GetElement(CString p_name);
  int             GetElementInteger(CString p_name);
  bool            GetElementBoolean(CString p_name);
  double          GetElementDouble(CString p_name);
  CString         GetElement(XMLElement* p_elem, CString p_name);
  int             GetElementInteger(XMLElement* p_elem, CString p_name);
  bool            GetElementBoolean(XMLElement* p_elem, CString p_name);
  double          GetElementDouble(XMLElement* p_elem, CString p_name);
  XMLElement*     GetElementFirstChild(XMLElement* p_elem);
  XMLElement*     GetElementSibling(XMLElement* p_elem);
  CString         GetAttribute(XMLElement* p_elem,CString p_attribName);
  int             GetAttributeInteger(XMLElement* p_elem,CString p_attribName);
  bool            GetAttributeBoolean(XMLElement* p_elem,CString p_attribName);
  double          GetAttributeDouble(XMLElement* p_elem,CString p_attribName);

  // FINDING
  XMLElement*     FindElement(CString p_name, bool p_recurse = true);
  XMLElement*     FindElement(XMLElement* p_base, CString p_name, bool p_recurse = true);
  XMLElement*     FindElementWithAttribute(XMLElement* p_base
                                          ,CString     p_elementName
                                          ,CString     p_attribName
                                          ,CString     p_attribValue = ""
                                          ,bool        p_recurse = true);
  XMLAttribute*   FindAttribute(XMLElement* p_elem, CString p_attribName);

  // EXTRA OPERATIONS on element nodes

  // Delete element with this name (possibly the nth instance of this name) (base can be NULL)
  bool            DeleteElement(XMLElement* p_base, CString p_name, int p_instance = 0);
  // Delete exactly this element (base can be NULL)
  bool            DeleteElement(XMLElement* p_base, XMLElement* p_element);
  // Delete exactly this attribute
  bool            DeleteAttribute(XMLElement* p_element, CString p_attribName);

  // Finding an element by attribute value
  XMLElement*     FindElementByAttribute(CString p_attribute, CString p_value);
  XMLElement*     FindElementByAttribute(XMLElement* p_element, CString p_attribute, CString p_value);

protected:
  // Print the WSDL Comments in the message
  CString         PrintWSDLComment(XMLElement* p_element);
  // Parser for the XML texts
  friend          XMLParser;
  friend          XMLParserImport;
  // The one and only rootnode
  XMLElement      m_root;                                     // All elements, from the root up
  XMLEncoding     m_encoding        { XMLEncoding::ENC_UTF8 };// Encoding scheme
  CString         m_version         { "1.0" };                // XML Version, most likely 1.0
  CString         m_standalone;                               // Stand alone from DTD or XSD's
  bool            m_condensed       { false } ;               // Condensed output format (no spaces/newlines)
  bool            m_whitespace      { false };                // Collapse whitespace
  bool            m_sendUnicode     { false };                // Construct UTF-16 on sending out
  bool            m_sendBOM         { false };                // Prepend Byte-Order-Mark before message (UTF-8 / UTF-16)
  bool            m_printRestiction { false };                // Print restrictions as comment before node
  // Status and other info
  XmlError        m_internalError   { XmlError::XE_NoError }; // Internal error status
  CString         m_internalErrorString;                      // Human readable form of the error
};

//////////////////////////////////////////////////////////////////////////
// XMLMessage

inline XMLElement*
XMLMessage::GetRoot()
{
  return &m_root;
}

inline void
XMLMessage::SetCondensed(bool p_condens)
{
  m_condensed = p_condens;
}

inline XmlError 
XMLMessage::GetInternalError()
{
  return m_internalError;
}

inline CString  
XMLMessage::GetInternalErrorString()
{
  return m_internalErrorString;
}

inline void
XMLMessage::SetRootNodeName(CString p_name)
{
  m_root.m_name = p_name;
}

inline CString
XMLMessage::GetRootNodeName()
{
  return m_root.m_name;
}

inline XMLEncoding
XMLMessage::GetEncoding()
{
  return m_encoding;
}

inline bool
XMLMessage::GetCondensed()
{
  return m_condensed;
}

inline bool
XMLMessage::GetSendUnicode()
{
  return m_sendUnicode;
}

inline bool
XMLMessage::GetSendBOM()
{
  return m_sendBOM;
}

inline void
XMLMessage::SetSendBOM(bool p_bom)
{
  m_sendBOM = p_bom;
}

inline bool
XMLMessage::GetPrintRestrictions()
{
  return m_printRestiction;
}

inline void
XMLMessage::SetPrintRestrictions(bool p_restrict)
{
  m_printRestiction = p_restrict;
}
