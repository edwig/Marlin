/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: XMLMessage.cpp
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
#include "XMLMessage.h"
#include "XMLParser.h"
#include "XMLRestriction.h"
#include "Namespace.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// Defined in FileBuffer
extern unsigned long g_streaming_limit; // = STREAMING_LIMIT;

#pragma region XMLElement

//////////////////////////////////////////////////////////////////////////
//
// XMLElement
//
//////////////////////////////////////////////////////////////////////////

XMLElement::XMLElement()
{
}

XMLElement::XMLElement(XMLElement* p_parent)
           :m_parent(p_parent)
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
  for(auto& element : source.m_elements)
  {
    XMLElement* param = new XMLElement(*element);
    param->m_parent = this;
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
  for(auto& element : m_elements)
  {
    element->DropReference();
  }
  m_elements.clear();
  // No more restrictions
  m_restriction = nullptr;
}

void XMLElement::AddReference()
{
  InterlockedIncrement(&m_references);
}

void XMLElement::DropReference()
{
  if(InterlockedDecrement(&m_references) <= 0)
  {
    delete this;
  }
}

bool
XMLElement::IsValidName(const XString& p_name)
{
  if(p_name.IsEmpty())
  {
    return false;
  }

  for (int i = p_name.GetLength() - 1; i >= 0; --i)
  {
    char c = p_name[i];
    if (c == '_' ||
     // c == ':' || Formally correct, but gives unwanted results
       (c >= 'A' && c <= 'Z') ||
       (c >= 'a' && c <= 'z'))
    {
      continue;
    }
    if (i == 0 || (c != '-' &&
                   c != '.' &&
                 !(c >= '0' && c <= '9')))
    {
      return false;
    }
  }
  return true;
}

XString
XMLElement::InvalidNameMessage(const XString& p_name)
{
  XString message;
  message.Format("Invalid XML element name: %s\n"
                 "Names must start with a letter or underscore followed by more or hyphen, digit or dot.", p_name.GetString());
  return message;
}

void
XMLElement::SetName(XString p_name)
{
  if (!p_name.IsEmpty() && !IsValidName(p_name))
  {
    throw StdException(InvalidNameMessage(p_name));
  }
  m_name = p_name; 
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
  m_root = new XMLElement();
  m_root->SetType(XDT_String);
  AddReference();
}

// XTOR from another message
XMLMessage::XMLMessage(XMLMessage* p_orig)
{
  // Copy the element chain
  m_root = new XMLElement(*p_orig->m_root);
  // Copy the contents
  m_version             = p_orig->m_version;
  m_encoding            = p_orig->m_encoding;
  m_standalone          = p_orig->m_standalone;
  m_condensed           = p_orig->m_condensed;
  m_whitespace          = p_orig->m_whitespace;
  m_sendBOM             = p_orig->m_sendBOM;
  m_internalError       = p_orig->m_internalError;
  m_internalErrorString = p_orig->m_internalErrorString;

  AddReference();
}

XMLMessage::~XMLMessage()
{
  m_root->DropReference();
}

void
XMLMessage::Reset()
{
  m_root->Reset();
}

void
XMLMessage::SetRoot(XMLElement* p_root)
{
  if(m_root)
  {
    m_root->DropReference();
  }
  m_root = p_root;
  m_root->AddReference();
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
XMLMessage::LoadFile(const XString& p_fileName)
{
  FILE* file = nullptr;
  if(fopen_s(&file,p_fileName,"r") == 0 && file)
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

    // Prepare buffer allocated on the heap
    char* buffer = new char[(size_t) length + 1];

    // Read the buffer
    size_t count = fread_s(buffer,length,1,length,file);
    if(ferror(file) || count > length)
    {
      fclose(file);
      return false;
    }
    buffer[count] = 0;

    // Buffer to string conversion
    XString inhoud(buffer);
    delete[] buffer;

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
XMLMessage::LoadFile(const XString& p_fileName,StringEncoding p_encoding)
{
  m_encoding = p_encoding;
  return LoadFile(p_fileName);
}

// Save to file
bool
XMLMessage::SaveFile(const XString& p_fileName,bool p_withBom /*= false*/)
{
  bool result = false;
  FILE* file = nullptr;
  if(fopen_s(&file,p_fileName,"w") == 0 && file)
  {
    if(p_withBom)
    {
      XString bom = ConstructBOM(m_encoding);
      fwrite(bom.GetString(),bom.GetLength(),1,file);
    }

    XString inhoud = Print();
    // Allow derived classes (SOAP) to encrypt the whole message
    EncryptMessage(inhoud);
    if(fwrite(inhoud.GetString(),inhoud.GetLength(),1,file) == 1)
    {
      result = true;
    }
    // Close and flush the file
    if(fclose(file))
    {
      return false;
    }
  }
  return result;
}

// Save to file in pre-known encoding
bool
XMLMessage::SaveFile(const XString& p_fileName,StringEncoding p_encoding,bool p_withBom /*= false*/)
{
  m_encoding = p_encoding;
  return SaveFile(p_fileName,p_withBom);
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
XMLMessage::ParseMessage(XString& p_message,WhiteSpace p_whiteSpace /* = PRESERVE_WHITESPACE*/)
{
  XMLParser parser(this);
  parser.ParseMessage(p_message,p_whiteSpace);
  m_whitespace = (p_whiteSpace == WhiteSpace::COLLAPSE_WHITESPACE);
}

void
XMLMessage::ParseMessage(XMLParser* p_parser,XString& p_message,WhiteSpace p_whiteSpace /*=PRESERVE_WHITESPACE*/)
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
XMLMessage::ParseForNode(XMLElement* p_node,XString& p_message,WhiteSpace p_whiteSpace /* = PRESERVE_WHITESPACE*/)
{
  XMLParser parser(this);
  parser.ParseForNode(p_node,p_message,p_whiteSpace);
  m_whitespace = (p_whiteSpace == WhiteSpace::COLLAPSE_WHITESPACE);
}

// Print the XML again
XString
XMLMessage::Print()
{
  bool utf8 = (m_encoding == StringEncoding::ENC_UTF8);

  XString message = PrintHeader();

  message += PrintStylesheet();
  message += PrintElements(m_root,utf8);
  if(m_condensed)
  {
    message += "\n";
  }
  return message;
}

XString
XMLMessage::PrintHeader()
{
  XString header("<?xml");

  // Check what we should do
  if(m_version.IsEmpty() && m_encoding == StringEncoding::ENC_Plain && m_standalone.IsEmpty())
  {
    return "";
  }

  // Construct the header
  if(!m_version.IsEmpty())
  {
    header.AppendFormat(" version=\"%s\"",m_version.GetString());
  }
  // Take care of character encoding
  int acp = -1;
  switch(m_encoding)
  {
    case StringEncoding::ENC_Plain:   acp =    -1; break; // Find Active Code Page
    case StringEncoding::ENC_UTF8:    acp = 65001; break; // See ConvertWideString.cpp
    case StringEncoding::ENC_UTF16:   acp =  1200; break; // See ConvertWideString.cpp
    case StringEncoding::ENC_ISO88591:acp = 28591; break; // See ConvertWideString.cpp
    default:                          break;
  }
  header.AppendFormat(" encoding=\"%s\"",CodepageToCharset(acp).GetString());

  // Add standalone?
  if(!m_standalone.IsEmpty())
  {
    header.AppendFormat(" standalone=\"%s\"",m_standalone.GetString());
  }
  // Closing of the header
  header += "?>";
  if(m_condensed == false)
  {
    header += "\n";
  }
  return header;
}

XString
XMLMessage::PrintStylesheet()
{
  XString header;
  if(!m_stylesheetType.IsEmpty() || !m_stylesheet.IsEmpty())
  {
    header = "<?xml-stylesheet";
    if(!m_stylesheetType.IsEmpty())
    {
      header.AppendFormat(" type=\"%s\"",m_stylesheetType.GetString());
    }
    if(!m_stylesheet.IsEmpty())
    {
      header.AppendFormat(" href=\"%s\"",m_stylesheet.GetString());
    }
    header += "?>";
  }
  return header;
}

// Print the elements stack
XString
XMLMessage::PrintElements(XMLElement* p_element
                         ,bool        p_utf8  /*=true*/
                         ,int         p_level /*=0*/)
{
  XString temp;
  XString spaces;
  XString newline;
  XString message;

  // Find indentation
  if(m_condensed == false)
  {
    newline = "\n";
    for(int ind = 0;ind < p_level; ++ind)
    {
      spaces += "  ";
    }
  }

  XString namesp = p_element->GetNamespace();
  XString name   = p_element->GetName();
  XString value  = p_element->GetValue();

  // Check namespace
  if(!namesp.IsEmpty())
  {
    name = namesp + ":" + name;
  }

  // Print domain value restriction of the element
  if(m_printRestiction && p_element->GetRestriction())
  {
    temp     = p_element->GetRestriction()->PrintRestriction(name);
    message += spaces + temp + newline;
  }
  if((p_element->GetType() & WSDL_Mask) & ~(WSDL_Mandatory | WSDL_Sequence))
  {
    message += spaces;
    message += PrintWSDLComment(p_element);
    message += newline;
  }

  // Print by type
  if(p_element->GetType() & XDT_CDATA)
  {
    // CDATA section
    temp.Format("<%s><![CDATA[%s]]>",XMLParser::PrintXmlString(name,p_utf8).GetString(),value.GetString());
    message += spaces + temp;
  }
  else if(value.IsEmpty() && p_element->GetAttributes().size() == 0 && p_element->GetChildren().size() == 0)
  {
    // A 'real' empty node
    temp.Format("<%s />%s",XMLParser::PrintXmlString(name,p_utf8).GetString(),newline.GetString());
    message += spaces + temp;
    return message;
  }
  else
  {
    // Parameter printing with attributes
    temp.Format("<%s",XMLParser::PrintXmlString(name,p_utf8).GetString());
    message += spaces + temp;

    // Print all of our attributes
    for(auto& attrib : p_element->GetAttributes())
    {
      XString attribute = attrib.m_name;

      // Append attribute name
      if(attrib.m_namespace.IsEmpty())
      {
        temp.Format(" %s=",XMLParser::PrintXmlString(attribute,p_utf8).GetString());
      }
      else
      {
        temp.Format(" %s:%s=",attrib.m_namespace.GetString(),XMLParser::PrintXmlString(attribute,p_utf8).GetString());
      }
      message += temp;

      switch(attrib.m_type & XDT_Mask & ~XDT_Type)
      {
        default:                    temp.Format("\"%s\"",attrib.m_value.GetString());
                                    break;
        case XDT_String:            [[fallthrough]];
        case XDT_AnyURI:            [[fallthrough]];
        case XDT_NormalizedString:  temp.Format("\"%s\"",XMLParser::PrintXmlString(attrib.m_value,p_utf8).GetString());
                                    break;
      }
      message += temp;
    }

    // Mandatory type in the xml
    if(p_element->GetType() & XDT_Type)
    {
      temp.Format(" type=\"%s\"",XmlDataTypeToString(p_element->GetType() & XDT_MaskTypes).GetString());
      message += temp;
    }

    // After the attributes, empty value or value
    if(value.IsEmpty() && p_element->GetChildren().empty())
    {
      message += XString("/>") + newline;
      return message;
    }
    else
    {
      // Write value and end of the key
      temp.Format(">%s",XMLParser::PrintXmlString(value,p_utf8).GetString());
      message += temp;
    }
  }

  if(p_element->GetChildren().size())
  {
    message += newline;
    // call recursively
    for(auto& element : p_element->GetChildren())
    {
      message += PrintElements(element,p_utf8,p_level + 1);
    }
    message += spaces;
  }
  // Write ending of parameter name
  temp.Format("</%s>%s",XMLParser::PrintXmlString(name,p_utf8).GetString(),newline.GetString());
  message += temp;

  return message;
}

XString
XMLMessage::PrintWSDLComment(XMLElement* p_element)
{
  XString comment("<!--");
  switch(p_element->GetType() & WSDL_MaskField)
  {
    case WSDL_Mandatory: comment += "Mandatory";    break;
    case WSDL_Optional:  [[fallthrough]];
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

XString 
XMLMessage::PrintJson(bool p_attributes)
{
  XString message = PrintElementsJson(m_root,p_attributes,m_encoding);
  if(m_condensed)
  {
    message += "\n";
  }
  return message;
}

// Print the elements stack
XString
XMLMessage::PrintElementsJson(XMLElement*     p_element
                             ,bool            p_attributes
                             ,StringEncoding  p_encoding /*=XMLEncoding = ENC_UTF8 */
                             ,int             p_level    /* = 0*/)
{
  XString temp;
  XString spaces;
  XString newline;
  XString message;

  // Find indentation
  if(m_condensed == false)
  {
    newline = "\n";
    for(int ind = 0; ind < p_level; ++ind)
    {
      spaces += "  ";
    }
  }

  XString name  = p_element->GetName();
  XString value = p_element->GetValue();

  if(!name.IsEmpty())
  {
    message = spaces + "\""+ name + "\": ";
  }

  // Optional attributes group
  if(p_attributes && !p_element->GetAttributes().empty())
  {
    message += XString("{") + newline;

    for(auto& attrib : p_element->GetAttributes())
    {
      XString attrName  = attrib.m_name;
      XString attrValue = attrib.m_value;
      temp.Format("\"@%s\": \"%s'\"",attrName.GetString(),attrValue.GetString());
      message = spaces + temp + newline;
    }
    message += spaces;
    message += "\"#text\": ";
  }

  // print element value
  switch(p_element->GetType() & XDT_Mask & ~XDT_Type)
  {
    default:                    temp.Format("%s",value.GetString());
                                break;
    case XDT_CDATA:             [[fallthrough]];
    case XDT_String:            [[fallthrough]];
    case XDT_AnyURI:            [[fallthrough]];
    case XDT_NormalizedString:  temp = XMLParser::PrintJsonString(value,m_encoding);
                                break;
  }
  message += temp + newline;

  // Closing of the attributes group
  if(p_attributes && !p_element->GetAttributes().empty())
  {
    message += spaces;
    message += "}";
    message += newline;
  }

  // Print all child elements of this one by recursing
  if(!p_element->GetChildren().empty())
  {
    message += spaces;
    message += "{";
    message += newline;
    for(auto& elem : p_element->GetChildren())
    {
      PrintElementsJson(elem,p_attributes,p_encoding,p_level + 1);
    }
    message += spaces;
    message += "}";
    message += newline;
  }

  return message;
}

// Encrypt the whole message: yielding a new message
void
XMLMessage::EncryptMessage(XString& /*p_message*/)
{
  // Does nothing for now.
  // Only used in derived classes
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
XMLMessage::AddElement(XMLElement* p_base,XString p_name,XmlDataType p_type,XString p_value,bool p_front /*=false*/)
{
  // Removing the namespace from the name
  XString namesp = SplitNamespace(p_name);

  // Check for stupid programmers
  if(p_name.Find(' ') > 0)
  {
    throw StdException("XML Messages with spaces in elementnames are invalid!");
  }

  XmlElementMap& elements = p_base ? p_base->GetChildren() : m_root->GetChildren();
  XMLElement* parent = p_base ? p_base : m_root;
  XMLElement* elem   = new XMLElement(parent);
  elem->SetNamespace(namesp);
  elem->SetName(p_name);
  elem->SetType(p_type);
  elem->SetValue(p_value);

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
XMLMessage::SetElement(XMLElement* p_base,XString p_name,XmlDataType p_type,XString p_value,bool p_front /*=false*/)
{
  XString name(p_name);
  XString namesp = SplitNamespace(name);
  XmlElementMap& elements = p_base ? p_base->GetChildren() : m_root->GetChildren();

  // Finding existing element
  for (auto& element : elements)
  {
    if (element->GetName().Compare(name) == 0)
    {
      // Just setting the values again
      element->SetNamespace(namesp);
      element->SetName(name);
      element->SetType(p_type);
      element->SetValue(p_value);
      return element;
    }
  }
  
  // Create a new node
  return AddElement(p_base,p_name,p_type,p_value,p_front);
}

XMLElement*
XMLMessage::SetElement(XString p_name,XString& p_value)
{
  return SetElement(m_root,p_name,XDT_String,p_value);
}

XMLElement*
XMLMessage::SetElement(XString p_name, const char* p_value)
{
  XString value(p_value);
  return SetElement(m_root,p_name,XDT_String,value);
}

XMLElement*
XMLMessage::SetElement(XString p_name,int p_value)
{
  XString value;
  value.Format("%d",p_value);
  return SetElement(m_root,p_name,XDT_Integer,value);
}

XMLElement*
XMLMessage::SetElement(XString p_name,bool p_value)
{
  XString value;
  value.Format("%s",p_value ? "true" : "false");
  return SetElement(m_root,p_name,XDT_Boolean,value);
}

XMLElement*
XMLMessage::SetElement(XString p_name,double p_value)
{
  XString value;
  value.Format("%G",p_value);
  return SetElement(m_root,p_name,XDT_Double,value);
}

XMLElement*
XMLMessage::SetElement(XMLElement* p_base,XString p_name,XString& p_value)
{
  return SetElement(p_base,p_name,XDT_String,p_value);
}

XMLElement*
XMLMessage::SetElement(XMLElement* p_base,XString p_name,const char* p_value)
{
  XString value(p_value);
  return SetElement(p_base,p_name,XDT_String,p_value);
}

XMLElement*
XMLMessage::SetElement(XMLElement* p_base,XString p_name,int p_value)
{
  XString value;
  value.Format("%d", p_value);
  return SetElement(p_base,p_name,XDT_Integer,value);
}

XMLElement*
XMLMessage::SetElement(XMLElement* p_base,XString p_name,bool p_value)
{
  XString value;
  value.Format("%s",p_value ? "true" : "false");
  return SetElement(p_base,p_name,XDT_Boolean,value);
}

XMLElement*
XMLMessage::SetElement(XMLElement* p_base,XString p_name,double p_value)
{
  XString value;
  value.Format("%G",p_value);
  return SetElement(p_base,p_name,XDT_Double,value);
}

void
XMLMessage::SetElementValue(XMLElement* p_elem,XmlDataType p_type,XString p_value)
{
  if(p_elem)
  {
    p_elem->SetType(p_type);
    p_elem->SetValue(p_value);
  }
}

void
XMLMessage::SetElementOptions(XMLElement* p_elem,XmlDataType p_options)
{
  if(p_elem)
  {
    // Mask-off previous WSDL options
    // All need to be set in one go!!
    p_elem->SetType((p_elem->GetType() & XDT_Mask) | (p_options & WSDL_Mask));
  }
}

// Setting the namespace' name and contract of an element
// Beware that a contract never ends on a '/' or a '\\'
void
XMLMessage::SetElementNamespace(XMLElement* p_elem, XString p_namespace, XString p_contract,bool p_trailingSlash /*=false*/)
{
  XString name = "xmlns";
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
XMLMessage::SetAttribute(XMLElement* p_elem,XString p_name,XString& p_value)
{
  if(p_elem == NULL)
  {
    return NULL;
  }
  XString namesp = SplitNamespace(p_name);

  // Find in attribute map
  for(auto& attrib : p_elem->GetAttributes())
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

  p_elem->GetAttributes().push_back(attrib);
  return &(p_elem->GetAttributes().back());
}

// Set attribute of a element
XMLAttribute*
XMLMessage::SetAttribute(XMLElement* p_elem,XString p_name,const char* p_value)
{
  XString value(p_value);
  return SetAttribute(p_elem,p_name,value);
}

// Set attribute of a element (integer)
XMLAttribute*
XMLMessage::SetAttribute(XMLElement* p_elem,XString p_name,int p_value)
{
  XString value;
  value.Format("%d",p_value);
  XMLAttribute* attrib = SetAttribute(p_elem,p_name,value);
  attrib->m_type = XDT_Integer;
  return attrib;
}

// Set attribute of a element (boolean)
XMLAttribute*
XMLMessage::SetAttribute(XMLElement* p_elem,XString p_name,bool p_value)
{
  XString value;
  value.Format("%s",p_value ? "true" : "false");
  XMLAttribute* attrib = SetAttribute(p_elem,p_name,value);
  attrib->m_type = XDT_Boolean;
  return attrib;
}

// Set attribute of a element (double)
XMLAttribute*
XMLMessage::SetAttribute(XMLElement* p_elem,XString p_name,double p_value)
{
  XString value;
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
XString  
XMLMessage::GetElement(XMLElement* p_elem,XString p_name)
{
  XmlElementMap& map = p_elem ? p_elem->GetChildren() : m_root->GetChildren();

  // Find in the current mapping
  for(unsigned ind = 0; ind < map.size(); ++ind)
  {
    if(map[ind]->GetName().Compare(p_name) == 0)
    {
      return map[ind]->GetValue();
    }
  }
  return "";
}

int      
XMLMessage::GetElementInteger(XMLElement* p_elem,XString p_name)
{
  return atoi(GetElement(p_elem,p_name));
}

bool
XMLMessage::GetElementBoolean(XMLElement* p_elem,XString p_name)
{
  XString value = GetElement(p_elem,p_name);
  if((value.CompareNoCase("true") == 0) ||
     (value.CompareNoCase("yes") == 0))
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
XMLMessage::GetElementDouble(XMLElement* p_elem,XString p_name)
{
  return atof(GetElement(p_elem,p_name));
}

XString
XMLMessage::GetElement(XString p_name)
{
  return GetElement(NULL,p_name);
}

int
XMLMessage::GetElementInteger(XString p_name)
{
  return GetElementInteger(NULL,p_name);
}

bool
XMLMessage::GetElementBoolean(XString p_name)
{
  return GetElementBoolean(NULL,p_name);
}

double
XMLMessage::GetElementDouble(XString p_name)
{
  return GetElementDouble(NULL,p_name);
}

XMLElement*
XMLMessage::GetElementFirstChild(XMLElement* p_elem)
{
  if(p_elem)
  {
    if(p_elem->GetChildren().size())
    {
      return p_elem->GetChildren().front();
    }
  }
  return nullptr;
}

XMLElement*
XMLMessage::GetElementSibling(XMLElement* p_elem)
{
  if(p_elem)
  {
    XMLElement* parent = p_elem->GetParent();
    if(parent)
    {
      for(unsigned ind = 0;ind < parent->GetChildren().size(); ++ind)
      {
        if(parent->GetChildren()[ind] == p_elem)
        {
          // Check for last node
          if(ind < parent->GetChildren().size() - 1)
          {
            return parent->GetChildren()[(size_t)ind + 1];
          }
        }
      }
    }
  }
  return nullptr;
}

XMLElement*
XMLMessage::FindElement(XMLElement* p_base, XString p_name,bool p_recurse /*=true*/)
{
  XString elementName(p_name);
  XString namesp = SplitNamespace(elementName);
  XMLElement* base = p_base ? p_base : m_root;

  if(base->GetName().Compare(elementName) == 0)
  {
    if(namesp.IsEmpty() || base->GetNamespace().Compare(namesp) == 0)
    {
      return base;
    }
  }

  for(auto& element : base->GetChildren())
  {
    if(element->GetName().Compare(elementName) == 0)
    {
      if(namesp.IsEmpty() || element->GetNamespace().Compare(namesp) == 0)
      {
        return element;
      }
    }
  }
  if(p_recurse)
  {
    for(auto& element : base->GetChildren())
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
XMLMessage::FindElement(XString p_name,bool p_recurse /*=true*/)
{
  return FindElement(m_root,p_name,p_recurse);
}

XMLElement*
XMLMessage::FindElementWithAttribute(XMLElement* p_base
                                    ,XString     p_elementName
                                    ,XString     p_attribName
                                    ,XString     p_attribValue /*=""*/
                                    ,bool        p_recurse /*=true*/)
{
  XString namesp = SplitNamespace(p_elementName);
  XMLElement* base = p_base ? p_base : m_root;

  for(auto& element : base->GetChildren())
  {
    if(element->GetName().Compare(p_elementName) == 0)
    {
      if(namesp.IsEmpty() || element->GetNamespace().Compare(namesp) == 0)
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
XString
XMLMessage::GetAttribute(XMLElement* p_elem,XString p_attribName)
{
  if(p_elem)
  {
    for(auto& attrib : p_elem->GetAttributes())
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
XMLMessage::GetAttributeInteger(XMLElement* p_elem,XString p_attribName)
{
  return atoi(GetAttribute(p_elem,p_attribName));
}

// Get the boolean attribute
bool     
XMLMessage::GetAttributeBoolean(XMLElement* p_elem,XString p_attribName)
{
  XString value = GetAttribute(p_elem,p_attribName);
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
XMLMessage::GetAttributeDouble(XMLElement* p_elem,XString p_attribName)
{
  return atof(GetAttribute(p_elem,p_attribName));
}

XMLAttribute*
XMLMessage::FindAttribute(XMLElement* p_elem,XString p_attribName)
{
  for(auto& attrib : p_elem->GetAttributes())
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
XMLMessage::DeleteElement(XMLElement* p_base, XString p_name, int p_instance /*= 0*/)
{
  XmlElementMap& map = p_base ? p_base->GetChildren() : m_root->GetChildren();
  XmlElementMap::iterator it = map.begin();
  int  count = 0;

  do
  {
    bool found = false;
    while(it != map.end())
    {
      if((*it)->GetName().Compare(p_name) == 0)
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
  XmlElementMap& map = p_base ? p_base->GetChildren() : m_root->GetChildren();
  XmlElementMap::iterator it;

  for (it = map.begin(); it != map.end(); ++it)
  {
    if ((*it) == p_element)
    {
      p_element->DropReference();
      map.erase(it);
      return true;
    }
  }
  return false;
}

// Delete exactly this attribute
bool
XMLMessage::DeleteAttribute(XMLElement* p_element,XString p_attribName)
{
  if(p_element == nullptr)
  {
    return false;
  }
  XmlAttribMap::iterator it;
  for(it = p_element->GetAttributes().begin();it != p_element->GetAttributes().end(); ++it)
  {
    if(it->m_name.Compare(p_attribName) == 0)
    {
      p_element->GetAttributes().erase(it);
      return true;
    }
  }
  return false;
}

// Clean up (delete) elements if empty
bool
XMLMessage::CleanUpElement(XMLElement* p_element,bool p_recurse)
{
  // Check element for root or nullptr reference
  if(!p_element || p_element == m_root)
  {
    return false;
  }

  // Possible recurse through child elements
  if(p_recurse && !p_element->GetChildren().empty())
  {
    // Use index number, as size can shrink during the loop!
    for(int num = (int)p_element->GetChildren().size() - 1;num >= 0;--num)
    {
      CleanUpElement(p_element->GetChildren()[num],true);
    }
  }
  if(p_element->GetChildren().empty() && p_element->GetValue().IsEmpty())
  {
    XMLElement* parent = p_element->GetParent();
    return DeleteElement(parent,p_element);
  }
  return false;
}

// Find an element by attribute
// Attribute values must be unique for this method to work
// Preferable: attribute = 'Id', value = unique-identifier-or-number
XMLElement*
XMLMessage::FindElementByAttribute(XString p_attribute, XString p_value)
{
  return FindElementByAttribute(m_root, p_attribute, p_value);
}

XMLElement*
XMLMessage::FindElementByAttribute(XMLElement* p_element, XString p_attribute, XString p_value)
{
  if (p_element)
  {
    XString value = GetAttribute(p_element, p_attribute);
    if (value.CompareNoCase(p_value) == 0)
    {
      return p_element;
    }
    // Find recursive depth-first
    for (auto& element : p_element->GetChildren())
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
XMLMessage::SetEncoding(StringEncoding p_encoding)
{
  m_encoding    = p_encoding;
  m_sendUnicode = (p_encoding == StringEncoding::ENC_UTF16);
}

// Set sending in Unicode
void
XMLMessage::SetSendUnicode(bool p_unicode)
{
  m_sendUnicode = p_unicode;
  if(p_unicode)
  {
    m_encoding = StringEncoding::ENC_UTF16;
  }
  else if(m_encoding == StringEncoding::ENC_UTF16)
  {
    // Reset/Degrade to UTF-8 (Default encoding)
    m_encoding = StringEncoding::ENC_UTF8;
  }
}

#pragma endregion Operations

#pragma region References

// XMLElements can be stored elsewhere. Use the reference mechanism to add/drop references
// With the drop of the last reference, the object WILL destroy itself

void
XMLMessage::AddReference()
{
  InterlockedIncrement(&m_references);
}

void
XMLMessage::DropReference()
{
  if(InterlockedDecrement(&m_references) <= 0)
  {
    delete this;
  }
}

#pragma endregion References
