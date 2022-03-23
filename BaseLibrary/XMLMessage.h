/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: XMLMessage.h
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
#pragma once
#include "XMLDataType.h"
#include "ConvertWideString.h"
#include <deque>

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
class XMLMessage;
class XMLParser;
class XMLParserImport;
class XMLRestriction;

// Different types of maps for the server message
using XmlElementMap = std::deque<XMLElement*>;
using XmlAttribMap  = std::deque<XMLAttribute>;
using ushort        = unsigned short;

// SOAP parameters and attributes are stored in these
class XMLAttribute
{
public:
  XmlDataType  m_type { 0 };
  XString      m_namespace;
  XString      m_name;
  XString      m_value;
};

class XMLElement
{
public:
  XMLElement();
  XMLElement(XMLElement* p_parent);
  XMLElement(const XMLElement& p_source);

 ~XMLElement();
  void            Reset();

  // GETTERS
  XString         GetNamespace()    { return m_namespace;   };
  XString         GetName()         { return m_name;        };
  XmlDataType     GetType()         { return m_type;        };
  XString         GetValue()        { return m_value;       };
  XmlAttribMap&   GetAttributes()   { return m_attributes;  };
  XmlElementMap&  GetChildren()     { return m_elements;    };
  XMLElement*     GetParent()       { return m_parent;      };
  XMLRestriction* GetRestriction()  { return m_restriction; };

  // TESTERS
  static bool     IsValidName(const XString& p_name);
  static XString  InvalidNameMessage(const XString& p_name);

  // SETTERS
  void            SetParent(XMLElement* parent)  { m_parent    = parent;    };
  void            SetNamespace(XString p_namesp) { m_namespace = p_namesp;  };
  void            SetName(XString p_name);
  void            SetType(XmlDataType p_type)    { m_type      = p_type;    };
  void            SetValue(XString p_value)      { m_value     = p_value;   };
  void            SetRestriction(XMLRestriction* p_restrict) { m_restriction = p_restrict; };

  // REFERENCE SYSTEM
  void            AddReference();
  void            DropReference();

private:
  // Our element node data
  XString         m_namespace;
  XString         m_name;
  XmlDataType     m_type { 0 };
  XString         m_value;
  XmlAttribMap    m_attributes;
  XmlElementMap   m_elements;
  XMLElement*     m_parent      { nullptr };
  XMLRestriction* m_restriction { nullptr };
  long            m_references  { 1       };
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
  virtual void    ParseMessage(XString& p_message, WhiteSpace p_whiteSpace = WhiteSpace::PRESERVE_WHITESPACE);
  // Parse string with specialized XML parser
  virtual void    ParseMessage(XMLParser* p_parser,XString& p_message,WhiteSpace p_whiteSpace = WhiteSpace::PRESERVE_WHITESPACE);
    // Parse from a beginning node
  virtual void    ParseForNode(XMLElement* p_node, XString& p_message, WhiteSpace p_whiteSpace = WhiteSpace::PRESERVE_WHITESPACE);
  // Print the XML again
  virtual XString Print();
  // Print the XML header
  XString         PrintHeader();
  XString         PrintStylesheet();
  // Print the elements stack
  virtual XString PrintElements(XMLElement* p_element
                               ,bool        p_utf8  = true
                               ,int         p_level = 0);
  // Print the XML as a JSON object
  virtual XString PrintJson(bool p_attributes);
  // Print the elements stack as a JSON string
  virtual XString PrintElementsJson(XMLElement*     p_element
                                   ,bool            p_attributes
                                   ,StringEncoding  p_encoding = StringEncoding::ENC_UTF8
                                   ,int             p_level    = 0);

  // FILE OPERATIONS

  // Load from file
  virtual bool    LoadFile(const XString& p_fileName);
  virtual bool    LoadFile(const XString& p_fileName, StringEncoding p_encoding);
  // Save to file
  virtual bool    SaveFile(const XString& p_fileName,bool p_withBom = false);
  virtual bool    SaveFile(const XString& p_fileName,StringEncoding p_encoding,bool p_withBom = false);

  // SETTERS
  // Set the output encoding of the message
  void            SetEncoding(StringEncoding p_encoding);
  // Set the name of the root-node
  void            SetRootNodeName(XString p_name);
  // Set condensed format (no spaces or newlines)
  void            SetCondensed(bool p_condens);
  void            SetPrintRestrictions(bool p_restrict);
  // Set sending in Unicode
  void            SetSendUnicode(bool p_unicode);
  void            SetSendBOM(bool p_bom);
  // Stylesheet info
  void            SetStylesheetType(XString p_type);
  void            SetStylesheet(XString p_sheet);
  // Setting an element
  XMLElement*     SetElement(XString p_name, XString&    p_value);
  XMLElement*     SetElement(XString p_name, const char* p_value);
  XMLElement*     SetElement(XString p_name, int         p_value);
  XMLElement*     SetElement(XString p_name, bool        p_value);
  XMLElement*     SetElement(XString p_name, double      p_value);

  XMLElement*     SetElement(XMLElement* p_base, XString p_name, XString&    p_value);
  XMLElement*     SetElement(XMLElement* p_base, XString p_name, const char* p_value);
  XMLElement*     SetElement(XMLElement* p_base, XString p_name, int         p_value);
  XMLElement*     SetElement(XMLElement* p_base, XString p_name, bool        p_value);
  XMLElement*     SetElement(XMLElement* p_base, XString p_name, double      p_value);

  // Special setters for elements
  void            SetElementValue(XMLElement* p_elem, XmlDataType p_type, XString p_value);
  void            SetElementOptions(XMLElement* p_elem, XmlDataType p_options);
  void            SetElementNamespace(XMLElement* p_elem, XString p_namespace, XString p_contract, bool p_trailingSlash = false);

  // General base setting of an element
  XMLElement*     SetElement(XMLElement* p_base, XString p_name, XmlDataType p_type, XString p_value, bool p_front = false);
  // General add an element (always adds, so multiple parameters of same name can be added)
  XMLElement*     AddElement(XMLElement* p_base, XString p_name, XmlDataType p_type, XString p_value, bool p_front = false);
  // Set attribute of an element
  XMLAttribute*   SetAttribute(XMLElement* p_elem, XString p_name, XString&    p_value);
  XMLAttribute*   SetAttribute(XMLElement* p_elem, XString p_name, const char* p_value);
  XMLAttribute*   SetAttribute(XMLElement* p_elem, XString p_name, int         p_value);
  XMLAttribute*   SetAttribute(XMLElement* p_elem, XString p_name, bool        p_value);
  XMLAttribute*   SetAttribute(XMLElement* p_elem, XString p_name, double      p_value);

  // GETTERS
  XString         GetRootNodeName();
  XmlError        GetInternalError();
  XString         GetInternalErrorString();
  StringEncoding  GetEncoding();
  bool            GetCondensed();
  bool            GetPrintRestrictions();
  bool            GetSendUnicode();
  bool            GetSendBOM();
  XMLElement*     GetRoot();
  void            SetRoot(XMLElement* p_root);
  XString         GetElement(XString p_name);
  int             GetElementInteger(XString p_name);
  bool            GetElementBoolean(XString p_name);
  double          GetElementDouble(XString p_name);
  XString         GetElement(XMLElement* p_elem, XString p_name);
  int             GetElementInteger(XMLElement* p_elem, XString p_name);
  bool            GetElementBoolean(XMLElement* p_elem, XString p_name);
  double          GetElementDouble(XMLElement* p_elem, XString p_name);
  XMLElement*     GetElementFirstChild(XMLElement* p_elem);
  XMLElement*     GetElementSibling(XMLElement* p_elem);
  XString         GetAttribute(XMLElement* p_elem,XString p_attribName);
  int             GetAttributeInteger(XMLElement* p_elem,XString p_attribName);
  bool            GetAttributeBoolean(XMLElement* p_elem,XString p_attribName);
  double          GetAttributeDouble(XMLElement* p_elem,XString p_attribName);
  XString         GetStylesheetType();
  XString         GetStylesheet();

  // FINDING
  XMLElement*     FindElement(XString p_name, bool p_recurse = true);
  XMLElement*     FindElement(XMLElement* p_base, XString p_name, bool p_recurse = true);
  XMLElement*     FindElementWithAttribute(XMLElement* p_base
                                          ,XString     p_elementName
                                          ,XString     p_attribName
                                          ,XString     p_attribValue = ""
                                          ,bool        p_recurse = true);
  XMLAttribute*   FindAttribute(XMLElement* p_elem, XString p_attribName);

  // EXTRA OPERATIONS on element nodes

  // Delete element with this name (possibly the nth instance of this name) (base can be NULL)
  bool            DeleteElement(XMLElement* p_base, XString p_name, int p_instance = 0);
  // Delete exactly this element (base can be NULL)
  bool            DeleteElement(XMLElement* p_base, XMLElement* p_element);
  // Delete exactly this attribute
  bool            DeleteAttribute(XMLElement* p_element, XString p_attribName);

  // Finding an element by attribute value
  XMLElement*     FindElementByAttribute(XString p_attribute, XString p_value);
  XMLElement*     FindElementByAttribute(XMLElement* p_element, XString p_attribute, XString p_value);

  // XMLElements can be stored elsewhere. Use the reference mechanism to add/drop references
  // With the drop of the last reference, the object WILL destroy itself
  void            AddReference();
  void            DropReference();

protected:
  // Encrypt the whole message: yielding a new message
  virtual void    EncryptMessage(XString& p_message);
  // Print the WSDL Comments in the message
  XString         PrintWSDLComment(XMLElement* p_element);
  // Parser for the XML texts
  friend          XMLParser;
  friend          XMLParserImport;
  // The one and only rootnode
  XMLElement*     m_root            { nullptr };              // All elements, from the root up
  StringEncoding  m_encoding        { StringEncoding::ENC_UTF8 };// Encoding scheme
  XString         m_version         { "1.0" };                // XML Version, most likely 1.0
  XString         m_standalone;                               // Stand alone from DTD or XSD's
  bool            m_condensed       { false } ;               // Condensed output format (no spaces/newlines)
  bool            m_whitespace      { false };                // Collapse whitespace
  bool            m_sendUnicode     { false };                // Construct UTF-16 on sending out
  bool            m_sendBOM         { false };                // Prepend Byte-Order-Mark before message (UTF-8 / UTF-16)
  bool            m_printRestiction { false };                // Print restrictions as comment before node
  // Stylesheet info
  XString         m_stylesheetType;                           // Content type of the XSL stylesheet
  XString         m_stylesheet;                               // Link tot he XSL stylesheet
  // Status and other info
  XmlError        m_internalError   { XmlError::XE_NoError }; // Internal error status
  XString         m_internalErrorString;                      // Human readable form of the error
  long            m_references      { 1 };                    // Externally referenced
};

//////////////////////////////////////////////////////////////////////////
// XMLMessage

inline XMLElement*
XMLMessage::GetRoot()
{
  return m_root;
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

inline XString  
XMLMessage::GetInternalErrorString()
{
  return m_internalErrorString;
}

inline void
XMLMessage::SetRootNodeName(XString p_name)
{
  m_root->SetName(p_name);
}

inline XString
XMLMessage::GetRootNodeName()
{
  return m_root->GetName();
}

inline StringEncoding
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

inline XString
XMLMessage::GetStylesheetType()
{
  return m_stylesheetType;
}

inline XString
XMLMessage::GetStylesheet()
{
  return m_stylesheet;
}

inline void
XMLMessage::SetStylesheetType(XString p_type)
{
  m_stylesheetType = p_type;
}

inline void
XMLMessage::SetStylesheet(XString p_sheet)
{
  m_stylesheet = p_sheet;
}
