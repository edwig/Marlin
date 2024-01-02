/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: XSDSchema.cpp
//
// Copyright (c) 2014-2024 ir. W.E. Huisman
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

#include "pch.h"
#include "XSDSchema.h"
#include "XMLRestriction.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//////////////////////////////////////////////////////////////////////////
//
// XSDComplexType
//
//////////////////////////////////////////////////////////////////////////

XSDComplexType::XSDComplexType(XString p_name)
               :m_name(p_name)
               ,m_order(WsdlOrder::WS_Sequence)
{
}

XSDComplexType::~XSDComplexType()
{
  for(auto& elem : m_elements)
  {
    delete elem;
  }
  m_elements.clear();
}

//////////////////////////////////////////////////////////////////////////
//
// XSDSchema
//
//////////////////////////////////////////////////////////////////////////

XSDSchema::XSDSchema()
          :m_qualified(false)
{
}

XSDSchema::~XSDSchema()
{
  // Delete all elements
  for(auto& elem : m_elements)
  {
    delete elem;
  }
  m_elements.clear();

  // Delete all complex types
  for(const auto& type : m_types)
  {
    if(type.second)
    {
      delete type.second;
    }
  }
  m_types.clear();

  // Remove all restrictions
  for(auto& restrict : m_restrictions)
  {
    delete restrict;
  }
  m_restrictions.clear();
}

XsdError
XSDSchema::ReadXSDSchema(XString p_fileName)
{
  XMLMessage doc;
  if(!doc.LoadFile(p_fileName))
  {
    return XsdError::XSDE_Cannot_load_xsd_file;
  }
  if(doc.GetInternalError() != XmlError::XE_NoError)
  {
    return XsdError::XSDE_No_valid_xml_definition;
  }
  // Read the schema root and validate it!
  XsdError result = ReadXSDSchemaRoot(doc);
  if(result != XsdError::XSDE_NoError)
  {
    return result;
  }
  // Read all complex types first
  result = ReadXSDComplexTypes(doc);
  if(result != XsdError::XSDE_NoError)
  {
    return result;
  }
  // Read all elements
  result = ReadXSDElements(doc);
  if(result != XsdError::XSDE_NoError)
  {
    return result;
  }
  // Reached the end with success
  return XsdError::XSDE_NoError;
}

bool
XSDSchema::WriteXSDSchema(XString p_fileName)
{
  XMLMessage doc;

  // Set the root
  WriteXSDSchemaRoot(doc);
  // Write all elements
  WriteXSDElements(doc,doc.GetRoot(),m_elements);
  // Write all complex types
  WriteXSDComplexTypes(doc);

  // Save as human readable file
  doc.SetCondensed(false);
  doc.SetEncoding(Encoding::UTF8);
  return doc.SaveFile(p_fileName);
}

// Validate an XML document, with optional starting point
// Typically SOAP messages will present the first child node after their <Body> 
// to this function to validate the functional SOAPMessage
//
XsdError 
XSDSchema::ValidateXML(XMLMessage& p_document
                      ,XString&    p_error
                      ,XMLElement* p_start /*= nullptr*/)
{
  XsdError result = XsdError::XSDE_NoError;

  // Find our starting element
  XMLElement* starting = p_start ? p_start : p_document.GetRoot();

  if(m_elements.empty())
  {
    result  = XsdError::XSDE_Schema_has_no_starting_element;
    p_error = _T("XSDSchema has no starting element");
  }
  else
  {
    // Begin of validation loop
    for(int index = 0;index < (int)m_elements.size(); ++index)
    {
      result   = ValidateElement(p_document,starting,m_elements[index],p_error);
      starting = p_document.GetElementSibling(starting);

      if(result != XsdError::XSDE_NoError)
      {
        break;
      }
      if(starting == nullptr && index < (int)m_elements.size() - 1)
      {
        XsdError::XSDE_Missing_elements_in_xml_at_level_0;
        p_error.AppendFormat(_T("Missing elements after index: %d"),index + 1);
        break;
      }
    }
    if(starting)
    {
      XsdError::XSDE_Extra_elements_in_xml_at_level_0;
      p_error = _T("More elements in XMLMessage than in XSDSchema.");
    }
  }
  return result;
}

//////////////////////////////////////////////////////////////////////////
//
// PRIVATE METHODS
//
//////////////////////////////////////////////////////////////////////////

// <xs:schema     xmlns = "http://www.standard.nl/instruction/SALES/001"
//             xmlns:xs = "http://www.w3.org/2001/XMLSchema"
//      targetNamespace = "http://www.stanaard.nl/instruction/SALES/001"
//   elementFormDefault = "qualified">

// Read the schema root node
XsdError
XSDSchema::ReadXSDSchemaRoot(XMLMessage& p_doc)
{
  XMLElement* root = p_doc.GetRoot();
  if(root->GetName().Compare(_T("schema")))
  {
    return XsdError::XSDE_schema_missing;
  }
  m_xs = root->GetNamespace();
  XMLAttribute* xmlns = p_doc.FindAttribute(root,_T("xmlns"));
  if(xmlns)
  {
    m_namespace = xmlns->m_value;
    if(!xmlns->m_namespace.IsEmpty())
    {
      return XsdError::XSDE_primary_namespace_not_qualified;
    }
  }
  else return XsdError::XSDE_primary_namespace_missing;

  XMLAttribute* xs = p_doc.FindAttribute(root,_T("xs"));
  if(xs)
  {
    m_xs = _T("xs");
    if(xs->m_namespace.Compare(_T("xmlns")))
    {
      return XsdError::XSDE_xml_schema_must_be_xml_namespace;
    }
  }
  else return XsdError::XSDE_xml_schema_missing;

  XMLAttribute* target = p_doc.FindAttribute(root,_T("targetNamespace"));
  if(target)
  {
    m_targetNamespace = target->m_value;
    if(!target->m_namespace.IsEmpty())
    {
      return XsdError::XSDE_targetnamespace_must_not_be_qualified;
    }
  }
  else return XsdError::XSDE_missing_targetnamespace;

  XMLAttribute* formdef = p_doc.FindAttribute(root,_T("elementFormDefault"));
  if(formdef)
  {
    if(formdef->m_value.Compare(_T("qualified")) == 0)
    {
      m_qualified = true;
    }
  }

  // Reached the end with success
  return XsdError::XSDE_NoError;
}

// Read the list of complex types
XsdError 
XSDSchema::ReadXSDComplexTypes(XMLMessage& p_doc)
{
  XMLElement* complex = p_doc.FindElement(_T("complexType"),false);
  while(complex)
  {
    // Skip past elements
    if(complex->GetName().CompareNoCase(_T("complexType")) == 0)
    {
      XString name = p_doc.GetAttribute(complex,_T("name"));
      if(name.IsEmpty())
      {
        return XsdError::XSDE_complex_type_without_a_name;
      }
      XSDComplexType* type = AddComplexType(name);

      XsdError result = ReadXSDComplexType(p_doc,complex,type);
      if(result != XsdError::XSDE_NoError)
      {
        return result;
      }
    }
    // Read next node to scan for more complex types
    complex = p_doc.GetElementSibling(complex);
  }
  // Reached the end with success
  return XsdError::XSDE_NoError;
}

// Read the content of one ComplexType
XsdError
XSDSchema::ReadXSDComplexType(XMLMessage&     p_doc
                             ,XMLElement*     p_elem
                             ,XSDComplexType* p_type)
{
  XsdError result = XsdError::XSDE_NoError;
  XMLElement* selector = p_doc.GetElementFirstChild(p_elem);
  if(selector->GetName().Compare(_T("sequence")) == 0)
  {
    p_type->m_order = WsdlOrder::WS_Sequence;
  }
  if(selector->GetName().Compare(_T("all")) == 0)
  {
    p_type->m_order = WsdlOrder::WS_All;
  }
  if(selector->GetName().Compare(_T("choice")) == 0)
  {
    p_type->m_order = WsdlOrder::WS_Choice;
  }

  XMLElement* elem = p_doc.GetElementFirstChild(selector);
  while(elem)
  {
    XMLElement* next = nullptr;
    result = ReadElementDefinition(p_doc,elem,&next);
    p_type->m_elements.push_back(next);

    if(result != XsdError::XSDE_NoError)
    {
      break;
    }
    // read next element
    elem = p_doc.GetElementSibling(elem);
  }
  return result;
}

// Read all elements in the schema
XsdError
XSDSchema::ReadXSDElements(XMLMessage& p_doc)
{
  XsdError result = XsdError::XSDE_NoError;

  XMLElement* elem = p_doc.FindElement(_T("element"),false);
  while(elem)
  {
    // Skip past complex types
    if(elem->GetName().Compare(_T("element")) == 0)
    {
      XMLElement* next = nullptr;
      result = ReadElementDefinition(p_doc,elem,&next);
      m_elements.push_back(next);

      if(result != XsdError::XSDE_NoError)
      {
        break;
      }
    }
    // Next element 
    elem = p_doc.GetElementSibling(elem);
  }
  // Reached the end with success
  return result;
}

XsdError
XSDSchema::ReadElementDefinition(XMLMessage&  p_doc
                                ,XMLElement*  p_elem
                                ,XMLElement** p_next)
{
  XsdError result   = XsdError::XSDE_NoError;
  XString name      = p_doc.GetAttribute(p_elem,_T("name"));
  XString type      = p_doc.GetAttribute(p_elem,_T("type"));
  XString minoccurs = p_doc.GetAttribute(p_elem,_T("minOccurs"));
  XString maxoccurs = p_doc.GetAttribute(p_elem,_T("maxOccurs"));

  XmlDataType xmlType = 0;
  XMLRestriction* restrict = new XMLRestriction(type);
  m_restrictions.push_back(restrict);

  if(minoccurs.GetLength())
  {
    restrict->AddMinOccurs(minoccurs);
  }
  if(maxoccurs.GetLength())
  {
    restrict->AddMaxOccurs(maxoccurs);
  }

  if(StripSchemaNS(type))
  {
    xmlType = StringToXmlDataType(type);
    restrict->AddBaseType(type);
  }
  else if(type.IsEmpty())
  {
    // Simple type
    XMLElement* simple = p_doc.GetElementFirstChild(p_elem);
    if(simple && simple->GetName().Compare(_T("simpleType")) == 0)
    {
      XMLElement* rest = p_doc.GetElementFirstChild(simple);
      if(rest && rest->GetName().Compare(_T("restriction")) == 0)
      {
        XString base = p_doc.GetAttribute(rest,_T("base"));
        StripSchemaNS(base);
        restrict->AddBaseType(base);

        // Scan all parts
        XMLElement* part = p_doc.GetElementFirstChild(rest);
        while(part)
        {
          XString rname = part->GetName();
          XString value = p_doc.GetAttribute(part,_T("value"));

               if(rname.Compare(_T("length"))         == 0)  restrict->AddLength(_ttoi(value));
          else if(rname.Compare(_T("minLength"))      == 0)  restrict->AddMinLength(_ttoi(value));
          else if(rname.Compare(_T("maxLength"))      == 0)  restrict->AddMaxLength(_ttoi(value));
          else if(rname.Compare(_T("totalDigits"))    == 0)  restrict->AddTotalDigits(_ttoi(value));
          else if(rname.Compare(_T("fractionDigits")) == 0)  restrict->AddFractionDigits(_ttoi(value));
          else if(rname.Compare(_T("minExclusive"))   == 0)  restrict->AddMinExclusive(value);
          else if(rname.Compare(_T("maxExclusive"))   == 0)  restrict->AddMaxExclusive(value);
          else if(rname.Compare(_T("minInclusive"))   == 0)  restrict->AddMinInclusive(value);
          else if(rname.Compare(_T("maxInclusive"))   == 0)  restrict->AddMaxInclusive(value);
          else if(rname.Compare(_T("pattern"))        == 0)  restrict->AddPattern(value);
          else if(rname.Compare(_T("enumeration"))    == 0)
          {
            XString documentation;
            XMLElement* anno = p_doc.GetElementFirstChild(part);
            if(anno && anno->GetName().Compare(_T("annotation")) == 0)
            {
              XMLElement* doc = p_doc.GetElementFirstChild(anno);
              if(doc)
              {
                documentation = doc->GetValue();
              }
            }
            restrict->AddEnumeration(value,documentation);
          }
          else if(rname.Compare(_T("whitespace")) == 0)
          {
            int ws = 0;
                 if(value.Compare(_T("preserve")) == 0) ws = 1;
            else if(value.Compare(_T("replace"))  == 0) ws = 2;
            else if(value.Compare(_T("collapse")) == 0) ws = 3;
            else
            {
              result = XsdError::XSDE_unknown_simple_type_restriction;
            }
            restrict->AddWhitespace(ws);
          }
          else
          {
            result = XsdError::XSDE_unknown_simple_type_restriction;
          }
          // Next part
          part = p_doc.GetElementSibling(part);
        }
      }
    }
  }
  else
  {
    // Complex type reference
  }
  XMLElement* next = new XMLElement();
  next->SetName(name);
  next->SetType(xmlType);
  next->SetRestriction(restrict);

  // Tell it our caller
  *p_next = next;
  return result;
}

//////////////////////////////////////////////////////////////////////////

// <xs:schema     xmlns = "http://www.standard.nl/instruction/SALES/001"
//             xmlns:xs = "http://www.w3.org/2001/XMLSchema"
//      targetNamespace = "http://www.stanaard.nl/instruction/SALES/001"
//   elementFormDefault = "qualified">

// Set the root
void
XSDSchema::WriteXSDSchemaRoot(XMLMessage& p_doc)
{
  XMLElement* root = p_doc.GetRoot();
  root->SetName(_T("schema"));
  root->SetNamespace(_T("ns"));
  p_doc.SetAttribute(root,_T("xmlns"),m_namespace);
  p_doc.SetAttribute(root,_T("xmlns:xs"),XSDSCHEMA);
  p_doc.SetAttribute(root,_T("targetNamespace"),m_targetNamespace);
  p_doc.SetAttribute(root,_T("elementFormDefault"),m_qualified ? _T("qualified") : _T("unqualified"));
}

// Write all elements
void
XSDSchema::WriteXSDElements(XMLMessage& p_doc,XMLElement* p_base,ElementMap& p_elements)
{
  for(int index = 0;index < (int) p_elements.size();++index)
  {
    XMLElement* elem = p_doc.AddElement(p_base,_T("xs:element"),XDT_String,_T(""));
    p_doc.SetAttribute(elem,_T("name"),p_elements[index]->GetName());
    XMLRestriction* restrict = p_elements[index]->GetRestriction();
    if(restrict)
    {
      p_doc.SetAttribute(elem,_T("type"),restrict->GetName());
    }
    if(restrict->HasMinOccurs() != 1)
    {
      p_doc.SetAttribute(elem,_T("minOccurs"),(int)restrict->HasMinOccurs());
    }
    if(restrict->HasMaxOccurs() != 1)
    {
      if(restrict->HasMaxOccurs() == UINT_MAX)
      {
        p_doc.SetAttribute(elem,_T("maxOccurs"),_T("unbounded"));
      }
      else
      {
        p_doc.SetAttribute(elem,_T("maxOccurs"),(int) restrict->HasMaxOccurs());
      }
    }
    if(FindComplexType(restrict->GetName()) == nullptr)
    {
      if(XMLRestrictionMoreThanBase(restrict))
      {
        // Must be a simple type
        XMLElement* simple = p_doc.AddElement(elem,  _T("xs:simpleType"), XDT_String,_T(""));
        XMLElement* restrt = p_doc.AddElement(simple,_T("xs:restriction"),XDT_String,_T(""));
        p_doc.SetAttribute(restrt,_T("base"),_T("xs:") + restrict->HasBaseType());

        WriteXSDRestrictions(p_doc,restrt,restrict);
      }
    }
  }
}

// Write all complex types
void
XSDSchema::WriteXSDComplexTypes(XMLMessage& p_doc)
{
  for(const auto& comp : m_types)
  {
    XSDComplexType* complex = comp.second;
    XMLElement* type = p_doc.AddElement(nullptr,_T("xs:complexType"),XDT_String,_T(""));
    p_doc.SetAttribute(type,_T("name"),complex->m_name);
    
    XMLElement* order(nullptr);
    switch(complex->m_order)
    {
      case WsdlOrder::WS_All:      order = p_doc.AddElement(type,_T("xs:all"),     XDT_String,_T("")); break;
      case WsdlOrder::WS_Choice:   order = p_doc.AddElement(type,_T("xs:choice"),  XDT_String,_T("")); break;
      case WsdlOrder::WS_Sequence: order = p_doc.AddElement(type,_T("xs:sequence"),XDT_String,_T("")); break;
      default:                     return;
    }
    // Write all parts of the complex type
    WriteXSDElements(p_doc,order,complex->m_elements);
  }
}

bool
XSDSchema::XMLRestrictionMoreThanBase(XMLRestriction* p_restrict)
{
  if(p_restrict->HasLength()          > 0     ||
     p_restrict->HasMaxLength()       > 0     ||
     p_restrict->HasMinLength()       > 0     ||
     p_restrict->HasTotalDigits()     > 0     ||
     p_restrict->HasFractionDigits()  > 0     ||
     p_restrict->HasWhitespace()      > 0     ||
    !p_restrict->HasPattern().IsEmpty()       ||
    !p_restrict->HasMaxExclusive().IsEmpty()  ||
    !p_restrict->HasMaxInclusive().IsEmpty()  ||
    !p_restrict->HasMinExclusive().IsEmpty()  ||
    !p_restrict->HasMinInclusive().IsEmpty()  ||
    !p_restrict->GetEnumerations().empty())
  {
    return true;
  }
  return false;
}

void
XSDSchema::WriteXSDRestrictions(XMLMessage& p_doc,XMLElement* p_elem,XMLRestriction* p_restrict)
{
  XMLElement* extra(nullptr);
  XString empty;

  if(p_restrict->HasLength() > 0)
  {
    extra = p_doc.AddElement(p_elem,_T("xs:length"),XDT_String,empty);
    p_doc.SetAttribute(extra,_T("value"),p_restrict->HasLength());
  }
  if(p_restrict->HasMaxLength() > 0)
  {
    extra = p_doc.AddElement(p_elem,_T("xs:maxLength"),XDT_String,empty);
    p_doc.SetAttribute(extra,_T("value"),p_restrict->HasMaxLength());
  }
  if(p_restrict->HasMinLength() > 0)
  {
    extra = p_doc.AddElement(p_elem,_T("xs:minLength"),XDT_String,empty);
    p_doc.SetAttribute(extra,_T("value"),p_restrict->HasMinLength());
  }
  if(p_restrict->HasTotalDigits())
  {
    extra = p_doc.AddElement(p_elem,_T("xs:totalDigits"),XDT_String,empty);
    p_doc.SetAttribute(extra,_T("value"),p_restrict->HasTotalDigits());
  }
  if(p_restrict->HasFractionDigits())
  {
    extra = p_doc.AddElement(p_elem,_T("xs:fractionDigits"),XDT_String,empty);
    p_doc.SetAttribute(extra,_T("value"),p_restrict->HasFractionDigits());
  }
  if(!p_restrict->HasMaxExclusive().IsEmpty())
  {
    extra = p_doc.AddElement(p_elem,_T("xs:maxExclusive"),XDT_String,empty);
    p_doc.SetAttribute(extra,_T("value"),p_restrict->HasMaxExclusive());
  }
  if(!p_restrict->HasMaxInclusive().IsEmpty())
  {
    extra = p_doc.AddElement(p_elem,_T("xs:maxInclusive"),XDT_String,empty);
    p_doc.SetAttribute(extra,_T("value"),p_restrict->HasMaxInclusive());
  }
  if(!p_restrict->HasMinExclusive().IsEmpty())
  {
    extra = p_doc.AddElement(p_elem,_T("xs:minExclusive"),XDT_String,empty);
    p_doc.SetAttribute(extra,_T("value"),p_restrict->HasMinExclusive());
  }
  if(!p_restrict->HasMinInclusive().IsEmpty())
  {
    extra = p_doc.AddElement(p_elem,_T("xs:minInclusive"),XDT_String,empty);
    p_doc.SetAttribute(extra,_T("value"),p_restrict->HasMinInclusive());
  }
  if(!p_restrict->HasPattern().IsEmpty())
  {
    extra = p_doc.AddElement(p_elem,_T("xs:pattern"),XDT_String,empty);
    p_doc.SetAttribute(extra,_T("value"),p_restrict->HasPattern());
  }
  if(p_restrict->HasWhitespace())
  {
    extra = p_doc.AddElement(p_elem,_T("xs:whiteSpace"),XDT_String,empty);
    // 1=preserve, 2=replace, 3=collapse
    switch(p_restrict->HasWhitespace())
    {
      case 1: p_doc.SetAttribute(extra,_T("value"),_T("preserve")); break;
      case 2: p_doc.SetAttribute(extra,_T("value"),_T("replace"));  break;
      case 3: p_doc.SetAttribute(extra,_T("value"),_T("collapse")); break;
    }
  }
  XmlEnums& enums = p_restrict->GetEnumerations();
  if(!enums.empty())
  {
    for(auto& num : enums)
    {
      extra = p_doc.AddElement(p_elem,_T("xs:enumeration"),XDT_String,empty);
      p_doc.SetAttribute(extra,_T("value"),num.first);
      if(!num.second.IsEmpty())
      {
        XMLElement* anno = p_doc.AddElement(extra,_T("annotation"),XDT_String,empty);
        p_doc.AddElement(anno,_T("documentation"),XDT_String,num.second);
      }
    }
  }
}

//////////////////////////////////////////////////////////////////////////
//
// Helper methods
//
//////////////////////////////////////////////////////////////////////////

// Handle ComplexType objects
XSDComplexType* 
XSDSchema::FindComplexType(XString p_name)
{
  ComplexMap::iterator it = m_types.find(p_name);
  if(it != m_types.end())
  {
    return it->second;
  }
  return nullptr;
}

XSDComplexType*
XSDSchema::AddComplexType(XString p_name)
{
  XSDComplexType* complex = FindComplexType(p_name);
  if(!complex)
  {
    complex = new XSDComplexType(p_name);
    m_types[p_name] = complex;
  }
  return complex;
}

// Helper functions
bool
XSDSchema::StripSchemaNS(XString& p_name)
{
  int pos = p_name.Find(':');
  if(pos < 0)
  {
    return false;
  }
  if(p_name.Left(pos).Compare(m_xs))
  {
    return false;
  }
  p_name = p_name.Mid(pos + 1);
  return true;
}

XMLElement* 
XSDSchema::FindElement(ElementMap& p_map,XString p_name)
{
  for(auto& elem : p_map)
  {
    if(elem->GetName().Compare(p_name) == 0)
    {
      return elem;
    }
  }
  return nullptr;
}

//////////////////////////////////////////////////////////////////////////
//
// VALIDATION
//
//////////////////////////////////////////////////////////////////////////

XsdError
XSDSchema::ValidateElement(XMLMessage& p_doc
                          ,XMLElement* p_compare
                          ,XMLElement* p_schemaElement
                          ,XString&    p_error)
{
  XsdError result = XsdError::XSDE_NoError;

  // 1) Find restriction and data type
  XMLRestriction* rest = p_schemaElement->GetRestriction();
  XString  type = rest->GetName();

  // 2) Check for qualified element name
  //   if(m_qualified && p_compare->GetNamespace().IsEmpty())
  //   {
  //     p_error = "Unqualified element: " + p_compare->GetName();
  //     return XsdError::XSDE_element_not_qualified;
  //   }

  // 3) Complex type -> more steps -> Find XSDComplexType
  XSDComplexType* complex = FindComplexType(type);

  // 4) Simple type -> Validate the element value
  if(complex == nullptr || p_schemaElement->GetType() > 0)
  {
    return ValidateValue(rest,p_compare,p_schemaElement->GetType(),type,p_error);
  }

  // 5) Validate Wsdl Order in the XSDComplexType
  result = ValidateOrder(p_doc,p_compare,complex,p_error);
  if(result != XsdError::XSDE_NoError)
  {
    return result;
  }

  // 6) Validate Elements (recursive calling ourselves)
  XMLElement* child = p_doc.GetElementFirstChild(p_compare);
  while(child)
  {
    XMLElement* valAgainst = FindElement(complex->m_elements,child->GetName());
    if(valAgainst == nullptr)
    {
      XsdError::XSDE_Element_not_in_xsd;
      p_error.AppendFormat(_T("Unknown element [%s] in: %s"),child->GetName().GetString(),complex->m_name.GetString());
      break;
    }
    result = ValidateElement(p_doc,child,valAgainst,p_error);
    if(result != XsdError::XSDE_NoError)
    {
      break;
    }
    // Getting next element to validate
    child = p_doc.GetElementSibling(child);
  }
  // Come to the end
  return result;
}

XsdError  
XSDSchema::ValidateOrder(XMLMessage&      p_doc
                        ,XMLElement*      p_compare
                        ,XSDComplexType*  p_complex
                        ,XString&         p_error)
{
  XsdError result = XsdError::XSDE_NoError;
  switch(p_complex->m_order)
  {
    case WsdlOrder::WS_Sequence:  result = ValidateOrderSequence(p_doc,p_compare,p_complex->m_elements,p_error);
                                  break;
    case WsdlOrder::WS_Choice:    result = ValidateOrderChoice  (p_doc,p_compare,p_complex->m_elements,p_error);
                                  break;
    case WsdlOrder::WS_All:       result = ValidateOrderAll     (p_doc,p_compare,p_complex->m_elements,p_error);
                                  break;
    default:                      XsdError::XSDE_Unknown_ComplexType_ordering;
                                  p_error += _T("Unknown ComplexType ordering found (NOT sequence,choice,all)");
                                  break;
  }
  return result;
}

XsdError 
XSDSchema::ValidateOrderSequence(XMLMessage& p_doc,XMLElement* p_compare,ElementMap& p_elements,XString& p_error)
{
  // Elements must occur in the same order, or must have "minOccur = 0"
  size_t   posXSD = 0;
  unsigned occurs = 0;

  XMLElement* elementXML = p_doc.GetElementFirstChild(p_compare);
  while(elementXML)
  {
    if(posXSD >= p_elements.size())
    {
      // Element not found in definition
      p_error = _T("Element not found in XSD type definition: ") + elementXML->GetName();
      return XsdError::XSDE_Element_not_in_xsd;
    }
    XMLElement* elementXSD = p_elements[posXSD];
    XMLRestriction* restrict = elementXSD->GetRestriction();

    if(elementXML->GetName().Compare(elementXSD->GetName()) == 0)
    {
      // Found: check maxOccurs
      if(++occurs > (restrict ? restrict->HasMaxOccurs() : 1))
      {
        p_error = _T("Extra element in message: ") + elementXSD->GetName();
        return XsdError::XSDE_Extra_elements_in_xml;
      }
    }
    else
    {
      // Not found: check for unknown element
      if(occurs == 0 && (!restrict || restrict->HasMinOccurs() > 0))
      {
        // Element not found in definition
        p_error = _T("Element not found in XSD type definition: ") + elementXML->GetName();
        return XsdError::XSDE_Element_not_in_xsd;
      }

      // Not found: check minOccurs
      if(occurs < (restrict ? restrict->HasMinOccurs() : 1))
      {
        p_error = _T("Missing element in message: ") + elementXSD->GetName();
        return XsdError::XSDE_Missing_element_in_xml;
      }

      // Next XSD
      ++posXSD;
      occurs = 0;
      continue;
    }
    // Next element to check
    elementXML = p_doc.GetElementSibling(elementXML);
  }

  // Check uncompleted XSD elements
  for(; posXSD < p_elements.size(); ++posXSD,occurs = 0)
  {
    XMLElement* elementXSD = p_elements[posXSD];
    XMLRestriction* restrict = elementXSD->GetRestriction();

    if(occurs < (restrict ? restrict->HasMinOccurs() : 1))
    {
      // Element not found in xml
      p_error = _T("Element not found in xml: ") + elementXSD->GetName();
      return XsdError::XSDE_Missing_element_in_xml;
    }
  }
  return XsdError::XSDE_NoError;
}

XsdError 
XSDSchema::ValidateOrderChoice(XMLMessage& p_doc,XMLElement* p_compare,ElementMap& p_elements,XString& p_error)
{
  XsdError result = XsdError::XSDE_NoError;

  XMLElement* found(nullptr);
  XMLElement* elem = p_doc.GetElementFirstChild(p_compare);
  if(elem)
  {
    found = FindElement(p_elements,elem->GetName());
  }
  if(found == nullptr)
  {
    p_error = _T("Element by <choice> not defined in complex type: ") + p_compare->GetName();
    result  = XsdError::XSDE_Missing_element_in_xml;
  }
  else if(p_doc.GetElementSibling(p_compare))
  {
    p_error = _T("<choice> selection can have only ONE (1) element");
    result  = XsdError::XSDE_only_one_choice_element;
  }
  return result;
}

XsdError 
XSDSchema::ValidateOrderAll(XMLMessage& p_doc,XMLElement* p_compare,ElementMap& p_elements,XString& p_error)
{
  XsdError result = XsdError::XSDE_NoError;

  XMLElement* elem = p_doc.GetElementFirstChild(p_compare);
  while(elem)
  {
    const XMLElement* found = FindElement(p_elements,elem->GetName());
    if(found == nullptr)
    {
      p_error = _T("Element not found in type definition: ") + elem->GetName();
      result  = XsdError::XSDE_Missing_element_in_xml;
      break;
    }
  }
  return result;
}

XsdError
XSDSchema::ValidateValue(XMLRestriction*  p_restrict
                        ,XMLElement*      p_compare
                        ,XmlDataType      p_datatype
                        ,XString          p_type
                        ,XString&         p_error)
{
  XsdError result = XsdError::XSDE_NoError;

  XmlDataType basetype = p_datatype;
  if(!basetype && !p_type.IsEmpty())
  {
    basetype = StringToXmlDataType(p_type);
  }
  else
  {
    basetype = StringToXmlDataType(p_restrict->HasBaseType());
  }

  // Check datatype contents
  XString errors = p_restrict->CheckDatatype(basetype,p_compare->GetValue());
  if(!errors.IsEmpty())
  {
    result   = XsdError::XSDE_base_datatype_violation;
    p_error += _T("Field: ") + p_compare->GetName();
    p_error += _T(" : ") + errors;
  }

  // Check extra restrictions on datatype value
  errors = p_restrict->CheckRestriction(basetype,p_compare->GetValue());
  if(!errors.IsEmpty())
  {
    result   = XsdError::XSDE_datatype_restriction_violation;
    p_error += _T("Field: ") + p_compare->GetName();
    p_error += _T(" : ") + errors;
  }

  return result;
}

