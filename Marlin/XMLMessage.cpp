/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: XMLMessage.cpp
//
// Marlin Server: Internet server/client
// 
// Copyright (c) 2017 ir. W.E. Huisman
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
#include "stdafx.h"
#include "XMLMessage.h"
#include "XMLParser.h"
#include "XMLRestriction.h"
#include "JSONMessage.h"
#include "Namespace.h"
#include "ConvertWideString.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#pragma region XMLElement

//////////////////////////////////////////////////////////////////////////
//
// XMLElement
//
//////////////////////////////////////////////////////////////////////////

XMLElement::XMLElement()
{
}

XMLElement::XMLElement(const XMLElement& source)
           :m_namespace  (source.m_namespace)
           ,m_name       (source.m_name)
           ,m_type       (source.m_type)
           ,m_value      (source.m_value)
           ,m_attributes (source.m_attributes)
           ,m_restriction(source.m_restriction)
           ,m_parent     (nullptr)
{
  for(unsigned ind = 0; ind < source.m_elements.size(); ++ind)
  {
    XMLElement* param = new XMLElement(*source.m_elements[ind]);
    param->m_parent   = this;
    m_elements.push_back(param);
  }
}

XMLElement::~XMLElement()
{
  Reset();
}

void
XMLElement::Reset()
{
  // Empty all data
  m_namespace.Empty();
  m_value.Empty();
  m_name.Empty();
  m_type = 0;

  // Remove all attributes
  m_attributes.clear();
  // Remove all dependent elements
  for(unsigned ind = 0; ind < m_elements.size(); ++ind)
  {
    delete m_elements[ind];
  }
  m_elements.clear();
  // No more restrictions
  m_restriction = nullptr;
}

#pragma endregion XMLElement

#pragma region XMLMessage_XTOR

//////////////////////////////////////////////////////////////////////////
//
// XMLMessage from here
//
//////////////////////////////////////////////////////////////////////////

// XTOR XML Message
XMLMessage::XMLMessage()
{
  m_root.m_type   = XDT_String;
  m_root.m_parent = nullptr;
}

// XTOR from another message
XMLMessage::XMLMessage(XMLMessage* p_orig)
           :m_root(p_orig->m_root)
{
  m_version             = p_orig->m_version;
  m_encoding            = p_orig->m_encoding;
  m_standalone          = p_orig->m_standalone;
  m_condensed           = p_orig->m_condensed;
  m_whitespace          = p_orig->m_whitespace;
  m_sendBOM             = p_orig->m_sendBOM;
  m_internalError       = p_orig->m_internalError;
  m_internalErrorString = p_orig->m_internalErrorString;
}

XMLMessage::~XMLMessage()
{
  Reset();
}

void
XMLMessage::Reset()
{
  m_root.Reset();
}

#pragma endregion XMLMessage_XTOR

#pragma region File_Load_Save

//////////////////////////////////////////////////////////////////////////
//
// FILES Loading and Saving from/to file
//
//////////////////////////////////////////////////////////////////////////

// Load from file
bool
XMLMessage::LoadFile(const CString& p_fileName)
{
  FILE* file = nullptr;
  if(fopen_s(&file,p_fileName,"rb") == 0 && file)
  {
    // Find the length of a file
    fseek(file,0,SEEK_END);
    unsigned long length = ftell(file);
    fseek(file,0,SEEK_SET);

    // Check for the streaming limit
    if(length > g_streaming_limit)
    {
      fclose(file);
      return false;
    }

    // Prepare buffer
    // CString buffers are allocated on the heap
    // so shut up the warning about stack overflow
    CString inhoud;
    char* buffer = inhoud.GetBufferSetLength(length + 1);

    // Read the buffer
    if(fread(buffer,1,length,file) < length)
    {
      fclose(file);
      return false;
    }
    buffer[length] = 0;

    // Buffer unlock
    inhoud.ReleaseBuffer(length);

    // Close the file
    if(fclose(file))
    {
      return false;
    }

    // And parse it
    ParseMessage(inhoud);
    return true;
  }
  return false;
}

// Load from file in pre-known encoding
bool
XMLMessage::LoadFile(const CString& p_fileName,XMLEncoding p_encoding)
{
  m_encoding = p_encoding;
  return LoadFile(p_fileName);
}

// Save to file
bool
XMLMessage::SaveFile(const CString& p_fileName)
{
  bool result = false;
  FILE* file = nullptr;
  if(fopen_s(&file,p_fileName,"w") == 0 && file)
  {
    CString inhoud = Print();
    if(fwrite(inhoud.GetString(),inhoud.GetLength(),1,file) == 1)
    {
      result = true;
    }
    // Close and flush the file
    fclose(file);
  }
  return result;
}

// Save to file in pre-known encoding
bool
XMLMessage::SaveFile(const CString& p_fileName,XMLEncoding p_encoding)
{
  m_encoding = p_encoding;
  return SaveFile(p_fileName);
}

#pragma endregion File_Load_Save

#pragma region Parsing_and_printing

//////////////////////////////////////////////////////////////////////////
//
// Parsing and Printing the XML
//
//////////////////////////////////////////////////////////////////////////

// Parse incoming message to members
void
XMLMessage::ParseMessage(CString& p_message,WhiteSpace p_whiteSpace /* = PRESERVE_WHITESPACE*/)
{
  XMLParser parser(this);
  parser.ParseMessage(p_message,p_whiteSpace);
  m_whitespace = (p_whiteSpace == WhiteSpace::COLLAPSE_WHITESPACE);
}

void
XMLMessage::ParseMessage(XMLParser* p_parser,CString& p_message,WhiteSpace p_whiteSpace /*=PRESERVE_WHITESPACE*/)
{
  if(p_parser)
  {
    p_parser->ParseMessage(p_message,p_whiteSpace);
    m_whitespace = (p_whiteSpace == WhiteSpace::COLLAPSE_WHITESPACE);
  }
  else
  {
    m_internalError = XmlError::XE_UnknownXMLParser;
    m_internalErrorString = "No registered XML parser for this message";
  }
}

// Parse from a beginning node
void    
XMLMessage::ParseForNode(XMLElement* p_node,CString& p_message,WhiteSpace p_whiteSpace /* = PRESERVE_WHITESPACE*/)
{
  XMLParser parser(this);
  parser.ParseForNode(p_node,p_message,p_whiteSpace);
  m_whitespace = (p_whiteSpace == WhiteSpace::COLLAPSE_WHITESPACE);
}

// Print the XML again
CString
XMLMessage::Print()
{
  bool utf8 = (m_encoding == XMLEncoding::ENC_UTF8);

  CString message = PrintHeader();
  message += PrintElements(&m_root,utf8);
  if(m_condensed)
  {
    message += "\n";
  }
  return message;
}

CString
XMLMessage::PrintHeader()
{
  CString temp;
  CString header("<?xml");

  // Check what we should do
  if(m_version.IsEmpty() && m_encoding == XMLEncoding::ENC_Plain && m_standalone.IsEmpty())
  {
    return temp;
  }

  // Construct the header
  if(!m_version.IsEmpty())
  {
    temp.Format(" version=\"%s\"",m_version);
    header += temp;
  }
  // Take care of character encoding
  int acp = -1;
  switch(m_encoding)
  {
    case XMLEncoding::ENC_Plain:   acp =    -1; break; // Find Active Code Page
    case XMLEncoding::ENC_UTF8:    acp = 65001; break; // See ConvertWideString.cpp
    case XMLEncoding::ENC_UTF16:   acp =  1200; break; // See ConvertWideString.cpp
    case XMLEncoding::ENC_ISO88591:acp = 28591; break; // See ConvertWideString.cpp
    default:          break;
  }
  header.AppendFormat(" encoding=\"%s\"",CodepageToCharset(acp));

  // Add standalone?
  if(!m_standalone.IsEmpty())
  {
    temp.Format(" standalone=\"%s\"",m_standalone);
    header += temp;
  }
  // Closing of the header
  header += "?>";
  if(m_condensed == false)
  {
    header += "\n";
  }
  return header;
}

// Print the elements stack
CString
XMLMessage::PrintElements(XMLElement* p_element
                         ,bool        p_utf8  /*=true*/
                         ,int         p_level /*=0*/)
{
  CString temp;
  CString spaces;
  CString newline;
  CString message;

  // Find indentation
  if(m_condensed == false)
  {
    newline = "\n";
    for(int ind = 0;ind < p_level; ++ind)
    {
      spaces += "  ";
    }
  }

  CString namesp = p_element->m_namespace;
  CString name   = p_element->m_name;
  CString value  = p_element->m_value;

  // Check namespace
  if(!namesp.IsEmpty())
  {
    name = namesp + ":" + name;
  }

  // Print domain value restriction of the element
  if(m_printRestiction && p_element->m_restriction)
  {
    temp     = p_element->m_restriction->PrintRestriction(name);
    message += spaces + temp + newline;
  }
  if((p_element->m_type & WSDL_Mask) & ~(WSDL_Mandatory | WSDL_Sequence))
  {
    message += spaces + PrintWSDLComment(p_element) + newline;
  }

  // Print by type
  if(p_element->m_type & XDT_CDATA)
  {
    // CDATA section
    temp.Format("<%s><![CDATA[%s]]>",PrintXmlString(name,p_utf8),value);
    message += spaces + temp;
  }
  if(p_element->m_type & XDT_Complex)
  {
    // Other XML data 
    temp.Format("<%s>%s",PrintXmlString(name,p_utf8),value);
    message += spaces + temp;
  }
  else if(value.IsEmpty() && p_element->m_attributes.size() == 0 && p_element->m_elements.size() == 0)
  {
    // A 'real' empty node
    temp.Format("<%s />%s",PrintXmlString(name,p_utf8),newline);
    message += spaces + temp;
    return message;
  }
  else if(p_element->m_attributes.size() == 0)
  {
    // Exact string with escapes
    temp.Format("<%s>%s",PrintXmlString(name,p_utf8),PrintXmlString(value,p_utf8));
    message += spaces + temp;
  }
  else
  {
    // Parameter printing with attributes
    temp.Format("<%s",PrintXmlString(name,p_utf8));
    message += spaces + temp;
    for(auto& attrib : p_element->m_attributes)
    {
      CString attribute = attrib.m_name;

      // Append attribute name
      if(attrib.m_namespace.IsEmpty())
      {
        temp.Format(" %s=",PrintXmlString(attribute,p_utf8));
      }
      else
      {
        temp.Format(" %s:%s=",attrib.m_namespace,PrintXmlString(attribute,p_utf8));
      }
      message += temp;

      switch(attrib.m_type & XDT_Mask)
      {
        default:                    temp.Format("\"%s\"",attrib.m_value);
                                    break;
        case XDT_String:            // Fall through
        case XDT_AnyURI:            // Fall through
        case XDT_NormalizedString:  temp.Format("\"%s\"",PrintXmlString(attrib.m_value,p_utf8)); 
                                    break;
      }
      message += temp;
    }
    if(value.IsEmpty() && p_element->m_elements.empty())
    {
      message += "/>" + newline;
      return message;
    }
    else
    {
      // Write value and end of the key
      temp.Format(">%s",PrintXmlString(value,p_utf8));
      message += temp;
    }
  }

  if(p_element->m_elements.size())
  {
    message += newline;
    // call recursively
    for(auto element : p_element->m_elements)
    {
      message += PrintElements(element,p_utf8,p_level + 1);
    }
    message += spaces;
  }
  // Write ending of parameter name
  temp.Format("</%s>%s",PrintXmlString(name,p_utf8),newline);
  message += temp;

  return message;
}

CString
XMLMessage::PrintWSDLComment(XMLElement* p_element)
{
  CString comment("<!--");
  switch(p_element->GetType() & WSDL_MaskField)
  {
    case WSDL_Mandatory: comment += "Mandatory";    break;
    case WSDL_Optional:  // Fall through
    case WSDL_ZeroOne:   comment += "Optional";     break;
    case WSDL_OnceOnly:  comment += "Exactly ONE";  break;
    case WSDL_ZeroMany:  comment += "ZERO or more"; break;
    case WSDL_OneMany:   comment += "At LEAST one"; break;
  }
  switch(p_element->GetType() & WSDL_MaskOrder)
  {
    case WSDL_Choice:    comment += ": You have a CHOICE at this level"; break;
    case WSDL_Sequence:  comment += ": Exactly in this SEQUENCE";        break;
  }
  comment += ":-->";

  return comment;
}

CString 
XMLMessage::PrintJson(bool p_attributes)
{
  bool utf8 = (m_encoding == XMLEncoding::ENC_UTF8);

  CString message = PrintElementsJson(&m_root,p_attributes,utf8);
  if(m_condensed)
  {
    message += "\n";
  }
  return message;
}

// Print the elements stack
CString
XMLMessage::PrintElementsJson(XMLElement* p_element
                             ,bool        p_attributes
                             ,bool        p_utf8  /*=true*/
                             ,int         p_level /*=0*/)
{
  CString temp;
  CString spaces;
  CString newline;
  CString message;

  // Find indentation
  if(m_condensed == false)
  {
    newline = "\n";
    for(int ind = 0; ind < p_level; ++ind)
    {
      spaces += "  ";
    }
  }

  CString name  = p_element->m_name;
  CString value = p_element->m_value;

  if(!name.IsEmpty())
  {
    message = spaces + "\""+ name + "\": ";
  }

  // Optional attributes group
  if(p_attributes && !p_element->m_attributes.empty())
  {
    message += "{" + newline;

    for(auto& attrib : p_element->m_attributes)
    {
      CString attrName  = attrib.m_name;
      CString attrValue = attrib.m_value;
      temp.Format("\"@%s\": \"%s'\"",attrName,attrValue);
      message = spaces + temp + newline;
    }
    message += spaces + "\"#text\": ";
  }

  // print element value
  switch(p_element->m_type & XDT_Mask)
  {
    default:                    temp.Format("%s",value);
                                break;
    case XDT_CDATA:             // Fall through
    case XDT_String:            // Fall through
    case XDT_AnyURI:            // Fall through
    case XDT_NormalizedString:  temp = JSONvalue::FormatAsJsonString(value,p_utf8);
                                break;
  }
  message += temp + newline;

  // Closing of the attributes group
  if(p_attributes && !p_element->m_attributes.empty())
  {
    message += spaces + "}" + newline;
  }

  // Print all child elements of this one by recursing
  if(!p_element->m_elements.empty())
  {
    message += spaces + "{" + newline;
    for(auto& elem : p_element->m_elements)
    {
      PrintElementsJson(elem,p_attributes,p_utf8,p_level + 1);
    }
    message += spaces + "}" + newline;
  }

  return message;
}

#pragma endregion Parsing

#pragma region Setting_Elements

//////////////////////////////////////////////////////////////////////////
//
// SETTING ELEMENTS AND ATTRIBUTES
//
//////////////////////////////////////////////////////////////////////////

// General add a parameter (always adds, so multiple parameters of same name can be added)
XMLElement*
XMLMessage::AddElement(XMLElement* p_base,CString p_name,XmlDataType p_type,CString p_value,bool p_front /*=false*/)
{
  // Removing the namespace from the name
  CString namesp = SplitNamespace(p_name);

  // Check for stupid programmers
  if(p_name.Find(' ') > 0)
  {
    throw CString("XML Messages with spaces in elementnames are invalid!");
  }

  XmlElementMap& elements = p_base ? p_base->m_elements : m_root.m_elements;
  XMLElement* parent = p_base ? p_base : &m_root;
  XMLElement* elem   = new XMLElement();
  elem->m_namespace  = namesp;
  elem->m_parent     = parent;
  elem->m_name       = p_name;
  elem->m_type       = p_type;
  elem->m_value      = p_value;

  if(p_front)
  {
    elements.push_front(elem);
  }
  else
  {
    elements.push_back(elem);
  }
  return elem;
}

// Setting an element: Finding an existing node
XMLElement*
XMLMessage::SetElement(XMLElement* p_base,CString p_name,XmlDataType p_type,CString p_value,bool p_front /*=false*/)
{
  CString name(p_name);
  CString namesp = SplitNamespace(name);
  XmlElementMap& elements = p_base ? p_base->m_elements : m_root.m_elements;

  // Finding existing element
  for (auto& element : elements)
  {
    if (element->m_name.Compare(name) == 0)
    {
      // Just setting the values again
      element->m_namespace = namesp;
      element->m_name      = name;
      element->m_type      = p_type;
      element->m_value     = p_value;
      return element;
    }
  }
  
  // Create a new node
  return AddElement(p_base,p_name,p_type,p_value,p_front);
}

XMLElement*
XMLMessage::SetElement(CString p_name,CString& p_value)
{
  return SetElement(&m_root,p_name,XDT_String,p_value);
}

XMLElement*
XMLMessage::SetElement(CString p_name, const char* p_value)
{
  CString value(p_value);
  return SetElement(&m_root,p_name,XDT_String,value);
}

XMLElement*
XMLMessage::SetElement(CString p_name,int p_value)
{
  CString value;
  value.Format("%d",p_value);
  return SetElement(&m_root,p_name,XDT_Integer,value);
}

XMLElement*
XMLMessage::SetElement(CString p_name,bool p_value)
{
  CString value;
  value.Format("%s",p_value ? "true" : "false");
  return SetElement(&m_root,p_name,XDT_Boolean,value);
}

XMLElement*
XMLMessage::SetElement(CString p_name,double p_value)
{
  CString value;
  value.Format("%G",p_value);
  return SetElement(&m_root,p_name,XDT_Double,value);
}

XMLElement*
XMLMessage::SetElement(XMLElement* p_base,CString p_name,CString& p_value)
{
  return SetElement(p_base,p_name,XDT_String,p_value);
}

XMLElement*
XMLMessage::SetElement(XMLElement* p_base,CString p_name,const char* p_value)
{
  CString value(p_value);
  return SetElement(p_base,p_name,XDT_String,p_value);
}

XMLElement*
XMLMessage::SetElement(XMLElement* p_base,CString p_name,int p_value)
{
  CString value;
  value.Format("%d", p_value);
  return SetElement(p_base,p_name,XDT_Integer,value);
}

XMLElement*
XMLMessage::SetElement(XMLElement* p_base,CString p_name,bool p_value)
{
  CString value;
  value.Format("%s",p_value ? "yes" : "no");
  return SetElement(p_base,p_name,XDT_Boolean,value);
}

XMLElement*
XMLMessage::SetElement(XMLElement* p_base,CString p_name,double p_value)
{
  CString value;
  value.Format("%G",p_value);
  return SetElement(p_base,p_name,XDT_Double,value);
}

void
XMLMessage::SetElementValue(XMLElement* p_elem,XmlDataType p_type,CString p_value)
{
  if(p_elem)
  {
    p_elem->m_type  = p_type;
    p_elem->m_value = p_value;
  }
}

void
XMLMessage::SetElementOptions(XMLElement* p_elem,XmlDataType p_options)
{
  if(p_elem)
  {
    // Mask-off previous WSDL options
    // All need to be set in one go!!
    p_elem->m_type = (p_elem->m_type & XDT_Mask) | (p_options & WSDL_Mask);
  }
}

// Setting the namespace' name and contract of an element
// Beware that a contract never ends on a '/' or a '\\'
void
XMLMessage::SetElementNamespace(XMLElement* p_elem, CString p_namespace, CString p_contract,bool p_trailingSlash /*=false*/)
{
  CString name = "xmlns";
  if (!p_namespace.IsEmpty())
  {
    name += ":";
    name += p_namespace;
  }
  if(p_trailingSlash == false)
  {
    p_contract.TrimRight('/');
    p_contract.TrimRight('\\');
  }
  SetAttribute(p_elem,name,p_contract);
}

// Set attribute of an element
XMLAttribute*
XMLMessage::SetAttribute(XMLElement* p_elem,CString p_name,CString& p_value)
{
  if(p_elem == NULL)
  {
    return NULL;
  }
  CString namesp = SplitNamespace(p_name);

  // Find in attribute map
  for(auto& attrib : p_elem->m_attributes)
  {
    if(attrib.m_name.Compare(p_name) == 0)
    {
      // Already there, register new value
      attrib.m_type  = XDT_String;
      attrib.m_value = p_value;
      return &(attrib);
    }
  }
  // New attribute
  XMLAttribute attrib;
  attrib.m_type      = XDT_String;
  attrib.m_namespace = namesp;
  attrib.m_name      = p_name;
  attrib.m_value     = p_value;

  p_elem->m_attributes.push_back(attrib);
  return &(p_elem->m_attributes.back());
}

// Set attribute of a element
XMLAttribute*
XMLMessage::SetAttribute(XMLElement* p_elem,CString p_name,const char* p_value)
{
  CString value(p_value);
  return SetAttribute(p_elem,p_name,value);
}

// Set attribute of a element (integer)
XMLAttribute*
XMLMessage::SetAttribute(XMLElement* p_elem,CString p_name,int p_value)
{
  CString value;
  value.Format("%d",p_value);
  XMLAttribute* attrib = SetAttribute(p_elem,p_name,value);
  attrib->m_type = XDT_Integer;
  return attrib;
}

// Set attribute of a element (boolean)
XMLAttribute*
XMLMessage::SetAttribute(XMLElement* p_elem,CString p_name,bool p_value)
{
  CString value;
  value.Format("%s",p_value ? "true" : "false");
  XMLAttribute* attrib = SetAttribute(p_elem,p_name,value);
  attrib->m_type = XDT_Boolean;
  return attrib;
}

// Set attribute of a element (double)
XMLAttribute*
XMLMessage::SetAttribute(XMLElement* p_elem,CString p_name,double p_value)
{
  CString value;
  value.Format("%G",p_value);
  XMLAttribute* attrib = SetAttribute(p_elem,p_name,value);
  attrib->m_type = XDT_Double;
  return attrib;
}

#pragma endregion Setting_Elements

#pragma region Getting_Elements

//////////////////////////////////////////////////////////////////////////
//
// GETTING THE INFO FROM ELEMENTS AND ATTRIBUTES
//
//////////////////////////////////////////////////////////////////////////

// Get an element in a format
CString  
XMLMessage::GetElement(XMLElement* p_elem,CString p_name)
{
  XmlElementMap& map = p_elem ? p_elem->m_elements : m_root.m_elements;

  // Find in the current mapping
  for(unsigned ind = 0; ind < map.size(); ++ind)
  {
    if(map[ind]->m_name.Compare(p_name) == 0)
    {
      return map[ind]->GetValue();
    }
  }
  return "";
}

int      
XMLMessage::GetElementInteger(XMLElement* p_elem,CString p_name)
{
  return atoi(GetElement(p_elem,p_name));
}

bool
XMLMessage::GetElementBoolean(XMLElement* p_elem,CString p_name)
{
  CString value = GetElement(p_elem,p_name);
  if(value.CompareNoCase("true") == 0)
  {
    return true;
  }
  if(atoi(value) > 0)
  {
    return true;
  }
  return false;
}

double
XMLMessage::GetElementDouble(XMLElement* p_elem,CString p_name)
{
  return atof(GetElement(p_elem,p_name));
}

CString
XMLMessage::GetElement(CString p_name)
{
  return GetElement(NULL,p_name);
}

int
XMLMessage::GetElementInteger(CString p_name)
{
  return GetElementInteger(NULL,p_name);
}

bool
XMLMessage::GetElementBoolean(CString p_name)
{
  return GetElementBoolean(NULL,p_name);
}

double
XMLMessage::GetElementDouble(CString p_name)
{
  return GetElementDouble(NULL,p_name);
}

XMLElement*
XMLMessage::GetElementFirstChild(XMLElement* p_elem)
{
  if(p_elem)
  {
    if(p_elem->m_elements.size())
    {
      return p_elem->m_elements.front();
    }
  }
  return nullptr;
}

XMLElement*
XMLMessage::GetElementSibling(XMLElement* p_elem)
{
  if(p_elem)
  {
    XMLElement* parent = p_elem->m_parent;
    if(parent)
    {
      for(unsigned ind = 0;ind < parent->m_elements.size(); ++ind)
      {
        if(parent->m_elements[ind] == p_elem)
        {
          // Check for last node
          if(ind < parent->m_elements.size() - 1)
          {
            return parent->m_elements[ind + 1];
          }
        }
      }
    }
  }
  return nullptr;
}

XMLElement*
XMLMessage::FindElement(XMLElement* p_base, CString p_name,bool p_recurse /*=true*/)
{
  CString elementName(p_name);
  CString namesp = SplitNamespace(elementName);
  XMLElement* base = p_base ? p_base : &m_root;

  for(auto& element : base->m_elements)
  {
    if(element->m_name.Compare(elementName) == 0)
    {
      if(namesp.IsEmpty() || element->m_namespace.Compare(namesp) == 0)
      {
        return element;
      }
    }
  }
  if(p_recurse)
  {
    for(auto& element : base->m_elements)
    {
      XMLElement* elem = FindElement(element,p_name,p_recurse);
      if(elem)
      {
        return elem;
      }
    }
  }
  return nullptr;
}

XMLElement*
XMLMessage::FindElement(CString p_name,bool p_recurse /*=true*/)
{
  return FindElement(&m_root,p_name,p_recurse);
}

XMLElement*
XMLMessage::FindElementWithAttribute(XMLElement* p_base
                                    ,CString     p_elementName
                                    ,CString     p_attribName
                                    ,CString     p_attribValue /*=""*/
                                    ,bool        p_recurse /*=true*/)
{
  CString namesp = SplitNamespace(p_elementName);
  XMLElement* base = p_base ? p_base : &m_root;

  for(auto& element : base->m_elements)
  {
    if(element->m_name.Compare(p_elementName) == 0)
    {
      if(namesp.IsEmpty() || element->m_namespace.Compare(namesp) == 0)
      {
        XMLAttribute* attrib = FindAttribute(element,p_attribName);
        if(attrib)
        {
          if(p_attribValue.IsEmpty() || p_attribValue.CompareNoCase(attrib->m_value) == 0)
          {
            return element;
          }
        }
      }
    }
    if(p_recurse)
    {
      XMLElement* elem = FindElementWithAttribute(element,p_elementName,p_attribName,p_attribValue,p_recurse);
      if(elem)
      {
        return elem;
      }
    }
  }
  return nullptr;
}

// Get the attribute 
CString
XMLMessage::GetAttribute(XMLElement* p_elem,CString p_attribName)
{
  if(p_elem)
  {
    for(auto& attrib : p_elem->m_attributes)
    {
      if(attrib.m_name.Compare(p_attribName) == 0)
      {
        return attrib.m_value;
      }
    }
  }
  return "";
}

// Get the integer attribute
int
XMLMessage::GetAttributeInteger(XMLElement* p_elem,CString p_attribName)
{
  return atoi(GetAttribute(p_elem,p_attribName));
}

// Get the boolean attribute
bool     
XMLMessage::GetAttributeBoolean(XMLElement* p_elem,CString p_attribName)
{
  CString value = GetAttribute(p_elem,p_attribName);
  if(value.CompareNoCase("true") == 0)
  {
    return true;
  }
  if(atoi(value) > 0)
  {
    // Fallback for integer booleans
    return true;
  }
  return false;
}

// Get the double attribute
double
XMLMessage::GetAttributeDouble(XMLElement* p_elem,CString p_attribName)
{
  return atof(GetAttribute(p_elem,p_attribName));
}

XMLAttribute*
XMLMessage::FindAttribute(XMLElement* p_elem,CString p_attribName)
{
  for(auto& attrib : p_elem->m_attributes)
  {
    if(attrib.m_name.Compare(p_attribName) == 0)
    {
      return &attrib;
    }
  }
  return nullptr;
}

#pragma endregion Getting_Elements

#pragma region Operations

//////////////////////////////////////////////////////////////////////////
//
// EXTRA OPERATIONS ON ELEMENTS / HELPER FUNCTIONS
//
//////////////////////////////////////////////////////////////////////////

// Delete parameter with this name (possibly the nth instance of this name)
bool
XMLMessage::DeleteElement(XMLElement* p_base, CString p_name, int p_instance /*= 0*/)
{
  XmlElementMap& map = p_base ? p_base->m_elements : m_root.m_elements;
  XmlElementMap::iterator it = map.begin();
  int  count = 0;

  do
  {
    bool found = false;
    while(it != map.end())
    {
      if((*it)->m_name.Compare(p_name) == 0)
      {
        ++count;
        found = true;
        break;
      }
      // Next instance
      ++it;
    }
    if(found && (count > p_instance))
    {
      delete *it;
      map.erase(it);
      return true;
    }
  } 
  while(count <= p_instance);

  // Instance not found
  return false;
}

// Delete exactly this parameter
bool
XMLMessage::DeleteElement(XMLElement* p_base,XMLElement* p_element)
{
  XmlElementMap& map = p_base ? p_base->m_elements : m_root.m_elements;
  XmlElementMap::iterator it;

  for (it = map.begin(); it != map.end(); ++it)
  {
    if ((*it) == p_element)
    {
      delete p_element;
      map.erase(it);
      return true;
    }
  }
  return false;
}

// Delete exactly this attribute
bool
XMLMessage::DeleteAttribute(XMLElement* p_element,CString p_attribName)
{
  if(p_element == nullptr)
  {
    return false;
  }
  XmlAttribMap::iterator it;
  for(it = p_element->m_attributes.begin();it != p_element->m_attributes.end(); ++it)
  {
    if(it->m_name.Compare(p_attribName) == 0)
    {
      p_element->m_attributes.erase(it);
      return true;
    }
  }
  return false;
}

// Find an element by attribute
// Attribute values must be unique for this method to work
// Preferable: attribute = 'Id', value = unique-identifier-or-number
XMLElement*
XMLMessage::FindElementByAttribute(CString p_attribute, CString p_value)
{
  return FindElementByAttribute(&m_root, p_attribute, p_value);
}

XMLElement*
XMLMessage::FindElementByAttribute(XMLElement* p_element, CString p_attribute, CString p_value)
{
  if (p_element)
  {
    CString value = GetAttribute(p_element, p_attribute);
    if (value.CompareNoCase(p_value) == 0)
    {
      return p_element;
    }
    // Find recursive depth-first
    for (auto& element : p_element->m_elements)
    {
      XMLElement* elem = FindElementByAttribute(element, p_attribute, p_value);
      if (elem)
      {
        return elem;
      }
    }
  }
  return nullptr;
}

void
XMLMessage::SetEncoding(XMLEncoding p_encoding)
{
  m_encoding    = p_encoding;
  m_sendUnicode = (p_encoding == XMLEncoding::ENC_UTF16);
}

// Set sending in Unicode
void
XMLMessage::SetSendUnicode(bool p_unicode)
{
  m_sendUnicode = p_unicode;
  if(p_unicode)
  {
    m_encoding = XMLEncoding::ENC_UTF16;
  }
  else if(m_encoding == XMLEncoding::ENC_UTF16)
  {
    // Reset/Degrade to UTF-8 (Default encoding)
    m_encoding = XMLEncoding::ENC_UTF8;
  }
}

#pragma endregion Operations
