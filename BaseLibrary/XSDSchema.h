/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: XSDSchema.h
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
// Partial implementation of XSD Schema as defined by W3C
// See: https://www.w3.org/TR/xmlschema11-1
//
//////////////////////////////////////////////////////////////////////////

#pragma once
#include "XMLMessage.h"
#include <vector>
#include <map>

enum class XsdError
{
  XSDE_NoError
 ,XSDE_Cannot_load_xsd_file
 ,XSDE_No_valid_xml_definition
 ,XSDE_schema_root_invalid
 ,XSDE_schema_missing
 ,XSDE_primary_namespace_not_qualified
 ,XSDE_primary_namespace_missing
 ,XSDE_xml_schema_missing
 ,XSDE_xml_schema_must_be_xml_namespace
 ,XSDE_missing_targetnamespace
 ,XSDE_targetnamespace_must_not_be_qualified
 ,XSDE_complex_type_without_a_name
 ,XSDE_unknown_simple_type_restriction
 ,XSDE_Schema_has_no_starting_element
 ,XSDE_Missing_elements_in_xml_at_level_0
 ,XSDE_Extra_elements_in_xml_at_level_0
 ,XSDE_Missing_elements_in_xml
 ,XSDE_Extra_elements_in_xml // no more
 ,XSDE_Element_not_in_xsd
 ,XSDE_Unknown_ComplexType_ordering
 ,XSDE_base_datatype_violation
 ,XSDE_datatype_restriction_violation
 ,XSDE_Missing_element_in_xml
 ,XSDE_only_one_choice_element
 ,XSDE_element_not_qualified
};

#define XSDSCHEMA "http://www.w3.org/2001/XMLSchema"

using ElementMap  = std::vector<XMLElement*>;
using RestrictMap = std::vector<XMLRestriction*>;

class XSDComplexType: public XMLElement
{
public:
  XSDComplexType(XString p_name);
 ~XSDComplexType();

  XString    m_name;
  WsdlOrder  m_order;
  ElementMap m_elements;
};

using ComplexMap = std::map<XString,XSDComplexType*>;

//////////////////////////////////////////////////////////////////////////

class XSDSchema
{
public:
  XSDSchema();
  virtual ~XSDSchema();

  // Reading and writing
  XsdError ReadXSDSchema(XString p_fileName);
  bool    WriteXSDSchema(XString p_fileName);

  // Validate an XML document, with optional starting point
  XsdError ValidateXML(XMLMessage& p_document,XString& p_error,XMLElement* p_start = nullptr);

private:
  // Read the schema root node
  XsdError ReadXSDSchemaRoot(XMLMessage& p_doc);
  // Read the list of complex types
  XsdError ReadXSDComplexTypes(XMLMessage& p_doc);
  // Read all elements in the schema
  XsdError ReadXSDElements(XMLMessage& p_doc);
  // Read the content of one ComplexType
  XsdError ReadXSDComplexType   (XMLMessage& p_doc,XMLElement* p_elem,XSDComplexType* p_type);
  XsdError ReadElementDefinition(XMLMessage& p_doc,XMLElement* p_elem,XMLElement** p_next);

  // Writing the root of the XSD schema
  void     WriteXSDSchemaRoot(XMLMessage& p_doc);
  // Write all XSD elements
  void     WriteXSDElements(XMLMessage& doc,XMLElement* p_base,ElementMap& p_elements);
  // Write all XSD complex types
  void     WriteXSDComplexTypes(XMLMessage& p_doc); 
  // Write all XSD restrictions to a node
  void     WriteXSDRestrictions(XMLMessage& p_doc,XMLElement* p_elem,XMLRestriction* p_restrict);

  // Handle ComplexType objects and helper functions
  XSDComplexType* FindComplexType(XString p_name);
  XSDComplexType* AddComplexType (XString p_name);
  bool            StripSchemaNS  (XString& p_name);
  XMLElement*     FindElement(ElementMap& p_map,XString p_name);
  bool            XMLRestrictionMoreThanBase(XMLRestriction* p_restrict);

  // Validation
  XsdError  ValidateElement(XMLMessage& p_doc,XMLElement* p_compare,XMLElement* p_schemaElement,XString& p_error);
  XsdError  ValidateOrder  (XMLMessage& p_doc,XMLElement* p_compare,XSDComplexType* p_complex,XString& p_error);
  XsdError  ValidateValue  (XMLRestriction* p_restrict,XMLElement* p_compare,XmlDataType p_datatype,XString p_type,XString& p_error);

  XsdError ValidateOrderSequence(XMLMessage& p_doc,XMLElement* p_compare,ElementMap& p_elements,XString& p_error);
  XsdError ValidateOrderChoice  (XMLMessage& p_doc,XMLElement* p_compare,ElementMap& p_elements,XString& p_error);
  XsdError ValidateOrderAll     (XMLMessage& p_doc,XMLElement* p_compare,ElementMap& p_elements,XString& p_error);

  // Schema names
  XString     m_namespace;        // Namespace of the schema (xmlns)
  XString     m_xmlNamespace;     // XML Schema ("http://www.w3.org/2001/XMLSchema")
  XString     m_targetNamespace;  // Target namespace of the schema (targetNamespace)
  XString     m_xs;               // XML Schema qualifier (xs:)
  bool        m_qualified;        // true or false
  // Contents of the schema
  ElementMap  m_elements;         // All elements in the schema
  ComplexMap  m_types;            // All complex types
  RestrictMap m_restrictions;     // Keep track of all restrictions
};

